#include "AESCommand.h"
#include <string>
#include <map>
#include <cassert>

using namespace std;

typedef std::map<string, AESCommand*> CommandMap;
static CommandMap* s_Commands = NULL;

AESCommand* LookupCommand(const string& name)
{
	assert(s_Commands != NULL);
	CommandMap::iterator i = s_Commands->find(name);
	if (i == s_Commands->end()) return NULL;
	return i->second;
}

AESCommand::AESCommand(const char* name)
{
	if (s_Commands == NULL)
		s_Commands = new CommandMap();
    
	s_Commands->insert(make_pair(name,this));
}
