#include "ExternalProcess.h"
#include <iostream>
#include <fstream>

using namespace std;

void printStatus(bool ok);
 
int main(int argc, char* argv[])
{
	bool verbose = argc > 3 ? string(argv[3]) == "verbose" : false;

	if (verbose)
		cout << "Pllugin : " << argv[1] << endl;

	cout << "Testing " << argv[2] << " ";
	if (verbose)
		cout << endl;

	vector<string> arguments;
	ExternalProcess p(argv[1], arguments);
	p.Launch();

	ifstream testscript(argv[2]);

	const int BUFSIZE = 4096;
	char buf[BUFSIZE];

	const string restartline = "<restartplugin>";
	const string commanddelim = "--";
	const string expectdelim = "--";
	const string matchtoken = "re:";
	const string regextoken = "==:";
	const string exittoken = "<exit>";

	bool ok = true;
	while (!testscript.eof())
	{
		// Just forward the command to the plugin
		if (restartline == buf)
		{
			if (verbose)
				cout << "Restarting plugin";
			p = ExternalProcess(argv[1], arguments);
			p.Launch();
			continue;
		}

		while (testscript.getline(buf, BUFSIZE))
		{
			string command(buf);
			
			if (verbose)
				cout << command << endl;

			if (command.find(commanddelim) == 0)
				break; // done command lines

			if (command.find(exittoken) == 0)
				return 0;

			if (command.find(restartline) == 0)
			{
				p = ExternalProcess(argv[1], arguments);
				p.Launch();
				continue;
			}
			
			p.Write(buf);
			p.Write("\n");
		}

		while (testscript.getline(buf, BUFSIZE))
		{
			string expect(buf);

			if (verbose)
				cout << expect << endl;

			if (expect.find(expectdelim) == 0)
				break; // done expect lines

			if (expect.find(exittoken) == 0)
				return 0;

			if (expect.find(restartline) == 0)
			{
				p = ExternalProcess(argv[1], arguments);
				p.Launch();
				continue;
			}

			string msg = p.ReadLine();
			if (expect.find(regextoken) == 0)
			{
				// TODO: implement regex match
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

	if (ok)
		cout << greenColor << "OK" << endColor << endl;
	else
		cout << redColor << "Failed" << endColor << endl;
}
