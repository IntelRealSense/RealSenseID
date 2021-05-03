#include "MSMFCapture.h"
#include "Logger.h"

#include <evr.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <stdexcept>

#include <string>
#include <sstream>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Strmiids")
#pragma comment(lib, "Mfreadwrite")


namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "MSMFCapture";
static const DWORD STREAM_NUMBER = MF_SOURCE_READER_FIRST_VIDEO_STREAM;
static const GUID W10_FORMAT = {FCC('pBAA'), 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};


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
    IMFMediaSource* media_device = nullptr;
    IMFAttributes* cap_config = nullptr;
    IMFMediaType* mediaType = nullptr;


    try
    {
        ThrowIfFailed("create media source",
                      HRESULT(CreateMediaSource(&media_device, &cap_config, _config.cameraNumber)));
        ThrowIfFailed("create source reader",
                      MFCreateSourceReaderFromMediaSource(media_device, cap_config, &_video_src));

        GUID stream_format = _config.previewMode == PreviewMode::VGA ? MFVideoFormat_YUY2 : W10_FORMAT;

        ThrowIfFailed("create mediatype ", MFCreateMediaType(&mediaType));
        ThrowIfFailed("set mediaType guid", mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        ThrowIfFailed("set mediaType minor type", mediaType->SetGUID(MF_MT_SUBTYPE, stream_format));
        if (_config.previewMode == PreviewMode::VGA)
            ThrowIfFailed("set size", MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, VGA_WIDTH, VGA_HEIGHT));
        ThrowIfFailed("set stream ", _video_src->SetCurrentMediaType(0, NULL, mediaType));

        // save stream attributes
        UINT64 width_height;
        ThrowIfFailed(" get stream attributes ", _video_src->GetCurrentMediaType(STREAM_NUMBER, &mediaType));
        mediaType->GetUINT64(MF_MT_FRAME_SIZE, &width_height);

        _stream_converter.InitStream((UINT32)(width_height >> 32), (UINT32)(width_height), _config.previewMode);
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
    unsigned char* tmpBuffer = NULL;

    _video_src->ReadSample(STREAM_NUMBER, 0, &streamIndex, &flags, &timestamp, &sample);

    if (sample)
    {
        ThrowIfFailed("ConvertToContiguousBuffer", sample->ConvertToContiguousBuffer(&_buf));
        _buf->Lock(&tmpBuffer, &maxsize, &cursize);

        valid_read = _stream_converter.Buffer2Image(res, tmpBuffer, cursize);

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