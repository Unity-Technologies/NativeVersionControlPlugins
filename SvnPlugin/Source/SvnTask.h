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

	// Setup
	void SetRepository(const std::string& p);
	const std::string& GetRepository() const;
	void SetUser(const std::string& u);
	const std::string& GetUser() const;
	void SetPassword(const std::string& p);
	const std::string& GetPassword() const;
	void SetOptions(const std::string& p);
	const std::string& GetOptions() const;
	void SetSvnExecutable(const std::string& e);
	const std::string& GetSvnExecutable() const;

	void SetAssetsPath(const std::string& p);
	const std::string& GetAssetsPath() const;
	std::string GetProjectPath() const;

	int Run();

	APOpen RunCommand(const std::string& cmd);
	void GetStatus(const VersionedAssetList& assets, VersionedAssetList& result, bool recursive);
	void GetStatusWithChangelists(const VersionedAssetList& assets, VersionedAssetList& result, 
								  std::vector<std::string>& changelistAssoc, bool recursive);
	void GetLog(SvnLogResult& result, const std::string& from, const std::string& to, bool includeAssets = false);

	void NotifyOffline(const std::string& reason, bool invalidWorkingCopy = false);
	void NotifyOnline();
	bool IsOnline() { return m_IsOnline; }

private:
	//	bool Dispatch(UnityCommand c, const std::vector<std::string>& args);

	std::string GetCredentials() const;
	bool GetStatusWithChangelists(const VersionedAssetList& assets, VersionedAssetList& result, 
								  std::vector<std::string>& changelistAssoc, 
								  const char* depth, bool queryRemote);
	Task* m_Task;
	std::string m_SvnPath;
	std::string m_AssetsPath;

	std::string m_RepositoryConfig;
	std::string m_UserConfig;
	std::string m_PasswordConfig;
	std::string m_OptionsConfig;
	
	bool m_IsOnline;
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
