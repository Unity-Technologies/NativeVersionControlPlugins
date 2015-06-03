using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Linq;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{

    public class TfsDeleteCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            var toDelete = req.Assets.Select(each => each.GetPath()).ToArray();

            try
            {
                string[] toLock;
                string[] notToLock;
                task.SplitByLockPolicy(req.Assets, out toLock, out notToLock);

                var toDeleteWithPendingChanges = task.Workspace.GetPendingChanges(toDelete);

                if (toDeleteWithPendingChanges.Length != 0)
                {
                    task.Workspace.Undo(toDeleteWithPendingChanges);
                }

                if (toLock != null && toLock.Length != 0)
                {
                    task.Workspace.PendDelete(toLock, RecursionType.None, task.GetLockLevel());
                }

                if (notToLock != null && notToLock.Length != 0)
                {
                    task.Workspace.PendDelete(notToLock, RecursionType.None, LockLevel.None);
                }

            }
            catch (Exception e)
            {
                req.Conn.WarnLine(e.Message);
            }

            task.GetStatus(req.Assets, resp.Assets, false, true);

            task.WarnAboutFailureToPendChange(resp.Assets.Where(item => !item.HasState(State.kLocal) && !item.HasState(State.kDeletedLocal)));
            task.WarnAboutBinaryFilesCheckedOutByOthers(resp.Assets.Where(item => !(!item.HasState(State.kLocal) && !item.HasState(State.kDeletedLocal))));

            resp.Write();
            return true;
        }
    }
}
