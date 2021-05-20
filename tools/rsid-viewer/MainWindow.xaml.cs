// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using System.Windows.Shapes;
using Microsoft.Win32;


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

#if RSID_PREVIEW
        private bool _previewEnabled = true;
#else
        private bool _previewEnabled = false;
#endif

        private DeviceState _deviceState;
        private rsid.Authenticator _authenticator;
        private FlowMode _flowMode;

        private bool _dumpEnabled = false;
        private rsid.Preview _preview;
        private WriteableBitmap _previewBitmap;
        private byte[] _previewBuffer = new byte[0]; // store latest frame from the preview callback
        // tuple of (Face,IsAuthenticated,UserId) in current session
        private List<(rsid.FaceRect, rsid.AuthStatus?, string userId)> _detectedFaces = new List<(rsid.FaceRect, rsid.AuthStatus?, string)>();
        private List<UInt32> _detectedFacesTs = new List<UInt32>();
        private object _previewMutex = new object();

        private string[] _userList = new string[0]; // latest user list that was queried from the device

        private bool _authloopRunning = false;
        private bool _cancelWasCalled = false;
        private string _lastEnrolledUserId;
        private rsid.AuthStatus _lastAuthHint = rsid.AuthStatus.Serial_Ok; // To show only changed hints. 

        private IntPtr _signatureHelpeHandle = IntPtr.Zero;
        private Database _db;// = new Database();

        private string _dumpDir;
        private ConsoleWindow _consoleWindow;
        private ProgressBarDialog _progressBar;

        private float _userFeedbackTime = 0;

        private int _fps = 0;
        private System.Diagnostics.Stopwatch _fpsStopWatch = new System.Diagnostics.Stopwatch();
        private FrameDumper _frameDumper;

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
            _fpsStopWatch.Start();
            if (_previewEnabled == false)
                LabelPreview.Visibility = Visibility.Collapsed;
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
            OnStartSession(string.Empty, false);
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
                    Thread.Sleep(500); // give time to device to cancel before exiting

                }
                catch { }
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

            if (ShowWindowDialog(new DeleteUserInput()) == true)
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
            rsid.DeviceConfig? deviceConfig = null;
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

            var dialog = new AuthSettingsInput(_deviceState.FirmwareVersion, deviceConfig, _flowMode, _dumpEnabled, _previewEnabled);

            if (ShowWindowDialog(dialog) == true)
            {
                if (string.IsNullOrEmpty(dialog.FirmwareFileName) == true)
                {
                    ThreadPool.QueueUserWorkItem(SetDeviceConfigJob, (deviceConfig, dialog.Config, dialog.FlowMode));
                    _dumpEnabled = dialog.DumpingEnabled;
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

            SetUIEnabled(false);
            var dbfilename = sfd.FileName;
            if (!ConnectAuth()) return;
            var db = new Database(dbfilename);
            var exported_db = _authenticator.GetUsersFaceprints();
            try
            {
                foreach (var uf in exported_db)
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
                SetUIEnabled(true);
            }
            if (exported_db.Count > 0)
                ShowSuccessTitle("Database file was created successfully");
            else
            {
                ShowFailedTitle("No users were exported");
                ShowErrorMessage("Export DB", "Error while exporting users!");
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
            List<rsid.UserFaceprints> users_from_db = new List<rsid.UserFaceprints>();

            var db = new Database(dbfilename);
            db.Load();

            foreach (var (faceprintsDb, userIdDb) in db.faceprintsArray)
            {
                var uf = new rsid.UserFaceprints();
                uf.userID = userIdDb;
                uf.faceprints = faceprintsDb;
                users_from_db.Add(uf);
            }

            try
            {
                SetUIEnabled(false);
                if (_authenticator.SetUsersFaceprints(users_from_db))
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
                ThreadPool.QueueUserWorkItem(StandbyJob);
                SetUIEnabled(true);
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
        // never show preview if with we in "Dump" preview mode
        // Show as requested in "RGB" preview mode
        private void SetPreviewVisibility(Visibility visibility)
        {
            var previewMode = _deviceState.PreviewConfig.previewMode;
            LabelPreview.Content = $"Camera Preview\n({previewMode.ToString().ToLower()} preview mode)";
            PreviewImage.Visibility = visibility;
        }

        private void PreviewImage_MouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (_deviceState.PreviewConfig.previewMode == rsid.PreviewMode.RAW10_1080P || !_deviceState.IsOperational)
                return;
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

        private bool? ShowWindowDialog(Window window)
        {
            SetUIEnabled(false);
            bool? returnOK = window.ShowDialog();
            SetUIEnabled(true);
            return returnOK;
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

        private string GetFailedSpoofMsg(rsid.AuthStatus status)
        {
            //var msg = status.ToString();
            if ((int)status >= (int)rsid.AuthStatus.Reserved1 || (int)status == (int)rsid.AuthStatus.Forbidden)
                return "Spoof Attempt";
            else if (status == rsid.AuthStatus.NoFaceDetected)
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

        private void VerifyResultAuth(rsid.AuthStatus status, string successMessage, string failMessage, Action onSuccess = null, string userId = null)
        {
            VerifyResult(status == rsid.AuthStatus.Success, successMessage, failMessage, onSuccess);
            UpdateFaceResult(status, userId);
        }

        private void UpdateFaceResult(rsid.AuthStatus status, string userId)
        {
            // updated the detected face success balue if exists
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
                _progressBar.Close();
                _progressBar = null;
            });
        }

        private void SetUIEnabled(bool isEnabled)
        {
            SettingsButton.IsEnabled = isEnabled;
            DeleteButton.IsEnabled = isEnabled && UsersListView.SelectedItems.Count > 0;
            ImportButton.IsEnabled = isEnabled && (_flowMode != FlowMode.Server); ;
            ExportButton.IsEnabled = isEnabled && (_flowMode != FlowMode.Server); ;
            EnrollButton.IsEnabled = isEnabled;
            AuthenticateButton.IsEnabled = isEnabled && _userList?.Length > 0;
            StandbyButton.IsEnabled = isEnabled && (_flowMode != FlowMode.Server);
            UsersListView.IsEnabled = isEnabled;
            SelectAllUsersCheckBox.IsEnabled = isEnabled && _userList?.Length > 0;
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
                StandbyButton.IsEnabled = false;
                ImportButton.IsEnabled = false;
                ExportButton.IsEnabled = false;

                int loadStatus = _db.Load();

                if (loadStatus < 0)
                {
                    HandleDbErrorServer();
                    ShowLog("Error occured during loading the DB. This may be due to faceprints version mismatch or other error. Saved the old DB to backup and started an empty DB.\n");
                    string guimsg = "DB version or load error.";
                    VerifyResult(false, string.Empty, guimsg);
                    return;
                }
            }
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
                ShowFailedTitle("Connection Error");
                ShowLog("Connection error");
                ShowErrorMessage($"Connection Failed to Port {_deviceState.SerialConfig.port}",
                    $"Connection Error.\n\nPlease check the serial port setting in the config file.");
                return false;
            }
            return true;
        }

        private void LogDeviceConfig(rsid.DeviceConfig deviceConfig)
        {
            ShowLog(" * Camera Rotation: " + deviceConfig.cameraRotation.ToString());
            ShowLog(" * Confidence Level: " + deviceConfig.securityLevel.ToString());
            ShowLog(" * Preview Mode: " + deviceConfig.previewMode.ToString());
            ShowLog(" * Algo Flow: " + deviceConfig.algoFlow);
            ShowLog(" * Face Selection Policy: " + deviceConfig.faceSelectionPolicy);
            ShowLog(" * Host Mode: " + _flowMode);
            ShowLog(" * Camera Index: " + _deviceState.PreviewConfig.cameraNumber);
            ShowLog("");
        }

        private rsid.DeviceConfig? QueryDeviceConfig()
        {

            ShowLog("");
            ShowLog("Query device config..");
            rsid.DeviceConfig deviceConfig;
            var rv = _authenticator.QueryDeviceConfig(out deviceConfig);
            if (rv != rsid.Status.Ok)
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
            rsid.DeviceConfig? deviceConfig = QueryDeviceConfig();
            if (!deviceConfig.HasValue)
            {
                var msg = "Failed to query device config";
                ShowLog(msg);
                ShowErrorMessage("QueryDeviceConfig Error", msg);
                throw new Exception("QueryDeviceConfig Error");
            }

            _deviceState.PreviewConfig.previewMode = (rsid.PreviewMode)deviceConfig.Value.previewMode;

            //if (_deviceState.AdvancedMode)
            //{
            //    _deviceState.PreviewConfig.previewMode = (rsid.PreviewMode)deviceConfig.Value.previewMode;
            //}
            //else
            //{
            //    if (deviceConfig.Value.previewMode == rsid.DeviceConfig.PreviewMode.RAW10_1080P)
            //    {
            //        rsid.DeviceConfig newDeviceConfig = deviceConfig.Value;
            //        newDeviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.MJPEG_1080P;
            //        SetDeviceConfigJob((deviceConfig, newDeviceConfig, _flowMode));
            //    }
            //}
        }

        private bool UpdateUser(int userIndex, string userId, ref rsid.Faceprints updatedFaceprints)
        {
            bool success = _db.UpdateUser(userIndex, userId, ref updatedFaceprints);

            if (success)
            {
                _db.Save();
            }

            return success;
        }

        private bool HandleDbErrorServer()
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
            bool success = true;

            if (_flowMode == FlowMode.Server)
            {
                success = _db.SaveBackupAndDeleteDb();
                RefreshUserListServer();
            }

            return success;
        }

        private void Match(rsid.Faceprints faceprintsToMatch)
        {
            try
            {
                ShowProgressTitle("Matching faceprints to database");

                // if Faceprints versions don't match - return with error message.
                if (!(_db.VerifyVersionMatched(ref faceprintsToMatch)))
                {
                    HandleDbErrorServer();
                    string logmsg = $"Faceprints (FP) version mismatch: DB={ _db.GetVersion()}, FP={faceprintsToMatch.version}. Saved the old DB to backup file and started a new DB from scratch.";
                    string guimsg = $"Faceprints (FP) version mismatch: DB={ _db.GetVersion()}, FP={faceprintsToMatch.version}. DB backuped and cleaned.";
                    ShowLog(logmsg);
                    VerifyResult(false, string.Empty, guimsg);
                    return;
                }

                // TODO yossidan - handle with/without mask vectors properly (if/as needed).

                foreach (var (faceprintsDb, userIdDb) in _db.faceprintsArray)
                {
                    // note we must send initialized vectors to MatchFaceprintsToFaceprints().
                    // so here we init the updated vector to the existing DB vector before calling MatchFaceprintsToFaceprints()
                    rsid.MatchArgs matchArgs = new rsid.MatchArgs
                    {
                        newFaceprints = faceprintsToMatch,
                        existingFaceprints = faceprintsDb,
                        updatedFaceprints = faceprintsDb // init updated to existing vector.
                    };

                    // TODO yossidan - handle with/without mask vectors properly (if/as needed).

                    var matchResult = _authenticator.MatchFaceprintsToFaceprints(ref matchArgs);
                    var userIndex = 0;
                    if (matchResult.success == 1)
                    {
                        VerifyResultAuth(rsid.AuthStatus.Success, $"\"{userIdDb}\"", string.Empty, null, userIdDb);

                        // update the DB with the updated faceprints.
                        if (matchResult.shouldUpdate > 0)
                        {
                            // take the updated vector from the matchArgs that were sent by reference and updated 
                            // during call to MatchFaceprintsToFaceprints() .

                            bool update_success = UpdateUser(userIndex, userIdDb, ref matchArgs.updatedFaceprints);
                            ShowLog($"Adaptive DB update status for user-id \"{userIdDb}\": {update_success} ");
                        }
                        else
                        {
                            ShowLog($"Macth succeeded for user \"{userIdDb}\". However adaptive update condition not passed, so no DB update applied.");
                        }
                        return;
                    }
                    userIndex++;
                }
                VerifyResultAuth(rsid.AuthStatus.Forbidden, string.Empty, "No match found");
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
            }
        }

        private void RenderDetectedFaces()
        {
            if (PreviewImage.Visibility != Visibility.Visible) // don't draw on empty image
                return;
            // show detected faces                        
            foreach (var (face, status, userId) in _detectedFaces)
            {
                // convert face rect coords FHD=>VGA
                double scaleX = _previewBitmap.Width / 1080.0;
                double scaleY = _previewBitmap.Height / 1920.0;
                var x = face.x * scaleX;
                var y = face.y * scaleY;
                var w = scaleX * face.width;
                var h = scaleY * face.height;

                var stroke = ProgressBrush;
                var strokeThickness = 2;
                // set rect color to green/red if operation succeeed/failed
                if (status.HasValue)
                {
                    stroke = status.Value == rsid.AuthStatus.Success ? SuccessBrush : FailBrush;
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
                var showStatus = status.HasValue && (status.Value != rsid.AuthStatus.Success || string.IsNullOrEmpty(userId));
                string statusString = showStatus ? Enum.GetName(typeof(rsid.AuthStatus), status) : string.Empty;
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

        private void UIHandlePreview(rsid.PreviewImage image)
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

        private bool RawPreviewHandler(ref rsid.PreviewImage raw_image, ref rsid.PreviewImage preview_image)
        {
            if (raw_image.metadata.sensor_id != 0) // preview only left sensor
            {
                return false;
            }
            if (PreviewImage.Visibility != Visibility.Visible)
                InvokePreviewVisibility(Visibility.Visible);
            preview_image.buffer = Marshal.AllocHGlobal(preview_image.size);
            if (!_preview.RawToRgb(ref raw_image, ref preview_image))
            {
                Marshal.FreeHGlobal(preview_image.buffer);
                return false;
            }
            Marshal.Copy(preview_image.buffer, _previewBuffer, 0, preview_image.size);
            Marshal.FreeHGlobal(preview_image.buffer);
            return true;
        }

        // Handle preview callback.         
        private void OnPreview(rsid.PreviewImage image, IntPtr ctx)
        {
            string previewLabel = null;
            rsid.PreviewImage preview_image = new rsid.PreviewImage();
            lock (_previewMutex)
            {
                // dump original data
                if (_dumpEnabled)
                {
                    DumpFrame(image);
                }

                // preview image is allways RGB24
                preview_image.size = image.width * image.height * 3;
                if (_previewBuffer.Length != preview_image.size)
                {
                    Console.WriteLine("Creating preview buffer");
                    _previewBuffer = new byte[preview_image.size];
                }
                // convert raw to rgb for preview
                if (_deviceState.PreviewConfig.previewMode == rsid.PreviewMode.RAW10_1080P)
                {
                    if (RawPreviewHandler(ref image, ref preview_image) == false)
                        return;
                }
                else
                {
                    preview_image = image;
                    Marshal.Copy(preview_image.buffer, _previewBuffer, 0, preview_image.size);
                }

                /// calculate FPS
                _fps++;
                if (_fpsStopWatch.Elapsed.Seconds >= 1)
                {
                    var dumpsLabel = _frameDumper != null ? " (dumps enabled)" : string.Empty;
                    previewLabel = $"{image.width}x{image.height}  {_fps} FPS {dumpsLabel}";
                    _fps = 0;
                    _fpsStopWatch.Restart();
                }
            }
            RenderDispatch(() =>
            {
                if (previewLabel != null)
                    LabelPreviewInfo.Content = previewLabel;
                UIHandlePreview(preview_image);
            });
        }

        private void HandleDumpException(Exception ex)
        {
            _dumpEnabled = false;
            _frameDumper = null;
            RenderDispatch(() =>
            {
                ShowErrorMessage("Dump failed", ex.Message + "\nDump stopped..");
            });
        }

        private void DumpFrame(rsid.PreviewImage image)
        {
            if (_dumpEnabled && _frameDumper != null)
            {
                try
                {
                    if (_deviceState.PreviewConfig.previewMode == rsid.PreviewMode.RAW10_1080P)
                        _frameDumper.DumpRawImage(image);
                    else
                        _frameDumper.DumpPreviewImage(image);
                }
                catch (Exception ex)
                {
                    HandleDumpException(ex);
                }
            }
        }

        private void MarkDumpedImagesAndReset()
        {
            if (_dumpEnabled && _frameDumper != null)
            {
                try
                {
                    _frameDumper.MarkSelectedPreviewImage(_detectedFacesTs);
                }
                catch (Exception ex)
                {
                    HandleDumpException(ex);
                }
                _detectedFacesTs.Clear();
            }
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
            Dispatcher.Invoke(() =>
            {
                ShowLogTitle(title);
                SetUIEnabled(false);
                RedDot.Visibility = Visibility.Visible;
                _cancelWasCalled = false;
                _lastAuthHint = rsid.AuthStatus.Serial_Ok;
                ResetDetectedFaces();
                try
                {
                    _frameDumper = activateDumps ? new FrameDumper(_dumpDir) : null;
                }
                catch (Exception ex)
                {
                    HandleDumpException(ex);
                }
            });
        }

        private void OnStopSession()
        {
            Dispatcher.Invoke(() =>
            {
                _frameDumper = null;
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
                string logmsg;

                if (status == rsid.EnrollStatus.Success)
                {
                    logmsg = "Enroll success";
                }
                else if (status == rsid.EnrollStatus.EnrollWithMaskIsForbidden)
                {
                    logmsg = "Enroll with mask is forbidden";
                }
                else
                {
                    logmsg = "Enroll failed";
                }

                string guimsg = logmsg;

                ShowLog(logmsg);

                VerifyResult(status == rsid.EnrollStatus.Success, guimsg, guimsg);
                MarkDumpedImagesAndReset();
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
                string logmsg;
                if (status != rsid.EnrollStatus.Success || faceprintsHandle == null)
                {
                    if (status == rsid.EnrollStatus.EnrollWithMaskIsForbidden)
                    {
                        logmsg = "Enroll with mask is forbidden.";
                    }
                    else
                    {
                        logmsg = "Enroll Failed.";
                    }

                    ShowFailedTitle(logmsg);
                    ShowLog(logmsg);
                    return;
                }

                logmsg = "Enroll Success.";
                var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                string guimsg = logmsg;

                // handle version mismatch (db version vs. faceprint version).
                if ((status == rsid.EnrollStatus.Success) && !(_db.VerifyVersionMatched(ref faceprints)))
                {
                    HandleDbErrorServer();
                    logmsg += $" Faceprints (FP) version mismatch. DB={ _db.GetVersion()}, FP={faceprints.version}. Saved the DB to backup file and started a new DB from scratch.";
                    guimsg += $" Faceprints version mismatch : DB backuped and cleaned.";
                }

                ShowLog(logmsg);

                // handle enroll 
                VerifyResult(status == rsid.EnrollStatus.Success, guimsg, guimsg, () =>
                {
                    if (_db.Push(faceprints, _lastEnrolledUserId))
                    {
                        _db.Save();
                    }
                    RefreshUserListServer();
                });

                MarkDumpedImagesAndReset();
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
                ShowFailedTitle("No face detected");
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
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No valid face detected" : status.ToString();
                VerifyResultAuth(status, $"{userId}", failMessage, null, userId);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
            MarkDumpedImagesAndReset();
        }

        private void OnFaceDeteced(IntPtr facesArr, int faceCount, uint ts, IntPtr ctx)
        {
            //convert to face rects
            ResetDetectedFaces();
            var faces = rsid.Authenticator.MarshalFaces(facesArr, faceCount);
            _detectedFacesTs.Add(ts);
            foreach (var face in faces)
            {
                ShowLog($"OnFaceDeteced [{face.x},{face.y} {face.width}x{face.height}]");
                RenderDispatch(() => _detectedFaces.Add((face, null, null)));
            }
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
                // TODO yossidan - handle with/without mask vectors properly (if/as needed).

                var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                Match(faceprints);
            }
            else
            {
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No valid face detected" : status.ToString();
                //VerifyResult(false, "", failMessage);
                VerifyResultAuth(status, string.Empty, failMessage);
                //ShowFailedTitle(failMessage);

            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
            MarkDumpedImagesAndReset();
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
                // TODO yossidan - handle with/without mask vectors properly (if/as needed).

                var faceprints = (rsid.Faceprints)Marshal.PtrToStructure(faceprintsHandle, typeof(rsid.Faceprints));
                Match(faceprints);
            }
            else
            {
                string failMessage = (status == rsid.AuthStatus.NoFaceDetected) ? "No valid face detected" : status.ToString();
                VerifyResultAuth(status, string.Empty, failMessage);
            }
            _lastAuthHint = rsid.AuthStatus.Serial_Ok; // show next hint, session is done
            MarkDumpedImagesAndReset();
        }

        private void OnDetectSpoofResult(rsid.AuthStatus status, string userId, IntPtr ctx)
        {
            ShowLog($"OnDetectSpoofResult status: {status}");
            if (_cancelWasCalled)
            {
                ShowSuccessTitle("Canceled");
            }
            //else if (status == rsid.AuthStatus.Success)
            //{
            //    ShowLog($"User is real");
            //    ShowSuccessTitle($"User is real");
            //}
            //else
            //{
            //    ShowLog(status.ToString());
            //    ShowFailedSpoofStatus(status);                
            //}

            VerifyResultAuth(status, "User is real", GetFailedSpoofMsg(status), null, "Real Face");
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
            var isEnabled = usersCount > 0;
            InstructionsEnrollUsers.Visibility = isEnabled ? Visibility.Collapsed : Visibility.Visible;
            SelectAllUsersCheckBox.IsEnabled = isEnabled;
            AuthenticateButton.IsEnabled = isEnabled;
            AuthenticateLoopToggle.IsEnabled = isEnabled;
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

        private DeviceState? QueryDeviceMetadata(rsid.SerialConfig config)
        {
            var device = new DeviceState();
            device.SerialConfig = config;

            using (var controller = new rsid.DeviceController())
            {
                ShowLog($"Connecting to {device.SerialConfig.port}...");
                var status = controller.Connect(device.SerialConfig);
                if (status != rsid.Status.Ok)
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

                ShowLog("Pinging device...");

                status = controller.Ping();
                device.IsOperational = status == rsid.Status.Ok;

                ShowLog($"{(device.IsOperational ? "Success" : "Failed")}\n");
            }

            var isCompatible = rsid.Authenticator.IsFwCompatibleWithHost(device.FirmwareVersion);
            device.IsCompatible = isCompatible;
            ShowLog($"Is compatible with host? {(device.IsCompatible ? "Yes" : "No")}\n");

            rsid.DeviceConfig? deviceConfig = null;
            if (_deviceState.IsOperational)
            {
                // device is in operational mode, we continue to query config as usual
                if (!ConnectAuth())
                {
                    ShowLog("Failed\n");
                    return null;
                }
                deviceConfig = QueryDeviceConfig();
                _authenticator.Disconnect();
                if (deviceConfig.HasValue)
                {
                    device.PreviewConfig = new rsid.PreviewConfig { cameraNumber = Settings.Default.CameraNumber, previewMode = (rsid.PreviewMode)deviceConfig.Value.previewMode };
                }
            }

            return device;
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

        private struct DeviceState
        {
            public string FirmwareVersion;
            public string RecognitionVersion;
            public string SerialNumber;
            public bool IsOperational;
            public bool IsCompatible;
            public rsid.SerialConfig SerialConfig;
            public rsid.PreviewConfig PreviewConfig;
        }

        private DeviceState DetectDevice()
        {
            rsid.SerialConfig config;

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

                ShowFailedTitle("Device Error");
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

                ShowFailedTitle("FW Incompatible");
                var msg = $"Firmware version is incompatible.\nPlease update to version { compatibleVersion } or newer.\n";
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
                if (_preview == null)
                    _preview = new rsid.Preview(_deviceState.PreviewConfig);
                else
                    _preview.UpdateConfig(_deviceState.PreviewConfig);
                _preview.Start(OnPreview);
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
                ShowFailedTitle(ex.Message);
            }
        }

        // Enroll Job
        private void EnrollJob(Object threadContext)
        {
            var userId = threadContext as string;

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll \"{userId}\"", _dumpEnabled);
            IntPtr userIdCtx = Marshal.StringToHGlobalUni(userId);
            try
            {
                ShowProgressTitle("Enroll in progress...");
                _authloopRunning = true;
                var enrollArgs = new rsid.EnrollArgs
                {
                    userId = userId,
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollResult,
                    progressClbk = OnEnrollProgress,
                    faceDetectedClbk = OnFaceDeteced,
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
                ShowFailedTitle(ex.Message);
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
                ShowFailedTitle("User ID already exists in database");
                return;
            }

            if (!ConnectAuth()) return;
            OnStartSession($"Enroll \"{userId}\"", _dumpEnabled);
            try
            {
                _lastEnrolledUserId = userId + '\0';
                var enrollExtArgs = new rsid.EnrollExtractArgs
                {
                    hintClbk = OnEnrollHint,
                    resultClbk = OnEnrollExtractionResult,
                    progressClbk = OnEnrollProgress,
                    faceDetectedClbk = OnFaceDeteced,
                };
                var operationStatus = _authenticator.EnrollExtractFaceprints(enrollExtArgs);
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
                    if (status == rsid.Status.Ok)
                    {
                        ShowLog("Detele Ok");
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
                VerifyResult(status == rsid.Status.Ok, "Delete success", "Delete failed", RefreshUserList);
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
                    if (success == true)
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
                VerifyResult(successAll == true, "Delete success", "Delete failed");
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
                VerifyResult(success == true, "Delete all success", "Delete all failed", RefreshUserListServer);
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
            OnStartSession("Authenticate", _dumpEnabled);
            try
            {
                var authArgs = new rsid.AuthArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };

                ShowProgressTitle("Authenticating..");
                _authloopRunning = true;
                _authenticator.Authenticate(authArgs);
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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
            OnStartSession("Authenticate Loop", _dumpEnabled);
            try
            {
                var authArgs = new rsid.AuthArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };

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
                ShowFailedTitle(ex.Message);
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
            OnStartSession("Extracting Faceprints", _dumpEnabled);
            try
            {
                var authExtArgs = new rsid.AuthExtractArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthExtractionResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };
                ShowProgressTitle("Extracting Faceprints");
                _authenticator.AuthenticateExtractFaceprints(authExtArgs);
            }
            catch (Exception ex)
            {
                ShowFailedTitle(ex.Message);
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

            OnStartSession("Authentication faceprints extraction loop", _dumpEnabled);
            try
            {
                var authLoopExtArgs = new rsid.AuthExtractArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnAuthLoopExtractionResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };
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
                ShowFailedTitle(ex.Message);
            }
            finally
            {
                OnStopSession();
                HideAuthenticatingLabelPanel();
                _authloopRunning = false;
                _authenticator.Disconnect();
            }
        }

        // Detect spoof job
        private void DetectSpoofJob(Object threadContext)
        {
            if (!ConnectAuth()) return;
            OnStartSession("Detect spoof", _dumpEnabled);
            try
            {
                var authArgs = new rsid.AuthArgs
                {
                    hintClbk = OnAuthHint,
                    resultClbk = OnDetectSpoofResult,
                    faceDetectedClbk = OnFaceDeteced,
                    ctx = IntPtr.Zero
                };

                ShowProgressTitle("Detecting spoof..");
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

        // SetDeviceConfig job
        private void SetDeviceConfigJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            (rsid.DeviceConfig? prevDeviceConfig, var deviceConfig, var flowMode) = ((rsid.DeviceConfig?, rsid.DeviceConfig, FlowMode))threadContext;
            OnStartSession("SetDeviceConfig", false);
            try
            {
                ShowProgressTitle("SetDeviceConfig");
                LogDeviceConfig(deviceConfig);

                rsid.Status status = rsid.Status.Ok;
                if (prevDeviceConfig.HasValue)
                {
                    rsid.DeviceConfig prevDeviceConfigValue = prevDeviceConfig.Value;
                    if (_preview != null)
                        _preview.Stop();
                    if (prevDeviceConfigValue.previewMode != deviceConfig.previewMode)
                    {
                        // restart preview
                        _deviceState.PreviewConfig = new rsid.PreviewConfig { cameraNumber = Settings.Default.CameraNumber, previewMode = (rsid.PreviewMode)deviceConfig.previewMode };
                        _preview.UpdateConfig(_deviceState.PreviewConfig);
                    }
                    ShowLog("Detected changes. Updating settings on device...");
                    status = _authenticator.SetDeviceConfig(deviceConfig);
                    if (_preview != null)
                        _preview.Start(OnPreview);
                    if (deviceConfig.previewMode != rsid.DeviceConfig.PreviewMode.RAW10_1080P)
                        InvokePreviewVisibility(Visibility.Visible);
                    else
                        InvokePreviewVisibility(Visibility.Hidden);
                }

                if (flowMode != _flowMode)
                {
                    _flowMode = flowMode;

                    if (flowMode == FlowMode.Server)
                    {
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
                            //MessageBox.Show(")", "", MessageBoxButton.OK);
                            ShowErrorMessage("Default DB", "No db file selected.\nUsing the default path (<current dir>/db.db)");
                            _db = new Database();
                        }


                        Dispatcher.Invoke(() =>
                        {
                            StandbyButton.IsEnabled = false;
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

                VerifyResult(status == rsid.Status.Ok, "Apply settings done", "Apply settings failed");
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

        private void StandbyJob(Object threadContext)
        {
            if (!ConnectAuth()) return;

            OnStartSession("Standby", false);
            try
            {
                ShowProgressTitle("Storing users data");
                var status = _authenticator.Standby();
                VerifyResult(status == rsid.Status.Ok, "Storing users data done", "Storing users data failed");
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
                var versions = _fwUpdater.ExtractFwVersion(binPath);
                var newFwVersion = versions?.OpfwVersion;
                var newRecognitionVersion = versions?.RecognitionVersion;

                if (newFwVersion == null || newRecognitionVersion == null)
                {
                    CloseProgressBar();
                    ShowErrorMessage("FW Update Error", "Unable to parse the selected firmware file.");
                    return;
                }

                // by default, we don't exclude recognition
                bool excludeRecognition = false;

                if (newRecognitionVersion != _deviceState.RecognitionVersion)
                {
                    var msg = "Would you like to preserve your faceprints database?\n";
                    msg += "\nSelect 'YES' to perform a partial update, without the latest recognition module.\n";
                    msg += "\nSelect 'NO' to perform a full update, which will also remove your database.\n";

                    bool? proceed = null;
                    Dispatcher.Invoke(() =>
                    {
                        var dialog = new FwUpdateInput("Database Incompatibility", msg);
                        proceed = ShowWindowDialog(dialog);
                        excludeRecognition = dialog.ExcludeRecognition();
                    });

                    // user cancelled the update
                    if (!proceed.HasValue || proceed.Value == false)
                    {
                        CloseProgressBar();
                        return;
                    }

                    ShowLog($"Preserve database? {excludeRecognition}");

                    if (!excludeRecognition)
                    {
                        try
                        {
                            ConnectAuth();
                            _authenticator?.RemoveAllUsers();

                            _db.RemoveAll();
                            _db.Save();
                        }
                        finally
                        {
                            _authenticator?.Disconnect();
                        }
                    }
                }

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
                TogglePreviewOpacity(false);
                _deviceState.IsOperational = false;
                Thread.Sleep(100);

                OnStartSession("Firmware Update", false);
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

                    var status = _fwUpdater.Update(binPath, eventHandler, fwUpdateSettings, excludeRecognition);
                    success = status == rsid.Status.Ok;
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
        private const string dllName = "rsid_secure_helper_debug";
#else
        private const string dllName = "rsid_secure_helper";
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