#include "ExternalProcess.h"
#include "Utility.h"
#include <iostream>
#include <fstream>
#include <exception>
#include <stdlib.h>
#include <time.h>

using namespace std;

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
		std::cerr << e.what() << endl;
		return 1;
	}
	catch (exception e)
	{
		std::cerr << e.what() << endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Caught unhandled exception"  << endl;
		return 1;
	}
}


static int runScript(ExternalProcess& p, const string& scriptPath, const string& indent = "");

bool verbose;
bool newbaseline;
bool noresults;
string root;
string absroot;

static void ConvertSeparatorsFromWindows( string& pathName )
{
	for (string::iterator i = pathName.begin(); i != pathName.end(); ++i)
		if (*i == '\\')
			*i = '/';
}

static void UnescapeString(string& target)
{
	string::size_type len = target.length();
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
		cout << indent << "Testing " << scriptPath << " " << flush;

	if (verbose)
		cout << endl;

	ifstream testscript(scriptPath.c_str());

	const int BUFSIZE = 4096;
	char buf[BUFSIZE];
	buf[0] = 0x00;

	bool isWindows = false;
#ifdef _WIN32
	isWindows = true;
#endif

	const string restartline = "<restartplugin>";
	const string includeline = "<include ";
	const string commanddelim = "--";
	const string expectdelim = "--";
	const string matchtoken = "==:";
	const string exittoken = "<exit>";
	const string ignoretoken = "<ignore>";
	const string ignorewintoken = "<ignorewin>";
	const string genfiletoken = "<genfile ";

	bool ok = true;
	int lineNum = 0;

	while (testscript.good())
	{
	restart:

		while (testscript.getline(buf, BUFSIZE))
		{
			lineNum++;
			string command(buf);
			
			replaceRootTagWithPath(command);

			if (verbose || newbaseline)
				cout << command << endl;

			if (command.find(commanddelim) == 0)
				break; // done command lines

			if (command.find(exittoken) == 0)
				return 0;

			if (command.find(restartline) == 0)
			{
				if (verbose)
					cout << "Restarting plugin";
				vector<string> arguments;
				p = ExternalProcess(p.GetApplicationPath(), arguments);
				p.Launch();
				goto restart;
			}

			if (command.find(includeline) == 0)
			{
				string incfile = command.substr(9, command.length() - 1 - 9);
				if (!newbaseline)
					cout << endl;
				bool orig_newbaseline = newbaseline;
				newbaseline = false;
				string subIndent = indent + "  ";
				int res = runScript(p, incfile, subIndent);
				newbaseline = orig_newbaseline;
				if (res)
				{
					if (verbose)
						cout << "Error in include script " << incfile << endl;					
					return res;
				}
				if (!newbaseline)
					cout << subIndent;
				continue;
			}

			if (command.find(genfiletoken) == 0)
			{
				// Generate and possibly overwrite a file
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


		bool readNextPluginLine = true;
		while (testscript.getline(buf, BUFSIZE))
		{
			lineNum++;
			string expect(buf);

			if (verbose || newbaseline)
				cout << expect << endl;

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

			string msg;
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

			if (expect != msg)
			{
				ok = false;
				printStatus(ok);
				cerr << "Output fail: expected '" << expect << "' at " << scriptPath << ":" << lineNum << endl;
				cerr << "             got      '" << msg << "'" << endl;

				// Read as much as possible from plugin and stop
				p.SetReadTimeout(0.3);
				try 
				{
					cerr << "             reading as much as possible from plugin:" << endl;
					cerr << msg << endl;
					do {
						string l = p.ReadLine();
						UnescapeString(msg);
						replaceRootPathWithTag(l);
						EscapeNewline(msg);
						cerr << l << endl;
					} while (true);

				} catch (...)
				{
					return 1;
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
