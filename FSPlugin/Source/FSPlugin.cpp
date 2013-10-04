#include "FSPlugin.h"
#include "FileSystem.h"

#include <set>
#include <sys/stat.h>

#if defined(_WINDOWS)
#include "windows.h"
#endif

using namespace std;

enum FSFields { kFSPath };

#if !defined(_WINDOWS)
int CompareFiles(const string& left, const string &right)
{
    struct stat sleft;
    if (stat(left.c_str(), &sleft) < 0)
        return -1;
    
    struct stat sright;
    if (stat(right.c_str(), &sright) < 0)
        return 1;
    
    if (sleft.st_mtimespec.tv_sec < sright.st_mtimespec.tv_sec)
        return -1;
    
    if (sleft.st_mtimespec.tv_sec > sright.st_mtimespec.tv_sec)
        return 1;
    
    /*
    if (sleft.st_mtimespec.tv_nsec < sright.st_mtimespec.tv_nsec)
        return -1;
     
    if (sleft.st_mtimespec.tv_nsec > sright.st_mtimespec.tv_nsec)
        return 1;
     */
    
    return 0;
}
#else 
int CompareFiles(const string& left, const string &right)
{
    return 0;
}
#endif

FSPlugin::FSPlugin() :
    VersionControlPlugin(),
    m_IsConnected(false)
{
    Initialize();
}

FSPlugin::FSPlugin(int argc, char** argv) :
    VersionControlPlugin(argc, argv),
    m_IsConnected(false)
{
    Initialize();
}

FSPlugin::FSPlugin(const char* args) :
    VersionControlPlugin(args),
    m_IsConnected(false)
{
    Initialize();
}

FSPlugin::~FSPlugin()
{
    Disconnect();
}

void FSPlugin::Initialize()
{
    string prefix = "vcFileSystem"; // Mandatory, MUST match 'vc' + plugin executable name

    m_Fields.reserve(1);
    m_Fields.push_back(VersionControlPluginCfgField(prefix + "Path", "Path", "The FS Path", "", VersionControlPluginCfgField::kRequiredField));
    
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

VersionControlPlugin::CommandsFlags FSPlugin::GetOnlineUICommands()
{
    return (kAdd | kDelete | kDownload | kGetLatest | kStatus | kSubmit);
}

int FSPlugin::Connect()
{
    GetConnection().Log().Debug() << "Connect" << Endl;
    if (IsConnected())
    {
        return 0;
    }
    
    if (!PathExists(m_Fields[kFSPath].GetValue()))
    {
        GetConnection().Log().Debug() << "Path " << m_Fields[kFSPath].GetValue() << " doesn't exists" << Endl;
        SetOffline();
        return -1;
    }
    
    m_IsConnected = true;
    return 0;
}

void FSPlugin::Disconnect()
{
    GetConnection().Log().Debug() << "Disconnect" << Endl;
    m_IsConnected = false;
    SetOffline();
}

int FSPlugin::Login()
{
    GetConnection().Log().Debug() << "Login" << Endl;
    return 0;
}

bool FSPlugin::CheckConnectedAndLogged()
{
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot connect to VC system"));
            return false;
        }

        if (Login() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot log to to VC system"));
            return false;
        }
    }

    return true;
}

bool FSPlugin::AddAssets(VersionedAssetList& assetList)
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
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;

        if (PathExists(localPath) || asset.HasState(kAddedLocal))
        {
            assetList.erase(i);
            continue;
        }
        
        if (asset.IsFolder())
        {
            asset.SetState(kLocal | kSynced | kAddedLocal);
            asset.RemoveState(kUpdating);

            m_Changes.insert(make_pair(asset.GetPath(), asset));
            
            assetList.erase(i);
            continue;
        }

        int state = kLocal | kSynced | kAddedLocal;
        if (asset.IsMeta()) state |= kMetaFile;
 
        asset.SetState(state);
        asset.RemoveState(kUpdating);

        m_Changes.insert(make_pair(asset.GetPath(), asset));
        
        i++;
    }
    
    return true;
}

bool FSPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    
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
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
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
        
        if (asset.HasState(kOutOfSync))
        {
            CopyAFile(localPath, path, true);
        }
        
        int state = kLocal | kSynced | kCheckedOutLocal;
        if (asset.IsMeta()) state |= kMetaFile;
        
        asset.SetState(state);
        asset.RemoveState(kUpdating);
        
        m_Changes.insert(make_pair(asset.GetPath(), asset));
        
        i++;
    }
    
    return true;
}

bool FSPlugin::DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
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
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
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
        
        CopyAFile(localPath, path, true);
        
        int state = kLocal | kSynced;
        if (asset.IsMeta()) state |= kMetaFile;
        
        asset.SetState(state);
        asset.RemoveState(kUpdating);
        
        i++;
    }
    
    return true;
}

bool FSPlugin::GetAssets(VersionedAssetList& assetList)
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
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
        if (asset.IsFolder())
        {
            assetList.erase(i);
            continue;
        }
        
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
        
        asset.RemoveState(kUpdating);
        
        i++;
    }
    
    return true;
}

bool FSPlugin::RevertAssets(VersionedAssetList& assetList)
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
        VersionedAssetMap::iterator j = m_Changes.find(asset.GetPath());
        if (j == m_Changes.end())
        {
            assetList.erase(i);
            continue;
        }
        
        asset.RemoveState(kCheckedOutLocal);
        asset.RemoveState(kLockedLocal);
        asset.RemoveState(kAddedLocal);
        
        //VersionedAsset& managedAsset = j->second;
        string path = asset.GetPath();
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
        m_Changes.erase(j);
        
        i++;
    }
    return true;
}

bool FSPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
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

bool FSPlugin::RemoveAssets(VersionedAssetList& assetList)
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

bool FSPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
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

bool FSPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;

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

bool FSPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;

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

bool FSPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
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

bool FSPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList)
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
        VersionedAssetMap::iterator j = m_Changes.find(asset.GetPath());
        if (j == m_Changes.end())
        {
            assetList.erase(i);
            continue;
        }
        
        VersionedAsset& managedAsset = j->second;
        string path = asset.GetPath();
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
        if (managedAsset.IsFolder() && (managedAsset.HasState(kCheckedOutLocal) || managedAsset.HasState(kAddedLocal)))
        {
            asset.SetState(kLocal | kSynced);
            GetConnection().Log().Debug() << "Create directory " << localPath << Endl;
            EnsureDirectory(localPath);
        }
        else if (managedAsset.IsFolder() && managedAsset.HasState(kDeletedLocal))
        {
            asset.SetState(kNone);
            GetConnection().Log().Debug() << "Delete directory " << localPath << Endl;
            DeleteRecursive(localPath);
        }
        else if (!managedAsset.IsFolder() && (managedAsset.HasState(kCheckedOutLocal) || managedAsset.HasState(kAddedLocal)))
        {
            asset.SetState(kLocal | kSynced);
            if (asset.IsMeta())
            {
                asset.AddState(kMetaFile);
            }
            GetConnection().Log().Debug() << "Copy file " << path << " to " << localPath << Endl;
            CopyAFile(path, localPath, true);
        }
        else if (!managedAsset.IsFolder() && managedAsset.HasState(kDeletedLocal))
        {
            m_Changes.erase(j);
            GetConnection().Log().Debug() << "Delete file " << localPath << Endl;
            DeleteRecursive(localPath);
            assetList.erase(i);
        }
        
        m_Changes.erase(j);

        i++;
    }
    return true;
}

bool FSPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
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
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        int state = asset.GetState();
        state |= kLocal;
        if (PathExists(localPath)) {
            if (CompareFiles(path, localPath) < 0)
            {
                state &= kSynced;
                state |= kOutOfSync;
            }
            else
            {
                state |= kSynced;
                state &= ~kOutOfSync;
            }
        }
        else
        {
            state &= ~kSynced;
            state &= ~kOutOfSync;
        }
        
        asset.SetState(state);
        if (asset.HasState(kAddedLocal) || asset.HasState(kDeletedLocal) || asset.HasState(kCheckedOutLocal) || asset.HasState(kLockedLocal))
        {
            m_Changes.insert(make_pair(asset.GetPath(), asset));
        }
        
        i++;
    }
    
    return true;
}

bool FSPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    assetList.clear();
    for (VersionedAssetMap::const_iterator i = m_Changes.begin() ; i != m_Changes.end() ; i++)
    {
        const VersionedAsset& asset = i->second;
        string path = asset.GetPath();
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
        if (!asset.IsFolder())
        {
            assetList.push_back(asset);
        }
    }
    return true;
}

bool FSPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
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

bool FSPlugin::GetAssetsChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsChanges" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        changes.clear();
        return true;
    }
    
    Changelist defaultItem;
    defaultItem.SetDescription("default");
    defaultItem.SetRevision(kDefaultListRevision);
    changes.push_back(defaultItem);

    if (m_Changes.size() != 0)
    {
    }
    
    return true;
}

bool FSPlugin::GetAssetsIncomingChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsIncomingChanges" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        changes.clear();
        return true;
    }
    
    return true;
}

bool FSPlugin::UpdateRevision(const ChangelistRevision& revision, string& description)
{
    GetConnection().Log().Debug() << "UpdateRevision" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        return true;
    }
    
    description.clear();
    
    return true;
}

bool FSPlugin::DeleteRevision(const ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "DeleteRevision" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        return true;
    }
    
    return true;
}

bool FSPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
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
