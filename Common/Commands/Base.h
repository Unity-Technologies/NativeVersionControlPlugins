#pragma once

class BaseRequest
{
public:
	BaseRequest(const CommandArgs& args, UnityConnection& conn) : args(args), conn(conn), invalid(false)
	{
	}  

	UnityPipe& Pipe() 
	{
		return conn.Pipe();
	}

	const CommandArgs& args;
	UnityConnection& conn;
	bool invalid;
};

class BaseResponse
{
public:
};
