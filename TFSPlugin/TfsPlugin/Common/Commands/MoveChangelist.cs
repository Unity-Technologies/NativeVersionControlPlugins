
namespace TfsPlugin
{
    public class MoveChangelistRequest : BaseRequest
    {
        public MoveChangelistRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {

            Changelist = conn.Read<ChangelistRevision>();
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
        public ChangelistRevision Changelist { get; set; }
    };

    public class MoveChangelistResponse : BaseResponse
    {
        public MoveChangelistResponse(MoveChangelistRequest req)
        {
            request = req;
            conn = req.Conn;
        }

        public override void Write()
        {
            if (request.Invalid)
                return;

            conn.Write(Assets);
            conn.EndResponse();
        }

        MoveChangelistRequest request;
        Connection conn;
        VersionedAssetList assets = new VersionedAssetList();
        public VersionedAssetList Assets { get { return assets; } }
    };

}
