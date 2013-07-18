#include "Command.h"
#include "string.h"

struct CmdInfo
{
	UnityCommand id;
	const char* name;
};

static const CmdInfo infos[] = {
	{ UCOM_Shutdown, "shutdown" },
	{ UCOM_Add, "add" }, //
	{ UCOM_ChangeDescription, "changeDescription" }, //
	{ UCOM_ChangeMove, "changeMove" }, //
	{ UCOM_ChangeStatus, "changeStatus" }, //
	{ UCOM_Changes, "changes" }, //
	{ UCOM_Checkout, "checkout" }, //  
	{ UCOM_Config, "pluginConfig" }, //
	{ UCOM_DeleteChanges, "deleteChanges" }, //
	{ UCOM_Delete, "delete" }, //
	{ UCOM_Download, "download" }, //
	{ UCOM_Exit, "exit" }, //
	{ UCOM_GetLatest, "getLatest" }, //
	{ UCOM_IncomingChangeAssets, "incomingChangeAssets" }, //
	{ UCOM_Incoming, "incoming" }, //
	{ UCOM_Lock, "lock" }, //
	{ UCOM_Login, "login" }, //
	{ UCOM_Move, "move" }, //
	{ UCOM_QueryConfigParameters, "queryConfigParameters" }, //
	{ UCOM_Resolve, "resolve" }, //
	{ UCOM_RevertChanges, "revertChanges" }, //
	{ UCOM_Revert, "revert" }, //
	{ UCOM_Status, "status" }, //
	{ UCOM_Submit, "submit" }, //
	{ UCOM_Unlock, "unlock" }, //
	{ UCOM_FileMode, "filemode" }, //
	{ UCOM_CustomCommand, "customCommand" }, //
	{ UCOM_Invalid, 0 } // delimiter
};

const char* UnityCommandToString(UnityCommand c)
{
	const CmdInfo* cur = infos;
	while (cur->name && cur->id != c) { cur++; }
	return cur->name ? cur->name : "invalid";
}


UnityCommand StringToUnityCommand(const char* name)
{
	const CmdInfo* cur = infos;
	while (cur->name && strncmp(cur->name, name, 50)) { cur++; }
	return cur->id;
}

CommandException::CommandException(UnityCommand c, const std::string& about)
{ 
	m_What += UnityCommandToString(c);
	m_What += "Command";
	m_What += " => ";
	m_What += about;
}


const char* CommandException::what() const throw()
{
	return m_What.c_str();
}
