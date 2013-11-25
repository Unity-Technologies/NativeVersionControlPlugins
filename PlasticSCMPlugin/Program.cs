using System;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace Codice.Unity3D
{
    internal class InstallChecker
    {
        static void Main(string[] args)
        {
            string installPath = GetInstallFolder(PLUGIN_EXEC_NAME);

            if (string.IsNullOrEmpty(installPath))
            {
                NotifyNotInstalled();
                return;
            }

            ExecutePlugin(PLUGIN_EXEC_NAME);
        }

        private static string GetInstallFolder(string fileToCheck)
        {
            StringDictionary env =
                Process.GetCurrentProcess().StartInfo.EnvironmentVariables;

            string pathEnvVble = env["PATH"];
            string[] paths = new string[] { };
            paths = pathEnvVble.Split(new char[] { ';' });

            foreach (string path in paths)
            {
                if (File.Exists(Path.Combine(path, fileToCheck)))
                    return path;
            }
            return string.Empty;
        }

        private static void NotifyNotInstalled()
        {
            try
            {
                UnityConnection conn = new UnityConnection();
                conn.WriteLine(string.Format("e1:{0}", NOT_INSTALLED_MSG));
                conn.WriteLine("r1:end of response");
                conn.Close();
            }
            catch
            {
                //nothing to do.We cannot neither log nor crash
            }
        }

        private static void ExecutePlugin(string execFile)
        {
            Process p = new Process();
            p.StartInfo.FileName = execFile;
            p.StartInfo.Arguments = string.Empty;
            p.StartInfo.CreateNoWindow = true;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;

            try
            {
                p.Start();
                p.WaitForExit();
            }
            catch
            {
                //nothing to do.We cannot neither log nor crash
            }
            finally
            {
                if (p != null)
                    p.Close();
            }
        }

        private static string GetExecutingLocation()
        {
            try
            {
                return Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            }
            catch
            {
                return string.Empty;
            }
        }

        const string PLUGIN_EXEC_NAME = "PlasticSCMUnityPlugin.exe";

        const string NOT_INSTALLED_MSG =
            "PlasticSCM not installed. Please visit http://www.plasticscm.com " +
            "to download PlasticSCM or contact support team at: " +
            "support@codicesoftware.com";
    }

    internal class UnityConnection
    {
        internal UnityConnection()
        {
            mClient = new PipeClient();
            mClient.Connect();
        }

        internal void WriteLine(string message)
        {
            if (!mClient.Connected)
                return;

            mClient.SendMessage(
                string.Format("{0}{1}", message, Environment.NewLine));
        }

        internal void Close()
        {
            mClient.Close();
        }

        PipeClient mClient;
    }
}
