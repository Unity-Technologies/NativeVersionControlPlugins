
namespace TfsPlugin
{
    public class ChangeDescriptionRequest : BaseRequest
    {

        public ChangeDescriptionRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            Revision = conn.Read<ChangelistRevision>();

            if (string.IsNullOrEmpty(Revision))
            {
                conn.DataLine("unknown");
                conn.ErrorLine("Cannot get description for empty revision");
                conn.EndResponse();
                Invalid = true;
            }
        }

        public ChangelistRevision Revision { get; set; }
    };

    public class ChangeDescriptionResponse : BaseResponse
    {

        public ChangeDescriptionResponse(ChangeDescriptionRequest req)
        {
            request = req;
            conn = req.Conn;
        }

        public override void Write()
        {
            if (request.Invalid)
                return;

            conn.DataLine(Description);
            conn.EndResponse();
        }

        ChangeDescriptionRequest request;
        Connection conn;
        public string Description { get; set; }
    };

}
