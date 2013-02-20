#pragma once
#include "UnityConnection.h"

class Task
{
public:
	Task(const std::string& logPath);
	UnityCommand ReadCommand(CommandArgs& args);
	UnityConnection& GetConnection();
	UnityPipe& Pipe();
	unityplugin::LogStream& Log();

protected:
	UnityConnection m_Connection;
};
