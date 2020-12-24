// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "MbedtlsWrapper.h"
#include "Logger.h"
#include "mbedtls/sha256.h"
#include <string.h>

static const char* LOG_TAG = "MbedtlsWrapper";

namespace RealSenseID
{
namespace PacketManager
{
MbedtlsWrapper::MbedtlsWrapper() : _ecdh_generate_key {false}, _shared_secret {}, _ecdh_signed_pubkey {}
{
    mbedtls_entropy_init(&_entropy_ctx);
    mbedtls_ctr_drbg_init(&_ctr_drbg_ctx);
    mbedtls_ecdh_init(&_edch_ctx);
    mbedtls_aes_init(&_aes_ctx);
}

MbedtlsWrapper::~MbedtlsWrapper()
{
    mbedtls_entropy_free(&_entropy_ctx);
    mbedtls_ctr_drbg_free(&_ctr_drbg_ctx);
    mbedtls_ecdh_free(&_edch_ctx);
    mbedtls_aes_free(&_aes_ctx);
}

void MbedtlsWrapper::Reset()
{
    ::memset(_shared_secret, 0, sizeof(_shared_secret));
    ::memset(_ecdh_signed_pubkey, 0, sizeof(_ecdh_signed_pubkey));
}

size_t MbedtlsWrapper::GetSignedEcdhPubkeySize()
{
    return SIGNED_PUBKEY_SIZE;
}

unsigned char* MbedtlsWrapper::GetSignedEcdhPubkey(SignCallback sign_clbk)
{
    Reset();

    if (!GenerateEcdhKey())
    {
        LOG_ERROR(LOG_TAG, "Failed to generate ecdh key");
        return nullptr;
    }

    int ret = mbedtls_mpi_write_binary(&_edch_ctx.Q.X, _ecdh_signed_pubkey, ECC_P256_KEY_X_Y_Z_SIZE_BYTES);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_write_binary returned %d", ret);
        return nullptr;
    }

    ret = mbedtls_mpi_write_binary(&_edch_ctx.Q.Y, _ecdh_signed_pubkey + ECC_P256_KEY_X_Y_Z_SIZE_BYTES,
                                   ECC_P256_KEY_X_Y_Z_SIZE_BYTES);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_write_binary returned %d", ret);
        return nullptr;
    }

    unsigned char digest[SHA_256_DIGEST_SIZE_BYTES];
    ret = mbedtls_sha256_ret(_ecdh_signed_pubkey, ECC_P256_KEY_SIZE_BYTES, digest, 0);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_sha256_ret returned %d", ret);
        return nullptr;
    }
    bool res = sign_clbk(digest, SHA_256_DIGEST_SIZE_BYTES, _ecdh_signed_pubkey + ECC_P256_KEY_SIZE_BYTES);
    if (!res)
    {
        LOG_ERROR(LOG_TAG, "Failed to sign key");
        return nullptr;
    }

    return _ecdh_signed_pubkey;
}

bool MbedtlsWrapper::VerifyEcdhSignedKey(const unsigned char* ecdh_signed_pubkey, VerifyCallback verify_clbk)
{
    int ret = mbedtls_mpi_lset(&_edch_ctx.Qp.Z, 1);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_lset returned %d", ret);
        return false;
    }

    ret = mbedtls_mpi_read_binary(&_edch_ctx.Qp.X, ecdh_signed_pubkey, ECC_P256_KEY_X_Y_Z_SIZE_BYTES);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_read_binary ecdh_signed_pubkey X returned %d", ret);
        return false;
    }

    ret = mbedtls_mpi_read_binary(&_edch_ctx.Qp.Y, ecdh_signed_pubkey + ECC_P256_KEY_X_Y_Z_SIZE_BYTES,
                                  ECC_P256_KEY_X_Y_Z_SIZE_BYTES);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_read_binary ecdh_signed_pubkey Y returned %d", ret);
        return false;
    }

    unsigned char digest[SHA_256_DIGEST_SIZE_BYTES];
    ret = mbedtls_sha256_ret(ecdh_signed_pubkey, ECC_P256_KEY_SIZE_BYTES, digest, 0);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_sha256_ret returned %d", ret);
        return false;
    }
    bool res = verify_clbk(digest, SHA_256_DIGEST_SIZE_BYTES, ecdh_signed_pubkey + ECC_P256_KEY_SIZE_BYTES,
                           ECC_P256_SIG_SIZE_BYTES);
    if (!res)
    {
        LOG_WARNING(LOG_TAG, "Failed to verify key - ignoring");
        // return false;
    }

    if (!GenerateEcdhKey())
    {
        LOG_ERROR(LOG_TAG, "Failed to generate ecdh key");
        return false;
    }

    ret = mbedtls_ecdh_compute_shared(&_edch_ctx.grp, &_edch_ctx.z, &_edch_ctx.Qp, &_edch_ctx.d,
                                      mbedtls_ctr_drbg_random, &_ctr_drbg_ctx);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_ecdh_compute_shared returned %d", ret);
        return false;
    }

    ret = mbedtls_mpi_write_binary(&_edch_ctx.z, _shared_secret, ECC_P256_KEY_X_Y_Z_SIZE_BYTES);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_mpi_write_binary returned %d", ret);
        return false;
    }

    ret = mbedtls_aes_setkey_enc(&_aes_ctx, _shared_secret, AES_CTR_256_BIT_KEY_SIZE_BYTES * 8);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_aes_setkey_enc returned %d", ret);
        return false;
    }

    return true;
}

bool MbedtlsWrapper::Encrypt(const unsigned char* input, unsigned char* output, const unsigned int length)
{
    bool res = AesCtr256(input, output, length);
    return res;
}

bool MbedtlsWrapper::Decrypt(const unsigned char* input, unsigned char* output, const unsigned int length)
{
    bool res = AesCtr256(input, output, length);
    return res;
}

bool MbedtlsWrapper::GenerateEcdhKey()
{
    if (_ecdh_generate_key)
        return true;

    int ret = mbedtls_ctr_drbg_seed(&_ctr_drbg_ctx, mbedtls_entropy_func, &_entropy_ctx, NULL, 0);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_ctr_drbg_seed returned %d", ret);
        return false;
    }

    ret = mbedtls_ecp_group_load(&_edch_ctx.grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_ecp_group_load returned %d", ret);
        return false;
    }

    ret = mbedtls_ecdh_gen_public(&_edch_ctx.grp, &_edch_ctx.d, &_edch_ctx.Q, mbedtls_ctr_drbg_random, &_ctr_drbg_ctx);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_ecdh_gen_public returned %d", ret);
        return false;
    }

    _ecdh_generate_key = true;
    return true;
}

bool MbedtlsWrapper::AesCtr256(const unsigned char* input, unsigned char* output, const unsigned int length)
{
    size_t nc_off = 0;

    unsigned char iv[AES_CTR_IV_SIZE_BYTES];
    memset(iv, 0, AES_CTR_IV_SIZE_BYTES);

    unsigned char stream_block[AES_CTR_IV_SIZE_BYTES];
    memset(stream_block, 0, AES_CTR_IV_SIZE_BYTES);

    auto ret = mbedtls_aes_crypt_ctr(&_aes_ctx, length, &nc_off, iv, stream_block, input, output);
    if (ret != 0)
    {
        LOG_ERROR(LOG_TAG, "Failed! mbedtls_aes_crypt_ctr returned %d", ret);
        return false;
    }

    return true;
}
} // namespace PacketManager
} // namespace RealSenseID
