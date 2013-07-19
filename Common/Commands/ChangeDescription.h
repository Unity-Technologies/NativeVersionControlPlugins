#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class ChangeDescriptionRequest : public BaseRequest
{
public:
	ChangeDescriptionRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
	{
		conn >> revision;

		if (revision.empty())
		{
			VersionedAssetList assets;
			conn << assets;
			conn.ErrorLine("Cannot get assets for empty revision");
			conn.EndResponse();
			invalid = true;
		}
	}	

	ChangelistRevision revision;
};

class ChangeDescriptionResponse : public BaseResponse
{
public:
	ChangeDescriptionResponse(ChangeDescriptionRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn.DataLine(description);
		conn.EndResponse();
	}

	ChangeDescriptionRequest& request;
	Connection& conn;
	std::string description;
};
