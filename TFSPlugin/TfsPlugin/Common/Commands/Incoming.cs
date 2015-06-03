using System.Collections.Generic;

namespace TfsPlugin
{
    public class IncomingRequest : BaseRequest
    {

        public IncomingRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
        }
    }

    public class IncomingResponse : BaseResponse
    {

        public IncomingResponse(IncomingRequest req)
        {
            this.request = req;
            this.conn = req.Conn;
        }

        public override void Write()
        {
            if (request.Invalid)
                return;

            conn.Write(changeSets);
            conn.EndResponse();
        }

        IncomingRequest request;
        Connection conn;
        List<Changelist> changeSets = new List<Changelist>();
        public List<Changelist> ChangeSets { get { return changeSets; } }
    }

}
