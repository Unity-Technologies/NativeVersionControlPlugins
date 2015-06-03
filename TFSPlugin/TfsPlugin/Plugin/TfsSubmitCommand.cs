using System;

namespace TfsPlugin
{
    class TfsSubmitCommand : ICommand<TfsTask, SubmitRequest, SubmitResponse>
    {
        public bool Run(TfsTask task, SubmitRequest req, SubmitResponse resp)
        {
            bool saveOnly = req.Args.Count > 1 && req.Args[1] == "saveOnly";

            if (saveOnly)
            {
                return true;
            }

            if (req.Assets.Count == 0)
            {
                return true;
            }

            req.Conn.Progress(-1, 0, "submitting");

            try
            {
                task.Workspace.CheckIn(task.GetProjectPendingChanges(req.Assets), req.Changelist.GetDescription());
            }
            catch (Exception e)
            {
                req.Conn.ErrorLine("Unable to Submit " + e.Message);
            }

            task.GetStatus(req.Assets, resp.Assets, false, true);

            resp.Write();
            return true;
        }
    }
}
