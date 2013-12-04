#if defined(_WINDOWS)
#include "windows.h"
#endif

#include "AESPlugin.h"
#include "FileSystem.h"
#include "JSON.h"

#include <set>
#include <vector>
#include <algorithm>
#include <time.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

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
    t = localtime(&timeInSeconds);
    strftime(buffer, sizeof(buffer), "%c", t);
    return string(buffer);
}

int updateStateForMeta(const string& path, int state)
{
	if (path.length() > 5 && path.substr(path.length() - 5, 5) == ".meta")
		state |= kMetaFile;
	return state;
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
    if (m_AES != NULL)
		delete m_AES;
}

static int PrintEntryCallBack(void *data, const std::string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	if (entry)
	{
		plugin->GetConnection().Log().Debug() << " - " << key << " (" << entry->GetState() << ")" << " [" << ToTime(entry->GetTimeStamp()) << "]" << Endl;
		return 0;
	}
	
	return 1;
}

int AESPlugin::Test()
{
	GetConnection().Log().Debug() << "Test" << Endl;
	
	Connect();
	Login();
	Disconnect();
	
	return 0;
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
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Repository", "Repository", "The AES Repository", "TestGitRepo3", VersionControlPluginCfgField::kRequiredField));
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
}

bool IsPathAuthorized(const string& path)
{
	if (path.length() > 9 && path.substr(path.length() - 9, 9) == ".DS_Store") return false;
	if (path.length() > 5 && path.substr(0, 5) == "Temp/") return false;
	if (path.length() > 7 && path.substr(0, 7) == "Library/") return false;
	return true;
}

typedef struct
{
	AESPlugin* plugin;
	TreeOfEntries* localChanges;
	TreeOfEntries* snapShot;
} ScanLocalChangeCallBackData;

static int ScanLocalChangeCallBack(void* data, const string& path, uint64_t size, bool isDirectory, time_t ts)
{
	ScanLocalChangeCallBackData* d = (ScanLocalChangeCallBackData*)data;
	d->plugin->GetConnection().Log().Debug() << " - " << path << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
	
	string p = path.substr(d->plugin->GetProjectPath().size()+1);
	if (isDirectory)
	{
		// Directory are kept in tree with a last backslash
		p.append("/");
	}
	
	// search in local changes
	AESEntry* entry = d->localChanges->search(p);
	if (entry == NULL)
	{
		// Not found, search in snapshot
		entry = d->snapShot->search(p);
		if (entry != NULL)
		{
			// Found, compare TS
			if (!isDirectory && (entry->GetTimeStamp() != ts || entry->GetSize() != size))
			{
				// Local file has been modified, add it to local changes
				d->localChanges->insert(p, AESEntry(kLocalRevison, entry->GetHash(), kCheckedOutLocal, (int)size, isDirectory, ts));
			}
		}
		else
		{
			if (IsPathAuthorized(p))
			{
				// Local file/path is new
				d->localChanges->insert(p, AESEntry(kLocalRevison, "", kLocal, (int)size, isDirectory, ts));
			}
		}
	}
	else
	{
		// Update entry
		entry->SetSize(size);
		entry->SetTimeStamp(ts);
	}
	return 0;
}

bool AESPlugin::RefreshSnapShot()
{
	GetConnection().Log().Debug() << "RefreshSnapShot" << Endl;
	
	if (!m_IsConnected)
		return false;
	
    m_SnapShotRevision.clear();
	m_SnapShotEntries.clear();
	m_LocalChangesEntries.clear();

	string aesCache = GetProjectPath() + kCacheFileName;
	if (PathExists(aesCache))
	{
		if (!RestoreSnapShotFromFile(aesCache))
		{
			GetConnection().Log().Debug() << "Cannot refresh snapshot from AES cache file" << Endl;
			return false;
		}
	}
	
	return true;
}

typedef struct
{
	AESPlugin* plugin;
	TreeOfEntries* localChanges;
} ApplySnapShotChangeCallBackData;

static int ApplySnapShotChangeCallBack(void *data, const std::string& key, AESEntry *entry)
{
	ApplySnapShotChangeCallBackData* d = (ApplySnapShotChangeCallBackData*)data;
	string localPath = d->plugin->GetProjectPath() + "/" + key;
	if (!PathExists(localPath))
	{
		d->localChanges->insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), kDeletedLocal, entry->GetSize(), entry->IsDir(), entry->GetTimeStamp()));
	}
	return 0;
}

bool AESPlugin::RefreshLocal()
{
	GetConnection().Log().Debug() << "RefreshLocal" << Endl;
	
	if (!m_IsConnected)
		return false;

	// Scan "Assets" and "ProjectSettings" directories for local changes
	ScanLocalChangeCallBackData data;
	data.plugin = this;
	data.snapShot = &m_SnapShotEntries;
	data.localChanges = &m_LocalChangesEntries;
	
	ScanDirectory(GetProjectPath()+"/Assets/", true, ScanLocalChangeCallBack, (void*)&data);
	ScanDirectory(GetProjectPath()+"/ProjectSettings/", true, ScanLocalChangeCallBack, (void*)&data);
	
	// Scan removed entries
	vector<string> paths = m_LocalChangesEntries.keys("");
	for (vector<string>::const_iterator i = paths.begin() ; i != paths.end() ; i++)
	{
		string path = *i;
		string localPath = GetProjectPath() + "/" + path;
		if (!PathExists(localPath))
		{
			AESEntry* entry = m_SnapShotEntries.search(path);
			if (entry == NULL)
			{
				m_LocalChangesEntries.erase(path);
			}
			else
			{
				m_LocalChangesEntries.insert(path, AESEntry(entry->GetRevisionID(), entry->GetHash(), kDeletedLocal, entry->GetSize(), entry->IsDir(), entry->GetTimeStamp()));
			}
		}
	}
	
	// Scan removed entries
	ApplySnapShotChangeCallBackData data2;
	data2.plugin = this;
	data2.localChanges = &m_LocalChangesEntries;
	
	m_SnapShotEntries.iterate(ApplySnapShotChangeCallBack, (void*)&data2);
	
	return true;
}

bool AESPlugin::RefreshRemote()
{
	GetConnection().Log().Debug() << "RefreshRemote" << Endl;
	
	if (!m_IsConnected)
		return false;
	
	// Get latest revision from remote
	m_LatestRevision.clear();
	if (!m_AES->GetLatestRevision(m_LatestRevision))
	{
		GetConnection().Log().Debug() << "Cannot get latest revision from AES, reason: " << m_AES->GetLastMessage() << Endl;
		return false;
	}
	
	if (m_SnapShotRevision.empty())
	{
		// If not snapshot was taken, treats latest revision as remote changes
		if (!m_AES->GetRevision(m_LatestRevision, m_RemoteChangesEntries))
		{
			GetConnection().Log().Debug() << "Cannot get remote changes from AES, reason: " << m_AES->GetLastMessage() << Endl;
			return false;
		}
	}
	else if (m_SnapShotRevision != m_LatestRevision)
	{
		// Get delta from snapshot revision and latest revision as remote changes
		if (!m_AES->GetRevisionDelta(m_LatestRevision, m_SnapShotRevision, m_RemoteChangesEntries))
		{
			GetConnection().Log().Debug() << "Cannot get remote changes from AES, reason: " << m_AES->GetLastMessage() << Endl;
			return false;
		}
	}
	
	return true;
}

typedef struct
{
	AESPlugin* plugin;
	JSONArray* array;
} EntryToJSONCallBackData;

static int EntryToJSONCallBack(void *data, const std::string& key, AESEntry *entry)
{
	EntryToJSONCallBackData* d = (EntryToJSONCallBackData*)data;
	JSONObject elt;
	elt["path"] = new JSONValue(key);
	if (!entry->GetHash().empty()) elt["hash"] = new JSONValue(entry->GetHash());
	elt["directory"] = new JSONValue(entry->IsDir());
	elt["state"] = new JSONValue((double)entry->GetState());
	if (entry->GetSize() > 0) elt["size"] = new JSONValue((double)entry->GetSize());
	if (entry->GetTimeStamp() > 0)
	{
		elt["mtime"] = new JSONValue((double)entry->GetTimeStamp());
		elt["mtimeStr"] = new JSONValue(ToTime(entry->GetTimeStamp())); // User-friendly output
	}
	
	d->array->push_back(new JSONValue(elt));
	return 0;
}

bool AESPlugin::SaveSnapShotToFile(const string& path)
{
	GetConnection().Log().Debug() << "SaveSnapShotToFile " << path << Endl;
	
	if (m_SnapShotRevision.empty())
	{
		return true;
	}
	
	// Build JSON array from snapshot entries
	JSONArray entries;
	EntryToJSONCallBackData data;
	data.plugin = this;
	data.array = &entries;
	m_SnapShotEntries.iterate(EntryToJSONCallBack, (void*)&data);

	// Build JSON array from local changes
	JSONArray changes;
	data.array = &changes;
	m_LocalChangesEntries.iterate(EntryToJSONCallBack, (void*)&data);

	// Build JSON object
	JSONObject obj;
	obj["entries"] = new JSONValue(entries);
	obj["changes"] = new JSONValue(changes);
	obj["snapshot"] = new JSONValue((string)m_SnapShotRevision);
	
	// Save to file
	JSONValue json(obj);
	return WriteAFile(path, json.Stringify(true));
}

bool AESPlugin::RestoreSnapShotFromFile(const string& path)
{
	GetConnection().Log().Debug() << "RestoreSnapShotFromFile " << path << Endl;
	
	bool res = false;
	string buffer;
	if (!ReadAFile(path, buffer))
	{
		return false;
	}
	
	m_SnapShotRevision.clear();
	m_SnapShotEntries.clear();
	m_LocalChangesEntries.clear();
	
	JSONValue* json = JSON::Parse(buffer.c_str());
	if (json != NULL && json->IsObject())
	{
		const JSONObject& obj = json->AsObject();
		if (obj.find("snapshot") != obj.end())
		{
			// Get Snapshot revision
			m_SnapShotRevision = (ChangelistRevision)(*(obj.at("snapshot")));
		}
		
		if (obj.find("changes") != obj.end())
		{
			// Get local changes
			const JSONArray& arr = *(obj.at("changes"));
			for (vector<JSONValue*>::const_iterator i = arr.begin() ; i != arr.end() ; i++)
			{
				const JSONObject& entry = (*i)->AsObject();

				string path = *(entry.at("path"));
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "";
				bool isDir = *(entry.at("directory"));
				int state = (int)((entry.at("state"))->AsNumber());
				uint64_t size = (entry.find("size") != entry.end()) ? (uint64_t)((entry.at("size"))->AsNumber()) : 0;
				time_t ts = (entry.find("mtime") != entry.end()) ? (time_t)((entry.at("mtime"))->AsNumber()) : 0;
				
				m_LocalChangesEntries.insert(path, AESEntry(kLocalRevison, hash, state, size, isDir, ts));
			}
		}
		
		if (obj.find("entries") != obj.end())
		{
			// Get snapshot entries
			const JSONArray& arr = *(obj.at("entries"));
			for (vector<JSONValue*>::const_iterator i = arr.begin() ; i != arr.end() ; i++)
			{
				const JSONObject& entry = (*i)->AsObject();

				string path = *(entry.at("path"));
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "";
				bool isDir = *(entry.at("directory"));
				int state = (int)((entry.at("state"))->AsNumber());
				int size = (entry.find("size") != entry.end()) ? (int)((entry.at("size"))->AsNumber()) : 0;
				time_t ts = (entry.find("mtime") != entry.end()) ? (time_t)((entry.at("mtime"))->AsNumber()) : 0;

				m_SnapShotEntries.insert(path, AESEntry(m_SnapShotRevision, hash, state, size, isDir, ts));
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
        SetOnline();
        NotifyOffline(m_AES->GetLastError());
        return -1;
    }
    
    m_IsConnected = true;
    return 0;
}

void AESPlugin::Disconnect()
{
    GetConnection().Log().Debug() << "Disconnect" << Endl;
    SetOffline();

	string aesCache = GetProjectPath() + kCacheFileName;
	if (!SaveSnapShotToFile(aesCache))
    {
        GetConnection().Log().Debug() << "Cannot save snapshot to AES cache file" << Endl;
    }
    
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
	
	// Refresh snapshot from file
	if (!RefreshSnapShot())
	{
		GetConnection().Log().Debug() << "Cannot refresh snapshot" << Endl;
	}
	
	// Apply local changes
	if (!RefreshLocal())
	{
		GetConnection().Log().Debug() << "Cannot refresh local" << Endl;
	}
	
	// Get remote changes
	if (!RefreshRemote())
	{
		GetConnection().Log().Debug() << "Cannot refresh remote" << Endl;
	}
	
	if (GetConnection().Log().GetLogLevel() == LOG_DEBUG)
	{
		GetConnection().Log().Debug() << "SnapShot Revision ID [" << m_SnapShotRevision << "]" << Endl;
		GetConnection().Log().Debug() << "Latest Revision ID [" << m_LatestRevision << "]" << Endl;
		
		GetConnection().Log().Debug() << "Remote Changes:" << Endl;
		if (m_RemoteChangesEntries.size() > 0)
			m_RemoteChangesEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
		
		GetConnection().Log().Debug() << "Local Changes:" << Endl;
		if (m_LocalChangesEntries.size() > 0)
			m_LocalChangesEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
		
		GetConnection().Log().Debug() << "Snapshot:" << Endl;
		if (m_SnapShotEntries.size() > 0)
			m_SnapShotEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
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
	GetConnection().Log().Debug() << "AddAssetsToChanges (with state " << state << ")" << Endl;
	
    for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        const VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry != NULL && (entry->GetState() & state) == state)
		{
			// Already in changes
			continue;
		}

		state = updateStateForMeta(path, state);
		
		GetConnection().Log().Debug() << "AddAssetsToChanges " << path << Endl;
		state |= asset.GetState();
		
		// Add or update local change
		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(asset.GetPath(), &size, NULL, &ts);
		m_LocalChangesEntries.insert(path, AESEntry(kLatestRevison, "", state, size, false, ts));
	}
}

void AESPlugin::RemoveAssetsFromChanges(const VersionedAssetList& assetList, int state)
{
	GetConnection().Log().Debug() << "RemoveAssetsFromChanges (with state " << state << ")" << Endl;

    for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        const VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry != NULL)
		{
			GetConnection().Log().Debug() << "RemoveAssetsFromChanges found " << path << " (" << entry->GetState() << ")" << Endl;
			if (state == kNone || (entry->GetState() & ~state) == kNone)
			{
				GetConnection().Log().Debug() << "RemoveAssetsFromChanges remove " << path << Endl;
				m_LocalChangesEntries.erase(path);
			}
			else if ((entry->GetState() & state) == state)
			{
				GetConnection().Log().Debug() << "RemoveAssetsFromChanges change " << path << Endl;
				entry->SetState(entry->GetState() & ~state);
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
        return false;
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
        return false;
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
        return false;
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
        return false;
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
        return false;
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
        return false;
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
        return false;
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
		AESEntry* entry = m_SnapShotEntries.search(path);
		if (entry != NULL)
		{
			::DeleteRecursive(asset.GetPath());
			if (!m_AES->Download(entry, path, GetProjectPath()))
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

typedef struct
{
	AESPlugin* plugin;
	AESClient* client;
	TreeOfEntries* localChanges;
	TreeOfEntries* snapshot;
} ApplyRemoteChangesCallBackData;

static int ApplyRemoteChangesCallBack(void *data, const std::string& key, AESEntry *entry)
{
	ApplyRemoteChangesCallBackData* d = (ApplyRemoteChangesCallBackData*)data;
	AESEntry* localEntry = d->localChanges->search(key);
	if (localEntry != NULL)
	{
		// Potential conflict
		if (localEntry->GetSize() != entry->GetSize() || localEntry->GetTimeStamp() != entry->GetTimeStamp())
		{
			d->plugin->GetConnection().Log().Debug() << "Conflict for " << key << Endl;
			localEntry->SetState(kConflicted);
		}
	}
	else
	{
		// Handle added and deleted files
		if ((entry->GetState() & kAddedRemote) == kAddedRemote)
		{
			// Download file
			if (!d->client->Download(entry, key, d->plugin->GetProjectPath()))
			{
				d->plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << d->client->GetLastMessage() << Endl;
				return 1;
			}
			
			// Update Snapshot
			int state = kSynced;
			state = updateStateForMeta(key, state);
			
			string localPath = d->plugin->GetProjectPath() + "/" + key;
			uint64_t size = 0;
			time_t ts = 0;
			GetAFileInfo(localPath, &size, NULL, &ts);
			
			d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
			d->snapshot->insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), state, size, false, ts));
		}
		else if ((entry->GetState() & kDeletedRemote) == kDeletedRemote)
		{
			// Delete local file
			string localPath = d->plugin->GetProjectPath() + "/" + key;
			d->plugin->GetConnection().Log().Debug() << "Delete file from remote change: " << localPath << Endl;
			if (!DeleteRecursive(localPath))
			{
				d->plugin->GetConnection().Log().Debug() << "Cannot delete " << localPath << Endl;
				return 1;
			}
			
			// Update snapshot
			d->snapshot->erase(key);
		}
	}
	return 0;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return false;
    }
	
	ApplyRemoteChangesCallBackData data;
	data.plugin = this;
	data.client = m_AES;
	data.localChanges = &m_LocalChangesEntries;
	data.snapshot = &m_SnapShotEntries;
	m_RemoteChangesEntries.iterate(ApplyRemoteChangesCallBack, (void*)&data);
	m_SnapShotRevision = m_LatestRevision;
	m_RemoteChangesEntries.clear();
	
	return GetAssetsStatus(assetList);
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
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return false;
    }
	
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
		asset.SetRevision(revision);
		
		if (revision == kDefaultListRevision)
		{
			string path = asset.GetPath().substr(GetProjectPath().size()+1);
			GetConnection().Log().Debug() << "Looking for: " << path << Endl;
			
			AESEntry* entry = m_LocalChangesEntries.search(path);
			if (entry == NULL)
			{
				uint64_t size = 0;
				time_t ts = 0;
				GetAFileInfo(asset.GetPath(), &size, NULL, &ts);
				m_LocalChangesEntries.insert(path, AESEntry(kLatestRevison, "", asset.GetState() | kCheckedOutLocal));
			}
		}
	}
	
    return GetAssetsStatus(assetList);
}

typedef struct
{
	AESPlugin* plugin;
	TreeOfEntries* snapshot;
	string* revision;
} ApplyLocalChangesCallBackData;

static int ApplyLocalChangesCallBack(void *data, const std::string& key, AESEntry *entry)
{
	ApplyLocalChangesCallBackData* d = (ApplyLocalChangesCallBackData*)data;
	if ((entry->GetState() & kSynced) == kSynced)
	{
		d->plugin->GetConnection().Log().Debug() << "Add/Update in snapshot " << key << Endl;
		d->snapshot->insert(key, AESEntry(*(d->revision), "", entry->GetState(), entry->GetSize(), false, entry->GetTimeStamp()));
	}
	else if ((entry->GetState() & kDeletedLocal) == kDeletedLocal)
	{
		d->plugin->GetConnection().Log().Debug() << "Delete in snapshot " << key << Endl;
		d->snapshot->erase(key);
	}
	return 0;
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList, bool saveOnly)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return false;
    }
	
	if (m_RemoteChangesEntries.size() > 0)
	{
		GetConnection().WarnLine("Merges still pending. Resolve before submitting.", MASystem);
		assetList.clear();
        return false;
	}
    
	TreeOfEntries entriesToAddOrMod;
	TreeOfEntries entriesToDelete;
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
		{
			// Skip folders
			continue;
		}
		
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(asset.GetPath(), &size, NULL, &ts);
		
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry == NULL)
		{
			entry = m_LocalChangesEntries.insert(path, AESEntry(kLocalRevison, "", asset.GetState(), size, false, ts));
		}
		
		int state  = entry->GetState();
		if ((state & kAddedLocal) == kAddedLocal || (state & kCheckedOutLocal) == kCheckedOutLocal)
		{
			GetConnection().Log().Debug() << "Add/Update " << path << Endl;
			state = updateStateForMeta(path, kSynced);
			entriesToAddOrMod.insert(path, AESEntry(kLocalRevison, "", state, size, false, ts));
		}
		else if ((state & kDeletedLocal) == kDeletedLocal)
		{
			GetConnection().Log().Debug() << "Remove " << path << Endl;
			state = updateStateForMeta(path, kDeletedLocal);
			entriesToDelete.insert(path, AESEntry(kLocalRevison, "", state));
		}
    }
	
	int succeededEntries;
	if (!m_AES->ApplyChanges(GetProjectPath(), entriesToAddOrMod, entriesToDelete, changeList.GetDescription(), &succeededEntries))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges failed, reason: " << m_AES->GetLastMessage() << " (" << m_AES->GetLastError() << ")" << Endl;
		assetList.clear();
        return false;
	}
	else
	{
		if (succeededEntries != (entriesToAddOrMod.size() + entriesToDelete.size()))
		{
			GetConnection().Log().Debug() << "AES ApplyChanges failed, not all files were submitted" << Endl;
			assetList.clear();
			return false;
		}
		GetConnection().Log().Debug() << "AES ApplyChanges success" << Endl;
	}
	
	if (!m_AES->GetLatestRevision(m_SnapShotRevision))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges cannot get latest revision, reason: " << m_AES->GetLastMessage() << Endl;
		assetList.clear();
        return false;
	}
	
	ApplyLocalChangesCallBackData data;
	data.plugin = this;
	data.snapshot = &m_SnapShotEntries;
	data.revision = &m_SnapShotRevision;
	
	entriesToAddOrMod.iterate(ApplyLocalChangesCallBack, (void*)&data);
	entriesToDelete.iterate(ApplyLocalChangesCallBack, (void*)&data);
	
	RemoveAssetsFromChanges(assetList);

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
        return false;
    }
	
	RefreshRemote();
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
		
		if (asset.IsFolder())
		{
			GetConnection().Log().Debug() << "Scan Local disk changes for " << asset.GetPath() << Endl;
			ScanLocalChangeCallBackData data;
			data.plugin = this;
			data.localChanges = &m_LocalChangesEntries;
			data.snapShot = &m_SnapShotEntries;
			ScanDirectory(GetProjectPath() + "/" + asset.GetPath(), false, ScanLocalChangeCallBack, (void*)&data);
		}

        int state = kNone;
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* localEntry = m_LocalChangesEntries.search(path);
		AESEntry* remoteEntry = m_RemoteChangesEntries.search(path);
		AESEntry* snapShotEntry = m_SnapShotEntries.search(path);
		
		if (localEntry == NULL)
		{
			state = (remoteEntry == NULL) ? ((snapShotEntry == NULL) ? kLocal : kSynced) : kOutOfSync;
		}
		else if (localEntry != NULL)
		{
			state = (remoteEntry == NULL) ? localEntry->GetState() : kConflicted;
		}
		
		state = updateStateForMeta(path, state);

        asset.SetState(state);
    }
    
	if (GetConnection().Log().GetLogLevel() == LOG_DEBUG)
	{
		GetConnection().Log().Debug() << "SnapShot Revision ID [" << m_SnapShotRevision << "]" << Endl;
		GetConnection().Log().Debug() << "Latest Revision ID [" << m_LatestRevision << "]" << Endl;
		
		GetConnection().Log().Debug() << "Remote Changes:" << Endl;
		if (m_RemoteChangesEntries.size() > 0)
			m_RemoteChangesEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
		
		GetConnection().Log().Debug() << "Local Changes:" << Endl;
		if (m_LocalChangesEntries.size() > 0)
			m_LocalChangesEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
		
		GetConnection().Log().Debug() << "Snapshot:" << Endl;
		if (m_SnapShotEntries.size() > 0)
			m_SnapShotEntries.iterate(PrintEntryCallBack, (void*)this);
		else
			GetConnection().Log().Debug() << " - None" << Endl;
	}

    return true;
}

bool AESPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return false;
    }
    
    assetList.clear();
    if (revision == kNewListRevision)
    {
		m_LocalChangesEntries.iterate(PrintEntryCallBack, (void*)this);
		EntriesToAssets(m_LocalChangesEntries, assetList, -1);
    }
    return true;
}

bool AESPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetIncomingAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return false;
    }
    
    assetList.clear();
	
	ChangelistRevisions::const_iterator i = std::find(m_Revisions.begin(), m_Revisions.end(), revision);
	if (i == m_Revisions.end())
	{
		GetConnection().Log().Debug() << "Cannot find revision " << revision << Endl;
		return false;
	}
	
	i++;
	string compareTo = (i == m_Revisions.end()) ? "" : (*i);
    TreeOfEntries entries;
	if (compareTo == "")
	{
		// First revision, no delta
		GetConnection().Log().Debug() << "First revision" << Endl;
		string rev = revision;
		if (m_AES->GetRevision(rev, entries))
		{
			EntriesToAssets(entries, assetList, -1);
		}
	}
	else
	{
		GetConnection().Log().Debug() << "Compare revision " << revision << " with " << compareTo << Endl;
		if (m_AES->GetRevisionDelta(revision, compareTo, entries))
		{
			EntriesToAssets(entries, assetList, -1);
		}
		else
		{
			GetConnection().Log().Debug() << "Compare revision failed, reason: " << m_AES->GetLastMessage() << Endl;
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
        return false;
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
        return false;
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
				comment.append(" [");
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
		return false;
	}

	description.clear();
	description = "Enter comment";
	if (revision != kNewListRevision && revision != kDefaultListRevision)
	{
		description.append(" for revision ");
		description.append(revision);
	}

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

typedef struct
{
	AESPlugin* plugin;
	VersionedAssetList* list;
	int state;
} EntryToAssetCallBackData;

static int EntryToAssetCallBack(void *data, const std::string& key, AESEntry *entry)
{
	EntryToAssetCallBackData* d = (EntryToAssetCallBackData*)data;
	if (!entry->IsDir())
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;
		d->list->push_back(VersionedAsset(localPath, (d->state != -1) ? d->state : entry->GetState()));
	}
	return 0;
}

void AESPlugin::EntriesToAssets(TreeOfEntries& entries, VersionedAssetList& assetList, int state)
{
	EntryToAssetCallBackData data;
	data.plugin = this;
	data.state = state;
	data.list = &assetList;
	
	entries.iterate(EntryToAssetCallBack, (void*)&data);
}