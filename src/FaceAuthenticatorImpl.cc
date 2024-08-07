// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "FaceAuthenticatorImpl.h"
#include "Logger.h"
#include "PacketManager/Timer.h"
#include "PacketManager/PacketSender.h"
#include "PacketManager/SerialPacket.h"
#include "StatusHelper.h"
#include "RealSenseID/MatcherDefines.h"
#include "RealSenseID/Faceprints.h"
#include "Matcher/Matcher.h"
#include "CommonValues.h"
#include "LicenseChecker/LicenseChecker.h"
#include "string.h"
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <string>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#else
#error "Platform not supported"
#endif //_WIN32

using RealSenseID::FaVectorFlagsEnum;

static const char* LOG_TAG = "FaceAuthenticatorImpl";

namespace RealSenseID
{
static const unsigned int MAX_FACES = 10;
static constexpr unsigned int MAX_UPLOAD_IMG_SIZE = 900 * 1024;
static constexpr size_t LICENSE_KEY_SIZE = 36;

// save callback functions to use in the secure session later
FaceAuthenticatorImpl::FaceAuthenticatorImpl(SignatureCallback* callback) :
#ifdef RSID_SECURE
    _session {[callback](const unsigned char* buffer, const unsigned int bufferLen, unsigned char* outSig) {
                  return callback->Sign(buffer, bufferLen, outSig);
              },
              [callback](const unsigned char* buffer, const unsigned int bufferLen, const unsigned char* sig,
                         const unsigned int sigLen) { return callback->Verify(buffer, bufferLen, sig, sigLen); }}
{
    assert(callback != nullptr);
    if (callback == nullptr)
    {
        throw(std::runtime_error("Got nullptr for SignatureCallback"));
    }
}
#else
    _session {}
{
    (void)callback; // callback not used
}
#endif // RSID_SECURE

Status FaceAuthenticatorImpl::Connect(const SerialConfig& config)
{
    try
    {
        // disconnect if already connected
        _serial.reset();
        PacketManager::SerialConfig serial_config;
        serial_config.port = config.port;

#ifdef _WIN32
        _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
        _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#else
        LOG_ERROR(LOG_TAG, "Serial connection method not supported for OS");
        return Status::Error;
#endif // _WIN32
        return Status::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception during serial connect");
        return Status::Error;
    }
}

void FaceAuthenticatorImpl::Disconnect()
{
    _serial.reset();
}

#ifdef RSID_SECURE
Status FaceAuthenticatorImpl::Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey)
{
    if (!_serial)
    {
        LOG_ERROR(LOG_TAG, "Not connected to a serial port");
        return Status::Error;
    }
    LOG_INFO(LOG_TAG, "Pairing start");

    unsigned char ecdsaSignedHostPubKey[SIGNED_PUBKEY_SIZE];
    ::memset(ecdsaSignedHostPubKey, 0, sizeof(ecdsaSignedHostPubKey));
    ::memcpy(ecdsaSignedHostPubKey, ecdsaHostPubKey, ECC_P256_KEY_SIZE_BYTES);
    ::memcpy(ecdsaSignedHostPubKey + ECC_P256_KEY_SIZE_BYTES, ecdsaHostPubKeySig, ECC_P256_SIG_SIZE_BYTES);

    PacketManager::DataPacket packet {PacketManager::MsgId::HostEcdsaKey, (char*)ecdsaSignedHostPubKey,
                                      sizeof(ecdsaSignedHostPubKey)};

    PacketManager::PacketSender sender {_serial.get()};
    auto status = sender.SendBinary(packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to send ecdsa public key");
        return ToStatus(status);
    }

    status = sender.Recv(packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv device ecdsa public key");
        return ToStatus(status);
    }
    if (packet.header.id != PacketManager::MsgId::DeviceEcdsaKey)
    {
        LOG_ERROR(LOG_TAG, "Mutual authentication failed");
        return Status::SecurityError;
    }

    assert(IsDataPacket(packet));

    ::memcpy(ecdsaDevicePubKey, packet.Data().data, ECC_P256_KEY_SIZE_BYTES);

    DEBUG_SERIAL(LOG_TAG, "Device Pubkey", ecdsaDevicePubKey, ECC_P256_KEY_SIZE_BYTES);
    LOG_INFO(LOG_TAG, "Pairing Ok");
    return Status::Ok;
}

Status FaceAuthenticatorImpl::Unpair()
{
    if (!_serial)
    {
        LOG_ERROR(LOG_TAG, "Not connected to a serial port");
        return Status::Error;
    }
    auto status = _session.Unpair(_serial.get());
    return ToStatus(status);
}
#endif // RSID_SECURE


// return list of faces from given packet
// serialization format:
//   First byte: face count
//   N FaceRect structs (little endian, packed)
static std::vector<FaceRect> GetDetectedFaces(const PacketManager::SerialPacket& packet, unsigned int& ts)
{
    assert(packet.header.id == PacketManager::MsgId::FaceDetected);

    auto* data = packet.payload.message.data_msg.data;
    unsigned int n_faces = static_cast<unsigned int>(data[0]);
    data++;
    const uint32_t* ts_ptr = reinterpret_cast<const uint32_t*>(data);
    ts = *ts_ptr;
    data += sizeof(uint32_t);
    static_assert(MAX_FACES * sizeof(FaceRect) < sizeof(packet.payload.message.data_msg.data),
                  "Not enough space payload for MAX_FACES");

    if (n_faces > MAX_FACES)
    {
        throw std::runtime_error("Got unexpected faces count in response: " + std::to_string(n_faces));
    }

    std::vector<FaceRect> rv;
    rv.reserve(n_faces);
    for (unsigned int i = 0; i < n_faces; i++)
    {
        FaceRect face;
        ::memcpy(&face, data, sizeof(face));
        data += sizeof(face);
        rv.push_back(face);
    }
    return rv;
}

// Do enroll session with the device. Call user's enroll callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
Status FaceAuthenticatorImpl::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    try
    {
        if (!ValidateUserId(user_id))
        {
            return Status::Error;
        }
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            callback.OnResult(ToEnrollStatus(status));
            return ToStatus(status);
        }

        PacketManager::FaPacket fa_packet {PacketManager::MsgId::Enroll, user_id, 0};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            callback.OnResult(ToEnrollStatus(status));
            return ToStatus(status);
        }

        PacketManager::Timer session_timer {CommonValues::enroll_max_timeout};
        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnResult(EnrollStatus::Failure);
                Cancel();
            }

            status = _session.RecvPacket(fa_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                callback.OnResult(ToEnrollStatus(status));
                return ToStatus(status);
            }

            auto msg_id = fa_packet.header.id;

            // handle face detected as data packet
            if (msg_id == PacketManager::MsgId::FaceDetected)
            {
                unsigned int ts;
                auto faces = GetDetectedFaces(fa_packet, ts);
                callback.OnFaceDetected(faces, ts);
                continue; // continue to recv next messages
            }

            auto fa_status = fa_packet.GetStatusCode();
            auto enroll_status = EnrollStatus(fa_status);
            const char* log_enroll_status = Description(enroll_status);

            switch (msg_id)
            {
                // end of transaction
            case (PacketManager::MsgId::Reply):
                LOG_DEBUG(LOG_TAG, "Got Reply: %s", log_enroll_status);
                return Status(fa_status);

            case (PacketManager::MsgId::Result):
                callback.OnResult(enroll_status);
                break;

            case (PacketManager::MsgId::Progress):
                callback.OnProgress((FacePose)fa_status);
                break;

            case (PacketManager::MsgId::Hint):
                callback.OnHint(enroll_status);
                break;

            default:
                LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                callback.OnResult(EnrollStatus::Failure);
                return Status::Error;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnResult(EnrollStatus::Failure);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(EnrollStatus::Failure);
        return Status::Error;
    }
}


Status FaceAuthenticatorImpl::SendImageToDevice(const unsigned char* buffer, unsigned int width, unsigned int height)
{
    if (buffer == nullptr)
    {
        LOG_ERROR(LOG_TAG, "Invalid buffer");
        return Status::Error;
    }

    if (width > 0xFFFF || height > 0xFFFF)
    {
        LOG_ERROR(LOG_TAG, "Invalid width/height");
        return Status::Error;
    }

    uint32_t image_size = ((uint32_t)width * (uint32_t)height) * 3;
    if (image_size == 0 || image_size > MAX_UPLOAD_IMG_SIZE)
    {
        LOG_ERROR(LOG_TAG, "Invalid image size %u", image_size);
        return Status::Error;
    }


    // split to chunks and send to device
    auto constexpr chunk_size = sizeof(PacketManager::DataMessage::data);
    static_assert(chunk_size > 128, "invalid chunk size");

    uint32_t image_chunk_size = (uint32_t)chunk_size - 6;
    uint32_t n_chunks = image_size / image_chunk_size;
    uint32_t n_chunks_mod = image_size % image_chunk_size;
    uint32_t last_chunk_size = image_chunk_size;

    if (n_chunks_mod > 0)
    {
        last_chunk_size = n_chunks_mod;
        n_chunks++;
    }

    auto width_16 = static_cast<uint16_t>(width);
    auto height_16 = static_cast<uint16_t>(height);

    LOG_DEBUG(LOG_TAG, "Sending %d chunks..", n_chunks);
    char chunk[chunk_size];
    size_t total_image_bytes_sent = 0;
    for (uint32_t i = 0; i < n_chunks; i++)
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            return ToStatus(status);
        }

        auto chunk_number = static_cast<uint16_t>(i);
        ::memcpy(&chunk[0], &chunk_number, sizeof(chunk_number));
        ::memcpy(&chunk[2], &width, sizeof(width_16));
        ::memcpy(&chunk[4], &height, sizeof(height_16));

        auto* image_chunk_ptr = &buffer[chunk_number * image_chunk_size];
        auto is_last_chunk = (i == n_chunks - 1);
        uint32_t bytes_to_send = is_last_chunk ? last_chunk_size : image_chunk_size;
        ::memcpy((unsigned char*)&chunk[6], image_chunk_ptr, bytes_to_send);

        LOG_DEBUG(LOG_TAG, "Send chunk %u/%u size=%u", chunk_number + 1, n_chunks, bytes_to_send);
        PacketManager::DataPacket data_packet {PacketManager::MsgId::UploadImage, chunk, chunk_size};
        status = _session.SendPacket(data_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending data packet (chunk %d status %d)", i, (int)status);
            return ToStatus(status);
        }

        total_image_bytes_sent += bytes_to_send;
        assert(total_image_bytes_sent <= image_size);

        // wait for reply
        status = _session.RecvDataPacket(data_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving reply packet (status %d)", (int)status);
            return ToStatus(status);
        }
        LOG_DEBUG(LOG_TAG, "Sent chunk %hu OK. %zu/%u bytes", chunk_number + 1, total_image_bytes_sent, image_size);
    }

    assert(total_image_bytes_sent == image_size);

    return Status::Ok;
}
// Do enroll session with the device using the given bgr24 face image
// split to chunks and send to device as multiple 'e' DataPackets.
//    image_size = Width x Height * 3
//    chunk_size = sizeof(PacketManager::DataMessage::data);
//    image_chunk_size= chunk_size - 6  (6 bytes = [chunkN,W,H])
//    Number of chunks = image_size / image_chunk_size
//    chunk format: [chunk-number (2 bytes)] [width (2 bytes)] [height (2 bytes)] [buffer (chunk size-6)]
//    Wait for ack response ('e' packet)
// Send 'EnrollImage' Fa packet with the user id and return the response to the caller.
EnrollStatus FaceAuthenticatorImpl::EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width,
                                                unsigned int height)
{
    if (!ValidateUserId(user_id))
    {
        return EnrollStatus::Failure;
    }

    Status imageSendingStatus = SendImageToDevice(buffer, width, height);
    if (Status::Ok != imageSendingStatus)
    {
        LOG_ERROR(LOG_TAG, "Error sending the image to the device. status %d", static_cast<int>(imageSendingStatus));
        return EnrollStatus::Failure;
    }

    // Now that the image was uploaded, send the enroll image request
    auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToEnrollStatus(status);
    }

    PacketManager::FaPacket fa_packet {PacketManager::MsgId::EnrollImage, user_id, 0};
    status = _session.SendPacket(fa_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
        return ToEnrollStatus(status);
    }

    status = _session.RecvFaPacket(fa_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
        return ToEnrollStatus(status);
    }

    return EnrollStatus(fa_packet.GetStatusCode());
}

EnrollStatus FaceAuthenticatorImpl::EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer,
                                                                 unsigned int width, unsigned int height,
                                                                 ExtractedFaceprints* faceprints)
{
    if (!ValidateUserId(user_id))
    {
        LOG_ERROR(LOG_TAG, "invalid user id");
        return EnrollStatus::Failure;
    }

    if (nullptr == faceprints)
    {
        LOG_ERROR(LOG_TAG, "the faceprints argument is null");
        return EnrollStatus::Failure;
    }

    Status imageSendingStatus = SendImageToDevice(buffer, width, height);
    if (Status::Ok != imageSendingStatus)
    {
        LOG_ERROR(LOG_TAG, "Error sending the image to the device. status %d", static_cast<int>(imageSendingStatus));
        return EnrollStatus::Failure;
    }

    // Now that the image was uploaded, send the enroll image request
    auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToEnrollStatus(status);
    }

    // Getting the image enrollment result from the device(on success the faceprints are returned)
    PacketManager::FaPacket fa_packet {PacketManager::MsgId::EnrollImageFeatureExtraction, user_id, 0};
    status = _session.SendPacket(fa_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
        return ToEnrollStatus(status);
    }

    status = _session.RecvFaPacket(fa_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
        return ToEnrollStatus(status);
    }

    if (EnrollStatus(fa_packet.GetStatusCode()) != EnrollStatus::Success)
    {
        return EnrollStatus(fa_packet.GetStatusCode());
    }

    // expect features message

    PacketManager::DataPacket data_packet(PacketManager::MsgId::Faceprints);
    status = _session.RecvDataPacket(data_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
        auto enroll_status = ToEnrollStatus(status);
        return enroll_status;
    }

    auto msg_id = data_packet.header.id;
    if (msg_id == PacketManager::MsgId::Faceprints)
    {
        LOG_DEBUG(LOG_TAG, "Got faceprints from device!");
        ExtractedFaceprintsElement* desc = (ExtractedFaceprintsElement*)(data_packet.payload.message.data_msg.data);

        //
        //  read the mask-detector indicator:
        feature_t vecFlags = desc->featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
        feature_t hasMask = (vecFlags == FaVectorFlagsEnum::VecFlagValidWithMask) ? 1 : 0;

        LOG_DEBUG(LOG_TAG, "Enrollment flow :  = %d, hasMask = %d.", vecFlags, hasMask);

        faceprints->data.version = desc->version;
        faceprints->data.featuresType = desc->featuresType;
        faceprints->data.flags = FaOperationFlagsEnum::OpFlagEnrollWithoutMask;

        size_t copySize = sizeof(desc->featuresVector);

        static_assert(sizeof(faceprints->data.featuresVector) == sizeof(desc->featuresVector),
                      "adaptive faceprints (without mask) sizes does not match");
        ::memcpy(faceprints->data.featuresVector, desc->featuresVector, copySize);

        // mark the enrolled vector flags as valid without mask.
        faceprints->data.featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] =
            FaVectorFlagsEnum::VecFlagValidWithoutMask;

        return EnrollStatus::Success;
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Got unexpected message id when expecting faceprints to arrive: %c", (char)msg_id);
        return EnrollStatus::SerialError;
    }    
}

// Do authenticate session with the device. Call user's authenticate callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
Status FaceAuthenticatorImpl::Authenticate(AuthenticationCallback& callback)
{
    try
    {
        auto status = _session.Start(_serial.get());           
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            callback.OnResult(ToAuthStatus(status), nullptr);
            return ToStatus(status);
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::Authenticate};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            callback.OnResult(ToAuthStatus(status), nullptr);
            return ToStatus(status);
        }

        PacketManager::Timer session_timer {CommonValues::auth_max_timeout};
        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnResult(AuthenticateStatus::Forbidden, nullptr);
                Cancel();
            }

            status = _session.RecvPacket(fa_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                callback.OnResult(ToAuthStatus(status), nullptr);
                return ToStatus(status);
            }

            auto msg_id = fa_packet.header.id;

            // handle face detected as data packet
            if (msg_id == PacketManager::MsgId::FaceDetected)
            {
                unsigned int ts;
                auto faces = GetDetectedFaces(fa_packet, ts);
                callback.OnFaceDetected(faces, ts);
                continue; // continue to recv next messages
            }

            auto fa_status = fa_packet.GetStatusCode();
            const char* user_id = fa_packet.GetUserId();
            auto auth_status = AuthenticateStatus(fa_status);
            const char* log_auth_status = Description(auth_status);

            switch (msg_id)
            {
            // end of transaction
            case (PacketManager::MsgId::Reply):
                LOG_DEBUG(LOG_TAG, "Got Reply: %s", log_auth_status);
                return Status(fa_status);

            case (PacketManager::MsgId::Result):
                LOG_DEBUG(LOG_TAG, "Got Result: %s", log_auth_status);
                callback.OnResult(auth_status, user_id);
                break;

            case (PacketManager::MsgId::Hint):
                LOG_DEBUG(LOG_TAG, "Got Hint: %s", log_auth_status);
                callback.OnHint(auth_status);
                break;

            default:
                LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                callback.OnHint(AuthenticateStatus::Failure);
                return Status::Error;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnResult(AuthenticateStatus::Failure, nullptr);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(AuthenticateStatus::Failure, nullptr);
        return Status::Error;
    }
}

// wait for cancel flag while sleeping upto timeout
void FaceAuthenticatorImpl::AuthLoopSleep(std::chrono::milliseconds timeout)
{
    PacketManager::Timer timer {timeout};
    LOG_DEBUG(LOG_TAG, "AuthLoopSleep upto %zu millis", timeout.count());
    // sleep in small interval to stop sleep if we got canceled
    while (!timer.ReachedTimeout() && !_cancel_loop)
    {
        auto sleep_interval = std::chrono::milliseconds {500};
        if (timer.TimeLeft() < sleep_interval)
        {
            sleep_interval = timer.TimeLeft();
            assert(sleep_interval > PacketManager::timeout_t::zero());
        }

        std::this_thread::sleep_for(sleep_interval);
    }
}


// Perform authentication loop. Call user's callbacks in the process.
// Wait for one of the following to happen:
//      * We get 'reply' from device ('Y' , would happen if "cancel" command was sent).
//      * Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//
// Keep delay interval between requests:
//      * 2100ms if no face was found or got error (or 1600ms in secure mode).
//      * 600ms otherwise (or 100ms in secure mode).

// Helper callback handler to deal with sleep intervals
class AuthLoopCallback : public AuthenticationCallback
{
    bool _face_found = false;
    AuthenticationCallback& _user_callback;

public:
    explicit AuthLoopCallback(AuthenticationCallback& user_callback) : _user_callback(user_callback)
    {
    }

    void OnResult(const AuthenticateStatus status, const char* userId) override
    {
        if (status == AuthenticateStatus::NoFaceDetected || status == AuthenticateStatus::DeviceError ||
            status == AuthenticateStatus::SerialError || status == AuthenticateStatus::Failure)
        {
            _face_found = false;
        }
        _user_callback.OnResult(status, userId);
    }

    void OnHint(const AuthenticateStatus hint) override
    {
        _user_callback.OnHint(hint);
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        _face_found = !faces.empty();
        _user_callback.OnFaceDetected(faces, ts);
    }

    bool face_found()
    {
        return _face_found;
    }
};

Status FaceAuthenticatorImpl::AuthenticateLoop(AuthenticationCallback& callback)
{
    _cancel_loop = false;
    do
    {
        AuthLoopCallback clbk_handler {callback};
        auto status = Authenticate(clbk_handler);
        if (status != Status::Ok || _cancel_loop)
        {
            return status; // return from the loop on first error
        }

        auto sleep_for = clbk_handler.face_found() ? _loop_interval_with_face : _loop_interval_no_face;
        AuthLoopSleep(sleep_for);
    } while (!_cancel_loop);

    return Status::Ok;
}

Status FaceAuthenticatorImpl::Cancel()
{
    try
    {
        _cancel_loop = true;
        // Send cancel packet.
        _session.Cancel();
        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
} // namespace RealSenseID

Status FaceAuthenticatorImpl::RemoveUser(const char* user_id)
{
    try
    {
        if (!ValidateUserId(user_id))
        {
            return Status::Error;
        }
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            return ToStatus(status);
        }

        PacketManager::FaPacket fa_packet {PacketManager::MsgId::RemoveUser, user_id, 0};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            return ToStatus(status);
        }

        status = _session.RecvFaPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
            return ToStatus(status);
        }

        return Status(fa_packet.GetStatusCode());
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
}

Status FaceAuthenticatorImpl::RemoveAll()
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            return ToStatus(status);
        }

        PacketManager::FaPacket fa_packet {PacketManager::MsgId::RemoveAllUsers};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            return ToStatus(status);
        }

        status = _session.RecvFaPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
            return ToStatus(status);
        }

        return Status(fa_packet.GetStatusCode());
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
}

Status FaceAuthenticatorImpl::SetDeviceConfig(const DeviceConfig& device_config)
{
    DeviceConfig prev_device_config;
    auto query_status = QueryDeviceConfig(prev_device_config);
    if (query_status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "QueryDeviceConfig failed");
        return query_status;
    }

    auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToStatus(status);
    }
    
    char settings[7];
    settings[0] = static_cast<char>(device_config.camera_rotation);
    settings[1] = static_cast<char>(device_config.security_level);
    settings[2] = static_cast<char>(device_config.algo_flow);
    settings[3] = static_cast<char>(device_config.face_selection_policy);
    settings[4] = static_cast<char>(device_config.dump_mode);
    settings[5] = static_cast<char>(device_config.matcher_confidence_level);
    settings[6] = static_cast<char>(device_config.max_spoofs);

    PacketManager::DataPacket data_packet {PacketManager::MsgId::SetDeviceConfig, settings, sizeof(settings)};

    status = _session.SendPacket(data_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
    }

    PacketManager::DataPacket data_packet_reply {PacketManager::MsgId::SetDeviceConfig};
    status = _session.RecvDataPacket(data_packet_reply);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving reply packet (status %d)", (int)status);
        return ToStatus(status);
    }

    if (data_packet_reply.header.id == PacketManager::MsgId::Status)
    {
        auto reply_status = data_packet_reply.Data().data[0];
        auto real_status = Status(reply_status);
        const char* log_real_status = Description(real_status);
        LOG_ERROR(LOG_TAG, "Received Reply with status: %s", log_real_status);
        return real_status;
    }

    if (data_packet_reply.header.id != PacketManager::MsgId::SetDeviceConfig)
    {
        LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", data_packet_reply.header.id);
        return Status::Error;
    }

    static_assert(sizeof(data_packet_reply.payload.message.data_msg.data) >= 5, "Invalid data message");
    // make sure we succeeeded - the response shoud contain the same values that we sent

    if (::memcmp(data_packet_reply.Data().data, settings, sizeof(settings)))
    {
        LOG_ERROR(LOG_TAG, "Settings at device were not applied");
        return Status::Error;
    }
    // convert internal status to api's serial status and return
    return ToStatus(status);
}

Status FaceAuthenticatorImpl::QueryDeviceConfig(DeviceConfig& device_config)
{
    auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToStatus(status);
    }
    PacketManager::DataPacket data_packet {PacketManager::MsgId::QueryDeviceConfig, NULL, 0};
    status = _session.SendPacket(data_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
    }

    PacketManager::DataPacket data_packet_reply {PacketManager::MsgId::QueryDeviceConfig};
    status = _session.RecvDataPacket(data_packet_reply);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
        return ToStatus(status);
    }

    if (data_packet_reply.header.id != PacketManager::MsgId::QueryDeviceConfig)
    {
        LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", data_packet_reply.header.id);
        return Status::Error;
    }

    static_assert(sizeof(data_packet_reply.payload.message.data_msg.data) >= 7, "data size too small");

    device_config.camera_rotation =
        static_cast<DeviceConfig::CameraRotation>(data_packet_reply.payload.message.data_msg.data[0]);

    device_config.security_level =
        static_cast<DeviceConfig::SecurityLevel>(data_packet_reply.payload.message.data_msg.data[1]);

    device_config.algo_flow = static_cast<DeviceConfig::AlgoFlow>(data_packet_reply.payload.message.data_msg.data[2]);

    device_config.face_selection_policy =
        static_cast<DeviceConfig::FaceSelectionPolicy>(data_packet_reply.payload.message.data_msg.data[3]);

    device_config.dump_mode = static_cast<DeviceConfig::DumpMode>(data_packet_reply.payload.message.data_msg.data[4]);

    device_config.matcher_confidence_level =
        static_cast<DeviceConfig::MatcherConfidenceLevel>(data_packet_reply.payload.message.data_msg.data[5]);

    device_config.max_spoofs= static_cast<unsigned char>(data_packet_reply.payload.message.data_msg.data[6]);

    // convert internal status to api's serial status and return
    return ToStatus(status);
}

Status FaceAuthenticatorImpl::QueryUserIds(char** user_ids, unsigned int& number_of_users)
{
    unsigned int retrieved_user_count = 0;
    constexpr unsigned int chunk_size = 5;

    if (user_ids == nullptr || number_of_users == 0)
    {
        LOG_ERROR(LOG_TAG, "QueryUserIds: Got invalid params (nullptr or zero)");
        number_of_users = 0;
        return Status::Error;
    }

    try
    {
        unsigned int arrived_users = 0;

        for (unsigned int i = 0; i < number_of_users && retrieved_user_count < number_of_users; i += arrived_users)
        {
            LOG_DEBUG(LOG_TAG, "Get userids.  So far:%u", i);
            auto status = _session.Start(_serial.get());
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                number_of_users = 0;
                return ToStatus(status);
            }
            // retrieve next NUMBER_OF_USERS_TO_QUERY users
            unsigned int settings[2];
            settings[0] = retrieved_user_count;
            settings[1] = chunk_size;

            PacketManager::DataPacket query_users_packet {PacketManager::MsgId::GetUserIds, (char*)settings,
                                                          sizeof(settings)};
            status = _session.SendPacket(query_users_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", static_cast<int>(status));
                number_of_users = 0;
                return ToStatus(status);
            }

            PacketManager::DataPacket reply_packet {PacketManager::MsgId::GetUserIds};
            status = _session.RecvDataPacket(reply_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", static_cast<int>(status));
                number_of_users = 0;
                return ToStatus(status);
            }

            if (reply_packet.header.id != PacketManager::MsgId::GetUserIds)
            {
                LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", reply_packet.header.id);
                number_of_users = 0;
                return Status::Error;
            }

            // get number of users from the response
            const char* data = reply_packet.Data().data;
            ::memcpy(&arrived_users, data, sizeof(unsigned int));
            if (arrived_users == 0)
            {
                break;
            }

            // extract user ids from the returned chunk. each user id is zero delimited c string.
            for (size_t j = 0, cur_pos = sizeof(unsigned int); j < arrived_users; j++)
            {
                if (retrieved_user_count >= number_of_users)
                {
                    break;
                }
                char* target = user_ids[retrieved_user_count];
                ::strncpy(target, &data[cur_pos], PacketManager::MaxUserIdSize);
                target[PacketManager::MaxUserIdSize] = '\0';
                cur_pos += ::strlen(target) + 1;
                retrieved_user_count++;
            }
        }

        assert(retrieved_user_count <= number_of_users);
        number_of_users = retrieved_user_count;

        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        number_of_users = 0;
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        number_of_users = 0;
        return Status::Error;
    }
}

Status FaceAuthenticatorImpl::QueryNumberOfUsers(unsigned int& number_of_users)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            number_of_users = 0;
            return ToStatus(status);
        }
        PacketManager::DataPacket get_nusers_packet {PacketManager::MsgId::GetNumberOfUsers};
        status = _session.SendPacket(get_nusers_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
            number_of_users = 0;
            return ToStatus(status);
        }

        status = _session.RecvDataPacket(get_nusers_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
            number_of_users = 0;
            return ToStatus(status);
        }

        if (get_nusers_packet.header.id != PacketManager::MsgId::GetNumberOfUsers)
        {
            LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", get_nusers_packet.header.id);
            number_of_users = 0;
            return Status::Error;
        }

        uint32_t serialized_n_users = 0;
        ::memcpy(&serialized_n_users, &get_nusers_packet.payload.message.data_msg.data[0], sizeof(serialized_n_users));
        number_of_users = static_cast<unsigned int>(serialized_n_users);

        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        number_of_users = 0;
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        number_of_users = 0;
        return Status::Error;
    }
}

Status FaceAuthenticatorImpl::Standby()
{
    LOG_ERROR(LOG_TAG, "Standby is not supported");
    return Status::Error;
}

Status FaceAuthenticatorImpl::Unlock()
{    
     auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));     
        return ToStatus(status);
    }
    PacketManager::FaPacket fa_packet {PacketManager::MsgId::Unlock, nullptr, '0'};
    status = _session.SendPacket(fa_packet);
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);                
    }
    return ToStatus(status);
}

// Do faceprints extraction using enrollment flow, on the device.
// If faceprints extraction was successful, the device will send a MsgId::Result with a Success value,
// then the host listens for a DataPacket which contains Faceprints from the device, and finally a MsgId::Reply to
// complete the operation. Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
Status FaceAuthenticatorImpl::ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            auto enroll_status = ToEnrollStatus(status);
            callback.OnHint(enroll_status);
            return ToStatus(status);
        }

        PacketManager::FaPacket fa_packet {PacketManager::MsgId::EnrollFaceprintsExtraction, nullptr, '0'};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            auto enroll_status = ToEnrollStatus(status);
            callback.OnHint(enroll_status);
            return ToStatus(status);
        }

        PacketManager::Timer session_timer {CommonValues::enroll_max_timeout};

        bool faceprints_extraction_completed_on_device = false, received_faceprints_in_host = false;

        // mask-detector
        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnResult(EnrollStatus::Failure, nullptr);
                Cancel();
            }

            if (faceprints_extraction_completed_on_device && !received_faceprints_in_host)
            {
                PacketManager::DataPacket data_packet(PacketManager::MsgId::Faceprints);
                status = _session.RecvDataPacket(data_packet);
                if (status != PacketManager::SerialStatus::Ok)
                {
                    LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                    auto enroll_status = ToEnrollStatus(status);
                    callback.OnHint(enroll_status);
                    return ToStatus(status);
                }

                auto msg_id = data_packet.header.id;

                if (msg_id == PacketManager::MsgId::Faceprints)
                {
                    LOG_DEBUG(LOG_TAG, "Got faceprints from device!");
                    ExtractedFaceprintsElement* desc =
                        (ExtractedFaceprintsElement*)(data_packet.payload.message.data_msg.data);

                    //
                    //  read the mask-detector indicator:
                    feature_t vecFlags = desc->featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
                    feature_t hasMask = (vecFlags == FaVectorFlagsEnum::VecFlagValidWithMask) ? 1 : 0;

                    LOG_DEBUG(LOG_TAG, "Enrollment flow :  = %d, hasMask = %d.", vecFlags, hasMask);

                    // set all the members of the enrolled faceprints before it is inserted into the DB.
                    ExtractedFaceprints faceprints;

                    faceprints.data.version = desc->version;
                    faceprints.data.featuresType = desc->featuresType;
                    faceprints.data.flags = FaOperationFlagsEnum::OpFlagEnrollWithoutMask;

                    size_t copySize = sizeof(desc->featuresVector);

                    static_assert(sizeof(faceprints.data.featuresVector) == sizeof(desc->featuresVector),
                                  "adaptive faceprints (without mask) sizes does not match");
                    ::memcpy(faceprints.data.featuresVector, desc->featuresVector, copySize);

                    // mark the enrolled vector flags as valid without mask.
                    faceprints.data.featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] =
                        FaVectorFlagsEnum::VecFlagValidWithoutMask;

                    // received enrollment faceprints must be without mask detection!
                    // assert(hasMask == 0);

                    received_faceprints_in_host = true;

                    callback.OnResult(EnrollStatus::Success, &faceprints);

                    continue;
                }
                else
                {
                    LOG_ERROR(LOG_TAG, "Got unexpected message id when expecting faceprints to arrive: %c",
                              (char)msg_id);

                    return Status::Error;
                }
            }

            status = _session.RecvPacket(fa_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                auto enroll_status = ToEnrollStatus(status);
                callback.OnHint(enroll_status);
                return Status::SerialError;
            }

            auto msg_id = fa_packet.header.id;

            // handle face detected as data packet
            if (msg_id == PacketManager::MsgId::FaceDetected)
            {
                unsigned int ts;
                auto faces = GetDetectedFaces(fa_packet, ts);
                callback.OnFaceDetected(faces, ts);
                continue; // continue to recv next messages
            }

            auto fa_status = fa_packet.GetStatusCode();
            auto enroll_status = EnrollStatus(fa_status);
            const char* log_enroll_status = Description(enroll_status);

            switch (msg_id)
            {
            case (PacketManager::MsgId::Reply):
                LOG_DEBUG(LOG_TAG, "Got Reply: %s", log_enroll_status);
                return Status(fa_status);

            case (PacketManager::MsgId::Result):

                if (enroll_status == EnrollStatus::Success)
                {
                    LOG_DEBUG(LOG_TAG,
                              "Faceprints extraction succeeded on device, ready to receive faceprints in host ...");
                    faceprints_extraction_completed_on_device = true;
                }
                else
                {
                    callback.OnResult(enroll_status, nullptr);
                }
                break;

            case (PacketManager::MsgId::Progress):
                callback.OnProgress((FacePose)fa_status);
                break;

            case (PacketManager::MsgId::Hint):
                callback.OnHint(enroll_status);
                break;

            default:
                callback.OnResult(EnrollStatus::DeviceError, nullptr);
                return Status::Error;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnResult(EnrollStatus::Failure, nullptr);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(EnrollStatus::Failure, nullptr);
        return Status::Error;
    }
}

// Do faceprints extraction using authentication flow on the device. Call user's authenticate callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
Status FaceAuthenticatorImpl::ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback)
{
    try
    {
        auto status = _session.Start(_serial.get());            
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            auto auth_status = ToAuthStatus(status);
            callback.OnResult(auth_status, nullptr);
            return ToStatus(status);
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::AuthenticateFaceprintsExtraction};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            auto auth_status = ToAuthStatus(status);
            callback.OnResult(auth_status, nullptr);
            return ToStatus(status);
        }
        PacketManager::Timer session_timer {CommonValues::auth_max_timeout};
        bool faceprints_extraction_completed_on_device = false, received_faceprints_in_host = false;
        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnResult(AuthenticateStatus::Failure, nullptr);
                Cancel();
            }

            if (faceprints_extraction_completed_on_device && !received_faceprints_in_host)
            {
                PacketManager::DataPacket data_packet(PacketManager::MsgId::Faceprints);
                status = _session.RecvDataPacket(data_packet);
                if (status != PacketManager::SerialStatus::Ok)
                {
                    LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                    auto auth_status = ToAuthStatus(status);
                    callback.OnResult(auth_status, nullptr);
                    return ToStatus(status);
                }

                auto msg_id = data_packet.header.id;

                if (msg_id == PacketManager::MsgId::Faceprints)
                {
                    LOG_DEBUG(LOG_TAG, "Got faceprints from device!");
                    ExtractedFaceprintsElement* received_desc =
                        (ExtractedFaceprintsElement*)(data_packet.payload.message.data_msg.data);

                    // note that it's the withoutMask[] vector that was written during authentication.
                    //
                    // mask-detector indicator:
                    // we allow authentication with mask (if low security mode), so just provide LOG msg here.
                    feature_t vecFlags = received_desc->featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
                    feature_t hasMask = (vecFlags == FaVectorFlagsEnum::VecFlagValidWithMask) ? 1 : 0;

                    LOG_DEBUG(LOG_TAG, "Authentication flow : vecFlags = %d, hasMask = %d.", vecFlags, hasMask);

                    received_faceprints_in_host = true;

                    // during authentication, only few metadata members matters and the adaptiveDescriptorWithoutMask[]
                    // vector.
                    ExtractedFaceprints faceprints;
                    faceprints.data.version = received_desc->version;
                    faceprints.data.featuresType = received_desc->featuresType;
                    faceprints.data.flags = (hasMask == 0)
                                                ? static_cast<int>((FaOperationFlagsEnum::OpFlagAuthWithoutMask))
                                                : static_cast<int>((FaOperationFlagsEnum::OpFlagAuthWithMask));

                    size_t copySize = sizeof(received_desc->featuresVector);
                    static_assert(sizeof(faceprints.data.featuresVector) == sizeof(received_desc->featuresVector),
                                  "adaptive faceprints (without mask) sizes does not match");
                    ::memcpy(faceprints.data.featuresVector, received_desc->featuresVector, copySize);

                    callback.OnResult(AuthenticateStatus::Success, &faceprints);
                    continue;
                }
                else
                {
                    LOG_ERROR(LOG_TAG, "Got unexpected message id when expecting faceprints to arrive: %c",
                              (char)msg_id);
                    return RealSenseID::Status::Error;
                }
            }

            status = _session.RecvPacket(fa_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                auto auth_status = ToAuthStatus(status);
                callback.OnHint(auth_status);
                return Status::SerialError;
            }

            auto msg_id = fa_packet.header.id;

            // handle face detected as data packet
            if (msg_id == PacketManager::MsgId::FaceDetected)
            {
                unsigned int ts;
                auto faces = GetDetectedFaces(fa_packet, ts);
                callback.OnFaceDetected(faces, ts);
                continue; // continue to recv next messages
            }

            auto fa_status = fa_packet.GetStatusCode();
            auto auth_status = AuthenticateStatus(fa_status);
            const char* log_auth_status = Description(auth_status);

            switch (msg_id)
            {
            case (PacketManager::MsgId::Reply):
                LOG_DEBUG(LOG_TAG, "Got Reply: %s", log_auth_status);
                return Status(fa_status);

            case (PacketManager::MsgId::Result):
                if (auth_status == AuthenticateStatus::Success)
                {
                    LOG_DEBUG(LOG_TAG,
                              "Faceprints extraction succeeded on device, ready to receive faceprints in host ...");
                    faceprints_extraction_completed_on_device = true;
                    received_faceprints_in_host = false;
                }
                else
                    callback.OnResult(auth_status, nullptr);
                break;

            case (PacketManager::MsgId::Hint):
                callback.OnHint(auth_status);
                break;

            default:
                callback.OnHint(AuthenticateStatus::DeviceError);
                return RealSenseID::Status::Error;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnHint(AuthenticateStatus::Failure);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(AuthenticateStatus::Failure, nullptr);
        return Status::Error;
    }
}

// Perform faceprints extraction loop. Call user's callbacks in the process.
// Wait for one of the following to happen:
//      * We get 'reply' from device ('Y' , would happen if "cancel" command was sent).
//      * Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//
// Keep delay interval between requests:
//      * 2100ms if no face was found or got error (or 1600ms in secure mode).
//      * 600ms otherwise (or 100ms in secure mode).

// Helper callback handler to deal with sleep intervals
class FaceprintsLoopCallback : public AuthFaceprintsExtractionCallback
{
    bool _face_found = false;
    AuthFaceprintsExtractionCallback& _user_callback;

public:
    explicit FaceprintsLoopCallback(AuthFaceprintsExtractionCallback& user_callback) : _user_callback(user_callback)
    {
    }

    void OnResult(const AuthenticateStatus status, const ExtractedFaceprints* faceprints) override
    {
        if (status == AuthenticateStatus::NoFaceDetected || status == AuthenticateStatus::DeviceError ||
            status == AuthenticateStatus::SerialError || status == AuthenticateStatus::Failure)
        {
            _face_found = false;
        }
        _user_callback.OnResult(status, faceprints);
    }

    void OnHint(const AuthenticateStatus hint) override
    {
        _user_callback.OnHint(hint);
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        _face_found = !faces.empty();
        _user_callback.OnFaceDetected(faces, ts);
    }

    bool face_found()
    {
        return _face_found;
    }
};

Status FaceAuthenticatorImpl::ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback)
{
    _cancel_loop = false;
    do
    {
        FaceprintsLoopCallback clbk_handler {callback};
        auto status = ExtractFaceprintsForAuth(clbk_handler);
        if (status != Status::Ok || _cancel_loop)
        {
            return status; // return from the loop on first error
        }

        auto sleep_for = clbk_handler.face_found() ? _loop_interval_with_face : _loop_interval_no_face;
        AuthLoopSleep(sleep_for);
    } while (!_cancel_loop);

    return Status::Ok;
}

MatchResultHost FaceAuthenticatorImpl::MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints,
                                                       Faceprints& updated_faceprints,
                                                       ThresholdsConfidenceEnum matcher_confidence_level)
{
    MatchResultHost finalResult;

    auto result =
        Matcher::MatchFaceprints(new_faceprints, existing_faceprints, updated_faceprints, matcher_confidence_level);
    finalResult.success = result.success;
    finalResult.should_update = result.should_update;
    finalResult.score = result.score;

    return finalResult;
}

// Validate given user id.
// Return true if valid, false otherwise.
bool FaceAuthenticatorImpl::ValidateUserId(const char* user_id)
{
    if (user_id == nullptr)
    {
        LOG_ERROR(LOG_TAG, "Invalid user id: nullptr");
        return false;
    }

    auto user_id_len = ::strlen(user_id);
    bool is_valid = user_id_len > 0 && user_id_len <= PacketManager::MaxUserIdSize;
    if (!is_valid)
    {
        LOG_ERROR(LOG_TAG, "Invalid user id length. Valid size: 1 - %zu", PacketManager::MaxUserIdSize);
    }
    return is_valid;
}

Status FaceAuthenticatorImpl::GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users)
{
    auto status = _session.Start(_serial.get());
    bool all_is_well = true;
    PacketManager::SerialStatus bad_status = PacketManager::SerialStatus::Ok;
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToStatus(status);
    }
    QueryNumberOfUsers(num_of_users);
    for (uint16_t i = 0; i < num_of_users; i++)
    {
        try
        {
            PacketManager::DataPacket get_features_packet {PacketManager::MsgId::GetUserFeatures, (char*)&i, sizeof(i)};
            status = _session.SendPacket(get_features_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                all_is_well = false;
                bad_status = status;
                continue;
            }
            PacketManager::DataPacket get_features_return_packet {PacketManager::MsgId::GetUserFeatures};
            status = _session.RecvDataPacket(get_features_return_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                all_is_well = false;
                bad_status = status;
                continue;
            }
            if (get_features_return_packet.header.id == PacketManager::MsgId::GetUserFeatures)
            {
                LOG_DEBUG(LOG_TAG, "Got faceprints from device!");
                DBFaceprintsElement* desc =
                    (DBFaceprintsElement*)(get_features_return_packet.payload.message.data_msg.data);

                user_features[i].data.version = desc->version;
                user_features[i].data.featuresType = (FaceprintsTypeEnum)(desc->featuresType);

                static_assert(sizeof(user_features[i].data.adaptiveDescriptorWithoutMask) ==
                                  sizeof(desc->adaptiveDescriptorWithoutMask),
                              "adaptive faceprints sizes (without mask) does not match");
                ::memcpy(user_features[i].data.adaptiveDescriptorWithoutMask, desc->adaptiveDescriptorWithoutMask,
                         sizeof(desc->adaptiveDescriptorWithoutMask));

                static_assert(sizeof(user_features[i].data.adaptiveDescriptorWithMask) ==
                                  sizeof(desc->adaptiveDescriptorWithMask),
                              "adaptive faceprints sizes (with mask) does not match");
                ::memcpy(user_features[i].data.adaptiveDescriptorWithMask, desc->adaptiveDescriptorWithMask,
                         sizeof(desc->adaptiveDescriptorWithMask));

                static_assert(sizeof(user_features[i].data.enrollmentDescriptor) == sizeof(desc->enrollmentDescriptor),
                              "enrollment faceprints sizes does not match");
                ::memcpy(user_features[i].data.enrollmentDescriptor, desc->enrollmentDescriptor,
                         sizeof(desc->enrollmentDescriptor));
            }
            else
            {
                LOG_ERROR(LOG_TAG, "Got unexpected message id when expecting faceprints to arrive: %c",
                          (char)get_features_packet.header.id);
                all_is_well = false;
                bad_status = status;
                continue;
            }
        }
        catch (std::exception& ex)
        {
            LOG_EXCEPTION(LOG_TAG, ex);
            all_is_well = false;
            bad_status = status;
            continue;
        }
        catch (...)
        {
            LOG_ERROR(LOG_TAG, "Unknown exception");
            all_is_well = false;
            bad_status = status;
            continue;
        }
    }
    return all_is_well ? Status::Ok : ToStatus(bad_status);
}

Status FaceAuthenticatorImpl::SetUsersFaceprints(UserFaceprints_t* user_features, unsigned int num_of_users)
{
    bool all_users_set = true;
    auto status = _session.Start(_serial.get());
    if (status != PacketManager::SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToStatus(status);
    }
    for (unsigned int i = 0; i < num_of_users; i++)
    {
        try
        {
            UserFaceprints_t& user_desc = user_features[i];
            const char* user_id = user_desc.user_id;
            if (!ValidateUserId(user_id))
            {
                return Status::Error;
            }
            char buffer[sizeof(DBFaceprintsElement) + PacketManager::MaxUserIdSize + 1] = {0};
            strncpy(buffer, user_id, PacketManager::MaxUserIdSize + 1);
            size_t offset = PacketManager::MaxUserIdSize + 1;
            DBFaceprintsElement* desc = (DBFaceprintsElement*)&user_desc.faceprints;
            memcpy(buffer + offset, (char*)desc, sizeof(*desc));
            offset += sizeof(*desc);
            PacketManager::DataPacket data_packet {PacketManager::MsgId::SetUserFeatures, buffer, offset};

            status = _session.SendPacket(data_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                all_users_set = false;
                continue;
            }

            status = _session.RecvDataPacket(data_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                all_users_set = false;
                continue;
            }
            if (data_packet.header.id != PacketManager::MsgId::SetUserFeatures)
            {
                LOG_ERROR(LOG_TAG, "Error updating/adding user to DB: %d", (int)status);
                all_users_set = false;
                continue;
            }
        }
        catch (std::exception& ex)
        {
            LOG_EXCEPTION(LOG_TAG, ex);
            all_users_set = false;
            continue;
        }
        catch (...)
        {
            LOG_ERROR(LOG_TAG, "Unknown exception");
            all_users_set = false;
            continue;
        }
    }
    return all_users_set ? Status::Ok : Status::Error;
}


Status FaceAuthenticatorImpl::SetLicenseKey(const std::string& license_key)
{
    if (!license_key.empty() && license_key.length() != LICENSE_KEY_SIZE)
    {
        LOG_ERROR(LOG_TAG, "SetLicenseKey(): Invalid license key size %zu. Expected: %zu", license_key.length(), LICENSE_KEY_SIZE);
        return Status::Error;
    }
    return LicenseUtils::GetInstance().SetLicenseKey(license_key, false).IsOk() ? Status::Ok : Status::Error;
}

std::string FaceAuthenticatorImpl::GetLicenseKey()
{
    std::string key;
    auto result = LicenseUtils::GetInstance().GetLicenseKey(key);
    if (!result.IsOk())
    {
        return std::string {};
    }

    if (!key.empty() && key.length() != LICENSE_KEY_SIZE)
    {
        LOG_ERROR(LOG_TAG, "GetLicenseKey(): Invalid license key size %zu. Expected: %zu", key.length(), LICENSE_KEY_SIZE);
        return std::string {};
    }
    return key; 
}

// Perform licesnse provision session with the device and the cloud based license server
Status FaceAuthenticatorImpl::ProvideLicense()
{
    using namespace PacketManager;    
   
    try
    {
        LOG_INFO(LOG_TAG, "Start ProvideLicense()");       
        auto status = _session.Start(_serial.get());
        if (status != SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));        
            return ToStatus(status);
        }

        DataPacket data_packet {MsgId::LicenseVerificationStart};
        status = _session.SendPacket(data_packet);
        if (status != SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending packet (status %d)", static_cast<int>(status));            
            return ToStatus(status);
        }
                
        status = _session.RecvDataPacket(data_packet);
        if (status != SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving license packet (status %d)", static_cast<int>(status));
            return ToStatus(status);
        }

        auto msg_id = data_packet.header.id;        
        if (MsgId::LicenseVerificationRequest != msg_id)
        {
            LOG_ERROR(LOG_TAG, "Got unexpected message id %d", static_cast<int>(msg_id));
            return Status::Error;
        }

        auto* data = reinterpret_cast<const unsigned char*>(data_packet.Data().data);
        unsigned char response[LICENSE_VERIFICATION_RES_SIZE + LICENSE_SIGNATURE_SIZE] = {0};
        int license_type = 0;                
        
        auto res = LicenseChecker::GetInstance().CheckLicense(data, response, license_type);

        if (res != LicenseCheckStatus::SUCCESS)
        {
            LOG_ERROR(LOG_TAG, "License verification failed");
            ::memset(response, 0, sizeof(response)); // send empty response to device
        }
        
        auto response_packet = std::make_unique<DataPacket>(MsgId::LicenseVerificationResponse, reinterpret_cast<char*>(response), sizeof(response));
        auto send_status = _session.SendPacket(*response_packet);
        if (send_status != SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed to send license verification response packet");
            return ToStatus(send_status);
        }

        // Wait for reply        
        FaPacket fa_packet {MsgId::MinFa};
        status = _session.RecvFaPacket(fa_packet);
        if (status != SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving license reply packet (status %d)", static_cast<int>(status));
            return ToStatus(status);
        }
        msg_id = fa_packet.header.id;
        if (PacketManager::MsgId::Reply != msg_id)
        {
            LOG_ERROR(LOG_TAG, "Got unexpected message id %d instead of MsgId::Reply", static_cast<int>(msg_id));
            return Status::Error;
        }                
        return Status(fa_packet.GetStatusCode());
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);        
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");        
        return Status::Error;
    }
}


} // namespace RealSenseID
