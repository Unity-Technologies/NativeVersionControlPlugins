#pragma once
#include <stdio.h>
#include <string>
#include <vector>

size_t Tokenize(std::vector<std::string>& result, const std::string& str, 
				const std::string& delimiters = " ");
std::string Join(const std::vector<std::string>& items, const std::string& delim = ", ");
std::string Replace(const std::string& str, const std::string& lookFor, const std::string& replaceWith);
std::string TrimStart(const std::string& str, char c);
std::string TrimEnd(const std::string& str, char c);
std::string Trim(const std::string& str, char c);
bool EndsWith(const std::string& str, const std::string& lookFor);
bool StartsWith(const std::string& str, const std::string& lookFor);
//bool IsReadOnly(const std::string& path);


#if defined(_WINDOWS)
#include <Windows.h>
std::string ErrorCodeToMsg( DWORD code );
#endif
