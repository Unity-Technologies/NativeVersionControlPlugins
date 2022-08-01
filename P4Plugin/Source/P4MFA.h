#pragma once
#include "Connection.h"

class P4MFA
{
public:
	P4MFA();
	~P4MFA();
    
    bool WaitForHelixMFA(Connection& conn, std::string port, std::string user, std::string& error);

    static P4MFA* s_Singleton;
};