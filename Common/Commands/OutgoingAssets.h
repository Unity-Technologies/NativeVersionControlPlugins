#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class OutgoingAssetsRequest : public BaseRequest
{
public:
	OutgoingAssetsRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
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

class OutgoingAssetsResponse : public BaseResponse
{
public:
	OutgoingAssetsResponse(OutgoingAssetsRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn << assets;
		conn.EndResponse();
	}

	OutgoingAssetsRequest& request;
	Connection& conn;
	VersionedAssetList assets;
};
