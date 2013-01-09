#pragma once
#include "P4Command.h"

class P4Task;

class P4FileSetBaseCommand : public P4Command
{
public:
	P4FileSetBaseCommand(const char* name, const char* cmdstr = "");
	virtual bool Run(P4Task& task, const CommandArgs& args);
	virtual std::string SetupCommand(const CommandArgs& args);	
	virtual int GetResolvePathFlags() const;
private:
	const std::string m_CmdStr;
};

