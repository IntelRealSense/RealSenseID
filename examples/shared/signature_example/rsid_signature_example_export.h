// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#ifdef WIN32
#if rsid_signature_example_EXPORTS
#define RSID_SIG_EXAMPLE_API __declspec(dllexport)
#else
#define RSID_SIG_EXAMPLE_API __declspec(dllimport)
#endif // rsid_signature_example_EXPORTS
#elif UNIX | ANDROID
#define RSID_SIG_EXAMPLE_API __attribute__((visibility("default")))
#else
#define RSID_SIG_EXAMPLE_API
#endif // WIN32