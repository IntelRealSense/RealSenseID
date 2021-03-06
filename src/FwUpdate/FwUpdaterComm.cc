// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "FwUpdaterComm.h"
#include "Logger.h"
#include "PacketManager/Timer.h"

#include <cstring>
#include <cassert>
#include <cmath>
#include <stdexcept>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#elif ANDROID
#include "PacketManager/SerialPacket.h"
#include "PacketManager/AndroidSerial.h"
#else
#error "Platform not supported"
#endif //_WIN32


namespace RealSenseID
{
namespace FwUpdate
{
static const char* LOG_TAG = "FwUpdater";

FwUpdaterComm::FwUpdaterComm(const char* port_name)
{
    _read_buffer = new char[ReadBufferSize];
    PacketManager::SerialConfig serial_config;
    serial_config.port = port_name;

#ifdef _WIN32
    _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
    _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#else
    throw std::runtime_error("FwUpdaterComm not supported for this OS yet");
#endif // WIN32

    // create thread thread
    _reader_thread = std::thread([this] { this->ReaderThreadLoop(); });
}

#ifdef ANDROID
FwUpdaterComm::FwUpdaterComm(const AndroidSerialConfig& config)
{
    _read_buffer = new char[ReadBufferSize];
    _serial = std::make_unique<PacketManager::AndroidSerial>(config.fileDescriptor, config.readEndpoint, config.writeEndpoint);
    // create thread thread
    _reader_thread = std::thread([this] { this->ReaderThreadLoop(); });
}
#endif

FwUpdaterComm::~FwUpdaterComm()
{
    try
    {
        this->StopReaderThread();
    }
    catch (...)
    {
    }
    
    try
    {        
        delete (_read_buffer);
    }
    catch (...)
    {
    }
}

void FwUpdaterComm::ReaderThreadLoop()
{
    while (!_should_stop_thread)
    {
        if (_read_index >= ReadBufferSize - 2)
        {
            // should never happen on normal execution, since 128kb should be enough for the entire session
            assert(false);
            _read_index = 0;
            _scan_index = 0;
        }
        auto status = this->_serial->RecvBytes(&_read_buffer[_read_index], 1);
        if (status == PacketManager::SerialStatus::Ok)
        {
            // putchar((int)_read_buffer[_read_index]);
            _read_buffer[++_read_index] = '\0'; // always null terminate
            continue;
        }
        if (status != PacketManager::SerialStatus::RecvTimeout)
        {
            break; // fail reading from the serial
        }
    }
}

void FwUpdaterComm::ConsumeScanned()
{
    _scan_index = _read_index.load();
}

char* FwUpdaterComm::GetScanPtr() const
{
    auto scan_idx = _scan_index.load();
    auto* rv = &_read_buffer[scan_idx];    
    return rv;
}

char* FwUpdaterComm::ReadBuffer() const
{
    return _read_buffer;
}

// wait until no more input (100 ms without any new bytes)
size_t FwUpdaterComm::WaitForIdle()
{
    while (true)
    {
        auto prevIndex = _read_index.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (prevIndex == _read_index.load())
        {
            return _read_index.load();
        }
    }
}

// 1. Break binary data into chunks of 16kb.
// 2. Send all chunks, one by one.
void FwUpdaterComm::WriteBinary(const char* buf, size_t n_bytes)
{
#ifdef _WIN32
    constexpr size_t max_chunk_size = 512 * 1024; // bigger chunks works much faster in windows
#else
    constexpr size_t max_chunk_size = 16 * 1024;
#endif
    
    auto n_chunks = static_cast<size_t>(::ceil(static_cast<double>(n_bytes) / max_chunk_size));

    LOG_DEBUG(LOG_TAG, "sending buffer in %zu chunks", n_chunks);
    for (size_t i = 0; i < n_chunks; ++i)
    {
        LOG_TRACE(LOG_TAG, "sending chunk #%d", i);

        // set chunk starting point and size to be sent
        auto chunk_start = &buf[i * max_chunk_size];
        auto chunk_size = i < (n_chunks - 1) ? max_chunk_size : n_bytes - ((n_chunks - 1) * max_chunk_size);
        auto status = _serial->SendBytes(chunk_start, chunk_size);
        if (status != PacketManager::SerialStatus::Ok)
        {            
            throw std::runtime_error("FwUpdaterComm::WriteBinary failed");
        }
        // Give the device time to process the chunk
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }    
}

// 1. Wait until input drained
// 2. Send the command
// 3. Wait for cmd "ack" upto 1 second, if wait_response is true
void FwUpdaterComm::WriteCmd(const std::string& cmd, bool wait_response)
{
    LOG_DEBUG(LOG_TAG, "WriteCmd \"%s\"", cmd.c_str());
    WaitForIdle();
    auto serial_status = _serial->SendBytes(cmd.c_str(), cmd.length());
    if (serial_status != PacketManager::SerialStatus::Ok)
    {
        throw std::runtime_error("FwUpdaterComm::WriteCmd failed");
    }

    // must send newline after cmd
    serial_status = _serial->SendBytes("\n", 1);
    if (serial_status != PacketManager::SerialStatus::Ok)
    {
        throw std::runtime_error("FwUpdaterComm::WriteCmd failed");
    }

    // wait of the ack response upto 1 second
    // expected response token has the form of "cmd_string ack" (without cmd args)
    // for example:
    //     "cmd1 arg1 arg2 arg3.." => "cmd1 ack"
    //     "cmd2" => "cmd2 ack"

    auto first_word = cmd.substr(0, cmd.find(' '));
    first_word += " ack";
    auto timeout = wait_response ? std::chrono::milliseconds {1000} : std::chrono::milliseconds {0};
    WaitForStr(first_word.c_str(), timeout);
}

void FwUpdaterComm::WaitForStr(const char* wait_str, std::chrono::milliseconds timeout)
{
    using PacketManager::SerialStatus;
    using PacketManager::Timer;

    Timer timer {timeout};
    bool should_wait = timeout.count() > 0;

    if (!should_wait || wait_str[0] == '\0')
    {
        return;
    }

    LOG_DEBUG(LOG_TAG, "waiting [%s] for %zu millis..", wait_str, timeout.count());    
    auto scan_idx = _scan_index.load();

    while (!timer.ReachedTimeout())
    {
        WaitForIdle();

        auto found = strstr(&_read_buffer[scan_idx], wait_str) != nullptr;
        if (found)
        {
            LOG_DEBUG(LOG_TAG, "Got the expected str \"%s\" after %zu millis", wait_str, timer.Elapsed().count());            
            return;
        }

        if (timer.ReachedTimeout())
        {                        
            ConsumeScanned();
            throw std::runtime_error("FwUpdaterComm::WaitForStr failed");            
        }
    }    
}

void FwUpdaterComm::StopReaderThread()
{    
    _should_stop_thread = true;
    if (_reader_thread.joinable())
    {
        LOG_DEBUG(LOG_TAG, "Stopping reader thread..");
        _reader_thread.join();
        LOG_DEBUG(LOG_TAG, "Reader thread stopped");
    }    
}
} // namespace FwUpdate
} // namespace RealSenseID
