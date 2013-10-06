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
    string prefix = "vcAssetExchangeServer"; // Mandatory, MUST match 'vc' + plugin executable name

    m_Fields.reserve(3);
    m_Fields.push_back(VersionControlPluginCfgField(prefix + "URL", "URL", "The AES URL", "https://localhost:3030/files/Clock", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(prefix + "Username", "Username", "The AES username", "emmanuelh@unity3d.com", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(prefix + "Password", "Password", "The AES password", "ignored4now", VersionControlPluginCfgField::kPasswordField));
    
    m_Versions.insert(kUnity43);
    
    m_Overlays[kLocal] = "default";
    m_Overlays[kOutOfSync] = "default";
    m_Overlays[kDeletedLocal] = "default";
    m_Overlays[kDeletedRemote] = "default";
    m_Overlays[kAddedLocal] = "default";
    m_Overlays[kAddedRemote] = "default";
    m_Overlays[kConflicted] = "default";
    
    m_CurrRevision = "current";

    Changelist defaultItem;
    defaultItem.SetDescription(m_CurrRevision);
    defaultItem.SetRevision(kDefaultListRevision);
    m_Revisions.push_back(defaultItem);
}

VersionControlPlugin::CommandsFlags AESPlugin::GetOnlineUICommands()
{
    return (kAdd | kChanges | kDelete | kDownload | kGetLatest | kIncomingChangeAssets | kIncoming | kStatus | kSubmit);
}

int AESPlugin::Connect()
{
    GetConnection().Log().Debug() << "Connect" << Endl;
    if (IsConnected())
        return 0;
    
    m_AES = new AESClient(m_Fields[kAESURL].GetValue());
    if (!m_AES->Ping())
    {
        GetConnection().Log().Debug() << "Connect failed " << m_AES->GetLastError() << Endl;
        //SetOnline();
        //NotifyOffline(m_AES->GetLastError());
        return -1;
    }
    
    m_IsConnected = true;
    return 0;
}

void AESPlugin::Disconnect()
{
    GetConnection().Log().Debug() << "Disconnect" << Endl;
    SetOffline();

    if (!IsConnected())
        return;
    
    delete m_AES;
    m_IsConnected = false;
}

int AESPlugin::Login()
{
    GetConnection().Log().Debug() << "Login" << Endl;
    if (!IsConnected())
        return -1;
    
    if (!m_AES->Login(m_Fields[kAESUserName].GetValue(), m_Fields[kAESPassword].GetValue()))
    {
        GetConnection().Log().Debug() << "Login failed " << m_AES->GetLastError() << Endl;
        return -1;
    }
    
    return 0;
}

bool AESPlugin::CheckConnectedAndLogged()
{
    GetConnection().Log().Debug() << "CheckConnectedAndLogged" << Endl;
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot connect to VC system"));
            return false;
        }
        GetConnection().Log().Debug() << "Connect successfully" << Endl;
        
        if (Login() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot log to to VC system"));
            return false;
        }
        GetConnection().Log().Debug() << "Logged successfully" << Endl;
    }
    
    return true;
}

string AESPlugin::BuildRemotePath(const VersionedAsset& asset)
{
    const string path = asset.GetPath();
    return m_Fields[kAESURL].GetValue() + '/' + m_CurrRevision + path.substr(GetProjectPath().size());
}

bool AESPlugin::AddAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "AddAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        if (asset.HasState(kAddedLocal) || !asset.HasState(kLocal))
        {
            assetList.erase(i);
            continue;
        }
        
        if (asset.IsFolder())
        {
            asset.SetState(kSynced | kAddedLocal);
            asset.RemoveState(kUpdating);
            
            m_Outgoing.insert(make_pair(asset.GetPath(), asset));
            
            assetList.erase(i);
            continue;
        }
        
        int state = kSynced | kAddedLocal;
        if (asset.IsMeta()) state |= kMetaFile;
        
        asset.SetState(state);
        asset.RemoveState(kUpdating);
        
        m_Outgoing.insert(make_pair(asset.GetPath(), asset));
        
        i++;
    }
    
    return true;
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    return false;
}

bool AESPlugin::DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        if (asset.HasState(kAddedLocal))
        {
            assetList.erase(i);
            continue;
        }
        
        if (asset.IsFolder())
        {
            assetList.erase(i);
            continue;
        }
        
        //CopyAFile(localPath, path, true);
        
        int state = kLocal | kSynced;
        if (asset.IsMeta()) state |= kMetaFile;
        
        asset.SetState(state);
        asset.RemoveState(kUpdating);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        if (asset.IsFolder())
        {
            assetList.erase(i);
            continue;
        }
        
        /*
        if (PathExists(localPath))
        {
            if (!(asset.HasState(kCheckedOutLocal) || asset.HasState(kLockedLocal)) && CompareFiles(localPath, path) >= 0)
            {
                CopyAFile(localPath, path, true);
                asset.AddState(kSynced);
                asset.RemoveState(kOutOfSync);
            }
        }
        else
        {
            asset.RemoveState(kSynced);
            asset.RemoveState(kOutOfSync);
        }
         */
        
        asset.RemoveState(kUpdating);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        VersionedAssetMap::iterator j = m_Outgoing.find(asset.GetPath());
        if (j == m_Outgoing.end())
        {
            assetList.erase(i);
            continue;
        }
        
        asset.RemoveState(kCheckedOutLocal);
        asset.RemoveState(kLockedLocal);
        asset.RemoveState(kAddedLocal);
        
        //VersionedAsset& managedAsset = j->second;
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        m_Outgoing.erase(j);
        
        i++;
    }
    return true;
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        //VersionedAsset& asset = (*i);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RemoveAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        //VersionedAsset& asset = (*i);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    GetConnection().Log().Debug() << "MoveAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        toAssetList.clear();
        return true;
    }
    
    VersionedAssetList::const_iterator i = fromAssetList.begin();
    VersionedAssetList::iterator j = toAssetList.begin();
    while (i != fromAssetList.end() && j != toAssetList.end())
    {
        //const VersionedAsset& fromAsset = (*i);
        //VersionedAsset& toAsset = (*j);
        
        i++; j++;
    }
    
    return true;
}

bool AESPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;
    return false;
}

bool AESPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;
    return false;
}

bool AESPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SetRevision" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        //VersionedAsset& asset = (*i);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        VersionedAssetMap::iterator j = m_Outgoing.find(asset.GetPath());
        if (j == m_Outgoing.end())
        {
            assetList.erase(i);
            continue;
        }
        
        VersionedAsset& managedAsset = j->second;
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        if (managedAsset.IsFolder() && (managedAsset.HasState(kCheckedOutLocal) || managedAsset.HasState(kAddedLocal)))
        {
            asset.SetState(kSynced);
            //GetConnection().Log().Debug() << "Create directory " << localPath << Endl;
            //EnsureDirectory(localPath);
        }
        else if (managedAsset.IsFolder() && managedAsset.HasState(kDeletedLocal))
        {
            asset.SetState(kNone);
            //GetConnection().Log().Debug() << "Delete directory " << localPath << Endl;
            //DeleteRecursive(localPath);
        }
        else if (!managedAsset.IsFolder() && (managedAsset.HasState(kCheckedOutLocal) || managedAsset.HasState(kAddedLocal)))
        {
            asset.SetState(kSynced);
            if (asset.IsMeta())
            {
                asset.AddState(kMetaFile);
            }
            //GetConnection().Log().Debug() << "Copy file " << path << " to " << localPath << Endl;
            //CopyAFile(path, localPath, true);
        }
        else if (!managedAsset.IsFolder() && managedAsset.HasState(kDeletedLocal))
        {
            m_Outgoing.erase(j);
            //GetConnection().Log().Debug() << "Delete file " << localPath << Endl;
            //DeleteRecursive(localPath);
            assetList.erase(i);
        }
        
        m_Outgoing.erase(j);
        
        i++;
    }
    return true;
}

bool AESPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    GetConnection().Log().Debug() << "GetAssetsStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        
        if (asset.IsFolder())
        {
            assetList.erase(i);
            continue;
        }
        
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        int state = asset.GetState();
        if (m_AES->Exists(m_CurrRevision, path.substr(GetProjectPath().size())))
        {
            GetConnection().Log().Debug() << "Found AES entry" << Endl;
            state &= ~kLocal;
            state |= kSynced;
            state &= ~kOutOfSync;
        }
        else
        {
            state |= kLocal;
            state &= ~kSynced;
            state &= ~kOutOfSync;
        }
        
        asset.SetState(state);
        if (asset.HasState(kAddedLocal) || asset.HasState(kDeletedLocal) || asset.HasState(kCheckedOutLocal) || asset.HasState(kLockedLocal))
        {
            m_Outgoing.insert(make_pair(asset.GetPath(), asset));
        }
        
        i++;
    }
    
    return true;
}

bool AESPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    assetList.clear();
    for (VersionedAssetMap::const_iterator i = m_Outgoing.begin() ; i != m_Outgoing.end() ; i++)
    {
        const VersionedAsset& asset = i->second;
        string path = asset.GetPath();
        string remotePath = BuildRemotePath(asset);
        GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
        
        if (!asset.IsFolder())
        {
            assetList.push_back(asset);
        }
    }
    return true;
}

bool AESPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetIncomingAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    assetList.clear();
    return true;
}

bool AESPlugin::GetAssetsChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsChanges" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        changes.clear();
        return true;
    }
    
    changes.clear();
    for (Changes::const_iterator i = m_Revisions.begin() ; i != m_Revisions.end() ; i++)
    {
        changes.push_back(*i);
    }
    
    if (m_Outgoing.size() != 0)
    {
    }
    
    return true;
}

bool AESPlugin::GetAssetsIncomingChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsIncomingChanges" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        changes.clear();
        return true;
    }
    
    return true;
}

bool AESPlugin::UpdateRevision(const ChangelistRevision& revision, string& description)
{
    GetConnection().Log().Debug() << "UpdateRevision" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        return true;
    }
    
    description.clear();
    
    return true;
}

bool AESPlugin::DeleteRevision(const ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "DeleteRevision" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        return true;
    }
    
    return true;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        //VersionedAsset& asset = (*i);
        
        i++;
    }
    
    return true;
}