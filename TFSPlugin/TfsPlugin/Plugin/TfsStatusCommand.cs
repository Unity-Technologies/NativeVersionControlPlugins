using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace TfsPlugin
{
    public class TfsStatusCommand : ICommand<TfsTask, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>>
    {
        const int BatchSize = 600;

        public bool Run(TfsTask task, BaseFileSetRequest req, BaseFileSetResponse<BaseFileSetRequest> resp)
        {
            bool recursive = req.Args.Count > 1 && req.Args[1] == "recursive";
            bool offline = req.Args.Count > 2 && req.Args[2] == "offline";

            if (req.Assets.Count <= BatchSize)
            {
                task.GetStatus(req.Assets, resp.Assets, recursive, !offline);
            }
            else
            {
                List<Task> tasks = new List<Task>();
                List<BaseFileSetResponse<BaseFileSetRequest>> responses = new List<BaseFileSetResponse<BaseFileSetRequest>>();

                int pages = (req.Assets.Count / BatchSize);

                if (req.Assets.Count % BatchSize != 0)
                {
                    pages += 1;
                }

                for (int i = 0; i < pages; i++)
                {
                    var batchedRequest = new BaseFileSetRequest();
                    batchedRequest.Conn = req.Conn;
                    batchedRequest.Args = req.Args;
                    batchedRequest.Assets.AddRange(req.Assets.Skip(i * BatchSize).Take(BatchSize));

                    var batchedResponse = new BaseFileSetResponse<BaseFileSetRequest>(batchedRequest);
                    responses.Add(batchedResponse);

                    tasks.Add(Task.Run(() =>
                    {
                        task.GetStatus(batchedRequest.Assets, batchedResponse.Assets, recursive, !offline);
                    }));
                }

                Task.WaitAll(tasks.ToArray());

                foreach (var item in responses)
                {
                    resp.Assets.AddRange(item.Assets);
                }
            }

            resp.Write();
            return true;
        }
    }
}
