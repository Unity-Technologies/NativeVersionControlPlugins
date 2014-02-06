#pragma once
#include "P4StatusBaseCommand.h"

class P4StatusCommand : public P4StatusBaseCommand
{
public:
	P4StatusCommand(const char* name);
	virtual bool Run(P4Task& task, const CommandArgs& args);
	void RunAndSend(P4Task& task, const VersionedAssetList& assets, bool recursive);
	void Run(P4Task& task, const VersionedAssetList& assetList, bool recursive, VersionedAssetList& result);
private:
	void PreStatus();
	void PostStatus();
	bool m_WasOnline;
};
