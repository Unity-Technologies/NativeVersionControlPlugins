#pragma once
#include "Connection.h"
#include "Commands/AllReqResp.h"

// Helper for Dispatch()
template <template<class> class CommandTmpl, class Req, class Resp, class Sess>
bool RunCommand(Connection& conn, Sess& session, const CommandArgs& args)
{
	Req req(args, conn);

	// Invalid requests will send warnings/error to unity and we can just continue.
	// If something fatal happens an exception is thrown when reading the request.
	if (req.invalid) return true; 

	Resp resp(req);
	return CommandTmpl<Req>().Run(session, req, resp);
}

/** Dispatch command to appropriate Command class
 * 
 * This function will create the correct Request and Response
 * objects for the specified command (cmd) and create a Command
 * object using the CommandTmpl<enum> where enum is cmd.
 * The Run method of the Command is then called using Request and 
 * Response objects.
 *
 * The during creation of the Request object all data for the request
 * is read from the connection.
 *
 * A Command can optionally fill out the Response object and call
 * its Write() method. Alternatively it can just use the response objects
 * conn member to send data directly. This is usefull for streaming data
 * directly to Unity instead buffering the result into the response object
 * first.
 */
template <template<class> class CommandTmpl, class Session>
bool Dispatch(Connection& conn, Session& session, 
			  UnityCommand cmd, const CommandArgs& args)
{
	conn.Log().Info() << Join(args, " ") << Endl;

	switch (cmd)
	{
	case UCOM_Invalid: 
		throw CommandException(cmd, "Cannot dispatch invalid command");
	case UCOM_Shutdown:
		return false;
	case UCOM_Config:
		return RunCommand<CommandTmpl, ConfigRequest, ConfigResponse>(conn, session, args);
	case UCOM_Add:
		return RunCommand<CommandTmpl, AddRequest, AddResponse>(conn, session, args);
	case UCOM_Move:
		return RunCommand<CommandTmpl, MoveRequest, MoveResponse>(conn, session, args);
	case UCOM_ChangeMove:
		return RunCommand<CommandTmpl, MoveChangelistRequest, MoveChangelistResponse>(conn, session, args);
	case UCOM_Checkout:
		return RunCommand<CommandTmpl, CheckoutRequest, CheckoutResponse>(conn, session, args);
	case UCOM_Delete:
		return RunCommand<CommandTmpl, DeleteRequest, DeleteResponse>(conn, session, args);
	case UCOM_GetLatest:
		return RunCommand<CommandTmpl, GetLatestRequest, GetLatestResponse>(conn, session, args);
	case UCOM_Resolve:
		return RunCommand<CommandTmpl, ResolveRequest, ResolveResponse>(conn, session, args);
	case UCOM_Lock:
		return RunCommand<CommandTmpl, LockRequest, LockResponse>(conn, session, args);
	case UCOM_Unlock:
		return RunCommand<CommandTmpl, UnlockRequest, UnlockResponse>(conn, session, args);
	case UCOM_Revert:
		return RunCommand<CommandTmpl, RevertRequest, RevertResponse>(conn, session, args);
	case UCOM_Submit:
		return RunCommand<CommandTmpl, SubmitRequest, SubmitResponse>(conn, session, args);
	case UCOM_Incoming:
		return RunCommand<CommandTmpl, IncomingRequest, IncomingResponse>(conn, session, args);
	case UCOM_IncomingChangeAssets:
		return RunCommand<CommandTmpl, IncomingAssetsRequest, IncomingAssetsResponse>(conn, session, args);
	case UCOM_Changes:
		return RunCommand<CommandTmpl, OutgoingRequest, OutgoingResponse>(conn, session, args);
	case UCOM_ChangeStatus:
		return RunCommand<CommandTmpl, OutgoingAssetsRequest, OutgoingAssetsResponse>(conn, session, args);
	case UCOM_Status:
		return RunCommand<CommandTmpl, StatusRequest, StatusResponse>(conn, session, args);
	case UCOM_Download:
		return RunCommand<CommandTmpl, DownloadRequest, DownloadResponse>(conn, session, args);
	case UCOM_ChangeDescription:
		return RunCommand<CommandTmpl, ChangeDescriptionRequest, ChangeDescriptionResponse>(conn, session, args);
	default:
		break;
	}
	throw CommandException(cmd, "Cannot dispatch unknown command type ");
}
