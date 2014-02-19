#if defined(_WINDOWS)
#include "windows.h"
#endif

#include "AESPlugin.h"
#include "FileSystem.h"
#include "JSON.h"

#include <set>
#include <vector>
#include <algorithm>

#if defined(_WIN32)
#define START_CLOCK()
#define ELAPSED_CLOCK(x)
#else
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#ifdef NDEBUG
#define START_CLOCK()
#define ELAPSED_CLOCK(x)
#else
#define START_CLOCK() clock_t timer = clock();
#define ELAPSED_CLOCK(x) timer = clock() - timer; GetConnection().Log().Debug() << x << " tooks " << (int)((double)timer / (double)(CLOCKS_PER_SEC) * 1000.0) << " ms" << Endl;
#endif
#endif

using namespace std;

static const char* kAESPluginVersion  = "1.0.0";

static const char* kCacheFileName  = "/Library/aesSnapshot.json";
static const char* kLogFileName = "Library/aesPlugin.log";
static const char* kLatestRevison = "current";
static const char* kLocalRevison = "local";
static const char* kPluginName = "AssetExchangeServer";

static const char* kMetaExtension = ".meta";

static const char* kFetchAllCommand = "fetchAll";

string ToTime(time_t timeInSeconds)
{
    struct tm* t;
    char buffer[80];
    t = localtime(&timeInSeconds);
    strftime(buffer, sizeof(buffer), "%c", t);
    return string(buffer);
}

string ToTimeLong(time_t timeInSeconds)
{
    char buffer[80];
    sprintf(buffer, "%ld", (long)timeInSeconds);
    return string(buffer);
}

int UpdateStateForMeta(const string& path, int state)
{
	if (path.length() > 5 && path.substr(path.length() - 5, 5) == kMetaExtension)
		state |= kMetaFile;
	return state;
}

enum AESFields { kAESURL, kAESPort, kAESRepository, kAESUserName, kAESPassword, kAESCreateToggle, kAESAvailableRepositories };

AESPlugin::AESPlugin(int argc, char** argv) :
    VersionControlPlugin(kPluginName, argc, argv),
    m_IsConnected(false),
	m_LastRemoteChangesRefrehTime(0L),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::AESPlugin(const char* args) :
    VersionControlPlugin(kPluginName, args),
    m_IsConnected(false),
	m_LastRemoteChangesRefrehTime(0L),
    m_Timer(0),
    m_LastTimer(0),
    m_AES(NULL)
{
    Initialize();
}

AESPlugin::~AESPlugin()
{
	Disconnect();
	
    if (m_AES != NULL)
		delete m_AES;
}

void AESPlugin::ResetTimer()
{
	m_Timer = time(NULL);
    m_LastTimer = 0;
}

time_t AESPlugin::GetTimerSoFar()
{
	TimerTick();
	return m_LastTimer - m_Timer;
}

bool AESPlugin::TimerTick()
{
    bool res = false;
    time_t now = time(NULL);
    if (now != m_LastTimer)
    {
        res = true;
    }
    
    m_LastTimer = now;
    return res;
}


int AESPlugin::PrintAllCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	string localPath = plugin->GetProjectPath() + "/" + key;
	plugin->GetConnection().Log().Trace() << " - Found '" << key << "'" << Endl;
	return 0;
}


int AESPlugin::Test()
{
	GetConnection().Log().Debug() << "Test" << Endl;

	m_Fields[kAESURL].SetValue("http://localhost");
	m_Fields[kAESPort].SetValue("3030");
	m_Fields[kAESRepository].SetValue("");
	m_Fields[kAESUserName].SetValue("");
	m_Fields[kAESPassword].SetValue("");
	m_Fields[kAESCreateToggle].SetValue("false");

	if (Connect() != 0)
	{
		return -1;
	}

	Disconnect();
	
	return 0;
}

const char* AESPlugin::GetLogFileName()
{
	return kLogFileName;
}

const AESPlugin::TraitsFlags AESPlugin::GetSupportedTraitFlags()
{
	return (kRequireNetwork | kEnablesGetLatestOnChangeSetSubset);
}

AESPlugin::CommandsFlags AESPlugin::GetOnlineUICommands()
{
	return (kAdd | kChanges | kDelete | kDownload | kGetLatest | kIncoming | kIncomingChangeAssets | kLock | kResolve | kRevert | kStatus | kSubmit | kUnlock );
}

void AESPlugin::Initialize()
{
    m_Fields.reserve(7);
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "URL", "URL", "The AES URL", "http://localhost", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Port", "Port", "The AES Port", "3030", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Repository", "Repository", "The AES Repository", "", VersionControlPluginCfgField::kRequiredField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Username", "Username", "The AES username", "none", VersionControlPluginCfgField::kNoneField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Password", "Password", "The AES password", "none", VersionControlPluginCfgField::kPasswordField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Create", "Create Repository", "Create the AES repository if not exists", false, VersionControlPluginCfgField::kToggleField));
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "AvailableRepositories", "Available AES Repositories", "The available AES repositories", "", VersionControlPluginCfgField::kHiddenField));

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

	m_CustomCommands.reserve(1);
	m_CustomCommands.push_back(VersionControlPluginCustomCommand(kFetchAllCommand, "Fetch latest revision"));
}

bool ShouldIgnoreFile(const string& path)
{
	if (path.length() >= 9 && path.substr(path.length() - 9, 9) == ".DS_Store") return true;
	if (path.length() >= 5 && path.substr(0, 5) == "Temp/") return true;
	if (path.length() >= 8 && path.substr(0, 8) == "Library/") return true;
	return false;
}

int AESPlugin::ScanLocalChangeCallBack(void* data, const string& path, uint64_t size, bool isDirectory, time_t ts)
{
	AESPlugin* plugin = (AESPlugin*)data;
    
    if (plugin->TimerTick())
    {
        plugin->GetConnection().Progress(-1, plugin->GetTimerSoFar(), "secs. while scanning local changes...");
    }
    
	string p = path.substr(plugin->GetProjectPath().size()+1);
	if (isDirectory)
	{
		// Directory are kept in tree with a last backslash
		p.append("/");
	}
	
	// search in local changes
	AESEntry* entry = plugin->m_LocalChangesEntries.search(p);
	if (entry == NULL)
	{
		// Not found, search in snapshot
		entry = plugin->m_SnapShotEntries.search(p);
		if (entry != NULL)
		{
			// Found, compare TS
			if (!isDirectory && (entry->GetTimeStamp() != ts || entry->GetSize() != size))
			{
				// Local file has been modified, add it to local changes
				plugin->m_LocalChangesEntries.insert(p, AESEntry(kLocalRevison, entry->GetHash(), UpdateStateForMeta(p, kCheckedOutLocal), (int)size, isDirectory, ts));
			}
		}
		else
		{
			if (!ShouldIgnoreFile(p) && !isDirectory)
			{
				// Local file/path is new
				plugin->m_LocalChangesEntries.insert(p, AESEntry(kLocalRevison, "", UpdateStateForMeta(p, kAddedLocal), (int)size, isDirectory, ts));
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
	START_CLOCK()
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
			StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh snapshot from AES cache file"));
			return false;
		}
	}
	
	ELAPSED_CLOCK("RefreshSnapShot")
	
	return true;
}

int AESPlugin::ApplySnapShotChangeCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	string localPath = plugin->GetProjectPath() + "/" + key;
	if (!PathExists(localPath))
	{
		plugin->m_LocalChangesEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), kDeletedLocal, entry->GetSize(), entry->IsDir(), entry->GetTimeStamp()));
	}
	return 0;
}

bool AESPlugin::RefreshLocal()
{
	START_CLOCK()
	GetConnection().Log().Debug() << "RefreshLocal" << Endl;
	
	if (!m_IsConnected)
		return false;

	// Scan "Assets" and "ProjectSettings" directories for local changes
    ResetTimer();
    GetConnection().Progress(-1, 0, "secs. while scanning local changes...");
	ScanDirectory(GetProjectPath()+"/Assets/", true, ScanLocalChangeCallBack, (void*)this);
	ScanDirectory(GetProjectPath()+"/ProjectSettings/", true, ScanLocalChangeCallBack, (void*)this);
	
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
	m_SnapShotEntries.iterate(ApplySnapShotChangeCallBack, (void*)this);
	
	ELAPSED_CLOCK("RefreshLocal")

	return true;
}

bool AESPlugin::RefreshRemote()
{
	START_CLOCK()
	GetConnection().Log().Debug() << "RefreshRemote" << Endl;
	
	if (!m_IsConnected)
		return false;
	
	time_t now = time(NULL);
	if (!m_LatestRevision.empty() && m_LastRemoteChangesRefrehTime > 0 && difftime(now, m_LastRemoteChangesRefrehTime) < 5.0)
	{
		GetConnection().Log().Debug() << "RefreshRemote skipped" << Endl;
		return true;
	}
	else
	{
		m_LastRemoteChangesRefrehTime = now;
	}
	
	// Get latest revision from remote
	string latestRevision;
	if (!m_AES->GetLatestRevision(latestRevision))
	{
		GetConnection().Log().Debug() << "Cannot get latest revision from AES, reason: " << m_AES->GetLastMessage() << Endl;
		return false;
	}
	
	m_LatestRevision = latestRevision;
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
	
	ELAPSED_CLOCK("RefreshRemote")
	
	return true;
}

typedef struct
{
	AESPlugin* plugin;
	JSONArray* array;
} EntryToJSONCallBackData;

int AESPlugin::EntryToJSONCallBack(void *data, const string& key, AESEntry *entry)
{
	JSONArray* array = (JSONArray*)data;
	JSONObject elt;
	elt["path"] = new JSONValue(key);
	if (!entry->GetHash().empty()) elt["hash"] = new JSONValue(entry->GetHash());
	elt["directory"] = new JSONValue(entry->IsDir());
	elt["state"] = new JSONValue((double)entry->GetState());
	if (entry->GetSize() > 0) elt["size"] = new JSONValue((double)entry->GetSize());
	if (entry->GetTimeStamp() > 0)
	{
		elt["mtime"] = new JSONValue((double)entry->GetTimeStamp());
	}
	
	array->push_back(new JSONValue(elt));
	return 0;
}

bool AESPlugin::SaveSnapShotToFile(const string& path)
{
	GetConnection().Log().Debug() << "SaveSnapShotToFile " << path << Endl;
	
	// Build JSON array from snapshot entries
	JSONArray entries;
	m_SnapShotEntries.iterate(EntryToJSONCallBack, (void*)&entries);

	// Build JSON array from local changes
	JSONArray changes;
	m_LocalChangesEntries.iterate(EntryToJSONCallBack, (void*)&changes);

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

	GetConnection().DataLine(string("Version:") + kAESPluginVersion, MARemote);

	string url = m_Fields[kAESURL].GetValue();
	if (url.empty())
	{
        GetConnection().Log().Debug() << "No value set for URL" << Endl;
        SetOnline();
        NotifyOffline("Missing value for URL");
        return -1;
	}

	string port = m_Fields[kAESPort].GetValue();
	if (port.empty())
	{
        GetConnection().Log().Debug() << "No value set for port" << Endl;
        SetOnline();
        NotifyOffline("Missing value for port");
        return -1;
	}

	string repo = m_Fields[kAESRepository].GetValue();
	if (repo.empty())
	{
        GetConnection().Log().Debug() << "No repository specified, returns all available repositories" << Endl;

		if (m_AES != NULL)
			delete m_AES;

		vector<string> repositories;
		m_AES = new AESClient(url + ":" + port + "/api");

		if (!m_AES->GetAvailableRepositories(repositories))
		{
			GetConnection().Log().Debug() << "Connect failed, reason: " << m_AES->GetLastMessage() << Endl;
			SetOnline();
			NotifyOffline(m_AES->GetLastMessage());

			delete m_AES;
			return -1;
		}

		for (vector<string>::const_iterator i = repositories.begin() ; i != repositories.end() ; i++)
		{
			GetConnection().DataLine(string("Repository:") + (*i), MARemote);
		}

        SetOnline();
        NotifyOffline("No repository specified");

		delete m_AES;
		m_AES = NULL;

		return -1;
	}

	if (m_AES != NULL)
		delete m_AES;

    m_AES = new AESClient(url + ":" + port + "/api/files/" + repo);
    if (!m_AES->Ping())
    {
		bool createIfNotPresent = (m_Fields[kAESCreateToggle].GetValue() == "true");
		if (!createIfNotPresent)
		{
			GetConnection().Log().Debug() << "Connect failed, reason: " << m_AES->GetLastMessage() << Endl;
			SetOnline();
			NotifyOffline(m_AES->GetLastMessage());
			return -1;
		}

		delete m_AES;
		m_AES = new AESClient(url + ":" + port + "/api");
		if (!m_AES->CreateRepository(repo, "git"))
		{
			GetConnection().Log().Debug() << "Creation failed, reason: " << m_AES->GetLastMessage() << Endl;
			SetOnline();
			NotifyOffline(m_AES->GetLastMessage());
			return -1;
		}

		delete m_AES;
		m_AES = new AESClient(url + ":" + port + "/api/files/" + repo);
		if (!m_AES->Ping())
		{
			GetConnection().Log().Debug() << "Connect failed, reason: " << m_AES->GetLastMessage() << Endl;
			SetOnline();
			NotifyOffline(m_AES->GetLastMessage());
			return -1;
		}
    }
    
    m_IsConnected = true;
    return 0;
}

void AESPlugin::Disconnect()
{
    GetConnection().Log().Debug() << "Disconnect" << Endl;
    if (!IsConnected())
        return;

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
        GetConnection().Log().Debug() << "Login failed, reason: " << m_AES->GetLastMessage() << Endl;
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
	
    return 0;
}

bool AESPlugin::CheckConnectedAndLogged()
{
    GetConnection().Log().Debug() << "CheckConnectedAndLogged" << Endl;
    if (!IsConnected())
    {
        if (Connect() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot connect to AES, reason: ") + (m_AES ? m_AES->GetLastMessage() : "not connected to AES")));
            return false;
        }
        
        if (Login() != 0)
        {
            StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot log to to AES, reason: ") + m_AES->GetLastMessage()));
            return false;
        }

		StatusAdd(VCSStatusItem(VCSSEV_OK, "Successfully connected to AES"));
	}
	else
	{
		if (!m_AES->Ping())
		{
            StatusAdd(VCSStatusItem(VCSSEV_Error, string("Connection lost, reason: ") + m_AES->GetLastMessage()));
			SetOnline();
			NotifyOffline("Connection lost");
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
		
		if (state == kDeletedLocal && (asset.GetState() & kLocal) == kLocal)
		{
			// Local change deleted
			GetConnection().Log().Debug() << "AddAssetsToChanges local change deleted for " << path << Endl;
			m_LocalChangesEntries.erase(path);
			continue;
		}

		state = UpdateStateForMeta(path, state);
		
		GetConnection().Log().Debug() << "AddAssetsToChanges " << path << Endl;
		state |= asset.GetState();
		state &= ~kSynced;
		
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
        //assetList.clear();
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
        //assetList.clear();
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
        //toAssetList.clear();
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
        //assetList.clear();
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
        //assetList.clear();
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
        //assetList.clear();
        return false;
    }
    
    RemoveAssetsFromChanges(assetList, kLockedLocal);
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::DownloadAssets(const string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }

	if (!EnsureDirectory(targetDir))
	{
		GetConnection().ErrorLine(string("Could not create temp dir: ") + targetDir);
		//assetList.clear();
		return false;
	}
	
	VersionedAssetList result;
	int idx = 0;
	for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++, ++idx)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		for (ChangelistRevisions::const_iterator j = changes.begin() ; j != changes.end() ; j++)
		{
			const string& revision = *j;
			if (revision == kDefaultListRevision || revision == "head")
			{
				AESEntry* entry = m_SnapShotEntries.search(path);
				string target = targetDir + "/" + IntToString(idx) + "_" + m_SnapShotRevision;
				if (entry != NULL)
				{
					if (!m_AES->Download(entry, path, target))
					{
						GetConnection().Log().Debug() << "Cannot download " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
						//assetList.clear();
						return false;
					}
					
					result.push_back(VersionedAsset(target, kNone, m_SnapShotRevision));
				}
			}
			else if (revision == "mineAndConflictingAndBase")
			{
				AESEntry* entry = m_LocalChangesEntries.search(path);
				if (entry == NULL)
				{
					GetConnection().Log().Notice() << "No 'mine' file " << path << Endl;
					GetConnection().ErrorLine(string("No 'mine' file for ") + path);
					continue;
				}
				else
				{
					string target = targetDir + "/" + IntToString(idx) + "_" + kLocalRevison;
					if (!CopyAFile(asset.GetPath(), target, true))
					{
						GetConnection().Log().Debug() << "Cannot copy " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
						//assetList.clear();
						return false;
					}
					
					result.push_back(VersionedAsset(target, kNone, kLocalRevison));
				}
				
				entry = m_RemoteChangesEntries.search(path);
				if (entry == NULL)
				{
					GetConnection().Log().Notice() << "No 'theirs' file " << path << Endl;
					GetConnection().ErrorLine(string("No 'theirs' file for ") + path);
					continue;
				}
				else
				{
					string target = targetDir + "/" + IntToString(idx) + "_" + m_LatestRevision;
					if (!m_AES->Download(entry, path, target))
					{
						GetConnection().Log().Debug() << "Cannot download " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
						//assetList.clear();
						return false;
					}
					
					result.push_back(VersionedAsset(target, kNone, m_LatestRevision));
				}
				
				entry = m_SnapShotEntries.search(path);
				if (entry == NULL)
				{
					GetConnection().Log().Notice() << "No 'base' file " << path << Endl;
					GetConnection().ErrorLine(string("No 'base' file for ") + path);
					continue;
				}
				else
				{
					string target = targetDir + "/" + IntToString(idx) + "_" + m_SnapShotRevision;
					if (!m_AES->Download(entry, path, target))
					{
						GetConnection().Log().Debug() << "Cannot download " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
						return true;
					}
					
					result.push_back(VersionedAsset(target, kNone, m_SnapShotRevision));
				}
			}
			else
			{
				GetConnection().Log().Debug() << "DownloadAssets, unknown revision " << revision << Endl;
			}
		}
	}
	
	assetList.swap(result);
	return true;
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
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
			string target = GetProjectPath() + "/" + path;
			if (!m_AES->Download(entry, path, target))
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

int AESPlugin::ApplyRemoteChangesCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	AESEntry* localEntry = plugin->m_LocalChangesEntries.search(key);
	if (localEntry != NULL)
	{
		// Potential conflict
		if (localEntry->GetSize() != entry->GetSize() || localEntry->GetTimeStamp() != entry->GetTimeStamp())
		{
			plugin->GetConnection().Log().Debug() << "Conflict for " << key << Endl;
			localEntry->SetState(kConflicted);
		}
	}
	else
	{
		// Handle added and deleted files
		if ((entry->GetState() & kAddedRemote) == kAddedRemote)
		{
			// Download file
			string target = plugin->GetProjectPath() + "/" + key;
			if (!plugin->m_AES->Download(entry, key, target))
			{
				plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << plugin->m_AES->GetLastMessage() << Endl;
				return 1;
			}
			
			// Update Snapshot
			int state = kSynced;
			state = UpdateStateForMeta(key, state);
			
			string localPath = plugin->GetProjectPath() + "/" + key;
			uint64_t size = 0;
			time_t ts = 0;
			GetAFileInfo(localPath, &size, NULL, &ts);
			
			plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
			plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), state, size, false, ts));
		}
		else if ((entry->GetState() & kDeletedRemote) == kDeletedRemote)
		{
			// Delete local file
			string localPath = plugin->GetProjectPath() + "/" + key;
			plugin->GetConnection().Log().Debug() << "Delete file from remote change: " << localPath << Endl;
			if (!DeleteRecursive(localPath))
			{
				plugin->GetConnection().Log().Debug() << "Cannot delete " << localPath << Endl;
				return 1;
			}
			
			// Update snapshot
			plugin->m_SnapShotEntries.erase(key);
		}
	}
	return 0;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }
	
	GetConnection().Log().Debug() << "GetAssets apply remote changes (RevisionID: " << m_LatestRevision << ")" << Endl;
	m_RemoteChangesEntries.iterate(ApplyRemoteChangesCallBack, (void*)this);
	m_SnapShotRevision = m_LatestRevision;
	m_RemoteChangesEntries.clear();
	
	return GetAssetsStatus(assetList);
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;

    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }

	for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
			// Skip folders
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		switch (method)
		{
			case VersionControlPlugin::kMine:
				{
					AESEntry* entry = m_LocalChangesEntries.search(path);
					if (entry != NULL)
					{
						int state = entry->GetState();
						state |= kCheckedOutLocal;
						state &= ~kConflicted;
						entry->SetState(state);
					}
				}
				break;

			case VersionControlPlugin::kTheirs:
				{
					AESEntry* entry = m_RemoteChangesEntries.search(path);
					if (entry != NULL)
					{
						string target = GetProjectPath()+ "/" + path;
						if (!m_AES->Download(entry, path, target))
						{
							GetConnection().Log().Debug() << "Cannot download " << path << ", reason:" << m_AES->GetLastMessage() << Endl;
							//assetList.clear();
							return false;
						}

						m_RemoteChangesEntries.erase(path);
					}
				}
				break;

			case VersionControlPlugin::kMerged:
				GetConnection().Log().Debug() << "ResolveAssets, kMerged not yet supported" << Endl;
				break;
		}
	}
	
	return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SetRevision" << Endl;
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
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

int AESPlugin::ApplyLocalChangesCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	if ((entry->GetState() & kSynced) == kSynced)
	{
		plugin->GetConnection().Log().Debug() << "Add/Update in snapshot " << key << Endl;
		plugin->m_SnapShotEntries.insert(key, AESEntry(plugin->m_SnapShotRevision, entry->GetHash(), entry->GetState(), entry->GetSize(), false, entry->GetTimeStamp()));
	}
	else if ((entry->GetState() & kDeletedLocal) == kDeletedLocal)
	{
		plugin->GetConnection().Log().Debug() << "Delete in snapshot " << key << Endl;
		plugin->m_SnapShotEntries.erase(key);
	}
	return 0;
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList, bool saveOnly)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }

	if (m_RemoteChangesEntries.size() > 0)
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Incoming changes still pending. Pull changes before submitting again."));
		GetConnection().WarnLine(IntToString(m_RemoteChangesEntries.size()) + " incoming change(s) not applied.", MAGeneral);
		//assetList.clear();
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
			state = UpdateStateForMeta(path, kSynced);
			entriesToAddOrMod.insert(path, AESEntry(kLocalRevison, "", state, size, false, ts));
		}
		else if ((state & kDeletedLocal) == kDeletedLocal)
		{
			GetConnection().Log().Debug() << "Remove " << path << Endl;
			state = UpdateStateForMeta(path, kDeletedLocal);
			entriesToDelete.insert(path, AESEntry(kLocalRevison, "", state));
		}
    }
	
	int succeededEntries;
	if (!m_AES->ApplyChanges(GetProjectPath(), entriesToAddOrMod, entriesToDelete, changeList.GetDescription(), &succeededEntries))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges failed, reason: " << m_AES->GetLastMessage() << Endl;
		//assetList.clear();
        return false;
	}
	else
	{
		if (succeededEntries != (entriesToAddOrMod.size() + entriesToDelete.size()))
		{
			GetConnection().Log().Debug() << "AES ApplyChanges failed, not all files were submitted" << Endl;
			//assetList.clear();
			return false;
		}
		GetConnection().Log().Debug() << "AES ApplyChanges success" << Endl;
	}
	
	if (!m_AES->GetLatestRevision(m_SnapShotRevision))
	{
		GetConnection().Log().Fatal() << "AES ApplyChanges cannot get latest revision, reason: " << m_AES->GetLastMessage() << Endl;
		//assetList.clear();
        return false;
	}
	
	entriesToAddOrMod.iterate(ApplyLocalChangesCallBack, (void*)this);
	entriesToDelete.iterate(ApplyLocalChangesCallBack, (void*)this);
	
	RemoveAssetsFromChanges(assetList);
    
    // Make sure refresh remote will be performed
    m_LastRemoteChangesRefrehTime = 0;
    
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode)
{
    GetConnection().Log().Debug() << "SetAssetsFileMode" << Endl;
	GetConnection().Log().Trace() << "### NOT SUPPORTED ###" << Endl;
    assetList.clear();
    return false;
}

bool AESPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    GetConnection().Log().Debug() << "GetAssetsStatus (" << (recursive ? "" : "Non-") << "Recursive)" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }
	
	if (!RefreshRemote())
	{
		GetConnection().Log().Debug() << "Cannot refresh remote" << Endl;
	}
    
    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
		
		if (asset.IsFolder())
		{
            ResetTimer();
            GetConnection().Progress(-1, 0, "secs. while scanning local changes...");
			ScanDirectory(asset.GetPath(), false, ScanLocalChangeCallBack, (void*)this);
            
            // Skip folders
            i = assetList.erase(i);
            continue;
		}

        int state = kNone;
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* localEntry = m_LocalChangesEntries.search(path);
		AESEntry* remoteEntry = m_RemoteChangesEntries.search(path);
		AESEntry* snapShotEntry = m_SnapShotEntries.search(path);
		
		if (localEntry == NULL)
		{
			state = kLocal;
			if (remoteEntry != NULL && snapShotEntry == NULL)
			{
				state = kOutOfSync;
			}
			else if (snapShotEntry != NULL)
			{
                state = kSynced;
			}
		}
		else
		{
			state = localEntry->GetState();
			if (remoteEntry != NULL && localEntry->GetRevisionID() != remoteEntry->GetRevisionID())
			{
				state = kConflicted;
			}
		}
		
		state = UpdateStateForMeta(path, state);
        asset.SetState(state);
        
        i++;
    }
    
    return true;
}

int AESPlugin::CheckConflictCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	AESEntry* remoteEntry = plugin->m_RemoteChangesEntries.search(key);
	if (remoteEntry != NULL && entry->GetRevisionID() != remoteEntry->GetRevisionID())
	{
		entry->SetState(kConflicted);
	}
	return 0;
}

bool AESPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus (" << revision << ")" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }
    
    assetList.clear();
    if (revision == kNewListRevision)
    {
		m_LocalChangesEntries.iterate(CheckConflictCallBack, (void*)this);
		EntriesToAssets(m_LocalChangesEntries, assetList, kSynced);
    }
    return true;
}

bool AESPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetIncomingAssetsChangeStatus" << Endl;
    
    if (!CheckConnectedAndLogged())
    {
        //assetList.clear();
        return false;
    }
    
    assetList.clear();
	
	ChangelistRevisions::const_iterator i = find(m_Revisions.begin(), m_Revisions.end(), revision);
	if (i == m_Revisions.end())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot find revision ") + revision));
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
			EntriesToAssets(entries, assetList);
		}
		else
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
		}
	}
	else
	{
		GetConnection().Log().Debug() << "Compare revision " << revision << " with " << compareTo << Endl;
		if (m_AES->GetRevisionDelta(revision, compareTo, entries))
		{
			EntriesToAssets(entries, assetList);
		}
		else
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
		}
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
				comment.append(" by ");
				comment.append(rev.GetComitterEmail());
				comment.append(" on ");
				comment.append(ToTimeLong(rev.GetTimeStamp()));

				item.SetCommitter(rev.GetComitterName());
				item.SetDescription(comment);
				item.SetRevision(rev.GetRevisionID());
				item.SetTimestamp(ToTime(rev.GetTimeStamp()));
				changes.push_back(item);
			}
			
			m_Revisions.push_back(rev.GetRevisionID());
        }
		reverse(changes.begin(), changes.end());
    }
	else
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
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
	GetConnection().Log().Trace() << "### NOT SUPPORTED ###" << Endl;
    return false;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
	GetConnection().Log().Trace() << "### NOT SUPPORTED ###" << Endl;
    //assetList.clear();
    return false;
}

typedef struct {
	AESPlugin* plugin;
	size_t size;
	size_t iter;
} FetchAllCallBackData;

int AESPlugin::FetchAllCallBack(void *data, const string& key, AESEntry *entry)
{
	FetchAllCallBackData* d = (FetchAllCallBackData*) data;
	if (!entry->IsDir())
	{
		// Delete file
		string localPath = d->plugin->GetProjectPath() + "/" + key;
		::DeleteRecursive(localPath);
		
		// Download file
		string target = d->plugin->GetProjectPath() + "/" + key;
		//int pct = (int)(d->iter*100/d->size);
		d->plugin->GetConnection().Progress(-1, d->plugin->GetTimerSoFar(), string("Downloading file ") + key);
		if (!d->plugin->m_AES->Download(entry, key, target))
		{
			d->plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << d->plugin->m_AES->GetLastMessage() << Endl;
			return 1;
		}
		
		// Update Snapshot
		int state = kSynced;
		state = UpdateStateForMeta(key, state);
		
		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(localPath, &size, NULL, &ts);
		
		d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
		d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), state, size, false, ts));
	}
	else
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;
		::EnsureDirectory(localPath);
        /*
		d->plugin->GetConnection().Log().Debug() << "Add directory from remote change: " << localPath << Endl;
		d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), kSynced, 0, true, 0));
        */
	}
	
	d->iter = d->iter+1;
	return 0;
}

int AESPlugin::CleanupLocalCallBack(void* data, const string& path, uint64_t size, bool isDirectory, time_t ts)
{
	AESPlugin* plugin = (AESPlugin*)data;
	string p = path.substr(plugin->GetProjectPath().size()+1);
	AESEntry* entry = plugin->m_SnapShotEntries.search(p);
	if (entry == NULL)
	{
		if (!isDirectory)
		{
			plugin->GetConnection().Log().Debug() << "Delete " << path << " because not part of snapshot" << Endl;
			::DeleteRecursive(path);
		}
		else
		{
			p.append(kMetaExtension);
			entry = plugin->m_SnapShotEntries.search(p);
			if (entry == NULL)
			{
				plugin->GetConnection().Log().Debug() << "Delete " << path << " because not part of snapshot" << Endl;
				::DeleteRecursive(path);
			}
		}
	}
	return 0;
}

typedef struct {
	AESPlugin* plugin;
	size_t size;
	size_t iter;
	TreeOfEntries* ignoredEntries;
} UpdateToRevisionCallBackData;

int AESPlugin::UpdateToRevisionCallBack(void *data, const string& key, AESEntry *entry)
{
	UpdateToRevisionCallBackData* d = (UpdateToRevisionCallBackData*) data;
	if (!entry->IsDir())
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;

		// Find if ignored or not
		AESEntry* ignoredEntry = d->ignoredEntries->search(key);
		if (ignoredEntry == NULL)
		{
			// Delete file
			::DeleteRecursive(localPath);

			// Download file
			string target = d->plugin->GetProjectPath() + "/" + key;
			//int pct = (int)(d->iter*100/d->size);
			d->plugin->GetConnection().Progress(-1, d->plugin->GetTimerSoFar(), string("Downloading file ") + key);
			if (!d->plugin->m_AES->Download(entry, key, target))
			{
				d->plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << d->plugin->m_AES->GetLastMessage() << Endl;
				return 1;
			}

			// Remove from local changes
			d->plugin->m_LocalChangesEntries.erase(key);
		}

		// Update Snapshot
		int state = kSynced;
		state = UpdateStateForMeta(key, state);

		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(localPath, &size, NULL, &ts);

		d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
		d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), state, size, false, ts));
	}

	d->iter = d->iter+1;
	return 0;
}

bool AESPlugin::UpdateToRevision(const ChangelistRevision& revision, const VersionedAssetList& ignoredAssetList, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UpdateToRevision " << Endl;

    if (!CheckConnectedAndLogged())
    {
        return false;
    }

	assetList.clear();
	vector<AESRevision> revisions;
	if (m_AES->GetRevisions(revisions))
	{
		bool revisionFound = false;
		ChangelistRevision lastRevision;
		for (vector<AESRevision>::const_iterator i = revisions.begin() ; i != revisions.end() && !revisionFound; i++)
		{
			const AESRevision& rev = (*i);
			if (lastRevision.empty())
			{
				lastRevision = rev.GetRevisionID();
			}

			if (rev.GetRevisionID() == revision)
			{
				revisionFound = true;
			}
		}

		if (revision == kLatestRevison)
		{
			revisionFound = !lastRevision.empty();
		}

		if (!revisionFound)
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, string("Unable to find revision ") + revision));
			return true;
		}

		TreeOfEntries ignoredEntries;
		for (VersionedAssetList::const_iterator i = ignoredAssetList.begin() ; i != ignoredAssetList.end() ; i++)
		{
			const VersionedAsset& asset = (*i);
			if (asset.IsFolder())
			{
				// Skip folders
				continue;
			}

			string path = asset.GetPath().substr(GetProjectPath().size()+1);
			int state = UpdateStateForMeta(path, kCheckedOutLocal);
			ignoredEntries.insert(path, AESEntry(kLocalRevison, "", state));
		}

		TreeOfEntries entries;
		ChangelistRevision newRevision = (revision == kLatestRevison) ? lastRevision : revision;
		if (m_AES->GetRevision(newRevision, entries))
		{
			m_SnapShotEntries.clear();
			m_SnapShotRevision = newRevision;

			if (entries.size() > 0)
			{
				UpdateToRevisionCallBackData data;
				data.plugin = this;
				data.size = entries.size();
				data.iter = 0;
				data.ignoredEntries = &ignoredEntries;

				entries.iterate(UpdateToRevisionCallBack, (void*)&data);
			}

			// Apply local changes
			if (!RefreshLocal())
			{
				GetConnection().Log().Debug() << "Cannot refresh local" << Endl;
			}

			EntriesToAssets(m_SnapShotEntries, assetList);
			for (VersionedAssetList::const_iterator i = ignoredAssetList.begin() ; i != ignoredAssetList.end() ; i++)
			{
				const VersionedAsset& asset = (*i);
				if (asset.IsFolder())
				{
					// Skip folders
					continue;
				}

				string path = asset.GetPath().substr(GetProjectPath().size()+1);
				if (m_SnapShotEntries.search(path) == NULL)
				{
					assetList.push_back(asset);
				}
			}

			return GetAssetsStatus(assetList, false);
		}
		else
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
		}
	}
	else
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
	}
	
	return true;
}

bool AESPlugin::GetCurrentRevision(ChangelistRevision& revision)
{
    GetConnection().Log().Debug() << "GetCurrentRevision" << Endl;

    if (!CheckConnectedAndLogged())
    {
        return false;
    }

	revision = m_SnapShotRevision;
	if (revision.empty())
	{
		revision = kDefaultListRevision;
	}

	return true;
}

bool AESPlugin::FetchAllAssets()
{
    GetConnection().Log().Debug() << "FetchAllAssets" << Endl;
	
    if (!CheckConnectedAndLogged())
    {
        return false;
    }
	
	m_LatestRevision.clear();
	m_LocalChangesEntries.clear();
	m_SnapShotEntries.clear();
	m_SnapShotRevision.clear();

	if (!RefreshRemote())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh remote"));
		return true;
	}

	GetConnection().Log().Debug() << "FetchAllAssets apply remote changes (RevisionID: " << m_LatestRevision << ")" << Endl;
	
	ResetTimer();
	
	if (m_RemoteChangesEntries.size() > 0)
	{
        FetchAllCallBackData data;
        data.plugin = this;
        data.size = m_RemoteChangesEntries.size();
        data.iter = 0;
        
        m_RemoteChangesEntries.iterate(FetchAllCallBack, (void*)&data);
    }
    
	m_SnapShotRevision = m_LatestRevision;
	m_RemoteChangesEntries.clear();
	
	// Scan project directories to removed non-versionned file
	ScanDirectory(GetProjectPath()+"/Assets/", true, CleanupLocalCallBack, (void*)this);
	ScanDirectory(GetProjectPath()+"/ProjectSettings/", true, CleanupLocalCallBack, (void*)this);

	GetConnection().Log().Trace() << "Local:" << Endl;
	m_LocalChangesEntries.iterate(PrintAllCallBack, (void*)this);
	
	GetConnection().Log().Trace() << "Snapshot:" << Endl;
	m_SnapShotEntries.iterate(PrintAllCallBack, (void*)this);
	
	GetConnection().Log().Trace() << "Remote:" << Endl;
	m_RemoteChangesEntries.iterate(PrintAllCallBack, (void*)this);
	
	return true;
}

bool AESPlugin::PerformCustomCommand(const string& command, const CommandArgs& args)
{
    GetConnection().Log().Debug() << "PerformCustomCommand with parameters: ";
	for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i)
	{
		GetConnection().Log().Debug() << (*i) << " ";
	}
	GetConnection().Log().Debug() << Endl;

	if (command == kFetchAllCommand)
	{
		return FetchAllAssets();
	}

	GetConnection().Log().Debug() << "PerformCustomCommand unkown command " << command << Endl;
	
    return false;
}

typedef struct
{
	AESPlugin* plugin;
	VersionedAssetList* list;
	int ignoredState;
} EntryToAssetCallBackData;

int AESPlugin::EntryToAssetCallBack(void *data, const string& key, AESEntry *entry)
{
	EntryToAssetCallBackData* d = (EntryToAssetCallBackData*)data;
	if (!entry->IsDir() && (d->ignoredState == kNone || (entry->GetState() & d->ignoredState) != d->ignoredState))
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;
		// TODO: check in localChanges if potential conflict
		d->list->push_back(VersionedAsset(localPath, entry->GetState()));
	}
	return 0;
}

void AESPlugin::EntriesToAssets(TreeOfEntries& entries, VersionedAssetList& assetList, int ignoredState)
{
	EntryToAssetCallBackData data;
	data.plugin = this;
	data.list = &assetList;
	data.ignoredState = ignoredState;
	entries.iterate(EntryToAssetCallBack, (void*)&data);
}