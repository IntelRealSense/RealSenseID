// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Microsoft.Win32;
using rsid;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;


namespace rsid_wrapper_csharp
{
    /// <summary>
    /// Interaction logic for EnrollInput.xaml
    /// </summary>
    public partial class AuthSettingsInput : Window
    {
        public DeviceConfig Config { get; private set; }
        public MainWindow.FlowMode FlowMode { get; private set; }
        public string FirmwareFileName { get; private set; } = string.Empty;
        public bool DumpingEnabled { get; private set; }

        public AuthSettingsInput(string fwVersion, rsid.DeviceConfig? config, MainWindow.FlowMode flowMode, bool dumpingEnabled,bool previewEnabled)
        {
            this.Owner = Application.Current.MainWindow;

            InitializeComponent();

            // Init dialog values according to current config
            FirmwareVersionNumber.Text = fwVersion;
            bool hasAuth = config.HasValue;
            if (hasAuth == true)
            {
                Config = config.Value;
                FlowMode = flowMode;
                UpdateUISettingsValues(config.Value, flowMode, dumpingEnabled);
            }

            // enable/disable controls 
            SecurityLevelHigh.IsEnabled = hasAuth;
            SecurityLevelMedium.IsEnabled = hasAuth;

            FaceSelectionPolicySingle.IsEnabled = hasAuth;
            FaceSelectionPolicyAll.IsEnabled = hasAuth;

            AlgoFlow_All.IsEnabled = hasAuth;
            AlgoFlow_DetectionOnly.IsEnabled = hasAuth;
            AlgoFlow_RecognitionOnly.IsEnabled = hasAuth;
            AlgoFlow_SpoofOnly.IsEnabled = hasAuth;

            CameraRotation0.IsEnabled = hasAuth;
            CameraRotation180.IsEnabled = hasAuth;

            ServerModeYes.IsEnabled = hasAuth;
            ServerModeNo.IsEnabled = hasAuth;

            bool preview_enabled_auth = previewEnabled && hasAuth;
            PreviewModeMJPEG_1080P.IsEnabled = preview_enabled_auth;
            PreviewModeMJPEG_720P.IsEnabled = preview_enabled_auth;
            PreviewModeRAW10_1080P.IsEnabled =  preview_enabled_auth;
            DumpingCheckBoxYes.IsEnabled = preview_enabled_auth;
            DumpingCheckBoxNo.IsEnabled = preview_enabled_auth;
        }

        private void UpdateUISettingsValues(rsid.DeviceConfig deviceConfig, MainWindow.FlowMode flowMode, bool dumpingEnabled)
        {
            SecurityLevelHigh.IsChecked = deviceConfig.securityLevel == rsid.DeviceConfig.SecurityLevel.High;
            SecurityLevelMedium.IsChecked = deviceConfig.securityLevel == rsid.DeviceConfig.SecurityLevel.Medium;
            //SecurityLevelRecognitionOnly.IsChecked = false;

            FaceSelectionPolicySingle.IsChecked = deviceConfig.faceSelectionPolicy == rsid.DeviceConfig.FaceSelectionPolicy.Single;
            FaceSelectionPolicyAll.IsChecked = deviceConfig.faceSelectionPolicy == rsid.DeviceConfig.FaceSelectionPolicy.All;

            AlgoFlow_All.IsChecked = deviceConfig.algoFlow == rsid.DeviceConfig.AlgoFlow.All;
            AlgoFlow_DetectionOnly.IsChecked = deviceConfig.algoFlow == rsid.DeviceConfig.AlgoFlow.FaceDetectionOnly;
            AlgoFlow_RecognitionOnly.IsChecked = deviceConfig.algoFlow == rsid.DeviceConfig.AlgoFlow.RecognitionOnly;
            AlgoFlow_SpoofOnly.IsChecked = deviceConfig.algoFlow == rsid.DeviceConfig.AlgoFlow.SpoofOnly;


            CameraRotation0.IsChecked = deviceConfig.cameraRotation == rsid.DeviceConfig.CameraRotation.Rotation_0_Deg;
            CameraRotation180.IsChecked = deviceConfig.cameraRotation == rsid.DeviceConfig.CameraRotation.Rotation_180_Deg;

            ServerModeNo.IsChecked = flowMode == MainWindow.FlowMode.Device;
            ServerModeYes.IsChecked = flowMode == MainWindow.FlowMode.Server;

            PreviewModeMJPEG_1080P.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.MJPEG_1080P;
            PreviewModeMJPEG_720P.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.MJPEG_720P;
            PreviewModeRAW10_1080P.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.RAW10_1080P;

            DumpingCheckBoxYes.IsChecked = dumpingEnabled;
        }

        void QueryUISettingsValues(out rsid.DeviceConfig deviceConfig, out MainWindow.FlowMode flowMode, out bool dumpingEnabled)
        {
            deviceConfig = new rsid.DeviceConfig();
            // securiy level
            deviceConfig.securityLevel = rsid.DeviceConfig.SecurityLevel.Medium;
            if (SecurityLevelHigh.IsChecked.GetValueOrDefault())
                deviceConfig.securityLevel = rsid.DeviceConfig.SecurityLevel.High;

            // policy
            if (FaceSelectionPolicyAll.IsChecked.GetValueOrDefault())
                deviceConfig.faceSelectionPolicy = rsid.DeviceConfig.FaceSelectionPolicy.All;
            else
                deviceConfig.faceSelectionPolicy = rsid.DeviceConfig.FaceSelectionPolicy.Single;

            // algo flow
            if (AlgoFlow_All.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = rsid.DeviceConfig.AlgoFlow.All;
            else if (AlgoFlow_DetectionOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = rsid.DeviceConfig.AlgoFlow.FaceDetectionOnly;
            else if (AlgoFlow_RecognitionOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = rsid.DeviceConfig.AlgoFlow.RecognitionOnly;
            else if (AlgoFlow_SpoofOnly.IsChecked.GetValueOrDefault())
                deviceConfig.algoFlow = rsid.DeviceConfig.AlgoFlow.SpoofOnly;

            // camera rotation
            deviceConfig.cameraRotation = CameraRotation0.IsChecked.GetValueOrDefault() ? rsid.DeviceConfig.CameraRotation.Rotation_0_Deg : rsid.DeviceConfig.CameraRotation.Rotation_180_Deg;

            // flow mode
            flowMode = ServerModeNo.IsChecked.GetValueOrDefault() ? MainWindow.FlowMode.Device : MainWindow.FlowMode.Server;

            // preview mode
            if (PreviewModeMJPEG_1080P.IsChecked.GetValueOrDefault())
                deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.MJPEG_1080P;
            else if (PreviewModeMJPEG_720P.IsChecked.GetValueOrDefault())
                deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.MJPEG_720P;
            else if (PreviewModeRAW10_1080P.IsChecked.GetValueOrDefault())
                deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.RAW10_1080P;

            // dump
            dumpingEnabled = DumpingCheckBoxYes.IsChecked.GetValueOrDefault();
        }

        string GetFirmwareDirectory()
        {
            string executablePath = System.IO.Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
            string firmwarePath = System.IO.Path.Combine(Directory.GetParent(executablePath).FullName, "firmware");
            if (Directory.Exists(firmwarePath) == true)
            {
                return firmwarePath;
            }
            else
            {
                return executablePath;
            }
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
            DialogResult = true;
        }

        private void SettingsApplyButton_Click(object sender, RoutedEventArgs e)
        {
            QueryUISettingsValues(out rsid.DeviceConfig config, out MainWindow.FlowMode flowMode, out bool dumpingEnabled);
            Config = config;
            FlowMode = flowMode;
            DumpingEnabled = dumpingEnabled;
            DialogResult = true;
        }

        private void SettingsCancelButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
                this.DragMove();
        }
    }
}
