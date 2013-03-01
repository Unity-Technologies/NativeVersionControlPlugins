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
