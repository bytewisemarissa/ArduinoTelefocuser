using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FocusDriverUI
{
    public static class LogManager
    {
        public delegate void HandleLogMessage(string log);
        public static event HandleLogMessage LogRecievedEvent;

        public static void LogMessage(string message)
        {
            LogRecievedEvent?.Invoke(message);
        }
    }
}
