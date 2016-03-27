using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TeleFocusUI
{
    public static class SerialSupport
    {
        public static string CurrentDevicePortName;

        public static bool SearchForDevice()
        {
            LoggingUtility.LogItem("Searching for device...");
            
            List<string> livePorts = new List<string>();
            string[] portNames = SerialPort.GetPortNames();
            foreach (string portName in portNames)
            {
                using(SerialPort port = BuildSerialPort(portName))
                {
                    string response = SendCommand("E#", port);
                    if (response != null && response.Contains("Focuser v0.1"))
                    {
                        LoggingUtility.LogItem(String.Format("Found focuser device on {0}.", port.PortName));
                        CurrentDevicePortName = port.PortName;
                        return true;
                    }
                }
            }

            return false;
        }

        private static SerialPort BuildSerialPort(string portName)
        {
            SerialPort newPort = new SerialPort(portName, 115200, Parity.None, 8, StopBits.One);
            newPort.ReadTimeout = 3000;
            newPort.Handshake = Handshake.None;
            return newPort;
        }

        public static string SendCommand(string command, SerialPort port)
        {
            string response = null;
            try
            {
                if (!port.IsOpen)
                {
                    LoggingUtility.LogItem(String.Format("Opening port {0}.", port.PortName));
                    port.Open();
                    port.BaseStream.Flush();
                }

                byte[] commandBytes = Encoding.Default.GetBytes(command);
                LoggingUtility.LogItem(String.Format("Writing ({0}) {1} to port {2}.", command, ByteArrayToString(commandBytes), port.PortName));
                
                port.Write(commandBytes, 0, commandBytes.Length);

                response = port.ReadTo("#");
                LoggingUtility.LogItem(String.Format("Got {0} from port {1}.", response, port.PortName));
            }
            catch (TimeoutException)
            {
                LoggingUtility.LogItem("Timeout");
                return null;
            }
            catch (UnauthorizedAccessException)
            {
                LoggingUtility.LogItem("Auth Error");
            }
            finally
            {
                if (port.IsOpen)
                {
                    port.Close();
                    LoggingUtility.LogItem(String.Format("Port {0} closed.", port.PortName));
                }
            }

            return response;
        }

        public static string ByteArrayToString(byte[] ba)
        {
            StringBuilder hex = new StringBuilder(ba.Length * 2);
            foreach (byte b in ba)
                hex.AppendFormat("0x{0:x2} ", b);
            return hex.ToString();
        }
    }
}
