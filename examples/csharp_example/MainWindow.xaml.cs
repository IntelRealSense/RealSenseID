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
        private rsid.AuthStatus _lastAuthHint = rsid.AuthStatus.Serial_Ok; // To show only changed hints. 
        private WriteableBitmap _previewBitmap;

        private static Brush ProgressBrush = Application.Current.TryFindResource("ProgressBrush") as Brush;
        private static Brush FailBrush = Application.Current.TryFindResource("FailBrush") as Brush;
        private static Brush SuccessBrush = Application.Current.TryFindResource("SuccessBrush") as Brush;
        private static int MaxLogSize = 1024 * 5;


        public MainWindow()
        {
            InitializeComponent();


            LoadConfig();
            _preview = new rsid.Preview(_previewConfig);
            ContentRendered += MainWindow_ContentRendered;
            Closing += MainWindow_Closing;
            Microsoft.Win32.SystemEvents.SessionSwitch += SystemEvents_SessionSwitch;
            StateChanged += MainWindow_StateChanged;
        }

        private void LoadConfig()
        {
            var serialPort = Settings.Default.Port;
            var serialType = Settings.Default.SerialType.ToUpper();
            var cameraNumber = Settings.Default.CameraNumber;
            _previewConfig = new rsid.PreviewConfig { cameraNumber = cameraNumber };
            var serType = serialType == "USB" ? rsid.SerialType.USB : rsid.SerialType.UART;
            _serialConfig = new rsid.SerialConfig { port = serialPort, serialType = serType };
        }

        // pause/resume preview on lock/unlock windows session
        private void SystemEvents_SessionSwitch(object sender, Microsoft.Win32.SessionSwitchEventArgs e)
        {
            if (e.Reason == Microsoft.Win32.SessionSwitchReason.SessionUnlock)
                _preview.Resume();
            else if (e.Reason == Microsoft.Win32.SessionSwitchReason.SessionLock)
                _preview.Pause();
        }

        // pause/resume preview on minimize/maximize window
        private void MainWindow_StateChanged(object sender, EventArgs e)
        {
            if (WindowState == WindowState.Minimized)
                _preview.Pause();
            else
                _preview.Resume();
        }

        // Dispatch with background priority
        private void BackgroundDispatch(Action action)
        {
            Dispatcher.BeginInvoke(action, DispatcherPriority.Background, null);
        }

        // Create authenticator 
        private rsid.Authenticator CreateAuthenticator()
        {            
            var handle = rsid_create_example_sig_clbk();            
            var sigCallback = (rsid.SignatureCallback)Marshal.PtrToStructure(handle, typeof(rsid.SignatureCallback));         
            return new rsid.Authenticator(sigCallback);
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            _authenticator = CreateAuthenticator();
            var version = _authenticator.Version();
            ShowLog("rsid v" + version);
            _preview.Start(OnPreview);
        }

        private void MainWindow_Closing(object sender, EventArgs e)
        {
            if (_authloopRunning)
            {
                try
                {
                    _authenticator.Cancel();
                }
                catch { }
            }
        }

        private void OnStartSession(string title)
        {
            Dispatcher.Invoke(() =>
            {
                LogTextBox.Text = title + "\n===========";
                LogScroll.ScrollToEnd();
                EnrollBtn.IsEnabled = false;
                AuthBtn.IsEnabled = false;
                DeleteUsersBtn.IsEnabled = false;
                AuthLoopBtn.IsEnabled = false;
                AuthSettingsBtn.IsEnabled = false;
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
                RedDot.Visibility = Visibility.Hidden;
            });
        }


        private void ShowLog(string message)
        {
            BackgroundDispatch(() =>
            {
                // keep log panel under control
                if (LogTextBox.Text.Length > MaxLogSize)
                {
                    LogTextBox.Text = "";
                }
                // add log line
                LogTextBox.Text += "\n" + message;
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

        private void ShowProgressTitle(string message)
        {
            ShowTitle(message, ProgressBrush);
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

        private void OnEnrollStatus(rsid.EnrollStatus status, IntPtr ctx)
        {
            ShowLog(status.ToString());
            if (status == rsid.EnrollStatus.Success)
            {
                ShowSuccessTitle("Enroll Success");
            }
            else
            {
                ShowFailedStatus("Enroll: " + status.ToString());
            }
        }

        // Authentication callback
        private void OnAuthHint(rsid.AuthStatus hint, IntPtr ctx)
        {
            if (_lastAuthHint != hint)
            {
                _lastAuthHint = hint;
                ShowLog(hint.ToString());
            }
        }

        private void OnAuthStatus(rsid.AuthStatus status, string userId, IntPtr ctx)
        {
            if (status == rsid.AuthStatus.Success)
            {
                ShowLog($"\"{userId}\"");
                ShowSuccessTitle($"{userId}");
            }
            else
            {
                ShowLog(status.ToString());
                ShowFailedStatus(status.ToString());
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
        }

        // Handle preview callback.         
        private void OnPreview(rsid.PreviewImage image, IntPtr ctx)
        {
            if (image.height < 2) return;
            Dispatcher.Invoke(() =>
            {
                var targetWidth = (int)PreviewImage.Width;
                var targetHeight = (int)PreviewImage.Height;

                //creae writable bitmap if not exists or if image size changed
                if (_previewBitmap == null || targetWidth != image.width || targetHeight != image.height)
                {
                    PreviewImage.Width = image.width;
                    PreviewImage.Height = image.height;
                    Console.WriteLine($"Creating new WriteableBitmap preview buffer {image.width}x{image.height}");
                    _previewBitmap = new WriteableBitmap(image.width, image.height, 96, 96, PixelFormats.Bgr24, null);
                    PreviewImage.Source = _previewBitmap;
                    LabelPreview.Visibility = Visibility.Collapsed; // Hide preview placeholder once we have first frame
                }

                Int32Rect sourceRect = new Int32Rect(0, 0, image.width, image.height);
                _previewBitmap.WritePixels(sourceRect, image.buffer, image.size, image.stride);

            }, DispatcherPriority.Render);
        }

        // Enroll Jop
        private void EnrollJob(Object threadContext)
        {
            var userId = threadContext as string;

            OnStartSession($"Enroll \"{userId}\"");
            try
            {
                ShowProgressTitle("Connecting..");
                _authenticator.Connect(_serialConfig);
                ShowProgressTitle("Enrolling..");
                var enrollArgs = new rsid.EnrollArgs { userId = userId, hintClbk = OnEnrollHint, statusClbk = OnEnrollStatus, progressClbk = OnEnrollProgress };
                var status = _authenticator.Enroll(enrollArgs);
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

        // Authenticate job
        private void AuthenticateJob(Object threadContext)
        {
            OnStartSession("Authenticate");
            try
            {
                ShowProgressTitle("Connecting..");
                _authenticator.Connect(_serialConfig);
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, statusClbk = OnAuthStatus, ctx = IntPtr.Zero };
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
            var authConfig = (rsid.AuthConfig)threadContext;
            OnStartSession("SetAuthSettings");
            try
            {
                ShowProgressTitle("Connecting..");
                _authenticator.Connect(_serialConfig);

                ShowProgressTitle("SetAuthSettings " + authConfig.securityLevel.ToString());
                ShowLog("Security " + authConfig.securityLevel.ToString());
                ShowLog(authConfig.cameraRotation.ToString().Replace("_", " "));
                var status = _authenticator.SetAuthSettings(authConfig);

                if (status == rsid.SerialStatus.Ok)
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

        // Toggle auth loop button content (start/stop images and tooltips)
        private void ToggleLoopButton(bool isRunning)
        {
            Dispatcher.Invoke(() =>
            {
                if (isRunning)
                {
                    AuthLoopBtn.Foreground = Brushes.Red;
                    AuthLoopBtn.ToolTip = "Cancel";
                    AuthLoopBtn.Content = "\u275A\u275A";
                    AuthLoopBtn.FontSize = 13;

                }
                else
                {
                    AuthLoopBtn.Foreground = Brushes.White;
                    AuthLoopBtn.ToolTip = "Authentication Loop";
                    AuthLoopBtn.Content = "\u221e";
                    AuthLoopBtn.FontSize = 14;
                }
                AuthLoopBtn.IsEnabled = true;
            });
        }


        // Authentication loop job
        private void AuthenticateLoopJob(Object threadContext)
        {
            OnStartSession("Auth Loop");
            try
            {
                ShowProgressTitle("Connecting..");
                _authenticator.Connect(_serialConfig);
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, statusClbk = OnAuthStatus, ctx = IntPtr.Zero };
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
                var status = _authenticator.Cancel();
                ShowLog($"Cancel status: {status}");
                if (status == rsid.SerialStatus.Ok)
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

        // Wrapper method for use with thread pool.
        private void DeleteUsersJob(Object threadContext)
        {
            OnStartSession("Delete Users");
            try
            {
                ShowProgressTitle("Connecting..");
                _authenticator.Connect(_serialConfig);
                ShowProgressTitle("Deleting..");
                var status = _authenticator.RemoveAllUsers();

                if (status == rsid.SerialStatus.Ok)
                {
                    ShowSuccessTitle("Delete: Ok");
                    ShowLog("Ok");
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
            ThreadPool.QueueUserWorkItem(DeleteUsersJob);
        }

        private void AuthSettings_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new AuthSettingsInput();
            if (dialog.ShowDialog() == true)
            {
                ThreadPool.QueueUserWorkItem(SetAuthSettingsJob, dialog.Config);
            }
        }

        // use the signature callbacks defined in the rsid_signature_example dll
#if DEBUG
        private const string dllName = "rsid_signature_example_debug";
#else
        private const string dllName = "rsid_signature_example";
     
#endif
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_example_sig_clbk();

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_example_sig_clbk(IntPtr rsid_signature_clbk);
    }
}
