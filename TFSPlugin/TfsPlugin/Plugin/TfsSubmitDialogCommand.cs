using System;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace TfsPlugin
{
    class TfsSubmitDialogCommand : ICommand<TfsTask, SubmitRequest, SubmitResponse>
    {
        public bool Run(TfsTask task, SubmitRequest req, SubmitResponse resp)
        {
            bool saveOnly = req.Args.Count > 1 && req.Args[1] == "saveOnly";

            if (saveOnly)
            {
                return true;
            }

            if (req.Assets.Count == 0)
            {
                return true;
            }

            req.Conn.Progress(-1, 0, "submitting");

            var projectPending = task.GetProjectPendingChanges();
            var pending = task.GetProjectPendingChanges(req.Assets);
            var comment = req.Changelist.GetDescription();
            var tfPath = GetTfsPath();

            if (tfPath == null)
            {
                req.Conn.ErrorLine("Unable to find tf.exe");
            }
            else
            {
                try
                {
                    var args = "submit ";

                    // if the set of requested assets to checkin match the set of all project pending changes, 
                    // use the recursive project path for the item spec because using an item spec per file will overflow the command line args for large checkins.
                    if (TfsTask.PendingChangesAreTheSame(projectPending, pending))
                    {
                        args += "/recursive " + "\"" + task.ProjectPath + "\"";
                    }
                    else
                    {
                        args += string.Join(" ", pending.Select(each => "\"" + each.LocalItem + "\"").ToArray());
                    }

                    args += " /comment:" + "\"" + comment.Replace("\"", "\"\"").Replace("\n", "\r\n") + "\"";

                    var p = Process.Start(new ProcessStartInfo { FileName = tfPath, Arguments = args });
                    p.WaitForExit();

                    var exitCode = p.ExitCode;

                    if (exitCode != 0)
                    {
                        var message = exitCode.ToString();

                        if (exitCode == 100)
                        {
                            message = "Nothing succeeded";
                        }
                        else if (exitCode == 1)
                        {
                            message = "There were no pending changes, or none of the files had been modified and were undone.";
                        }

                        req.Conn.ErrorLine("Submit error: " + message);
                    }

                }
                catch (Exception e)
                {
                    req.Conn.ErrorLine(e.Message);
                }
            }

            task.GetStatus(req.Assets, resp.Assets, false, true);

            resp.Write();
            return true;
        }

        private static string GetTfsPath()
        {
            string toolsFolder = Environment.GetEnvironmentVariable("VS120COMNTOOLS");

            if (string.IsNullOrEmpty(toolsFolder))
            {
                toolsFolder = Environment.GetEnvironmentVariable("VS110COMNTOOLS");

                if (string.IsNullOrEmpty(toolsFolder))
                {
                    toolsFolder = Environment.GetEnvironmentVariable("VS100COMNTOOLS");
                }
            }

            if (string.IsNullOrEmpty(toolsFolder))
            {
                return null;
            }

            return Path.Combine(toolsFolder, "../IDE/tf.exe");
        }
    }
}
