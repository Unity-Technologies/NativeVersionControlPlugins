#pragma once
#include <string>
#include <set>

enum VCSSeverity
{
	VCSSEV_OK,
	VCSSEV_Info,
	VCSSEV_Warn,
	VCSSEV_Error
};

const char* VCSSeverityToString(VCSSeverity s);

struct VCSStatusItem
{
	VCSStatusItem(VCSSeverity sev, const std::string& msg) : severity(sev), message(msg) { }
	VCSSeverity severity;
	std::string message;
};

// Sort higher severity as before when iterating a set
struct VCSStatusItemCmp 
{
	bool operator()(const VCSStatusItem& a, const VCSStatusItem& b)
	{
		if (a.severity > b.severity)
			return true;
		else if (a.severity < b.severity)
			return false;
		return a.message < b.message;
	}
};

typedef std::set<VCSStatusItem, VCSStatusItemCmp> VCSStatus;
