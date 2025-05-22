// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.IO;
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

        public FwUpdater(DeviceType deviceType)
        {
            _handle = rsid_create_fw_updater((int)deviceType);
        }

        ~FwUpdater()
        {
            Dispose();
        }

        public struct FwVersion
        {
            public DeviceType DeviceType { get; set; }
            public string OpfwVersion { get; set; }
            public string RecognitionVersion { get; set; }
        }

        public FwVersion? ExtractFwVersion(string binPath)
        {
            // Decide device type according to filename 
            string filename = Path.GetFileName(binPath);
            string ext = Path.GetExtension(binPath).ToLower();
            DeviceType deviceType = DeviceType.Unknown;
            if (filename.StartsWith("F45") && ext == ".bin")
            {
                deviceType = DeviceType.F45x;
            }
            else if (filename.StartsWith("F46") && ext == ".bin")
            {
                deviceType = DeviceType.F46x;
            }
            else
            {
                return null;
            }

            var newFwVersion = new byte[100];
            var newRecognitionVersion = new byte[100];
            var result = rsid_extract_firmware_version(_handle, binPath, newFwVersion, newFwVersion.Length, newRecognitionVersion, newRecognitionVersion.Length);

            if (result == Status.Ok)
            {
                return new FwVersion
                {
                    DeviceType = deviceType,
                    OpfwVersion = Encoding.ASCII.GetString(newFwVersion).TrimEnd('\0'),
                    RecognitionVersion = Encoding.ASCII.GetString(newRecognitionVersion).TrimEnd('\0')
                };
            }

            return null;
        }

        public Status Update(string binPath, EventHandler eventHandler, FwUpdateSettings settings)
        {
            _eventHandler = eventHandler;
            return rsid_update_firmware(_handle, ref eventHandler, ref settings, binPath);
        }

        public bool IsSkuCompatible(FwUpdateSettings settings, string bin_path, out int expected_sku_ver, out int device_sku_ver)
        {
            return rsid_is_sku_compatible(_handle, settings, bin_path, out expected_sku_ver, out device_sku_ver) != 0;
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
        static extern IntPtr rsid_create_fw_updater(int deviceType);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_fw_updater(IntPtr handle);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_extract_firmware_version(IntPtr handle, string binPath,
            [Out, MarshalAs(UnmanagedType.LPArray)] byte[] fw_output, int fw_output_len,
            [Out, MarshalAs(UnmanagedType.LPArray)] byte[] recognition_output, int recognition_output_len);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_update_firmware(IntPtr handle, ref EventHandler eventHandler, ref FwUpdateSettings settings, string binPath);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_is_sku_compatible(IntPtr rsid_authenticator, FwUpdateSettings settings, string bin_path, out int expected_sku_ver, out int device_sku_ver);
    }
}
