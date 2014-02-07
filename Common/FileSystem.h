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
bool ReadAFile(const std::string& path, std::string& data);
bool WriteAFile(const std::string& path, const std::string& data);
size_t GetFileLength(const std::string& path);

#if WIN32 || LINUX
#include <stdint.h>
#endif
typedef int (*FileCallBack)(void* data, const std::string& path, uint64_t size, bool isDirectory, time_t ts);
bool ScanDirectory(const std::string& path, bool recurse, FileCallBack cb, void* data);

bool TouchAFile(const std::string& path, time_t ts);
bool GetAFileInfo(const std::string& path, uint64_t* size, bool* isDirectory, time_t* ts);

#if WIN32
#include "windows.h"
const size_t kDefaultPathBufferSize = 1024;
const size_t kDefaultBufferSize = 1024 * 4;
void ConvertUnityPathName( const char* utf8, wchar_t* outBuffer, int outBufferSize );
std::string PluginPath();
#endif