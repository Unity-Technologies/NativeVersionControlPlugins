
namespace TfsPlugin
{
    public class RevertChangesRequest : BaseRequest
    {
        public RevertChangesRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            conn.Read(Revisions);

            if (Revisions.Count == 0)
            {
                conn.WarnLine("Changes to revert is empty", MessageArea.MARemote);
                conn.EndResponse();
                Invalid = true;
            }
        }

        public ChangelistRevisions Revisions = new ChangelistRevisions();
    }

    public class RevertChangesResponse : BaseResponse
    {

        public RevertChangesResponse(RevertChangesRequest req)
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

        RevertChangesRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    }
}
