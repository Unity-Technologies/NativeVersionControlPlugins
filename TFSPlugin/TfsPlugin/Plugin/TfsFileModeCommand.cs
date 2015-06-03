using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{
    public class TfsFileModeCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            task.GetStatus(req.Assets, resp.Assets, false, true);

            resp.Write();
            return true;
        }
    }
}
