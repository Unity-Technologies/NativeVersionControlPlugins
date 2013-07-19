#pragma once
#include "Command.h"
#include "Changes.h"

class IncomingRequest : public BaseRequest
{
public:
	IncomingRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
	{
	}	
};

class IncomingResponse : public BaseResponse
{
public:
	IncomingResponse(IncomingRequest& req) : request(req), conn(req.conn) {}

	void AddChangeSet(const Changelist& cl)
	{
		changeSets.push_back(cl);
	}
	
	void Write()
	{
		if (request.invalid)
			return;
		
		conn << changeSets;
		conn.EndResponse();
	}

	IncomingRequest& request;
	Connection& conn;
	Changes changeSets;
};

