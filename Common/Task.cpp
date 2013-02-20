#include "Task.h"

using namespace std;
using namespace unityplugin;

Task::Task(const std::string& logPath)
	: m_Connection(logPath)
{
}

UnityCommand Task::ReadCommand(CommandArgs& args)
{
	UnityCommand cmd = UCOM_Invalid;
	args.clear();
	try
	{
		if (!m_Connection.IsConnected())
			m_Connection.Connect();

		// Read
		try 
		{
			cmd = m_Connection.ReadCommand(args);
		} 
		catch (CommandException& ce)
		{
			m_Connection.Pipe().ErrorLine(string("invalid command - ") + Join(args));
			m_Connection.Pipe().ErrorLine(string("                - ") + ce.what());
			return UCOM_Invalid;
		}
	}
	catch (exception& e)
	{
		m_Connection.Log() << "While reading command: " << e.what() << Endl;
		return UCOM_Invalid;
	}

    return cmd;
}

UnityConnection& Task::GetConnection()
{
	return m_Connection;
}

UnityPipe& Task::Pipe()
{
	return m_Connection.Pipe();
}

unityplugin::LogStream& Task::Log()
{
	return m_Connection.Log();
}
