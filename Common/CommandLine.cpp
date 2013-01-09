#include <stdio.h>
#include <stdarg.h>
#include "CommandLine.h"

// Simple utilities for parsing command lines.  OSX is supported by remapping the windows commands
// This needs to be tidied or removed eventually as its very "Micrsoft!"
#if defined(_WINDOWS)
#   include <windows.h>
#else
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#   define PVOID void*
#   define PCHAR char*
#   define PUCHAR unsigned char*
#   define CHAR char
#   define ULONG unsigned int
#   define HGLOBAL void*
#   define BOOLEAN bool
#   define TRUE true
#   define FALSE false
#   define GlobalAlloc(AREA,SIZE) malloc(SIZE)
#   define GlobalFree(PTR) free(PTR)
#endif

void CommandLineFreeArgs(char** argv)
{
    if ( argv != 0 )
        GlobalFree((HGLOBAL)argv);
}

char** CommandLineToArgv( const char* CmdLine, int* _argc )
{
    PCHAR*  argv;
    PCHAR   _argv;
    ULONG   len;
    ULONG   argc;
    CHAR    a;
    ULONG   i, j;

    BOOLEAN in_QM;
    BOOLEAN in_TEXT;
    BOOLEAN in_SPACE;

    len = strlen(CmdLine);
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
        i + (len+2)*sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while( a = CmdLine[i] ) {
        if(in_QM) {
            if(a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case '\"':
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if(in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

#if defined( _WINDOWS )
#   include <malloc.h>
#   define VSNPRINTF _vsnprintf
#   define STACK_ALLOC( SIZE ) _alloca( SIZE )
#   define PRINT( OUTPUT ) OutputDebugStringA( OUTPUT )
#else
#   include <alloca.h>
#   define VSNPRINTF vsnprintf
#   define STACK_ALLOC( SIZE ) alloca( SIZE )
#   define PRINT( OUTPUT ) fprintf(stderr, "%s", OUTPUT )
#endif

// Cross platform simple printf that prints to console on both VS and XCode
void Trace( const char msg[], ... )
{
    const int maxSize = 1024;

    va_list arg;
    char * traceMessage;
    va_start( arg, msg ); 

    int required = VSNPRINTF( NULL, 0, msg, arg );
    va_end( arg );
    if ( required >= 0 )
    {
        if ( ++required > maxSize )
            required = maxSize;

        char* allocated = (char*)STACK_ALLOC( required );
        if ( allocated != 0 )
        {
            traceMessage = allocated;
            va_start( arg, msg );
            VSNPRINTF( traceMessage, required, msg, arg );
            va_end( arg );
            traceMessage[ required - 1 ] = 0;
        }
    }
    else 
    {
        traceMessage = (char*)STACK_ALLOC(1);
        traceMessage[0] = 0;
    }

    PRINT( traceMessage );
}
