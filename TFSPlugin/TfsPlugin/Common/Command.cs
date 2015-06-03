using System;
using System.Collections.Generic;

namespace TfsPlugin
{
    public enum UnityCommand
    {
        UCOM_Invalid,
        UCOM_Shutdown,
        UCOM_Add,
        UCOM_ChangeDescription,
        UCOM_ChangeMove,
        UCOM_ChangeStatus,
        UCOM_Changes,
        UCOM_Checkout,
        UCOM_Config,
        UCOM_DeleteChanges,
        UCOM_Delete,
        UCOM_Download,
        UCOM_Exit,
        UCOM_GetLatest,
        UCOM_IncomingChangeAssets,
        UCOM_Incoming,
        UCOM_Lock,
        UCOM_Login,
        UCOM_Move,
        UCOM_QueryConfigParameters,
        UCOM_Resolve,
        UCOM_RevertChanges,
        UCOM_Revert,
        UCOM_Status,
        UCOM_Submit,
        UCOM_Unlock,
        UCOM_FileMode,
        UCOM_CustomCommand,
    };


    public class CommandArgs : List<string>
    {

    }

    public interface ICommand<Sess, Req, Resp>
        where Req : BaseRequest
        where Resp : BaseResponse
    {
        bool Run(Sess task, Req req, Resp resp);
    }

    [Serializable]
    public class CommandException : Exception
    {
        public CommandException() { }
        public CommandException(string message) : base(message) { }
        public CommandException(string message, Exception inner) : base(message, inner) { }
        protected CommandException(
          System.Runtime.Serialization.SerializationInfo info,
          System.Runtime.Serialization.StreamingContext context)
            : base(info, context) { }
    }
}
