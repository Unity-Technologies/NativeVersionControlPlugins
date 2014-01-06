#ifndef AESPLUGIN_H
#define AESPLUGIN_H

#include "VersionControlPlugin.h"
#include "Connection.h"
#include "AESClient.h"
#include "RadixTree.h"

#include <string>
#include <time.h>

// AES main task
class AESPlugin: public VersionControlPlugin
{
public:
    AESPlugin(int argc, char** argv);
    AESPlugin(const char* args);
    ~AESPlugin();
    
    const char* GetLogFileName();
    const VersionControlPluginVersions& GetSupportedVersions() { return m_Versions; }
    const TraitsFlags GetSupportedTraitFlags();
    VersionControlPluginCfgFields& GetConfigFields() { return m_Fields; }
    const VersionControlPluginOverlays& GetOverlays() { return m_Overlays; }
	const VersionControlPluginCustomCommands& GetCustomCommands() { return m_CustomCommands; }

    CommandsFlags GetOnlineUICommands();
    inline CommandsFlags GetOfflineUICommands() { return kNone; }

    int Connect();
    void Disconnect();
    inline bool IsConnected() { return m_IsConnected; }

	int Test();
	
protected:
    
    int Login();
    bool CheckConnectedAndLogged();
    
    bool AddAssets(VersionedAssetList& assetList);
    bool CheckoutAssets(VersionedAssetList& assetList);
    bool DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList);
    bool GetAssets(VersionedAssetList& assetList);
    bool RevertAssets(VersionedAssetList& assetList);
    bool ResolveAssets(VersionedAssetList& assetList, ResolveMethod method = kMine);
    bool RemoveAssets(VersionedAssetList& assetList);
    bool MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList);
    bool LockAssets(VersionedAssetList& assetList);
    bool UnlockAssets(VersionedAssetList& assetList);
    bool SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList, bool saveOnly = false);
    bool SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode);
    bool GetAssetsStatus(VersionedAssetList& assetList, bool recursive = false);
    bool GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList);
    bool GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList);
    bool GetAssetsChanges(Changes& changes);
    bool GetAssetsIncomingChanges(Changes& changes);
    bool SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList);
    bool UpdateRevision(const ChangelistRevision& revision, std::string& description);
    bool DeleteRevision(const ChangelistRevision& revision);
    bool RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList);
	bool PerformCustomCommand(const std::string& command);

private:
    void Initialize();
	
	bool RefreshRemote();
	bool RefreshLocal();
	bool RefreshSnapShot();
	
	bool SaveSnapShotToFile(const std::string& path);
	bool RestoreSnapShotFromFile(const std::string& path);
	
	void AddAssetsToChanges(const VersionedAssetList& assetList, int state);
	void RemoveAssetsFromChanges(const VersionedAssetList& assetList, int state = kNone);
    
	static int EntryToAssetCallBack(void *data, const std::string& key, AESEntry *entry);
	void EntriesToAssets(TreeOfEntries& entries, VersionedAssetList& assetList, int ignoredState = kNone);
	
	bool FetchAllAssets();
	bool ReconcileAssets();
	
	void ResetTimer();
	time_t GetTimerSoFar();
    bool TimerTick();
    
    bool m_IsConnected;
	        
    VersionControlPluginCfgFields m_Fields;

    VersionControlPluginVersions m_Versions;
    VersionControlPluginOverlays m_Overlays;
    VersionControlPluginCustomCommands m_CustomCommands;
    
    ChangelistRevision m_LatestRevision;
    ChangelistRevision m_SnapShotRevision;
	TreeOfEntries m_SnapShotEntries;
	TreeOfEntries m_LocalChangesEntries;
	TreeOfEntries m_RemoteChangesEntries;

	ChangelistRevisions m_Revisions;
	
	time_t m_LastRemoteChangesRefrehTime;
	
	time_t m_Timer;
	time_t m_LastTimer;

    AESClient* m_AES;

	static int ScanLocalChangeCallBack(void* data, const std::string& path, uint64_t size, bool isDirectory, time_t ts);
	static int CleanupLocalCallBack(void* data, const std::string& path, uint64_t size, bool isDirectory, time_t ts);
	
	static int EntryToJSONCallBack(void *data, const std::string& key, AESEntry *entry);
	static int ApplySnapShotChangeCallBack(void *data, const std::string& key, AESEntry *entry);
	static int ApplyRemoteChangesCallBack(void *data, const std::string& key, AESEntry *entry);
	static int ApplyLocalChangesCallBack(void *data, const std::string& key, AESEntry *entry);
	static int CheckConflictCallBack(void *data, const std::string& key, AESEntry *entry);
	static int FetchAllCallBack(void *data, const std::string& key, AESEntry *entry);
	static int PrintAllCallBack(void *data, const std::string& key, AESEntry *entry);
};


#endif // AESPLUGIN_H