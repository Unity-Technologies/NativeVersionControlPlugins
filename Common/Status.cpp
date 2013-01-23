#include "BaseTask.h"

using namespace std;

const char* VCSSeverityToString(VCSSeverity s)
{
	switch (s)
	{
	case VCSSEV_OK: return "OK";
	case VCSSEV_Info: return "Info";
	case VCSSEV_Warn: return "Warn";
	case VCSSEV_Error: return "Error";
	default: break;
	}
	return "<UNKNOWN SEVERITY>";
}
