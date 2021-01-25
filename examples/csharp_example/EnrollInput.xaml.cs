// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

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
    public partial class EnrollInput : Window
    {
        public EnrollInput()
        {
            this.Owner = Application.Current.MainWindow;
            InitializeComponent();
        }

        public string EnrolledUsername
        {
            get { return Username.Text; }
            set { Username.Text = value; }
        }

        private void OKButton_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            DialogResult = IsValidUserId(Username.Text);
        }


        private void Username_TextChanged(object sender, TextChangedEventArgs e)
        {                        
            if (IsValidUserId(Username.Text) || Username.Text.Length == 0)
            {
                // show error label and disable ok button on invalid input
                InputErrorLabel.Visibility = Visibility.Hidden;
                NewUserIdOKBtn.IsEnabled = Username.Text.Any();
            }
            else
            {
                // hide error label and enable ok button on valid input
                InputErrorLabel.Visibility = Visibility.Visible;
                NewUserIdOKBtn.IsEnabled = false;
            }            
        }

        private bool IsValidUserId(string val)
        {
            return !string.IsNullOrEmpty(val) && val.All((ch) => ch <= 127) && val.Length <= 15;
        }


    }
}
