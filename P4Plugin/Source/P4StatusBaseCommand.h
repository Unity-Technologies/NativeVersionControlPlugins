#pragma once
#include "P4Command.h"
class VersionedAsset;

// Base for status commands
class P4StatusBaseCommand : public P4Command
{
public:
	P4StatusBaseCommand(const char* name, bool streamResultToConnection = true);
	
	// Called once per file for status commands
	virtual void OutputStat( StrDict *varList );
	virtual void HandleError( Error *err );
	bool AddUnknown(VersionedAsset& current, const std::string& value);	
protected:
	bool m_StreamResultToConnection;
	VersionedAssetList m_StatusResult;
};
