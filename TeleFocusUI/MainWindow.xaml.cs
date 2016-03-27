using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO.Ports;
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

namespace TeleFocusUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private DependencyPropertyDescriptor fwdButtonDescriptor, bwdButtonDescriptor;
        private SerialPort _currentPort;

        public MainWindow()
        {
            InitializeComponent();
            LoggingUtility.ItemWrittenToLog += LoggingUtility_ItemWrittenToLog;
            BindPropertyDescriptors();

            Task.Run(() => SerialSupport.SearchForDevice())
                .ContinueWith((prev) => _currentPort = new SerialPort(SerialSupport.CurrentDevicePortName))
                .ContinueWith((prev) => UnlockControls())
                .ContinueWith((prev) => SetSpeed());
        }

        private void SetSpeed()
        {
            string command = String.Concat("SS", (char)10, '#');
            LoggingUtility.LogItem(String.Format("Speed set result: {0}",SerialSupport.SendCommand(command, _currentPort)));
        }

        void LoggingUtility_ItemWrittenToLog(string message)
        {
            if (LogListBox.Dispatcher.CheckAccess())
            {
                LogListBox.Items.Add(message);
            }
            else
            {
                this.LogListBox.Dispatcher.Invoke(
                    System.Windows.Threading.DispatcherPriority.Normal,
                    new Action(() => LogListBox.Items.Add(message)));
            }
        }

        private void BindPropertyDescriptors()
        {
            fwdButtonDescriptor = DependencyPropertyDescriptor.FromProperty(Button.IsPressedProperty, typeof(Button));
            fwdButtonDescriptor.AddValueChanged(this.FwdButton, new EventHandler(FwdButtonPressedHandler));
            bwdButtonDescriptor = DependencyPropertyDescriptor.FromProperty(Button.IsPressedProperty, typeof(Button));
            bwdButtonDescriptor.AddValueChanged(this.BwdButton, new EventHandler(BwdButtonPressedHandler));
        }

        private void FwdButtonPressedHandler(object sender, EventArgs e)
        {
            if (FwdButton.IsPressed)
            {
                SerialSupport.SendCommand("MF", _currentPort);
            }
            else
            {
                SerialSupport.SendCommand("MS", _currentPort);
            }
        }

        private void BwdButtonPressedHandler(object sender, EventArgs e)
        {
            if (FwdButton.IsPressed)
            {
                SerialSupport.SendCommand("MB", _currentPort);
            }
            else
            {
                SerialSupport.SendCommand("MS", _currentPort);
            }
        }

        private void UnlockControls()
        {
            if (FwdButton.Dispatcher.CheckAccess())
            {
                FwdButton.IsEnabled = true;
            }
            else
            {
                this.FwdButton.Dispatcher.Invoke(
                    System.Windows.Threading.DispatcherPriority.Normal,
                    new Action(() => FwdButton.IsEnabled = true));
            }

            if (BwdButton.Dispatcher.CheckAccess())
            {
                BwdButton.IsEnabled = true;
            }
            else
            {
                this.BwdButton.Dispatcher.Invoke(
                    System.Windows.Threading.DispatcherPriority.Normal,
                    new Action(() => BwdButton.IsEnabled = true));
            }
        }
    }
}
