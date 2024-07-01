// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#ifdef _WIN32
#if rsid_EXPORTS
#define RSID_API __declspec(dllexport)
#else
#define RSID_API __declspec(dllimport)
#endif // rsid_EXPORTS
#elif LINUX
#define RSID_API __attribute__((visibility("default")))
#else
#define RSID_API
#endif //_WIN32