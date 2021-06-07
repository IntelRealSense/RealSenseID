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
        MJPEG_1080P = 0, // default
        MJPEG_720P = 1,
        RAW10_1080P = 2 // dump all frames
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct PreviewConfig
    {
        public int cameraNumber;
        public PreviewMode previewMode;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 0)]
    public struct FaceRect
    {
        public UInt32 x;
        public UInt32 y;
        public UInt32 width;
        public UInt32 height;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct PreviewImageMetadata
    {
        public UInt32 timestamp;
        public UInt32 status;
        public UInt32 sensor_id;
        public bool led;
        public bool projector;
        public bool is_snapshot;
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
        PreviewCallback _clbkDelegateSnapshot;
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
                if (_handle != IntPtr.Zero)
                    rsid_destroy_preview(_handle);
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

        public bool Start(PreviewCallback clbkPreview)
        {            
            if (_handle == IntPtr.Zero)
                return false;

            _clbkDelegate = clbkPreview; //save it to prevent from the delegate garbage collected
            var rv = rsid_start_preview(_handle, _clbkDelegate, IntPtr.Zero) != 0;
            return rv;
        }

        public bool Start(PreviewCallback clbkPreview, PreviewCallback clbkSnapshot)
        {
            if (_handle == IntPtr.Zero)
                return false;

            _clbkDelegate = clbkPreview; //save it to prevent from the delegate garbage collected
            _clbkDelegateSnapshot = clbkSnapshot;
            var rv = rsid_start_preview_and_snapshots(_handle, _clbkDelegate, _clbkDelegateSnapshot, IntPtr.Zero) != 0;
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

        public bool RawToRgb(ref PreviewImage in_img,ref PreviewImage out_img)
        {
            if (_handle == IntPtr.Zero)
                return false;
            return rsid_raw_to_rgb(_handle,ref in_img,ref out_img) != 0;
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
        static extern int rsid_start_preview(IntPtr rsid_preview, PreviewCallback clbkPreview,IntPtr ctx);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_start_preview_and_snapshots(IntPtr rsid_preview, PreviewCallback clbkPreview, PreviewCallback clbkSnapshots, IntPtr ctx);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_pause_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_resume_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_stop_preview(IntPtr rsid_preview);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_raw_to_rgb(IntPtr rsid_preview, ref PreviewImage in_img,ref PreviewImage out_img);
    }

}
