// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/AuthConfig.h"
#include "RealSenseID/Version.h"
#include "rsid_c/rsid_client.h"

#include <memory>

namespace
{
class EnrollClbk : public RealSenseID::EnrollmentCallback
{
    rsid_enroll_args _enroll_args;

public:
    explicit EnrollClbk(rsid_enroll_args args) : _enroll_args {args}
    {
    }

    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        if (_enroll_args.status_clbk)
            _enroll_args.status_clbk(static_cast<rsid_enroll_status>(status), _enroll_args.ctx);
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        if (_enroll_args.progress_clbk)
            _enroll_args.progress_clbk(static_cast<rsid_face_pose>(pose), _enroll_args.ctx);
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        if (_enroll_args.hint_clbk)
            _enroll_args.hint_clbk(static_cast<rsid_enroll_status>(hint), _enroll_args.ctx);
    }
};

class AuthClbk : public RealSenseID::AuthenticationCallback
{
    rsid_auth_args _auth_args;

public:
    explicit AuthClbk(rsid_auth_args args) : _auth_args {args}
    {
    }

    void OnResult(const RealSenseID::AuthenticateStatus status, const char* userId) override
    {
        if (_auth_args.result_clbk)
            _auth_args.result_clbk(static_cast<rsid_auth_status>(status), (const char*)userId, _auth_args.ctx);
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        if (_auth_args.hint_clbk)
            _auth_args.hint_clbk(static_cast<rsid_auth_status>(hint), _auth_args.ctx);
    }
};


// Signature callbacks - called by the lib to sign outcoming messaages to the device,
// and to verify incoming messages from the device.
class WrapperSignatureClbk : public RealSenseID::SignatureCallback
{
    rsid_signature_clbk _user_clbk;

public:
    WrapperSignatureClbk(rsid_signature_clbk* clbk) : _user_clbk {*clbk}
    {
    }

    ~WrapperSignatureClbk() = default;

    // Called by the lib to get a signature for the given buffer.
    // The generated signature should be copied to the given *outSig (already pre-allocated, 32 bytes, output buffer).
    // Return value: true on success, false otherwise.
    bool Sign(const unsigned char* buffer, const unsigned int length, unsigned char* outSig) override
    {
        return _user_clbk.sign_clbk(buffer, length, outSig, _user_clbk.ctx) != 0;
    }

    // Called by the lib to verify the buffer and the given signature.
    // Return value: true on verify success, false otherwise.
    bool Verify(const unsigned char* buffer, const unsigned int bufferLen, const unsigned char* sig,
                const unsigned int sigLen) override
    {
        return _user_clbk.verify_clbk(buffer, bufferLen, sig, sigLen, _user_clbk.ctx) != 0;
    }
};

// Context class
// Will be stored as opaque pointer in the rsid_face_authenticator struct.
class SecureAuthContext
{
    std::unique_ptr<WrapperSignatureClbk> _signature_callback;
    std::unique_ptr<RealSenseID::FaceAuthenticator> _authenticator;

public:
    SecureAuthContext(rsid_signature_clbk* clbk) :
        _signature_callback {std::make_unique<WrapperSignatureClbk>(clbk)},
        _authenticator {std::make_unique<RealSenseID::FaceAuthenticator>(_signature_callback.get())}
    {
    }

    WrapperSignatureClbk* sign_clbk()
    {
        return _signature_callback.get();
    }

    RealSenseID::FaceAuthenticator* authenticator()
    {
        return _authenticator.get();
    }
};
using auth_context_t = SecureAuthContext;


RealSenseID::SerialConfig from_c_struct(const rsid_serial_config* serial_config)
{
    RealSenseID::SerialConfig config;
    config.serType = static_cast<RealSenseID::SerialType>(serial_config->serial_type);
    config.port = serial_config->port;
    return config;
}

RealSenseID::AuthConfig auth_config_from_c_struct(const rsid_auth_config* auth_config)
{
    RealSenseID::AuthConfig config;
    config.camera_rotation = static_cast<RealSenseID::AuthConfig::CameraRotationType>(auth_config->camera_rotation);
    config.security_level = static_cast<RealSenseID::AuthConfig::SecurityLevelType>(auth_config->security_level);
    return config;
}

RealSenseID::FaceAuthenticator* get_auth_impl(rsid_authenticator* authenticator)
{
    auto* auth_ctx = static_cast<auth_context_t*>(authenticator->_impl);
    return auth_ctx->authenticator();
}
} // namespace

rsid_authenticator* rsid_create_authenticator(rsid_signature_clbk* signature_clbk)
{
    auth_context_t* auth_ctx = nullptr;
    try
    {
        auth_ctx = new auth_context_t {signature_clbk};
        if (!auth_ctx)
        {
            return nullptr; // create failed
        }
        auto* rv = new rsid_authenticator();
        rv->_impl = static_cast<void*>(auth_ctx);
        return rv;
    }
    catch (...)
    {
        if (auth_ctx)
        {
            try
            {
                delete (auth_ctx);
            }
            catch (...)
            {
            }
        }
        return nullptr;
    }
}

rsid_serial_status rsid_set_auth_settings(rsid_authenticator* authenticator, const rsid_auth_config* auth_config)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto config = auth_config_from_c_struct(auth_config);
    auto status = auth_impl->SetAuthSettings(config);
    return static_cast<rsid_serial_status>(status);
}

void rsid_destroy_authenticator(rsid_authenticator* authenticator)
{
    if (authenticator == nullptr)
    {
        return;
    }

    try
    {
        auto* auth_ctx = static_cast<auth_context_t*>(authenticator->_impl);
        delete (auth_ctx);
    }
    catch (...)
    {
    }
    delete authenticator;
}

rsid_serial_status rsid_connect(rsid_authenticator* authenticator, const rsid_serial_config* serial_config)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto config = from_c_struct(serial_config);
    auto status = auth_impl->Connect(config);
    return static_cast<rsid_serial_status>(status);
}

void rsid_disconnect(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auth_impl->Disconnect();
}

rsid_enroll_status rsid_enroll(rsid_authenticator* authenticator, const rsid_enroll_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    EnrollClbk enrollCallback(*args);
    auto status = auth_impl->Enroll(enrollCallback, args->user_id);
    return static_cast<rsid_enroll_status>(status);
}

rsid_auth_status rsid_authenticate(rsid_authenticator* authenticator, const rsid_auth_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    AuthClbk authCallback(*args);
    auto status = auth_impl->Authenticate(authCallback);
    return static_cast<rsid_auth_status>(status);
}

rsid_auth_status rsid_authenticate_loop(rsid_authenticator* authenticator, const rsid_auth_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    AuthClbk authCallback(*args);
    auto status = auth_impl->AuthenticateLoop(authCallback);
    return static_cast<rsid_auth_status>(status);
}

rsid_serial_status rsid_cancel(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto status = auth_impl->Cancel();
    return static_cast<rsid_serial_status>(status);
}

rsid_serial_status rsid_remove_user(rsid_authenticator* authenticator, const char* user_id)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_serial_status>(auth_impl->RemoveUser(user_id));
}

rsid_serial_status rsid_remove_all_users(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_serial_status>(auth_impl->RemoveAll());
}

// c strings
const char* rsid_serial_status_str(rsid_serial_status status)
{
    return RealSenseID::Description(static_cast<RealSenseID::SerialStatus>(status));
}

const char* rsid_auth_status_str(rsid_auth_status status)
{
    return RealSenseID::Description(static_cast<RealSenseID::AuthenticateStatus>(status));
}

const char* rsid_enroll_status_str(rsid_enroll_status status)
{
    return RealSenseID::Description(static_cast<RealSenseID::EnrollStatus>(status));
}

const char* rsid_face_pose_str(rsid_face_pose pose)
{
    return RealSenseID::Description(static_cast<RealSenseID::FacePose>(pose));
}

const char* rsid_version()
{
    return RealSenseID::Version();
}
