// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/AuthConfig.h"
#include "RealSenseID/Version.h"
#include "RealSenseID/Logging.h"
#include "RealSenseID/MatchResult.h"
#include "RealSenseID/Faceprints.h"
#include "RealSenseID/AuthFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollFaceprintsExtractionCallback.h"
#include "rsid_c/rsid_client.h"
#include <memory>
#include <string.h>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 26812) // supress msvc's unscoped enum warnings
#endif

namespace
{
    using RealSenseID::Faceprints;
    using RealSenseID::MatchResult;

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

static void copy_to_c_faceprints(RealSenseID::Faceprints& faceprints, rsid_faceprints* c_faceprints)
{    
    c_faceprints->number_of_descriptors = faceprints.numberOfDescriptors;
    c_faceprints->version = faceprints.version;
    ::memcpy(c_faceprints->avg_descriptor, faceprints.avgDescriptor, RSID_NUMBER_OF_RECOGNITION_FACEPRINTS * sizeof(float));
}

// TODO: change name to AuthFeatExtClbk
class FeatExtClbk : public RealSenseID::AuthFaceprintsExtractionCallback
{
    rsid_feat_ext_args _feat_ext_args;

public:
    explicit FeatExtClbk(rsid_feat_ext_args args) : _feat_ext_args {args}
    {
    }

    // TODO: remove face_fatures from interface or decide on something unified in device/host shared authCallback.h file
    void OnResult(const RealSenseID::AuthenticateStatus status) override
    {
        if (_feat_ext_args.result_clbk)
            _feat_ext_args.result_clbk(static_cast<rsid_auth_status>(status), _feat_ext_args.ctx);
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        if (_feat_ext_args.hint_clbk)
            _feat_ext_args.hint_clbk(static_cast<rsid_auth_status>(hint), _feat_ext_args.ctx);
    }
};

class AuthLoopFeatExtClbk :
    public RealSenseID::AuthFaceprintsExtractionCallback
{
    rsid_feat_ext_args _feat_ext_args;
    Faceprints& _faceprints;
    rsid_faceprints* _c_faceprints;


public:
    explicit AuthLoopFeatExtClbk(rsid_feat_ext_args args, Faceprints& faceprints, rsid_faceprints* c_faceprints) :
        _feat_ext_args {args}, _faceprints(faceprints), _c_faceprints(c_faceprints)
    {
    }

    void OnResult(const RealSenseID::AuthenticateStatus status) override
    {
        copy_to_c_faceprints(_faceprints, _c_faceprints);
        if (_feat_ext_args.result_clbk)
            _feat_ext_args.result_clbk(static_cast<rsid_auth_status>(status), _feat_ext_args.ctx);
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        if (_feat_ext_args.hint_clbk)
            _feat_ext_args.hint_clbk(static_cast<rsid_auth_status>(hint), _feat_ext_args.ctx);
    }
};

class EnrollFeatExtClbk : public RealSenseID::EnrollFaceprintsExtractionCallback
{
    rsid_enroll_ext_args _enroll_ext_args;

public:
    explicit EnrollFeatExtClbk(rsid_enroll_ext_args args) : _enroll_ext_args {args}
    {
    }

    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        // TODO: copy 
        if (_enroll_ext_args.status_clbk)
            _enroll_ext_args.status_clbk(static_cast<rsid_enroll_status>(status),  _enroll_ext_args.ctx);
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        if (_enroll_ext_args.progress_clbk)
            _enroll_ext_args.progress_clbk(static_cast<rsid_face_pose>(pose), _enroll_ext_args.ctx);
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        if (_enroll_ext_args.hint_clbk)
            _enroll_ext_args.hint_clbk(static_cast<rsid_enroll_status>(hint), _enroll_ext_args.ctx);
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
    config.camera_rotation = static_cast<RealSenseID::AuthConfig::CameraRotation>(auth_config->camera_rotation);
    config.security_level = static_cast<RealSenseID::AuthConfig::SecurityLevel>(auth_config->security_level);
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

rsid_status rsid_set_auth_settings(rsid_authenticator* authenticator, const rsid_auth_config* auth_config)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto config = auth_config_from_c_struct(auth_config);
    auto status = auth_impl->SetAuthSettings(config);
    return static_cast<rsid_status>(status);
}


rsid_status rsid_query_auth_settings(rsid_authenticator* authenticator, rsid_auth_config* auth_config)
{
    auto* auth_impl = get_auth_impl(authenticator);        
    auto config = auth_config_from_c_struct(auth_config);
    auto status = auth_impl->QueryAuthSettings(config);    

    auth_config->camera_rotation = static_cast<rsid_camera_rotation_type>(config.camera_rotation);
    auth_config->security_level = static_cast<rsid_security_level_type>(config.security_level);
    return static_cast<rsid_status>(status);

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

rsid_status rsid_connect(rsid_authenticator* authenticator, const rsid_serial_config* serial_config)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto config = from_c_struct(serial_config);
    auto status = auth_impl->Connect(config);
    return static_cast<rsid_status>(status);
}

void rsid_disconnect(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auth_impl->Disconnect();
}

rsid_status rsid_pair(rsid_authenticator* authenticator, rsid_pairing_args* pairing_args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto status =
        auth_impl->Pair(pairing_args->host_pubkey, pairing_args->host_pubkey_sig, pairing_args->device_pubkey_result);
    return static_cast<rsid_status>(status);
}

rsid_status rsid_enroll(rsid_authenticator* authenticator, const rsid_enroll_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    EnrollClbk enrollCallback(*args);
    auto status = auth_impl->Enroll(enrollCallback, args->user_id);
    return static_cast<rsid_status>(status);
}

rsid_status rsid_authenticate(rsid_authenticator* authenticator, const rsid_auth_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    AuthClbk authCallback(*args);
    auto status = auth_impl->Authenticate(authCallback);
    return static_cast<rsid_status>(status);
}

static RealSenseID::Faceprints convert_to_cpp_faceprints(rsid_faceprints* rsid_faceprints_instance)
{
    RealSenseID::Faceprints faceprints;
    faceprints.version = rsid_faceprints_instance->version;    
    faceprints.numberOfDescriptors = rsid_faceprints_instance->number_of_descriptors;
    static_assert(sizeof(faceprints.avgDescriptor) == sizeof(rsid_faceprints_instance->avg_descriptor), "faceprints sizes does not match");
    ::memcpy(faceprints.avgDescriptor, rsid_faceprints_instance->avg_descriptor, sizeof(faceprints.avgDescriptor));
    return faceprints;
}

static void create_enroll_args_from_enroll_ext_args(rsid_enroll_ext_args* args, rsid_enroll_args& enroll_args)
{
    enroll_args.ctx = args->ctx;
    enroll_args.hint_clbk = args->hint_clbk;
    enroll_args.progress_clbk = args->progress_clbk;
    enroll_args.status_clbk = args->status_clbk;
    enroll_args.user_id = args->user_id;
}

// TODO: remove function:
#include <iostream>
static void ValidateFaceprints(RealSenseID::Faceprints& faceprints)
{
    float abs_avg = 0.0; // TODO: remove temporary block
    for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS; i++)
    {
        if (faceprints.avgDescriptor[i] > 1 || faceprints.avgDescriptor[i] < -1)
            std::cout << "faceprints is invalid: " << faceprints.avgDescriptor[i] << std::endl;
        abs_avg += std::abs(faceprints.avgDescriptor[i]);
    }
    std::cout << "faceprints absolute-average: " << abs_avg / (float)RSID_NUMBER_OF_RECOGNITION_FACEPRINTS << std::endl;
}

rsid_status rsid_enroll_extract_faceprints(rsid_authenticator* authenticator, rsid_enroll_ext_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    rsid_enroll_args enroll_args;
    create_enroll_args_from_enroll_ext_args(args, enroll_args);
    EnrollClbk enroll_callback(enroll_args);
    RealSenseID::Faceprints faceprints = convert_to_cpp_faceprints(args->faceprints);
    auto status = auth_impl->EnrollExtractFaceprints(enroll_callback, args->user_id, faceprints);        
    copy_to_c_faceprints(faceprints, args->faceprints);    
    return static_cast<rsid_status>(status);
}

rsid_status rsid_authenticate_extract_faceprints(rsid_authenticator* authenticator, rsid_feat_ext_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    FeatExtClbk auth_callback(*args);
    RealSenseID::Faceprints faceprints = convert_to_cpp_faceprints(args->faceprints);
    auto status = auth_impl->AuthenticateExtractFaceprints(auth_callback, faceprints);    
    // TODO: verify why the average faceprint is a bit different compared to the one in FAImpl    
    copy_to_c_faceprints(faceprints, args->faceprints);
    return static_cast<rsid_status>(status);
}

rsid_status rsid_authenticate_loop_extract_faceprints(rsid_authenticator* authenticator, rsid_feat_ext_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);    
    RealSenseID::Faceprints faceprints = convert_to_cpp_faceprints(args->faceprints);
    AuthLoopFeatExtClbk auth_callback(*args, faceprints, args->faceprints);
    auto status = auth_impl->AuthenticateExtractFaceprintsLoop(auth_callback, faceprints);        
    return static_cast<rsid_status>(status);
}

rsid_match_result* rsid_match_faceprints(rsid_authenticator* authenticator, rsid_match_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);

    rsid_faceprints* c_faceprints1 = args->new_faceprints;
    rsid_faceprints* c_faceprints2 = args->existing_faceprints;
    rsid_faceprints* c_updatedFaceprints = args->updated_faceprints;
    Faceprints faceprints1 = convert_to_cpp_faceprints(c_faceprints1);
    Faceprints faceprints2 = convert_to_cpp_faceprints(c_faceprints2);
    Faceprints updatedFaceprints = convert_to_cpp_faceprints(c_updatedFaceprints);
    
    auto result = auth_impl->MatchFaceprints(faceprints1, faceprints2, updatedFaceprints);

    rsid_match_result* match_result = new rsid_match_result();    
    match_result->should_update = result.should_update;
    match_result->success = result.success;    

    return match_result;
}

rsid_faceprints* rsid_create_faceprints()
{    
    rsid_faceprints* faceprints = new rsid_faceprints();
    return faceprints;
}

void rsid_destroy_faceprints(rsid_faceprints* faceprints)
{    
    delete faceprints;
}

rsid_status rsid_authenticate_loop(rsid_authenticator* authenticator, const rsid_auth_args* args)
{
    auto* auth_impl = get_auth_impl(authenticator);
    AuthClbk authCallback(*args);
    auto status = auth_impl->AuthenticateLoop(authCallback);
    return static_cast<rsid_status>(status);
}

rsid_status rsid_cancel(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    auto status = auth_impl->Cancel();
    return static_cast<rsid_status>(status);
}

rsid_status rsid_remove_user(rsid_authenticator* authenticator, const char* user_id)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_status>(auth_impl->RemoveUser(user_id));
}

rsid_status rsid_remove_all_users(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_status>(auth_impl->RemoveAll());
}

// c strings
const char* rsid_status_str(rsid_status status)
{
    return RealSenseID::Description(static_cast<RealSenseID::Status>(status));
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

const char* rsid_auth_settings_rotation(rsid_camera_rotation_type rotation)
{
    return RealSenseID::Description(static_cast<RealSenseID::AuthConfig::CameraRotation>(rotation));
}

const char* rsid_auth_settings_level(rsid_security_level_type level)
{
    return RealSenseID::Description(static_cast<RealSenseID::AuthConfig::SecurityLevel>(level));
}

const char* rsid_version()
{
    return RealSenseID::Version();
}




void rsid_set_log_clbk(rsid_log_clbk clbk, rsid_log_level min_level, int do_formatting)
{
    auto log_clbk = [clbk](RealSenseID::LogLevel level, const char* msg) {
        clbk(static_cast<rsid_log_level>(level), msg);
    };

    auto required_level = static_cast<RealSenseID::LogLevel>(min_level);
    auto required_formatting = static_cast<bool>(do_formatting);
    RealSenseID::SetLogCallback(log_clbk, required_level, required_formatting);
}

rsid_status rsid_query_user_ids(rsid_authenticator* authenticator, char** user_ids,
                                           unsigned int* number_of_users)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_status>(auth_impl->QueryUserIds(user_ids, *number_of_users));
}

// concat user ids to single buffer for easier usage from managed languages
// result buf size must be number_of_users * 16
rsid_status rsid_query_user_ids_to_buf(rsid_authenticator* authenticator, char* result_buf, unsigned int* number_of_users)
{
    auto* auth_impl = get_auth_impl(authenticator);
    
    // allocate needed array of user ids
    char** user_ids = new char*[*number_of_users];
    for (unsigned i = 0; i < *number_of_users; i++)
    {
        user_ids[i] = new char[16];
    }
    unsigned int nusers_in_out = *number_of_users;
    auto status = auth_impl->QueryUserIds(user_ids, nusers_in_out);
    if (status != RealSenseID::Status::Ok)
    {        
        // free allocated memory and return on error
        for (unsigned int i = 0; i < *number_of_users; i++)
        {
            delete user_ids[i];
        }
        delete[] user_ids;
        *number_of_users = 0; 
        return static_cast<rsid_status>(status);        
    }
    // concat to the output buffer
    for (unsigned int i = 0; i < nusers_in_out; i++)
    {
        ::memcpy((char*)&result_buf[i * 16], user_ids[i], 16);
    }
    
    // free allocated memory
    for (unsigned int i = 0; i < *number_of_users; i++)
    {
        delete user_ids[i];
    }
    delete[] user_ids;
    *number_of_users = nusers_in_out;
    return rsid_status::RSID_Ok;
}


rsid_status rsid_query_number_of_users(rsid_authenticator* authenticator, unsigned int* number_of_users)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_status>(auth_impl->QueryNumberOfUsers(*number_of_users));
}

rsid_status rsid_standby(rsid_authenticator* authenticator)
{
    auto* auth_impl = get_auth_impl(authenticator);
    return static_cast<rsid_status>(auth_impl->Standby());
}

#ifdef _WIN32
#pragma warning(pop)
#endif
