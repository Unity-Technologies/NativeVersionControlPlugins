#include "FSPlugin.h"
#include <windows.h>

// Program Entry point and set-up for windows
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)
{
    FSPlugin plugin(cmdLine);
    return plugin.Run();
}
