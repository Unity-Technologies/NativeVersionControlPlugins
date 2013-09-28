#include "AESPlugin.h"

#include <set>

#if defined(_WINDOWS)
#include "windows.h"
#endif

using namespace std;

enum AESFields { kAESURL, kAESUserName, kAESPassword };

AESPlugin::AESPlugin() :
    VersionControlPlugin(),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::AESPlugin(int argc, char** argv) :
    VersionControlPlugin(argc, argv),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::AESPlugin(const char* args) :
    VersionControlPlugin(args),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::~AESPlugin()
{
    Disconnect();
}

void AESPlugin::Initialize()
{
    m_Fields.reserve(3);
    m_Fields.push_back(VersionControlPluginCfgField("aesURL", "URL", "The AES URL", "", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField("aesUsername", "Username", "The AES username", "", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField("aesPassword", "Password", "The AES password", "", VersionControlPluginCfgField::kPasswordField));
    
    m_Versions.insert(kUnity43);
    
    m_Overlays[kLocal] = "default";
    m_Overlays[kOutOfSync] = "default";
    m_Overlays[kCheckedOutLocal] = "default";
    m_Overlays[kCheckedOutRemote] = "default";
    m_Overlays[kDeletedLocal] = "default";
    m_Overlays[kDeletedRemote] = "default";
    m_Overlays[kAddedLocal] = "default";
    m_Overlays[kAddedRemote] = "default";
    m_Overlays[kConflicted] = "default";
}

VersionControlPlugin::CommandsFlags AESPlugin::GetOnlineUICommands()
{
    return (kAdd | kChangeDescription | kChangeMove | kChanges | kDelete | kDownload | kGetLatest | kIncomingChangeAssets | kIncoming | kStatus | kSubmit);
}

int AESPlugin::Connect()
{
    GetConnection().Log().Debug() << "Connect" << Endl << Flush;
    if (IsConnected())
        return 0;
    
    m_AES = new AESClient();
    m_IsConnected = m_AES->Initialize();
    
    return (m_IsConnected ? 0 : 1);
}

void AESPlugin::Disconnect()
{
    GetConnection().Log().Debug() << "Disconnect" << Endl << Flush;
    if (!IsConnected())
        return;
    
    delete m_AES;
    m_IsConnected = false;
}

int AESPlugin::Login()
{
    GetConnection().Log().Debug() << "Login" << Endl << Flush;
    if (!IsConnected())
        return -1;
    
    return 0;
}

bool DummyPerform(VersionedAssetList& assetList, int state)
{
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
            assetList.erase(i);
            continue;
        }
        
        if (state > 0)
        {
            if (asset.IsMeta()) state |= kMetaFile;
            asset.SetState(state);
        }
        
        asset.AddState(kLocal);
        asset.RemoveState(kUpdating);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::AddAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, kLocal|kSynced);
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, kLocal|kSynced|kCheckedOutLocal);
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, kLocal|kSynced);
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, kLocal|kSynced);
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, -1);
}

bool AESPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    return DummyPerform(assetList, kLocal|kSynced);
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    return DummyPerform(toAssetList, kLocal|kSynced);
}

bool AESPlugin::LockAssets(VersionedAssetList& assetList)
{
    return false;
}

bool AESPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    return false;
}

bool AESPlugin::ChangeOrMoveAssets(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    return DummyPerform(assetList, -1);
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList)
{
    return DummyPerform(assetList, -1);
}

bool AESPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    return DummyPerform(assetList, -1);
}

bool AESPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    return true;
}

bool AESPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    return true;
}

bool AESPlugin::GetAssetsChanges(Changes& changes)
{
    return true;
}

bool AESPlugin::GetAssetsIncomingChanges(Changes& changes)
{
    return true;
}

bool AESPlugin::UpdateRevision(const ChangelistRevision& revision)
{
    return true;
}

bool AESPlugin::DeleteRevision(const ChangelistRevision& revision)
{
    return true;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    return true;
}
