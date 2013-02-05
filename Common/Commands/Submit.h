#pragma once
#include "Base.h"
#include "VersionedAsset.h"

class SubmitRequest : public BaseRequest
{
public:
	SubmitRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn)
	{
		assets.clear();
		UnityPipe& upipe = conn.Pipe();
		upipe >> changelist;
		upipe >> assets;

		if (assets.empty())
		{
			assets.clear();
			upipe << assets;
			upipe.EndResponse();
			invalid = true;
		}
	}

	VersionedAssetList assets;
	Changelist changelist;
};


class SubmitResponse : public BaseResponse
{
public:
    SubmitResponse(SubmitRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
	
		upipe << assets;
		upipe.EndResponse();
	}

	SubmitRequest& request;
	VersionedAssetList assets;
};

