#include "MSMFCapture.h"
#include "Logger.h"

#include <evr.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <ks.h>
#include <ksmedia.h>
#include <string>
#include <sstream>

#include <ntverp.h>
#if VER_PRODUCTBUILD > 9600 // Sensor timestamps require WinSDK ver 10 (10.0.15063) or later. see Readme for more info.
#include <atlbase.h>
#define METADATA_ENABLED_WIN
#endif

namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "MSMFCapture";
static const DWORD STREAM_NUMBER = MF_SOURCE_READER_FIRST_VIDEO_STREAM;
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
    err_stream << what << "MSMF failed with  HResult error: " << std::hex << static_cast<unsigned long>(hr);
    throw std::runtime_error(err_stream.str());
}

#ifdef METADATA_ENABLED_WIN
void ExtractMetadataBuffer(IMFSample* pSample,buffer& buf)
{
    static bool md_patch_windows_exist = true;
    if (md_patch_windows_exist)
    {
        try
        {
            CComPtr<IUnknown> spUnknown;
            CComPtr<IMFAttributes> spSample;
            HRESULT hr = S_OK;
            ThrowIfFailed("query sample interface", hr = pSample->QueryInterface(IID_PPV_ARGS(&spSample)));
            ThrowIfFailed("get unknown",
                          spSample->GetUnknown(MFSampleExtension_CaptureMetadata, IID_PPV_ARGS(&spUnknown)));

            CComPtr<IMFAttributes> spMetadata;
            CComPtr<IMFMediaBuffer> spBuffer;
            PKSCAMERA_METADATA_ITEMHEADER pMetadata = nullptr;
            DWORD dwMaxLength = 0;
            DWORD dwCurrentLength = 0;
            ThrowIfFailed("query metadata interface", spUnknown->QueryInterface(IID_PPV_ARGS(&spMetadata)));
            ThrowIfFailed("get unknown",
                          hr = spMetadata->GetUnknown(MF_CAPTURE_METADATA_FRAME_RAWSTREAM, IID_PPV_ARGS(&spBuffer)));
            ThrowIfFailed("lock", spBuffer->Lock((BYTE**)&pMetadata, &dwMaxLength, &dwCurrentLength));
            if (nullptr == pMetadata)
                return;
            if (pMetadata->MetadataId != MetadataId_UsbVideoHeader)
                return;
            auto md_raw = reinterpret_cast<byte*>(pMetadata);
            buf.size = dwCurrentLength - MS_HEADER_SIZE;
            buf.data = new byte[buf.size];
            memcpy(buf.data, md_raw + MS_HEADER_SIZE, buf.size);
        }
        catch (...)
        {
            LOG_DEBUG(LOG_TAG, "Failed to get metadata from stream. Try running scripts/realsenseid_metadata_win10.ps1 to enable it.");
            md_patch_windows_exist = false;
        }
    }
    return;
}
#else
void ExtractMetadataBuffer(IMFSample* pSample, buffer& buf)
{
    buf = buffer();
    return;
}
#endif

bool CreateMediaSource(IMFMediaSource** media_device, IMFAttributes** cap_config, int capture_number)
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

        ThrowIfFailed("create mediatype ",MFCreateMediaType(&mediaType));
        ThrowIfFailed("set mediaType guid",mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        ThrowIfFailed("set mediaType minor type", mediaType->SetGUID(MF_MT_SUBTYPE, stream_format));
        ThrowIfFailed("set size", MFSetAttributeSize(mediaType,MF_MT_FRAME_SIZE ,attr.width, attr.height));
        ThrowIfFailed("set stream ", _video_src->SetCurrentMediaType(0, NULL, mediaType));
    }
    catch (const std::exception& ex)
    {
        if (mediaType)
            mediaType->Release();
        if (cap_config)
            cap_config->Release();
        if (media_device)
            media_device->Release();
        if (_video_src)
            _video_src->Release();
        throw ex;
    }
    if (mediaType)
        mediaType->Release();
    if (cap_config)
        cap_config->Release();
    if (media_device)
        media_device->Release();
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
    bool valid_read = false;
    IMFSample* sample = NULL;
    DWORD streamIndex, flags;
    LONGLONG timestamp;
    DWORD maxsize = 0, cursize = 0;
    buffer frame_buffer;
    buffer meta_buffer;

    _video_src->ReadSample(STREAM_NUMBER, 0, &streamIndex, &flags, &timestamp, &sample);

    if (sample)
    {
        ThrowIfFailed("ConvertToContiguousBuffer", sample->ConvertToContiguousBuffer(&_buf));

        _buf->Lock(&(frame_buffer.data), &maxsize, &cursize);
        frame_buffer.size = (unsigned int)cursize;
    
       // extract basic metadata also for non-raw streams. metadata patch need to be installed.
       // metadata for non-raw streams includes timestamps.
       ExtractMetadataBuffer(sample, meta_buffer);

       valid_read = _stream_converter->Buffer2Image(res, frame_buffer, meta_buffer);

        if (meta_buffer.data)
            delete[] meta_buffer.data;
        if (_buf)
        {
            _buf->Unlock();
            _buf->Release();
        }
        sample->Release();
    }
    return valid_read;
}
} // namespace Capture
} // namespace RealSenseID
