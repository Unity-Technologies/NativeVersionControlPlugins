#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class IncomingAssetsRequest : public BaseRequest
{
public:
	IncomingAssetsRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn) 
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

class IncomingAssetsResponse : public BaseResponse
{
public:
	IncomingAssetsResponse(IncomingAssetsRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
		upipe << assets;
		upipe.EndResponse();
	}

	IncomingAssetsRequest& request;
	VersionedAssetList assets;
};

