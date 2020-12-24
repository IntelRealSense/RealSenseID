// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Runtime.InteropServices;

namespace rsid
{

    public enum SerialType
    {
        USB,
        UART
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SerialConfig
    {
        public SerialType serialType;
        public string port;        
    }
    
    // 
    // Serial status
    // 
    public enum SerialStatus
    {
        Ok = 100,
        Error,
        SecurityError
    }

    //
    // Enroll API structs 
    //
    public enum EnrollStatus
    {
        Success,
        BadFrameQuality,
        NoFaceDetected,
        FaceDetected,
        LedFlowSuccess,
        FaceIsTooFarToTheTop,
        FaceIsTooFarToTheBottom,
        FaceIsTooFarToTheRight,
        FaceIsTooFarToTheLeft,
        FaceTiltIsTooUp,
        FaceTiltIsTooDown,
        FaceTiltIsTooRight,
        FaceTiltIsTooLeft,
        FaceIsNotFrontal,
        FaceIsTooFarFromTheCamera,
        FaceIsTooCloseToTheCamera,
        CameraStarted,
        CameraStopped,
        MultipleFacesDetected,
        Failure,
        DeviceError,
        Serial_Ok = 100,
        Serial_Error,
        Serial_SecurityError,
    }

    public enum FacePose
    {
        Center,
        Up,
        Down,
        Left,
        Right
    }

    // Signature callbacks
    public delegate int Sign(IntPtr buffer, int bufferLen, IntPtr outSig, IntPtr ctx);
    public delegate int Verify(IntPtr buffer, int bufferLen, IntPtr sig, int sigLen, IntPtr ctx);

    [StructLayout(LayoutKind.Sequential)]
    public struct SignatureCallback
    {
        public Sign signCallback;
        public Verify verifyCallback;
        public IntPtr ctx;
    }

    //
    // Enroll callbacks
    //
    public delegate void EnrollStatusCallback(EnrollStatus status, IntPtr ctx);
    public delegate void EnrollHintCallback(EnrollStatus status, IntPtr ctx);
    public delegate void EnrollProgressCallback(FacePose status, IntPtr ctx);

    [StructLayout(LayoutKind.Sequential)]
    public struct EnrollArgs
    {
        public string userId;
        public EnrollStatusCallback statusClbk;
        public EnrollProgressCallback progressClbk;
        public EnrollHintCallback hintClbk;
        public IntPtr ctx;
    }

    //
    // Auth config
    //
    [StructLayout(LayoutKind.Sequential)]
    public struct AuthConfig
    {
        public enum CameraRotation
        {
            Rotation_90_Deg = 0, // default
            Rotation_180_Deg
        };

        public enum SecurityLevel
        {
            High = 0,  // default
            Medium = 1 
        };

        public CameraRotation cameraRotation;
        public SecurityLevel securityLevel;
    }

    //
    // Authenticate API structs
    // 
    public enum AuthStatus
    {
        Success,
        BadFrameQuality,
        NoFaceDetected,
        FaceDetected,
        LedFlowSuccess,
        FaceIsTooFarToTheTop,
        FaceIsTooFarToTheBottom,
        FaceIsTooFarToTheRight,
        FaceIsTooFarToTheLeft,
        FaceTiltIsTooUp,
        FaceTiltIsTooDown,
        FaceTiltIsTooRight,
        FaceTiltIsTooLeft,
        FaceIsTooFarFromTheCamera,
        FaceIsTooCloseToTheCamera,
        CameraStarted,
        CameraStopped,
        Forbidden,
        DeviceError,
        Failure,
        Serial_Ok = 100,
        Serial_Error,
        Serial_SecurityError,
    }

    public delegate void AuthStatusCallback(AuthStatus status, string userId, IntPtr ctx);
    public delegate void AuthlHintCallback(AuthStatus status, IntPtr ctx);

    [StructLayout(LayoutKind.Sequential)]
    public struct AuthArgs
    {
        public AuthStatusCallback statusClbk;
        public AuthlHintCallback hintClbk;
        public IntPtr ctx;
    }

    public class Authenticator : IDisposable
    {
        public Authenticator(SignatureCallback signatureCallback)
        {            
            _handle = rsid_create_authenticator(ref signatureCallback);
        }

        ~Authenticator()
        {
            Dispose(false);
        }

        public void Connect(SerialConfig config)
        {
            var rv = rsid_connect(_handle, ref config);
            if (rv != 100) // TODO
            {
                throw new Exception("Failed connecting to port " + config.port);
            }

            /*AuthConfig args;
            args.cameraRotation = AuthConfig.CameraRotation.ROTATION_90_DEG;
            args.securityLevel = AuthConfig.SecurityLevel.MEDIUM;

            SetAuthSettings(args);*/
        }  
        public SerialStatus SetAuthSettings(AuthConfig args)
        {            
            return rsid_set_auth_settings(_handle, ref args);
        }

        public void Disconnect()
        {
            rsid_disconnect(_handle);
        }

        public void Dispose()
        {
            Dispose(true);
            // prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }

        public EnrollStatus Enroll(EnrollArgs args)
        {
            _enrollArgs = args; // store to prevent the delegates to be garbage collected
            return rsid_enroll(_handle, ref args);
        }

        public AuthStatus Authenticate(AuthArgs args)
        {
            _authArgs = args;
            return rsid_authenticate(_handle, ref args);
        }

        public AuthStatus AuthenticateLoop(AuthArgs args)
        {
            _authArgs = args;
            return rsid_authenticate_loop(_handle, ref args);
        }

        public string Version()
        {
            //return rsid_version();
            //var p = rsid_version();
            return Marshal.PtrToStringAnsi(rsid_version());
        }

        public SerialStatus Cancel()
        {
            return rsid_cancel(_handle);
        }

        public SerialStatus RemoveAllUsers()
        {
            return rsid_remove_all_users(_handle);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                rsid_destroy_authenticator(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }


#if DEBUG
        private const string dllName = "rsid_c_debug";
#else
        private const string dllName = "rsid_c";
#endif
        private IntPtr _handle;
        private bool _disposed = false;

        private EnrollArgs _enrollArgs;
        private AuthArgs _authArgs;

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_authenticator(ref SignatureCallback signatureCallback);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_authenticator(IntPtr rsid_authenticator);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_connect(IntPtr rsid_authenticator, ref SerialConfig serialConfig);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern SerialStatus rsid_set_auth_settings(IntPtr rsid_authenticator, ref AuthConfig authConfig);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_disconnect(IntPtr rsid_authenticator);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern EnrollStatus rsid_enroll(IntPtr rsid_authenticator, ref EnrollArgs enrollArgs);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern AuthStatus rsid_authenticate(IntPtr rsid_authenticator, ref AuthArgs authArgs);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern AuthStatus rsid_authenticate_loop(IntPtr rsid_authenticator, ref AuthArgs authArgs);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern SerialStatus rsid_cancel(IntPtr rsid_authenticator);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern SerialStatus rsid_remove_all_users(IntPtr rsid_authenticator);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_version();
    }
}
