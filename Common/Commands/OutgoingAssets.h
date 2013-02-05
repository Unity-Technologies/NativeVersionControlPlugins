#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class OutgoingAssetsRequest : public BaseRequest
{
public:
	OutgoingAssetsRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn) 
	{
		UnityPipe& upipe = conn.Pipe();
		upipe >> revision;

		if (revision.empty())
		{
			VersionedAssetList assets;
			upipe << assets;
			upipe.ErrorLine("Cannot get assets for empty revision");
			upipe.EndResponse();
			invalid = true;
		}
	}	

	ChangelistRevision revision;
};

class OutgoingAssetsResponse : public BaseResponse
{
public:
	OutgoingAssetsResponse(OutgoingAssetsRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
		upipe << assets;
		upipe.EndResponse();
	}

	OutgoingAssetsRequest& request;
	VersionedAssetList assets;
};
