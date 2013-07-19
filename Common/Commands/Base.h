#pragma once

class BaseRequest
{
public:
	BaseRequest(const CommandArgs& args, Connection& conn) : args(args), conn(conn), invalid(false)
	{
	}  

	const CommandArgs& args;
	Connection& conn;
	bool invalid;
};

class BaseResponse
{
public:
};
