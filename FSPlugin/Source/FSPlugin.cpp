#include "FSPlugin.h"
#include "FileSystem.h"

#include <set>

#if defined(_WINDOWS)
#include "windows.h"
#endif

using namespace std;

enum FSFields { kFSPath };

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

bool FSPlugin::DummyPerform(const std::string cmd, VersionedAssetList& assetList, int state)
{
    GetConnection().Log().Debug() << cmd << Endl;
    
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            assetList.clear();
            return true;
        }
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
            GetConnection().Log().Debug() << "Asset " << asset.GetPath() << " is a folder" << Endl;
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

bool FSPlugin::AddAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "AddAssets" << Endl;

    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            assetList.clear();
            return true;
        }
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
            asset.SetState(kLocal | kSynced | kAddedLocal);
            asset.RemoveState(kUpdating);

            m_Changes.insert(make_pair(asset.GetPath(), FSChange(FSChange::kAddDirectory, asset)));
            
            assetList.erase(i);
            continue;
        }

        int state = kLocal | kSynced | kAddedLocal;
        if (asset.IsMeta()) state |= kMetaFile;
 
        asset.SetState(state);
        asset.RemoveState(kUpdating);

        m_Changes.insert(make_pair(asset.GetPath(), FSChange(FSChange::kAddOrUpdate, asset)));
        
        i++;
    }
    
    return true;
}

bool FSPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    return DummyPerform("CheckoutAssets", assetList, kLocal|kSynced|kCheckedOutLocal);
}

bool FSPlugin::GetAssets(VersionedAssetList& assetList)
{
    return DummyPerform("GetAssets", assetList, kLocal|kSynced);
}

bool FSPlugin::RevertAssets(VersionedAssetList& assetList)
{
    return DummyPerform("RevertAssets", assetList, kLocal|kSynced);
}

bool FSPlugin::ResolveAssets(VersionedAssetList& assetList)
{
    return DummyPerform("ResolveAssets", assetList, -1);
}

bool FSPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    return DummyPerform("RemoveAssets", assetList, kLocal|kSynced);
}

bool FSPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    return DummyPerform("MoveAssets", toAssetList, kLocal|kSynced);
}

bool FSPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;
    return false;
}

bool FSPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;
    return false;
}

bool FSPlugin::ChangeOrMoveAssets(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    return DummyPerform("ChangeOrMoveAssets", assetList, -1);
}

bool FSPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            assetList.clear();
            return true;
        }
    }
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        FSChanges::iterator j = m_Changes.find(asset.GetPath());
        if (j == m_Changes.end())
        {
            assetList.erase(i);
            continue;
        }
        
        FSChange& change = j->second;
        string path = asset.GetPath();
        string localPath = m_Fields[kFSPath].GetValue() + path.substr(GetProjectPath().size());
        
        GetConnection().Log().Debug() << "Asset Path: " << path << ", LocalPath: " << localPath << Endl;
        
        if (change.getAction() == FSChange::kAddDirectory)
        {
            asset.SetState(kLocal | kSynced);
            GetConnection().Log().Debug() << "Create directory " << localPath << Endl;
            EnsureDirectory(localPath);
        }
        else if (change.getAction() == FSChange::kDeleteDirectory)
        {
            asset.SetState(kNone);
            GetConnection().Log().Debug() << "Delete directory " << localPath << Endl;
            DeleteRecursive(localPath);
        }
        else if (change.getAction() == FSChange::kAddOrUpdate)
        {
            asset.SetState(kLocal | kSynced);
            if (asset.IsMeta())
            {
                asset.AddState(kMetaFile);
            }
            GetConnection().Log().Debug() << "Copy file " << path << " to " << localPath << Endl;
            CopyAFile(path, localPath, true);
        }
        else if (change.getAction() == FSChange::kAddOrUpdate)
        {
            asset.SetState(kNone);
            GetConnection().Log().Debug() << "Delete file " << localPath << Endl;
            DeleteRecursive(localPath);
        }
        
        m_Changes.erase(j);
        
        i++;
    }
    return true;
}

bool FSPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    return DummyPerform("GetAssetsStatus", assetList, -1);
}

bool FSPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus" << Endl;
    
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            assetList.clear();
            return true;
        }
    }
    
    assetList.clear();
    for (FSChanges::const_iterator i = m_Changes.begin() ; i != m_Changes.end() ; i++)
    {
        const FSChange& change = i->second;
        const VersionedAsset& asset = change.getAsset();
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
    return DummyPerform("GetIncomingAssetsChangeStatus", assetList, -1);
}

bool FSPlugin::GetAssetsChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsChanges" << Endl;
    
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            changes.clear();
            return true;
        }
    }

    if (m_Changes.size() == 0)
    {
        changes.clear();
    }
    
    return true;
}

bool FSPlugin::GetAssetsIncomingChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsIncomingChanges" << Endl;
    
    return true;
}

bool FSPlugin::UpdateRevision(const ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "UpdateRevision" << Endl;
    
    return true;
}

bool FSPlugin::DeleteRevision(const ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "DeleteRevision" << Endl;
    
    return true;
}

bool FSPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
    
    return true;
}
