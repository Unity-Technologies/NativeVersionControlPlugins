#pragma once
#include <string>
#include <vector>
#include <exception>
#include <cstdlib>
#include "Utility.h"

#if defined(_WINDOWS)
#include <sstream>
#include <windows.h>
#else
#include <iostream>
#endif

class PipeException : public std::exception
{
public:
	PipeException(const std::string& about) : m_What(about) {}
	~PipeException() throw() {}
	virtual const char* what() const throw() { return m_What.c_str(); }
private:
	std::string m_What;
};

// Pipe that uses unix pipes on mac and linux. NamedPipes on windows
struct Pipe {
public:
	Pipe();
	~Pipe();

	void Flush();

	template <typename T>
	Pipe& Write(const T& v)
	{
#if defined(_WINDOWS)
		std::stringstream ss;
		ss << v;
		std::string str = ss.str();
		Write(ss.str());
#else
		std::cout << v;
#endif
		return *this;
	}

	Pipe& Write(const std::string& str);
	std::string& ReadLine(std::string& target);
	std::string& PeekLine(std::string& dest);
	bool IsEOF() const;

private:
	bool m_LineBufferValid;
	std::string m_LineBuffer;

#if defined(_WINDOWS)
	HANDLE m_NamedPipe;
	std::string m_Buffer;
#endif
};
					
