#include "Status.h"
#include "Connection.h"

using namespace std;

Connection& SendToConnection(Connection& p, const VCSStatus& st, MessageArea ma)
{
	for (VCSStatus::const_iterator i = st.begin(); i != st.end(); ++i)
	{
		switch (i->severity)
		{
            case VCSSEV_OK:
                p.VerboseLine(i->message, ma);
                break;
            case VCSSEV_Info:
                p.InfoLine(i->message, ma);
                break;
            case VCSSEV_Warn:
                p.WarnLine(i->message, ma);
                break;
            case VCSSEV_Error:
                p.ErrorLine(i->message, ma);
                break;
            case VCSSEV_Command:
                p.Command(i->message, ma);
                break;
            default:
                // MAPlugin will make Unity restart the plugin
                p.ErrorLine(string("<Unknown errortype>: ") + i->message, MAPlugin);
                break;
		}
	}
	return p;
}

Connection& operator<<(Connection& p, const VCSStatus& st)
{
	return SendToConnection(p, st, MAGeneral);
}


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
