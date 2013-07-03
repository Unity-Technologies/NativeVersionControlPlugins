#include "ExternalProcess.h"
#include <iostream>
#include <fstream>
#include <exception>
#include <stdlib.h>

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

static void replaceRoot(string& str)
{
	string::size_type i = str.find(root);
	if (i != string::npos)
		str.replace(i, root.length(), "<root>");

	i = str.find(absroot);
	if (i != string::npos)
		str.replace(i, absroot.length(), "<absroot>");
}

int run(int argc, char* argv[])
{
	newbaseline = argc > 3 ? string(argv[3]) == "newbaseline" : false;
	noresults = newbaseline;
	verbose = argc > 3 ? string(argv[3]) == "verbose" && !newbaseline : false;
	root = argc > 4 ? argv[4] : "";
	absroot = string(getenv("PWD")) + "/" + root;

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
		cout << indent << "Testing " << scriptPath << " ";

	if (verbose)
		cout << endl;

	ifstream testscript(scriptPath.c_str());

	const int BUFSIZE = 4096;
	char buf[BUFSIZE];

	const string restartline = "<restartplugin>";
	const string includeline = "<include ";
	const string commanddelim = "--";
	const string expectdelim = "--";
	const string matchtoken = "re:";
	const string regextoken = "==:";
	const string exittoken = "<exit>";
	const string ignoretoken = "<ignore>";

	bool ok = true;
	while (testscript.good())
	{
	restart:

		while (testscript.getline(buf, BUFSIZE))
		{
			string command(buf);

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
					cout << "{" << endl;
				bool orig_newbaseline = newbaseline;
				newbaseline = false;
				int res = runScript(p, incfile, indent + "  ");
				newbaseline = orig_newbaseline;
				if (res)
				{
					if (verbose)
						cout << "Error in include script " << incfile << endl;					
					return res;
				}
				if (!newbaseline)
					cout << "} ";
				continue;
			}

			p.Write(buf);
			p.Write("\n");
		}

		while (testscript.getline(buf, BUFSIZE))
		{
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

			string msg = p.ReadLine();
			replaceRoot(msg);
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
					cerr << "Output fail: expected '" << expect << "'" << endl;
					cerr << "             got      '" << msg << "'" << endl;

					// Read as much as possible from plugin and stop
					p.SetReadTimeout(0.3);
					try 
					{
						cerr << "             reading as much as possible from plugin:" << endl;
						do {
							string l = p.ReadLine();
							replaceRoot(l);
							cerr << l << endl;
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
