
namespace TfsPlugin
{
    /*
     * Returns the Changelists that are pending to be submitted to the server.
     * The assets associated with the changelists are not included.
     */
    class TfsOutgoingCommand : ICommand<TfsTask, OutgoingRequest, OutgoingResponse>
    {
        public const string DefaultChangelist = "Pending Changes";

        public bool Run(TfsTask task, OutgoingRequest req, OutgoingResponse resp)
        {
            var pending = task.Workspace.GetPendingChanges();

            if (pending.Length != 0)
            {
                Changelist cl = new Changelist();

                cl.Description = DefaultChangelist;
                cl.Revision = ChangelistRevision.kDefaultListRevision;
                resp.ChangeSets.Add(cl);
            }

            resp.Write();
            return true;
        }
    };
}
