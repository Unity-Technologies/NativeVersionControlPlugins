#pragma once
#include "Command.h"

class AddRequest : public BaseRequest
{
public:
	AddRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn) 
	{
		conn.Pipe() >> assetList;
	}
	
	VersionedAssetList assetList;
};

class AddResponse : public BaseReponse
{
public:
};
