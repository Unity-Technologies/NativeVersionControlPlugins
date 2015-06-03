using Microsoft.TeamFoundation.VersionControl.Client;
using System.Linq;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{

    public class TfsGetLatestCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            var requests = req.Assets.Select(each => new GetRequest(new ItemSpec(each.GetPath(), each.IsFolder() ? RecursionType.Full : RecursionType.None), VersionSpec.Latest)).ToArray();
            task.Workspace.Get(requests, GetOptions.None);

            task.GetStatus(req.Assets, resp.Assets, true, true);

            resp.Write();
            return true;
        }
    }
}
