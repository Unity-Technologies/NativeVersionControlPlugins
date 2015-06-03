using Microsoft.TeamFoundation.VersionControl.Client;
using System.Linq;

namespace TfsPlugin
{
    /*
     * Returns the Changelists that are on the server but not locally.
     * The changelists does not include the assets. To get that use the 
     * IncomingChangeAssetsCommand.
     */
    public class TfsIncomingCommand : ICommand<TfsTask, IncomingRequest, IncomingResponse>
    {
        public bool Run(TfsTask task, IncomingRequest req, IncomingResponse resp)
        {
            var workspaceVersion = task.VersionControlServer.QueryHistory(new QueryHistoryParameters(task.ProjectPath, RecursionType.Full)
            {
                VersionEnd = new WorkspaceVersionSpec(task.Workspace),
                MaxResults = 1
            }).Cast<Changeset>().ToArray();

            if (workspaceVersion.Length != 0)
            {
                var incomingChanges = task.VersionControlServer.QueryHistory(new QueryHistoryParameters(task.ProjectPath, RecursionType.Full)
                {
                    VersionStart = new ChangesetVersionSpec(workspaceVersion[0].ChangesetId),
                    VersionEnd = VersionSpec.Latest
                }).Cast<Changeset>().ToList();

                // remove the last change because it will be the workspace version
                if (incomingChanges.Count != 0 && incomingChanges[incomingChanges.Count - 1].ChangesetId == workspaceVersion[0].ChangesetId)
                {
                    incomingChanges.RemoveAt(incomingChanges.Count - 1);
                }

                if (incomingChanges.Count != 0)
                {
                    resp.ChangeSets.AddRange(incomingChanges.Select(each => TfsTask.ConvertToChangelist(each)));
                }
            }

            resp.Write();
            return true;
        }
    }
}
