#include "ExternalProcess.h"
#include "Utility.h"
#include "CommandLine.h"
#include <iostream>
#include <fstream>
#include <exception>
#include <stdlib.h>
#include <time.h>

using namespace std;

const static string restartline = "<restartplugin>";
const static string includeline = "<include ";
const static string onceline = "<once ";
const static string commanddelim = "--";
const static string expectdelim = "--";
const static string matchtoken = "re:";
const static string regextoken = "==:";
const static string exittoken = "<exit>";
const static string ignoretoken = "<ignore>";
const static string genfiletoken = "<genfile ";

void printStatus(bool ok);
int run(int argc, char* argv[]);

#ifdef WIN32
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)
{
    int argc = 0;
    char** argv = CommandLineToArgv(cmdLine, &argc);
#else
int main(int argc, char* argv[])
{
#endif
	try
	{
		return run(argc, argv);
	}
	catch (ExternalProcessException e)
	{
#ifdef WIN32
		CommandLineFreeArgs(argv);
#endif
		cerr << e.what() << endl;
		return 1;
	}
	catch (exception e)
	{
#ifdef WIN32
		CommandLineFreeArgs(argv);
#endif
		cerr << e.what() << endl;
		return 1;
	}
	catch (...)
	{
#ifdef WIN32
		CommandLineFreeArgs(argv);
#endif
		cerr << "Caught unhandled exception"  << endl;
		return 1;
	}
}

static int runScript(ExternalProcess& p, const string& scriptPath, const string& indent = "");

static bool verbose;
static bool newbaseline;
static bool noresults;
static string root;
static string absroot;
static vector<string> incfiles;

static void ConvertSeparatorsFromWindows( string& pathName )
{
	for (string::iterator i = pathName.begin(); i != pathName.end(); ++i)
		if (*i == '\\')
			*i = '/';
}

static void UnescapeString(string& target)
{
	string::size_type len = target.length();
	string::size_type n1 = 0;
	string::size_type n2 = 0;

	while ( n1 < len && (n2 = target.find('\\', n1)) != string::npos &&
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

static void EscapeNewline(string& str)
{
	string::size_type i = str.find('\n');
	while (i != string::npos)
	{
		str.replace(i, 1, "\\n");
		i = str.find('\n');
	}
}

static void replaceRootTagWithPath(string& str)
{
	string::size_type i = str.find("<absroot>");

	while (i != string::npos)
	{
		str.replace(i, 9, absroot);
		i = str.find("<absroot>");
	}

	i = str.find("<root>");
	while (i != string::npos)
	{
		str.replace(i, 6, root);
		i = str.find("<root>");
	}
}

static void replaceRootPathWithTag(string& instr)
{
	string str = instr;
	ConvertSeparatorsFromWindows(str);
	string::size_type i = str.find(absroot);
	bool replacedSomething = false;

	while (i != string::npos)
	{
		str.replace(i, absroot.length(), "<absroot>");
		i = str.find(absroot);
		replacedSomething = true;
	}

#ifdef WIN32
	// Need this hack because p4 on windows create result paths like
	// c:/foo/bar\tmp/client/path
	// ie. backslash and slash as path separator mixed in some cases
	string wabsroot = absroot;
	ConvertSeparatorsFromWindows(wabsroot);
	i = str.find(wabsroot);
	while (i != string::npos)
	{
		str.replace(i, wabsroot.length(), "<absroot>");
		i = str.find(wabsroot);
		replacedSomething = true;
	}
	
	string wroot = root;
	ConvertSeparatorsFromWindows(wroot);
	i = str.find(wroot);
	while (i != string::npos)
	{
		str.replace(i, wroot.length(), "<root>");
		i = str.find(wroot);
		replacedSomething = true;
	}
#endif

	i = str.find(root);
	while (i != string::npos)
	{
		str.replace(i, root.length(), "<root>");
		i = str.find(root);
		replacedSomething = true;
	}
	if (replacedSomething)
		instr.swap(str);

#ifdef _WIN32_NOTDEFINED
	string wroot = root;
	ConvertSeparatorsFromWindows(wroot);
	i = str.find(wroot);
	if (i != string::npos)
		str.replace(i, wroot.length(), "<root>");

	string wabsroot = absroot;
	ConvertSeparatorsFromWindows(wabsroot);
	i = str.find(wabsroot);
	if (i != string::npos)
		str.replace(i, wabsroot.length(), "<root>");
#endif
	
}

int run(int argc, char* argv[])
{
	newbaseline = argc > 3 ? string(argv[3]) == "newbaseline" : false;
	noresults = newbaseline;
	verbose = argc > 3 ? string(argv[3]) == "verbose" && !newbaseline : false;
	root = Trim(argc > 4 ? argv[4] : "", '\'');
#ifdef _WIN32
	absroot = string(getenv("PWD")) + "\\" + root;
#else
	absroot = string(getenv("PWD")) + "/" + root;
#endif

	cout << "Root:    " << root <<  endl;
	cout << "AbsRoot: " << absroot <<  endl;

	if (verbose)
		cout << "Plugin : " << argv[1] << endl;

	vector<string> arguments;
	ExternalProcess p(argv[1], arguments);
	p.Launch();
	int res = runScript(p, argv[2]);
	return res;
}

static int runScript(ExternalProcess& p, const string& scriptPath, const string& indent)
{
	if (!noresults)
		cout << indent << "Testing " << scriptPath << " " << endl << flush;

	ifstream testscript(scriptPath.c_str());

	const int BUFSIZE = 4096;
	char buf[BUFSIZE];
	buf[0] = 0x00;

	bool ok = true;
	while (testscript.good())
	{
	restart:

		while (testscript.getline(buf, BUFSIZE))
		{
			string command(buf);
			
			replaceRootTagWithPath(command);

			if (verbose || newbaseline)
				cout << indent << command << endl;

			if (command.find(commanddelim) == 0)
				break; // done command lines

			if (command.find(exittoken) == 0)
				return 0;

			if (command.find(restartline) == 0)
			{
				if (verbose)
					cout << indent << "Restarting plugin";
				vector<string> arguments;
				p = ExternalProcess(p.GetApplicationPath(), arguments);
				p.Launch();
				goto restart;
			}

			if (command.find(includeline) == 0 || command.find(onceline) == 0)
			{
				bool once = command.find(onceline) == 0;
				string incfile = once ? command.substr(6, command.length() - 1 - 6) : command.substr(9, command.length() - 1 - 9) ;
				if (once && find(incfiles.begin(), incfiles.end(), incfile) != incfiles.end())
				{
					if (verbose)
						cout << indent << "Already included " << incfile << endl;
					continue;
				}

				bool orig_newbaseline = newbaseline;
				newbaseline = false;
				string subIndent = indent + "        ";
				int res = runScript(p, incfile, subIndent);
				newbaseline = orig_newbaseline;
				if (res)
				{
					if (verbose)
						cout << indent << "Error in include script " << incfile << endl;
					return res;
				}

				if (once)
					incfiles.push_back(incfile);

				continue;
			}

			if (command.find(genfiletoken) == 0)
			{
				// Generate and posssibly overwrite a file
				string genfile = command.substr(9, command.length() - 1 - 9);
				{
					fstream f(genfile.c_str(), ios_base::trunc | ios_base::out);
					f << "Random: " << rand() 
					  << "\nTime: " << time(0) 
					  << "\nRandom: " << rand() << endl;
					f.flush();
				}

				continue;
			}

			if (!command.empty())
			{
				p.Write(command);
				p.Write("\n");
			}
		}

		while (testscript.getline(buf, BUFSIZE))
		{
			string expect(buf);

			if (verbose || newbaseline)
				cout << indent << expect << endl;

			if (expect.find(expectdelim) == 0)
				break; // done expect lines

			if (expect.find(exittoken) == 0)
				return 0;

			if (expect.find(restartline) == 0)
			{
				vector<string> arguments;
				p = ExternalProcess(p.GetApplicationPath(), arguments);
				p.Launch();
				continue;
			}

			string msg = p.ReadLine();
			UnescapeString(msg);
			replaceRootPathWithTag(msg);
			EscapeNewline(msg);
			if (expect.find(regextoken) == 0)
			{
				// TODO: implement regex match
			}
			else if (expect.find(ignoretoken) == 0)
			{
				continue; // ignore this line for match
			}
			else 
			{
				// Optional match token
				if (expect.find(matchtoken) == 0)
					expect = expect.substr(0, matchtoken.length());

				if (expect != msg)
				{
					ok = false;
					printStatus(ok);
					cout << "Output fail: expected '" << expect << "'" << endl;
					cout << "             got      '" << msg << "'" << endl << flush;

					// Read as much as possible from plugin and stop
					p.SetReadTimeout(0.3);
					try 
					{
						cout << "             reading as much as possible from plugin:" << endl;
						cout << "             " << msg << endl;
						do {
							string l = p.ReadLine();
							UnescapeString(msg);
							replaceRootPathWithTag(l);
							EscapeNewline(msg);
							cout << "             " << l << endl;
						} while (true);

					} catch (...)
					{
						return 1;
					}
				}
			}
		}
	}

	printStatus(ok);

	return 0;
}

#if defined(_WIN32)

void printStatus(bool ok)
{
	if (noresults)
		return;
	cout.flush();

	HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO conBufInfo;
	GetConsoleScreenBufferInfo(hcon, &conBufInfo);

	SetConsoleTextAttribute(hcon, ok ? FOREGROUND_GREEN : FOREGROUND_RED);
	
	if (ok)
		cout << "OK" << endl;
	else
		cout << "Failed" << endl;
	;
	SetConsoleTextAttribute(hcon, conBufInfo.wAttributes);
	cout.flush();
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
		cout << greenColor << "OK" << endColor << endl;
	else
		cout << redColor << "Failed" << endColor << endl;
}
#endif
