#ifndef AESCOMMAND_H
#define AESCOMMAND_H

#include "AESTask.h"
#include <string>

class AESCommand {
public:
	virtual bool Run(AESTask& task, const CommandArgs& args) = 0;

protected:
	AESCommand(const char* name);
};

// Lookup command handler by name
AESCommand* LookupCommand(const std::string& name);

#endif // AESCOMMAND_H