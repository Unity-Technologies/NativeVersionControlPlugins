#include "P4Command.h"
#include "P4Task.h"
#include "Utility.h"
#include <sstream>

/// ONLY FOR TESTING PURPOSES
class P4Login2Command : public P4Command
{
public:
	P4Login2Command(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		if (!task.IsConnected()) // Cannot login without being connected
		{
			Conn().Log().Info() << "Cannot login2 when not connected" << Endl;
			return false;
		}

		ClearStatus();

		m_LoggedIn = false;
		m_Password = task.GetP4Password();

		const std::string cmd = std::string("login2");
		task.CommandRun(cmd, this);		
		Conn().Log().Info() << "Multi factor authentication " << (m_LoggedIn ? "succeeded" : "failed") << Endl;

		return m_LoggedIn;
	}

	void OutputInfo(char level, const char* data)
	{
		std::string d(data);
		Conn().VerboseLine(d);
		
		m_LoggedIn = d == "User mfa_test_user does not use multi factor authentication.";
		if (m_LoggedIn)
			return;

		m_LoggedIn = d == "Multi factor authentication already approved.";
		if (m_LoggedIn)
			return;

		m_LoggedIn = d == "Multi factor authentication approved.";
		if (m_LoggedIn)
			return;
		GetStatus().insert(VCSStatusItem(VCSSEV_Error, d));
	}

	// Default handler of P4 error output. Called by the default P4Command::Message() handler.
	void HandleError(Error* err)
	{
		if (err == 0)
			return;

		StrBuf buf;
		err->Fmt(&buf);
		std::string value(buf.Text());

		if (value.find("Unicode server permits only unicode enabled clients") != std::string::npos ||
			value.find("The authenticity of") != std::string::npos)
		{
			VCSStatus s = errorToVCSStatus(*err);
			GetStatus().insert(s.begin(), s.end());
			return; // Do not propagate. It will be handled by P4Task and is not an error. 
		}

		// Base implementation. Will callback to P4Command::OutputError 
		P4Command::HandleError(err);
	}

	void Prompt(const StrPtr& msg, StrBuf& buf, int noEcho, Error* e)
	{
		std::string prompt(msg.Text());

		Conn().Log().Debug() << "Prompt: " << prompt << Endl;

		bool m_step1 = EndsWith(prompt, "Enter the number for the chosen method: ");
		bool m_step2 = EndsWith(prompt, "Enter OTP (ABBACD): ");

		if (m_step1)
			buf.Set("2");
		else if (m_step2)
			buf.Set("ABBACD");
		else
			buf.Set("");
	}

private:
	bool m_LoggedIn;
	std::string m_Password;

} cLogin2("login2");
