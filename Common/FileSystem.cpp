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
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

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
	return PathExists(path) && IsReadOnlyW(widePath);
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

bool ChangeCWD(const std::string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	return _wchdir(widePath) != -1;	
}

size_t GetFileLength(const std::string& pathName)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(pathName.c_str(), widePath, kDefaultPathBufferSize);
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (GetFileAttributesExW(widePath, GetFileExInfoStandard, &attrs) == 0)
	{
		throw std::runtime_error("Error getting file length attribute");
	}

	if (attrs.nFileSizeHigh)
		return UINT_MAX;
	return attrs.nFileSizeLow;
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

static bool RemoveDirectoryRecursiveWide( const std::wstring& path )
{
	if( path.empty() )
		return false;

	// base path
	std::wstring basePath = path;
	if( basePath[basePath.size()-1] != L'\\' )
		basePath += L'\\';

	// search pattern: anything inside the directory
	std::wstring searchPat = basePath + L'*';

	// find the first file
	WIN32_FIND_DATAW findData;
	HANDLE hFind = ::FindFirstFileW( searchPat.c_str(), &findData );
	if( hFind == INVALID_HANDLE_VALUE )
		return false;

	bool hadFailures = false;

	bool bSearch = true;
	while( bSearch )
	{
		if( ::FindNextFileW( hFind, &findData ) )
		{
			if( wcscmp(findData.cFileName,L".")==0 || wcscmp(findData.cFileName,L"..")==0 )
				continue;

			std::wstring filePath = basePath + findData.cFileName;
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				// we have found a directory, recurse
				if( !RemoveDirectoryRecursiveWide( filePath ) ) {
					hadFailures = true;
				} else {
					::RemoveDirectoryW( filePath.c_str() ); // remove the empty directory
				}
			}
			else
			{
				if( findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
					::SetFileAttributesW( filePath.c_str(), FILE_ATTRIBUTE_NORMAL );
				if( !::DeleteFileW( filePath.c_str() ) ) {
					hadFailures = true;
				}
			}
		}
		else
		{
			if( ::GetLastError() == ERROR_NO_MORE_FILES )
			{
				bSearch = false; // no more files there
			}
			else
			{
				// some error occurred
				::FindClose( hFind );
				return false;
			}
		}
	}
	::FindClose( hFind );

	if( !RemoveDirectoryW( path.c_str() ) ) { // remove the empty directory
		hadFailures = true;
	}

	return !hadFailures;
}

static bool RemoveDirectoryRecursive( const std::string& pathUtf8 )
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName( pathUtf8.c_str(), widePath, kDefaultPathBufferSize );
	return RemoveDirectoryRecursiveWide( widePath );
}

bool DeleteRecursive(const string& path)
{
	if( IsDirectory(path) )
		return RemoveDirectoryRecursive( path );
	else
	{
		wchar_t widePath[kDefaultPathBufferSize];
		ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);

		if (DeleteFileW(widePath))
		{
			return true;
		}

		#if UNITY_EDITOR

		if (ERROR_ACCESS_DENIED == GetLastError())
		{
			if (RemoveReadOnlyW(widePath))
			{
				return (FALSE != DeleteFileW(widePath));
			}
		}

		#endif

		return false;
	}
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
#include <unistd.h>
#include <dirent.h>

bool IsReadOnly(const string& path)
{
	struct stat st;
	// @TODO: Error handling
	int res = stat(path.c_str(), &st);

	if (res == -1)
	{
		// Notice if file actually exists by we could not stat
		if (errno != ENOENT && errno != ENOTDIR)
		{
			char buf[1024];
			snprintf(buf, 1024, "Could not stat %s error : %s (%i)\n", path.c_str(), strerror(errno), errno);
			throw std::runtime_error(buf);
		}
		return false;
	}

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

static int DeleteRecursiveHelper(const string& path)
{
	int res;

	struct stat status;
	res = lstat(path.c_str(), &status);
	if (res != 0)
		return res;

	if (S_ISDIR(status.st_mode) && !S_ISLNK(status.st_mode))
	{
		DIR *dirp = opendir (path.c_str());
		if (dirp == NULL)
			return -1;

		struct dirent *dp;
		while ( (dp = readdir(dirp)) )
		{
			if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
			{
				string name = dp->d_name;
				res = DeleteRecursiveHelper(path + "/" + name);
				if (res != 0)
				{
					closedir(dirp);
					return res;
				}
			}
		}
		closedir(dirp);

		res = rmdir(path.c_str());
	}
	else
	{
		res = unlink(path.c_str());
	}

	return res;
}

bool DeleteRecursive(const string& path)
{
	return DeleteRecursiveHelper(path) == 0;
}

bool PathExists(const std::string& path)
{
	return access(path.c_str(), F_OK) == 0;
}

bool ChangeCWD(const std::string& path)
{
	return chdir(path.c_str()) != -1;
}

size_t GetFileLength(const std::string& pathName)
{
	struct stat statbuffer;
	if( stat(pathName.c_str(), &statbuffer) != 0 )
	{
		throw std::runtime_error("Error getting file length");
	}
	
	return statbuffer.st_size;
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
