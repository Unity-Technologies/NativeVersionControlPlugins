#if defined(_WINDOWS)
#include "windows.h"
#endif

#include "AESPlugin.h"
#include "FileSystem.h"
#include "JSON.h"

#include <set>
#include <algorithm> 
#include <time.h>

using namespace std;

const char* kCacheFileName = "/Library/aesSnapshot.json";
const char* kLogFileName = "./Library/aesPlugin.log";
const char* kLatestRevison = "current";
const char* kLocalRevison = "local";
const char* kPluginName = "AssetExchangeServer";

string ToTime(time_t timeInSeconds)
{
    struct tm* t;
    char buffer[80];
    t = gmtime(&timeInSeconds);
    strftime(buffer, sizeof(buffer), "%c", t);
    return string(buffer);
}

void FillMapOfEntries(const AESEntries& entries, MapOfEntries& map, int state = -1)
{
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end(); i++)
    {
        AESEntry entry = (*i);
		string path = entry.GetPath();
        map[path] = entry;
		if (state != -1)
		{
			int s = state;
			if (path.length() > 5 && path.substr(path.length() - 5, 5) == ".meta")
				s |= kMetaFile;
			
			(map[path]).SetState(s);
		}
    }
}

enum AESFields { kAESURL, kAESRepository, kAESUserName, kAESPassword };

AESPlugin::AESPlugin(int argc, char** argv) :
    VersionControlPlugin(kPluginName, argc, argv),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::AESPlugin(const char* args) :
    VersionControlPlugin(kPluginName, args),
    m_IsConnected(false),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::~AESPlugin()
{
    Disconnect();
}

const char* AESPlugin::GetLogFileName()
{
	return kLogFileName;
}

const AESPlugin::TraitsFlags AESPlugin::GetSupportedTraitFlags()
{
	return (kRequireNetwork | kEnablesCheckout | kEnablesChangelists | kEnablesLocking | kEnablesGetLatestOnChangeSetSubset | kEnablesRevertUnchanged);
}

AESPlugin::CommandsFlags AESPlugin::GetOnlineUICommands()
{
	return (kAdd | kChanges | kDelete | kDownload | kGetLatest | kIncomingChangeAssets | kIncoming | kStatus | kSubmit | kCheckout | kLock | kUnlock);
}

void AESPlugin::Initialize()
{
    m_Fields.reserve(4);
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "URL", "URL", "The AES URL", "http://localhost:3030", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Repository", "Repository", "The AES Repository", "TestGitRepo2", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Username", "Username", "The AES username", "", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Password", "Password", "The AES password", "", VersionControlPluginCfgField::kPasswordField));
    
    m_Versions.insert(kUnity43);
    
	m_Overlays[kLocal] = "default";
	m_Overlays[kOutOfSync] = "default";
	m_Overlays[kCheckedOutLocal] = "default";
	m_Overlays[kCheckedOutRemote] = "default";
	m_Overlays[kDeletedLocal] = "default";
	m_Overlays[kDeletedRemote] = "default";
	m_Overlays[kAddedLocal] = "default";
	m_Overlays[kAddedRemote] = "default";
	m_Overlays[kLockedLocal] = "default";
	m_Overlays[kLockedRemote] = "default";
	m_Overlays[kConflicted] = "default";
    
    m_SnapShotRevision.clear();
	m_SnapShotEntries.clear();
}

bool AESPlugin::RefreshSnapShot()
{
	GetConnection().Log().Debug() << "RefreshSnapShot" << Endl;
	
	if (!m_IsConnected)
		return false;
	
	AESEntries entries;
	string latest;
	MapOfEntries snapShot;

	if (!m_AES->GetLatestRevision(latest))
	{
		GetConnection().Log().Debug() << "Failed to get latest snapshot revision, reason:" << m_AES->GetLastMessage() << Endl;
		return false;
	}
	
	GetConnection().Log().Debug() << "Latest snapshot revision is " << latest << Endl;
	if (m_AES->GetRevision(latest, entries))
	{
		FillMapOfEntries(entries, snapShot, kSynced);
	}
	else
	{
		GetConnection().Log().Debug() << "Failed to refresh snapshot, reason:" << m_AES->GetLastMessage() << Endl;
		return false;
	}

	m_SnapShotRevision = latest;
	m_SnapShotEntries.swap(snapShot);

	GetConnection().Log().Debug() << "Cleanup changes" << Endl;
	MapOfEntries::iterator i = m_ChangedEntries.begin();
	while (i != m_ChangedEntries.end())
	{
		MapOfEntries::const_iterator j = m_SnapShotEntries.find(i->first);
		if (j != m_SnapShotEntries.end())
		{
			i = m_ChangedEntries.erase(i);
		}
		else
		{
			i++;
		}
	}
	
	return true;
}

bool AESPlugin::SaveSnapShotToFile(const string& path)
{
	GetConnection().Log().Debug() << "SaveSnapShotToFile " << path << Endl;
	
	if (m_SnapShotRevision.empty())
	{
		return false;
	}
	
	JSONArray entries;
    for (MapOfEntries::const_iterator i = m_SnapShotEntries.begin() ; i != m_SnapShotEntries.end(); i++)
    {
		JSONObject elt;
		const AESEntry& entry = i->second;
		elt["path"] = new JSONValue(entry.GetPath());
		if (!entry.GetHash().empty()) elt["hash"] = new JSONValue(entry.GetHash());
		elt["directory"] = new JSONValue(entry.IsDir());
		elt["state"] = new JSONValue((double)entry.GetState());
		if (entry.GetSize() > 0) elt["size"] = new JSONValue((double)entry.GetSize());
		
		entries.push_back(new JSONValue(elt));
	}

	JSONArray changes;
    for (MapOfEntries::const_iterator i = m_ChangedEntries.begin() ; i != m_ChangedEntries.end(); i++)
    {
		JSONObject elt;
		const AESEntry& entry = i->second;
		elt["path"] = new JSONValue(entry.GetPath());
		if (!entry.GetHash().empty()) elt["hash"] = new JSONValue(entry.GetHash());
		elt["directory"] = new JSONValue(entry.IsDir());
		elt["state"] = new JSONValue((double)entry.GetState());
		if (entry.GetSize() > 0) elt["size"] = new JSONValue((double)entry.GetSize());
		
		changes.push_back(new JSONValue(elt));
	}

	JSONObject obj;
	obj["entries"] = new JSONValue(entries);
	obj["changes"] = new JSONValue(changes);
	obj["snapshot"] = new JSONValue((string)m_SnapShotRevision);
	
	JSONValue json(obj);
	return WriteAFile(path, json.Stringify());
}

bool AESPlugin::RestoreSnapShotFromFile(const string& path)
{
	GetConnection().Log().Debug() << "RestoreSnapShotFromFile " << path << Endl;
	
	string buffer;
	if (!ReadAFile(path, buffer))
	{
		return false;
	}
	
	m_SnapShotRevision.clear();
	m_SnapShotEntries.clear();
	m_ChangedEntries.clear();

	bool res = false;
    JSONValue* json = JSON::Parse(buffer.c_str());
	if (json != NULL && json->IsObject())
	{
		const JSONObject& obj = json->AsObject();
		if (obj.find("snapshot") != obj.end())
			m_SnapShotRevision = (ChangelistRevision)(*(obj.at("snapshot")));
		
		if (obj.find("entries") != obj.end())
		{
			const JSONArray& arr = *(obj.at("entries"));
			for (vector<JSONValue*>::const_iterator i = arr.begin() ; i != arr.end() ; i++)
			{
				const JSONObject& entry = (*i)->AsObject();
				string path = *(entry.at("path"));
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "";
				bool isDir = *(entry.at("directory"));
				int state = (int)((entry.at("state"))->AsNumber());
				int size = (entry.find("size") != entry.end()) ? (int)((entry.at("size"))->AsNumber()) : 0;
				
				m_SnapShotEntries[path] = AESEntry(path, m_SnapShotRevision, hash, state, size, isDir);
			}
		}
		
		if (obj.find("changes") != obj.end())
		{
			const JSONArray& arr = *(obj.at("changes"));
			for (vector<JSONValue*>::const_iterator i = arr.begin() ; i != arr.end() ; i++)
			{
				const JSONObject& entry = (*i)->AsObject();
				string path = *(entry.at("path"));
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "";
				bool isDir = *(entry.at("directory"));
				int state = (int)((entry.at("state"))->AsNumber());
				int size = (entry.find("size") != entry.end()) ? (int)((entry.at("size"))->AsNumber()) : 0;
				
				m_ChangedEntries[path] = AESEntry(path, kLocalRevison, hash, state, size, isDir);
			}
		}
		
		res = true;
	}
	
    if (json)
        delete json;
    return res;
}

int AESPlugin::Connect()
{
    GetConnection().Log().Debug() << "Connect" << Endl;
    if (IsConnected())
        return 0;
    
    m_AES = new AESClient(m_Fields[kAESURL].GetValue() + "/api/files/" + m_Fields[kAESRepository].GetValue());
    if (!m_AES->Ping())
    {
        GetConnection().Log().Debug() << "Connect failed, reason: " << m_AES->GetLastError() << Endl;
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
	
	string aesCache = GetProjectPath() + kCacheFileName;
	if (!SaveSnapShotToFile(aesCache))
    {
        GetConnection().Log().Debug() << "Cannot save snapshot to AES cache file" << Endl;
    }
    
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
        GetConnection().Log().Debug() << "Login failed, reason: " << m_AES->GetLastError() << Endl;
        return -1;
    }
	
	string aesCache = GetProjectPath() + kCacheFileName;
	/*
	if (!PathExists(aesCache))
	{
		if (!RefreshSnapShot() || !SaveSnapShotToFile(aesCache))
		{
			GetConnection().Log().Debug() << "Cannot get snapshot from AES, reason: " << m_AES->GetLastMessage() << Endl;
		}
	}
    else
	{
	*/
		if (!RestoreSnapShotFromFile(aesCache))
		{
			GetConnection().Log().Debug() << "Cannot refresh snapshot from AES cache file" << Endl;
		}
	/*
	}
	*/
	
	GetConnection().Log().Debug() << "Current SnapShot " << m_SnapShotRevision << Endl;
	for (MapOfEntries::const_iterator i = m_SnapShotEntries.begin() ; i != m_SnapShotEntries.end() ; i++)
	{
		const AESEntry& entry = i->second;
		GetConnection().Log().Debug() << " - " << entry.GetPath() << " (" << entry.GetState() << ")" << Endl;
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
        
        if (Login() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot log to to VC system"));
            return false;
        }
    }
    
    return true;
}

void AESPlugin::AddAssetsToChanges(const VersionedAssetList& assetList, int state)
{
	GetConnection().Log().Debug() << "AddAssetsToChanges (" << state << ")" << Endl;
	
    for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        const VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		MapOfEntries::const_iterator j = m_ChangedEntries.find(path);
		if (j != m_ChangedEntries.end() && (j->second.GetState() & state) == state)
		{
			// Already in changes
			continue;
		}

		if (path.length() > 5 && path.substr(path.length() - 5, 5) == ".meta")
			state |= kMetaFile;
		
		GetConnection().Log().Debug() << "AddAssetsToChanges " << path << Endl;
		m_ChangedEntries[path] = AESEntry(path, kLatestRevison, "", asset.GetState() | state);
	}

	GetConnection().Log().Debug() << "Changes:" << Endl;
	for (MapOfEntries::const_iterator i = m_ChangedEntries.begin() ; i != m_ChangedEntries.end() ; i++)
	{
		GetConnection().Log().Debug() << " - " << i->first << " (" << i->second.GetState() << ")" << Endl;
	}
}

void AESPlugin::RemoveAssetsFromChanges(const VersionedAssetList& assetList, int state)
{
	GetConnection().Log().Debug() << "RemoveAssetsFromChanges (" << state << ")" << Endl;

	GetConnection().Log().Debug() << "Changes:" << Endl;
	for (MapOfEntries::const_iterator i = m_ChangedEntries.begin() ; i != m_ChangedEntries.end() ; i++)
	{
		GetConnection().Log().Debug() << " - " << i->first << " (" << i->second.GetState() << ")" << Endl;
	}
	
    for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        const VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		MapOfEntries::const_iterator j = m_ChangedEntries.find(path);
		if (j != m_ChangedEntries.end())
		{
			GetConnection().Log().Debug() << "RemoveAssetsFromChanges found " << path << " (" << j->second.GetState() << ")" << Endl;
			if (state == kNone || (j->second.GetState() & state) == state)
			{
				GetConnection().Log().Debug() << "RemoveAssetsFromChanges remove " << path << Endl;
				m_ChangedEntries.erase(j);
			}
		}
	}
}

bool AESPlugin::AddAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "AddAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    AddAssetsToChanges(assetList, kAddedLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RemoveAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    AddAssetsToChanges(assetList, kDeletedLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    GetConnection().Log().Debug() << "MoveAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        toAssetList.clear();
        return true;
    }
    
    AddAssetsToChanges(fromAssetList, kDeletedLocal);
    AddAssetsToChanges(toAssetList, kAddedLocal);
    return GetAssetsStatus(toAssetList, false);
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    AddAssetsToChanges(assetList, kCheckedOutLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    AddAssetsToChanges(assetList, kLockedLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    RemoveAssetsFromChanges(assetList, kLockedLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return false;
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    RemoveAssetsFromChanges(assetList, kNone);
	
	for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		MapOfEntries::const_iterator j = m_SnapShotEntries.find(path);
		if (j != m_SnapShotEntries.end())
		{
			::DeleteRecursive(asset.GetPath());
			if (!m_AES->Download(j->second, GetProjectPath()))
			{
				GetConnection().Log().Debug() << "RevertAssets, cannot download " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
			}
			else
			{
				asset.SetState(kNone);
			}
		}
	}
	
	return GetAssetsStatus(assetList, false);
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
	
	if (RefreshSnapShot())
	{
		GetConnection().Log().Debug() << "Latest snapshot revision is " << m_SnapShotRevision << Endl;
		for (MapOfEntries::const_iterator i = m_SnapShotEntries.begin() ; i != m_SnapShotEntries.end() ; i++)
		{
			const AESEntry& entry = i->second;
			if (!m_AES->Download(entry, GetProjectPath()))
			{
				GetConnection().Log().Debug() << "Failed to download '" << entry.GetPath() << "', reason: " << m_AES->GetLastMessage() << Endl;
			}
		}
		
		return GetAssetsStatus(assetList);
	}

	assetList.clear();
	GetConnection().Log().Debug() << "Failed to refresh snapshot, reason: " << m_AES->GetLastMessage() << Endl;
	
	return true;
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return false;
}

bool AESPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SetRevision" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
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
        if (!asset.IsFolder())
		{
			string path = asset.GetPath().substr(GetProjectPath().size()+1);
			if (asset.HasState(kAddedLocal))
			{
				GetConnection().Log().Debug() << "Add " << path << Endl;
				entriesToAddOrMod.push_back(AESEntry(path, kLatestRevison, "", asset.GetState()));
			}
			else if (asset.HasState(kDeletedLocal))
			{
				GetConnection().Log().Debug() << "Remove " << path << Endl;
				entriesToDelete.push_back(AESEntry(path, kLatestRevison, "", asset.GetState()));
			}
		}
    }
	
	if (!m_AES->ApplyChanges(GetProjectPath(), entriesToAddOrMod, entriesToDelete, changeList.GetDescription()))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges failed, reason: " << m_AES->GetLastMessage() << " (" << m_AES->GetLastError() << ")" << Endl;
		assetList.clear();
        return true;
	}
	
	RemoveAssetsFromChanges(assetList);
	if (!RefreshSnapShot())
    {
        assetList.clear();
        return true;
    }

    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode)
{
    GetConnection().Log().Debug() << "SetAssetsFileMode" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
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
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        int state = kNone;
		if (::PathExists(asset.GetPath()))
		{
			state |= kLocal;
		}
		
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		GetConnection().Log().Debug() << "Looking for: " << path << Endl;
        
		MapOfEntries::const_iterator j = m_ChangedEntries.find(path);
        if (j != m_ChangedEntries.end())
		{
			const AESEntry& entry = j->second;
			GetConnection().Log().Debug() << "Found AES entry " << entry.GetPath() << " (" << entry.GetState() << ")" << Endl;
			state = entry.GetState();
		}
		else
        {
			j = m_SnapShotEntries.find(path);
			if (j != m_SnapShotEntries.end())
			{
				const AESEntry& entry = j->second;
				GetConnection().Log().Debug() << "Found AES entry " << entry.GetPath() << " (" << entry.GetState() << ")" << Endl;
				state = entry.GetState();
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
		EntriesToAssets(m_ChangedEntries, assetList);
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
	
	ChangelistRevisions::const_iterator i = std::find(m_Revisions.begin(), m_Revisions.end(), revision);
	if (i == m_Revisions.end())
	{
		GetConnection().Log().Debug() << "Cannot find revision " << revision << Endl;
		return true;
	}
	
	i++;
	string compareTo = (i == m_Revisions.end()) ? "" : (*i);
    AESEntries entries;
	if (compareTo == "")
	{
		// First revision, no delta
		GetConnection().Log().Debug() << "First revision" << Endl;
		string rev = revision;
		if (m_AES->GetRevision(rev, entries))
		{
			EntriesToAssets(entries, assetList);
		}
	}
	else
	{
		GetConnection().Log().Debug() << "Compare revision " << revision << " with " << compareTo << Endl;
		if (m_AES->GetRevisionDelta(revision, compareTo, entries))
		{
			EntriesToAssets(entries, assetList);
		}
	}

    for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        const VersionedAsset& asset = (*i);
		GetConnection().Log().Debug() << " - " << asset.GetPath() << Endl;
	}
	
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
    Changelist defaultItem;
    defaultItem.SetDescription("Default");
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
	m_Revisions.clear();
    vector<AESRevision> revisions;
    if (m_AES->GetRevisions(revisions))
    {
		bool snapShotFound = false;
        for (vector<AESRevision>::const_iterator i = revisions.begin() ; i != revisions.end() ; i++)
        {
            const AESRevision& rev = (*i);
			if (rev.GetRevisionID() == m_SnapShotRevision)
			{
				snapShotFound = true;
			}

			GetConnection().Log().Debug() << "GetAssetsIncomingChanges found revision " << rev.GetRevisionID() << (snapShotFound ? ", skipt it" : "") << Endl;
			if (!snapShotFound)
			{
				Changelist item;
				
				string comment = rev.GetComment();
				comment.append(" [Rev ");
				comment.append(rev.GetRevisionID());
				comment.append(" by ");
				comment.append(rev.GetComitterEmail());
				comment.append(" on ");
				comment.append(ToTime(rev.GetTimeStamp()));
				comment.append("]");
				
				item.SetCommitter(rev.GetComitterName());
				item.SetDescription(comment);
				item.SetRevision(rev.GetRevisionID());
				item.SetTimestamp(ToTime(rev.GetTimeStamp()));
				changes.push_back(item);
			}
			
			m_Revisions.push_back(rev.GetRevisionID());
        }
		std::reverse(changes.begin(), changes.end());
    }
	else
	{
		GetConnection().Log().Debug() << "GetAssetsIncomingChanges failed, reason: " << m_AES->GetLastMessage() << Endl;
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
	description = "Enter comment";

	return true;
}

bool AESPlugin::DeleteRevision(const ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "DeleteRevision" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return false;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return false;
}

void AESPlugin::EntriesToAssets(const MapOfEntries& entries, VersionedAssetList& assetList, int state)
{
    for (MapOfEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
    {
        const AESEntry& entry = i->second;
        if (!entry.IsDir())
        {
			string localPath = GetProjectPath() + "/" + entry.GetPath();
            assetList.push_back(VersionedAsset(localPath, (state != -1) ? state : entry.GetState()));
        }
    }
}

void AESPlugin::EntriesToAssets(const AESEntries& entries, VersionedAssetList& assetList, int state)
{
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
    {
        const AESEntry& entry = (*i);
        if (!entry.IsDir())
        {
			string localPath = GetProjectPath() + "/" + entry.GetPath();
            assetList.push_back(VersionedAsset(localPath, (state != -1) ? state : entry.GetState()));
        }
    }
}