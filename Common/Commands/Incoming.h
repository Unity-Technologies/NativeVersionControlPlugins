#pragma once
#include "Command.h"
#include "Changes.h"

class IncomingRequest : public BaseRequest
{
public:
	IncomingRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn) 
	{
	}	
};

class IncomingResponse : public BaseResponse
{
public:
	IncomingResponse(IncomingRequest& req) : request(req) {}

	void AddChangeSet(const Changelist& cl)
	{
		changeSets.push_back(cl);
	}
	
	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
		upipe << changeSets;
		upipe.EndResponse();
	}

	IncomingRequest& request;
	Changes changeSets;
};

