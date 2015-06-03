using System.Collections.Generic;

namespace TfsPlugin
{
    class TfsChangeDescriptionCommand : ICommand<TfsTask, ChangeDescriptionRequest, ChangeDescriptionResponse>
    {
        public bool Run(TfsTask task, ChangeDescriptionRequest req, ChangeDescriptionResponse resp)
        {
            List<ChangesetData> found = null;

            if (req.Revision == ChangelistRevision.kDefaultListRevision || req.Revision == ChangelistRevision.kNewListRevision)
            {
                resp.Description = string.Empty;
            }
            else
            {
                found = task.GetChangesets(new HashSet<int> { req.Revision.ToInt() });
            }

            if (found != null && found.Count != 0)
            {
                resp.Description = found[0].Comment;
            }

            resp.Write();
            return true;
        }
    }
}
