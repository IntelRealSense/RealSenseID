// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using Microsoft.Win32;
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
    public partial class ConsoleWindow : Window
    {
        public bool _isEnabled = true;
        
        public ConsoleWindow()
        {
            this.Owner = Application.Current.MainWindow;            

            InitializeComponent();

            BindLogging();
        }

        public void OnLog(Logging.LogLevel level, string msg)
        {
            if (_isEnabled == false)
                return;

            Dispatcher.InvokeAsync(() =>
            {
                LogTextBox.Text += msg;
            });
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
                this.DragMove();
        }

        public void BindLogging()
        {
            rsid.Logging.SetLogCallback(OnLog, Logging.LogLevel.Debug, true);
        }

        public void Disable()
        {
            _isEnabled = false;
        }

        public void ToggleVisibility()
        {
            if (IsVisible == false)
            {
                Show();
            }
            else
            {
                Hide();
            }
        }
    }
}
