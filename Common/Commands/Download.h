#pragma once
#include "Base.h"
#include "VersionedAsset.h"

class DownloadRequest : public BaseRequest
{
public:
	DownloadRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn)
	{
		assets.clear();
		UnityPipe& upipe = conn.Pipe();
		upipe >> targetDir;
		upipe >> revisions;
		upipe >> assets;

		if (assets.empty() || revisions.empty())
		{
			assets.clear();
			upipe << assets;
			upipe.EndResponse();
			invalid = true;
		}
	}

	// One revisions per asset ie. the two containers are of same size
	std::string targetDir;
	ChangelistRevisions revisions;
	VersionedAssetList assets;
};


class DownloadResponse : public BaseResponse
{
public:
    DownloadResponse(DownloadRequest& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
	
		upipe << assets;
		upipe.EndResponse();
	}

	DownloadRequest& request;
	VersionedAssetList assets;
};
