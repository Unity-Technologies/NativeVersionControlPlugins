
namespace TfsPlugin
{
    class TfsIncomingAssetsCommand : ICommand<TfsTask, IncomingAssetsRequest, IncomingAssetsResponse>
    {
        public bool Run(TfsTask task, IncomingAssetsRequest req, IncomingAssetsResponse resp)
        {
            var changes = task.GetChangesetWithChanges(req.Revision.ToInt());

            task.GetStatus(changes.Changes, resp.Assets, false, true);

            resp.Assets.RemoveAll(each => each.HasState(State.kSynced));

            resp.Write();
            return true;
        }
    }
}
