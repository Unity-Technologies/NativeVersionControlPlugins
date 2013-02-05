#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

std::string IntToString (int i);
size_t Tokenize(std::vector<std::string>& result, const std::string& str, 
				const std::string& delimiters = " ");
std::string Join(const std::vector<std::string>& items, const std::string& delim = ", ", const std::string& surround = "");
std::string Replace(const std::string& str, const std::string& lookFor, const std::string& replaceWith);
std::string TrimStart(const std::string& str, char c = ' ');
std::string TrimEnd(const std::string& str, char c = ' ');
std::string Trim(const std::string& str, char c = ' ');
bool EndsWith(const std::string& str, const std::string& lookFor);
bool StartsWith(const std::string& str, const std::string& lookFor);
//bool IsReadOnly(const std::string& path);


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

// Run a command line read result.
// This is done blocking which is ok since Unity will timeout
class POpen
{
public:
	POpen(const std::string& cmd);
	~POpen();
	bool ReadLine(std::string& result);
	void ReadIntoFile(const std::string& path);
private:
	std::string m_Command;
#if defined(_WINDOWS)
#else
	FILE* m_Handle;
#endif	
};

typedef std::auto_ptr<POpen> APOpen;

#if defined(_WINDOWS)
#include <Windows.h>
std::string ErrorCodeToMsg( DWORD code );
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

