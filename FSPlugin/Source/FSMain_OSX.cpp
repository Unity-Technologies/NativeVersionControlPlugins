#include "FSPlugin.h"

// Program Entry point and set-up for OSX
int main(int argc, char **argv)
{
    FSPlugin plugin(argc, argv);
    return plugin.Run();
}
