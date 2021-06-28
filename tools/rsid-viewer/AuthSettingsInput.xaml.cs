// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Microsoft.Win32;
using rsid;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Input;


namespace rsid_wrapper_csharp
{
    /// <summary>
    /// Interaction logic for EnrollInput.xaml
    /// </summary>
    public partial class AuthSettingsInput : Window
    {
        private string Sku { get; set; }
        public DeviceConfig Config { get; private set; }
        public MainWindow.FlowMode FlowMode { get; private set; }
        public PreviewConfig PreviewConfig { get; private set; }
        public string FirmwareFileName { get; private set; } = string.Empty;

        public AuthSettingsInput(string fwVersion, string sku, DeviceConfig? config,PreviewConfig? previewConfig, MainWindow.FlowMode flowMode, bool previewEnabled)
        {
            this.Owner = Application.Current.MainWindow;

            InitializeComponent();

            // Init dialog values according to current config
            FirmwareVersionNumber.Text = fwVersion;
            Sku = sku;
            var hasConfig = config.HasValue;
            if (hasConfig)
            {
                Config = config.Value;
                FlowMode = flowMode;
                PreviewConfig = previewConfig.Value;
                UpdateUiSettingsValues(config.Value, previewConfig.Value, flowMode);
            }

            // enable/disable controls 
            SecurityLevelHigh.IsEnabled = hasConfig;
            SecurityLevelMedium.IsEnabled = hasConfig;

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

            bool previewEnabledAuth = previewEnabled && hasConfig;
            
            PreviewModeMJPEG_1080P.IsEnabled = previewEnabledAuth;
            PreviewModeMJPEG_720P.IsEnabled = previewEnabledAuth;
            PreviewModeRAW10_1080P.IsEnabled = previewEnabledAuth;

            DumpModeNone.IsEnabled = previewEnabledAuth;
            DumpModeCropped.IsEnabled = previewEnabledAuth;
            DumpModeFull.IsEnabled = previewEnabledAuth;
        }


        private void UpdateUiSettingsValues(DeviceConfig deviceConfig, PreviewConfig previewConfig, MainWindow.FlowMode flowMode)
        {
            SecurityLevelHigh.IsChecked = deviceConfig.securityLevel == DeviceConfig.SecurityLevel.High;
            SecurityLevelMedium.IsChecked = deviceConfig.securityLevel == DeviceConfig.SecurityLevel.Medium;

            FaceSelectionPolicySingle.IsChecked = deviceConfig.faceSelectionPolicy == DeviceConfig.FaceSelectionPolicy.Single;
            FaceSelectionPolicyAll.IsChecked = deviceConfig.faceSelectionPolicy == DeviceConfig.FaceSelectionPolicy.All;

            AlgoFlow_All.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.All;
            AlgoFlow_DetectionOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.FaceDetectionOnly;
            AlgoFlow_RecognitionOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.RecognitionOnly;
            AlgoFlow_SpoofOnly.IsChecked = deviceConfig.algoFlow == DeviceConfig.AlgoFlow.SpoofOnly;

            ServerModeNo.IsChecked = flowMode == MainWindow.FlowMode.Device;
            ServerModeYes.IsChecked = flowMode == MainWindow.FlowMode.Server;

            CameraRotation0.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg;
            CameraRotation180.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;
            CameraRotation90.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_90_Deg;
            CameraRotation270.IsChecked = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_270_Deg;

            PreviewModeMJPEG_1080P.IsChecked = previewConfig.previewMode == PreviewMode.MJPEG_1080P;
            PreviewModeMJPEG_720P.IsChecked = previewConfig.previewMode == PreviewMode.MJPEG_720P;
            PreviewModeRAW10_1080P.IsChecked = previewConfig.previewMode == PreviewMode.RAW10_1080P;

            DumpModeNone.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.None;
            DumpModeCropped.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.CroppedFace;
            DumpModeFull.IsChecked = deviceConfig.dumpMode == DeviceConfig.DumpMode.FullFrame;
        }

        void QueryUiSettingsValues(out DeviceConfig deviceConfig,out PreviewConfig previewConfig, out MainWindow.FlowMode flowMode)
        {
            deviceConfig = new DeviceConfig();
            previewConfig = new PreviewConfig();

            // securiy level
            deviceConfig.securityLevel = DeviceConfig.SecurityLevel.Medium;
            if (SecurityLevelHigh.IsChecked.GetValueOrDefault())
                deviceConfig.securityLevel = DeviceConfig.SecurityLevel.High;

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

            previewConfig.portraitMode = deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_0_Deg || deviceConfig.cameraRotation == DeviceConfig.CameraRotation.Rotation_180_Deg;

            if (PreviewModeMJPEG_1080P.IsChecked.GetValueOrDefault())
                previewConfig.previewMode = PreviewMode.MJPEG_1080P;
            else if (PreviewModeMJPEG_720P.IsChecked.GetValueOrDefault())
                previewConfig.previewMode = PreviewMode.MJPEG_720P;
            else if (PreviewModeRAW10_1080P.IsChecked.GetValueOrDefault()) 
                previewConfig.previewMode = PreviewMode.RAW10_1080P;

            // dump mode
            if (DumpModeNone.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.None;
            else if (DumpModeCropped.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.CroppedFace;
            else if (DumpModeFull.IsChecked.GetValueOrDefault())
                deviceConfig.dumpMode = DeviceConfig.DumpMode.FullFrame;
        }

        private string GetFirmwareDirectory()
        {
            var executablePath = Path.GetDirectoryName(Assembly.GetEntryAssembly()?.Location);
            var firmwarePath = Path.Combine(Directory.GetParent(executablePath)?.FullName, "firmware", Sku);
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
            DialogResult = true;
        }

        private void SettingsApplyButton_Click(object sender, RoutedEventArgs e)
        {
            QueryUiSettingsValues(out DeviceConfig config,out PreviewConfig previewConfig, out MainWindow.FlowMode flowMode);
            Config = config;
            FlowMode = flowMode;
            PreviewConfig = previewConfig;

            // verify config options
            if (config.dumpMode == DeviceConfig.DumpMode.CroppedFace &&
                previewConfig.previewMode == PreviewMode.RAW10_1080P)
            {
                var errDialog = new ErrorDialog("Config Not Supported",
                    "RAW10 format does not support cropped dump mode.");
                errDialog.ShowDialog();
                DialogResult = null;
                return;
            }

            if (config.cameraRotation != DeviceConfig.CameraRotation.Rotation_0_Deg &&
                    previewConfig.previewMode == PreviewMode.RAW10_1080P)
            {
                var errDialog = new ErrorDialog("Rotated Raw10 Preview not supported",
                    "Algo will be working with desired camera rotation but preview will not be rotated.");
                errDialog.ShowDialog();
            }

            if (previewConfig.portraitMode==false && config.securityLevel == DeviceConfig.SecurityLevel.High)
            {
                var errDialog = new ErrorDialog("High Security not enabled with non-portrait Modes","Change security level or rotation mode");
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
            DialogResult = false;
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
                this.DragMove();
        }
    }
}
