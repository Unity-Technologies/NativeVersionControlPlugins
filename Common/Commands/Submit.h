#pragma once
#include "Base.h"
#include "VersionedAsset.h"

class SubmitRequest : public BaseRequest
{
public:
	SubmitRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn)
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
	Changelist changelist;
};


class SubmitResponse : public BaseResponse
{
public:
    SubmitResponse(SubmitRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn << assets;
		conn.EndResponse();
	}

	SubmitRequest& request;
	Connection& conn;
	VersionedAssetList assets;
};

