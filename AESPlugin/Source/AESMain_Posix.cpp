#include "AESPlugin.h"

// Program Entry point and set-up for Posix
int main(int argc, char **argv)
{
    AESPlugin plugin(argc, argv);
    return plugin.Run();
}
