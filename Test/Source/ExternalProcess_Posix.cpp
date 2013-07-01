#include "ExternalProcess.h"

#include <cstdio>
#include <cerrno>

#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>

ExternalProcess::ExternalProcess(const std::string& app, const std::vector<std::string>& arguments) :
m_LineBuffer(""),
m_LineBufferValid (false),
m_Pid(0),
m_Input(NULL),
m_Output(NULL),
m_ExitCode(-1),
m_Exited(false),
m_ApplicationPath(app),
m_Arguments(arguments),
m_ReadTimeout (5.0)
{
}

ExternalProcess::~ExternalProcess()
{
	Shutdown ();
	fclose (m_Input);
	fclose (m_Output);
}

bool ExternalProcess::Launch()
{
	int readpipe[2];   // file descriptors for the parent to read and the child to write
	int writepipe[2];   // file descriptors for the parent to write and the child to read

	// Setup communication pipeline first
	if (pipe(readpipe) || pipe(writepipe))
	{
		std::cerr << "Pipe error!" << std::endl;
		return false;
	}

	// Attempt to fork and check for errors
	m_Pid = fork ();

	if (m_Pid == -1)
	{
		std::cerr << "fork error" << std::endl;  // something went wrong
		return false;
	}

	if (m_Pid != 0) {
		// A positive (non-negative) PID indicates the parent process
		m_Input = fdopen (readpipe[0], "r");
		m_Output = fdopen (writepipe[1], "w");
		close (readpipe[1]);
		close (writepipe[0]);

		if (!(m_Input && m_Output)) {
			std::cerr << "error in fdopen()" << std::endl;
			fclose (m_Input);
			fclose (m_Output);
			return false;
		}

		return true;
	} else {
		// A zero PID indicates that this is the child process
		dup2 (writepipe[0], STDIN_FILENO);      // Replace stdin with the IN side of the pipe
		dup2 (readpipe[1], STDOUT_FILENO);      // Replace stdout with the OUT side of the pipe
		close (readpipe[0]);
		close (writepipe[1]);

		// Extract binary name from the path:
		std::string bin = m_ApplicationPath;
		int lastSlash = m_ApplicationPath.rfind ('/');

		if (lastSlash != std::string::npos)
			bin = m_ApplicationPath.substr (lastSlash + 1);

		std::vector<char const*> args;
		args.push_back (bin.c_str ());

		for (std::vector<std::string>::const_iterator it = m_Arguments.begin (),
		     end = m_Arguments.end (); it != end; ++it)
			args.push_back (it->c_str ());

		args.push_back (NULL);

		// Replace the child fork with a new process
		int ret = execvp (m_ApplicationPath.c_str (), const_cast<char* const*> (&args[0]));
		if( -1 == ret ) {
			std::cerr << "Error: execvp(" << m_ApplicationPath << ") : " << strerror(errno) << std::endl;
			exit (ret);
		}
	}

	return true;
}

bool ExternalProcess::IsRunning()
{
	if (m_Exited)
		return false;

	int status;
	int retval = waitpid (m_Pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
	if (0 == retval) // State hasn't changed since last time
		return true;

	if (WIFEXITED (status)) {
		m_Exited = true;
		m_ExitCode = WEXITSTATUS (status);
	} else if (WIFSIGNALED (status)) {
		m_Exited = true;
		m_ExitCode = WTERMSIG (status);
	}
	return !m_Exited;
}

bool ExternalProcess::Write(const std::string& data)
{
	fprintf (m_Output, "%s", data.c_str ());
	fflush (m_Output);
	return true;
}

std::string ExternalProcess::ReadLine (FILE *file, std::string &buffer, bool removeFromBuffer)
{
	int readyFDs,
	    fd = fileno (file);
	fd_set fdReadSet;
	std::string::size_type i = buffer.find('\n');
	int retries = 3;
	struct timeval timeout;
	timeout.tv_sec = (time_t) m_ReadTimeout;
	timeout.tv_usec = (suseconds_t) ((m_ReadTimeout - timeout.tv_sec) * 1000000.0);

	while (retries > 0)
	{
		if (!IsRunning())
			throw ExternalProcessException(EPSTATE_NotRunning, "Trying to read from process that is not running");

		if (i == std::string::npos)
		{
			// No newline found - read more data

			FD_ZERO(&fdReadSet);
			FD_SET(fd, &fdReadSet);
			readyFDs = select (fd + 1, &fdReadSet, NULL, NULL, &timeout);

			if ( readyFDs == -1 ) {
				if (errno == EAGAIN) {
					--retries;
					continue;
				}
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error polling external process");
			} else if (readyFDs == 0 || !FD_ISSET (fd, &fdReadSet)) {
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Got unknown read ready file description while write from external process");
			}

			char charBuffer[BUFSIZ];
			ssize_t bytesread = read (fd, charBuffer, BUFSIZ-1);
			if (0 >= bytesread)
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error reading external process - read 0 bytes");

			charBuffer[bytesread] = '\0';
			buffer += charBuffer;
			retries = 3;
		}

		i = buffer.find ('\n');
		if (std::string::npos != i) {
			std::string line = buffer.substr (0, i);
			if (removeFromBuffer)
				buffer.erase (0, i+1);
			return line;
		}
	}

	throw ExternalProcessException(EPSTATE_BrokenPipe, "Failed waiting for read from external process");
}

std::string ExternalProcess::ReadLine()
{
	return ReadLine (m_Input, m_LineBuffer, true);
}

std::string ExternalProcess::PeekLine()
{
	return ReadLine (m_Input, m_LineBuffer, false);
}

void ExternalProcess::Shutdown()
{
	kill (m_Pid, SIGINT);
	usleep (100000);
	if (IsRunning ())
		kill (m_Pid, SIGKILL);
}

void ExternalProcess::SetReadTimeout(double secs)
{
	m_ReadTimeout = secs;
}

double ExternalProcess::GetReadTimeout()
{
	return m_ReadTimeout;
}

void ExternalProcess::SetWriteTimeout(double secs)
{
	m_WriteTimeout = secs;
}

double ExternalProcess::GetWriteTimeout()
{
	return m_WriteTimeout;
}
