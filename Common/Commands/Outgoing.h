#pragma once
#include "Command.h"
#include "Changes.h"

class OutgoingRequest : public BaseRequest
{
public:
	OutgoingRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
	{
	}	
};

class OutgoingResponse : public BaseResponse
{
public:
	OutgoingResponse(OutgoingRequest& req) : request(req), conn(req.conn) {}

	void AddChangeSet(const std::string& name, const std::string& revision)
	{
		Changelist cl;
		cl.SetDescription(name);
		cl.SetRevision(revision);
		changeSets.push_back(cl);
	}
	
	void Write()
	{
		if (request.invalid)
			return;
		
		conn << changeSets;
		conn.EndResponse();
	}

	OutgoingRequest& request;
	Connection& conn;
	Changes changeSets;
};
