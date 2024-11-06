// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DeviceController.h"
#include "RealSenseID/SerialConfig.h"
#include "rsid_c/rsid_client.h"

#include <cstring>

static RealSenseID::DeviceController* get_controller_impl(rsid_device_controller* device_controller)
{
    return static_cast<RealSenseID::DeviceController*>(device_controller->_impl);
}

rsid_device_controller* rsid_create_device_controller()
{
    rsid_device_controller* rv = nullptr;
    try
    {
        rv = new rsid_device_controller {nullptr};
        rv->_impl = new RealSenseID::DeviceController();
        return rv;
    }
    catch (...)
    {
        try
        {
            if (rv != nullptr)
                delete (RealSenseID::DeviceController*)rv->_impl;
        }
        catch (...)
        {
        }

        try
        {
            delete rv;
        }
        catch (...)
        {
        }
        return nullptr;
    }
}

void rsid_destroy_device_controller(rsid_device_controller* device_controller)
{
    if (device_controller == nullptr)
    {
        return;
    }

    try
    {
        auto* impl = get_controller_impl(device_controller);
        delete impl;
    }
    catch (...)
    {
    }
    delete device_controller;
}

/* connect device controller */
rsid_status rsid_connect_controller(rsid_device_controller* device_controller, const rsid_serial_config* serial_config)
{
    RealSenseID::SerialConfig config;
    config.port = serial_config->port;

    auto* controller_impl = get_controller_impl(device_controller);
    auto status = controller_impl->Connect(config);
    return static_cast<rsid_status>(status);
}

/* disconnect device controller */
void rsid_disconnect_controller(rsid_device_controller* device_controller)
{
    auto* controller_impl = get_controller_impl(device_controller);
    controller_impl->Disconnect();
}

/* firmware version */
rsid_status rsid_query_firmware_version(rsid_device_controller* device_controller, char* output, size_t output_length)
{
    auto* controller_impl = get_controller_impl(device_controller);

    std::string version;
    auto status = controller_impl->QueryFirmwareVersion(version);

    if (status != RealSenseID::Status::Ok)
        return static_cast<rsid_status>(status);

    if (version.length() >= output_length)
        return rsid_status::RSID_Error;

    ::strncpy(output, version.c_str(), output_length);
    return rsid_status::RSID_Ok;
}

RSID_C_API rsid_status rsid_query_serial_number(rsid_device_controller* device_controller, char* output,
                                                size_t output_length)
{
    auto* controller_impl = get_controller_impl(device_controller);

    std::string serial;
    auto status = controller_impl->QuerySerialNumber(serial);

    if (status != RealSenseID::Status::Ok)
        return static_cast<rsid_status>(status);

    if (serial.length() >= output_length)
        return rsid_status::RSID_Error;

    ::strncpy(output, serial.c_str(), output_length);
    return rsid_status::RSID_Ok;
}

RSID_C_API rsid_status rsid_ping(rsid_device_controller* device_controller)
{
    auto* controller_impl = get_controller_impl(device_controller);
    auto status = controller_impl->Ping();
    return static_cast<rsid_status>(status);
}

RSID_C_API rsid_status rsid_fetch_log(rsid_device_controller* device_controller, char* output, size_t output_length)
{
    if (output == nullptr || output_length < 16)
        return rsid_status::RSID_Error;

    std::memset(output, 0, output_length);

    auto* controller_impl = get_controller_impl(device_controller);

    std::string log;
    auto status = controller_impl->FetchLog(log);

    if (status != RealSenseID::Status::Ok)
        return static_cast<rsid_status>(status);

    // If output is smalller than needed copy only last bytes, leaving room for the last '\0' byte
    if (log.size() >= output_length)
    {
        size_t offset = log.size() - (output_length - 1);
        std::memcpy(output, &log[offset], output_length - 1);
    }
    else
    {        
        std::memcpy(output, log.data(), log.size());
    }
    output[output_length - 1] = '\0';
    return rsid_status::RSID_Ok;
}

RSID_C_API rsid_status rsid_get_color_gains(rsid_device_controller* device_controller, int* red, int* blue)
{
    if(red == nullptr || blue == nullptr)
		return rsid_status::RSID_Error;

    auto* controller_impl = get_controller_impl(device_controller);
    auto status = controller_impl->GetColorGains(*red, *blue);
    return static_cast<rsid_status>(status);
}

RSID_C_API rsid_status rsid_set_color_gains(rsid_device_controller* device_controller, int red, int blue)
{
    auto* controller_impl = get_controller_impl(device_controller);
    auto status = controller_impl->SetColorGains(red, blue);
    return static_cast<rsid_status>(status);
}
