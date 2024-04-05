using rsid;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using static rsid.FwUpdater;

namespace rsid
{
    public class UpdateChecker
    {
     /*
        uint64_t sw_version;
        uint64_tfw_version;
        const char* sw_version_str;
        const char* fw_version_str;
        const char* release_url;
        const char* release_notes_url;
         */

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct ReleaseInfoMarshalled
        {
            public ulong sw_version;
            public ulong fw_version;
            public IntPtr SwVersionStr;
            public IntPtr FwVersionStr;
            public IntPtr ReleaseUrl;
            public IntPtr ReleaseNotesUrl;
        }

        public class ReleaseInfo
        {
            public ulong sw_version;
            public ulong fw_version;
            public string sw_version_str;
            public string fw_version_str;
            public string release_url;
            public string release_notes_url;
        }
       

        private static ReleaseInfo FromMarshalled(ReleaseInfoMarshalled marshalled)
        {
            return new ReleaseInfo
            {
                sw_version = marshalled.sw_version,
                fw_version = marshalled.fw_version,
                sw_version_str = Marshal.PtrToStringAnsi(marshalled.SwVersionStr),
                fw_version_str = Marshal.PtrToStringAnsi(marshalled.FwVersionStr),
                release_url = Marshal.PtrToStringAnsi(marshalled.ReleaseUrl),
                release_notes_url = Marshal.PtrToStringAnsi(marshalled.ReleaseNotesUrl)
            };
        }
        

        public static Status GetRemoteReleaseInfo(out ReleaseInfo result)
        {                        
            var marshalled = new ReleaseInfoMarshalled();
            var status = rsid_get_remote_release_info(ref marshalled);
            result = FromMarshalled(marshalled);
            rsid_free_release_info(ref marshalled);            
            return status;
        }

        public static Status GetLocalReleaseInfo(SerialConfig serialConfig, out ReleaseInfo result)
        {
            var marshalled = new ReleaseInfoMarshalled();
            var status = rsid_get_local_release_info(ref serialConfig, ref marshalled);
            result = FromMarshalled(marshalled);
            rsid_free_release_info(ref marshalled);
            return status;
        }

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_get_remote_release_info(ref ReleaseInfoMarshalled result);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_get_local_release_info(ref SerialConfig serialConfig, ref ReleaseInfoMarshalled result);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_free_release_info(ref ReleaseInfoMarshalled releaseInfo);
    }
}
