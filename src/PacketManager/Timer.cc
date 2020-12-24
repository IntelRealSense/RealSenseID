// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Timer.h"

namespace RealSenseID
{
namespace PacketManager
{
Timer::Timer(timeout_t threshold) : _timeout {threshold}, _start_tp {clock::now()}
{
}

timeout_t Timer::Elapsed() const
{
    return std::chrono::duration_cast<timeout_t>(clock::now() - _start_tp);
}

timeout_t Timer::TimeLeft() const
{
    return _timeout - Elapsed();
}

bool Timer::ReachedTimeout() const
{
    return TimeLeft() <= timeout_t {0};
}

void Timer::Reset()
{
    _start_tp = clock ::now();
}
} // namespace PacketManager
} // namespace RealSenseID
