#include "AESPlugin.h"

// Program Entry point and set-up for Posix
int main(int argc, char **argv)
{
    MapOfArguments arguments;
    BuildMapOfArgument(argc, argv, &arguments);
    
    AESPlugin plugin(arguments);
    return plugin.Run();
}
