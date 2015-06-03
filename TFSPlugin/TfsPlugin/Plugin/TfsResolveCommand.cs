using Microsoft.TeamFoundation.VersionControl.Client;
using Request = TfsPlugin.BaseFileSetRequest;
using Response = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;

namespace TfsPlugin
{
    public class TfsResolveCommand : ICommand<TfsTask, Request, Response>
    {
        public bool Run(TfsTask task, Request req, Response resp)
        {
            Resolution resolution = Resolution.AcceptTheirs;

            if (req.Args.Count == 1)
                resolution = Resolution.AcceptTheirs;
            else if (req.Args[1] == "mine")
                resolution = Resolution.AcceptYours;
            else if (req.Args[1] == "theirs")
                resolution = Resolution.AcceptTheirs;
            else if (req.Args[1] == "merged")
                resolution = Resolution.AcceptMerge;

            Resolve(task, resolution, req.Assets);

            task.GetStatus(req.Assets, resp.Assets, false, true);

            resp.Write();
            return true;
        }

        public static void Resolve(TfsTask task, Resolution resolution, VersionedAssetList assets)
        {
            Conflict[] conflicts = task.GetConflicts(assets);

            foreach (var item in conflicts)
            {
                if (resolution == Resolution.AcceptMerge)
                {
                    item.Resolution = Resolution.AcceptYours;
                    task.Workspace.ResolveConflict(item);
                }
                else
                {
                    Conflict[] resolved;
                    item.Resolution = resolution;

                    task.Workspace.ResolveConflict(item, out resolved);

                    if (resolution == Resolution.AcceptTheirs && resolved.Length == 0)
                    {
                        item.Resolution = Resolution.OverwriteLocal;
                        task.Workspace.ResolveConflict(item, out resolved);
                    }
                }
            }
        }
    }
}
