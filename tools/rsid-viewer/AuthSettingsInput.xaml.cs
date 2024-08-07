// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Microsoft.Win32;
using rsid;
using System;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Input;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Runtime.Remoting.Lifetime;
using System.Threading;
using static rsid_wrapper_csharp.MainWindow;
using System.Diagnostics;
using System.Windows.Controls;
using System.Text.RegularExpressions;

namespace rsid_wrapper_csharp
{
    /// <summary>
    /// Interaction logic for EnrollInput.xaml
    /// </summary>
    public partial class AuthSettingsInput : Window
    {
        private MainWindow MyMainWindow { get => (MainWindow)this.Owner; }
        private SerialConfig serialConfig;
        private CancellationTokenSource cts = new CancellationTokenSource();
        public DeviceConfig Config { get; private set; }
        public MainWindow.FlowMode FlowMode { get; private set; }
        public PreviewConfig PreviewConfig { get; private set; }
        public string FirmwareFileName { get; private set; } = string.Empty;
        public bool ForceFirmwareUpdate { get; private set; } = false;

        public AuthSettingsInput(string fwVersion, DeviceConfig? config,
            PreviewConfig? previewConfig,
            MainWindow.FlowMode flowMode,
            bool previewEnabled,
            SerialConfig serialConfig)
        {
            this.Owner = Application.Current.MainWindow;
            this.serialConfig = serialConfig;

            InitializeComponent();

            // Init dialog values according to current config
            FirmwareVersionNumber.Text = fwVersion;
            var hasConfig = config.HasValue;
            if (hasConfig)
            {
                Config = config.Value;
                FlowMode = flowMode;
                PreviewConfig = previewConfig.Value;
                UpdateUiSettingsValues(config.Value, previewConfig.Value, flowMode);
            }

            FaceSelectionPolicySingle.IsEnabled = hasConfig;
            FaceSelectionPolicyAll.IsEnabled = hasConfig;

            AlgoFlow_All.IsEnabled = hasConfig;
            AlgoFlow_DetectionOnly.IsEnabled = hasConfig;
            AlgoFlow_RecognitionOnly.IsEnabled = hasConfig;
            AlgoFlow_SpoofOnly.IsEnabled = hasConfig;

            CameraRotation0.IsEnabled = hasConfig;
            CameraRotation180.IsEnabled = hasConfig;
            CameraRotation90.IsEnabled = hasConfig;
            CameraRotation270.IsEnabled = hasConfig;

            ServerModeYes.IsEnabled = hasConfig;
            ServerModeNo.IsEnabled = hasConfig;

            ConfidenceHigh.IsEnabled = hasConfig;
            ConfidenceMedium.IsEnabled = hasConfig;
            ConfidenceLow.IsEnabled = hasConfig;

            bool previewEnabledAuth = previewEnabled && hasConfig;

            PreviewModeMJPEG_1080P.IsEnabled = previewEnabledAuth;
            PreviewModeMJPEG_720P.IsEnabled = previewEnabledAuth;

            DumpModeNone.IsEnabled = previewEnabledAuth;
            DumpModeFace.IsEnabled = previewEnabledAuth;
            DumpModeFull.IsEnabled = previewEnabledAuth;
        }


        private void UpdateUiSettingsValues(DeviceConfig deviceConfig, PreviewConfig previewConfig, MainWindow.FlowMode flowMode)
        {
            FaceSelectionPolicySingle.IsChecked = deviceConfig.faceSelectionPolicy == DeviceConfig.FaceSelectionPolicy.Single;
            FaceSelectionPolicyAll.IsChecked = deviceConfig.faceSelectionPolicy == DeviceConfig.FaceSelectionPolicy.All;

            AlgoFlow_All.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.All;
            AlgoFlow_DetectionOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.FaceDetectionOnly;
            AlgoFlow_RecognitionOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.RecognitionOnly;
            AlgoFlow_SpoofOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.SpoofOnly;

            ServerModeNo.IsChecked = flowMode == MainWindow.FlowMode.Device;
            ServerModeYes.IsChecked = flowMode == MainWindow.FlowMode.Server;

            ConfidenceHigh.IsChecked = deviceConfig.matcherConfidenceLevel == MatcherConfidenceLevel.High;
            ConfidenceMedium.IsChecked = deviceConfig.matcherConfidenceLevel == MatcherConfidenceLevel.Medium;
            ConfidenceLow.IsChecked = deviceConfig.matcherConfidenceLevel == MatcherConfidenceLevel.Low;

            CameraRotation0.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg;
            CameraRotation180.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;
            CameraRotation90.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_90_Deg;
            CameraRotation270.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_270_Deg;

            PreviewModeMJPEG_1080P.IsChecked = previewConfig.previewMode == PreviewMode.MJPEG_1080P;
            PreviewModeMJPEG_720P.IsChecked = previewConfig.previewMode == PreviewMode.MJPEG_720P;

            DumpModeNone.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.None;
            DumpModeFace.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.CroppedFace;
            DumpModeFull.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.FullFrame;

            MaxSpoofs.Text = deviceConfig.maxSpoofs.ToString();
        }

        void QueryUiSettingsValues(out DeviceConfig deviceConfig, out PreviewConfig previewConfig, out MainWindow.FlowMode flowMode)
        {
            deviceConfig = new DeviceConfig();
            previewConfig = new PreviewConfig();


            // policy
            if (FaceSelectionPolicyAll.IsChecked.GetValueOrDefault())
                deviceConfig.faceSelectionPolicy = DeviceConfig.FaceSelectionPolicy.All;
            else
                deviceConfig.faceSelectionPolicy = DeviceConfig.FaceSelectionPolicy.Single;

            // algo flow
            if (AlgoFlow_All.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = DeviceConfig.AlgoFlow.All;
            else if (AlgoFlow_DetectionOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = DeviceConfig.AlgoFlow.FaceDetectionOnly;
            else if (AlgoFlow_RecognitionOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = DeviceConfig.AlgoFlow.RecognitionOnly;
            else if (AlgoFlow_SpoofOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = DeviceConfig.AlgoFlow.SpoofOnly;

            // camera rotation
            if (CameraRotation0.IsChecked.GetValueOrDefault())
                deviceConfig.cameraRotation = DeviceConfig.CameraRotation.Rotation_0_Deg;
            else if (CameraRotation90.IsChecked.GetValueOrDefault())
                deviceConfig.cameraRotation = DeviceConfig.CameraRotation.Rotation_90_Deg;
            else if (CameraRotation270.IsChecked.GetValueOrDefault())
                deviceConfig.cameraRotation = DeviceConfig.CameraRotation.Rotation_270_Deg;
            else if (CameraRotation180.IsChecked.GetValueOrDefault())
                deviceConfig.cameraRotation = DeviceConfig.CameraRotation.Rotation_180_Deg;

            // flow mode
            flowMode = ServerModeNo.IsChecked.GetValueOrDefault() ? MainWindow.FlowMode.Device : MainWindow.FlowMode.Server;

            // matcher confidence level
            if (ConfidenceHigh.IsChecked.GetValueOrDefault())
            {
                deviceConfig.matcherConfidenceLevel = MatcherConfidenceLevel.High;
                deviceConfig.securityLevel = DeviceConfig.SecurityLevel.High;
            }
            else if (ConfidenceMedium.IsChecked.GetValueOrDefault())
            {
                deviceConfig.matcherConfidenceLevel = MatcherConfidenceLevel.Medium;
                deviceConfig.securityLevel = DeviceConfig.SecurityLevel.Medium;
            }
            else if (ConfidenceLow.IsChecked.GetValueOrDefault())
            {
                deviceConfig.matcherConfidenceLevel = MatcherConfidenceLevel.Low;
                deviceConfig.securityLevel = DeviceConfig.SecurityLevel.Low;
            }

            previewConfig.portraitMode = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg || deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;

            if (PreviewModeMJPEG_1080P.IsChecked.GetValueOrDefault())
                previewConfig.previewMode = PreviewMode.MJPEG_1080P;
            else if (PreviewModeMJPEG_720P.IsChecked.GetValueOrDefault())
                previewConfig.previewMode = PreviewMode.MJPEG_720P;
            else // default mode
                previewConfig.previewMode = PreviewMode.RAW10_1080P;

            // dump mode
            if (DumpModeNone.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.None;
            else if (DumpModeFace.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.CroppedFace;
            else if (DumpModeFull.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.FullFrame;
            else // default is no dump
                deviceConfig.dumpMode = DeviceConfig.DumpMode.None;

            // max spoofs
            if (Byte.TryParse(MaxSpoofs.Text.Trim(), out byte maxSpoofs))
            {
                deviceConfig.maxSpoofs = maxSpoofs;
            }
            else
            {
                throw new Exception("Max Spoofs is invalid. Must be in range of 0-255.");
            }

        }

        private string GetFirmwareDirectory()
        {
            var executablePath = Path.GetDirectoryName(Assembly.GetEntryAssembly()?.Location);
            var firmwarePath = Path.Combine(Directory.GetParent(executablePath)?.FullName, "firmware");
            return Directory.Exists(firmwarePath) ? firmwarePath : executablePath;
        }

        private void UpdateFirmwareButton_Click(object sender, RoutedEventArgs e)
        {
            var openFileDialog = new OpenFileDialog
            {
                CheckFileExists = true,
                Multiselect = false,
                InitialDirectory = GetFirmwareDirectory(),
                Title = "Select Firmware Image",
                Filter = "bin files (*.bin)|*.bin|All files (*.*)|*.*",
                FilterIndex = 1
            };
            if (openFileDialog.ShowDialog() == false)
                return;

            FirmwareFileName = openFileDialog.FileName;
            ForceFirmwareUpdate = ForceUpdateChk.IsChecked.Value;
            DialogResult = true;
        }

        private void SettingsApplyButton_Click(object sender, RoutedEventArgs e)
        {
            QueryUiSettingsValues(out DeviceConfig config, out PreviewConfig previewConfig, out MainWindow.FlowMode flowMode);
            Config = config;
            FlowMode = flowMode;
            PreviewConfig = previewConfig;
                        
            if (previewConfig.portraitMode == false && config.securityLevel == DeviceConfig.SecurityLevel.High)
            {
                var errDialog = new ErrorDialog("High Security not enabled with non-portrait Modes", "Change security level or rotation mode");
                errDialog.ShowDialog();
                DialogResult = null;
                return;
            }

            if (config.dumpMode == DeviceConfig.DumpMode.CroppedFace &&
                config.faceSelectionPolicy == DeviceConfig.FaceSelectionPolicy.All)
            {
                var errDialog = new ErrorDialog("Config Not Supported",
                    "Face Selection->All does not support cropped dump mode.");
                errDialog.ShowDialog();
                DialogResult = null;
                return;
            }

            DialogResult = true;
        }

        private void SettingsCancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.cts.Cancel();
            DialogResult = false;
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
                this.DragMove();
        }

        private void OnUpdateCheckStart()
        {
            UpdateFirmwareButton.IsEnabled = false;
            SettingsApplyButton.IsEnabled = false;
            CheckForUpdatesLink.Visibility = Visibility.Collapsed;
            ActivateLicenseLink.Visibility = Visibility.Collapsed;
            CheckUpdatesBar.Visibility = Visibility.Visible;
        }

        private void OnUpdateCheckEnd()
        {
            UpdateFirmwareButton.IsEnabled = true;
            SettingsApplyButton.IsEnabled = true;
            CheckForUpdatesLink.Visibility = Visibility.Visible;
            ActivateLicenseLink.Visibility = Visibility.Visible;
            CheckUpdatesBar.Visibility = Visibility.Collapsed;
        }

        private async void CheckForUpdates_Click(object sender, RoutedEventArgs e)
        {
            var operationTimeout = TimeSpan.FromSeconds(8);
            rsid.UpdateChecker.ReleaseInfo remoteInfo = null;
            rsid.UpdateChecker.ReleaseInfo localInfo = null;
            OnUpdateCheckStart();

            var cancellationToken = cts.Token;
            var timeout = Task.Delay(operationTimeout, cancellationToken);
            var task1 = Task.Run(() =>
            {
                var status = rsid.UpdateChecker.GetRemoteReleaseInfo(out remoteInfo);
                cancellationToken.ThrowIfCancellationRequested();
                return status;
            }, cancellationToken);
            var task2 = Task.Run(() =>
            {
                var status = rsid.UpdateChecker.GetLocalReleaseInfo(serialConfig, out localInfo);
                cancellationToken.ThrowIfCancellationRequested();
                return status;
            }, cancellationToken);


            // Wait for either all tasks to complete or the timeout            
            try
            {
                var taskResult = await Task.WhenAny(Task.WhenAll(task1, task2, Task.Delay(1000)), timeout);
                OnUpdateCheckEnd();

                if (taskResult == timeout)
                {
                    if (!taskResult.IsCanceled)
                        ErrorDialog.Show("Error retreiving Info", "Operation timed out.");
                    return;
                }

                var status1 = task1.Result;
                if (status1 != Status.Ok)
                {

                    this.MyMainWindow.ShowLog("Error retrieving remote release info. Status: " + status1.ToString());
                    ErrorDialog.Show(status1.ToString(), "Error retrieving remote release info");
                    return;
                }

                MyMainWindow.ShowLog("Remote release info:");
                MyMainWindow.ShowLog($" * host={remoteInfo.sw_version_str}  fw={remoteInfo.fw_version_str}");
                MyMainWindow.ShowLog($" * fw={remoteInfo.fw_version_str}");


                var status2 = task2.Result;
                if (status2 != Status.Ok)
                {
                    this.MyMainWindow.ShowLog("Error retrieving remote local info. Status: " + status2.ToString());
                    ErrorDialog.Show(status2.ToString(), "Error retrieving local release info");
                    return;
                }

                MyMainWindow.ShowLog("Local release info:");
                MyMainWindow.ShowLog($" * host={localInfo.sw_version_str}  fw={localInfo.fw_version_str}");
                MyMainWindow.ShowLog($" * fw={localInfo.fw_version_str}");


                new UpdateAvailableDialog(localInfo, remoteInfo).ShowDialog();


            }
            catch (OperationCanceledException)

            {
                // Handle cancellation
                MyMainWindow.ShowLog("Check for updates was canceled.");
            }

            catch (Exception ex)
            {

                MyMainWindow.ShowLog("Error retrieving release info. Exception: " + ex.Message);
                ErrorDialog.Show("Error retrieving release info", ex.Message);
            }
            finally
            {
                OnUpdateCheckEnd();
            }
        }

        private async void ActivateLicense_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var auth = MyMainWindow.GetAuthenticator();
                if (auth == null)
                {
                    ErrorDialog.Show("Cannot activate", "Device not connected");
                    return;
                }
                var licenseKey = Authenticator.GetLicenseKey();
                var licenseDialog = new OKCancelDialog("Enter License Key", "");
                licenseDialog.SetInputText(licenseKey);
                if (!licenseDialog.ShowDialog().GetValueOrDefault()) return;
                licenseKey = licenseDialog.DialogInput.Text;
                if (licenseKey?.Length != Authenticator.LicenseKeySize)
                {
                    ErrorDialog.Show("Invalid License Key", "License key must be 36 chars long");
                    return;
                }

                var setkey_status = Authenticator.SetLicenseKey(licenseKey);
                if (setkey_status != Status.Ok)
                {
                    ErrorDialog.Show("Error setting license key", setkey_status.ToString());
                    return;
                }

                OnUpdateCheckStart();
                MyMainWindow.ShowLog($"Activating {licenseKey}");
                var cancellationToken = cts.Token;
                await Task.Run(() =>
                {
                    var status = auth.Connect(serialConfig);
                    if (status != Status.Ok)
                    {
                        throw new Exception($"Error connecting to device on serial port {serialConfig.port} (status={status}).");
                    }
                    cancellationToken.ThrowIfCancellationRequested();
                    status = auth.ProvideLicense();
                    cancellationToken.ThrowIfCancellationRequested();
                    if (status != Status.Ok)
                    {
                        throw new Exception($"Please check your internet connection and license key (status={status}).");
                    }
                }, cancellationToken);
                OnUpdateCheckEnd();
                MyMainWindow.ShowLog("Activation Successful");
                ErrorDialog.Show("Activation Successful", "License activated successfully.");
                Close();
            }
            catch (Exception ex)
            {
                OnUpdateCheckEnd();
                MyMainWindow.ShowLog(ex.Message);
                ErrorDialog.Show("License activation failed", ex.Message);
            }
            finally { OnUpdateCheckEnd(); }
        }


        private void ValidateMaxSpoofs(object sender, TextCompositionEventArgs e)
        {
            bool ignore = MaxSpoofs.Text.Length > 3 || Regex.IsMatch(e.Text, "[^0-9]+");
            e.Handled = ignore;
        }
    }

}

