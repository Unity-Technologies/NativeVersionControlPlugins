using Microsoft.TeamFoundation.VersionControl.Client;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{

    class TfsRevertCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            bool unchangedOnly = req.Args.Count > 1 && req.Args[1] == "unchangedOnly";

            bool keepLocalModifications = req.Args.Count > 1 && req.Args[1] == "keepLocalModifications";

            var files = req.Assets.Select(each => new ItemSpec(each.GetPath(), each.IsFolder() ? RecursionType.Full : RecursionType.None)).ToArray();

            RevertCore(task, unchangedOnly, keepLocalModifications, files);

            task.GetStatus(req.Assets, resp.Assets, false, true);

            resp.Write();
            return true;
        }

        public static void RevertCore(TfsTask task, bool unchangedOnly, bool keepLocalModifications, ItemSpec[] files)
        {
            if (files.Length == 0)
            {
                return;
            }

            if (unchangedOnly)
            {
                var localItems = task.Workspace.QueryPendingSets(files, null, null, false).SelectMany(each => each.PendingChanges).ToList();

                RevertUnchanged(task, files, localItems);
            }
            else
            {
                task.Workspace.Undo(files, !keepLocalModifications);

                if (task.IsPartiallyOffline)
                {
                    try
                    {
                        var offlineFiles = files.Where(each => each.RecursionType == RecursionType.None && File.Exists(each.Item) && ((File.GetAttributes(each.Item) & FileAttributes.ReadOnly) != FileAttributes.ReadOnly));
                        var toForceGet = offlineFiles.Select(each => new GetRequest(new ItemSpec(each.Item, RecursionType.None), VersionSpec.Latest)).ToArray();

                        if (toForceGet.Length != 0)
                        {
                            task.Workspace.Get(toForceGet, GetOptions.Overwrite| GetOptions.GetAll);
                        }
                    }
                    catch
                    {
                    }
                }
            }
        }

        public static void RevertUnchanged(TfsTask task, ItemSpec[] files, List<PendingChange> localItems)
        {
            if (localItems.Count != 0)
            {
                var serverItems = task.VersionControlServer.GetItems(files, VersionSpec.Latest, DeletedState.Any, ItemType.File).SelectMany(each => each.Items);

                HashSet<PendingChange> toUndo = new HashSet<PendingChange>();

                foreach (var item in localItems)
                {
                    // skip changes that dont have local path like deletes
                    if (item.LocalItem == null)
                    {
                        continue;
                    }

                    var found = serverItems.FirstOrDefault(each => each.ServerItem == item.ServerItem);

                    if (found != null && File.Exists(item.LocalItem))
                    {
                        byte[] localHash = null;

                        using (var md5 = MD5.Create())
                        {
                            using (var stream = File.OpenRead(item.LocalItem))
                            {
                                localHash = md5.ComputeHash(stream);
                            }
                        }

                        if (localHash != null && Enumerable.SequenceEqual(found.HashValue, localHash))
                        {
                            toUndo.Add(item);
                        }
                    }
                }

                if (toUndo.Count != 0)
                {
                    task.Workspace.Undo(toUndo.ToArray());
                }
            }
        }
    }
}
