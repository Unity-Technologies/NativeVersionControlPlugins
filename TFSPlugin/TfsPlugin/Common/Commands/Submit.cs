
namespace TfsPlugin
{
    public class SubmitRequest : BaseRequest
    {
        public SubmitRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            Changelist = conn.Read<Changelist>();
            conn.Read(Assets);

            if (Assets.Count == 0)
            {
                conn.Write(Assets);
                conn.EndResponse();
                Invalid = true;
            }
        }

        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
        public Changelist Changelist { get; set; }
    };


    class SubmitResponse : BaseResponse
    {

        public SubmitResponse(SubmitRequest req)
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

        SubmitRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    };


}
