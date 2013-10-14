#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include "POpen.h"

std::string IntToString (int i);
size_t Tokenize(std::vector<std::string>& result, const std::string& str, 
				const std::string& delimiters = " ");
std::string Join(const std::vector<std::string>::const_iterator i1, 
				 const std::vector<std::string>::const_iterator i2,
				 const std::string& delim = ", ", const std::string& surround = "");
std::string Join(const std::vector<std::string>& items, const std::string& delim = ", ", const std::string& surround = "");
std::string Replace(const std::string& str, const std::string& lookFor, const std::string& replaceWith);
std::string TrimStart(const std::string& str, char c = ' ');
std::string TrimEnd(const std::string& str, char c = ' ');
std::string Trim(const std::string& str, char c = ' ');
bool EndsWith(const std::string& str, const std::string& lookFor);
bool StartsWith(const std::string& str, const std::string& lookFor);
//bool IsReadOnly(const std::string& path);
std::string Quote(const std::string& str);

template <typename T> 
std::string ToString(const T& v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
}

template <typename T> 
std::string ToString(const T& v1, const T& v2)
{
	std::stringstream ss;
	ss << v1 << v2;
	return ss.str();
}

template <typename T> 
std::string ToString(const char* v1, const T& v2)
{
	std::stringstream ss;
	ss << v1 << v2;
	return ss.str();
}

template <typename T> 
std::string ToString(const T& v1, const T& v2, const T& v3)
{
	std::stringstream ss;
	ss << v1 << v2 << v3;
	return ss.str();
}

#if defined(_WINDOWS)
std::string ErrorCodeToMsg( DWORD code );
std::string LastErrorToMsg();
#endif

class PluginException : public std::exception
{
public:
	PluginException(const std::string& about);
	~PluginException() throw() {} 
	virtual const char* what() const throw();
private:
	std::string m_What;
};

template<class Ex, typename P1>
void Enforce(bool cond, const P1& exParam1)
{
	if (!cond)
		throw Ex(exParam1);
}

template<class Ex, typename P1, typename P2>
void Enforce(bool cond, const P1& exParam1, const P2& exParam2)
{
	if (!cond)
		throw Ex(exParam1, exParam2);
}

