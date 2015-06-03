
namespace TfsPlugin
{
    public class BaseRequest
    {
        public BaseRequest(CommandArgs args, Connection conn)
        {
            this.Args = args;
            this.Conn = conn;
            this.Invalid = false;
        }

        public CommandArgs Args { get; set; }
        public Connection Conn { get; set; }
        public bool Invalid { get; set; }
    }

    public class BaseResponse
    {
        public virtual void Write()
        {
        }
    }
}
