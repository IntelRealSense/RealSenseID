// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace rsid
{
    static class FaceprintsConsts
    {
        public const int RSID_NUMBER_OF_RECOGNITION_FACEPRINTS = 256;
        // here we should use the same vector lengths as in RSID_FEATURES_VECTOR_ALLOC_SIZE (256 for now, may increase to 257 in the future).
        public const int RSID_FEATURES_VECTOR_ALLOC_SIZE = 256;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PairingArgs
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
        public byte[] HostPubkey;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] hostPubkeySignature;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
        public byte[] DevicePubkey;
    }

    // Enroll API struct
    public enum EnrollStatus
    {
        Success,
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
        CameraStarted,
        CameraStopped,
        MultipleFacesDetected,
        Failure,
        DeviceError,
        Serial_Ok = 100,
        Serial_Error,
        Serial_SecurityError,
        Serial_VersionMismatch,
        Serial_CrcError,
        Reserved1 = 120,
        Reserved2,
        Reserved3

    }

    public enum FacePose
    {
        Center,
        Up,
        Down,
        Left,
        Right
    }

    //
    // Enroll callbacks
    //
    public delegate void EnrollResultCallback(EnrollStatus status, IntPtr ctx);
    public delegate void EnrollHintCallback(EnrollStatus status, IntPtr ctx);
    public delegate void EnrollProgressCallback(FacePose status, IntPtr ctx);
    public delegate void EnrollExtractionResultCallback(EnrollStatus status, IntPtr faceprintsHandle, IntPtr ctx);

    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Faceprints
    {
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int version;
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int numberOfDescriptors;
        [MarshalAs(UnmanagedType.U2, SizeConst = 1)]
        public ushort featuresType;

        // here we should use the same vector lengths as in RSID_FEATURES_VECTOR_ALLOC_SIZE (256 for now, may increase to 257 in the future).
        // we have 2 vectors : orig for the enrollement vector (saved once), and avg for the ongoing updated avg vector.
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = FaceprintsConsts.RSID_FEATURES_VECTOR_ALLOC_SIZE)]
        public short[] avgDescriptor;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = FaceprintsConsts.RSID_FEATURES_VECTOR_ALLOC_SIZE)]
        public short[] origDescriptor;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct EnrollExtractArgs
    {
        public EnrollExtractionResultCallback resultClbk;
        public EnrollProgressCallback progressClbk;
        public EnrollHintCallback hintClbk;
        public FaceDetecedCallback faceDetectedClbk;
        public IntPtr ctx;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct EnrollArgs
    {
        public string userId;
        public EnrollResultCallback resultClbk;
        public EnrollProgressCallback progressClbk;
        public EnrollHintCallback hintClbk;
        public FaceDetecedCallback faceDetectedClbk;
        public IntPtr ctx;
    }

    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct MatchArgs
    {
        public rsid.Faceprints newFaceprints;
        public rsid.Faceprints existingFaceprints;
        public rsid.Faceprints updatedFaceprints;
    }

    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct UserFeatures
    {
        public string userID;
        public rsid.Faceprints faceprints;
    }

    //
    // Auth config
    //
    [StructLayout(LayoutKind.Sequential)]
    public struct DeviceConfig
    {
        public enum CameraRotation
        {
            Rotation_0_Deg = 0, // default
            Rotation_180_Deg
        };

        public enum SecurityLevel
        {
            High = 0,  // high security, no mask support, all AS algo(s) will be activated
            Medium = 1, // default mode, supports masks, only main AS algo will be activated.  
            RecognitionOnly = 2 // configures device to run recognition only without AS
        };

        public enum PreviewMode
        {
            VGA = 0,  // default
            FHD_Rect = 1,
            Dump = 2
        };

        public CameraRotation cameraRotation;
        public SecurityLevel securityLevel;
        public PreviewMode previewMode;
        public bool advancedMode;
    }

    //
    // Authenticate API struct
    // 
    public enum AuthStatus
    {
        Success,
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
        CameraStarted,
        CameraStopped,
        Forbidden,
        DeviceError,
        Failure,
        Serial_Ok = 100,
        Serial_Error,
        Serial_SecurityError,
        Serial_VersionMismatch,
        Serial_CrcError,
        Reserved1 = 120,
        Reserved2,
        Reserved3
    }


    [StructLayout(LayoutKind.Sequential)]
    
    public struct MatchResult
    {
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int success;
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int shouldUpdate;
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int score;
        [MarshalAs(UnmanagedType.I4, SizeConst = 1)]
        public int confidence;
    }

    public delegate void AuthResultCallback(AuthStatus status, string userId, IntPtr ctx);
    public delegate void AuthlHintCallback(AuthStatus status, IntPtr ctx);
    public delegate void FaceDetecedCallback(IntPtr faces, int count, IntPtr ctx);
    public delegate void AuthExtractionResultCallback(AuthStatus status, IntPtr faceprints, IntPtr ctx);

    [StructLayout(LayoutKind.Sequential)]
    public struct AuthArgs
    {
        public AuthResultCallback resultClbk;
        public AuthlHintCallback hintClbk;
        public FaceDetecedCallback faceDetectedClbk;
        public IntPtr ctx;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct AuthExtractArgs
    {
        public AuthExtractionResultCallback resultClbk;
        public AuthlHintCallback hintClbk;
        public FaceDetecedCallback faceDetectedClbk;
        public IntPtr ctx;
    }

    public class Authenticator : IDisposable
    {
        public const int MaxUserIdSize = 30;
#if RSID_SECURE
        public Authenticator(SignatureCallback signatureCallback)
        {
            _handle = rsid_create_authenticator(ref signatureCallback);
        }
#else
        public Authenticator()
        {
            _handle = rsid_create_authenticator();
        }
#endif


        ~Authenticator()
        {
            Dispose(false);
        }

        public Status Connect(SerialConfig config)
        {
            return rsid_connect(_handle, ref config);
        }

        public void Disconnect()
        {
            rsid_disconnect(_handle);
        }

        public Status Pair(ref PairingArgs args)
        {
            return rsid_pair(_handle, ref args);
        }

        public Status Unpair()
        {
            return rsid_unpair(_handle);
        }

        public Status SetDeviceConfig(DeviceConfig args)
        {
            return rsid_set_device_config(_handle, ref args);
        }

        public Status QueryDeviceConfig(out DeviceConfig result)
        {
            result = new DeviceConfig();
            return rsid_query_device_config(_handle, ref result);
        }

        public void Dispose()
        {
            Dispose(true);
            // prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }

        public Status Enroll(EnrollArgs args)
        {
            _enrollArgs = args; // store to prevent the delegates to be garbage collected
            return rsid_enroll(_handle, ref args);
        }

        public Status Authenticate(AuthArgs args)
        {
            _authArgs = args;
            return rsid_authenticate(_handle, ref args);
        }

        public Status DetectSpoof(AuthArgs args)
        {
            _authArgs = args;
            return rsid_detect_spoof(_handle, ref args);
        }

        public Status AuthenticateLoop(AuthArgs args)
        {
            _authArgs = args;
            return rsid_authenticate_loop(_handle, ref args);
        }

        public static string Version()
        {
            return Marshal.PtrToStringAnsi(rsid_version());
        }

        public static string CompatibleFirmwareVersion()
        {
            return Marshal.PtrToStringAnsi(rsid_compatible_firmware_version());
        }

        public static bool IsFwCompatibleWithHost(string fw_version)
        {
            return rsid_is_fw_compatible_with_host(fw_version) != 0;
        }

        public Status Cancel()
        {
            return rsid_cancel(_handle);
        }

        public Status RemoveAllUsers()
        {
            return rsid_remove_all_users(_handle);
        }

        public Status RemoveUser(string userId)
        {
            return rsid_remove_user(_handle, userId);
        }


        public Status QueryNumberOfUsers(out int numberOfUsers)
        {
            return rsid_query_number_of_users(_handle, out numberOfUsers);
        }

        public Status QueryUserIds(out string[] userIds)
        {

            int userCount;
            userIds = new string[0];

            // Query user count first
            var status = QueryNumberOfUsers(out userCount);
            if (status != rsid.Status.Ok || userCount == 0)
            {
                return status;
            }

            // Allocate buffer to hold the results (31 bytes for each user)
            var chunkSize = MaxUserIdSize + 1;
            var buf = new byte[chunkSize * userCount];
            status = rsid_query_user_ids_to_buf(_handle, buf, ref userCount);

            if (status != rsid.Status.Ok || userCount <= 0)
            {
                return status;
            }

            // translate to string array. 
            userIds = new string[userCount];
            for (var i = 0; i < userCount; i++)
            {
                userIds[i] = Encoding.ASCII.GetString(buf, i * chunkSize, chunkSize).TrimEnd('\0'); ;
            }
            return status;
        }

        // Send device to standby
        public Status Standby()
        {
            return rsid_standby(_handle);
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

        public Status EnrollExtractFaceprints(EnrollExtractArgs args)
        {
            _enrollExtractArgs = args; // store to prevent the delegates to be garbage collected
            return rsid_extract_faceprints_for_enroll(_handle, ref args);
        }

        public Status AuthenticateExtractFaceprints(AuthExtractArgs args)
        {
            _authExtractArgs = args;
            return rsid_extract_faceprints_for_auth(_handle, ref args);
        }

        public Status AuthenticateLoopExtractFaceprints(AuthExtractArgs args)
        {
            _authExtractArgs = args;
            return rsid_extract_faceprints_for_auth_loop(_handle, ref args);
        }

        public MatchResult MatchFaceprintsToFaceprints(ref MatchArgs args)
        {
            _matchArgs = args;

            MatchResult result = rsid_match_faceprints(_handle, ref args);

            return result;
        }

        // Helper to get FaceRect from IntPtr to faces array passed in the callbacks
        public static FaceRect[] MarshalFaces(IntPtr facesArr, int faceCount)
        {
            // Marshal the IntPtr to array of FaceRects
            var faces = new rsid.FaceRect[faceCount];            
            for (int i = 0; i < faces.Length; i++)
            {
                faces[i] = (rsid.FaceRect)Marshal.PtrToStructure(facesArr, typeof(rsid.FaceRect));
                facesArr += Marshal.SizeOf(typeof(rsid.FaceRect));
            }
            return faces;
        }

        public List<UserFeatures> GetUserFeatures()
        {
            var exported_db = new List<UserFeatures>();
            int number_of_users = 0;
            var status = QueryNumberOfUsers(out number_of_users);
            if (status != Status.Ok)
                return null;
            String[] user_ids = new String[number_of_users];
            status = QueryUserIds(out user_ids);
            if (status != Status.Ok)
                return null;

            foreach (String id in user_ids)
            {
                UserFeatures user_features = new UserFeatures();
                user_features.faceprints = new rsid.Faceprints();
                status = rsid_get_user_features(_handle, id, out user_features.faceprints);
                if (status != Status.Ok)
                    return null;
                user_features.userID = id;
                exported_db.Add(user_features);
            }
            return exported_db;
        }

        public bool SetUserFeatures(rsid.UserFeatures user_features)
        {
            var status = rsid_set_user_features(_handle, user_features.userID, ref user_features.faceprints);
            return (status == Status.Ok);
        }

        private IntPtr _handle;
        private bool _disposed = false;

        private EnrollArgs _enrollArgs;
        private AuthArgs _authArgs;
        private EnrollExtractArgs _enrollExtractArgs;
        private AuthExtractArgs _authExtractArgs;
        private MatchArgs _matchArgs;

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
#if RSID_SECURE
        static extern IntPtr rsid_create_authenticator(ref SignatureCallback signatureCallback);
#else
        static extern IntPtr rsid_create_authenticator();
#endif


        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_authenticator(IntPtr rsid_authenticator);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_connect(IntPtr rsid_authenticator, ref SerialConfig serialConfig);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_set_device_config(IntPtr rsid_authenticator, ref DeviceConfig deviceConfig);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_device_config(IntPtr rsid_authenticator, ref DeviceConfig deviceConfig);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_disconnect(IntPtr rsid_authenticator);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_pair(IntPtr rsid_device_controller, ref PairingArgs pairingArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_unpair(IntPtr rsid_device_controller);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_enroll(IntPtr rsid_authenticator, ref EnrollArgs enrollArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_authenticate(IntPtr rsid_authenticator, ref AuthArgs authArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_authenticate_loop(IntPtr rsid_authenticator, ref AuthArgs authArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_detect_spoof(IntPtr rsid_authenticator, ref AuthArgs authArgs);        

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_cancel(IntPtr rsid_authenticator);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_remove_all_users(IntPtr rsid_authenticator);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_remove_user(IntPtr rsid_authenticator, string userId);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_version();

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_compatible_firmware_version();

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int rsid_is_fw_compatible_with_host(string fw_version);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_number_of_users(IntPtr rsid_authenticator, out int result);


        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_user_ids(
            IntPtr rsid_authenticator,
            [In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] ref StringBuilder[] users,
            [In, Out] ref int result);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_query_user_ids_to_buf(IntPtr rsid_device_controller, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] output, ref int n_users);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_standby(IntPtr rsid_authenticator);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_extract_faceprints_for_enroll(IntPtr rsid_authenticator, ref EnrollExtractArgs enrollExtractArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_extract_faceprints_for_auth(IntPtr rsid_authenticator, ref AuthExtractArgs authExtractArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_extract_faceprints_for_auth_loop(IntPtr rsid_authenticator, ref AuthExtractArgs authArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]

        static extern MatchResult rsid_match_faceprints(IntPtr rsid_authenticator, ref MatchArgs matchArgs);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_get_user_features(IntPtr rsid_authenticator, string userId, out rsid.Faceprints userFeatures);

        [DllImport(Shared.DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern Status rsid_set_user_features(IntPtr rsid_authenticator, string userId, ref rsid.Faceprints userFeatures);
    }

}
