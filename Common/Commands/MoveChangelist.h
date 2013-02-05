#pragma once
#include "Base.h"
#include "VersionedAsset.h"

// Move asset from one changelist to another
class MoveChangelistRequest : public BaseRequest
{
public:
	MoveChangelistRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn)
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
	ChangelistRevision changelist;
};


class MoveChangelistResponse : public BaseResponse
{
public:
    MoveChangelistResponse(MoveChangelistRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
	
		upipe << assets;
		upipe.EndResponse();
	}

	MoveChangelistRequest& request;
	VersionedAssetList assets;
};
