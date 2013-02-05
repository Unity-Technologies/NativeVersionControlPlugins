#pragma once
#include "Task.h"
#include "VersionedAsset.h"
#include "Utility.h"

class SvnLogResult;

class SvnTask 
{
public:
	SvnTask();
	~SvnTask();

	/*
	// Setup
	void SetPort(const std::string& p);
	void SetUser(const std::string& u);
	std::string GetUser();
	void SetPassword(const std::string& p);
	const std::string& GetPassword() const;
	void SetAssetsPath(const std::string& p);
	const std::string& GetAssetsPath() const;
	*/

	void SetAssetsPath(const std::string& p);
	const std::string& GetAssetsPath() const;
	std::string GetProjectPath() const;

	int Run();

	APOpen RunCommand(const std::string& cmd);
	void GetStatus(const VersionedAssetList& assets, VersionedAssetList& result, bool recursive);
	void GetStatusWithChangelists(const VersionedAssetList& assets, VersionedAssetList& result, 
								  std::vector<std::string>& changelistAssoc, bool recursive);
	void GetLog(SvnLogResult& result, const std::string& from, const std::string& to, bool includeAssets = false);

private:
	//	bool Dispatch(UnityCommand c, const std::vector<std::string>& args);

	void GetStatusWithChangelists(const VersionedAssetList& assets, VersionedAssetList& result, 
								  std::vector<std::string>& changelistAssoc, 
								  const char* depth);
	Task* m_Task;
	std::string m_SvnPath;
	std::string m_AssetsPath;

	/*
	std::string m_PortConfig;
	std::string m_UserConfig;
	std::string m_PasswordConfig;
	std::string m_AssetsPathConfig;
	*/
};

class SvnException : public std::exception
{
public:
	SvnException(const std::string& about);
	~SvnException() throw() {} 
	virtual const char* what() const throw();
private:
	std::string m_What;
};
