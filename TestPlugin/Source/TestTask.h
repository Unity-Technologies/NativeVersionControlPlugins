#pragma once
#include <stdio.h>

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
class TestTask
{
public:
	TestTask();
	~TestTask();

	int Run();
};