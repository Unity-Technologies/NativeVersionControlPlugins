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
    AESPlugin();
    AESPlugin(int argc, char** argv);
    AESPlugin(const char* args);
    ~AESPlugin();
    
    inline const char* GetLogFileName() { return "./Library/aesPlugin.log"; }
    inline const VersionControlPluginVersions& GetSupportedVersions() { return m_Versions; }
    inline const TraitsFlags GetSupportedTraitFlags() {
        return (kRequireNetwork | kEnablesCheckout | kEnablesGetLatestOnChangeSetSubset );
    }
    inline VersionControlPluginCfgFields& GetConfigFields() { return m_Fields; }
    virtual const VersionControlPluginOverlays& GetOverlays() { return m_Overlays; };
    CommandsFlags GetOnlineUICommands();

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
    
    bool m_IsConnected;
    
    VersionControlPluginCfgFields m_Fields;

    VersionControlPluginVersions m_Versions;
    VersionControlPluginOverlays m_Overlays;
    
    AESClient* m_AES;
};

#endif // AESPLUGIN_H