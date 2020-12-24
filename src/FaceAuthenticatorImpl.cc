// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "FaceAuthenticatorImpl.h"
#include "Logger.h"
#include "PacketManager/Timer.h"
#include "StatusHelper.h"
#include "RealSenseID/AuthConfig.h"

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#else
#error "Platform not supported"
#endif //  _WIN32


static const char* LOG_TAG = "FaceAuthenticatorImpl";

namespace RealSenseID
{

// save callback functions to use in the secure session later
FaceAuthenticatorImpl::FaceAuthenticatorImpl(SignatureCallback* callback) :
    _session {[callback](const unsigned char* buffer, const unsigned int bufferLen, unsigned char* outSig) {
                  return callback->Sign(buffer, bufferLen, outSig);
              },
              [callback](const unsigned char* buffer, const unsigned int bufferLen, const unsigned char* sig,
                         const unsigned int sigLen) { return callback->Verify(buffer, bufferLen, sig, sigLen); }}
{
}



SerialStatus FaceAuthenticatorImpl::Connect(const SerialConfig& config)
{
    try
    {
        // disconnect if already connected
        _serial.reset();
        PacketManager::SerialConfig serial_config;
        serial_config.ser_type = static_cast<PacketManager::SerialType>(config.serType);
        serial_config.port = config.port;

#ifdef _WIN32
        _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
        _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#endif

        return SerialStatus::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return SerialStatus::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception durting serial connect");
        return SerialStatus::Error;
    }
}

SerialStatus FaceAuthenticatorImpl::SetAuthSettings(const RealSenseID::AuthConfig& auth_config)
{
    auto status = _session.Start(_serial.get());
    if (status != PacketManager::Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
        return ToSerialStatus(status);
    }
    int size = sizeof(auth_config);
    char settings[2];
    settings[0] = static_cast<char>(auth_config.camera_rotation);
    settings[1] = static_cast<char>(auth_config.security_level);
    PacketManager::DataPacket data_packet {PacketManager::MsgId::SetAuthSettings, settings, sizeof(settings)};

    status = _session.SendPacket(data_packet);
    if (status != PacketManager::Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
    }
    // convert internal status to api's serial status and return
    return ToSerialStatus(status);
}

void FaceAuthenticatorImpl::Disconnect()
{
    _serial.reset();
}

// Do enroll session with the device. Call user's enroll callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
EnrollStatus FaceAuthenticatorImpl::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            auto enroll_status = ToEnrollStatus(status);
            callback.OnHint(enroll_status);
            return enroll_status;
        }

        PacketManager::FaPacket fa_packet {PacketManager::MsgId::Enroll, user_id, '0'};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            auto enroll_status = ToEnrollStatus(status);
            callback.OnHint(enroll_status);
            return enroll_status;
        }

        PacketManager::Timer session_timer {_enroll_session_timeout};

        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnResult(EnrollStatus::Failure);
                return EnrollStatus::Failure;
            }

            status = _session.RecvFaPacket(fa_packet);
            if (status != PacketManager::Status::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                auto enroll_status = ToEnrollStatus(status);
                callback.OnHint(enroll_status);
                return enroll_status;
            }

            auto msg_id = fa_packet.payload.id;
            auto fa_status = fa_packet.GetStatusCode();

            switch (msg_id)
            {
            case (PacketManager::MsgId::Reply):
                return EnrollStatus(fa_status);

            case (PacketManager::MsgId::Result):
                callback.OnResult(EnrollStatus(fa_status));
                break;

            case (PacketManager::MsgId::Progress):
                callback.OnProgress((FacePose)fa_status);
                break;

            case (PacketManager::MsgId::Hint):
                callback.OnHint(EnrollStatus(fa_status));
                break;

            default:
                callback.OnResult(EnrollStatus::DeviceError);
                return RealSenseID::EnrollStatus::DeviceError;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnResult(EnrollStatus::Failure);
        return EnrollStatus::Failure;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(EnrollStatus::Failure);
        return EnrollStatus::Failure;
    }
}

// Do authenticate session with the device. Call user's authenticate callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y').
//      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
//      Unexpected msg_id in the fa response.
AuthenticateStatus FaceAuthenticatorImpl::Authenticate(AuthenticationCallback& callback)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            auto auth_status = ToAuthStatus(status);
            callback.OnHint(auth_status);
            return auth_status;
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::Authenticate};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            auto auth_status = ToAuthStatus(status);
            callback.OnHint(auth_status);
            return auth_status;
        }
        PacketManager::Timer session_timer {_auth_session_timeout};
        while (true)
        {
            if (session_timer.ReachedTimeout())
            {
                LOG_ERROR(LOG_TAG, "session timeout");
                callback.OnHint(AuthenticateStatus::DeviceError);
                return AuthenticateStatus::DeviceError;
            }

            status = _session.RecvFaPacket(fa_packet);
            if (status != PacketManager::Status::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                auto auth_status = ToAuthStatus(status);
                callback.OnHint(auth_status);
                return auth_status;
            }

            auto msg_id = fa_packet.payload.id;
            auto fa_status = fa_packet.GetStatusCode();

            switch (msg_id)
            {
            case (PacketManager::MsgId::Reply):
                return AuthenticateStatus(fa_status);

            case (PacketManager::MsgId::Result): {
                char user_id[32];
                fa_packet.GetUserId(user_id, sizeof(user_id));
                callback.OnResult(AuthenticateStatus(fa_status), user_id);
                break;
            }

            case (PacketManager::MsgId::Hint):
                callback.OnHint(AuthenticateStatus(fa_status));
                break;

            default:
                callback.OnHint(AuthenticateStatus::DeviceError);
                return RealSenseID::AuthenticateStatus::DeviceError;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnHint(AuthenticateStatus::Failure);
        return AuthenticateStatus::Failure;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(AuthenticateStatus::Failure, nullptr);
        return AuthenticateStatus::Failure;
    }
}

// Do infinite authenticate loop with the device. Call user's authenticate callbacks in the process.
// Wait for one of the following to happen:
//      We get 'reply' from device ('Y' , would happen if "cancel" command was sent by a different thread to this
//      device). Any non ok status from the session object(i.e. serial comm failed, or session timeout). Unexpected
//      msg_id in the fa response.
AuthenticateStatus FaceAuthenticatorImpl::AuthenticateLoop(AuthenticationCallback& callback)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            auto auth_status = ToAuthStatus(status);
            callback.OnHint(auth_status);
            return auth_status;
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::AuthenticateLoop};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            auto auth_status = ToAuthStatus(status);
            callback.OnHint(auth_status);
            return auth_status;
        }

        // number of retries in case of communication failure before giving up
        const int max_retries = 3;
        int retry_counter = 0;
        while (true)
        {
            status = _session.RecvFaPacket(fa_packet);
            if (status != PacketManager::Status::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                auto auth_status = ToAuthStatus(status);
                callback.OnHint(auth_status);
                if (++retry_counter <= max_retries)
                {
                    LOG_WARNING(LOG_TAG, "Timeout waiting for packet (try #%d)", retry_counter);
                    continue;
                }
                return auth_status;
            }
            // received succesfully packet
            retry_counter = 0;
            auto msg_id = fa_packet.payload.id;
            auto fa_status = fa_packet.GetStatusCode();

            switch (msg_id)
            {
            case (PacketManager::MsgId::Reply):
                return AuthenticateStatus(fa_status);

            case (PacketManager::MsgId::Result): {
                char user_id[32];
                fa_packet.GetUserId(user_id, sizeof(user_id));
                callback.OnResult(AuthenticateStatus(fa_status), user_id);
                break;
            }

            case (PacketManager::MsgId::Hint):
                callback.OnHint(AuthenticateStatus(fa_status));
                break;

            default:
                LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                callback.OnHint(AuthenticateStatus::DeviceError);
                return RealSenseID::AuthenticateStatus::DeviceError;
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        callback.OnHint(AuthenticateStatus::Failure);
        return AuthenticateStatus::Failure;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        callback.OnResult(AuthenticateStatus::Failure, nullptr);
        return AuthenticateStatus::Failure;
    }
}

SerialStatus FaceAuthenticatorImpl::Cancel()
{
    try
    {
        // note session should have been already opened, otherwise this will fail
        if (!_session.IsOpen())
        {
            LOG_ERROR(LOG_TAG, "Cannot cancel. Session is not open");
            return SerialStatus::Error;
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::Cancel};
        auto status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
        }
        // convert internal status to api's serial status and return
        return ToSerialStatus(status);
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return SerialStatus::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return SerialStatus::Error;
    }
}


SerialStatus FaceAuthenticatorImpl::RemoveUser(const char* user_id)
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            return ToSerialStatus(status);
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::RemoveUser, user_id, 0};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            return ToSerialStatus(status);
        }

        status = _session.RecvFaPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
            return ToSerialStatus(status);
        }

        auto fa_status = fa_packet.GetStatusCode();
        LOG_ERROR(LOG_TAG, "Failed removing user (status %d)", (int)status);
        return fa_status > 0 ? SerialStatus::Ok : SerialStatus::Error;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return SerialStatus::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return SerialStatus::Error;
    }
}

SerialStatus FaceAuthenticatorImpl::RemoveAllUsers()
{
    try
    {
        auto status = _session.Start(_serial.get());
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
            return ToSerialStatus(status);
        }
        PacketManager::FaPacket fa_packet {PacketManager::MsgId::RemoveAllUsers};
        status = _session.SendPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
            return ToSerialStatus(status);
        }
        status = _session.RecvFaPacket(fa_packet);
        if (status != PacketManager::Status::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
            return ToSerialStatus(status);
        }
        auto fa_status = fa_packet.GetStatusCode();
        return fa_status > 0 ? SerialStatus::Ok : SerialStatus::Error;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return SerialStatus::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return SerialStatus::Error;
    }
}
} // namespace RealSenseID
