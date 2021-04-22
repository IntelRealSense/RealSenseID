// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
namespace rsid
{
    public enum PreviewMode
    {
        VGA = 0,       // default
        FHD_Rect = 1,  // result frame with face rect
        Dump = 2       // dump all frames
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct PreviewConfig
    {
        public int cameraNumber;
        public PreviewMode previewMode;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct FaceRect
    {
        public UInt32 x;
        public UInt32 y;
        public UInt32 width;
        public UInt32 height;
    };
    /*
     *   [StructLayout(LayoutKind.Sequential)]
    public struct DetectedFace
    {
        [MarshalAs(UnmanagedType.U4, SizeConst = 1)]
        int x;
        [MarshalAs(UnmanagedType.U4, SizeConst = 1)]
        int y;
        [MarshalAs(UnmanagedType.U4, SizeConst = 1)]
        int w;
        [MarshalAs(UnmanagedType.U4, SizeConst = 1)]
        int h;
    }*/

    [StructLayout(LayoutKind.Sequential)]
    public struct PreviewImageMetadata
    {
        public FaceRect face_rect;
        public UInt32 timestamp;
        public UInt32 status;
        public UInt32 sensor_id;
        public bool led;
        public bool projector;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PreviewImage
    {
        public IntPtr buffer;
        public int size;
        public int width;
        public int height;
        public int stride;
        public int number;
        public PreviewImageMetadata metadata;
    }

    public delegate void PreviewCallback(PreviewImage image, IntPtr ctx);

    public class Preview : IDisposable
    {
        PreviewCallback _clbkDelegate;
        PreviewConfig _config;

        public Preview(PreviewConfig config)
        {            
           UpdateConfig(config);
        }

        public void UpdateConfig(PreviewConfig config)
        {
            _config = config;
            try
            {
                _handle = rsid_create_preview(ref _config);
            }
            catch (TypeLoadException)
            {
                // work without preview
                _handle = IntPtr.Zero;
            }
        }

        ~Preview()
        {
            Dispose(false);
        }

        public bool Start(PreviewCallback clbk)
        {            
            if (_handle == IntPtr.Zero)
                return false;

            _clbkDelegate = clbk; //save it to prevent from the delegate garbage collected
            var rv = rsid_start_preview(_handle, _clbkDelegate, IntPtr.Zero) != 0;
            return rv;
        }


        public bool Pause()
        {
            if (_handle == IntPtr.Zero)
                return false;
            return rsid_pause_preview(_handle) != 0;
        }

        public bool Resume()
        {
            if (_handle == IntPtr.Zero)
                return false;
            return rsid_resume_preview(_handle) != 0;
        }

        public bool Stop()
        {
            if (_handle == IntPtr.Zero)
                return false;
            return rsid_stop_preview(_handle) != 0;
        }

        public void Dispose()
        {
            Dispose(true);
            // prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }

        private IntPtr _handle = IntPtr.Zero;
        private bool _disposed = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (_handle != IntPtr.Zero)
                    rsid_destroy_preview(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_preview(ref PreviewConfig config);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_start_preview(IntPtr rsid_preview, PreviewCallback clbk, IntPtr ctx);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_pause_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_resume_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_stop_preview(IntPtr rsid_preview);
    }
}
