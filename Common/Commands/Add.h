#pragma once
#include "Command.h"

class AddRequest : public BaseRequest
{
public:
	AddRequest(const CommandArgs& args, Connection& conn) : BaseRequest(args, conn) 
	{
		conn >> assetList;
	}
	
	VersionedAssetList assetList;
};

class AddResponse : public BaseReponse
{
public:
};
