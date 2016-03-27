using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            using (SerialPort port = new SerialPort("COM3", 115200))
            {
                port.Open();
                while (true)
                {
                    Console.Write("Stuff to send: ");
                    string input = Console.ReadLine();
                    if (input == "quit")
                    {
                        break;
                    }
                    Console.WriteLine();
                    byte[] stringBytes = Encoding.Default.GetBytes(input);
                    Console.WriteLine(String.Format("Sending : {0}", ByteArrayToString(stringBytes)));
                    port.Write(stringBytes, 0, stringBytes.Length);
                    Task.Delay(1000).Wait();
                    byte[] returnValue = new byte[port.BytesToRead];
                    port.Read(returnValue, 0, port.BytesToRead);
                    Console.WriteLine(String.Format("Got : {0}", ByteArrayToString(returnValue)));
                }
                port.Close();
            }
        }

        static string ByteArrayToString(byte[] ba)
        {
            StringBuilder hex = new StringBuilder(ba.Length * 2);
            foreach (byte b in ba)
                hex.AppendFormat("0x{0:x2} ", b);
            return hex.ToString();
        }
    }
}
