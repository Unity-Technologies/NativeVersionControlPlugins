#pragma once

template<>
class SvnCommand<ChangeDescriptionRequest>
{
public:
	bool Run(SvnTask& task, ChangeDescriptionRequest& req, ChangeDescriptionResponse& resp)
	{
		resp.description = "";
		resp.Write();
		return true;
	}
};
