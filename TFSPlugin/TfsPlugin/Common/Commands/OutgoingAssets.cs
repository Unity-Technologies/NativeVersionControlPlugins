
namespace TfsPlugin
{
    public class OutgoingAssetsRequest : BaseRequest
    {

        public OutgoingAssetsRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            Revision = conn.Read<ChangelistRevision>();

            if (string.IsNullOrEmpty(Revision))
            {
                VersionedAssetList assets = new VersionedAssetList();
                conn.Write(assets);
                conn.ErrorLine("Cannot get assets for empty revision");
                conn.EndResponse();
                Invalid = true;
            }
        }

        public ChangelistRevision Revision { get; set; }
    }

    public class OutgoingAssetsResponse : BaseResponse
    {

        public OutgoingAssetsResponse(OutgoingAssetsRequest req)
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

        OutgoingAssetsRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    }

}
