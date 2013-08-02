#pragma once
#include <string>

struct P4Info
{
	bool clientIsKnown;
	std::string clientName;
	std::string clientHost;
	std::string currentDir;
	std::string peerAddress;
	std::string clientAddress;
	std::string serverAddress;
	std::string serverRoot;
	std::string serverVersion;
	std::string serverLicense;
	std::string caseHandling; 
};
