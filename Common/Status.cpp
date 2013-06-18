#include "Status.h"

using namespace std;

const char* VCSSeverityToString(VCSSeverity s)
{
	switch (s)
	{
	case VCSSEV_OK: return "OK";
	case VCSSEV_Info: return "Info";
	case VCSSEV_Warn: return "Warn";
	case VCSSEV_Error: return "Error";
	case VCSSEV_Command: return "Command";
	default: break;
	}
	return "<UNKNOWN SEVERITY>";
}

bool StatusContains( const VCSStatus& status, const std::string& needle )
{
	for (VCSStatus::const_iterator i = status.begin(); i != status.end(); ++i)
	{
		if (i->message.find(needle) != string::npos)
		{
			return true;
		}
	}
	return false;
}
