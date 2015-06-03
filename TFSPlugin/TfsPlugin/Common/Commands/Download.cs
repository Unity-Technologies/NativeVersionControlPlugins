
namespace TfsPlugin
{
    class DownloadRequest : BaseRequest
    {
        public DownloadRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            TargetDir = conn.ReadLine();
            conn.Read(Revisions);
            conn.Read(Assets);

            if (Assets.Count == 0 || Revisions.Count == 0)
            {
                Assets.Clear();
                conn.Write(Assets);
                conn.EndResponse();
                Invalid = true;
            }
        }

        // One revisions per asset ie. the two containers are of same size
        public string TargetDir { get; set; }
        ChangelistRevisions revisions = new ChangelistRevisions();
        public ChangelistRevisions Revisions { get { return revisions; } }

        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    };

    class DownloadResponse : BaseResponse
    {

        public DownloadResponse(DownloadRequest req)
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

        DownloadRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    };
}
