#include "AESPlugin.h"
#include <windows.h>

// Program Entry point and set-up for windows
int __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* cmdLine, int cmdShow)
{
    AESPlugin plugin(cmdLine);
    return plugin.Run();
}
