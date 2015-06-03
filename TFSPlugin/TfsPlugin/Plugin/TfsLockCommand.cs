using System;
using System.Linq;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{

    class TfsLockCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            var files = req.Assets.Select(each => each.GetPath()).ToArray();
            var filesPass2 = TfsTask.GetFolderContentsWildcardPaths(files).ToArray();

            try
            {
                if (files.Length > 0)
                {
                    task.Workspace.SetLock(files, task.GetLockLevel());
                }
                if (filesPass2.Length > 0)
                {
                    task.Workspace.SetLock(filesPass2, task.GetLockLevel());
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

            foreach (var item in resp.Assets)
            {
                if (!item.HasState(State.kLockedLocal))
                {
                    req.Conn.ErrorLine("Unable to lock " + item.GetPath());
                }
            }

            resp.Write();
            return true;
        }
    }
}
