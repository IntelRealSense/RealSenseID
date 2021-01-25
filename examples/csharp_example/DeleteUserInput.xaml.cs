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
    public partial class DeleteUserInput : Window
    {
        public DeleteUserInput(string[] userIds)
        {
            this.Owner = Application.Current.MainWindow;            
            DataContext = this;
            UserIds = userIds;
            InitializeComponent();            
        }

        public IEnumerable<string> UserIds { get; private set; }

        public string SelectedUser { get; set; }

        public bool DeleteAll { get; set; } = false;

        private void OKButton_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            DeleteAll = false;
            DialogResult = true;
        }

        private void DeleteAllBtn_Click(object sender, RoutedEventArgs e)
        {
            DeleteAll = true;
            DialogResult = true;
        }
    }
}
