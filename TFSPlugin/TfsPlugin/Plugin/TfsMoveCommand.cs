using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{
    class TfsMoveCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            VersionedAssetList statAssets = new VersionedAssetList();

            try
            {
                for (int i = 0; i < req.Assets.Count; i++)
                {
                    var firstAsset = req.Assets[i];
                    var srcPath = firstAsset.GetPath();

                    ++i;
                    if (i >= req.Assets.Count)
                    {
                        req.Conn.ErrorLine("No destination path while moving source path " + srcPath);
                        break;
                    }

                    var dstPath = req.Assets[i].GetPath();

                    task.Workspace.PendRename(srcPath, dstPath, task.ShouldLock(firstAsset) ? task.GetLockLevel() : LockLevel.None, true, true);

                    statAssets.Add(req.Assets[i]);
                }
            }
            catch (Exception e)
            {
                req.Conn.ErrorLine(e.Message);
            }

            task.GetStatus(statAssets, resp.Assets, false, true);

            resp.Write();
            return true;
        }
    }
}
