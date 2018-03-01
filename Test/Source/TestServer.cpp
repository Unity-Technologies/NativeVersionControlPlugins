#include "ExternalProcess.h"
#include "Utility.h"
#include <iostream>
#include <fstream>
#include <exception>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#endif

void printStatus(bool ok);
int run(int argc, char* argv[]);
 
int main(int argc, char* argv[])
{
	try
	{
		return run(argc, argv);
	}
	catch (ExternalProcessException e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	catch (std::exception e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Caught unhandled exception"  << std::endl;
		return 1;
	}
}

static void sleepInSeconds(const int sleepTimeInSeconds)
{
#ifdef WIN32
	Sleep(sleepTimeInSeconds*1000);
#else // #ifdef WIN32
	sleep(sleepTimeInSeconds);
#endif // #ifdef WIN32
}

static int runScript(ExternalProcess& p, const std::string& testDir, const std::string& testScript, const std::string& indent = "");

bool verbose;
bool newbaseline;
bool noresults;
std::string absroot;

static void ConvertSeparatorsToWindows( std::string& pathName )
{
	for (std::string::iterator i = pathName.begin(); i != pathName.end(); ++i)
		if (*i == '/')
			*i = '\\';
}

static void ConvertSeparatorsFromWindows( std::string& pathName )
{
	for (std::string::iterator i = pathName.begin(); i != pathName.end(); ++i)
		if (*i == '\\')
			*i = '/';
}

static void UnescapeString(std::string& target)
{
	std::string::size_type len = target.length();
	std::string::size_type n1 = 0;
	std::string::size_type n2 = 0;

	while ( n1 < len && (n2 = target.find('\\', n1)) != std::string::npos &&
		n2+1 < len )
	{
		char c = target[n2+1];
		if ( c == '\\' )
		{
			target.replace(n2, 2, "\\");
			len--;
		}
		else if ( c == 'n')
		{
			target.replace(n2, 2, "\n");
			len--;
		}
		n1 = n2 + 1;
	}
}

static void EscapeNewline(std::string& str)
{
	std::string::size_type i = str.find('\n');
	while (i != std::string::npos)
	{
		str.replace(i, 1, "\\n");
		i = str.find('\n');
	}
}

static void replaceRootTagWithPath(std::string& str)
{
	std::string::size_type i = str.find("<absroot>");

	while (i != std::string::npos)
	{
		str.replace(i, 9, absroot);
		i = str.find("<absroot>");
	}
}

static void replaceRootPathWithTag(std::string& instr)
{
	std::string str = instr;
	ConvertSeparatorsFromWindows(str);
	std::string::size_type i = str.find(absroot);
	bool replacedSomething = false;

	while (i != std::string::npos)
	{
		str.replace(i, absroot.length(), "<absroot>");
		i = str.find(absroot);
		replacedSomething = true;
	}

#ifdef WIN32
	// Need this hack because p4 on windows create result paths like
	// c:/foo/bar\tmp/client/path
	// ie. backslash and slash as path separator mixed in some cases
	std::string wabsroot = absroot;
	ConvertSeparatorsFromWindows(wabsroot);
	i = str.find(wabsroot);
	while (i != std::string::npos)
	{
		str.replace(i, wabsroot.length(), "<absroot>");
		i = str.find(wabsroot);
		replacedSomething = true;
	}
#endif

	if (replacedSomething)
		instr.swap(str);
}

int run(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cerr << "Usage: testserver <FULL_PATH_TO_PLUGIN> <FULL_PATH_TO_TEST_DIR> <TEST_SCRIPT_PATH_RELATIVE_TO_TEST_DIR> <OPTION>" << std::endl;
		return 1;
	}
	newbaseline = argc > 4 ? std::string(argv[4]) == "newbaseline" : false;
	noresults = newbaseline;
	verbose = argc > 4 ? std::string(argv[4]) == "verbose" && !newbaseline : false;
	char buffer[4096];
	char *answer = getcwd(buffer, sizeof(buffer));
	if (answer)
	{
		absroot = buffer;
	}
#ifdef _WIN32
	ConvertSeparatorsToWindows(absroot);
#endif

	std::cout << "AbsRoot: " << absroot <<  std::endl;

	if (verbose)
		std::cout << "Plugin : " << argv[1] << std::endl;

	std::vector<std::string> arguments;
	ExternalProcess p(argv[1], arguments);
	p.Launch();
	int res = runScript(p, argv[2], argv[3]);
	return res;
}

static int runScript(ExternalProcess& p, const std::string& testDir, const std::string& testScript, const std::string& indent)
{
	const std::string scriptPath = testDir + "/" + testScript;
	std::ifstream testscript(scriptPath.c_str());

	if (!noresults)
		std::cout << indent << "Testing " << scriptPath << " " << std::flush;

	if (verbose)
		std::cout << std::endl;

	if (!testscript)
	{
		std::cerr << "Test script failed to open '" << scriptPath << std::endl;
		return 1;
	}

	const int BUFSIZE = 4096;
	char buf[BUFSIZE];
	buf[0] = 0x00;

	bool isWindows = false;
#ifdef _WIN32
	isWindows = true;
#endif

	const std::string restartline = "<restartplugin>";
	const std::string includeline = "<include ";
	const std::string commanddelim = "--";
	const std::string expectdelim = "--";
	const std::string matchtoken = "==:";
	const std::string exittoken = "<exit>";
	const std::string ignoretoken = "<ignore>";
	const std::string ignorewintoken = "<ignorewin>";
	const std::string genfiletoken = "<genfile ";
	const std::string delfiletoken = "<delfile ";
	const std::string p4pluginlogtoken = "<p4pluginlog:";
	const std::string progressToken = "<p:";

	bool ok = true;
	int lineNum = 0;

	std::string p4pluginLogPath(absroot + "/Library/p4plugin.log");
	std::ifstream p4pluginLog(p4pluginLogPath.c_str());

	while (testscript.good())
	{
	restart:

		while (testscript.getline(buf, BUFSIZE))
		{
			lineNum++;
			std::string command(buf);
			
			replaceRootTagWithPath(command);

			if (verbose || newbaseline)
				std::cout << command << std::endl;

			if (command.find(commanddelim) == 0)
				break; // done command lines

			if (command.find(exittoken) == 0)
				return 0;

			if (command.find(restartline) == 0)
			{
				if (verbose)
					std::cout << "Restarting plugin";
				std::vector<std::string> arguments;
				p = ExternalProcess(p.GetApplicationPath(), arguments);
				p.Launch();
				goto restart;
			}

			if (command.find(includeline) == 0)
			{
				std::string incfile = command.substr(includeline.length(), command.length() - 1 - includeline.length());
				if (!newbaseline)
					std::cout << std::endl;
				bool orig_newbaseline = newbaseline;
				newbaseline = false;
				std::string subIndent = indent + "  ";
				int res = runScript(p, testDir,	 incfile, subIndent);
				newbaseline = orig_newbaseline;
				if (res)
				{
					if (verbose)
						std::cout << "Error in include script " << incfile << std::endl;					
					return res;
				}
				if (!newbaseline)
					std::cout << subIndent;
				continue;
			}

			if (command.find(genfiletoken) == 0)
			{
				// Generate and possibly overwrite a file
				std::string genfile = command.substr(genfiletoken.length(), command.length() - 1 - genfiletoken.length());
				{
					std::fstream f(genfile.c_str(), std::ios_base::trunc | std::ios_base::out);
					f << "Random: " << rand() 
					  << "\nTime: " << time(0) 
					  << "\nRandom: " << rand() << std::endl;
					f.flush();
				}
				continue;
			}
			if (command.find(delfiletoken) == 0)
			{
				std::string delfile = command.substr(delfiletoken.length(), command.length() - 1 - delfiletoken.length());
				// Delete a local file
				unlink(delfile.c_str());
				continue;
			}

			if (!command.empty())
			{
				p.Write(command);
				p.Write("\n");
			}
		}

		bool readNextPluginLine = true;
		while (testscript.getline(buf, BUFSIZE))
		{
			lineNum++;
			std::string expect(buf);

			if (verbose || newbaseline)
				std::cout << expect << std::endl;

			if (expect.find(expectdelim) == 0)
				break; // done expect lines

			if (expect.find(exittoken) == 0)
				return 0;

			if (expect.find(restartline) == 0)
			{
				std::vector<std::string> arguments;
				p = ExternalProcess(p.GetApplicationPath(), arguments);
				p.Launch();
				continue;
			}

			std::string msg;
			if (expect.find(p4pluginlogtoken) == 0)
			{
				if (!p4pluginLog.good())
				{
					const int sleepTimeInSeconds = 1;
					sleepInSeconds(sleepTimeInSeconds);
					p4pluginLog.open(p4pluginLogPath.c_str());
				}
				expect = expect.substr(p4pluginlogtoken.length());
				if (verbose)
					std::cerr << "P4Plugin Expect:'" << expect << "'" << std::endl;
				const int  matchLen = expect.length();
				while (p4pluginLog.good())
				{
					std::getline(p4pluginLog, msg);
					if (verbose)
						std::cerr << "P4Plugin:'" << lineNum << " '" << msg << "'" << std::endl;
					msg = msg.substr(0, matchLen);
					if (expect == msg)
					{
						if (verbose)
							std::cerr << "P4Plugin: MATCH '" << expect << "'" << std::endl;
						break;
					}
				}
				readNextPluginLine = false;
			}

			if (readNextPluginLine)
			{
				msg = p.ReadLine();
				UnescapeString(msg);
				replaceRootPathWithTag(msg);
				EscapeNewline(msg);
			}
			readNextPluginLine = true;

			if (expect.find(ignoretoken) == 0)
			{
				continue; // ignore this line for match
			}
			else if (expect.find(ignorewintoken) == 0)
			{
				if (!isWindows)
					readNextPluginLine = false;
				continue;
			}

			// Optional match token
			if (expect.find(matchtoken) == 0)
			{
				expect = expect.substr(matchtoken.length());
				msg = msg.substr(0, expect.length());
			}

			// Match "p*:* * "
			if (expect.find(progressToken) == 0)
			{
				expect = expect.substr(progressToken.length());
				size_t colonIndex = msg.find(":");
				if (colonIndex != std::string::npos)
				{
					std::string buffer(msg.substr(colonIndex+1));
					// skip to the second " "
					size_t space1Index = buffer.find(" ");
					if (space1Index != std::string::npos)
					{
						buffer = buffer.substr(space1Index+1);
						size_t space2Index = buffer.find(" ");
						if (space2Index != std::string::npos)
							msg = buffer.substr(space2Index+1);
					}
				}
			}

			if (expect != msg)
			{
				ok = false;
				printStatus(ok);
				std::cerr << "Output fail: expected '" << expect << "' at " << scriptPath << ":" << lineNum << std::endl;
				std::cerr << "             got      '" << msg << "'" << std::endl;

				// Read as much as possible from plugin and stop
				p.SetReadTimeout(0.3);
				try 
				{
					std::cerr << "             reading as much as possible from plugin:" << std::endl;
					std::cerr << msg << std::endl;
					do {
						std::string l = p.ReadLine();
						UnescapeString(msg);
						replaceRootPathWithTag(l);
						EscapeNewline(msg);
						std::cerr << l << std::endl;
					} while (true);

				} catch (...)
				{
					return 1;
				}
			}
		}
	}
	if (lineNum == 0)
	{
		std::cerr << "Invalid test script it has no lines " << testscript << std::endl;
		return 1;
	}

	printStatus(ok);

	return 0;
}

#if defined(_WIN32)

void printStatus(bool ok)
{
	if (noresults)
		return;
	std::cout.flush();

	HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO conBufInfo;
	GetConsoleScreenBufferInfo(hcon, &conBufInfo);

	SetConsoleTextAttribute(hcon, ok ? FOREGROUND_GREEN : FOREGROUND_RED);
	
	if (ok)
		std::cout << "OK" << std::endl;
	else
		std::cout << "Failed" << std::endl;
	;
	SetConsoleTextAttribute(hcon, conBufInfo.wAttributes);
	std::cout.flush();
}

#else
void printStatus(bool ok)
{
	const char * redColor = "\033[;1;31m";
	const char * greenColor = "\033[;1;32m";
	const char * endColor = "\033[0m";
	
	if (noresults)
		return;

	if (ok)
		std::cout << greenColor << "OK" << endColor << std::endl;
	else
		std::cout << redColor << "Failed" << endColor << std::endl;
}
#endif
