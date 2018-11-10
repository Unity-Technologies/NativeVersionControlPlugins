#pragma once
#include "TestCommand.h"

template<>
class TestCommand<CheckoutRequest>
{
public:
	bool Run(TestTask& task, CheckoutRequest& req, CheckoutResponse& resp)
	{
		req.conn.Log().Notice() << "[CheckoutRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
