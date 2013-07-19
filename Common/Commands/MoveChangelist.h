#pragma once
#include "Base.h"
#include "VersionedAsset.h"

// Move asset from one changelist to another
class MoveChangelistRequest : public BaseRequest
{
public:
	MoveChangelistRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn)
	{
		assets.clear();
		conn >> changelist;
		conn >> assets;

		if (assets.empty())
		{
			assets.clear();
			conn << assets;
			conn.EndResponse();
			invalid = true;
		}
	}

	VersionedAssetList assets;
	ChangelistRevision changelist;
};


class MoveChangelistResponse : public BaseResponse
{
public:
    MoveChangelistResponse(MoveChangelistRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn << assets;
		conn.EndResponse();
	}

	MoveChangelistRequest& request;
	Connection& conn;
	VersionedAssetList assets;
};
