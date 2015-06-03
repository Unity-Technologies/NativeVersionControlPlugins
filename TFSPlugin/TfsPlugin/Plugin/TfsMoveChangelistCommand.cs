
namespace TfsPlugin
{
    public class TfsMoveChangelistCommand : ICommand<TfsTask, MoveChangelistRequest, MoveChangelistResponse>
    {
        public bool Run(TfsTask task, MoveChangelistRequest req, MoveChangelistResponse resp)
        {
            task.GetStatus(req.Assets, resp.Assets, false, true);
            resp.Write();
            return true;
        }
    }
}
