#pragma once
#include "Base.h"
#include "VersionedAsset.h"

class DownloadRequest : public BaseRequest
{
public:
	DownloadRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn)
	{
		assets.clear();
		conn >> targetDir;
		conn >> revisions;
		conn >> assets;

		if (assets.empty() || revisions.empty())
		{
			assets.clear();
			conn << assets;
			conn.EndResponse();
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
    DownloadResponse(DownloadRequest& req) : request(req), conn(req.conn) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		conn << assets;
		conn.EndResponse();
	}

	DownloadRequest& request;
	Connection& conn;
	VersionedAssetList assets;
};
