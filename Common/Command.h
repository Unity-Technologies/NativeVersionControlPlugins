#pragma once
#include <exception>
#include <string>

enum UnityCommand
{
	UCOM_Invalid,
	UCOM_Shutdown,
	UCOM_Add,
	UCOM_ChangeDescription,
	UCOM_ChangeMove,
	UCOM_ChangeStatus,
	UCOM_Changes,
	UCOM_Checkout,
	UCOM_Config,
	UCOM_DeleteChanges,
	UCOM_Delete,
	UCOM_Download,
	UCOM_Exit,
	UCOM_FileSetBase,
	UCOM_GetLatest,
	UCOM_IncomingChangeAssets,
	UCOM_Incoming,
	UCOM_Lock,
	UCOM_Login,
	UCOM_Move,
	UCOM_QueryConfigParameters,
	UCOM_Resolve,
	UCOM_RevertChanges,
	UCOM_Revert,
	UCOM_Spec,
	UCOM_StatusBase,
	UCOM_Status,
	UCOM_Submit,
	UCOM_Unlock,
};

// Command string as received from unity pipe
const char* UnityCommandToString(UnityCommand c);
UnityCommand StringToUnityCommand(const char* name);

class CommandException : public std::exception
{
public:
	CommandException(UnityCommand c, const std::string& about);
	~CommandException() throw() {} 
	virtual const char* what() const throw();
private:
	std::string m_What;
};
