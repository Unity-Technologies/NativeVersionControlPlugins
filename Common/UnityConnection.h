#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "UnityPipe.h"
#include "Command.h"

class UnityConnection
{
public:
	UnityConnection(const std::string& logPath);
	~UnityConnection();

	// Connect to Unity
	UnityPipe* Connect();

	// Read a command from unity
	UnityCommand ReadCommand(std::vector<std::string>& args);

	// Get the log stream
	std::ofstream& Log();
	UnityPipe& Pipe();

private:
	std::ofstream m_Log;
	UnityPipe* m_UnityPipe;
};
