// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Runtime.InteropServices;
using System.Text;

namespace rsid
{
    public class DeviceController : IDisposable
    {
        public DeviceController()
        {            
            _handle = rsid_create_device_controller();
            if (_handle == IntPtr.Zero) throw new Exception("Error creating device controller");
        }

        ~DeviceController()
        {
            Dispose(false);
        }

        public Status Connect(SerialConfig config)
        {
            return rsid_connect_controller(_handle, ref config);                       
        }

        public string QueryFirmwareVersion()
        {
            var output = new byte[250];
            if (rsid_query_firmware_version(_handle, output, output.Length) != Status.Ok)
                return "";

            return Encoding.ASCII.GetString(output).TrimEnd('\0'); ;
        }

        public string QuerySerialNumber()
        {
            var output = new byte[30];
            if (rsid_query_serial_number(_handle, output, output.Length) != Status.Ok)
                return "";

            return Encoding.ASCII.GetString(output).TrimEnd('\0'); ;
        }

        public Status Ping()
        {
            return rsid_ping(_handle);
        }

        public void Disconnect()
        {
            rsid_disconnect_controller(_handle);
        }

        public void Dispose()
        {
            Dispose(true);
            // prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }
        
        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                rsid_destroy_device_controller(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }
        private IntPtr _handle;
        private bool _disposed = false;
       
        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_device_controller();

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_device_controller(IntPtr rsid_device_controller);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_connect_controller(IntPtr rsid_device_controller, ref SerialConfig serialConfig);
        
        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_disconnect_controller(IntPtr rsid_device_controller);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_firmware_version(IntPtr rsid_device_controller, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] output, int len);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_serial_number(IntPtr rsid_device_controller, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] output, int len);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_ping(IntPtr rsid_device_controller);
    }
}
