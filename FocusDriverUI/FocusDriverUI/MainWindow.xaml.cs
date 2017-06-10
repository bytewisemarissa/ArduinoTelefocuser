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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace FocusDriverUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            LogManager.LogMessage("Main window created.");
        }

        private void LogButton_OnClick(object sender, RoutedEventArgs e)
        {
            if (LogButton.IsChecked.HasValue && LogButton.IsChecked.Value)
            {
                Logger.Visibility = Visibility.Visible;
            }
            else
            {
                Logger.Visibility = Visibility.Collapsed;
            }
        }

        private void RefreshButton_OnClick(object sender, RoutedEventArgs e)
        {
            throw new NotImplementedException();
        }
    }
}
