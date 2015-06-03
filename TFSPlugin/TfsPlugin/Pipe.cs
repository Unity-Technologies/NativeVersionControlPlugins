using Microsoft.Win32.SafeHandles;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace TfsPlugin
{
    public class Pipe : IDisposable
    {
        class NativeMethods
        {
            // using unicode for pipename may cause editor crash 
            [DllImport("kernel32.dll",  SetLastError = true)]
            internal static extern SafeFileHandle CreateFile(
               String pipeName,
               uint dwDesiredAccess,
               uint dwShareMode,
               IntPtr lpSecurityAttributes,
               uint dwCreationDisposition,
               uint dwFlagsAndAttributes,
               IntPtr hTemplate);
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

        public bool Connected
        {
            get { return mbConnected; }
        }

        public void Connect(Stream toStream)
        {
            mbConnected = true;

            mStream = toStream;

            mReader = new StreamReader(mStream);
        }

        public void Connect()
        {
            this.mHandle =
               NativeMethods.CreateFile(
                  PIPE_NAME,
                  GENERIC_READ | GENERIC_WRITE,
                  0,
                  IntPtr.Zero,
                  OPEN_EXISTING,
                  FILE_FLAG_OVERLAPPED,
                  IntPtr.Zero);

            //could not create handle - server probably not running
            if (mHandle.IsInvalid)
                return;

            mbConnected = true;

            mStream = new FileStream(
                mHandle, FileAccess.ReadWrite, BUFFER_SIZE, true);

            mReader = new StreamReader(mStream);
        }

        public void SendMessage(string message)
        {
            ASCIIEncoding encoder = new ASCIIEncoding();
            byte[] messageBuffer = encoder.GetBytes(message);

            mStream.Write(messageBuffer, 0, messageBuffer.Length);
            mStream.Flush();
        }

        public void SendMessageLine(string message)
        {
            ASCIIEncoding encoder = new ASCIIEncoding();
            byte[] messageBuffer = encoder.GetBytes(message + Environment.NewLine);

            mStream.Write(messageBuffer, 0, messageBuffer.Length);
            mStream.Flush();
        }

        public void Close()
        {
            if (mReader != null)
            {
                mReader.Close();
                mReader = null;
            }

            if (mStream != null)
            {
                mStream.Close();
                mStream = null;
            }

            if (mHandle != null)
            {
                mHandle.Close();
                mHandle = null;
            }
        }

        const uint GENERIC_READ = (0x80000000);
        const uint GENERIC_WRITE = (0x40000000);
        const uint OPEN_EXISTING = 3;
        const uint FILE_FLAG_OVERLAPPED = (0x40000000);
        const int BUFFER_SIZE = 4096;

        const string PIPE_NAME = "\\\\.\\pipe\\UnityVCS";

        Stream mStream;
        StreamReader mReader;
        SafeFileHandle mHandle;
        bool mbConnected;

        public bool IsEOF()
        {
            return mReader.Peek() < 0;
        }

        public string ReadLine()
        {
            return mReader.ReadLine();
        }
    }
}