#pragma once
#include <vector>
#include <string>

struct P4Stream
{
	std::string stream;
	std::string type;
};

typedef std::vector<P4Stream> P4Streams;
