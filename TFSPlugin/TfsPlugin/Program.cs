
using System.Security.Cryptography;
using System.Text;

namespace TfsPlugin
{
    internal class InstallChecker
    {
        const string VisualStudioVersion = "12";

        static int Main(string[] args)
        {
            // Uncomment this line to break into the debugger to debug the plugin while running Unity
            //  Debugger.Launch();

            return Run();
        }

        static int Run()
        {
            TfsTask task = new TfsTask(null, null, string.Empty);
            return task.Run();
        }
    }
}
