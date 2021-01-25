// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using Properties;
using System.Windows.Controls.Primitives;

namespace rsid_wrapper_csharp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private rsid.SerialConfig _serialConfig;
        private rsid.PreviewConfig _previewConfig;
        private rsid.Preview _preview;
        private rsid.Authenticator _authenticator;
        private bool _authloopRunning = false;
        private bool _cancelWasCalled = false;
        private rsid.AuthStatus _lastAuthHint = rsid.AuthStatus.Serial_Ok; // To show only changed hints. 
        private WriteableBitmap _previewBitmap;
        private byte[] _previewBuffer = new byte[0]; // store latest frame from the preview callback        
        private object _previewMutex = new object();
        private string[] _userList; // latest user list that was queried from the device

        private static readonly Brush ProgressBrush = Application.Current.TryFindResource("ProgressBrush") as Brush;
        private static readonly Brush FailBrush = Application.Current.TryFindResource("FailBrush") as Brush;
        private static readonly Brush SuccessBrush = Application.Current.TryFindResource("SuccessBrush") as Brush;
        private static readonly Brush FgColorBrush = Application.Current.TryFindResource("FgColor") as Brush;
        private static readonly int MaxLogSize = 1024 * 5;
        private IntPtr _signatureHelpeHandle = IntPtr.Zero;

        public MainWindow()
        {
            InitializeComponent();

            // start with hidden console
            AllocConsole();
            ToggleConsoleAsync(false);

            ContentRendered += MainWindow_ContentRendered;
            Closing += MainWindow_Closing;
            Microsoft.Win32.SystemEvents.SessionSwitch += SystemEvents_SessionSwitch;            
        }

        private void LoadConfig()
        {
            var serialPort = string.Empty;
            var serType = rsid.SerialType.USB;

            var autoDetect = Settings.Default.AutoDetect;
            if (autoDetect)
            {
                var enumerator = new DeviceEnumerator();
                var enumeration = enumerator.Enumerate();

                if (enumeration.Count == 1)
                {
                    serialPort = enumeration[0].port;
                    serType = enumeration[0].serialType;
                }
                else
                {
                    var msg = "Serial port auto detection failed!\n\n";
                    msg += $"Expected 1 connected port, but found {enumeration.Count}.\n";
                    msg += "Please make sure the device is connected properly.\n\n";
                    msg += "To manually specify the serial port:\n";
                    msg += "  1. Set \"AutoDetect\" to False.\n";
                    msg += "  2. Set \"Port\" to your serial port.\n";
                    msg += "  3. Set \"SerialType\" to either USB or UART.\n";

                    MessageBox.Show(msg, "Configuration Error", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                    Application.Current.Shutdown();
                }
            }
            else
            {
                serialPort = Settings.Default.Port;
                var serialTypeString = Settings.Default.SerialType.ToUpper();
                serType = serialTypeString == "UART" ? rsid.SerialType.UART : rsid.SerialType.USB;
            }

            _serialConfig = new rsid.SerialConfig { port = serialPort, serialType = serType };

            var cameraNumber = Settings.Default.CameraNumber;
            _previewConfig = new rsid.PreviewConfig { cameraNumber = cameraNumber };
        }

        // start/stop preview on lock/unlock windows session
        private void SystemEvents_SessionSwitch(object sender, Microsoft.Win32.SessionSwitchEventArgs e)
        {
            if (e.Reason == Microsoft.Win32.SessionSwitchReason.SessionUnlock)
                _preview.Start(OnPreview);
            else if (e.Reason == Microsoft.Win32.SessionSwitchReason.SessionLock)
                _preview.Stop();
        }
      
        // Dispatch with background priority
        private void BackgroundDispatch(Action action)
        {
            Dispatcher.BeginInvoke(action, DispatcherPriority.Background, null);
        }

        // Dispatch with Render priority
        private void RenderDispatch(Action action)
        {
            try
            {
                Dispatcher.BeginInvoke(action, DispatcherPriority.Render, null);
            }
            catch (Exception ex)
            {
                Console.WriteLine("RenderDispatch: " + ex.Message);
            }
        }

        // Create authenticator 
        private rsid.Authenticator CreateAuthenticator()
        {
            _signatureHelpeHandle = rsid_create_example_sig_clbk();
            var sigCallback = (rsid.SignatureCallback)Marshal.PtrToStructure(_signatureHelpeHandle, typeof(rsid.SignatureCallback));
            return new rsid.Authenticator(sigCallback);
        }


        // show lib and fw version both on left panel and summary in top window title
        private void ShowVersions()
        {
            // show lib version
            var hostVersion = _authenticator.Version();
            ShowLog("Host: v" + hostVersion);
            ShowLog("");

            Title = $"RealSenseID v{hostVersion}";

            // show fw module versions
            using (var controller = new rsid.DeviceController())
            {
                controller.Connect(_serialConfig);
                var fwVersion = controller.QueryFirmwareVersion();
                var versionLines = fwVersion.ToLower().Split('|');

                ShowLog("Firmware:");
                foreach (var v in versionLines)
                {

                    ShowLog(" * " + v);
                    if (v.Contains("opfw"))
                    {
                        var splitted = v.Split(':');
                        if (splitted.Length == 2)
                            Title += $" (firmware {splitted[1]})";
                    }
                }
            }
            ShowLog("");
        }

        private rsid.AuthConfig QueryAuthSettings()
        {
            ShowLog("");
            ShowLog("Query settings..");
            rsid.AuthConfig authConfig;
            var rv = _authenticator.QueryAuthSettings(out authConfig);
            if (rv != rsid.Status.Ok)
            {
                throw new Exception("Query error: " + rv.ToString());
            }

            ShowLog(" * " + authConfig.cameraRotation.ToString());
            ShowLog(" * Security " + authConfig.securityLevel.ToString());
            ShowLog("");
            return authConfig;
        }
        // query user list from the device and update the display
        private void RefreshUserList()
        {
            // Query users and update the user list display            
            ShowLog("Query users..");
            string[] users;
            var rv = _authenticator.QueryUserIds(out users);
            if (rv != rsid.Status.Ok)
            {
                throw new Exception("Query error: " + rv.ToString());
            }

            ShowLog($"{users.Length} users");

            // update the gui and save the list into _userList
            BackgroundDispatch(() =>
            {
                UsersListTxtBox.Text = string.Empty;
                if (users.Any()) UsersListTxtBox.Text = $"{users.Count()} Users\n=========\n";
                foreach (var userId in users)
                {
                    UsersListTxtBox.Text += $"{userId}\n";
                }
            });
            _userList = users;
        }

        // show/hide console
        private void ToggleConsoleAsync(bool show)
        {
            const int SW_HIDE = 0;
            const int SW_SHOW = 5;
            ShowWindow(GetConsoleWindow(), show ? SW_SHOW : SW_HIDE);
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            // load serial port and preview configuration
            LoadConfig();

            // create face authenticator and show version
            _authenticator = CreateAuthenticator();

            // show lib version
            ShowVersions();

            // start preview
            _preview = new rsid.Preview(_previewConfig);
            _preview.Start(OnPreview);

            // pair to the device            
            ThreadPool.QueueUserWorkItem(PairAndInitialQuery);

        }

        private void MainWindow_Closing(object sender, EventArgs e)
        {
            if (_authloopRunning)
            {
                try
                {
                    _cancelWasCalled = true;
                    _authenticator.Cancel();
                }
                catch { }
            }
        }

        private void OnStartSession(string title)
        {
            Dispatcher.Invoke(() =>
            {
                if (!string.IsNullOrEmpty(title)) LogTextBox.Text = title + "\n===========\n";
                LogScroll.ScrollToEnd();
                EnrollBtn.IsEnabled = false;
                AuthBtn.IsEnabled = false;
                DeleteUsersBtn.IsEnabled = false;
                AuthLoopBtn.IsEnabled = false;
                AuthSettingsBtn.IsEnabled = false;
                StandbyBtn.IsEnabled = false;
                _cancelWasCalled = false;
                RedDot.Visibility = Visibility.Visible;
                _lastAuthHint = rsid.AuthStatus.Serial_Ok;
            });
        }

        private void OnStopSession()
        {
            Dispatcher.Invoke(() =>
            {
                EnrollBtn.IsEnabled = true;
                AuthBtn.IsEnabled = true;
                DeleteUsersBtn.IsEnabled = true;
                AuthLoopBtn.IsEnabled = true;
                AuthSettingsBtn.IsEnabled = true;
                StandbyBtn.IsEnabled = true;
                RedDot.Visibility = Visibility.Hidden;
            });
        }


        private void ShowLog(string message)
        {
            BackgroundDispatch(() =>
            {
                // keep log panel size under control
                if (LogTextBox.Text.Length > MaxLogSize)
                {
                    LogTextBox.Text = "";
                }
                // add log line
                LogTextBox.Text += message + "\n";
            });
        }


        private void ShowTitle(string message, Brush color)
        {

            BackgroundDispatch(() =>
            {
                StatusLabel.Content = message;
                StatusLabel.Background = color;
            });
        }

        private void ShowSuccessTitle(string message)
        {
            ShowTitle(message, SuccessBrush);
        }

        private void ShowFailedStatus(string message)
        {
            ShowTitle(message, FailBrush);
        }

        private void ShowFailedStatus(rsid.AuthStatus status)
        {
            // show "Authenticate Failed" message on serial errors
            var msg = (int)status > (int)rsid.AuthStatus.Serial_Ok ? "Authenticate Failed" : status.ToString();
            ShowFailedStatus(msg);
        }

        private void ShowFailedStatus(rsid.EnrollStatus status)
        {
            // show "Enroll Failed" message on serial errors
            var msg = (int)status > (int)rsid.EnrollStatus.Serial_Ok ? "Enroll Failed" : status.ToString();
            ShowFailedStatus(msg);
        }

        private void ShowProgressTitle(string message)
        {
            ShowTitle(message, ProgressBrush);
        }

        private bool ConnectAuth()
        {
            var status = _authenticator.Connect(_serialConfig);
            if (status != rsid.Status.Ok)
            {
                ShowFailedStatus("Connection Error");
                ShowLog("Connection error");
                MessageBox.Show($"Connection Error.\n\nPlease check the serial port setting in the config file.",
                    $"Connection Failed to Port {_serialConfig.port}", MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }
            return true;
        }


        // Enroll callbacks
        private void OnEnrollHint(rsid.EnrollStatus hint, IntPtr ctx)
        {
            ShowLog(hint.ToString());
        }

        private void OnEnrollProgress(rsid.FacePose pose, IntPtr ctx)
        {
            ShowLog(pose.ToString());
        }

        private void OnEnrollResult(rsid.EnrollStatus status, IntPtr ctx)
        {
            ShowLog(status.ToString());
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Enroll Canceled");
                ShowFailedStatus("Canceled");
            }
            else if (status == rsid.EnrollStatus.Success)
            {
                ShowSuccessTitle("Enroll Success");
            }
            else
            {
                ShowFailedStatus(status);
            }
        }

        // Authentication callbacks
        private void OnAuthHint(rsid.AuthStatus hint, IntPtr ctx)
        {
            if (_lastAuthHint != hint)
            {
                _lastAuthHint = hint;
                ShowLog(hint.ToString());
            }
        }

        private void OnAuthResult(rsid.AuthStatus status, string userId, IntPtr ctx)
        {
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Authentication Canceled");
                ShowFailedStatus("Canceled");
            }
            else if (status == rsid.AuthStatus.Success)
            {
                ShowLog($"Success \"{userId}\"");
                ShowSuccessTitle($"{userId}");
            }
            else
            {
                ShowLog(status.ToString());
                ShowFailedStatus(status);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
        }


        private void UIHandlePreview(int width, int height, int stride)
        {
            var targetWidth = (int)PreviewImage.Width;
            var targetHeight = (int)PreviewImage.Height;

            //creae writable bitmap if not exists or if image size changed
            if (_previewBitmap == null || targetWidth != width || targetHeight != height)
            {
                PreviewImage.Width = width;
                PreviewImage.Height = height;
                Console.WriteLine($"Creating new WriteableBitmap preview buffer {width}x{height}");
                _previewBitmap = new WriteableBitmap(width, height, 96, 96, PixelFormats.Bgr24, null);
                PreviewImage.Source = _previewBitmap;
                // Hide preview placeholder once we have first frame
                LabelPreview.Visibility = Visibility.Collapsed;
            }
            Int32Rect sourceRect = new Int32Rect(0, 0, width, height);
            lock (_previewMutex)
            {
                _previewBitmap.WritePixels(sourceRect, _previewBuffer, stride, 0);
            }
        }

        // Handle preview callback.         
        private void OnPreview(rsid.PreviewImage image, IntPtr ctx)
        {
            if (image.height < 2) return;

            lock (_previewMutex)
            {
                if (_previewBuffer.Length != image.size)
                {
                    Console.WriteLine("Creating preview buffer");
                    _previewBuffer = new byte[image.size];
                }
                Marshal.Copy(image.buffer, _previewBuffer, 0, image.size);
            }
            RenderDispatch(() => UIHandlePreview(image.width, image.height, image.stride));
        }


        // Pairing to device job and query auth settings and users
        private void PairAndInitialQuery(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession(string.Empty);
            ShowLog("Pairing..");
            IntPtr pairArgsHandle = IntPtr.Zero;
            try
            {
                pairArgsHandle = rsid_create_pairing_args_example(_signatureHelpeHandle);
                var pairingArgs = (rsid.PairingArgs)Marshal.PtrToStructure(pairArgsHandle, typeof(rsid.PairingArgs));

                var rv = _authenticator.Pair(ref pairingArgs);
                if (rv != rsid.Status.Ok)
                {
                    throw new Exception("Failed pairing");
                }
                ShowLog("Pairing Ok");
                rsid_update_device_pubkey_example(_signatureHelpeHandle, Marshal.UnsafeAddrOfPinnedArrayElement(pairingArgs.DevicePubkey, 0));

                QueryAuthSettings();
                RefreshUserList();

            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                if (pairArgsHandle != IntPtr.Zero) rsid_destroy_pairing_args_example(pairArgsHandle);
                OnStopSession();
                _authenticator.Disconnect();
            }
        }

        // Enroll Job
        private void EnrollJob(Object threadContext)
        {
            var userId = threadContext as string;

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll \"{userId}\"");
            IntPtr userIdCtx = Marshal.StringToHGlobalUni(userId);
            try
            {
                ShowProgressTitle("Enrolling..");
                var enrollArgs = new rsid.EnrollArgs
                {
                    userId = userId,
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollResult,
                    progressClbk = OnEnrollProgress,
                    ctx = userIdCtx
                };
                var status = _authenticator.Enroll(enrollArgs);
                if (status == rsid.Status.Ok) RefreshUserList();
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
                Marshal.FreeHGlobal(userIdCtx);
            }
        }

        // Authenticate job
        private void AuthenticateJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Authenticate");
            try
            {
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, resultClbk = OnAuthResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Authenticating..");
                _authenticator.Authenticate(authArgs);
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }

        // SetAuthSettings job
        private void SetAuthSettingsJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            var authConfig = (rsid.AuthConfig)threadContext;
            OnStartSession("SetAuthSettings");
            try
            {
                ShowProgressTitle("SetAuthSettings " + authConfig.securityLevel.ToString());
                ShowLog("Security " + authConfig.securityLevel.ToString());
                ShowLog(authConfig.cameraRotation.ToString().Replace("_", " "));
                var status = _authenticator.SetAuthSettings(authConfig);

                if (status == rsid.Status.Ok)
                {
                    ShowSuccessTitle("AuthSettings Done");
                    ShowLog("Ok");
                }
                else
                {
                    ShowFailedStatus("SetAuthSettings: Failed");
                    ShowLog("Failed");
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }

        private void StandbyJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("SetAuthSettings");
            try
            {
                ShowProgressTitle("Standby");
                ShowLog("Security..");
                var status = _authenticator.Standby();

                if (status == rsid.Status.Ok)
                {
                    ShowSuccessTitle("Standby Done");
                    ShowLog("Ok");
                }
                else
                {
                    ShowFailedStatus("Standby: Failed");
                    ShowLog("Failed");
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }


        // Toggle auth loop button content (start/stop images and tooltips)
        private void ToggleLoopButton(bool isRunning)
        {
            Dispatcher.Invoke(() =>
            {
                if (isRunning)
                {
                    AuthLoopBtn.Foreground = Brushes.Red;
                    AuthLoopBtn.ToolTip = "Cancel";
                    AuthLoopBtn.Content = "\u275A\u275A"; //  "pause" symbol
                    AuthLoopBtn.FontSize = 13;

                }
                else
                {
                    AuthLoopBtn.Foreground = FgColorBrush;
                    AuthLoopBtn.ToolTip = "Authentication Loop";
                    AuthLoopBtn.Content = "\u221e"; // the "infinite" symbol
                    AuthLoopBtn.FontSize = 14;
                }
                AuthLoopBtn.IsEnabled = true;
            });
        }


        // Authentication loop job
        private void AuthenticateLoopJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("Auth Loop");
            try
            {
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, resultClbk = OnAuthResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Authenticating..");
                _authloopRunning = true;
                ToggleLoopButton(true);
                _authenticator.AuthenticateLoop(authArgs);
            }
            catch (Exception ex)
            {
                try
                {
                    _authenticator.Cancel(); //try to cancel the auth loop
                }
                catch { }
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                ToggleLoopButton(false);
                OnStopSession();
                _authloopRunning = false;
                _authenticator.Disconnect();
            }

        }

        // Cancel job
        private void CancelJob(Object threadContext)
        {
            try
            {
                ShowProgressTitle("Cancel..");
                ShowLog("Cancel..");
                _cancelWasCalled = true;
                var status = _authenticator.Cancel();
                ShowLog($"Cancel status: {status}");
                if (status == rsid.Status.Ok)
                {
                    ShowSuccessTitle("Cancel Ok");
                }
                else
                {
                    ShowFailedStatus("Cancel Failed");
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
        }

        private void DeleteUsersJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Delete Users");
            try
            {
                ShowProgressTitle("Deleting..");
                var status = _authenticator.RemoveAllUsers();

                if (status == rsid.Status.Ok)
                {
                    ShowSuccessTitle("Delete: Ok");
                    ShowLog("Ok");
                    RefreshUserList();
                }
                else
                {
                    ShowFailedStatus("Delete: Failed");
                    ShowLog("Failed");
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }

        // Wrapper method for use with thread pool.
        private void DeleteSingleUserJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            var userId = (string)threadContext;
            OnStartSession($"Delete \"{userId}\"");

            try
            {
                ShowProgressTitle("Deleting..");
                var status = _authenticator.RemoveUser(userId);

                if (status == rsid.Status.Ok)
                {
                    ShowSuccessTitle("Delete: Ok");
                    ShowLog("Ok");
                    RefreshUserList();
                }
                else
                {
                    ShowFailedStatus("Delete: Failed");
                    ShowLog("Failed");
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }
        //
        //
        // Button Handlers
        //
        private void Auth_Click(object sender, RoutedEventArgs e)
        {

            ThreadPool.QueueUserWorkItem(AuthenticateJob);
        }

        private void AuthLoop_Click(object sender, RoutedEventArgs e)
        {
            if (_authloopRunning)
            {
                // cancel auth loop.
                // disable cancel button until canceled.
                AuthLoopBtn.IsEnabled = false;
                ThreadPool.QueueUserWorkItem(CancelJob);
            }
            else
            {
                // start auth loop.
                // enable cancel button until canceled.
                AuthLoopBtn.IsEnabled = true;
                ThreadPool.QueueUserWorkItem(AuthenticateLoopJob);
            }

        }

        private void Enroll_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new EnrollInput();
            if (dialog.ShowDialog() == true)
            {
                ThreadPool.QueueUserWorkItem(EnrollJob, dialog.EnrolledUsername);
            }
        }

        private void DeleteUsers_Click(object sender, RoutedEventArgs e)
        {

            var dialog = new DeleteUserInput(_userList);
            if (dialog.ShowDialog() == true)
            {
                if (dialog.DeleteAll)
                    ThreadPool.QueueUserWorkItem(DeleteUsersJob);
                else if (!string.IsNullOrWhiteSpace(dialog.SelectedUser))
                    ThreadPool.QueueUserWorkItem(DeleteSingleUserJob, dialog.SelectedUser);
            }
        }

        private void AuthSettings_Click(object sender, RoutedEventArgs e)
        {
            if (!ConnectAuth()) return;
            var authConfig = QueryAuthSettings();
            var dialog = new AuthSettingsInput(authConfig);
            if (dialog.ShowDialog() == true)
            {
                ThreadPool.QueueUserWorkItem(SetAuthSettingsJob, dialog.Config);
            }
        }

        private void StandbyBtn_Click(object sender, RoutedEventArgs e)
        {
            ThreadPool.QueueUserWorkItem(StandbyJob);
        }

        // show/hide console
        private void ShowLogChkbox_Click(object sender, RoutedEventArgs e)
        {
            ToggleConsoleAsync(ShowLogCheckbox.IsChecked.GetValueOrDefault());
        }

        private void PreviewImage_MouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            LabelPlayStop.Visibility = LabelPlayStop.Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;
            if (LabelPlayStop.Visibility == Visibility.Hidden)
            {
                _preview.Start(OnPreview);
                PreviewImage.Opacity = 1.0;
            }
            else
            {
                _preview.Stop();
                PreviewImage.Opacity = 0.7;
            }
        }

        // Debug console support
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool AllocConsole();

        [DllImport("kernel32.dll")]
        static extern IntPtr GetConsoleWindow();

        [DllImport("user32.dll")]
        static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

#if DEBUG
        private const string dllName = "rsid_signature_example_debug";
#else
        private const string dllName = "rsid_signature_example";
#endif        
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_example_sig_clbk();

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_example_sig_clbk(IntPtr rsid_signature_clbk);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_get_host_pubkey_example(IntPtr rsid_signature_clbk);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_update_device_pubkey_example(IntPtr rsid_signature_clbk, IntPtr device_key);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_pairing_args_example(IntPtr rsid_signature_clbk);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_pairing_args_example(IntPtr pairing_args);
    }
}