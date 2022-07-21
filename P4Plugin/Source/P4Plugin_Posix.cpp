#include "P4Task.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>


// Program Entry point and set-up for OSX
int main(int argc, char **argv)
{
    const bool istestmode = argc > 1 ? std::string(argv[1]) == "-test" : false;
    P4Task task;
    return task.Run(istestmode);
}
