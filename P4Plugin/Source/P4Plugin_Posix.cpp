#include "P4Task.h"
#include "CommandLine.h"
#include <iostream>
#include <sstream>
#include <string>


// Program Entry point and set-up for OSX
int main(int argc, char **argv)
{
    P4Task task;
    return task.Run();
}
