using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

using Microsoft.Win32.SafeHandles;

namespace Codice.Unity3D
{
    internal class PipeClient
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern SafeFileHandle CreateFile(
           String pipeName,
           uint dwDesiredAccess,
           uint dwShareMode,
           IntPtr lpSecurityAttributes,
           uint dwCreationDisposition,
           uint dwFlagsAndAttributes,
           IntPtr hTemplate);

        internal bool Connected
        {
            get { return mbConnected; }
        }

        internal void Connect()
        {
            this.mHandle =
               CreateFile(
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
        }

        internal void SendMessage(string message)
        {
            ASCIIEncoding encoder = new ASCIIEncoding();
            byte[] messageBuffer = encoder.GetBytes(message);

            mStream.Write(messageBuffer, 0, messageBuffer.Length);
            mStream.Flush();
        }

        internal void Close()
        {
            if (mStream != null)
                mStream.Close();

            if (mHandle != null)
                mHandle.Close();
        }

        const uint GENERIC_READ = (0x80000000);
        const uint GENERIC_WRITE = (0x40000000);
        const uint OPEN_EXISTING = 3;
        const uint FILE_FLAG_OVERLAPPED = (0x40000000);
        const int BUFFER_SIZE = 4096;

        const string PIPE_NAME = "\\\\.\\pipe\\UnityVCS";

        FileStream mStream;
        SafeFileHandle mHandle;
        bool mbConnected;
    }
}