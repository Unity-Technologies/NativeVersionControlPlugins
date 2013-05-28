#pragma once
#include "Command.h"
#include "Changes.h"
#include "VersionedAsset.h"

class ChangeDescriptionRequest : public BaseRequest
{
public:
	ChangeDescriptionRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn) 
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

class ChangeDescriptionResponse : public BaseResponse
{
public:
	ChangeDescriptionResponse(ChangeDescriptionRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
		upipe.DataLine(description);
		upipe.EndResponse();
	}

	ChangeDescriptionRequest& request;
	std::string description;
};
