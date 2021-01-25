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
        }

        ~DeviceController()
        {
            Dispose(false);
        }

        public void Connect(SerialConfig config)
        {
            var rv = rsid_connect_controller(_handle, ref config);
            if (rv != Status.Ok) 
            {
                throw new Exception("Failed connecting to port " + config.port);
            }
           
        }

        public string QueryFirmwareVersion()
        {
            var output = new byte[250];
            if (rsid_query_firmware_version(_handle, output, output.Length) != Status.Ok)
                return "";

            return Encoding.UTF8.GetString(output);
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
    }
}
