
namespace TfsPlugin
{
    public class BaseFileSetRequest : BaseRequest
    {
        public BaseFileSetRequest()
            : base(null, null)
        {

        }

        public BaseFileSetRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            Assets.Clear();
            conn.Read(Assets);

            if (Assets.Count == 0)
            {
                Assets.Clear();
                conn.Write(Assets);
                conn.EndResponse();
                Invalid = true;
            }
        }

        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    }

    public class BaseFileSetResponse<Req> : BaseResponse where Req : BaseFileSetRequest
    {

        public BaseFileSetResponse(Req req)
        {
            this.request = req;
            this.conn = req.Conn;
        }

        public override void Write()
        {
            if (request.Invalid)
                return;

            conn.Write(Assets);
            conn.EndResponse();
        }

        Req request;
        Connection conn;

        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    }
}
