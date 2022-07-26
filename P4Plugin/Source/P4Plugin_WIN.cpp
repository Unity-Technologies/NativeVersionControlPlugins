#include "P4Task.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

// Program Entry point and set-up for windows
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)
{
    std::string args(cmdLine);
    const bool istestmode = args.find("-test") != std::string::npos ? true : false;
    P4Task task;
    return task.Run(istestmode);
}
