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

        public struct FwVersion
        {
            public string OpfwVersion { get; set; }
            public string RecognitionVersion { get; set; }
        }

        public enum UpdatePolicy
        {
            Continous,
            Opfw_First,
            Require_Intermediate_Fw,
            Not_Allowed
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct UpdatePolicyInfo
        {
            public UpdatePolicy updatePolicy;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
            public char[] intermediateVersion;
        }

        public FwVersion? ExtractFwVersion(string binPath)
        {
            var newFwVersion = new byte[100];
            var newRecognitionVersion = new byte[100];
            var result = rsid_extract_firmware_version(_handle, binPath, newFwVersion, newFwVersion.Length, newRecognitionVersion, newRecognitionVersion.Length);

            if (result != 0)
            {
                return new FwVersion
                {
                    OpfwVersion = Encoding.ASCII.GetString(newFwVersion).TrimEnd('\0'),
                    RecognitionVersion = Encoding.ASCII.GetString(newRecognitionVersion).TrimEnd('\0')
                };
            }

            return null;
        }

        public Status Update(string binPath, EventHandler eventHandler, FwUpdateSettings settings, bool updateRecognition)
        {
            _eventHandler = eventHandler;
            return rsid_update_firmware(_handle, ref eventHandler, ref settings, binPath, updateRecognition ? 1 : 0);
        }

        public bool IsEncyptionCompatibleWithDevice(string bin_path, string serial_number)
        {
            return rsid_is_encryption_compatible_with_device(_handle, bin_path, serial_number) != 0;
        }

        public void DecideUpdatePolicy(string bin_path, FwUpdateSettings settings, out UpdatePolicyInfo updatePolicyInfo)
        {
            rsid_decide_update_policy(_handle, settings, bin_path, out updatePolicyInfo);
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
        static extern int rsid_extract_firmware_version(IntPtr handle, string binPath,
            [Out, MarshalAs(UnmanagedType.LPArray)] byte[] fw_output, int fw_output_len,
            [Out, MarshalAs(UnmanagedType.LPArray)] byte[] recognition_output, int recognition_output_len);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_update_firmware(IntPtr handle, ref EventHandler eventHandler, ref FwUpdateSettings settings, string binPath, int exclude_recognition);
        
        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_is_encryption_compatible_with_device(IntPtr rsid_authenticator, string bin_path, string serial_number);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_decide_update_policy(IntPtr handle,FwUpdateSettings settings, string binPath, out UpdatePolicyInfo updatePolicyInfo);
    }
}
