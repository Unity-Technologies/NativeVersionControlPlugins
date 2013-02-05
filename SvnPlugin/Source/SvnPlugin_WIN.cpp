#include "SvnTask.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

using namespace std;

// Program Entry point and set-up for windows
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)
{
    SvnTask task;
    return task.Run();
}
