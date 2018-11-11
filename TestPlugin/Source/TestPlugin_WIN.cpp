#include "TestTask.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

// Program Entry point and set-up for windows
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)   
{
    TestTask task;
    return task.Run();
}