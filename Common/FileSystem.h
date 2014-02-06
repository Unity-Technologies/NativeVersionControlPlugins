/*
 * File system related functions
 */
#pragma once
#include <string>

bool IsReadOnly(const std::string& path);
bool EnsureDirectory(const std::string& path);
bool DeleteRecursive(const std::string& path);
bool CopyAFile(const std::string& fromPath, const std::string& toPath, bool createMissingFolders);
bool MoveAFile(const std::string& fromPath, const std::string& toPath);
bool IsDirectory(const std::string& path);
bool PathExists(const std::string& path);
bool ChangeCWD(const std::string& path);
size_t GetFileLength(const std::string& path);

#if WIN32
#include "windows.h"
const size_t kDefaultPathBufferSize = 1024;
void ConvertUnityPathName( const char* utf8, wchar_t* outBuffer, int outBufferSize );
std::string PluginPath();
#endif
