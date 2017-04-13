#include "Utility.h"
#include <sstream>
#include <iterator>
#include <algorithm>
#include <functional>

std::string IntToString (int i)
{
	char buf[255];
#ifdef WIN32
	_snprintf (buf, sizeof(buf), "%i", i);
#else
	snprintf (buf, sizeof(buf), "%i", i);
#endif
	return std::string (buf);		
}

size_t Tokenize(std::vector<std::string>& result,
				const std::string& str, 
				const std::string& delimiters)
{
	std::string::size_type startPos = 0;
	std::string::size_type endPos = str.find_first_of(delimiters);
	
	result.clear();
	std::string::size_type strLen = str.length();
	
	while (endPos != std::string::npos) {
		
		std::string tok = str.substr(startPos, endPos-startPos);
		startPos = ++endPos;
		
		if (!tok.empty()) result.push_back(tok);
		
		if (startPos >= strLen) return result.size();
		
		endPos = str.find_first_of(delimiters, startPos);
	}
	
	std::string tok = str.substr(startPos);
	result.push_back(tok);
	
	return result.size();
}

std::string Join(const std::vector<std::string>& items, const std::string& delim, const std::string& surround)
{
	return Join(items.begin(), items.end(), delim, surround);
}

std::string Join(const std::vector<std::string>::const_iterator i1, 
			const std::vector<std::string>::const_iterator i2,
			const std::string& delim, const std::string& surround)
{
	std::string result;
	for (std::vector<std::string>::const_iterator i = i1; i != i2; ++i)
	{
		if (i != i1)
			result += delim;
		result += surround + *i + surround;
	}
	return result;
}

// Replace all occurences of lookFor in str by replaceWith
std::string Replace(const std::string& str, const std::string& lookFor, const std::string& replaceWith)
{
	std::string result(str);
	size_t lflen = lookFor.length();
	size_t rwlen = replaceWith.length();
	
	std::string::size_type i1 = 0;
	std::string::size_type i2 = 0;
	do 
	{
		i2 = result.find(lookFor, i1);
		if (i2 == std::string::npos)
			return result;
		result.replace(i2, lflen, replaceWith);
		i1 = i2 + rwlen;
	}
	while (true) ;
	return result;
}

std::string TrimStart(const std::string& str, char c)
{
	std::string::size_type iend = str.length();
	std::string::size_type i1 = 0;
	
	while ( i1 < iend && str[i1] == c) 
	{
		++i1;
	}
	
	if (i1 >= iend) return "";
	
	return str.substr(i1);
}

std::string TrimEnd(const std::string& str, char c)
{ 
   
	std::string::size_type i1 = str.length();
	
	while ( i1 != 0 && str[i1-1] == c) 
	{
		--i1;
	}
	
	if (i1 == 0) return "";
	
	return str.substr(0, i1);
}

std::string Trim(const std::string& str, char c)
{
	// Could be optimized
	return TrimStart(TrimEnd(str,c), c);
}

bool EndsWith(const std::string& str, const std::string& lookFor)
{
	if (str.length() < lookFor.length()) return false;
	
	std::string::const_reverse_iterator i1 = str.rbegin();
	std::string::const_reverse_iterator i2 = lookFor.rbegin();
	
	for ( ; i2 != lookFor.rend(); i2++, i1++)
		if (*i1 != *i2) return false;
	return true;
}

bool StartsWith(const std::string& str, const std::string& lookFor)
{
	if (str.length() < lookFor.length()) return false;
	
	std::string::const_iterator i1 = str.begin();
	std::string::const_iterator i2 = lookFor.begin();
	
	for ( ; i2 != lookFor.end(); i2++, i1++)
		if (*i1 != *i2) return false;
	return true;
}

std::string Quote(const std::string& str)
{
	std::string s = "\"";
	s += str;
	s += "\"";
	return s;
}

#if defined(_WINDOWS)
#include <stdio.h>
std::string ErrorCodeToMsg( DWORD code )
{
	void* msgBuf;
	if( !FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&msgBuf, 0, NULL ))
	{
		char buf[100];
		_snprintf( buf, 100, "Unknown error [%i]", code );
		return buf;
	}
	else
	{
		std::string msg = (const char*)msgBuf;
		LocalFree( msgBuf );
		return msg;
	}
}

std::string LastErrorToMsg()
{
	return ErrorCodeToMsg(GetLastError());
}

#endif

void ToLower( std::string& localPath ) 
{
	for (size_t i = 0; i < localPath.length(); ++i)
		localPath[i] = static_cast<char>(tolower(localPath[i]));
}

PluginException::PluginException(const std::string& about) : m_What(about) {}

const char* PluginException::what() const throw()
{
	return m_What.c_str();
}

#if defined(_WINDOWS)
std::string Backtrace()
{
  return "";
}
#elif defined(LINUX)

#else

#include <execinfo.h>
#include <stdio.h>

std::string Backtrace()
{
  void* callstack[128];
  int i, frames = backtrace(callstack, 128);
  char** strs = backtrace_symbols(callstack, frames);
  std::string result;
  for (i = 0; i < frames; ++i) {
    result += strs[i];
    result += "\n";
  }
  free(strs);
  return result;
}
#endif
