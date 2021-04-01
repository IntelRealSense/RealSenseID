// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "NonSecureSession.h"
#include "PacketSender.h"
#include "Logger.h"
#include <stdexcept>
#include <string.h>
#include <cassert>

static const char* LOG_TAG = "NonSecureSession";
static const int MAX_SEQ_NUMBER_DELTA = 20;

namespace RealSenseID
{
namespace PacketManager
{
NonSecureSession::~NonSecureSession()
{
    try
    {
        LOG_DEBUG(LOG_TAG, "Close session");
        auto ignored = HandleCancelFlag(); // cancel if requested but not handled yet
        (void)ignored;
    }
    catch (...)
    {
    }
}

SerialStatus NonSecureSession::Start(SerialConnection* serial_conn)
{
    LOG_DEBUG(LOG_TAG, "Start session");

    _is_open = false;
    _cancel_required = false;

    if (serial_conn == nullptr)
    {
        throw std::runtime_error("NonSecureSession: serial connection is null");
    }
    _serial = serial_conn;
    _last_sent_seq_number = 0;
    _last_recv_seq_number = 0;

    DataPacket packet {MsgId::StartSession};
    PacketSender sender {_serial};
    auto status = sender.SendBinary(packet);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to send start session packet");
        return status;
    }

    status = sender.Recv(packet);
    if (status != SerialStatus::Ok || packet.header.id != MsgId::StartSession)
        LOG_ERROR(LOG_TAG, "Failed to recv device start session response");

    return status;
}

bool NonSecureSession::IsOpen()
{
    return _is_open;
}

SerialStatus NonSecureSession::SendPacket(SerialPacket& packet)
{
    return SendPacketImpl(packet);
}

SerialStatus NonSecureSession::RecvPacket(SerialPacket& packet)
{
    return RecvPacketImpl(packet);
}

SerialStatus NonSecureSession::RecvFaPacket(FaPacket& packet)
{
    auto status = RecvPacketImpl(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }
    return IsFaPacket(packet) ? SerialStatus::Ok : SerialStatus::RecvUnexpectedPacket;
}

SerialStatus NonSecureSession::RecvDataPacket(DataPacket& packet)
{
    auto status = RecvPacketImpl(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }
    return IsDataPacket(packet) ? SerialStatus::Ok : SerialStatus::RecvUnexpectedPacket;
}

SerialStatus NonSecureSession::SendPacketImpl(SerialPacket& packet)
{
    // increment and set sequence number in the packet
    packet.payload.sequence_number = ++_last_sent_seq_number;
    assert(_serial != nullptr);
    PacketSender sender {_serial};
    return sender.SendBinary(packet);
}

// new sequence number should advance by max of MAX_SEQ_NUMBER_DELTA from last number
static bool ValidateSeqNumber(uint32_t last_recv_number, uint32_t seq_number)
{
    return (last_recv_number < seq_number && seq_number <= last_recv_number + MAX_SEQ_NUMBER_DELTA);
}

SerialStatus NonSecureSession::RecvPacketImpl(SerialPacket& packet)
{
    assert(_serial != nullptr);
    PacketSender sender {_serial};

    // Handle cancel flag
    auto status = HandleCancelFlag(); 
    if (status != SerialStatus::Ok)
    {        
        return status;
    }

    status = sender.Recv(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }

    // validate sequence number
    auto current_seq = packet.payload.sequence_number;
    if (!ValidateSeqNumber(_last_recv_seq_number, current_seq))
    {
        LOG_ERROR(LOG_TAG, "Invalid sequence number. Last: %zu, Current: %zu", _last_recv_seq_number, current_seq);
        return SerialStatus::SecurityError;
    }
    _last_recv_seq_number = current_seq;
    return SerialStatus::Ok;
}

void NonSecureSession::Cancel()
{
    LOG_DEBUG(LOG_TAG, "Cancel requested.");
    _cancel_required = true;
}

SerialStatus NonSecureSession::HandleCancelFlag()
{
    if (!_cancel_required)
    {
        return SerialStatus::Ok;
    }
    _cancel_required = false;
    if (!_serial)
    {
        LOG_WARNING(LOG_TAG, "Cannot send cancel, no serial connection");
        return SerialStatus::SendFailed;
    }

    LOG_DEBUG(LOG_TAG, "Sending cancel..");
    return _serial->SendBytes(Commands::face_cancel, ::strlen(Commands::face_cancel));
}
} // namespace PacketManager
} // namespace RealSenseID
