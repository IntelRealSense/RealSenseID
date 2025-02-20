// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "SerialConnection.h"
#include "SerialPacket.h"
#include "CommonTypes.h"
#include "Timer.h"
#include <atomic>
#include <functional>

// Thread safe, non secure session manager. sends/receive packets without any encryption or signing
// Session starts on Start(serial_connection*) and ends in destruction.
// Note:
// This class is not responsible for opening/closing the serial connection. It only uses the given one.
namespace RealSenseID
{
namespace PacketManager
{
class NonSecureSession
{
public:
    NonSecureSession() = default;
    ~NonSecureSession();

    NonSecureSession(const NonSecureSession&) = delete;
    NonSecureSession& operator=(const NonSecureSession&) = delete;

    // Start the session using the given (already open) serial connection.
    // return Status::Ok on success, or error Status otherwise.
    SerialStatus Start(SerialConnection* serial_conn);

    // return true if session is open
    bool IsOpen() const;

    // Send packet
    // return Status::Ok on success, or error status otherwise.
    SerialStatus SendPacket(SerialPacket& packet);

    // Wait for any packet until timeout.
    // Fill the given packet with the received packet.
    // return Status::Ok on success, or error status otherwise.
    SerialStatus RecvPacket(SerialPacket& packet);

    // Wait for fa packet until timeout.
    // Fill the given packet with the received fa packet.
    // If no fa packet available, return timeout status.
    // If the wrong packet type arrives, return RecvUnexpectedPacket status.
    // return Status::Ok on success, or error status otherwise.
    SerialStatus RecvFaPacket(FaPacket& packet);

    // Wait for data packet until timeout.
    // Fill the given packet with the received data packet.
    // If no data packet available, return timeout status.
    // If the wrong packet type arrives, return RecvUnexpectedPacket status.
    // return Status::Ok on success, or error status otherwise.
    SerialStatus RecvDataPacket(DataPacket& packet);

    // async cancel. set the _cancel_required flag and send cancel before next recv
    void Cancel();

private:
    SerialConnection* _serial = nullptr;
    uint32_t _last_sent_seq_number = 0;
    uint32_t _last_recv_seq_number = 0;
    bool _is_open = false;

    // cancel may be called from different threads
    std::atomic<bool> _cancel_required {false};

    SerialStatus SendPacketImpl(SerialPacket& packet);
    SerialStatus RecvPacketImpl(SerialPacket& packet);
    SerialStatus HandleCancelFlag(); // if _cancel_required, send cancel. otherwise do nothing
};
} // namespace PacketManager
} // namespace RealSenseID
