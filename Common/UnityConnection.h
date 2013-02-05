#pragma once
#include <string>
#include <vector>
#include "UnityPipe.h"
#include "Command.h"

class UnityConnection
{
public:
	UnityConnection(const std::string& logPath);
	~UnityConnection();

	// Connect to Unity
	UnityPipe* Connect();
	bool IsConnected() const;

	// Read a command from unity
	UnityCommand ReadCommand(std::vector<std::string>& args);

	// Get the log stream
	unityplugin::LogStream& Log();

	// Get the raw pipe to Unity. 
	// Make sure IsConnected() is true before using.
	UnityPipe& Pipe();

private:
	unityplugin::LogStream m_Log;
	UnityPipe* m_UnityPipe;
};
