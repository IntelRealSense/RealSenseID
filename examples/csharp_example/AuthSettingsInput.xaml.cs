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
        public AuthConfig Config { get; private set; }
        public MainWindow.FlowMode FlowMode { get; private set; }
        public string FirmwareFileName { get; private set; } = string.Empty;

        public AuthSettingsInput(string fwVersion, rsid.AuthConfig? config, MainWindow.FlowMode flowMode)
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
            CameraRotation0.IsEnabled = hasAuth;
            CameraRotation180.IsEnabled = hasAuth;
            ServerModeYes.IsEnabled = hasAuth;
            ServerModeNo.IsEnabled = hasAuth;
        }

        private void UpdateUISettingsValues(rsid.AuthConfig authConfig, MainWindow.FlowMode flowMode)
        {
            SecurityLevelHigh.IsChecked = authConfig.securityLevel == rsid.AuthConfig.SecurityLevel.High;
            SecurityLevelMedium.IsChecked = authConfig.securityLevel == rsid.AuthConfig.SecurityLevel.Medium;

            CameraRotation0.IsChecked = authConfig.cameraRotation == rsid.AuthConfig.CameraRotation.Rotation_0_Deg;
            CameraRotation180.IsChecked = authConfig.cameraRotation == rsid.AuthConfig.CameraRotation.Rotation_180_Deg;

            ServerModeNo.IsChecked = flowMode == MainWindow.FlowMode.Device;
            ServerModeYes.IsChecked = flowMode == MainWindow.FlowMode.Server;
        }

        void QueryUISettingsValues(out rsid.AuthConfig authConfig, out MainWindow.FlowMode flowMode)
        {
            authConfig = new rsid.AuthConfig();
            authConfig.securityLevel = SecurityLevelHigh.IsChecked.GetValueOrDefault() ? rsid.AuthConfig.SecurityLevel.High : rsid.AuthConfig.SecurityLevel.Medium;
            authConfig.cameraRotation = CameraRotation0.IsChecked.GetValueOrDefault() ? rsid.AuthConfig.CameraRotation.Rotation_0_Deg : rsid.AuthConfig.CameraRotation.Rotation_180_Deg;
            flowMode = ServerModeNo.IsChecked.GetValueOrDefault() ? MainWindow.FlowMode.Device : MainWindow.FlowMode.Server;
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
            QueryUISettingsValues(out rsid.AuthConfig config, out MainWindow.FlowMode flowMode);
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
