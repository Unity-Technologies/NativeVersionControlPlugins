#include "AESPlugin.h"

#include "FileSystem.h"

#include <set>
#include <algorithm> 
#include <time.h>

#if defined(_WINDOWS)
#include "windows.h"
#endif

using namespace std;

string ToTime(time_t timeInSeconds)
{
    struct tm* t;
    char buffer[80];
    t = gmtime(&timeInSeconds);
    strftime(buffer, sizeof(buffer), "%c", t);
    return string(buffer);
}

int CompareHash(const string& file, const AESEntry* entry)
{
    size_t size = GetFileLength(file);
    if ((int)size != entry->GetSize())
        return -1;
    
    return 0;
}

void FillMapOfEntries(const AESEntries& entries, MapOfEntries& map)
{
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end(); i++)
    {
        const AESEntry* pEntry = &(*i);
        map[pEntry->GetPath()] = pEntry;
        if (pEntry->IsDir())
        {
            FillMapOfEntries(pEntry->GetChildren(), map);
        }
    }
}

enum AESFields { kAESURL, kAESRepository, kAESUserName, kAESPassword };

AESPlugin::AESPlugin(int argc, char** argv) :
    VersionControlPlugin("AssetExchangeServer", argc, argv),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::AESPlugin(const char* args) :
    VersionControlPlugin("AssetExchangeServer", args),
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
    m_Fields.reserve(4);
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "URL", "URL", "The AES URL", "http://localhost:3030", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Repository", "Repository", "The AES Repository", "TestGitRepo", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Username", "Username", "The AES username", "", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Password", "Password", "The AES password", "", VersionControlPluginCfgField::kPasswordField));
    
    m_Versions.insert(kUnity43);
    
    m_Overlays[kLocal] = "default";
    m_Overlays[kOutOfSync] = "default";
    m_Overlays[kDeletedLocal] = "default";
    m_Overlays[kDeletedRemote] = "default";
    m_Overlays[kAddedLocal] = "default";
    m_Overlays[kAddedRemote] = "default";
    m_Overlays[kConflicted] = "default";
    
    m_CurrRevision = "current";
}

int AESPlugin::Connect()
{
    GetConnection().Log().Debug() << "Connect" << Endl;
    if (IsConnected())
        return 0;
    
    m_AES = new AESClient(m_Fields[kAESURL].GetValue() + "/api/files/" + m_Fields[kAESRepository].GetValue());
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
    return m_Fields[kAESURL].GetValue() + "/api/files/" + m_Fields[kAESRepository].GetValue() + '/' + m_CurrRevision + path.substr(GetProjectPath().size());
}

bool AESPlugin::AddAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "AddAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder() || asset.HasState(kAddedLocal) || asset.HasState(kSynced))
        {
            continue;
        }

        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		GetConnection().Log().Debug() << "Add Outgoing" << path << Endl;
        m_Outgoing.insert(make_pair(path, asset));
    }
    
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    return false;
}

bool AESPlugin::DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
    return false;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    return false;
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
    return false;
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;
    return false;
}

bool AESPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RemoveAssets" << Endl;
    return false;
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    GetConnection().Log().Debug() << "MoveAssets" << Endl;
    return false;
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
    return false;
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
	AESEntries entriesToAddOrMod;
	AESEntries entriesToDelete;
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		string remotePath = BuildRemotePath(asset);
        if (!asset.IsFolder())
		{
			if (asset.HasState(kAddedLocal))
			{
				GetConnection().Log().Debug() << "Add " << asset.GetPath() << " (" << path << ", " << remotePath << ")" << Endl;
				entriesToAddOrMod.push_back(AESEntry(path, asset.GetPath(), remotePath, "", false, -1));
			}
			else if (asset.HasState(kDeletedLocal))
			{
				GetConnection().Log().Debug() << "Remove " << asset.GetPath() << " (" << path << ", " << remotePath << ")" << Endl;
				entriesToDelete.push_back(AESEntry(path, asset.GetPath(), remotePath, "", false, -1));
			}
		}
    }
	
	if (!m_AES->ApplyChanges(entriesToAddOrMod, entriesToDelete, changeList.GetDescription()))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges failed" << Endl;
		assetList.clear();
        return true;
	}
	
    m_Outgoing.clear();
	
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode)
{
    GetConnection().Log().Debug() << "SetAssetsFileMode" << Endl;
    return false;
}

bool AESPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    GetConnection().Log().Debug() << "GetAssetsStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    AESEntries entries;
    if (!m_AES->GetRevision(m_CurrRevision, entries))
    {
        assetList.clear();
        return true;
    }
	
	MapOfEntries map;
	FillMapOfEntries(entries, map);
	for (MapOfEntries::const_iterator j = map.begin() ; j != map.end() ; j++)
	{
		GetConnection().Log().Debug() << "Found in current: " << j->first << Endl;
	}
	
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        int state = kLocal;
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		GetConnection().Log().Debug() << "Looking for: " << path << Endl;
        
		MapOfEntries::const_iterator j = map.find(path);
        if (j != map.end())
        {
			const AESEntry* entry = j->second;
            GetConnection().Log().Debug() << "Found AES entry " << entry->GetReference() << " (" << entry->GetSize() << ")" << Endl;
            if (!entry->IsDir() && CompareHash(path, entry) < 0)
            {
                state |= kOutOfSync;
            }
            else
            {
                state |= kSynced;
            }
        }
		else
		{
			VersionedAssetMap::const_iterator k = m_Outgoing.find(path);
			if (k != m_Outgoing.end())
			{
				state |= kAddedLocal;
			}
		}
        
        asset.SetState(state);
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
    if (revision == kNewListRevision)
    {
        for (VersionedAssetMap::const_iterator i = m_Outgoing.begin() ; i != m_Outgoing.end() ; i++)
        {
            const VersionedAsset& asset = i->second;
            string path = asset.GetPath();
            string remotePath = BuildRemotePath(asset);
            GetConnection().Log().Debug() << "Asset: LocalPath = " << path << ", RemotePath: " << remotePath << Endl;
            assetList.push_back(asset);
        }
    }
    return true;
}

void EntriesToAssets(const AESEntries& entries, VersionedAssetList& assetList)
{
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
    {
        const AESEntry& entry = (*i);
        if (entry.IsDir())
        {
            EntriesToAssets(entry.GetChildren(), assetList);
        }
        else
        {
            assetList.push_back(VersionedAsset(entry.GetPath(), kNone));
        }
    }
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
    AESEntries entries;
    if (m_AES->GetRevision(revision, entries))
    {
        EntriesToAssets(entries, assetList);
    }
    return true;
}

bool CompareRevisions(const AESRevision& lhs, const AESRevision& rhs)
{
    return lhs.GetTimeStamp() < rhs.GetTimeStamp();
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
    Changelist defaultItem;
    defaultItem.SetDescription(m_CurrRevision);
    defaultItem.SetRevision(kNewListRevision);
    changes.push_back(defaultItem);

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
    
    changes.clear();
    vector<AESRevision> revisions;
    if (m_AES->GetRevisions(revisions))
    {
        sort(revisions.begin(), revisions.end(), CompareRevisions);
        for (vector<AESRevision>::const_iterator i = revisions.begin() ; i != revisions.end() ; i++)
        {
            const AESRevision& rev = (*i);
            Changelist item;
            item.SetCommitter(rev.GetComitterName());
            item.SetDescription(rev.GetComment());
            item.SetRevision(rev.GetRevisionID());
            item.SetTimestamp(ToTime(rev.GetTimeStamp()));
            changes.push_back(item);
        }
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
    return false;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
    return false;
}