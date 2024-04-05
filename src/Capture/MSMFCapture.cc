#include "MSMFCapture.h"
#include "Logger.h"

#include <evr.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <ks.h>
#include <ksmedia.h>
#include <string>
#include <sstream>
#include <vector>
#include <ntverp.h>
#include <stdexcept>

#if VER_PRODUCTBUILD > 9600 // Sensor timestamps require WinSDK ver 10 (10.0.15063) or later. see Readme for more info.
#include <atlbase.h>
#define METADATA_ENABLED_WIN
#endif

namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "MSMFCapture";
static const DWORD STREAM_NUMBER = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
static const GUID W10_FORMAT = {FCC('pBAA'), 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
constexpr uint8_t MS_HEADER_SIZE = 40;

MsmfInitializer::MsmfInitializer()
{
    auto ok = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    if (!ok)
    {
        throw std::runtime_error("CoInitializeEx failed");
    }

    ok = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET));
    if (!ok)
    {
        CoUninitialize();
        throw std::runtime_error("init MSMFbackend failed");
    }
}

MsmfInitializer::~MsmfInitializer()
{
    try
    {
        MFShutdown();
        CoUninitialize();
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "uninit MSMFbackend failed");
    }
}

static void ThrowIfFailed(const char* what, HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    std::stringstream err_stream;
    err_stream << what << ". MSMF failed with HResult error: " << std::hex << static_cast<unsigned long>(hr);
    throw std::runtime_error(err_stream.str());
}

#ifdef METADATA_ENABLED_WIN
static void ExtractMetadataBuffer(IMFSample* pSample, std::vector<unsigned char>& result)
{
    static bool md_patch_windows_exist = true;
    result.clear();
    if (!md_patch_windows_exist)
        return;

    try
    {
        CComPtr<IUnknown> spUnknown;
        CComPtr<IMFAttributes> spSample;
        HRESULT hr = S_OK;
        ThrowIfFailed("query sample interface", hr = pSample->QueryInterface(IID_PPV_ARGS(&spSample)));
        ThrowIfFailed("get unknown", spSample->GetUnknown(MFSampleExtension_CaptureMetadata, IID_PPV_ARGS(&spUnknown)));

        CComPtr<IMFAttributes> spMetadata;
        CComPtr<IMFMediaBuffer> spBuffer;
        PKSCAMERA_METADATA_ITEMHEADER pMetadata = nullptr;
        DWORD dwMaxLength = 0;
        DWORD dwCurrentLength = 0;
        ThrowIfFailed("query metadata interface", spUnknown->QueryInterface(IID_PPV_ARGS(&spMetadata)));
        ThrowIfFailed("get unknown",
                      hr = spMetadata->GetUnknown(MF_CAPTURE_METADATA_FRAME_RAWSTREAM, IID_PPV_ARGS(&spBuffer)));
        ThrowIfFailed("spBuffer->Lock()", spBuffer->Lock((BYTE**)&pMetadata, &dwMaxLength, &dwCurrentLength));
        if (nullptr != pMetadata && pMetadata->MetadataId == MetadataId_UsbVideoHeader &&
            dwCurrentLength > MS_HEADER_SIZE)
        {
            auto* md_raw = reinterpret_cast<char*>(pMetadata);
            auto* md_data_start = md_raw + MS_HEADER_SIZE;
            auto* md_data_end = md_raw + dwCurrentLength;
            result.insert(result.end(), md_data_start, md_data_end);
        }
        ThrowIfFailed("spBuffer->Unlock()", spBuffer->Unlock());
    }
    catch (...)
    {
        result.clear();
        md_patch_windows_exist = false;
        LOG_WARNING(
            LOG_TAG,
            "Failed to get metadata from stream. Try running scripts/realsenseid_metadata_win10.ps1 to enable it.");
    }
}
#else
static void ExtractMetadataBuffer(IMFSample* pSample, std::vector<unsigned char>& result)
{
    result.clear();
}
#endif

static bool CreateMediaSource(IMFMediaSource** media_device, IMFAttributes** cap_config, int capture_number)
{
    const char* stage_tag = "init MSMF backend";
    IMFActivate** ppDevices = NULL;
    UINT32 count = 0;

    ThrowIfFailed(stage_tag, MFCreateAttributes(cap_config, 10));
    ThrowIfFailed(
        stage_tag,
        (*cap_config)->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    ThrowIfFailed(stage_tag, MFEnumDeviceSources((*cap_config), &ppDevices, &count));

    if (capture_number < 0 || static_cast<UINT32>(capture_number) >= count)
    {
        return false;
    }

    ThrowIfFailed(stage_tag, ppDevices[capture_number]->ActivateObject(IID_PPV_ARGS(media_device)));

    for (DWORD i = 0; i < count; i++)
    {
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
    return true;
}

CaptureHandle::CaptureHandle(const PreviewConfig& config) : _config(config)
{
    _stream_converter = std::make_unique<StreamConverter>(_config);

    IMFMediaSource* media_device = nullptr;
    IMFAttributes* cap_config = nullptr;
    IMFMediaType* mediaType = nullptr;
    std::exception_ptr last_ex;

    try
    {
        if (!CreateMediaSource(&media_device, &cap_config, _config.cameraNumber))
        {
            throw std::runtime_error("CreateMediaSource() failed");
        }

        ThrowIfFailed("create source reader",
                      MFCreateSourceReaderFromMediaSource(media_device, cap_config, &_video_src));

        StreamAttributes attr = _stream_converter->GetStreamAttributes();
        GUID stream_format = attr.format == MJPEG ? MFVideoFormat_MJPG : W10_FORMAT;

        ThrowIfFailed("create mediatype ", MFCreateMediaType(&mediaType));
        ThrowIfFailed("set mediaType guid", mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        ThrowIfFailed("set mediaType minor type", mediaType->SetGUID(MF_MT_SUBTYPE, stream_format));
        ThrowIfFailed("set size", MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, attr.width, attr.height));
        ThrowIfFailed("set stream ", _video_src->SetCurrentMediaType(0, NULL, mediaType));
    }
    catch (...)
    {
        last_ex = std::current_exception();
    }

    // clean up
    if (mediaType)
        mediaType->Release();
    if (cap_config)
        cap_config->Release();
    if (media_device)
        media_device->Release();

    if (last_ex)
    {
        if (_video_src)
        {
            _video_src->Release();
            _video_src = nullptr;
        }
        std::rethrow_exception(last_ex);
    }
};

CaptureHandle::~CaptureHandle()
{
    if (_video_src)
    {
        _video_src->Flush(STREAM_NUMBER);
        _video_src->Release();
    }
}

bool CaptureHandle::Read(RealSenseID::Image* res)
{
    IMFSample* sample = NULL;
    DWORD streamIndex, flags;
    LONGLONG timestamp;
    DWORD maxsize = 0, cursize = 0;
    buffer frame_buffer;

    auto hr = _video_src->ReadSample(STREAM_NUMBER, 0, &streamIndex, &flags, &timestamp, &sample);
    if (hr != MF_E_NOTACCEPTING) // if not "flush operation is pending"
    {
        ThrowIfFailed("ReadSample()", hr);
    }

    if (sample == nullptr)
    {
        return false; // not yet available. caller should retry soon
    }

    std::exception_ptr last_ex;
    IMFMediaBuffer* media_buf = nullptr;
    bool valid_read = false;

    try
    {
        ThrowIfFailed("ConvertToContiguousBuffer", sample->ConvertToContiguousBuffer(&media_buf));
        ThrowIfFailed("Lock()", media_buf->Lock(&(frame_buffer.data), &maxsize, &cursize));
        frame_buffer.size = static_cast<unsigned int>(cursize);
        ExtractMetadataBuffer(sample, _md_vector);
        buffer metadata_buffer;
        metadata_buffer.data = _md_vector.data();
        metadata_buffer.size = static_cast<int>(_md_vector.size());
        valid_read = _stream_converter->Buffer2Image(res, frame_buffer, metadata_buffer);
    }
    catch (...)
    {
        last_ex = std::current_exception();
    }

    // cleanup
    if (media_buf)
    {
        media_buf->Unlock();
        media_buf->Release();
    }
    sample->Release();

    if (last_ex)
    {
        std::rethrow_exception(last_ex);
    }

    return valid_read;
}

} // namespace Capture
} // namespace RealSenseID
