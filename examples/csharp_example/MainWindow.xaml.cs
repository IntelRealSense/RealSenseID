// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Microsoft.Win32;
using Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Threading;

namespace rsid_wrapper_csharp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public enum FlowMode
        {
            Device,
            Server
        }

        private static readonly Brush ProgressBrush = Application.Current.TryFindResource("ProgressBrush") as Brush;
        private static readonly Brush FailBrush = Application.Current.TryFindResource("FailBrush") as Brush;
        private static readonly Brush SuccessBrush = Application.Current.TryFindResource("SuccessBrush") as Brush;
        private static readonly int _userIdLength = 15;
        private static readonly float _updateLoopInterval = 0.05f;
        private static readonly float _userFeedbackDuration = 3.0f;
        private static readonly rsid.FacePose[] _enrollSequence = new rsid.FacePose[] { rsid.FacePose.Center, rsid.FacePose.Left, rsid.FacePose.Right };
        private static readonly string[] _enrollSequenceInstructions = new string[]
        {
            "Look forward",
            "Look slightly to left",
            "Look slightly to right",
        };

        private DeviceState _deviceState;
        private rsid.Authenticator _authenticator;
        private FlowMode _flowMode;

        private rsid.Preview _preview;
        private WriteableBitmap _previewBitmap;
        private byte[] _previewBuffer = new byte[0]; // store latest frame from the preview callback        
        private object _previewMutex = new object();

        private string[] _userList = new string[0]; // latest user list that was queried from the device

        private bool _authloopRunning = false;
        private bool _cancelWasCalled = false;
        private string _lastEnrolledUserId;
        private List<rsid.FacePose> _detectedEnrollmenetPoses = new List<rsid.FacePose>();
        private rsid.AuthStatus _lastAuthHint = rsid.AuthStatus.Serial_Ok; // To show only changed hints. 

        private IntPtr _mutableFaceprintsHandle = IntPtr.Zero;

        private IntPtr _signatureHelpeHandle = IntPtr.Zero;
        private readonly Database _db = new Database();

        private string _dumpDir;
        private bool _debugMode;
        private ConsoleWindow _consoleWindow;
        private ProgressBarDialog _progressBar;

        private float _userFeedbackTime = 0;

        public MainWindow()
        {
            InitializeComponent();

            CreateConsole();

            ContentRendered += MainWindow_ContentRendered;
            Closing += MainWindow_Closing;

            DispatcherTimer timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromMilliseconds(_updateLoopInterval * 1000);
            timer.Tick += Timer_Tick;
            timer.Start();
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            // load serial port and preview configuration
            LoadConfig();

            // creating console window
            //_consoleWindow = new ConsoleWindow();

            // create face authenticator and show version
            _authenticator = CreateAuthenticator();

            // pair to the device       
            OnStartSession(string.Empty);
            ClearTitle();
            ThreadPool.QueueUserWorkItem(InitialSession);
        }

        private void MainWindow_Closing(object sender, EventArgs e)
        {
            _consoleWindow?.Disable();
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

        private void EnrollButton_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new EnrollInput();
            if (dialog.ShowDialog() == true)
            {
                EnrollPanel.Visibility = Visibility.Visible;

                if (_flowMode == FlowMode.Server)
                    ThreadPool.QueueUserWorkItem(EnrollExtractFaceprintsJob, dialog.Username);
                else
                    ThreadPool.QueueUserWorkItem(EnrollJob, dialog.Username);
            }
        }

        private void CancelEnrollButton_Click(object sender, RoutedEventArgs e)
        {
            ThreadPool.QueueUserWorkItem(CancelJob);
            EnrollPanel.Visibility = Visibility.Collapsed;
        }

        private void AuthenticateButton_Click(object sender, RoutedEventArgs e)
        {
            AuthenticationPanel.Visibility = Visibility.Visible;
            bool isLoop = AuthenticateLoopToggle.IsChecked.GetValueOrDefault();
            if (_flowMode == FlowMode.Server)
            {
                if (isLoop == true)
                {
                    ThreadPool.QueueUserWorkItem(AuthenticateExtractFaceprintsLoopJob);
                }
                else
                {
                    ThreadPool.QueueUserWorkItem(AuthenticateExtractFaceprintsJob);
                }
            }
            else
            {
                if (isLoop == true)
                {
                    ThreadPool.QueueUserWorkItem(AuthenticateLoopJob);
                }
                else
                {
                    ThreadPool.QueueUserWorkItem(AuthenticateJob);
                }
            }
        }

        private void UsersListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            DeleteButton.IsEnabled = UsersListView.SelectedItems.Count > 0;
        }

        private void SelectAllUsersCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            UsersListView.SelectAll();
        }

        private void SelectAllUsersCheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            UsersListView.UnselectAll();
        }

        private void CancelAuthenticationButton_Click(object sender, RoutedEventArgs e)
        {
            ThreadPool.QueueUserWorkItem(CancelJob);
            AuthenticationPanel.Visibility = Visibility.Collapsed;
        }

        private void DeleteButton_Click(object sender, RoutedEventArgs e)
        {
            List<string> usersToDelete = UsersListView.SelectedItems.Cast<string>().ToList();
            bool deleteAll = _userList.Length == usersToDelete.Count;

            var dialog = new DeleteUserInput();
            if (dialog.ShowDialog() == true)
            {
                if (_flowMode == FlowMode.Server)
                {
                    if (deleteAll == true)
                    {
                        ThreadPool.QueueUserWorkItem(DeleteUsersServerJob);
                    }
                    else
                    {
                        ThreadPool.QueueUserWorkItem(DeleteSingleUserServerJob, usersToDelete);
                    }
                }
                else
                {
                    if (deleteAll == true)
                    {
                        ThreadPool.QueueUserWorkItem(DeleteUsersJob);
                    }
                    else
                    {
                        ThreadPool.QueueUserWorkItem(DeleteSingleUserJob, usersToDelete);
                    }
                }

            }
        }

        private void SettingsButton_Click(object sender, RoutedEventArgs e)
        {
            // different behavior when in recovery/operational modes

            rsid.AuthConfig? authConfig = null;
            if (_deviceState.IsOperational)
            {
                // device is in operational mode, we continue to query settings as usual
                if (!ConnectAuth()) return;
                authConfig = QueryAuthSettings();
                _authenticator.Disconnect();
            }
            else
            {
                // device is in recovery mode, we attempt to detect its settings again
                try
                {
                    _deviceState = DetectDevice();
                }
                catch (Exception ex)
                {
                    OnStopSession();
                    ShowErrorMessage("Connection Error", ex.Message);
                    return;
                }
            }

            var dialog = new AuthSettingsInput(_deviceState.FirmwareVersion, authConfig, _flowMode);
            if (dialog.ShowDialog() == true)
            {
                if (string.IsNullOrEmpty(dialog.FirmwareFileName) == true)
                {
                    ThreadPool.QueueUserWorkItem(SetAuthSettingsJob, (dialog.Config, dialog.FlowMode));
                }
                else
                {
                    TabsControl.SelectedIndex = 1;  // Switch to logs tab
                    _progressBar = new ProgressBarDialog();
                    ThreadPool.QueueUserWorkItem(FwUpdateJob, dialog.FirmwareFileName);
                }
            }
        }

        private void StandbyButton_Click(object sender, RoutedEventArgs e)
        {
            ThreadPool.QueueUserWorkItem(StandbyJob);
        }

        private void ClearLogButton_Click(object sender, RoutedEventArgs e)
        {
            ClearLog();
        }

        private void OpenConsoleToggle_Click(object sender, RoutedEventArgs e)
        {
            _consoleWindow?.ToggleVisibility();
            ToggleConsoleAsync(OpenConsoleToggle.IsChecked.GetValueOrDefault());
        }

        private void TogglePreviewOpacity(bool isActive)
        {
            PreviewImage.Opacity = isActive ? 1.0 : 0.85;
        }

        private void PreviewImage_MouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            LabelPlayStop.Visibility = LabelPlayStop.Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;
            if (LabelPlayStop.Visibility == Visibility.Hidden)
            {
                _preview.Start(OnPreview);
                TogglePreviewOpacity(true);
            }
            else
            {
                _preview.Stop();
                TogglePreviewOpacity(false);
            }
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            if (_userFeedbackTime > 0)
            {
                _userFeedbackTime -= _updateLoopInterval;
                if (_userFeedbackTime < 2.0f)
                {
                    UserFeedbackContainer.Opacity -= _updateLoopInterval * 0.5;
                }
                if (_userFeedbackTime <= 0)
                {
                    ClearTitle();
                }
            }
        }

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

        private void ShowErrorMessage(string title, string message)
        {
            Dispatcher.Invoke(() => (new ErrorDialog(title, message)).ShowDialog());
        }

        private void ClearLog()
        {
            LogTextBox.Text = "";
            OutputText.Text = "";
            LogScroll.ScrollToEnd();
        }

        private void ShowLogTitle(string title)
        {
            if (string.IsNullOrEmpty(title) == false)
            {
                LogTextBox.Text += $"\n{title}\n===========\n";
                OutputText.Text = title;
            }
            LogScroll.ScrollToEnd();
        }

        private void ShowLog(string message)
        {
            BackgroundDispatch(() =>
            {
                // add log line
                LogTextBox.Text += message + "\n";
                OutputText.Text = message;
            });
        }

        private void ShowTitle(string message, Brush color, float duration = 0)
        {
            BackgroundDispatch(() =>
            {
                _userFeedbackTime = duration;
                UserFeedbackText.Text = message;
                UserFeedbackPanel.Background = color;
                UserFeedbackContainer.Visibility = Visibility.Visible;
                UserFeedbackContainer.Opacity = 1.0f;
            });
        }

        private void ClearTitle()
        {
            BackgroundDispatch(() =>
            {
                UserFeedbackContainer.Visibility = Visibility.Collapsed;
            });
        }

        private void ShowSuccessTitle(string message)
        {
            ShowTitle(message, SuccessBrush, _userFeedbackDuration);
        }

        private void ShowFailedStatus(string message)
        {
            ShowTitle(message, FailBrush, _userFeedbackDuration);
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

        private void VerifyResult(bool result, string successMessage, string failMessage, Action onSuccess = null)
        {
            if (result)
            {
                ShowSuccessTitle(successMessage);
                ShowLog("Ok");
                onSuccess?.Invoke();
            }
            else
            {
                ShowFailedStatus(failMessage);
                ShowLog("Failed");
            }
        }

        private void UpdateProgressBar(float progress)
        {
            BackgroundDispatch(() =>
            {
                _progressBar.Update(progress);
            });
        }

        private void CloseProgressBar()
        {
            BackgroundDispatch(() =>
            {
                _progressBar.Close();
                _progressBar = null;
            });
        }

        private void SetUIEnabled(bool isEnabled)
        {
            SettingsButton.IsEnabled = isEnabled;
            DeleteButton.IsEnabled = isEnabled && UsersListView.SelectedItems.Count > 0;
            EnrollButton.IsEnabled = isEnabled;
            AuthenticateButton.IsEnabled = isEnabled;
            StandbyButton.IsEnabled = isEnabled && (_flowMode != FlowMode.Server);
            UsersListView.IsEnabled = isEnabled;
            SelectAllUsersCheckBox.IsEnabled = isEnabled && _userList != null && _userList.Length > 0;
        }

        private FlowMode StringToFlowMode(string flowModeString)
        {
            if (flowModeString == "SERVER")
            {
                ShowLog("Server Mode");
                return FlowMode.Server;
            }
            else if (flowModeString == "DEVICE")
            {
                ShowLog("Device Mode\n");
                return FlowMode.Device;
            }

            ShowFailedStatus("Mode " + flowModeString + " not supported, using Device Mode instead");
            ShowLog("Device Mode\n");
            return FlowMode.Device;
        }

        private void GetNextEnrollmentPoseAndInstructions(out rsid.FacePose nextPose, out string nextInstructions)
        {
            nextPose = rsid.FacePose.Center;
            nextInstructions = "";
            for (int i = 0; i < _enrollSequence.Length; ++i)
            {
                if (_detectedEnrollmenetPoses.Contains(_enrollSequence[i]) == false)
                {
                    nextPose = _enrollSequence[i];
                    nextInstructions = _enrollSequenceInstructions[i];
                    return;
                }
            }
        }

        private void LoadConfig()
        {
            _debugMode = Settings.Default.DebugMode;
            _dumpDir = Settings.Default.DumpDir;

            if (_debugMode)
            {
                Directory.CreateDirectory(_dumpDir);
            }

            _flowMode = StringToFlowMode(Settings.Default.FlowMode.ToUpper());
            if (_flowMode == FlowMode.Server)
            {
                StandbyButton.IsEnabled = false;
                _db.Load();
            }
        }

        private static bool IsValidUserId(string user)
        {
            return !string.IsNullOrEmpty(user) && user.All((ch) => ch <= 127) && user.Length <= 15;
        }

        // Create authenticator
        private rsid.Authenticator CreateAuthenticator()
        {
#if RSID_SECURE
            _signatureHelpeHandle = rsid_create_example_sig_clbk();
            var sigCallback = (rsid.SignatureCallback)Marshal.PtrToStructure(_signatureHelpeHandle, typeof(rsid.SignatureCallback));
            return new rsid.Authenticator(sigCallback);
#else
            return new rsid.Authenticator();
#endif //RSID_SECURE

        }

        private bool ConnectAuth()
        {
            var status = _authenticator.Connect(_deviceState.SerialConfig);
            if (status != rsid.Status.Ok)
            {
                ShowFailedStatus("Connection Error");
                ShowLog("Connection error");
                ShowErrorMessage($"Connection Failed to Port {_deviceState.SerialConfig.port}",
                    $"Connection Error.\n\nPlease check the serial port setting in the config file.");
                return false;
            }
            return true;
        }

        private rsid.AuthConfig? QueryAuthSettings()
        {
            ShowLog("");
            ShowLog("Query settings..");
            rsid.AuthConfig authConfig;
            var rv = _authenticator.QueryAuthSettings(out authConfig);
            if (rv != rsid.Status.Ok)
            {
                //throw new Exception("Query error: " + rv.ToString());
                ShowLog("Query error: " + rv.ToString());
                ShowFailedStatus("Query error: " + rv.ToString());
                return null;
            }

            ShowLog(" * Confidence Level: " + authConfig.securityLevel.ToString());
            ShowLog(" * Camera Rotation: " + authConfig.cameraRotation.ToString());
            ShowLog(" * Host Mode: " + _flowMode);
            ShowLog(" * Camera Index: " + _deviceState.PreviewConfig.cameraNumber);
            ShowLog("");
            return authConfig;
        }

        private void ShowMatchResult(rsid.MatchResult matchResult, string userId)
        {
            ShowLog($"MatchResult matchResult: {matchResult} \"{userId}\"");
            if (matchResult.success == 1)
            {
                ShowSuccessTitle(userId);
            }
            else
            {
                ShowFailedStatus("Faceprints extracted but did not match any user");
                ShowLog("Faceprints extracted but did not match any user");
            }
        }

        private bool GetNextFaceprints(out rsid.Faceprints newFaceprints, out string userId)
        {
            var db_result = _db.GetNext();
            newFaceprints = db_result.Item1;
            userId = String.Copy(db_result.Item2);
            var isDone = db_result.Item3;
            return (!isDone);
        }

        private void Match(rsid.Faceprints faceprintsToMatch)
        {
            try
            {
                ShowProgressTitle("Matching faceprints to database");
                _db.ResetIndex();
                string userIdDb = new string('\0', _userIdLength + 1);
                rsid.Faceprints updatedFaceprints = new rsid.Faceprints();
                rsid.Faceprints faceprintsDb = new rsid.Faceprints();
                rsid.MatchResult matchResult = new rsid.MatchResult { success = 0, shouldUpdate = 0 };

                while (GetNextFaceprints(out faceprintsDb, out userIdDb))
                {
                    var matchArgs = new rsid.MatchArgs { newFaceprints = faceprintsToMatch, existingFaceprints = faceprintsDb, updatedFaceprints = updatedFaceprints };
                    var matchResultHandle = _authenticator.MatchFaceprintsToFaceprints(matchArgs);

                    matchResult = (rsid.MatchResult)Marshal.PtrToStructure(matchResultHandle, typeof(rsid.MatchResult));

                    if (matchResult.success == 1)
                        break;
                }

                ShowMatchResult(matchResult, userIdDb);
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
        }

        private bool ByteArrayToFile(string fileName, byte[] byteArray)
        {
            try
            {
                using (var fs = new FileStream(fileName, FileMode.Create, FileAccess.Write))
                {
                    Console.WriteLine($"saving file {fileName}");
                    fs.Write(byteArray, 0, byteArray.Length);
                    return true;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception caught in process: {0}", ex);
                return false;
            }
        }

        private bool HandleRawImage(rsid.PreviewImage image)
        {
            int timeStamp = Marshal.ReadInt32(image.buffer);
            if (timeStamp == 0)
                return false;

            byte status = Marshal.ReadByte(image.buffer, 4);
            byte metaData = Marshal.ReadByte(image.buffer, 5);

            int sensorId = (metaData) & 1;
            int led = (metaData) & (1 << 1);
            int projector = (metaData) & (1 << 2);

            var sensorStr = (sensorId != 0) ? "right" : "left";
            var ledStr = (led != 0) ? "led_on" : "led_off";
            var projectorStr = (projector != 0) ? "projector_on" : "projector_off";


            var byteArray = new Byte[image.size];
            Marshal.Copy(image.buffer, byteArray, 0, image.size);
            //var filePath = "timestamp_" + timeStamp + "_status_" + status.ToString() +"_" + sensorStr + "_" + projectorStr + "_" + ledStr + ".w10";            
            var filename = $"timestamp_{timeStamp}_status_{status}_{sensorStr}_{projectorStr}_{ledStr}.w10";
            var fullPath = Path.Combine(_dumpDir, filename);
            ByteArrayToFile(fullPath, byteArray);
            return true;
        }

        private void UIHandlePreview(int width, int height, int stride)
        {
            var targetWidth = (int)PreviewImage.Width;
            var targetHeight = (int)PreviewImage.Height;

            //create writable bitmap if not exists or if image size changed
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
            // If in dump frame mode, save the raw image to disk instead of displaying
            if (_deviceState.PreviewConfig.debugMode == 1)
            {
                HandleRawImage(image);
                return;
            }

            if (image.height < 2) return;

            // Copy frame to managed buffer and display it
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

        private void OnStartSession(string title)
        {
            Dispatcher.Invoke(() =>
            {
                ShowLogTitle(title);
                SetUIEnabled(false);
                RedDot.Visibility = Visibility.Visible;
                _cancelWasCalled = false;
                _lastAuthHint = rsid.AuthStatus.Serial_Ok;
            });
        }

        private void OnStopSession()
        {
            Dispatcher.Invoke(() =>
            {
                SetUIEnabled(true);
                RedDot.Visibility = Visibility.Hidden;
            });
        }

        // Enroll callbacks
        private void OnEnrollHint(rsid.EnrollStatus hint, IntPtr ctx)
        {
            ShowLog(hint.ToString());
        }

        private void OnEnrollProgress(rsid.FacePose pose, IntPtr ctx)
        {
            ShowLog(pose.ToString());
            _detectedEnrollmenetPoses.Add(pose);
            GetNextEnrollmentPoseAndInstructions(out _, out string enrollInsutctions);
            ShowProgressTitle(enrollInsutctions);
        }

        private void OnEnrollResult(rsid.EnrollStatus status, IntPtr ctx)
        {
            ShowLog($"OnEnrollResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                VerifyResult(status == rsid.EnrollStatus.Success, "Enroll success", "Enroll failed");
            }
        }

        private void OnEnrollExtractionResult(rsid.EnrollStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnEnrollExtractionResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                VerifyResult(status == rsid.EnrollStatus.Success, "Enroll success", "Enroll failed", () =>
                {
                    var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                    if (_db.Push(faceprints, _lastEnrolledUserId))
                        _db.Save();
                    RefreshUserListServer();
                });
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
            if (hint == rsid.AuthStatus.NoFaceDetected)
            {
                ShowFailedStatus("No face detected");
            }
        }

        private void OnAuthResult(rsid.AuthStatus status, string userId, IntPtr ctx)
        {
            ShowLog($"OnAuthResult status: {status} \"{userId}\"");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No face detected" : status.ToString();
                VerifyResult(status == rsid.AuthStatus.Success, $"{userId}", failMessage);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
        }

        public void OnAuthLoopExtractionResult(rsid.AuthStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnAuthLoopExtractionResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else if (status == rsid.AuthStatus.Success)
            {
                var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                Match(faceprints);
            }
            else
            {
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No face detected" : status.ToString();
                ShowFailedStatus(failMessage);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
        }

        private void OnAuthExtractionResult(rsid.AuthStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnAuthExtractionResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else if (status == rsid.AuthStatus.Success)
            {
                var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                Match(faceprints);
            }
            else
            {
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No face detected" : status.ToString();
                ShowFailedStatus(failMessage);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
        }

        private void HideEnrollingLabelPanel()
        {
            Dispatcher.Invoke(() =>
            {
                EnrollPanel.Visibility = Visibility.Collapsed;
            });
        }

        private void HideAuthenticatingLabelPanel()
        {
            Dispatcher.Invoke(() =>
            {
                AuthenticationPanel.Visibility = Visibility.Collapsed;
            });
        }

        private void SetInstructionsToRefreshUsers(bool isRefresh)
        {
            string text = isRefresh ? "Updating users list" : "Press Enroll to add users";
            Dispatcher.Invoke(() =>
            {
                InstructionsEnrollUsers.Text = text;
            });
        }

        private void UpdateUsersUIList(string[] users)
        {
            UsersListView.ItemsSource = users.ToList<string>();
            UsersListView.UnselectAll();
            DeleteButton.IsEnabled = false;
            SelectAllUsersCheckBox.IsChecked = false;
            int usersCount = users.Length;
            UsersTab.Header = $"Users ({usersCount})";
            if (usersCount > 0)
            {
                InstructionsEnrollUsers.Visibility = Visibility.Collapsed;
                SelectAllUsersCheckBox.IsEnabled = true;
                AuthenticateButton.IsEnabled = true;
                AuthenticateLoopToggle.IsEnabled = true;
            }
            else
            {
                InstructionsEnrollUsers.Visibility = Visibility.Visible;
                SelectAllUsersCheckBox.IsEnabled = false;
                AuthenticateButton.IsEnabled = false;
                AuthenticateLoopToggle.IsEnabled = false;
            }
        }

        // query user list from the device and update the display
        private void RefreshUserList()
        {
            // Query users and update the user list display            
            ShowLog("Query users..");
            SetInstructionsToRefreshUsers(true);
            string[] users;
            var rv = _authenticator.QueryUserIds(out users);
            if (rv != rsid.Status.Ok)
            {
                throw new Exception("Query error: " + rv.ToString());
            }

            ShowLog($"{users.Length} users");

            // update the gui and save the list into _userList
            SetInstructionsToRefreshUsers(false);
            BackgroundDispatch(() =>
            {
                UpdateUsersUIList(users);
            });
            _userList = users;
        }

        private void RefreshUserListServer()
        {
            // Query users and update the user list display            
            ShowLog("Query users..");
            SetInstructionsToRefreshUsers(true);
            string[] users;
            _db.GetUserIds(out users);
            ShowLog($"{users.Length} users");

            // update the gui and save the list into _userList
            SetInstructionsToRefreshUsers(false);
            BackgroundDispatch(() =>
            {
                UpdateUsersUIList(users);
            });
            _userList = users;
        }

        private string QueryMetadata(rsid.SerialConfig config)
        {
            using (var controller = new rsid.DeviceController())
            {
                ShowLog("Querying metadata...");
                var status = controller.Connect(config);
                if (status != rsid.Status.Ok)
                {
                    ShowLog("Failed\n");
                    return ("Unknown");
                }

                ShowLog("Success\n");

                var fwVersion = controller.QueryFirmwareVersion();
                var sn = controller.QuerySerialNumber();

                ShowLog("S/N: " + sn);

                var versionLines = fwVersion.ToLower().Split('|');
                ShowLog("Firmware:");
                foreach (var v in versionLines)
                {
                    ShowLog(" * " + v);
                    if (v.Contains("opfw"))
                    {
                        var splitted = v.Split(':');
                        if (splitted.Length == 2)
                            fwVersion = splitted[1];
                    }
                }

                ShowLog("");

                return (fwVersion == String.Empty ? "Unknown" : fwVersion);
            }
        }

        private (bool isConnectionWorking, bool isOperational) TestConnection(rsid.SerialConfig config)
        {
            using (var controller = new rsid.DeviceController())
            {
                ShowLog("Testing connection... ");
                var status = controller.Connect(config);
                if (status != rsid.Status.Ok)
                {
                    ShowLog("Failed\n");
                    return (false, false);
                }
                ShowLog("Success\n");

                ShowLog("Pinging device...");
                status = controller.Ping();
                if (status != rsid.Status.Ok)
                {
                    ShowLog("Failed\n");
                    return (true, false);
                }

                ShowLog("Success\n");
                return (true, true);
            }
        }

        private bool PairDevice()
        {
#if RSID_SECURE
            ShowLog("Pairing..");

            IntPtr pairArgsHandle = IntPtr.Zero;
            pairArgsHandle = rsid_create_pairing_args_example(_signatureHelpeHandle);
            var pairingArgs = (rsid.PairingArgs)Marshal.PtrToStructure(pairArgsHandle, typeof(rsid.PairingArgs));

            var rv = _authenticator.Pair(ref pairingArgs);
            if (rv != rsid.Status.Ok)
            {
                ShowLog("Failed\n");
                if (pairArgsHandle != IntPtr.Zero) rsid_destroy_pairing_args_example(pairArgsHandle);
                return false;
            }

            ShowLog("Success\n");
            rsid_update_device_pubkey_example(_signatureHelpeHandle, Marshal.UnsafeAddrOfPinnedArrayElement(pairingArgs.DevicePubkey, 0));
#endif //RSID_SECURE

            return true;
        }

        private class DeviceState
        {
            public string FirmwareVersion;
            public string SerialNumber;
            public bool IsOperational;
            public bool IsCompatible;
            public rsid.SerialConfig SerialConfig;
            public rsid.PreviewConfig PreviewConfig;
        }

        private DeviceState DetectDevice()
        {
            var deviceState = new DeviceState();

            // acquire communication settings
            if (Settings.Default.AutoDetect)
            {
                var enumerator = new DeviceEnumerator();
                var enumeration = enumerator.Enumerate();

                if (enumeration.Count == 0)
                {
                    var msg = "Could not detect device.\nPlease reconnect the device and try again.";
                    ShowErrorMessage("Connection Error", msg);
                    throw new Exception("Connection Error");
                }
                else if (enumeration.Count > 1)
                {
                    var msg = "More than one device detected.\nPlease make sure only one device is connected and try again.";
                    ShowErrorMessage("Connection Error", msg);
                    throw new Exception("Connection Error");
                }

                deviceState.SerialConfig.port = enumeration[0].port;
                deviceState.SerialConfig.serialType = enumeration[0].serialType;
            }
            else
            {
                deviceState.SerialConfig.port = Settings.Default.Port;
                var serialTypeString = Settings.Default.SerialType.ToUpper();
                deviceState.SerialConfig.serialType = serialTypeString == "UART" ? rsid.SerialType.UART : rsid.SerialType.USB;
            }

            // test connection
            var (isConnectionWorking, isOperational) = TestConnection(deviceState.SerialConfig);
            deviceState.IsOperational = isOperational;

            if (!isConnectionWorking)
            {
                var msg = "Could not connect to device.\nPlease reconnect the device and try again.";
                ShowErrorMessage("Connection Error", msg);
                throw new Exception("Connection Error");
            }

            // query and display device metadata - can fail in loader for now, but we don't care
            var firmwareVersion = QueryMetadata(deviceState.SerialConfig);
            deviceState.FirmwareVersion = firmwareVersion;

            var isCompatible = rsid.Authenticator.IsFwCompatibleWithHost(deviceState.FirmwareVersion);
            deviceState.IsCompatible = isCompatible;

            return deviceState;
        }

        // 1. Query some initial info from the device:
        //   * FW Version        
        //   * Auth settings
        //   * List of enrolled users        
        // 2. Connect and pair to the device
        // 3. Start preview
        private void InitialSession(Object threadContext)
        {
            ShowProgressTitle("Connecting...");

            // show host library version
            var hostVersion = rsid.Authenticator.Version();
            ShowLog("Host: v" + hostVersion + "\n");
            var title = $"RealSenseID v{hostVersion}";
#if RSID_SECURE
            title += " secure mode";
#endif

            try
            {
                _deviceState = DetectDevice();
            }
            catch (Exception ex)
            {
                OnStopSession();
                ShowErrorMessage("Connection Error", ex.Message);
                return;
            }

            // is in loader
            if (!_deviceState.IsOperational)
            {
                OnStopSession();

                var compatibleVersion = rsid.Authenticator.CompatibleFirmwareVersion();

                ShowFailedStatus("Device Error");
                var msg = $"Device failed to respond. Please reconnect the device and try again." +
                    $"\nIf the the issue persists, flash firmware version { compatibleVersion } or newer.\n";
                ShowLog(msg);
                ShowErrorMessage("Device Error", msg);

                return;
            }

            if (!_deviceState.IsCompatible)
            {
                OnStopSession();

                var compatibleVersion = rsid.Authenticator.CompatibleFirmwareVersion();

                ShowFailedStatus("FW Incompatible");
                var msg = $"Firmware version is incompatible.\nPlease update to version { compatibleVersion } or newer.\n";
                ShowLog(msg);
                ShowErrorMessage("Firmware Version Error", msg);

                return;
            }

            // start preview
            _deviceState.PreviewConfig = new rsid.PreviewConfig { cameraNumber = Settings.Default.CameraNumber, debugMode = _debugMode ? 1 : 0 };
            _preview = new rsid.Preview(_deviceState.PreviewConfig);
            _preview.Start(OnPreview);

            try
            {
                if (!ConnectAuth())
                {
                    throw new Exception("Connection failed");
                }

                bool isPaired = PairDevice();
                if (!isPaired)
                {
                    ShowErrorMessage("Pairing Error", "Device pairing failed.\nPlease make sure the device wasn't previously paired and try again.");
                    throw new Exception("Pairing failed");
                }

                if (_flowMode == FlowMode.Server)
                    RefreshUserListServer();
                else
                    RefreshUserList();

                ShowSuccessTitle("Connected");
            }
            catch (Exception ex)
            {
                ShowErrorMessage("Authenticator Error", ex.Message);
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
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
                VerifyResult(status == rsid.Status.Ok, "Canceled", "Cancel failed");
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
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
                _detectedEnrollmenetPoses.Clear();
                GetNextEnrollmentPoseAndInstructions(out _, out string enrollInstructions);
                ShowProgressTitle(enrollInstructions);
                _authloopRunning = true;
                var enrollArgs = new rsid.EnrollArgs
                {
                    userId = userId,
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollResult,
                    progressClbk = OnEnrollProgress,
                    ctx = userIdCtx
                };
                var status = _authenticator.Enroll(enrollArgs);
                if (status == rsid.Status.Ok)
                {
                    HideEnrollingLabelPanel();
                    RefreshUserList();
                }
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideEnrollingLabelPanel();
                _authloopRunning = false;
                _authenticator.Disconnect();
                Marshal.FreeHGlobal(userIdCtx);
            }
        }

        // Enroll Job
        private void EnrollExtractFaceprintsJob(Object threadContext)
        {
            var userId = threadContext as string;
            if (_db.DoesUserExist(userId))
            {
                ShowFailedStatus("User ID already exists in database");
                return;
            }

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll \"{userId}\"");
            try
            {
                _detectedEnrollmenetPoses.Clear();
                GetNextEnrollmentPoseAndInstructions(out _, out string enrollInstructions);
                ShowProgressTitle(enrollInstructions);
                _lastEnrolledUserId = userId + '\0';
                var enrollExtArgs = new rsid.EnrollExtractArgs
                {
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollExtractionResult,
                    progressClbk = OnEnrollProgress
                };
                var operationStatus = _authenticator.EnrollExtractFaceprints(enrollExtArgs);
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideEnrollingLabelPanel();
                _authenticator.Disconnect();

            }
        }

        // Wrapper method for use with thread pool.
        private void DeleteSingleUserJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            List<string> usersIds = (List<string>)threadContext;
            OnStartSession($"Delete {usersIds.Count} users");

            try
            {
                ShowProgressTitle("Deleting..");
                bool successAll = true;
                foreach (string userId in usersIds)
                {
                    ShowLog($"Delete user {userId}");
                    var status = _authenticator.RemoveUser(userId);
                    if (status == rsid.Status.Ok)
                    {
                        ShowLog("Ok");
                    }
                    else
                    {
                        ShowLog("Failed");
                        successAll = false;
                    }
                }

                VerifyResult(successAll == true, "Delete success", "Delete failed");
                RefreshUserList();
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

        private void DeleteUsersJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Delete Users");
            try
            {
                ShowProgressTitle("Deleting..");
                var status = _authenticator.RemoveAllUsers();
                VerifyResult(status == rsid.Status.Ok, "Delete success", "Delete failed", RefreshUserList);
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

        private void DeleteSingleUserServerJob(Object threadContext)
        {
            List<string> usersIds = (List<string>)threadContext;

            try
            {
                ShowProgressTitle("Deleting..");
                bool successAll = true;
                foreach (string userId in usersIds)
                {
                    ShowLog($"Delete user {userId}");
                    var success = _db.Remove(userId);
                    if (success == true)
                    {
                        ShowLog("Ok");
                    }
                    else
                    {
                        ShowLog("Failed");
                        successAll = false;
                    }
                }
                _db.Save();
                VerifyResult(successAll == true, "Delete success", "Delete failed");
                RefreshUserListServer();
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

        private void DeleteUsersServerJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Delete Users");
            try
            {
                ShowProgressTitle("Deleting..");
                var success = _db.RemoveAll();
                _db.Save();
                VerifyResult(success == true, "Delete all success", "Delete all failed", RefreshUserListServer);
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
            if (!ConnectAuth()) return;
            OnStartSession("Authenticate");
            try
            {
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, resultClbk = OnAuthResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Authenticating..");
                _authloopRunning = true;
                _authenticator.Authenticate(authArgs);
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authloopRunning = false;
                _authenticator.Disconnect();
            }
        }

        // Authentication loop job
        private void AuthenticateLoopJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("Authenticate Loop");
            try
            {
                var authArgs = new rsid.AuthArgs { hintClbk = OnAuthHint, resultClbk = OnAuthResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Authenticating..");
                _authloopRunning = true;
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
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authloopRunning = false;
                _authenticator.Disconnect();
            }
        }

        // Authenticate faceprints extraction job
        private void AuthenticateExtractFaceprintsJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Extracting Faceprints");
            try
            {
                var authExtArgs = new rsid.AuthExtractArgs { hintClbk = OnAuthHint, resultClbk = OnAuthExtractionResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Extracting faceprints for authentication ..");
                _authenticator.AuthenticateExtractFaceprints(authExtArgs);
            }
            catch (Exception ex)
            {
                ShowFailedStatus(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authenticator.Disconnect();
            }
        }

        // Authenticate loop faceprints extraction job
        private void AuthenticateExtractFaceprintsLoopJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("Authentication faceprints extraction loop");
            try
            {
                var authLoopExtArgs = new rsid.AuthExtractArgs { hintClbk = OnAuthHint, resultClbk = OnAuthLoopExtractionResult, ctx = IntPtr.Zero };
                ShowProgressTitle("Authenticating..");
                _authloopRunning = true;
                _authenticator.AuthenticateLoopExtractFaceprints(authLoopExtArgs);
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
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authloopRunning = false;
                _authenticator.Disconnect();
            }
        }

        // SetAuthSettings job
        private void SetAuthSettingsJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            (var authConfig, var flowMode) = ((rsid.AuthConfig, FlowMode))threadContext;
            OnStartSession("SetAuthSettings");
            try
            {
                ShowProgressTitle("SetAuthSettings " + authConfig.securityLevel.ToString());
                ShowLog("Security " + authConfig.securityLevel.ToString());
                ShowLog(authConfig.cameraRotation.ToString().Replace("_", " "));
                var status = _authenticator.SetAuthSettings(authConfig);

                if (flowMode != _flowMode)
                {
                    _flowMode = flowMode;
                    if (flowMode == FlowMode.Server)
                    {
                        Dispatcher.Invoke(() => StandbyButton.IsEnabled = false);
                        _db.Load();
                        RefreshUserListServer();
                    }
                    else
                    {
                        RefreshUserList();
                    }
                }

                VerifyResult(status == rsid.Status.Ok, "Apply settings done", "Apply settings failed");
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

            OnStartSession("Standby");
            try
            {
                ShowProgressTitle("Storing users data");
                var status = _authenticator.Standby();
                VerifyResult(status == rsid.Status.Ok, "Storing users data done", "Storing users data failed");
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

        // Update fw job using the given filename.
        // Update process:
        // 1. Check if the requested fw is compatible with current host
        // 2. Stop preview
        // 3. Update and show feeback on the progress
        // 4. If success, re connect to the device query the new version
        // 5. Start preview
        private void FwUpdateJob(Object threadContext)
        {
            var binPath = (string)threadContext;
            using (var _fwUpdater = new rsid.FwUpdater())
            {
                BackgroundDispatch(() => _progressBar.Show());
                var newFwVersion = _fwUpdater.ExtractFwVersion(binPath);
                var isCompatible = rsid.Authenticator.IsFwCompatibleWithHost(newFwVersion);
                if (!isCompatible)
                {
                    CloseProgressBar();
                    var compatibleVer = rsid.Authenticator.CompatibleFirmwareVersion();
                    var msg = "Selected firmware version is not compatible with this SDK.\n";
                    msg += $"Chosen Version:   {newFwVersion}\n";
                    msg += $"Compatible Versions: {compatibleVer} and above\n";
                    ShowErrorMessage("Incompatible FW Version", msg);
                    return;
                }
                _authenticator?.Disconnect();
                _preview?.Stop();
                BackgroundDispatch(() => { TogglePreviewOpacity(false); });
                Thread.Sleep(100);

                OnStartSession("Firmware Update");
                bool success = false;
                try
                {
                    ShowProgressTitle("Updating Firmware..");
                    ShowLog("update to " + newFwVersion);

                    var eventHandler = new rsid.FwUpdater.EventHandler
                    {
                        progressClbk = (progress) => UpdateProgressBar(progress * 100)
                    };

                    var fwUpdateSettings = new rsid.FwUpdater.FwUpdateSettings
                    {
                        port = _deviceState.SerialConfig.port
                    };

                    var status = _fwUpdater.Update(binPath, eventHandler, fwUpdateSettings);
                    success = status == rsid.Status.Ok;
                    if (!success)
                        throw new Exception("Update Failed");
                }
                catch (Exception ex)
                {
                    _deviceState.IsOperational = false;
                    success = false;
                    ShowFailedStatus(ex.Message);
                    ShowErrorMessage("Firmware Update Failed", "Please reconnect your device and try again.");
                }
                finally
                {
                    CloseProgressBar();
                    OnStopSession();

                    if (success)
                    {
                        ShowProgressTitle("Rebooting...");
                        Thread.Sleep(7500);
                        //ClearTitle();
                        BackgroundDispatch(() => { TogglePreviewOpacity(true); });
                        InitialSession(null);
                    }
                }
            }
        }

        // Debug console support
        void CreateConsole()
        {
            AllocConsole();
            DeleteMenu(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_BYCOMMAND);
            ToggleConsoleAsync(false);
        }

        private const int MF_BYCOMMAND = 0x00000000;
        public const int SC_CLOSE = 0xF060;

        [DllImport("user32.dll")]
        public static extern int DeleteMenu(IntPtr hMenu, int nPosition, int wFlags);

        [DllImport("user32.dll")]
        private static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

        // show/hide console
        private void ToggleConsoleAsync(bool show)
        {
            const int SW_HIDE = 0;
            const int SW_SHOW = 5;
            ShowWindow(GetConsoleWindow(), show ? SW_SHOW : SW_HIDE);
        }

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
#endif //DEBUG
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
