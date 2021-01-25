// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using rsid;
using System;
using System.Collections.Generic;
using System.Linq;
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
        private AuthConfig _authConfig;
        public AuthSettingsInput(rsid.AuthConfig knownConfig)
        {
            this.Owner = Application.Current.MainWindow;            
            InitializeComponent();

            // Init dialog values according to current config
            Rotation0.IsChecked = knownConfig.cameraRotation == AuthConfig.CameraRotation.Rotation_0_Deg;
            Rotation180.IsChecked = knownConfig.cameraRotation == AuthConfig.CameraRotation.Rotation_180_Deg;

            RadioMedium.IsChecked = knownConfig.securityLevel == AuthConfig.SecurityLevel.Medium;
            RadioHigh.IsChecked = knownConfig.securityLevel == AuthConfig.SecurityLevel.High;                        
        }

        public AuthConfig Config
        {
            get { return _authConfig; }
            private set { _authConfig = value; }
        }

        private void OKButton_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            var securityLevel = RadioMedium.IsChecked.GetValueOrDefault() ? AuthConfig.SecurityLevel.Medium : AuthConfig.SecurityLevel.High;
            var cameraRotation = Rotation0.IsChecked.GetValueOrDefault() ? AuthConfig.CameraRotation.Rotation_0_Deg : AuthConfig.CameraRotation.Rotation_180_Deg;
            _authConfig = new AuthConfig { securityLevel = securityLevel, cameraRotation = cameraRotation };
            DialogResult = true;
        }

        // Enable submit button only if all required radio buttons were selected
        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            bool ok = RadioHigh.IsChecked.GetValueOrDefault() || RadioMedium.IsChecked.GetValueOrDefault();
            ok = ok && (Rotation0.IsChecked.GetValueOrDefault() || Rotation180.IsChecked.GetValueOrDefault());
            OkBtn.IsEnabled = ok;
        }
    }
}
