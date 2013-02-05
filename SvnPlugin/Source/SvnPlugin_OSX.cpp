#include "SvnTask.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>

// Program Entry point and set-up for OSX
int main(int argc, char **argv)
{
    SvnTask task;
    return task.Run();
}
