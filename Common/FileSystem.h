/*
 * File system related functions
 */
#pragma once
#include <string>

bool IsReadOnly(const std::string& path);
bool EnsureDirectory(const std::string& path);
bool CopyAFile(const std::string& fromPath, const std::string& toPath, bool createMissingFolders);
bool MoveAFile(const std::string& fromPath, const std::string& toPath);
bool IsDirectory(const std::string& path);
bool PathExists(const std::string& path);

#if WIN32
#include "windows.h"
void ConvertUnityPathName( const char* utf8, wchar_t* outBuffer, int outBufferSize );
std::string PluginPath();
#endif
