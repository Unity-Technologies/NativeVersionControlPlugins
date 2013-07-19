#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class IncomingAssetsRequest : public BaseRequest
{
public:
	IncomingAssetsRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
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

class IncomingAssetsResponse : public BaseResponse
{
public:
	IncomingAssetsResponse(IncomingAssetsRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn << assets;
		conn.EndResponse();
	}

	IncomingAssetsRequest& request;
	Connection& conn;
	VersionedAssetList assets;
};

