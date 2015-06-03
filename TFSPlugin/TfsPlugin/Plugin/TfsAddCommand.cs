using System;
using System.IO;
using System.Linq;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{
    public class TfsAddCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            var toAdd = req.Assets.Where(each => task.IsIgnored(each) == false).Where(each => each.ExistsOnDisk);

            // if anything has a deleted local tag, unity has replaced the file, we need to undo the delete and check it out instead of adding it.
            var toUndoAndThenCheckout = toAdd.Where(each => each.HasState(State.kDeletedLocal)).ToArray();

            RevertAndCheckout(task, req, toUndoAndThenCheckout);

            var toAddAry = toAdd.Where(each => !each.HasState(State.kDeletedLocal)).Select(each => each.GetPath()).ToArray();

            try
            {
                if (toAddAry.Length > 0)
                {
                    task.Workspace.PendAdd(toAddAry);
                }

            }
            catch (Exception e)
            {
                req.Conn.WarnLine(e.Message);
            }

            task.GetStatus(req.Assets, resp.Assets, false, true);


            resp.Write();
            return true;
        }

        /// <summary>
        /// When Unity replaces a file, it first deletes it, then calls to add it. 
        /// files that are already checked in need to take a special case path of a revert then a checkout
        /// </summary>
        /// <param name="task"></param>
        /// <param name="req"></param>
        /// <param name="toUndoAndThenCheckout"></param>
        public static void RevertAndCheckout(TfsTask task, Request req, VersionedAsset[] toUndoAndThenCheckout)
        {
            const string tmpFileExt = ".tfsplugintemp";

            if (toUndoAndThenCheckout.Length > 0)
            {
                try
                {
                    var toUndoAndThenCheckoutAry = toUndoAndThenCheckout.Select(each => each.GetPath()).ToArray();

                    // shuffle the files around to work around tfs api calls
                    foreach (var item in toUndoAndThenCheckout)
                    {
                        File.Delete(item.Path + tmpFileExt);
                        File.Move(item.Path, item.Path + tmpFileExt);
                    }

                    task.Workspace.Undo(toUndoAndThenCheckoutAry, Microsoft.TeamFoundation.VersionControl.Client.RecursionType.None);

                    foreach (var item in toUndoAndThenCheckout)
                    {
                        item.MakeWritable();
                        File.Replace(item.Path + tmpFileExt, item.Path, null);
                        item.MakeReadonly();
                    }

                    TfsCheckoutCommand.PendEdit(task, req.Conn, new VersionedAssetList(toUndoAndThenCheckout));
                }
                catch (Exception e)
                {
                    if (req != null)
                    {
                        req.Conn.WarnLine(e.Message);
                    }
                }
            }
        }
    }
}
