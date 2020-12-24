// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "NonSecureHostSession.h"
#include "PacketSender.h"
#include "Logger.h"

#include <stdexcept>
#include <string>
#include <cassert>

static const char* LOG_TAG = "NonSecureHostSession";

namespace RealSenseID
{
namespace PacketManager
{
Status NonSecureHostSession::Start(SerialConnection* serial_conn)
{
    std::lock_guard<std::mutex> lock {_mutex};
    LOG_DEBUG(LOG_TAG, "start session");
    if (serial_conn == nullptr)
    {
        throw std::runtime_error("NonSecureHostSession: SerialConnection is null");
    }
    _serial = serial_conn;
    return Status::Ok;
}

bool NonSecureHostSession::IsOpen()
{
    return _serial != nullptr;
}

NonSecureHostSession::~NonSecureHostSession()
{
    LOG_DEBUG(LOG_TAG, "close session");
}


Status NonSecureHostSession::SendPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};

    assert(_serial != nullptr);
    PacketSender sender {_serial};
    return sender.Send(packet);
}

Status NonSecureHostSession::RecvPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};

    assert(_serial != nullptr);
    PacketSender sender {_serial};
    return sender.Recv(packet);
}

Status NonSecureHostSession::RecvFaPacket(FaPacket& packet)
{
    auto res_status = RecvPacket(packet);
    if (res_status != Status::Ok)
    {
        return res_status;
    }
    return IsFaPacket(packet) ? Status::Ok : Status::RecvUnexpectedPacket;
}

Status NonSecureHostSession::RecvDataPacket(DataPacket& packet)
{
    auto res_status = RecvPacket(packet);
    if (res_status != Status::Ok)
    {
        return res_status;
    }
    return IsDataPacket(packet) ? Status::Ok : Status::RecvUnexpectedPacket;
}

} // namespace PacketManager
} // namespace RealSenseID
