using Microsoft.TeamFoundation.VersionControl.Client;

namespace TfsPlugin
{
    public class TfsChangeStatusCommand : ICommand<TfsTask, OutgoingAssetsRequest, OutgoingAssetsResponse>
    {
        public bool Run(TfsTask task, OutgoingAssetsRequest req, OutgoingAssetsResponse resp)
        {
            var pending = task.Workspace.GetPendingChanges(task.ProjectPath, RecursionType.Full);
            var pendingChange = task.ToVersionedAssetList(pending, false);

            task.GetStatus(pendingChange, resp.Assets, false, true);

            resp.Write();
            return true;
        }
    }
}
