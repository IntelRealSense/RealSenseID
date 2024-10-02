// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using System.Windows.Shapes;
using Microsoft.Win32;
using rsid;
using System.Text.RegularExpressions;
using Path = System.IO.Path;
using System.Runtime.CompilerServices;
using System.Diagnostics;
using System.Reflection;

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
        private static readonly float _updateLoopInterval = 0.05f;
        private static readonly float _userFeedbackDuration = 3.0f;
        private static readonly string _serialNumberFile = "sn.txt";

#if RSID_PREVIEW
        private bool _previewEnabled = true;
#else
        private bool _previewEnabled = false;
#endif

        private DeviceState _deviceState;
        private Authenticator _authenticator;
        private FlowMode _flowMode;

        private Preview _preview;
        private WriteableBitmap _previewBitmap;
        private byte[] _previewBuffer = new byte[0]; // store latest frame from the preview callback
        // tuple of (Face,IsAuthenticated,UserId) in current session
        private List<(FaceRect, AuthStatus?, string userId)> _detectedFaces = new List<(FaceRect, AuthStatus?, string)>();
        private object _previewMutex = new object();

        private string[] _userList = new string[0]; // latest user list that was queried from the device

        private bool _busy;
        private bool _pausePreview;
        private bool _cancelWasCalled;
        private string _lastEnrolledUserId;
        private AuthStatus _lastAuthHint = AuthStatus.Serial_Ok; // To show only changed hints. 

        private IntPtr _signatureHelpeHandle = IntPtr.Zero;
        private Database _db;// = new Database();

        private string _dumpDir;
        private ProgressBarDialog _progressBar;

        private float _userFeedbackTime;

        private int _fps;
        private readonly System.Diagnostics.Stopwatch _fpsStopWatch = new System.Diagnostics.Stopwatch();
        private FrameDumper _frameDumper;

        public MainWindow()
        {
            _cancelWasCalled = false;
            InitializeComponent();

            this.Dispatcher.UnhandledException += OnUnhandledException;
            CreateConsole();
            Title += $" v{Authenticator.Version()}";
#if RSID_SECURE
            Title += " (secure mode)";
#endif
            ContentRendered += MainWindow_ContentRendered;
            Closing += MainWindow_Closing;

            DispatcherTimer timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromMilliseconds(_updateLoopInterval * 1000);
            timer.Tick += Timer_Tick;
            timer.Start();
            _fpsStopWatch.Start();
            if (_previewEnabled == false)
                LabelPreview.Visibility = Visibility.Collapsed;
        }

        private void OnUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
        {
            var message = $"Exception occurred:\n{e.Exception.Message}";
            ShowWindowDialog(new ErrorDialog("Error", message));
            e.Handled = true;
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            // load serial port and preview configuration
            LoadConfig();

            // create face authenticator and show version
            _authenticator = CreateAuthenticator();

            // Set license check hander callbacks
            _authenticator.EnableLicenseCheckHandler(onStart: () => ShowProgressTitle("License Check"),
            onEnd: status => { if (status != Status.Ok) ShowFailedTitle(status.ToString()); });

            // pair to the device       
            OnStartSession(string.Empty, false);
            ClearTitle();
            ThreadPool.QueueUserWorkItem(InitialSession);
        }

        private void MainWindow_Closing(object sender, EventArgs e)
        {
            if (_busy)
            {
                try
                {
                    _cancelWasCalled = true;
                    _authenticator.Cancel();
                    Thread.Sleep(500); // give time to device to cancel before exiting

                }
                catch
                {
                    // ignored
                }
            }
        }

        private void EnrollButton_Click(object sender, RoutedEventArgs e)
        {
            var enrollInput = new EnrollInput();
            if (ShowWindowDialog(enrollInput) == true)
            {
                if (ShowWindowDialog(new EnrollInstructions()) == true)
                {
                    EnrollPanel.Visibility = Visibility.Visible;

                    if (_flowMode == FlowMode.Server)
                        ThreadPool.QueueUserWorkItem(EnrollExtractFaceprintsJob, enrollInput.Username);
                    else
                        ThreadPool.QueueUserWorkItem(EnrollJob, enrollInput.Username);
                }
            }
        }

        private void EnrollImgButton_Click(object sender, RoutedEventArgs e)
        {
            var enrollInput = new EnrollInput();
            if (ShowWindowDialog(enrollInput).GetValueOrDefault() == false)
                return;
            var openFileDialog = new OpenFileDialog
            {
                CheckFileExists = true,
                Multiselect = false,
                Title = "Select Image to Enroll",
                Filter = "Images|*.png;*.jpg;*.jpeg;*.bmp;",
                FilterIndex = 1
            };
            if (openFileDialog.ShowDialog() == false)
                return;
            var enrollData = new EnrollImageRecord
            {
                UserId = enrollInput.Username,
                Filename = openFileDialog.FileName
            };
            if (FlowMode.Server == _flowMode)
            {
                Task.Run(() => EnrollImageHostJob(enrollData, false));
            }
            else
            {
                Task.Run(() => EnrollImageJob(enrollData, false));
            }
        }

        private async void BatchEnrollImgButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var openFileDialog = new OpenFileDialog
                {
                    Multiselect = false,
                    Title = "Select File to enroll",
                    Filter = "json files (*.json)|*.json",
                    FilterIndex = 1
                };

                if (openFileDialog.ShowDialog() == false)
                    return;

                var enrollList = JsonHelper.LoadImagesToEnroll(openFileDialog.FileName);
                if (enrollList.Count == 0)
                {
                    throw new Exception("Empty enroll list");
                }
                _progressBar = new ProgressBarDialog { DialogTitle = { Text = "Enrolling" } };
                _progressBar.Show();
                var counter = 0;
                var successCounter = 0;
                var sw = System.Diagnostics.Stopwatch.StartNew();
                foreach (var record in enrollList)
                {
                    counter++;
                    var basename = Path.GetFileName(record.Filename);
                    _progressBar.DialogTitle.Text = $"{counter}/{enrollList.Count} {basename}...";
                    var progress = ((float)counter) / enrollList.Count;
                    _progressBar.Update(progress * 100);
                    // perform the enroll
                    var notLast = counter < enrollList.Count;
                    var success = true;
                    if (FlowMode.Server == _flowMode)
                    {
                        success = await Task.Run(() => EnrollImageHostJob(record, notLast));
                    }
                    else
                    {
                        success = await Task.Run(() => EnrollImageJob(record, notLast));
                    }

                    if (success)
                    {
                        successCounter++;
                    }

                    await Task.Delay(1000);
                }

                var summary =
                    $"Total:   {counter}\nSucceed: {successCounter}\nElapsed: {sw.Elapsed.Hours}:{sw.Elapsed.Minutes}:{sw.Elapsed.Seconds}";
                ShowErrorMessage($"Enroll Summary", summary);
            }
            catch (Exception ex)
            {
                ShowErrorMessage("Failed loading json", ex.Message.Substring(0, 300) + "\n...");
            }
            finally
            {
                CloseProgressBar();
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
                if (isLoop)
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
                if (isLoop)
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

            if (ShowWindowDialog(new DeleteUserInput()) == true)
            {
                if (_flowMode == FlowMode.Server)
                {
                    if (deleteAll)
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
                    if (deleteAll)
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
            DeviceConfig? deviceConfig = null;
            if (_deviceState.IsOperational)
            {
                // device is in operational mode, we continue to query config as usual
                if (!ConnectAuth()) return;
                deviceConfig = QueryDeviceConfig();
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


            var dialog = new AuthSettingsInput(_deviceState.FirmwareVersion, deviceConfig, _deviceState.PreviewConfig, _flowMode, _previewEnabled, _deviceState.SerialConfig);

            if (ShowWindowDialog(dialog) == true)
            {
                if (string.IsNullOrEmpty(dialog.FirmwareFileName) == true)
                {
                    ThreadPool.QueueUserWorkItem(SetDeviceConfigJob, (deviceConfig, dialog.Config, _deviceState.PreviewConfig, dialog.PreviewConfig, dialog.FlowMode));
                }
                else
                {
                    TabsControl.SelectedIndex = 1;  // Switch to logs tab
                    _progressBar = new ProgressBarDialog();
                    ThreadPool.QueueUserWorkItem(FwUpdateJob,
                        new Tuple<string, bool>(dialog.FirmwareFileName, dialog.ForceFirmwareUpdate));
                }
            }
        }

        private void ClearLogButton_Click(object sender, RoutedEventArgs e)
        {
            ClearLog();
        }

        private void OpenConsoleToggle_Click(object sender, RoutedEventArgs e)
        {
            ToggleConsoleAsync(OpenConsoleToggle.IsChecked.GetValueOrDefault());
        }

        private async void FetchDeviceLog_Click(object sender, RoutedEventArgs e)
        {            
            var sfd = new SaveFileDialog
            {
                Title = "Save As",
                FileName = "f450.log",
                InitialDirectory = Path.GetFullPath(_dumpDir),
                DefaultExt = ".log",
                Filter = "Log files (*.log)|*.log|Text documents (*.txt)|*.txt|All files (*.*)|*.*"
            };
            if (sfd.ShowDialog() == false)
            {
                return;
            }
            OnStartSession("Fetching Device Log..", false);
            ShowProgressTitle("Fetching Log...");
            try
            {
                var log = await Task.Run(() =>
                {
                    using (var deviceController = new DeviceController())
                    {
                        var status = deviceController.Connect(_deviceState.SerialConfig);
                        if (status != Status.Ok)
                        {
                            throw new Exception("Connection failed " + status);
                        }
                        return deviceController.FetchLog();
                    }
                });
                ShowSuccessTitle("Done");
                File.WriteAllText(sfd.FileName, log);
                OnStopSession();
                if (ShowWindowDialog(new OKCancelDialog("Open File?", "Open this file with the text editor?", true)) == true)
                {
                    Process.Start(new ProcessStartInfo(sfd.FileName) { UseShellExecute = true });
                }
            }
            catch (Exception ex)
            {
                ShowFailedTitle("Fetch Error");
                ShowErrorMessage("Error", ex.Message);
            }
            finally
            {
                OnStopSession();
            }
        }

        private void ExportButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog
            {
                Title = "Select File export DB to",
                Filter = "db files (*.db)|*.db",
                FilterIndex = 1
            };
            if (sfd.ShowDialog() == false)
                return;

            SetUiEnabled(false);
            var dbfilename = sfd.FileName;
            if (!ConnectAuth()) return;
            var db = new Database(dbfilename);
            var exportedDb = _authenticator.GetUsersFaceprints();
            if (exportedDb == null)
            {
                ShowErrorMessage("Export DB", "Error while exporting users");
                OnStopSession();
                _authenticator.Disconnect();
                return;
            }
            try
            {
                foreach (var uf in exportedDb)
                {
                    db.Push(uf.faceprints, uf.userID);
                }
                db.Save();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception: " + ex.Message);
            }
            finally
            {
                SetUiEnabled(true);
            }
            if (exportedDb.Count > 0)
                ShowSuccessTitle("Database file was created successfully");
            else
            {
                ShowFailedTitle("No users were exported");
                ShowErrorMessage("Export DB", "Error while exporting users");
            }
            OnStopSession();
            _authenticator.Disconnect();
        }

        private void ImportButton_Click(object sender, RoutedEventArgs e)
        {
            var openFileDialog = new OpenFileDialog
            {
                Multiselect = false,
                Title = "Select File to import from",
                Filter = "db files (*.db)|*.db",
                FilterIndex = 1
            };

            if (openFileDialog.ShowDialog() == false)
                return;

            var dbfilename = openFileDialog.FileName;
            if (!ConnectAuth()) return;
            var usersFromDb = new List<UserFaceprints>();

            var db = new Database(dbfilename);
            db.Load();

            var uf = new UserFaceprints();
            foreach (var (faceprintsDb, userIdDb) in db.FaceprintsArray)
            {
                uf.userID = userIdDb;
                uf.faceprints = faceprintsDb;
                usersFromDb.Add(uf);
            }

            try
            {
                SetUiEnabled(false);
                if (_authenticator.SetUsersFaceprints(usersFromDb))
                    ShowSuccessTitle("All users imported successfully!");
                else
                {
                    ShowFailedTitle("Some users were not imported");
                    ShowErrorMessage("Import DB", "Error while importing users!");
                }
                RefreshUserList();
            }
            catch (Exception ex)
            {
                ShowErrorMessage("Import DB", "Error while importing users!");
                ShowLog("Exception while inserting users: " + ex.Message);
                ShowFailedTitle("Some users were not imported");
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
                SetUiEnabled(true);
            }
        }



        private void TogglePreviewOpacity(bool isActive)
        {
            RenderDispatch(() =>
            {
                PreviewImage.Opacity = isActive ? 1.0 : 0.85;
                LabelPreviewInfo.Opacity = isActive ? 0.66 : 0.3;
            });
        }


        // invoke and wait for visibily change
        private void InvokePreviewVisibility(Visibility visibility)
        {
            Dispatcher.Invoke(() => SetPreviewVisibility(visibility));
        }

        private void SetPreviewVisibility(Visibility visibility)
        {
            var previewMode = _deviceState.PreviewConfig.previewMode;
            LabelPreview.Content = $"Camera Preview\n({previewMode.ToString().ToLower()} preview mode)";
            PreviewImage.Visibility = visibility;
        }

        private void PreviewImage_MouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (_deviceState.PreviewConfig.previewMode == PreviewMode.RAW10_1080P || !_deviceState.IsOperational)
                return;

            LabelPlayStop.Visibility = LabelPlayStop.Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;
            if (LabelPlayStop.Visibility == Visibility.Hidden)
            {
                ResumePreviewAfter(100);
                TogglePreviewOpacity(true);
            }
            else
            {
                PausePreview();
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
                    PreviewCanvas.Opacity -= _updateLoopInterval * 0.5;
                }
                if (_userFeedbackTime <= 0)
                {
                    ClearTitle();
                    PreviewCanvas.Visibility = Visibility.Hidden;
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

        private void DumpDispatch(Action action)
        {
            try
            {
                Dispatcher.BeginInvoke(action, DispatcherPriority.Normal, null);
            }
            catch (Exception ex)
            {
                Console.WriteLine("DumpDispatch: " + ex.Message);
            }
        }

        private bool? ShowWindowDialog(Window window)
        {
            SetUiEnabled(false);
            bool? returnOk = window.ShowDialog();
            SetUiEnabled(true);
            return returnOk;
        }

        private void ShowErrorMessage(string title, string message)
        {
            Dispatcher.Invoke(() => (ShowWindowDialog(new ErrorDialog(title, message))));
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

        public Authenticator GetAuthenticator()
        {
            return _authenticator;
        }

        public void ShowLog(string message)
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
                UserFeedbackContainer.Opacity = _detectedFaces.Count <= 1 ? 1.0 : 0.0f; // show title only if single face
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

        private void ShowFailedTitle(string message)
        {
            ShowTitle(message, FailBrush, _userFeedbackDuration);
        }

        private string GetFailedSpoofMsg(AuthStatus status)
        {
            //var msg = status.ToString();
            if ((int)status >= (int)AuthStatus.Spoof_2D || (int)status == (int)AuthStatus.Forbidden)
                return "Spoof Attempt";
            else if (status == AuthStatus.NoFaceDetected)
                return "No valid face detected";
            else
                return status.ToString();
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
                ShowLog(successMessage);
                onSuccess?.Invoke();
            }
            else
            {
                ShowFailedTitle(failMessage);
                ShowLog(failMessage);
            }
        }

        private void VerifyResultAuth(AuthStatus status, string successMessage, string failMessage, Action onSuccess = null, string userId = null)
        {
            // provide option to unlock device if locked by too many spoof attempts
            if (status == AuthStatus.TooManySpoofs)
            {
                BackgroundDispatch(async () =>
                {
                    await Task.Delay(750);
                    var dialog = new OKCancelDialog("Too Many Spoof Attempts!", "The device is locked due to multiple spoof attempts.\nUnlock the device?");
                    var dialogResult = ShowWindowDialog(dialog);
                    if (dialogResult == true)
                    {
                        ThreadPool.QueueUserWorkItem(UnlockJob);
                    }
                });
            }
            VerifyResult(status == AuthStatus.Success, successMessage, failMessage, onSuccess);
            UpdateFaceResult(status, userId);
        }

        private void UpdateFaceResult(AuthStatus status, string userId)
        {
            // updated the detected face success value if exists
            RenderDispatch(() =>
            {
                //find the next face that didn't get a result yet and update it
                for (var i = 0; i < _detectedFaces.Count; i++)
                {
                    var face = _detectedFaces[i];
                    if (!face.Item2.HasValue)
                    {
                        _detectedFaces[i] = (face.Item1, status, userId);
                        break;
                    }
                }
                RenderDetectedFaces();
            });
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
                _progressBar?.Close();
                _progressBar = null;
            });
        }

        private void UpdateAuthButtonText()
        {
            string text;
            switch (_deviceState.DeviceConfig.algoFlow)
            {
                case DeviceConfig.AlgoFlow.All:
                    text = "AUTHENTICATE";
                    break;
                case DeviceConfig.AlgoFlow.SpoofOnly:
                    text = "DETECT SPOOF";
                    break;
                case DeviceConfig.AlgoFlow.FaceDetectionOnly:
                    text = "DETECT FACE";
                    break;
                case DeviceConfig.AlgoFlow.RecognitionOnly:
                    text = "RECOGNIZE FACE";
                    break;
                default:
                    text = "AUTHENTICATE";
                    break;
            }
            AuthenticateButton.Content = text;
        }
        private void SetUiEnabled(bool isEnabled)
        {
            var isRecogEnabled = _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.All ||
                _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.RecognitionOnly;

            SettingsButton.IsEnabled = isEnabled;
            DeleteButton.IsEnabled = isEnabled && isRecogEnabled && UsersListView.SelectedItems.Count > 0;
            ImportButton.IsEnabled = isEnabled && isRecogEnabled && (_flowMode != FlowMode.Server);
            ExportButton.IsEnabled = isEnabled && isRecogEnabled && (_flowMode != FlowMode.Server);
            EnrollButton.IsEnabled = isEnabled && isRecogEnabled;
            EnrollImgButton.IsEnabled = isEnabled && isRecogEnabled;
            BatchEnrollButton.IsEnabled = isEnabled && isRecogEnabled;
            PairButton.IsEnabled = isEnabled;
            UnpairButton.IsEnabled = isEnabled;
            ClearLogButton.IsEnabled = isEnabled;
            FetchDeviceLogButton.IsEnabled = isEnabled;
            // auth button enabled if there are enrolled users or if we in spoof/face detection only mode
            var authBtnEnabled = AuthenticateButton.IsEnabled =
                isEnabled && (
                _userList?.Length > 0
                || _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.SpoofOnly
                || _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.FaceDetectionOnly);
            AuthenticateButton.IsEnabled = authBtnEnabled;
            AuthenticateLoopToggle.IsEnabled = authBtnEnabled;
            UsersListView.IsEnabled = isEnabled && isRecogEnabled;
            SelectAllUsersCheckBox.IsEnabled = isEnabled && isRecogEnabled && _userList?.Length > 0;

            UpdateAuthButtonText();
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

            ShowFailedTitle("Mode " + flowModeString + " not supported, using Device Mode instead");
            ShowLog("Device Mode\n");
            return FlowMode.Device;
        }

        private void LoadConfig()
        {
            _dumpDir = Settings.Default.DumpDir;

            _flowMode = StringToFlowMode(Settings.Default.FlowMode.ToUpper());

            if (_flowMode == FlowMode.Server)
            {
                ImportButton.IsEnabled = false;
                ExportButton.IsEnabled = false;

                int loadStatus = _db.Load();

                if (loadStatus < 0)
                {
                    HandleDbErrorServer();
                    ShowLog("Error occured during loading the DB. This may be due to faceprints version mismatch or other error. Saved the old DB to backup and started an empty DB.\n");
                    string guimsg = "DB version or load error.";
                    VerifyResult(false, string.Empty, guimsg);
                }
            }
        }

        // Create authenticator
        private Authenticator CreateAuthenticator()
        {
#if RSID_SECURE
            _signatureHelpeHandle = rsid_create_example_sig_clbk();
            var sigCallback = (SignatureCallback)Marshal.PtrToStructure(_signatureHelpeHandle, typeof(SignatureCallback));
            return new Authenticator(sigCallback);
#else
            return new Authenticator();
#endif //RSID_SECURE

        }

        private bool ConnectAuth()
        {
            var status = _authenticator.Connect(_deviceState.SerialConfig);
            if (status != Status.Ok)
            {
                ShowFailedTitle("Connection Error");
                ShowLog("Connection error");
                ShowErrorMessage($"Connection Failed to Port {_deviceState.SerialConfig.port}",
                    $"Connection Error.\n\nPlease check the serial port setting in the config file.");
                return false;
            }
            return true;
        }

        private void LogDeviceConfig(DeviceConfig deviceConfig)
        {
            ShowLog(" * Camera Rotation: " + deviceConfig.cameraRotation.ToString());
            ShowLog(" * AntiSpoof Level: " + deviceConfig.securityLevel.ToString());
            ShowLog(" * Algo Flow: " + deviceConfig.algoFlow);
            ShowLog(" * Face Selection Policy: " + deviceConfig.faceSelectionPolicy);
            ShowLog(" * Dump Mode: " + deviceConfig.dumpMode.ToString());
            ShowLog(" * Host Mode: " + _flowMode);
            ShowLog(" * Camera Index: " + _deviceState.PreviewConfig.cameraNumber);
            ShowLog(" * Preview Mode: " + _deviceState.PreviewConfig.previewMode);
            ShowLog(" * Matcher Confidence Level: " + deviceConfig.matcherConfidenceLevel.ToString());
            ShowLog(" * Max Spoofs: " + deviceConfig.maxSpoofs.ToString());
            ShowLog("");
        }

        private DeviceConfig? QueryDeviceConfig()
        {

            ShowLog("");
            ShowLog("Query device config..");
            DeviceConfig deviceConfig;
            var rv = _authenticator.QueryDeviceConfig(out deviceConfig);
            if (rv != Status.Ok)
            {
                ShowLog("Query error: " + rv.ToString());
                ShowFailedTitle("Query error: " + rv.ToString());
                return null;
            }
            LogDeviceConfig(deviceConfig);
            return deviceConfig;
        }

        private void UpdateAdvancedMode()
        {
            DeviceConfig? deviceConfig = QueryDeviceConfig();
            if (!deviceConfig.HasValue)
            {
                var msg = "Failed to query device config";
                ShowLog(msg);
                ShowErrorMessage("QueryDeviceConfig Error", msg);
                throw new Exception("QueryDeviceConfig Error");
            }
            _deviceState.DeviceConfig = deviceConfig.Value;
        }

        private bool UpdateUser(int userIndex, string userId, ref Faceprints updatedFaceprints)
        {
            if (_db == null)
                return false;
            bool success = _db.UpdateUser(userIndex, userId, ref updatedFaceprints);

            if (success)
            {
                _db.Save();
            }

            return success;
        }

        private void HandleDbErrorServer()
        {
            // goal of this handler - to handle two possible scenarios :
            //
            // (1) if Faceprints (FP) version changed, e.g. the FP on the db and the current FP changed version (and possibly their internal structure).
            // (2) if db Load() fails on some exception - this may be due to version mismatch (FP structure changed) or other error.
            //
            // in both cases we want to : 
            //
            // (a) backup the old db to a separated file.
            // (b) clear the db and start a new db from scratch.
            // (c) refresh the users list on the gui.
            //           

            if (_flowMode == FlowMode.Server)
            {
                _db.SaveBackupAndDeleteDb();
                RefreshUserListServer();
            }
        }

        private void Match(rsid.ExtractedFaceprints faceprintsToMatch)
        {
            try
            {
                ShowProgressTitle("Matching faceprints to database");

                // if Faceprints versions don't match - return with error message.
                if (!(_db.VerifyVersionMatchedPLE(ref faceprintsToMatch)))
                {
                    HandleDbErrorServer();
                    string logmsg = $"Faceprints (FP) version mismatch: DB={_db.GetVersion()}, FP={faceprintsToMatch.version}. Saved the old DB to backup file and started a new DB from scratch.";
                    string guimsg = $"Faceprints (FP) version mismatch: DB={_db.GetVersion()}, FP={faceprintsToMatch.version}. DB backuped and cleaned.";
                    ShowLog(logmsg);
                    VerifyResult(false, string.Empty, guimsg);
                    return;
                }

                // handle with/without mask vectors properly (if/as needed).

                rsid.MatchElement faceprintsToMatchObject = new rsid.MatchElement
                {
                    version = faceprintsToMatch.version,
                    flags = faceprintsToMatch.featuresVector[rsid.FaceprintsConsts.RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS],
                    featuresVector = faceprintsToMatch.featuresVector
                };

                int saveMaxScore = -1;
                int winningIndex = -1;
                string winningIdStr = "";
                rsid.MatchResult winningMatchResult = new rsid.MatchResult { success = 0, shouldUpdate = 0, score = 0 };
                rsid.Faceprints winningUpdatedFaceprints = new rsid.Faceprints { }; // dummy init, correct data is set below if condition met.

                int usersIndex = 0;

                // take the value from DeviceConfig.matcherConfidenceLevel.
                var matcherConfidenceLevel = _deviceState.DeviceConfig.matcherConfidenceLevel;

                foreach (var (faceprintsDb, userIdDb) in _db.FaceprintsArray)
                {
                    // note we must send initialized vectors to MatchFaceprintsToFaceprints().
                    // so here we init the updated vector to the existing DB vector before calling MatchFaceprintsToFaceprints()
                    MatchArgs matchArgs = new MatchArgs
                    {
                        newFaceprints = faceprintsToMatchObject,
                        existingFaceprints = faceprintsDb,
                        updatedFaceprints = faceprintsDb, // init updated to existing vector.
                        matcherConfidenceLevel = matcherConfidenceLevel
                    };

                    var matchResult = _authenticator.MatchFaceprintsToFaceprints(ref matchArgs);

                    int currentScore = matchResult.score;

                    // save the best winner that matched.
                    if (matchResult.success == 1)
                    {
                        if (currentScore > saveMaxScore)
                        {
                            saveMaxScore = currentScore;
                            winningMatchResult = matchResult;
                            winningIndex = usersIndex;
                            winningIdStr = userIdDb;
                            winningUpdatedFaceprints = matchArgs.updatedFaceprints;
                        }
                    }

                    usersIndex++;

                } // end of for() loop

                if (winningIndex >= 0) // we have a winner so declare success!
                {
                    VerifyResultAuth(AuthStatus.Success, winningIdStr, string.Empty, null, winningIdStr);

                    ShowLog($"Match info : userIdName = \"{winningIdStr}\", index = \"{winningIndex}\", success = {winningMatchResult.success}, score = {winningMatchResult.score}, should_update = {winningMatchResult.shouldUpdate}.");

                    // apply adaptive-update on the db.
                    if (winningMatchResult.shouldUpdate > 0)
                    {
                        // apply adaptive update
                        // take the updated vector from the matchArgs that were sent by reference and updated 
                        // during call to MatchFaceprintsToFaceprints() .

                        bool updateSuccess = UpdateUser(winningIndex, winningIdStr, ref winningUpdatedFaceprints);

                        ShowLog($"Adaptive DB update for userIdName = \"{winningIdStr}\" (index=\"{winningIndex}\"): status = {updateSuccess} ");
                    }
                    else
                    {
                        ShowLog($"Macth succeeded for userIdName = \"{winningIdStr}\" (index=\"{winningIndex}\"). However adaptive update condition not passed, so no DB update applied.");
                    }
                }
                else // no winner, declare authentication failed!
                {
                    VerifyResultAuth(AuthStatus.Forbidden, string.Empty, "No match found");
                }

            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
        }

        private void RenderDetectedFaces()
        {
            // don't draw on empty image
            if (PreviewImage.Visibility != Visibility.Visible || _previewBitmap == null)
                return;
            // show detected faces                        
            foreach (var (face, status, userId) in _detectedFaces)
            {
                // convert face rect coords FHD=>VGA
                double scaleX, scaleY;
                const double RawLongDim = 1920.0, RawShortDim = 1080.0;
                if (_previewBitmap.Width > _previewBitmap.Height)
                {
                    scaleX = _previewBitmap.Width / RawLongDim;
                    scaleY = _previewBitmap.Height / RawShortDim;
                }
                else
                {
                    scaleX = _previewBitmap.Width / RawShortDim;
                    scaleY = _previewBitmap.Height / RawLongDim;
                }

                var x = face.x * scaleX;
                var y = face.y * scaleY;
                var w = scaleX * face.width;
                var h = scaleY * face.height;

                var stroke = ProgressBrush;
                var strokeThickness = 2;
                // set rect color to green/red if operation succeeed/failed
                if (status.HasValue)
                {
                    stroke = status.Value == AuthStatus.Success ? SuccessBrush : FailBrush;
                    strokeThickness = 3;
                }

                var rect = new Rectangle
                {
                    Width = w,
                    Height = h,
                    Stroke = stroke,
                    StrokeThickness = strokeThickness,
                };

                PreviewCanvas.Children.Add(rect);
                Canvas.SetLeft(rect, x);
                Canvas.SetTop(rect, y);

                Console.WriteLine($"userid {userId}");

                string rectString = userId != null ? userId : string.Empty;
                var showStatus = status.HasValue && (status.Value != AuthStatus.Success || string.IsNullOrEmpty(userId));
                string statusString = showStatus ? Enum.GetName(typeof(AuthStatus), status) : string.Empty;
                rectString = rectString + " " + statusString;

                // print username near the rect if available
                if (!string.IsNullOrEmpty(rectString))
                {
                    var userTextBlock = new TextBlock
                    {
                        FontSize = 38,
                        // flip the back the text because the canvas horizontally flips the preview                   
                        RenderTransformOrigin = new Point(0, 0.5),
                        RenderTransform = new ScaleTransform { ScaleX = -1, ScaleY = 1 },
                        //FontFamily = new FontFamily("Arial"),                    
                        Text = rectString,
                        Foreground = Brushes.White
                    };
                    // display the text on bottom left 
                    PreviewCanvas.Children.Add(userTextBlock);
                    Canvas.SetLeft(userTextBlock, x + w - 4);
                    Canvas.SetTop(userTextBlock, y + h);
                }
            }
        }

        private void UiHandlePreview(PreviewImage image)
        {
            var targetWidth = (int)PreviewImage.Width;
            var targetHeight = (int)PreviewImage.Height;

            //create writable bitmap if not exists or if image size changed
            if (_previewBitmap == null || targetWidth != image.width || targetHeight != image.height)
            {
                PreviewImage.Width = image.width;
                PreviewImage.Height = image.height;
                Console.WriteLine($"Creating new WriteableBitmap preview buffer {image.width}x{image.height}");
                _previewBitmap = new WriteableBitmap(image.width, image.height, 96, 96, PixelFormats.Rgb24, null);
                PreviewImage.Source = _previewBitmap;
            }
            Int32Rect sourceRect = new Int32Rect(0, 0, image.width, image.height);
            lock (_previewMutex)
            {
                _previewBitmap.WritePixels(sourceRect, _previewBuffer, image.stride, 0);
            }
        }

        private bool RawPreviewHandler(ref PreviewImage previewImage)
        {
            System.Drawing.Bitmap bitmap = new System.Drawing.Bitmap(previewImage.width, previewImage.height, previewImage.stride, System.Drawing.Imaging.PixelFormat.Format24bppRgb, previewImage.buffer);

            // flip image for 180/270 degrees needed
            if (_deviceState.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg || _deviceState.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_270_Deg)
                bitmap.RotateFlip(System.Drawing.RotateFlipType.Rotate180FlipNone);

            previewImage.width = bitmap.Width;
            previewImage.height = bitmap.Height;
            previewImage.stride = previewImage.size / previewImage.height;

            var bitmap_data = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, bitmap.Width, bitmap.Height),
                System.Drawing.Imaging.ImageLockMode.ReadOnly, bitmap.PixelFormat);
            Marshal.Copy(bitmap_data.Scan0, _previewBuffer, 0, previewImage.size);
            bitmap.UnlockBits(bitmap_data);
            return true;
        }

        // Handle preview callback.         
        private void OnPreview(PreviewImage image, IntPtr ctx)
        {
            if (_pausePreview)
                return;

            if (image.metadata.sensor_id != 0) // preview only left sensor
                return;

            if (image.height == 640) // ignore snapshot type frames
                return;


            string previewLabel = null;

            lock (_previewMutex)
            {
                // preview image is allways RGB24
                if (_previewBuffer.Length < image.size)
                {
                    Console.WriteLine("Creating preview buffer");
                    _previewBuffer = new byte[image.size];
                }

                if (PreviewImage.Visibility != Visibility.Visible)
                    InvokePreviewVisibility(Visibility.Visible);

                if (_deviceState.PreviewConfig.previewMode == PreviewMode.RAW10_1080P)
                    RawPreviewHandler(ref image); // RAW preview for 180 and 270 need to be flipped
                else
                    Marshal.Copy(image.buffer, _previewBuffer, 0, image.size);

                //calculate FPS
                _fps++;
                if (_fpsStopWatch.Elapsed.Seconds >= 1)
                {
                    var dumpsLabel = _frameDumper != null ? " (dump mode)" : string.Empty;
                    previewLabel = $"{image.width}x{image.height}  {_fps} FPS {dumpsLabel}";
                    _fps = 0;
                    _fpsStopWatch.Restart();
                }
            }
            RenderDispatch(() =>
            {
                if (previewLabel != null)
                    LabelPreviewInfo.Content = previewLabel;
                UiHandlePreview(image);
            });
        }

        private void HandleDumpException(Exception ex)
        {
            _frameDumper = null;
            RenderDispatch(() =>
            {
                ShowErrorMessage("Dump failed", ex.Message + "\nDump stopped..");
            });
        }

        private void OnSnapshot(PreviewImage image, IntPtr ctx)
        {
            try
            {
                DumpImage(image);
            }
            catch (Exception ex)
            {
                HandleDumpException(ex);
            }
        }

        private void DumpImage(PreviewImage image)
        {
            if (_frameDumper != null)
            {
                try
                {
                    if (_deviceState.DeviceConfig.dumpMode == DeviceConfig.DumpMode.FullFrame)
                        _frameDumper.DumpRawImage(image);
                    else
                    {
                        var filename = _frameDumper.DumpPreviewImage(image);
                        ShowDumpFile(filename);

                    }
                }
                catch (Exception ex)
                {
                    HandleDumpException(ex);
                }
            }
        }

        // Display dump file at the top right corner
        private void ShowDumpFile(string filename)
        {
            RenderDispatch(() =>
            {
                try
                {
                    var image = ImageHelper.CreateImageControl(filename);
                    var border = new Border
                    {
                        BorderThickness = new Thickness(3),
                        BorderBrush = Brushes.White,
                        Child = image
                    };

                    Canvas.SetLeft(border, 25);
                    Canvas.SetTop(border, 190);
                    PreviewCanvas.Children.Add(border);
                }
                catch (Exception ex)
                {
                    Logger.Log("ShowDumpFile failed: " + ex.Message);
                }
            });
        }

        private void ResetDetectedFaces()
        {
            RenderDispatch(() =>
            {
                _detectedFaces.Clear();
                PreviewCanvas.Children.Clear();
                PreviewCanvas.Visibility = Visibility.Visible;
                PreviewCanvas.Opacity = 1.0;
            });
        }

        private void OnStartSession(string title, bool activateDumps)
        {
            //activate full preview dumps only if device's DumpMode is enabled 
            activateDumps = activateDumps && _deviceState.DeviceConfig.dumpMode != DeviceConfig.DumpMode.None;
            Dispatcher.Invoke(() =>
            {
                ShowLogTitle(title);
                SetUiEnabled(false);
                RedDot.Visibility = Visibility.Visible;
                _cancelWasCalled = false;
                _lastAuthHint = AuthStatus.Serial_Ok;
                AuthenticatingTextBlock.Text = "Authenticating...";
                EnrollingTextBlock.Text = "Enrolling user...";
                CancelAuthenticationButton.IsEnabled = true;
                ResetDetectedFaces();
                try
                {
                    _frameDumper = activateDumps ? new FrameDumper(_dumpDir, title) : null;
                }
                catch (Exception)
                {
                    _frameDumper = null;
                    ShowErrorMessage("Failed to create the 'dumps' folder",
                        "Please try changing the 'dumps' variable in the 'rsid-viewer.exe.config' file to a different location and restart.");
                }
            });
        }

        private void OnStopSession()
        {
            Dispatcher.Invoke(() =>
            {
                SetUiEnabled(true);
                RedDot.Visibility = Visibility.Hidden;
            });

        }

        // Enroll callbacks
        private void OnEnrollHint(EnrollStatus hint, IntPtr ctx)
        {
            ShowLog(hint.ToString());
            if (hint != EnrollStatus.Success)
                ShowProgressTitle(FriendlyString(hint));
        }

        private void OnEnrollProgress(FacePose pose, IntPtr ctx)
        {
            ShowLog(pose.ToString());
        }

        private void OnEnrollResult(EnrollStatus status, IntPtr ctx)
        {
            ShowLog($"OnEnrollResult status: {status}");

            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                string logmsg;

                if (status == EnrollStatus.Success)
                {
                    logmsg = "Enroll success";
                }
                else if (status == EnrollStatus.EnrollWithMaskIsForbidden)
                {
                    logmsg = "Enroll with mask is forbidden";
                }
                else
                {
                    logmsg = FriendlyString(status);
                }


                string guimsg = logmsg;

                ShowLog(logmsg);

                VerifyResult(status == EnrollStatus.Success, guimsg, guimsg);
            }
        }

        private void OnEnrollExtractionResult(EnrollStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnEnrollExtractionResult status: {status}");

            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                string logmsg;
                if (status != EnrollStatus.Success)
                {
                    if (status == EnrollStatus.EnrollWithMaskIsForbidden)
                    {
                        logmsg = "Enroll with mask is forbidden.";
                    }
                    else
                    {
                        logmsg = FriendlyString(status);
                    }

                    ShowFailedTitle(logmsg);
                    ShowLog(logmsg);
                    return;
                }

                logmsg = "Enroll success";
                var faceprints = (Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(Faceprints));
                string guimsg = logmsg;

                // handle version mismatch (db version vs. faceprint version).
                if ((status == rsid.EnrollStatus.Success) && !(_db.VerifyVersionMatchedDBLE(ref faceprints)))
                {
                    HandleDbErrorServer();
                    logmsg += $" Faceprints (FP) version mismatch. DB={_db.GetVersion()}, FP={faceprints.version}. Saved the DB to backup file and started a new DB from scratch.";
                    guimsg += $" Faceprints version mismatch : DB backuped and cleaned.";
                }

                ShowLog(logmsg);

                // handle enroll 
                VerifyResult(true, guimsg, guimsg, () =>
                {
                    if (_db.Push(faceprints, _lastEnrolledUserId))
                    {
                        _db.Save();
                    }
                    RefreshUserListServer();
                });

            }
        }
        // Return friendly string from authenticate or enroll status
        private string FriendlyString<T>(T status) where T : Enum
        {
            if (Enum.GetName(typeof(T), status) == "NoFaceDetected")
                return "No valid face detected";
            else if (Enum.GetName(typeof(T), status) == "InvalidFeatures")
                return "Image blurred";
            else
                return status.ToString();
        }


        // Authentication callbacks
        private void OnAuthHint(AuthStatus hint, IntPtr ctx)
        {
            if (_lastAuthHint != hint)
            {
                _lastAuthHint = hint;
                ShowLog(hint.ToString());
                ShowProgressTitle(hint.ToString());
            }
            if (hint == AuthStatus.NoFaceDetected)
            {
                ShowFailedTitle("No face detected");
            }
        }

        private void OnAuthResult(AuthStatus status, string userId, IntPtr ctx)
        {
            ShowLog($"OnAuthResult status: {status} \"{userId}\"");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else
            {
                VerifyResultAuth(status, $"{userId}", FriendlyString(status), null, userId);
            }
            _lastAuthHint = AuthStatus.Serial_Ok; // show next hint, session is done            
        }

        private void OnFaceDeteced(IntPtr facesArr, int faceCount, uint ts, IntPtr ctx)
        {
            //convert to face rects
            ResetDetectedFaces();
            var faces = Authenticator.MarshalFaces(facesArr, faceCount);
            foreach (var face in faces)
            {
                ShowLog($"OnFaceDeteced [{face.x},{face.y} {face.width}x{face.height}]");
                RenderDispatch(() => _detectedFaces.Add((face, null, null)));
            }
        }

        public void OnAuthLoopExtractionResult(AuthStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnAuthLoopExtractionResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else if (status == rsid.AuthStatus.Success)
            {
                // handle with/without mask vectors properly (if/as needed).

                var faceprints = (rsid.ExtractedFaceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.ExtractedFaceprints));
                Match(faceprints);
            }
            else
            {
                VerifyResultAuth(status, string.Empty, FriendlyString(status));
            }
            _lastAuthHint = AuthStatus.Serial_Ok; // show next hint, session is done
        }

        private void OnAuthExtractionResult(AuthStatus status, IntPtr faceprintsHandle, IntPtr ctx)
        {
            ShowLog($"OnAuthExtractionResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            else if (status == rsid.AuthStatus.Success)
            {
                // handle with/without mask vectors properly (if/as needed).

                var faceprints = (rsid.ExtractedFaceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.ExtractedFaceprints));
                Match(faceprints);
            }
            else
            {
                VerifyResultAuth(status, string.Empty, FriendlyString(status));
            }
            _lastAuthHint = AuthStatus.Serial_Ok; // show next hint, session is done
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

        private void UpdateUsersUiList(string[] users)
        {
            UsersListView.ItemsSource = users.ToList();
            UsersListView.UnselectAll();
            DeleteButton.IsEnabled = false;
            SelectAllUsersCheckBox.IsChecked = false;
            var usersCount = users.Length;
            UsersTab.Header = $"Users ({usersCount})";
            var usersExist = usersCount > 0;
            InstructionsEnrollUsers.Visibility = usersExist ? Visibility.Collapsed : Visibility.Visible;
            SelectAllUsersCheckBox.IsEnabled = usersExist &&
                _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.All || _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.RecognitionOnly;
            // auth button enabled if there are enrolled users or if we in spoof/face detection only mode
            var authBtnEnabled = AuthenticateButton.IsEnabled =
                usersExist
                || _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.SpoofOnly
                || _deviceState.DeviceConfig.algoFlow == DeviceConfig.AlgoFlow.FaceDetectionOnly;

            AuthenticateButton.IsEnabled = authBtnEnabled;
            AuthenticateLoopToggle.IsEnabled = authBtnEnabled;

            UpdateAuthButtonText();
        }

        // query user list from the device and update the display
        private void RefreshUserList()
        {
            // Query users and update the user list display            
            ShowLog("Query users..");
            SetInstructionsToRefreshUsers(true);
            string[] users;
            var rv = _authenticator.QueryUserIds(out users);
            if (rv != Status.Ok)
            {
                throw new Exception("Query error: " + rv.ToString());
            }

            ShowLog($"{users.Length} users");

            // update the gui and save the list into _userList
            SetInstructionsToRefreshUsers(false);
            BackgroundDispatch(() =>
            {
                UpdateUsersUiList(users);
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
                UpdateUsersUiList(users);
            });
            _userList = users;
        }

        private DeviceState? QueryDeviceMetadata(SerialConfig config)
        {
            var device = new DeviceState();
            device.SerialConfig = config;

            using (var controller = new DeviceController())
            {
                ShowLog($"Connecting to {device.SerialConfig.port}...");
                var status = controller.Connect(device.SerialConfig);
                if (status != Status.Ok)
                {
                    ShowLog("Failed\n");
                    return null;
                }
                ShowLog("Success\n");

                ShowLog("Firmware:");
                var fwVersion = controller.QueryFirmwareVersion();

                var versionLines = fwVersion.ToLower().Split('|');

                foreach (var v in versionLines)
                {
                    var splitted = v.Split(':');
                    if (splitted.Length == 2)
                    {
                        ShowLog($" * {splitted[0].ToUpper()} - {splitted[1]}");
                        if (splitted[0] == "opfw")
                            device.FirmwareVersion = splitted[1];
                        else if (splitted[0] == "recog")
                            device.RecognitionVersion = splitted[1];
                    }
                }
                ShowLog("");

                var sn = controller.QuerySerialNumber();
                device.SerialNumber = sn;
                ShowLog($"S/N: {device.SerialNumber}\n");
                BackgroundDispatch(() => SNText.Text = $"S/N: {device.SerialNumber}");

                try
                {
                    File.WriteAllText(_serialNumberFile, sn);
                }
                catch (Exception ex)
                {
                    // Not critical error. just log it and continue
                    ShowLog("Error writing to sn.txt:\n" + ex.Message);
                }

                ShowLog("Pinging device...");

                status = controller.Ping();
                device.IsOperational = status == Status.Ok;

                ShowLog($"{(device.IsOperational ? "Success" : "Failed")}\n");
            }

            var isCompatible = Authenticator.IsFwCompatibleWithHost(device.FirmwareVersion);
            device.IsCompatible = isCompatible;
            ShowLog($"Is compatible with host? {(device.IsCompatible ? "Yes" : "No")}\n");

            if (_deviceState.IsOperational)
            {
                // device is in operational mode, we continue to query config as usual
                if (!ConnectAuth())
                {
                    ShowLog("Failed\n");
                    return null;
                }
                var deviceConfig = QueryDeviceConfig();
                _authenticator.Disconnect();
                if (deviceConfig.HasValue)
                {
                    device.DeviceConfig = deviceConfig.Value;
                    _deviceState.PreviewConfig.portraitMode = device.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg || device.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;
                    device.PreviewConfig = new PreviewConfig { cameraNumber = Settings.Default.CameraNumber, previewMode = _deviceState.PreviewConfig.previewMode, portraitMode = _deviceState.PreviewConfig.portraitMode };
                }
            }

            return device;
        }
#if RSID_SECURE
        private bool PairDevice()
        {
            ShowLog("Pairing..");
            IntPtr pairArgsHandle = IntPtr.Zero;
            pairArgsHandle = rsid_create_pairing_args_example(_signatureHelpeHandle);
            var pairingArgs = (PairingArgs)Marshal.PtrToStructure(pairArgsHandle, typeof(PairingArgs));

            var rv = _authenticator.Pair(ref pairingArgs);
            if (rv != Status.Ok)
            {
                ShowLog($"Pairing Failed {rv}\n");
                if (pairArgsHandle != IntPtr.Zero) rsid_destroy_pairing_args_example(pairArgsHandle);
                return false;
            }

            ShowLog("Pairing Success\n");
            rsid_update_device_pubkey_example(_signatureHelpeHandle, Marshal.UnsafeAddrOfPinnedArrayElement(pairingArgs.DevicePubkey, 0));            
            return true;
        }

        private bool UnpairDevice()
        {
            ShowLog("Unpairing..");
            var rv = _authenticator.Unpair();
            if (rv != Status.Ok)
            {
                ShowLog("Unpair error " +  rv);                
                return false;
            }                        
            ShowLog("Unpairing success");            
            return true;            
        }
#else
        private bool PairDevice()
        {
            return true;
        }

        private bool UnpairDevice()
        {
            return true;
        }

#endif

        private struct DeviceState
        {
            public string FirmwareVersion;
            public string RecognitionVersion;
            public string SerialNumber;
            public bool IsOperational;
            public bool IsCompatible;
            public SerialConfig SerialConfig;
            public PreviewConfig PreviewConfig;
            public DeviceConfig DeviceConfig;
        }

        private DeviceState DetectDevice()
        {
            SerialConfig config;

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

                config.port = enumeration[0].port;
            }
            else
            {
                config.port = Settings.Default.Port;
            }


            var device = QueryDeviceMetadata(config);
            if (!device.HasValue)
            {
                var msg = "Could not connect to device.\nPlease reconnect the device and try again.";
                ShowErrorMessage("Connection Error", msg);
                throw new Exception("Connection Error");
            }

            return device.Value;
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
            var hostVersion = Authenticator.Version();
            ShowLog("Host: v" + hostVersion + "\n");
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

                var compatibleVersion = Authenticator.CompatibleFirmwareVersion();

                ShowFailedTitle("Device Error");
                var msg = $"Device failed to respond. Please reconnect the device and try again." +
                    $"\nIf the the issue persists, flash firmware version {compatibleVersion} or newer.\n";
                ShowLog(msg);
                ShowErrorMessage("Device Error", msg);

                return;
            }

            if (!_deviceState.IsCompatible)
            {
                OnStopSession();

                var compatibleVersion = Authenticator.CompatibleFirmwareVersion();

                ShowFailedTitle("FW Incompatible");
                var msg = $"Firmware version is incompatible.\nPlease update to version {compatibleVersion} or newer.\n";
                ShowLog(msg);
                ShowErrorMessage("Firmware Version Error", msg);

                return;
            }

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
                UpdateAdvancedMode();

                // start preview
                _deviceState.PreviewConfig.cameraNumber = Settings.Default.CameraNumber;
                _deviceState.PreviewConfig.portraitMode = _deviceState.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg ||
                    _deviceState.DeviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;
                _deviceState.PreviewConfig.rotateRaw = Settings.Default.RawRotate;

                // if device in full dump mode, show raw10 preview only
                if (_deviceState.DeviceConfig.dumpMode == DeviceConfig.DumpMode.FullFrame)
                {
                    _deviceState.PreviewConfig.previewMode = PreviewMode.RAW10_1080P;
                    InvokePreviewVisibility(Visibility.Hidden);

                }

                if (_preview == null)
                    _preview = new Preview(_deviceState.PreviewConfig);
                else
                    _preview.UpdateConfig(_deviceState.PreviewConfig);
                _preview.Start(OnPreview, OnSnapshot);
                if (_flowMode == FlowMode.Server)
                    RefreshUserListServer();
                else
                    RefreshUserList();

                ShowSuccessTitle("Connected");
            }
            catch (Exception ex)
            {
                ShowErrorMessage("Authenticator Error", ex.Message);
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
                RenderDispatch(() => UpdatePairingButtons(true));
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
                _frameDumper?.Cancel();
                VerifyResult(status == Status.Ok, "Canceled", "Cancel failed");
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
        }

        // Enroll Job
        private void EnrollJob(Object threadContext)
        {
            var userId = threadContext as string;

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll {userId}", true);
            IntPtr userIdCtx = Marshal.StringToHGlobalUni(userId);
            try
            {
                ShowProgressTitle("Enroll in progress...");
                _busy = true;
                var enrollArgs = new EnrollArgs
                {
                    userId = userId,
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollResult,
                    progressClbk = OnEnrollProgress,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = userIdCtx
                };
                var status = _authenticator.Enroll(enrollArgs);
                if (status == Status.Ok)
                {
                    HideEnrollingLabelPanel();
                    RefreshUserList();
                }
                else
                {
                    ShowFailedTitle(status.ToString());
                }
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideEnrollingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
                Marshal.FreeHGlobal(userIdCtx);
            }
        }

        // Enroll Job
        private void EnrollImageJob(Object threadContext)
        {
            const int maxImageSize = 900 * 1024;
            if (!ConnectAuth()) return;

            var args = (Tuple<string, string>)threadContext;
            var (userId, imageFilename) = args;

            var (buffer, w, h, bitmap) = ImageHelper.ToBgr(imageFilename, maxImageSize);

            OnStartSession($"Enroll {userId}", true);
            var userIdCtx = Marshal.StringToHGlobalUni(userId);
            try
            {
                // validate file not bigger than max allowed
                if (buffer.Length > maxImageSize)
                    throw new Exception("File too big");

                // show uploaded image on preview panel
                RenderDispatch(() =>
                {
                    var bi = ImageHelper.BitmapToImageSource(bitmap);
                    // flip back horizontally since the preview canvas is flipped
                    var transform = new ScaleTransform { ScaleX = -1 };
                    var image = new Image
                    {
                        Source = bi,
                        RenderTransformOrigin = new Point(0.5, 0.5),
                        RenderTransform = transform,
                        Height = 250,
                    };
                    var border = new Border
                    {
                        BorderThickness = new Thickness(2),
                        BorderBrush = Brushes.White,
                        Child = image
                    };

                    PreviewCanvas.Children.Add(border);
                    Canvas.SetRight(border, 16);
                    Canvas.SetTop(border, 205);
                });

                ShowProgressTitle("Uploading To Device..");
                _busy = true;

                var status = _authenticator.EnrollImage(userId, buffer, w, h);
                if (status == EnrollStatus.Success)
                {
                    RefreshUserList();
                }

                var logMsg = status == EnrollStatus.Success ? "Enroll success" : FriendlyString(status);
                ShowLog(logMsg);
                VerifyResult(status == EnrollStatus.Success, logMsg, logMsg);
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideEnrollingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
                Marshal.FreeHGlobal(userIdCtx);
            }
        }

        // Enroll Job
        private bool EnrollImageJob(EnrollImageRecord enrollRecord, bool isBatch)
        {

            var success = false;
            IntPtr userIdCtx = IntPtr.Zero;
            const int maxImageSize = 900 * 1024;
            if (!ConnectAuth()) return false;

            try
            {
                var (buffer, w, h, bitmap) = ImageHelper.ToBgr(enrollRecord.Filename, maxImageSize);

                OnStartSession($"Enroll {enrollRecord.UserId}", true);
                userIdCtx = Marshal.StringToHGlobalUni(enrollRecord.UserId);

                // validate file not bigger than max allowed
                if (buffer.Length > maxImageSize)
                    throw new Exception("File too big");

                // show uploaded image on preview panel
                RenderDispatch(() =>
                {
                    var bi = ImageHelper.BitmapToImageSource(bitmap);
                    // flip back horizontally since the preview canvas is flipped
                    var transform = new ScaleTransform { ScaleX = -1 };
                    var image = new Image
                    {
                        Source = bi,
                        RenderTransformOrigin = new Point(0.5, 0.5),
                        RenderTransform = transform,
                        Height = 250,
                    };
                    var border = new Border
                    {
                        BorderThickness = new Thickness(2),
                        BorderBrush = Brushes.White,
                        Child = image
                    };

                    PreviewCanvas.Children.Add(border);
                    Canvas.SetRight(border, 16);
                    Canvas.SetTop(border, 205);
                });

                ShowProgressTitle("Uploading To Device..");
                _busy = true;

                var status = _authenticator.EnrollImage(enrollRecord.UserId, buffer, w, h);
                if (status == EnrollStatus.Success && !isBatch)
                {
                    RefreshUserList();
                }

                var logMsg = status == EnrollStatus.Success ? "Enroll success" : FriendlyString(status);
                VerifyResult(status == EnrollStatus.Success, logMsg, logMsg);
                success = status == EnrollStatus.Success;
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
                OnStopSession();
            }
            finally
            {
                if (!isBatch) OnStopSession();
                HideEnrollingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
                if (userIdCtx != IntPtr.Zero)
                    Marshal.FreeHGlobal(userIdCtx);
            }
            return success;
        }

        private bool EnrollImageHostJob(EnrollImageRecord enrollRecord, bool isBatch)
        {

            var success = false;
            IntPtr userIdCtx = IntPtr.Zero;
            const int maxImageSize = 900 * 1024;
            if (!ConnectAuth()) return false;

            try
            {
                var (buffer, w, h, bitmap) = ImageHelper.ToBgr(enrollRecord.Filename, maxImageSize);

                OnStartSession($"Enroll {enrollRecord.UserId}", true);
                userIdCtx = Marshal.StringToHGlobalUni(enrollRecord.UserId);

                // validate file not bigger than max allowed
                if (buffer.Length > maxImageSize)
                    throw new Exception("File too big");

                // show uploaded image on preview panel
                RenderDispatch(() =>
                {
                    var bi = ImageHelper.BitmapToImageSource(bitmap);
                    // flip back horizontally since the preview canvas is flipped
                    var transform = new ScaleTransform { ScaleX = -1 };
                    var image = new Image
                    {
                        Source = bi,
                        RenderTransformOrigin = new Point(0.5, 0.5),
                        RenderTransform = transform,
                        Height = 250,
                    };
                    var border = new Border
                    {
                        BorderThickness = new Thickness(2),
                        BorderBrush = Brushes.White,
                        Child = image
                    };

                    PreviewCanvas.Children.Add(border);
                    Canvas.SetRight(border, 16);
                    Canvas.SetTop(border, 205);
                });

                ShowProgressTitle("Uploading To Device..");
                _busy = true;

                var faceprints = new rsid.Faceprints();
                var status = _authenticator.EnrollImageFeatureExtraction(enrollRecord.UserId, buffer, w, h, ref faceprints);
                if (status == EnrollStatus.Success && !isBatch)
                {
                    _db.Push(faceprints, enrollRecord.UserId);
                    _db.Save();
                    RefreshUserListServer();
                }

                var logMsg = status == EnrollStatus.Success ? "Enroll success" : FriendlyString(status);
                VerifyResult(status == EnrollStatus.Success, logMsg, logMsg);
                success = status == EnrollStatus.Success;
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
                OnStopSession();
            }
            finally
            {
                if (!isBatch) OnStopSession();
                HideEnrollingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
                if (userIdCtx != IntPtr.Zero)
                    Marshal.FreeHGlobal(userIdCtx);
            }
            return success;
        }

        // Enroll Job
        private void EnrollExtractFaceprintsJob(Object threadContext)
        {
            var userId = threadContext as string;

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll {userId}", true);
            try
            {
                _lastEnrolledUserId = userId + '\0';
                var enrollExtArgs = new EnrollExtractArgs
                {
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollExtractionResult,
                    progressClbk = OnEnrollProgress,
                    faceDetectedClbk = OnFaceDeteced,
                };
                Status status = _authenticator.EnrollExtractFaceprints(enrollExtArgs);
                if (status != Status.Ok)
                    ShowFailedTitle(status.ToString());
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
            OnStartSession($"Delete {usersIds.Count} users", false);

            try
            {
                ShowProgressTitle("Deleting..");
                bool successAll = true;
                foreach (string userId in usersIds)
                {
                    ShowLog($"Delete user {userId}");
                    var status = _authenticator.RemoveUser(userId);
                    if (status == Status.Ok)
                    {
                        ShowLog("Detele Ok");
                    }
                    else
                    {
                        ShowLog("Failed");
                        successAll = false;
                    }
                }

                VerifyResult(successAll, "Delete success", "Delete failed");
                RefreshUserList();
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
            OnStartSession("Delete Users", false);
            try
            {
                ShowProgressTitle("Deleting..");
                var status = _authenticator.RemoveAllUsers();
                VerifyResult(status == Status.Ok, "Delete success", "Delete failed", RefreshUserList);
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
                    if (success)
                    {
                        ShowLog("Delete Ok");
                    }
                    else
                    {
                        ShowLog("Delete Failed");
                        successAll = false;
                    }
                }
                _db.Save();
                VerifyResult(successAll, "Delete success", "Delete failed");
                RefreshUserListServer();
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
            OnStartSession("Delete Users", false);
            try
            {
                ShowProgressTitle("Deleting..");
                var success = _db.RemoveAll();
                _db.Save();
                VerifyResult(success, "Delete all success", "Delete all failed", RefreshUserListServer);
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
            OnStartSession("Authenticate", true);
            try
            {
                var authArgs = new AuthArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };

                ShowProgressTitle("Authenticating..");
                _busy = true;
                // don't show refresh preview while authenticating                 
                PausePreview();
                var sw = Stopwatch.StartNew();
                Status status = _authenticator.Authenticate(authArgs);
                ShowLog($"{sw.ElapsedMilliseconds} milliseconds");
                if (status != Status.Ok)
                    ShowFailedTitle(status.ToString());
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                ResumePreviewAfter(500);
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
            }
        }

        // Authentication loop job
        private void AuthenticateLoopJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Authenticate Loop", true);
            try
            {
                var authArgs = new AuthArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };

                ShowProgressTitle("Authenticating..");
                _busy = true;
                _authenticator.AuthenticateLoop(authArgs);
            }
            catch (Exception ex)
            {
                try
                {
                    _authenticator.Cancel(); //try to cancel the auth loop
                }
                catch
                {
                    // ignored
                }

                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
            }
        }

        // Authenticate faceprints extraction job
        private void AuthenticateExtractFaceprintsJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Extracting Faceprints", true);
            try
            {
                var authExtArgs = new AuthExtractArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthExtractionResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };
                ShowProgressTitle("Extracting Faceprints");
                _busy = true;
                PausePreview();
                Status status = _authenticator.AuthenticateExtractFaceprints(authExtArgs);
                if (status != Status.Ok)
                    ShowFailedTitle(status.ToString());
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                ResumePreviewAfter(500);
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authenticator.Disconnect();
                _busy = false;

            }
        }

        // Authenticate loop faceprints extraction job
        private void AuthenticateExtractFaceprintsLoopJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("Authentication faceprints extraction loop", true);
            try
            {
                var authLoopExtArgs = new AuthExtractArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthLoopExtractionResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };
                ShowProgressTitle("Authenticating..");
                _busy = true;
                _authenticator.AuthenticateLoopExtractFaceprints(authLoopExtArgs);
            }
            catch (Exception ex)
            {
                try
                {
                    _authenticator.Cancel(); //try to cancel the auth loop
                }
                catch
                {
                    // ignored
                }

                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _busy = false;
                _authenticator.Disconnect();
            }
        }

        private void UnlockJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Unlock", false);
            try
            {
                ShowProgressTitle("Unlocking..");
                var status = _authenticator.Unlock();
                VerifyResult(status == Status.Ok, "Unlock success", "Unlock failed");
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                _authenticator.Disconnect();
            }
        }

        private void SetHostDatabasePath()
        {
            if (File.Exists(Database.GetDatabseDefaultPath()))
            {
                var result = Application.Current.Dispatcher.Invoke(() =>
               {
                   var dialogResult = ShowWindowDialog(new OKCancelDialog("Existing DB detected!",
                       "Load existing default DB?"));
                   if (dialogResult == true)
                   {
                       _db = new Database();
                       return true;
                   }
                   return false;
               });
                if (result)
                    return;
            }

            OpenFileDialog openFileDialog = new OpenFileDialog()
            {
                Multiselect = false,
                Title = "Select Database File",
                Filter = "db files (*.db)|*.db",
                FilterIndex = 1,
                CheckFileExists = false
            };
            if (openFileDialog.ShowDialog() == true)
            {
                var dbfilename = openFileDialog.FileName;
                _db = new Database(dbfilename);
            }
            else
            {
                ShowErrorMessage("Default DB",
                    "No db file selected.\nUsing the default path (<current dir>/db.db)");
                _db = new Database();
            }

        }

        // SetDeviceConfig job
        private void SetDeviceConfigJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            (DeviceConfig? prevDeviceConfig, var deviceConfig, var prevPreviewConfig, var PreviewConfig, var flowMode) =
                ((DeviceConfig?, DeviceConfig, PreviewConfig, PreviewConfig, FlowMode))threadContext;
            OnStartSession("SetDeviceConfig", false);
            try
            {

                ShowProgressTitle("SetDeviceConfig");

                // Must use raw10 if full dump mode enabled
                if (deviceConfig.dumpMode == DeviceConfig.DumpMode.FullFrame)
                {
                    PreviewConfig.previewMode = PreviewMode.RAW10_1080P;
                }
                else // if not not in full dump mode, make sure we not in raw 10 preview
                {
                    if (PreviewConfig.previewMode == PreviewMode.RAW10_1080P)
                    {
                        PreviewConfig.previewMode = PreviewMode.MJPEG_1080P;
                    }
                }

                LogDeviceConfig(deviceConfig);


                if (prevDeviceConfig.HasValue)
                {
                    DeviceConfig prevDeviceConfigValue = prevDeviceConfig.Value;
                    ShowLog("Detected changes. Updating settings on device...");
                    var status = _authenticator.SetDeviceConfig(deviceConfig);
                    if (status != Status.Ok)
                    {
                        throw new Exception(status.ToString());
                    }

                    // restart preview with new config if needed
                    if (prevPreviewConfig.previewMode != PreviewConfig.previewMode || prevPreviewConfig.portraitMode != PreviewConfig.portraitMode)
                    {
                        _deviceState.PreviewConfig = new PreviewConfig
                        {
                            cameraNumber = Settings.Default.CameraNumber,
                            previewMode = PreviewConfig.previewMode,
                            portraitMode = PreviewConfig.portraitMode,
                            rotateRaw = Settings.Default.RawRotate
                        };
                        _preview?.Stop();
                        _preview?.UpdateConfig(_deviceState.PreviewConfig);
                        _preview?.Start(OnPreview, OnSnapshot);
                    }

                    _deviceState.DeviceConfig = deviceConfig;

                    if (_deviceState.PreviewConfig.previewMode != PreviewMode.RAW10_1080P)
                        InvokePreviewVisibility(Visibility.Visible);
                    else
                        InvokePreviewVisibility(Visibility.Hidden);
                }


                if (flowMode != _flowMode)
                {
                    _flowMode = flowMode;

                    if (flowMode == FlowMode.Server)
                    {

                        SetHostDatabasePath();

                        Dispatcher.Invoke(() =>
                        {
                            ImportButton.IsEnabled = false;
                            ExportButton.IsEnabled = false;
                        });

                        int loadStatus = _db.Load();

                        if (loadStatus < 0)
                        {
                            HandleDbErrorServer();
                            ShowLog("Error occured during load the DB. This may be due to faceprints version mismatch or other error. Saved backup and started empty DB.\n");
                            string guimsg = "DB load error. Saved backup and stared empty DB.";
                            VerifyResult(false, string.Empty, guimsg);
                            return;
                        }
                        else
                        {
                            RefreshUserListServer();
                        }

                    }
                    else
                    {
                        RefreshUserList();
                    }
                }

                VerifyResult(true, "Apply settings done", "Apply settings failed");
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
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
            var args = (Tuple<string, bool>)(threadContext);
            var (binPath, forceUpdate) = args;
            using (var fwUpdater = new FwUpdater())
            {
                BackgroundDispatch(() => _progressBar.Show());
                var versions = fwUpdater.ExtractFwVersion(binPath);
                var newFwVersion = versions?.OpfwVersion;

                if (newFwVersion == null)
                {
                    CloseProgressBar();
                    ShowErrorMessage("FW Update Error", "Unable to parse the selected firmware file.");
                    return;
                }

                if (!forceUpdate)
                {
                    var isCompatible = Authenticator.IsFwCompatibleWithHost(newFwVersion);
                    if (!isCompatible)
                    {
                        CloseProgressBar();
                        var compatibleVer = Authenticator.CompatibleFirmwareVersion();
                        var msg = "Selected firmware version is not compatible with this SDK.\n";
                        msg += $"Chosen Version:   {newFwVersion}\n";
                        msg += $"Compatible Versions: {compatibleVer} and above\n";
                        ShowErrorMessage("Incompatible FW Version", msg);
                        return;
                    }
                }

                var fwUpdateSettings = new FwUpdater.FwUpdateSettings
                {
                    port = _deviceState.SerialConfig.port,
                    force_full = forceUpdate ? 1 : 0
                };


                var isSkuCompatible = fwUpdater.IsSkuCompatible(fwUpdateSettings, binPath, out int expectedSkuVer, out int deviceSkuVer);
                if (!isSkuCompatible)
                {
                    CloseProgressBar();
                    var msg = "Selected firmware version is incompatible with your F450 model.\n";
                    msg += "Please make sure the firmware file you're using is for SKU" + deviceSkuVer + " devices and try again.\n";
                    ShowErrorMessage("Incompatible firmware encryption version", msg);
                    return;
                }

                FwUpdater.UpdatePolicyInfo updatePolicyInfo;
                fwUpdater.DecideUpdatePolicy(fwUpdateSettings, binPath, out updatePolicyInfo);
                if (updatePolicyInfo.updatePolicy == FwUpdater.UpdatePolicy.Not_Allowed)
                {
                    CloseProgressBar();
                    var msg = "Update from current device firmware to selected firmware file is unsupported by this host application.\n";
                    ShowErrorMessage("Unsupported firmware version", msg);
                    return;
                }
                if (updatePolicyInfo.updatePolicy == FwUpdater.UpdatePolicy.Require_Intermediate_Fw)
                {
                    CloseProgressBar();
                    string version = new string(updatePolicyInfo.intermediateVersion);
                    var msg = "Firmware cannot be updated directly to the chosen version.\n";
                    msg += "Flash firmware version " + version + " first.\n";
                    ShowErrorMessage("Unsupported firmware version", msg);
                    return;
                }

                _authenticator?.Disconnect();
                _preview?.Stop();
                TogglePreviewOpacity(false);
                _deviceState.IsOperational = false;
                Thread.Sleep(100);

                OnStartSession("Firmware Update", false);
                bool success = false;
                try
                {
                    ShowProgressTitle("Updating Firmware..");
                    ShowLog("update to " + newFwVersion);

                    var eventHandler = new FwUpdater.EventHandler
                    {
                        progressClbk = (progress) => UpdateProgressBar(progress * 100)
                    };

                    var status = fwUpdater.Update(binPath, eventHandler, fwUpdateSettings);
                    success = status == Status.Ok;
                    if (!success)
                        throw new Exception("Update Failed");
                }
                catch (Exception ex)
                {
                    _deviceState.IsOperational = false;
                    success = false;
                    ShowFailedTitle(ex.Message);
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
                        TogglePreviewOpacity(true);
                        InitialSession(null);
                    }
                }
            }

        }

        private void PausePreview()
        {
            // if in raw10 dont pause preview so preview snapshots can be displayed
            if (_deviceState.PreviewConfig.previewMode != PreviewMode.RAW10_1080P)
                _pausePreview = true;
        }

        // Resume preview aftr the given delay
        private void ResumePreviewAfter(int delayMillis)
        {
            if (_pausePreview)
            {
                Task.Delay(delayMillis).Wait();
                _pausePreview = false;
            }
        }

#if RSID_SECURE
        private void UpdatePairingButtons(bool paired)
        {
            if (paired)
            {
                PairButton.Visibility = Visibility.Collapsed;
                PairButton.IsEnabled = false;
                UnpairButton.Visibility = Visibility.Visible;
                UnpairButton.IsEnabled = true;
            }
            else
            {
                PairButton.Visibility = Visibility.Visible;
                PairButton.IsEnabled = true;
                UnpairButton.Visibility = Visibility.Collapsed;
                UnpairButton.IsEnabled = false;
            }
}
        private async void Unpair_Click(object sender, RoutedEventArgs e)
        {
            SetUiEnabled(false);
            var ok = await Task.Run(() =>
            {
                try
                {
                    ConnectAuth();                    
                    return UnpairDevice();
                }catch (Exception ex)
                {
                    ShowErrorMessage("Unpair", ex.Message);
                    return false;
                }
            });
            
            UpdatePairingButtons(!ok);
            if (ok)
            {
                new ErrorDialog("Device Successfully Unpaired", "Press the key button to pair the device again.").ShowDialog();
            }
        }

        private async void Pair_Click(object sender, RoutedEventArgs e)
        {
            SetUiEnabled(false);            
            var ok = await Task.Run(() => {
                try
                {
                    ConnectAuth();                    
                    return PairDevice();
                }
                catch (Exception ex)
                {
                    ShowErrorMessage("Pair", ex.Message);
                    return false;
                }
            });

            SetUiEnabled(ok);
            UpdatePairingButtons(ok);                        
        }
#else // Non secure mode. No pairing available
        private void UpdatePairingButtons(bool paired)
        {
            PairButton.Visibility = Visibility.Collapsed;
            UnpairButton.Visibility = Visibility.Collapsed;
        }

        private void Unpair_Click(object sender, RoutedEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void Pair_Click(object sender, RoutedEventArgs e)
        {
            throw new NotImplementedException();
        }
#endif


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
        private const string DllName = "rsid_secure_helper_debug";
#else
        private const string DllName = "rsid_secure_helper";
#endif //DEBUG
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_example_sig_clbk();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_example_sig_clbk(IntPtr rsid_signature_clbk);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_get_host_pubkey_example(IntPtr rsid_signature_clbk);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_update_device_pubkey_example(IntPtr rsid_signature_clbk, IntPtr device_key);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr rsid_create_pairing_args_example(IntPtr rsid_signature_clbk);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void rsid_destroy_pairing_args_example(IntPtr pairing_args);

    }
}