#ifndef FSPLUGIN_H
#define FSPLUGIN_H

#include "VersionControlPlugin.h"
#include "Connection.h"

#include <string>
#include <map>

// FS main task
class FSPlugin: public VersionControlPlugin
{
public:
    FSPlugin(int argc, char** argv);
    FSPlugin(const char* args);
    ~FSPlugin();
    
    inline const char* GetLogFileName() { return "./Library/fsPlugin.log"; }
    inline const VersionControlPluginVersions& GetSupportedVersions() { return m_Versions; }
    inline const TraitsFlags GetSupportedTraitFlags() {
        return (kRequireNetwork | kEnablesCheckout | kEnablesGetLatestOnChangeSetSubset );
    }
    inline VersionControlPluginCfgFields& GetConfigFields() { return m_Fields; }
    inline const VersionControlPluginOverlays& GetOverlays() { return m_Overlays; };
    inline CommandsFlags GetOnlineUICommands()
    {
        return (kAdd | kDelete | kDownload | kGetLatest | kStatus | kSubmit);
    }
    inline CommandsFlags GetOfflineUICommands() { return kNone; }

    int Connect();
    void Disconnect();
    inline bool IsConnected() { return m_IsConnected; }

protected:
    
    int Login();

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
    
    bool CheckConnectedAndLogged();

    bool m_IsConnected;
    
    VersionControlPluginCfgFields m_Fields;

    VersionControlPluginVersions m_Versions;
    VersionControlPluginOverlays m_Overlays;
    
    typedef std::map<std::string, VersionedAsset> VersionedAssetMap;
    VersionedAssetMap m_Changes;
    VersionedAssetMap m_IncomingChanges;
};

#endif // FSPLUGIN_H