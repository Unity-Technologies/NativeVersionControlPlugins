#ifndef AESPLUGIN_H
#define AESPLUGIN_H

#include "VersionControlPlugin.h"
#include "Connection.h"
#include "AESClient.h"

#include <string>

// AES main task
class AESPlugin: public VersionControlPlugin
{
public:
    AESPlugin(int argc, char** argv);
    AESPlugin(const char* args);
    ~AESPlugin();
    
    inline const char* GetLogFileName() { return "./Library/aesPlugin.log"; }
    const VersionControlPluginVersions& GetSupportedVersions() { return m_Versions; }
    inline const TraitsFlags GetSupportedTraitFlags()
    {
        return (kRequireNetwork | kEnablesCheckout | kEnablesChangelists | kEnablesLocking | kEnablesGetLatestOnChangeSetSubset | kEnablesRevertUnchanged);
    }
    VersionControlPluginCfgFields& GetConfigFields() { return m_Fields; }
    const VersionControlPluginOverlays& GetOverlays() { return m_Overlays; };
    inline CommandsFlags GetOnlineUICommands()
    {
        return (kAdd | kChanges | kDelete | kDownload | kGetLatest | kIncomingChangeAssets | kIncoming | kStatus | kSubmit | kCheckout | kLock | kUnlock);
    }
    inline CommandsFlags GetOfflineUICommands() { return kNone; }

    int Connect();
    void Disconnect();
    inline bool IsConnected() { return m_IsConnected; }
	
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
    bool SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList);
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

private:
    void Initialize();
	
	bool RefreshSnapShot();
	bool SaveSnapShotToFile(const std::string& path);
	bool RestoreSnapShotFromFile(const std::string& path);
	
	void AddAssetsToChanges(const VersionedAssetList& assetList, int state);
	void RemoveAssetsFromChanges(const VersionedAssetList& assetList, int state = kNone);
    
	void EntriesToAssets(const MapOfEntries& entries, VersionedAssetList& assetList, int state = -1);
	void EntriesToAssets(const AESEntries& entries, VersionedAssetList& assetList, int state = -1);

    bool m_IsConnected;
    
    std::string BuildRemotePath(const VersionedAsset& asset);
    
    VersionControlPluginCfgFields m_Fields;

    VersionControlPluginVersions m_Versions;
    VersionControlPluginOverlays m_Overlays;
    
    ChangelistRevision m_SnapShotRevision;
	MapOfEntries m_SnapShotEntries;
	MapOfEntries m_ChangedEntries;

	ChangelistRevisions m_Revisions;

    AESClient* m_AES;
};


#endif // AESPLUGIN_H