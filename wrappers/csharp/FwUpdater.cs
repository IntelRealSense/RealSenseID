// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Runtime.InteropServices;
using System.Text;

namespace rsid
{
    public class FwUpdater : IDisposable
    {
        public delegate void ProgressCallback(float progress);

        [StructLayout(LayoutKind.Sequential)]
        public struct EventHandler
        {
            public ProgressCallback progressClbk;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct FwUpdateSettings
        {
            public string port;
            public int force_full;
        }

        public FwUpdater()
        {
            _handle = rsid_create_fw_updater();
        }

        ~FwUpdater()
        {
            Dispose();
        }



        public String ExtractFwVersion(string binPath)
        {
            var newFwVersion = new byte[100];
            var result = rsid_extract_firmware_version(_handle, binPath, newFwVersion, newFwVersion.Length);

            if(result != 0)
            {
                return Encoding.ASCII.GetString(newFwVersion).TrimEnd('\0');
            }

            return null;
        }

        public Status Update(string binPath, EventHandler eventHandler, FwUpdateSettings settings)
        {
            _eventHandler = eventHandler;
            return rsid_update_firmware(_handle, ref eventHandler, ref settings, binPath);
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                rsid_destroy_fw_updater(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }

        private IntPtr _handle;
        private EventHandler _eventHandler;
        private bool _disposed = false;
       
        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_fw_updater();

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_fw_updater(IntPtr handle);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_extract_firmware_version(IntPtr handle, string binPath, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] output, int len);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_update_firmware(IntPtr handle, ref EventHandler eventHandler, ref FwUpdateSettings settings, string binPath);
    }
}
