using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TeleFocusUI
{
    public static class LoggingUtility
    {
        public delegate void WriteLogItemDelegate(string message);
        public static event WriteLogItemDelegate ItemWrittenToLog;

        public static void LogItem(string logItem)
        {
            if (ItemWrittenToLog != null)
            {
                ItemWrittenToLog(logItem);
            }
        }
    }
}
