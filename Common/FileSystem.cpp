#include "FileSystem.h"
#include <stdexcept>
#include <stdio.h>
#include <string.h>


using namespace std;

static string ParentDirectory(const string& path)
{
	if (path.empty()) return path;

	size_t i = string::npos;

	if (*path.rbegin() == '/')
		i = path.length() - 2;

	i = path.rfind('/', i);

	if (i == string::npos)
		return "";

	// include the ending / in the result
	return path.substr(0, i+1);
}

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
	string parent = ParentDirectory(path);
	if (!IsDirectory(parent) && !EnsureDirectory(parent))
		return false;

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
		upipe.Log().Notice() << "Error stat on " << path << endl;
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
	return PathIsDirectoryW(widePath) != FALSE; // Must be compared against FALSE and not TRUE!
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

bool CopyAFile(const string& fromPath, const string& toPath, bool createMissingFolders)
{
	if (createMissingFolders && !EnsureDirectory(ParentDirectory(toPath)))
		return false;

	wchar_t wideFrom[kDefaultPathBufferSize], wideTo[kDefaultPathBufferSize];
	ConvertUnityPathName( fromPath.c_str(), wideFrom, kDefaultPathBufferSize );
	ConvertUnityPathName( toPath.c_str(), wideTo, kDefaultPathBufferSize );

	BOOL b = FALSE;
	if( CopyFileExW( wideFrom, wideTo, NULL, NULL, &b, 0) )
		return true;
	
	if (ERROR_ACCESS_DENIED == GetLastError())
	{
		if (RemoveReadOnlyW(wideTo))
		{
			b = FALSE;
			if ( CopyFileExW( wideFrom, wideTo, NULL, NULL, &b, 0) )
				return true;
		}
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
		upipe.Log().Notice() << "Error stat on " << path << ": " << strerror(errno) << endl;
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
	string parent = ParentDirectory(path);
	if (!IsDirectory(parent) && !EnsureDirectory(parent))
		return false;

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

static bool fcopy(FILE *f1, FILE *f2)
{
    char            buffer[BUFSIZ];
    size_t          n;

    while ((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0)
    {
        if (fwrite(buffer, sizeof(char), n, f2) != n)
			return false;
    }
}

bool CopyAFile(const string& fromPath, const string& toPath, bool createMissingFolders)
{
	if (createMissingFolders && !EnsureDirectory(ParentDirectory(toPath)))
		return false;

    FILE *fp1;
    FILE *fp2;

    if ((fp1 = fopen(fromPath.c_str(), "rb")) == 0)
		return false;

	// Make sure the file does not exist already
	int l = unlink(toPath.c_str());
	if (l != 0 && errno != ENOENT && errno != ENOTDIR)
		return false; 

    if ((fp2 = fopen(toPath.c_str(), "wb")) == 0)
	{
		fclose(fp1);
		return false;
	}

	bool res = fcopy(fp1, fp2);
	fclose(fp1);
	fclose(fp2);
	return res;
}

bool MoveAFile(const string& fromPath, const string& toPath)
{
	int res = rename(fromPath.c_str(), toPath.c_str());
	return !res;
}
#endif
