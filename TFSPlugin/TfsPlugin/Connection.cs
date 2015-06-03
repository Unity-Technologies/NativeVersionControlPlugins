using System;
using System.Collections.Generic;
using System.IO;

namespace TfsPlugin
{
    public enum LogLevel
    {
        LOG_DEBUG,  // protocol messages
        LOG_INFO,   // commands
        LOG_NOTICE, // warnings 
        LOG_FATAL,  // errors and exceptions
    };

    public enum MessageArea
    {
        MAGeneral = 1,
        MASystem = 2,        // Error on local system e.g. cannot create dir
        MAPlugin = 4,        // Error in protocol while communicating with the plugin executable
        MAConfig = 8,        // Error in configuration e.g. config value not set
        MAConnect = 16,      // Error during connect to remote server
        MAProtocol = 32,     // Error in protocol while communicating with server
        MARemote = 64,       // Error on remote server
        MAInvalid = 128      // Must alway be the last entry
    };

    public interface IWritesToConnection
    {
        void Write(Connection p);
    }

    public interface IReadsFromConnection
    {
        void Read(Connection p);
    }

    public class Connection : IDisposable
    {
        string DATA_PREFIX = "o";
        string VERBOSE_PREFIX = "v";
        string ERROR_PREFIX = "e";
        string WARNING_PREFIX = "w";
        string INFO_PREFIX = "i";
        string COMMAND_PREFIX = "c";
        string PROGRESS_PREFIX = "p";

        const int MAX_LOG_FILE_SIZE = 2000000;

        Pipe m_Pipe;
        StreamWriter m_Log;
        LogLevel m_logLevel;


        public Connection(Stream logStream, Stream pipeStream)
        {
            m_Pipe = new Pipe();
            m_Pipe.Connect(pipeStream);

            m_Log = new StreamWriter(logStream);
        }

        public Connection(string logPath)
        {
            m_Pipe = new Pipe();
            m_Pipe.Connect();

            try
            {
                FileInfo logFile = new FileInfo(logPath);

                if (logFile.Exists && logFile.Length > MAX_LOG_FILE_SIZE)
                {
                    string prevPath = logPath;

                    prevPath += "-prev";
                    if (File.Exists(prevPath))
                        File.Delete(prevPath);
                    File.Move(logPath, prevPath);
                }

                if (File.Exists(logPath))
                {
                    m_Log = File.AppendText(logPath);
                }
                else
                {
                    m_Log = File.CreateText(logPath);
                }
            }
            catch
            {
                // if we are unable to write to the log on disk, disable logging 
                m_Log = null;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                Close();
            }
        }

        public bool IsConnected()
        {
            return m_Pipe != null && m_Pipe.Connected;
        }


        public void SetLogLevel(LogLevel l) { m_logLevel = l; }
        public LogLevel GetLogLevel() { return m_logLevel; }

        public void Log(string message, bool timestamp = true)
        {
            if (m_Log != null)
            {
                m_Log.WriteLine((timestamp ? DateTime.Now.ToString() + " " : string.Empty) + message);
                m_Log.Flush();
            }
        }

        public void Log(string message, LogLevel logLevel)
        {
            if (m_logLevel <= logLevel)
                Log(message);
        }

        public void LogDebug(string message, bool timestamp = true)
        {
            if (m_logLevel <= LogLevel.LOG_DEBUG)
                Log(message);
        }
        public void LogInfo(string message)
        {
            if (m_logLevel <= LogLevel.LOG_INFO)
                Log(message);
        }
        public void LogNotice(string message)
        {
            if (m_logLevel <= LogLevel.LOG_NOTICE)
                Log(message);
        }
        public void LogFatal(string message)
        {
            if (m_logLevel <= LogLevel.LOG_FATAL)
                Log(message);
        }

        public T ReadLine<T>(Func<string, T> parseFunc)
        {
            var target = m_Pipe.ReadLine();

            target = DecodeString(target);
            T result = parseFunc(target);

            LogDebug("UNITY > " + target + " " + result);
            return result;
        }

        public string ReadLine()
        {
            var target = m_Pipe.ReadLine();

            target = DecodeString(target);

            LogDebug("UNITY > " + target);
            return target;
        }

        // Params: -1 means not specified
        public void Progress(int pct, long timeSoFar, string message = "", MessageArea ma = MessageArea.MAGeneral)
        {
            string msg = pct.ToString() + " " + ((int)timeSoFar).ToString() + " " + message;
            WritePrefixLine(PROGRESS_PREFIX, ma, msg, LogLevel.LOG_NOTICE);
        }

        private string DecodeString(string target)
        {
            if (string.IsNullOrEmpty(target))
            {
                return string.Empty;
            }

            return target.Replace(@"\\", "\\").Replace(@"\n", "\n");
        }

        public UnityCommand ReadCommand(List<string> args)
        {
            if (!IsConnected())
            {
                return UnityCommand.UCOM_Invalid;
            }

            args.Clear();
            string read;
            read = ReadLine();

            if (read.Length == 0)
            {
                // broken pipe -> error.
                if (!m_Pipe.IsEOF())
                {
                    ErrorLine("Read empty command from connection");
                    return UnityCommand.UCOM_Invalid;
                }

                LogInfo("End of pipe");
                return UnityCommand.UCOM_Shutdown;
            }

            // Skip non-command lines if present
            int stopCounter = 1000;
            while (!read.StartsWith("c:") && stopCounter != 0)
            {
                read = ReadLine();
                stopCounter -= 1;
            }

            if (stopCounter == 0)
            {
                ErrorLine("Too many bogus lines");
                return UnityCommand.UCOM_Invalid;
            }

            string command = read.Substring(2);

            if (Tokenize(args, command) == 0)
            {
                ErrorLine("invalid formatted - '" + command + "'");
                return UnityCommand.UCOM_Invalid;
            }

            return StringToUnityCommand(args[0]);
        }

        private int Tokenize(List<string> result, string str, string delimiters = " ")
        {
            int startPos = 0;
            int endPos = str.IndexOf(delimiters);

            result.Clear();
            int strLen = str.Length;

            while (endPos != -1)
            {
                string tok = str.Substring(startPos, endPos - startPos);
                startPos = ++endPos;

                if (tok.Length != 0) result.Add(tok);

                if (startPos >= strLen) return result.Count;

                endPos = str.IndexOf(delimiters, startPos);
            }

            string token = str.Substring(startPos);
            result.Add(token);

            return result.Count;
        }

        public struct CmdInfo
        {
            public UnityCommand id;
            public string name;
        }

        public static readonly CmdInfo[] infos = new[]
        {
	        new CmdInfo{ id=UnityCommand.UCOM_Shutdown, name="shutdown" },
	        new CmdInfo{ id=UnityCommand.UCOM_Add, name="add" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_ChangeDescription, name="changeDescription" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_ChangeMove, name="changeMove" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_ChangeStatus, name="changeStatus" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Changes, name="changes" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Checkout, name="checkout" }, //  
	        new CmdInfo{ id=UnityCommand.UCOM_Config, name="pluginConfig" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_DeleteChanges, name="deleteChanges" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Delete, name="delete" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Download, name="download" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Exit, name="exit" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_GetLatest, name="getLatest" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_IncomingChangeAssets, name="incomingChangeAssets" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Incoming, name="incoming" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Lock, name="lock" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Login, name="login" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Move, name="move" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_QueryConfigParameters, name="queryConfigParameters" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Resolve, name="resolve" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_RevertChanges, name="revertChanges" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Revert, name="revert" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Status, name="status" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Submit, name="submit" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Unlock, name="unlock" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_FileMode, name="filemode" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_CustomCommand, name="customCommand" }, //
	        new CmdInfo{ id=UnityCommand.UCOM_Invalid,name= string.Empty } // delimiter
        };

        private UnityCommand StringToUnityCommand(string p)
        {
            foreach (var item in infos)
            {
                if (item.name == p)
                    return item.id;
            }

            return UnityCommand.UCOM_Invalid;
        }

        void BeginList()
        {
            // Start sending list of unknown size
            DataLine("-1");

        }

        void EndList()
        {
            // d is list delimiter
            WriteLine("d1:end of list", LogLevel.LOG_DEBUG);
        }


        public void EndResponse()
        {
            WriteLine("r1:end of response", LogLevel.LOG_DEBUG);

        }


        public void VerboseLine(string msg, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(VERBOSE_PREFIX, ma, msg);
        }

        public void InfoLine(string msg, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(INFO_PREFIX, ma, msg);
        }

        public void DataLine(string msg, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(DATA_PREFIX, ma, msg);
        }

        public void DataLine(int msg, MessageArea ma = MessageArea.MAGeneral, string logNote = null)
        {
            WritePrefixLine(DATA_PREFIX, ma, msg.ToString(), logNote: logNote);
        }

        public void ErrorLine(string msg, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(ERROR_PREFIX, ma, msg);
        }
        public void WarnLine(string msg, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(WARNING_PREFIX, ma, msg);
        }

        void WritePrefixLine(string prefix, MessageArea ma, string msg, LogLevel logLevel = LogLevel.LOG_DEBUG, string logNote = null)
        {
            var cleanedMessage = msg.Replace("\n", "").Replace("\r", "");

            WriteLine(string.Format("{0}{1}:{2}", prefix, ((int)ma).ToString(), cleanedMessage), logLevel, logNote);
        }

        internal void WriteLine(string message, LogLevel logLevel = LogLevel.LOG_DEBUG, string logNote = null)
        {
            if (!m_Pipe.Connected)
                return;

            var toSend = string.Format("{0}{1}", message, Environment.NewLine);
            Log(message + (logNote != null ? (" " + logNote) : string.Empty), logLevel);
            m_Pipe.SendMessage(toSend);

        }

        internal void Close()
        {
            if (m_Pipe != null)
            {
                m_Pipe.Close();
                m_Pipe = null;
            }

            if (m_Log != null)
            {
                m_Log.Close();
                m_Log = null;
            }
        }

        internal T Read<T>() where T : IReadsFromConnection, new()
        {
            var r = new T();
            r.Read(this);
            return r;
        }

        internal void Read<T>(List<T> list) where T : IReadsFromConnection, new()
        {
            string line;
            line = ReadLine();
            int count = int.Parse(line);
            if (count >= 0)
            {
                while (count-- != 0)
                {
                    T t = new T();
                    t.Read(this);
                    list.Add(t);
                }
            }


        }
        internal void Write<T>(List<T> list) where T : IWritesToConnection
        {
            DataLine(list.Count);
            foreach (var i in list)
            {
                i.Write(this);
            }
        }

        internal void Write<T>(HashSet<T> list) where T : IWritesToConnection
        {
            DataLine(list.Count);
            foreach (var i in list)
            {
                i.Write(this);
            }
        }

        internal void Read<T>(HashSet<T> list) where T : IReadsFromConnection, new()
        {
            string line;
            line = ReadLine();
            int count = int.Parse(line);
            if (count >= 0)
            {
                while (count-- != 0)
                {
                    T t = new T();
                    t.Read(this);
                    list.Add(t);
                }
            }


        }


        internal void Command(string cmd, MessageArea ma = MessageArea.MAGeneral)
        {
            WritePrefixLine(COMMAND_PREFIX, ma, cmd);
        }
    }
}
