#include "Utility.h"

using namespace std;

size_t Tokenize(std::vector<std::string>& result,
				const std::string& str, 
				const std::string& delimiters)
{
	string::size_type startPos = 0;
	string::size_type endPos = str.find_first_of(delimiters);
	
	result.clear();
	string::size_type strLen = str.length();
	
	while (endPos != string::npos) {
		
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


// Replace all occurences of lookFor in str by replaceWith
string Replace(const string& str, const string& lookFor, const string& replaceWith)
{
	string result(str);
	size_t lflen = lookFor.length();
	size_t rwlen = replaceWith.length();
	
	string::size_type i1 = 0;
	string::size_type i2 = 0;
	do 
	{
		i2 = result.find(lookFor, i1);
		if (i2 == string::npos)
			return result;
		result.replace(i2, lflen, replaceWith);
		i1 = i2 + rwlen;
	}
	while (true) ;
	return result;
}

string TrimStart(const string& str, char c)
{
	string::size_type iend = str.length();
	string::size_type i1 = 0;
	
	while ( i1 < iend && str[i1] == c) 
	{
		++i1;
	}
	
	if (i1 >= iend) return "";
	
	return str.substr(i1);
}

string TrimEnd(const string& str, char c)
{
	string::size_type i1 = str.length() - 1;
	
	while ( i1 >= 0 && str[i1] == c) 
	{
		--i1;
	}
	
	if (i1 < 0) return "";
	
	return str.substr(0, i1+1);
}

string Trim(const string& str, char c)
{
	// Could be optimized
	return TrimStart(TrimEnd(str,c), c);
}

bool EndsWith(const string& str, const string& lookFor)
{
	if (str.length() < lookFor.length()) return false;
	
	string::const_reverse_iterator i1 = str.rbegin();
	string::const_reverse_iterator i2 = lookFor.rbegin();
	
	for ( ; i2 != lookFor.rend(); i2++, i1++)
		if (*i1 != *i2) return false;
	return true;
}

bool StartsWith(const string& str, const string& lookFor)
{
	if (str.length() < lookFor.length()) return false;
	
	string::const_iterator i1 = str.begin();
	string::const_iterator i2 = lookFor.begin();
	
	for ( ; i2 != lookFor.end(); i2++, i1++)
		if (*i1 != *i2) return false;
	return true;
}
/*
bool IsReadOnly(const std::string& path)
{
	// TODO implement
	return false;
}
*/

#if defined(_WINDOWS)
#include <stdio.h>
string ErrorCodeToMsg( DWORD code )
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
		string msg = (const char*)msgBuf;
		LocalFree( msgBuf );
		return msg;
	}
}

#endif