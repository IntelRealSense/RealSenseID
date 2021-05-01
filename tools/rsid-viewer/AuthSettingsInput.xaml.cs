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

        public AuthSettingsInput(string fwVersion, rsid.DeviceConfig? config, MainWindow.FlowMode flowMode, bool advancedMode)
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
                UpdateUISettingsValues(config.Value, flowMode);
            }

            // enable/disable controls 
            SecurityLevelHigh.IsEnabled = hasAuth;
            SecurityLevelMedium.IsEnabled = hasAuth;
            SecurityLevelRecognitionOnly.IsEnabled = hasAuth;
            CameraRotation0.IsEnabled = hasAuth;
            CameraRotation180.IsEnabled = hasAuth;
            ServerModeYes.IsEnabled = hasAuth;
            ServerModeNo.IsEnabled = hasAuth;
            PreviewModeVGA.IsEnabled = hasAuth;
            PreviewModeDump.IsEnabled = hasAuth;

            if (advancedMode)
            {
                // show adv menu settings in adv mode
                PreviewModeVGA.Visibility = hasAuth ? Visibility.Visible : Visibility.Collapsed;
                //PreviewModeFHDRect.IsEnabled = hasAuth;
                PreviewModeDump.Visibility = hasAuth ? Visibility.Visible : Visibility.Collapsed; ;
            }
            else
            {
                // hide adv menu settings in non adv mode
                previewModeLabel.Visibility = Visibility.Collapsed;
                PreviewModeVGA.Visibility = Visibility.Collapsed;
                //PreviewModeFHDRect.Visibility = Visibility.Collapsed;
                PreviewModeDump.Visibility = Visibility.Collapsed;
            }
        }

        private void UpdateUISettingsValues(rsid.DeviceConfig deviceConfig, MainWindow.FlowMode flowMode)
        {
            SecurityLevelHigh.IsChecked = deviceConfig.securityLevel == rsid.DeviceConfig.SecurityLevel.High;
            SecurityLevelMedium.IsChecked = deviceConfig.securityLevel == rsid.DeviceConfig.SecurityLevel.Medium;
            SecurityLevelRecognitionOnly.IsChecked = deviceConfig.securityLevel == rsid.DeviceConfig.SecurityLevel.RecognitionOnly;

            CameraRotation0.IsChecked = deviceConfig.cameraRotation == rsid.DeviceConfig.CameraRotation.Rotation_0_Deg;
            CameraRotation180.IsChecked = deviceConfig.cameraRotation == rsid.DeviceConfig.CameraRotation.Rotation_180_Deg;

            ServerModeNo.IsChecked = flowMode == MainWindow.FlowMode.Device;
            ServerModeYes.IsChecked = flowMode == MainWindow.FlowMode.Server;

            PreviewModeVGA.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.VGA;
            //PreviewModeFHDRect.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.FHD_Rect;
            PreviewModeDump.IsChecked = deviceConfig.previewMode == rsid.DeviceConfig.PreviewMode.Dump;
        }

        void QueryUISettingsValues(out rsid.DeviceConfig deviceConfig, out MainWindow.FlowMode flowMode)
        {
            deviceConfig = new rsid.DeviceConfig();
            deviceConfig.securityLevel = rsid.DeviceConfig.SecurityLevel.Medium;
            if (SecurityLevelHigh.IsChecked.GetValueOrDefault())
                deviceConfig.securityLevel = rsid.DeviceConfig.SecurityLevel.High;
            else if (SecurityLevelRecognitionOnly.IsChecked.GetValueOrDefault())
                deviceConfig.securityLevel = rsid.DeviceConfig.SecurityLevel.RecognitionOnly;

            deviceConfig.cameraRotation = CameraRotation0.IsChecked.GetValueOrDefault() ? rsid.DeviceConfig.CameraRotation.Rotation_0_Deg : rsid.DeviceConfig.CameraRotation.Rotation_180_Deg;
            flowMode = ServerModeNo.IsChecked.GetValueOrDefault() ? MainWindow.FlowMode.Device : MainWindow.FlowMode.Server;

            if (PreviewModeVGA.IsChecked.GetValueOrDefault() == true)
            {
                deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.VGA;
            }
            //else if (PreviewModeFHDRect.IsChecked.GetValueOrDefault() == true)
            //{
            //    deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.FHD_Rect;
            //}
            else
            {
                deviceConfig.previewMode = rsid.DeviceConfig.PreviewMode.Dump;
            }

            //we in adv mode only if the adv options are enabled            
            deviceConfig.advancedMode = PreviewModeVGA.Visibility == Visibility.Visible;
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
            QueryUISettingsValues(out rsid.DeviceConfig config, out MainWindow.FlowMode flowMode);
            Config = config;
            FlowMode = flowMode;
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
