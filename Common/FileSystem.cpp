#include "FileSystem.h"
#include <stdexcept>
#include <stdio.h>
#include <string.h>


using namespace std;

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "shlwapi.h"

static inline void UTF8ToWide( const char* utf8, wchar_t* outBuffer, int outBufferSize )
{
	int res = MultiByteToWideChar( CP_UTF8, 0, utf8, -1, outBuffer, outBufferSize );
	if( res == 0 )
		outBuffer[0] = 0;
}

static inline void ConvertSeparatorsToWindows( wchar_t* pathName )
{
	while( *pathName != '\0' ) {
		if( *pathName == '/' )
			*pathName = '\\';
		++pathName;
	}
}

void ConvertUnityPathName( const char* utf8, wchar_t* outBuffer, int outBufferSize )
{
	UTF8ToWide( utf8, outBuffer, outBufferSize );
	ConvertSeparatorsToWindows( outBuffer );
}

string PluginPath()
{
	HMODULE hModule = GetModuleHandleW(NULL);
	if (hModule == NULL)
		return "";

	CHAR path[MAX_PATH];
	if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0)
		return "";
	return path;
}

const size_t kDefaultPathBufferSize = 1024;

bool EnsureDirectory(const string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	
	if (!PathFileExistsW(widePath))
	{
		// Create the path
		return 0 != CreateDirectoryW(widePath, NULL);
	} 
	else if (PathIsDirectoryW(widePath))
	{
		return true;
	}
	
	// Create the path
	return false;
	
}

static bool IsReadOnlyW(LPCWSTR path)
{
	DWORD attributes = GetFileAttributesW(path);
	// @TODO: Error handling
	/*
	if (INVALID_FILE_ATTRIBUTES == attributes)
		upipe.Log() << "Error stat on " << path << endl;
	 */
	return FILE_ATTRIBUTE_READONLY & attributes;
}

bool IsReadOnly(const string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	return IsReadOnlyW(widePath);
}


bool IsDirectory(const string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	return PathIsDirectoryW(widePath) == TRUE;
}

bool PathExists(const std::string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	return PathFileExistsW(widePath) == TRUE;	
}

static bool RemoveReadOnlyW(LPCWSTR path)
{
	DWORD attributes = GetFileAttributesW(path);
	
	if (INVALID_FILE_ATTRIBUTES != attributes)
	{
		attributes &= ~FILE_ATTRIBUTE_READONLY;
		return 0 != SetFileAttributesW(path, attributes);
	}
	
	return false;
}


bool MoveAFile(const string& fromPath, const string& toPath)
{
	wchar_t wideFrom[kDefaultPathBufferSize], wideTo[kDefaultPathBufferSize];
	ConvertUnityPathName( fromPath.c_str(), wideFrom, kDefaultPathBufferSize );
	ConvertUnityPathName( toPath.c_str(), wideTo, kDefaultPathBufferSize );
	if( MoveFileExW( wideFrom, wideTo, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ) )
		return true;
	
	if (ERROR_ACCESS_DENIED == GetLastError())
	{
		if (RemoveReadOnlyW(wideTo))
		{
			if (MoveFileExW(wideFrom, wideTo, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
			{
				return true;
			}
		}
	}
	return false;
}

#else // MACOS 

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

bool IsReadOnly(const string& path)
{
	struct stat st;
	// @TODO: Error handling
	/*int res = */ stat(path.c_str(), &st);
	/*
	if (res)
		upipe.Log() << "Error stat on " << path << ": " << strerror(errno) << endl;
	 */
	
	return !(st.st_mode & S_IWUSR);
}

bool IsDirectory(const std::string& path)
{
	struct stat status;
	if (stat(path.c_str(), &status) != 0)
		return false;
	return S_ISDIR(status.st_mode);
}

bool EnsureDirectory(const string& path)
{
	int res = mkdir(path.c_str(), 0777);
	if (res == -1 && errno != EEXIST)
	{
		char buf[1024];
		snprintf(buf, 1024, "EnsureDirectory error %s for: %s\n", strerror(errno), path.c_str());
		throw std::runtime_error(buf);
	}
	return true;
}

bool PathExists(const std::string& path)
{
	return access(path.c_str(), F_OK) == 0;
}

bool MoveAFile(const string& fromPath, const string& toPath)
{
	int res = rename(fromPath.c_str(), toPath.c_str());
	return !res;
}
#endif
