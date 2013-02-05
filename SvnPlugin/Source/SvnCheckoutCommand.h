#pragma once

template<>
class SvnCommand<CheckoutRequest>
{
public:
	bool Run(SvnTask& task, CheckoutRequest& req, CheckoutResponse& resp)
	{
		// Checkout is not a svn feature
		resp.Write();
		return true;
	}
};
