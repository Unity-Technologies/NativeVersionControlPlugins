using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Linq;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{

    class TfsUnlockCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            var files = req.Assets.Select(each => each.GetPath()).ToArray();
            var filesPass2 = TfsTask.GetFolderContentsWildcardPaths(files).ToArray();

            try
            {
                if (files.Length > 0)
                {
                    task.Workspace.SetLock(files, LockLevel.None);
                }
                if (filesPass2.Length > 0)
                {
                    task.Workspace.SetLock(filesPass2, LockLevel.None);
                }
            }
            catch (Exception e)
            {
                req.Conn.ErrorLine(e.Message);
            }

            if (files.Length > 0)
            {
                task.GetStatus(files, resp.Assets, false, true);
            }
            if (filesPass2.Length > 0)
            {
                task.GetStatus(filesPass2, resp.Assets, false, true);
            }

            resp.Write();
            return true;
        }
    }
}
