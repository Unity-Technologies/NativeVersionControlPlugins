
namespace TfsPlugin
{
    class IncomingAssetsRequest : BaseRequest
    {
        public IncomingAssetsRequest(CommandArgs args, Connection conn)
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
    };

    class IncomingAssetsResponse : BaseResponse
    {
        public IncomingAssetsResponse(IncomingAssetsRequest req)
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

        IncomingAssetsRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    };
}
