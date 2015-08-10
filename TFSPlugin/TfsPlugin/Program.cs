
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace TfsPlugin
{
    public class InstallChecker
    {
        static readonly string[] AssembliesToLoad = new string[]
        {
            "Microsoft.TeamFoundation.Client.dll",
            "Microsoft.TeamFoundation.Common.dll",
            "Microsoft.TeamFoundation.Diff.dll",
            "Microsoft.TeamFoundation.VersionControl.Client.dll",
            "Microsoft.TeamFoundation.VersionControl.Common.dll",
            "Microsoft.VisualStudio.Services.Client.dll",
            "Microsoft.VisualStudio.Services.Common.dll"
        };

        static readonly Dictionary<string, string[]> AssemblyLoadPaths = new Dictionary<string, string[]>
        {
            {"VS140COMNTOOLS", new string[] {@"..\IDE\Extensions" } },
            {"VS120COMNTOOLS", new string[] { @"..\IDE\ReferenceAssemblies" } },
        };

        static List<string> GetAssemblyLoadPaths()
        {
            foreach (var eachEnvVariable in new[] { "VS140COMNTOOLS", "VS120COMNTOOLS" })
            {
                string value = Environment.GetEnvironmentVariable(eachEnvVariable);

                if (value == null)
                {
                    continue;
                }

                var paths = AssemblyLoadPaths[eachEnvVariable];
                List<string> result = new List<string>();

                for (int i = 0; i < paths.Length; i++)
                {
                    result.Add( Path.GetFullPath(Path.Combine(value, paths[i])));
                }

                return result;
            }

            return null;
        }

        static string TrimAssemblyName(string name)
        {
            if (!string.IsNullOrEmpty(name))
            {
                int i = name.IndexOf(",");

                if (i != -1)
                {
                    return name.Substring(0, i);
                }
            }

            return name;
        }

        static List<string> GetAssembliesToLoad(string[] toLoad, List<string> loadPaths)
        {
            List<string> result = new List<string>();

            if (loadPaths == null)
            {
                return result;
            }

            foreach (var eachAssembly in toLoad)
            {
                foreach (var eachPath in loadPaths)
                {
                    string[] found = null;
                    var trialPath = Path.Combine(eachPath, eachAssembly);
                    if (File.Exists(trialPath))
                    {
                        found = new[] { trialPath };
                    }

                    if (found == null)
                    {
                        found = Directory.GetFiles(eachPath, eachAssembly, SearchOption.AllDirectories);
                    }

                    if (found.Length > 0 && File.Exists(found[0]))
                    {
                        result.Add(found[0]);

                        var dir = Path.GetDirectoryName(found[0]);
                        if (!loadPaths.Contains(dir))
                        {
                            loadPaths.Insert(0, dir);
                        }
                        break;
                    }
                }
            }

            return result;
        }

        public static Dictionary<string, Assembly> LoadAssemblies()
        {
            if (LoadedAssemblies != null)
            {
                return LoadedAssemblies;
            }
            
            var loadPaths = GetAssemblyLoadPaths();
            var assembliesToLoad = GetAssembliesToLoad(AssembliesToLoad, loadPaths);
            
            var result = LoadAssembliesFrom(assembliesToLoad);

            AppDomain.CurrentDomain.AssemblyResolve += CurrentDomain_AssemblyResolve;

            return result;
        }

        private static Dictionary<string, Assembly> LoadAssembliesFrom(List<string> assembliesToLoad)
        {
            Dictionary<string, Assembly> result = new Dictionary<string, Assembly>();
            foreach (var item in assembliesToLoad)
            {
                Assembly assembly = Assembly.LoadFrom(item);
                result[TrimAssemblyName(assembly.FullName)] = assembly;
            }

            return result;
        }

        static Dictionary<string, Assembly> LoadedAssemblies = LoadAssemblies();


        static int Main(string[] args)
        {
            // Uncomment this line to break into the debugger to debug the plugin while running Unity
            //  Debugger.Launch();   

            LoadedAssemblies = LoadAssemblies();

            return Run();
        }

        private static Assembly CurrentDomain_AssemblyResolve(object sender, ResolveEventArgs args)
        {
            Assembly found;
            if (LoadedAssemblies.TryGetValue(TrimAssemblyName(args.Name), out found))
            {
                return found;
            }

            return null;
        }

        static int Run()
        {
            TfsTask task = new TfsTask(null, null, string.Empty);
            return task.Run();
        }
    }
}
