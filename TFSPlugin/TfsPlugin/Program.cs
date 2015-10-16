
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

        static readonly Dictionary<string, string> FallbackEnvVarValues = new Dictionary<string, string>
        {
            {"VS140COMNTOOLS", @"C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools"  },
            {"VS120COMNTOOLS", @"C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools"  },
        };

        static List<List<string>> GetAssemblyLoadPaths()
        {
            List<List<string>> result = new List<List<string>>();

            foreach (var eachEnvVariable in new[] { "VS140COMNTOOLS", "VS120COMNTOOLS" })
            {
                string value = Environment.GetEnvironmentVariable(eachEnvVariable);

                if (string.IsNullOrEmpty(value) || !Directory.Exists(value))
                {
                    if (Directory.Exists(FallbackEnvVarValues[eachEnvVariable]))
                    {
                        value = eachEnvVariable;
                    }
                    else
                    {
                        continue;
                    }
                }

                var paths = AssemblyLoadPaths[eachEnvVariable];
                List<string> pathSet = new List<string>();

                for (int i = 0; i < paths.Length; i++)
                {
                    pathSet.Add(Path.GetFullPath(Path.Combine(value, paths[i])));
                }

                result.Add(pathSet);
            }

            return result;
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
                foreach (var eachPath in loadPaths.ToArray())
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
            try
            {
                if (LoadedAssemblies != null)
                {
                    return LoadedAssemblies;
                }

                var loadPathSets = GetAssemblyLoadPaths();

                Dictionary<string, Assembly> result = new Dictionary<string, Assembly>();

                foreach (var eachLoadPath in loadPathSets)
                {
                    var assembliesToLoad = GetAssembliesToLoad(AssembliesToLoad, eachLoadPath);

                    if (assembliesToLoad.Count != AssembliesToLoad.Length)
                    {
                        continue;
                    }

                    var loadedAssemblies = LoadAssembliesFrom(assembliesToLoad);

                    if (loadedAssemblies == null)
                    {
                        continue;
                    }

                    result = loadedAssemblies;
                    break;
                }

                AppDomain.CurrentDomain.AssemblyResolve += CurrentDomain_AssemblyResolve;

                return result;
            }
            catch
            {
                return new Dictionary<string, Assembly>();
            }
        }

        private static Dictionary<string, Assembly> LoadAssembliesFrom(List<string> assembliesToLoad)
        {
            Dictionary<string, Assembly> result = new Dictionary<string, Assembly>();
            foreach (var item in assembliesToLoad)
            {
                try
                {
                    Assembly assembly = Assembly.LoadFrom(item);
                    result[TrimAssemblyName(assembly.FullName)] = assembly;
                }
                catch
                {
                    return null;
                }
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
