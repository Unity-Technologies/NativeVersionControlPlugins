#pragma once
#include "UnityConnection.h"

class Task
{
public:
	Task(const std::string& logPath);
	UnityCommand ReadCommand(CommandArgs& args);
	UnityConnection& GetConnection();
	UnityPipe& Pipe();

protected:
	UnityConnection m_Connection;
};
