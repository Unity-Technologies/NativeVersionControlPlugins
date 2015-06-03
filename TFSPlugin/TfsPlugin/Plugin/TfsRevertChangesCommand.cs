using Microsoft.TeamFoundation.VersionControl.Client;
using System.Linq;

namespace TfsPlugin
{
    class TfsRevertChangesCommand : ICommand<TfsTask, RevertChangesRequest, RevertChangesResponse>
    {
        public bool Run(TfsTask task, RevertChangesRequest req, RevertChangesResponse resp)
        {
            bool unchangedOnly = req.Args.Count > 1 && req.Args[1] == "unchangedOnly";

            bool keepLocalModifications = req.Args.Count > 1 && req.Args[1] == "keepLocalModifications";

            foreach (var item in req.Revisions)
            {
                if (item.Value == ChangelistRevision.kDefaultListRevision)
                {
                    var pending = task.Workspace.GetPendingChanges(new[] { new ItemSpec(task.ProjectPath, RecursionType.Full) });

                    var itemSpecs = pending.Select(each => new ItemSpec(each.LocalItem, RecursionType.None)).ToArray();

                    TfsRevertCommand.RevertCore(task, unchangedOnly, keepLocalModifications, itemSpecs);

                    var versionedAssets = new VersionedAssetList();
                    versionedAssets.AddRange(pending.Select(each => new VersionedAsset(TfsTask.GetAssetPath(each, task.Workspace, true))));

                    task.GetStatus(versionedAssets, resp.Assets, false, true);
                }
            }

            resp.Write();
            return true;
        }
    }
}
