using System.Collections.Generic;

namespace TfsPlugin
{
    public class OutgoingRequest : BaseRequest
    {

        public OutgoingRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
        }
    };

    public class OutgoingResponse : BaseResponse
    {

        public OutgoingResponse(OutgoingRequest req)
        {
            this.request = req;
            this.conn = req.Conn;
        }

        public void AddChangeSet(string name, string revision)
        {
            Changelist cl = new Changelist();
            cl.SetDescription(name);
            cl.SetRevision(revision);
            ChangeSets.Add(cl);
        }

        public override void Write()
        {
            if (request.Invalid)
                return;

            conn.Write(ChangeSets);
            conn.EndResponse();
        }

        OutgoingRequest request;
        Connection conn;
        List<Changelist> changeSets = new List<Changelist>();
        public List<Changelist> ChangeSets { get { return changeSets; } }
    }

}
