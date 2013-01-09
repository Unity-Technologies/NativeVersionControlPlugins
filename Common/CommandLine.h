#pragma once

// Simple utilities for parsing command lines
void CommandLineFreeArgs(char** argv);
char** CommandLineToArgv( const char* CmdLine, int* _argc );
void Trace( const char msg[], ... );
