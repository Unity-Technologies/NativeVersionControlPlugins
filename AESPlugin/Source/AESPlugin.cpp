#if defined(_WINDOWS)
#include "windows.h"
#endif

#include "AESPlugin.h"
#include "FileSystem.h"
#include "JSON.h"
#include "openssl/md4.h"

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

static const char* kAESPluginVersion  = "1.0.10";

static const char* kCacheFileName  = "/Library/aesSnapshot_";
static const char* kCacheFileNameExt  = ".txt";
static const char* kLogFileName = "Library/aesPlugin.log";
static const char* kLatestRevison = "current";
static const char* kLocalRevison = "local";
static const char* kPluginName = "AssetExchangeServer";

static const char* kMetaExtension = ".meta";

static const char kHexChars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

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

#ifdef WIN32
const size_t kPathBufferSize = 1024;
const size_t kBufferSize = 1024 * 4;

bool GetAFileHash(const string& path, string& md4)
{
	unsigned char digest[MD4_DIGEST_LENGTH];
	wchar_t widePath[kPathBufferSize];

	// UTF8ToWide
	if (MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath, kPathBufferSize) == 0)
		widePath[0] = 0;

	// ConvertSeparatorsToWindows
	wchar_t* widePathPtr = &widePath[0];
	while (*widePathPtr != '\0')
	{
		if (*widePathPtr == '/') *widePathPtr = '\\';
		++widePathPtr;
	}

	HANDLE handle = CreateFileW(widePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == NULL)
		return false;

	char buffer[kBufferSize];
    DWORD bytesRead;
	MD4_CTX ctx;

	MD4_Init(&ctx);
	BOOL res = ReadFile(handle, buffer, kDefaultBufferSize, &bytesRead, 0);
	while (res == TRUE && bytesRead > 0)
	{
		MD4_Update(&ctx, buffer, bytesRead);
		res = ReadFile(handle, buffer, kDefaultBufferSize, &bytesRead, 0);
    }
	if (res == TRUE) MD4_Final(digest, &ctx);
	CloseHandle(handle);
	if (res != TRUE)
		return false;

	md4.clear();
	char tmp[MD4_DIGEST_LENGTH*2+1];
    for (int i = 0, j = 0; i < MD4_DIGEST_LENGTH; i++, j+=2)
	{
		tmp[j] = kHexChars[(digest[i] & 0xF0) >> 4];
		tmp[j+1] = kHexChars[(digest[i] & 0x0F) >> 0];
	}
	tmp[MD4_DIGEST_LENGTH*2] = '\0';
	md4 = tmp;

	return true;
}
#else
bool GetAFileHash(const string& path, string& md4)
{
	unsigned char digest[MD4_DIGEST_LENGTH];
	FILE* fp;
	if ((fp = fopen(path.c_str(), "rb")) == 0)
		return false;

	char buffer[BUFSIZ];
    size_t n;
	MD4_CTX ctx;

	MD4_Init(&ctx);
    while ((n = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
    {
		MD4_Update(&ctx, buffer, n);
    }
	MD4_Final(digest, &ctx);
	fclose(fp);

	md4.clear();
	char tmp[MD4_DIGEST_LENGTH*2+1];
    for (int i = 0, j = 0; i < MD4_DIGEST_LENGTH; i++, j+=2)
	{
		tmp[j] = kHexChars[(digest[i] & 0xF0) >> 4];
		tmp[j+1] = kHexChars[(digest[i] & 0x0F) >> 0];
	}
	tmp[MD4_DIGEST_LENGTH*2] = '\0';
	md4 = tmp;

	return true;
}
#endif

enum AESFields { kAESURL, kAESPort, kAESRepository, kAESUserName, kAESPassword, kAESCreateToggle, kAESAvailableRepositories };

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
	printf(" - %s (%s)\n", key.c_str(), entry->GetStateAsString().c_str());
	return 0;
}

void PrintAssets(const string& name, const VersionedAssetList& assetList)
{
	printf("%s:\n", name.c_str());
	if (assetList.empty())
	{
		printf(" - EMPTY\n");
		return;
	}

	for (VersionedAssetList::const_iterator i = assetList.begin() ; i != assetList.end() ; i++)
	{
		printf(" - %s (%s)\n", (*i).GetPath().c_str(), (*i).GetStateAsString().c_str());
	}
}

int AESPlugin::Test()
{
	GetConnection().Log().Debug() << "Test" << Endl;

	m_Fields[kAESURL].SetValue("http://localhost");
	m_Fields[kAESPort].SetValue("3030");
	m_Fields[kAESRepository].SetValue("EmptyRepo");
	m_Fields[kAESUserName].SetValue("");
	m_Fields[kAESPassword].SetValue("");
	m_Fields[kAESCreateToggle].SetValue("false");

	if (Connect() != 0)
		return -1;
	if (Login() != 0)
		return -1;

	string revision;
	if (!GetCurrentRevision(revision))
		return -1;
	string currRevisionID = revision.substr(0, revision.find(" "));
	printf("Current revision is [%s]\n", currRevisionID.c_str());

	if (!GetLatestRevision(revision))
		return -1;
	string lastRevisionID = revision.substr(0, revision.find(" "));
	printf("Latest revision is [%s]\n", lastRevisionID.c_str());

	/*
	VersionedAssetList list;
	if (!GetAssetsChangeStatus(kNewListRevision, list))
		return -1;
	PrintAssets("Outgoing changes", list);

	if (!MarkAssets(list, kUseTheirs))
		return -1;
	PrintAssets("Mark theirs", list);

	Changes changes;
	if (!GetAssetsIncomingChanges(changes))
		return -1;

	for (Changes::const_iterator i = changes.begin() ; i != changes.end() ; i++)
	{
		if (!GetIncomingAssetsChangeStatus((*i).GetRevision(), list))
			return -1;
		PrintAssets(string("Incoming changes for ")+ (*i).GetRevision(), list);
	}

	VersionedAssetList ignoredList;
	if (!UpdateToRevision(lastRevisionID, ignoredList, list))
		return -1;
	PrintAssets("UpdateToRevision", list);

	if (!GetCurrentRevision(revision))
		return -1;
	currRevisionID = revision.substr(0, revision.find(" "));
	printf("Current revision is [%s]\n", currRevisionID.c_str());

	if (!GetLatestRevision(revision))
		return -1;
	lastRevisionID = revision.substr(0, revision.find(" "));
	printf("Latest revision is [%s]\n", lastRevisionID.c_str());

	if (!GetAssetsChangeStatus(kNewListRevision, list))
		return -1;
	PrintAssets("Outgoing changes", list);
	 */

	Disconnect();
	
	return 0;
}

const char* AESPlugin::GetLogFileName()
{
	return kLogFileName;
}

const AESPlugin::TraitsFlags AESPlugin::GetSupportedTraitFlags()
{
	return (kRequireNetwork | kEnablesCheckout | kEnablesGetLatestOnChangeSetSubset);
}

AESPlugin::CommandsFlags AESPlugin::GetOnlineUICommands()
{
	return (kAdd | kChanges | kDelete | kDownload | kGetLatest | kIncoming | kIncomingChangeAssets | kStatus | kSubmit );
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
	m_Overlays[kConflicted] = "default";

	m_CustomCommands.clear();

    m_SnapShotRevision = kLocalRevison;
	m_SnapShotEntries.clear();
	m_LocalChangesEntries.clear();
	m_RemoteChangesEntries.clear();

	m_Revisions.clear();
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
	if (isDirectory || ShouldIgnoreFile(p))
	{
		return 0;
	}
	
	string md4 = "00000000000000000000000000000000";
	GetAFileHash(path, md4);

	// search in local changes
	AESEntry* localEntry = plugin->m_LocalChangesEntries.search(p);
	if (localEntry == NULL)
	{
		// Not found, search in snapshot
		AESEntry* snapShotEntry = plugin->m_SnapShotEntries.search(p);
		if (snapShotEntry != NULL)
		{
			// Found, compare Hash
			if (snapShotEntry->GetHash() != md4)
			{
				// Local file has been modified, add it to local changes
				plugin->m_LocalChangesEntries.insert(p, AESEntry(kLocalRevison, md4, UpdateStateForMeta(p, kAddedLocal | kCheckedOutLocal), (int)size, isDirectory, ts));
			}
			else
			{
				// Local file is synced
				plugin->m_LocalChangesEntries.insert(p, AESEntry(snapShotEntry->GetRevisionID(), md4, UpdateStateForMeta(p, kSynced), (int)size, isDirectory, ts));
			}
		}
		else
		{
			// Local file is new, add it to local changes
			plugin->m_LocalChangesEntries.insert(p, AESEntry(kLocalRevison, md4, UpdateStateForMeta(p, kAddedLocal), (int)size, isDirectory, ts));
		}
	}
	else
	{
		// Found, search in snapshot
		AESEntry* snapShotEntry = plugin->m_SnapShotEntries.search(p);
		if (snapShotEntry != NULL)
		{
			// Found, compare Hash
			if (snapShotEntry->GetHash() != md4)
			{
				// Local file has been modified, update it to local changes
				localEntry->AddState(kCheckedOutLocal);
				localEntry->RemoveState(kSynced);
			}
			else
			{
				// Local file is up to date
				localEntry->RemoveState(kCheckedOutLocal);
				localEntry->AddState(kSynced);
			}
		}
		else
		{
			// Local file/path is updated
			if (localEntry->GetHash() == md4)
				localEntry->RemoveState(kCheckedOutLocal);

			localEntry->RemoveState(kSynced);
			localEntry->AddState(kAddedLocal);
		}

		// Update entry
		localEntry->SetHash(md4);
		localEntry->SetSize(size);
		localEntry->SetTimeStamp(ts);

	}
	return 0;
}

int AESPlugin::SnapshotRemovedCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;
	string localPath = plugin->GetProjectPath() + "/" + key;
	if (!PathExists(localPath))
	{
		plugin->m_LocalChangesEntries.insert(key, AESEntry(entry->GetRevisionID(), "00000000000000000000000000000000", UpdateStateForMeta(key, kDeletedLocal), 0, entry->IsDir(), 0));
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
	ScanDirectory(GetProjectPath()+"/Assets", true, ScanLocalChangeCallBack, (void*)this);
	ScanDirectory(GetProjectPath()+"/ProjectSettings", true, ScanLocalChangeCallBack, (void*)this);
	
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
				m_LocalChangesEntries.insert(path, AESEntry(entry->GetRevisionID(), "00000000000000000000000000000000", UpdateStateForMeta(path, kDeletedLocal), 0, entry->IsDir(), 0));
			}
		}
	}
	
	// Scan removed entries
	int res = m_SnapShotEntries.iterate(SnapshotRemovedCallBack, (void*)this);
	ELAPSED_CLOCK("RefreshLocal")

	if (res != 0)
		GetConnection().Log().Debug() << "Snapshot removed callBack failed." << Endl;

	return (res == 0);
}

bool AESPlugin::RefreshRemote()
{
	START_CLOCK()
	GetConnection().Log().Debug() << "RefreshRemote" << Endl;
	
	if (!m_IsConnected)
		return false;
	
	// Get latest revision from remote
	AESRevision latestRevision;
	if (!m_AES->GetLatestRevision(latestRevision))
	{
		GetConnection().Log().Debug() << "Cannot get latest revision from AES, reason: " << m_AES->GetLastMessage() << Endl;
		return false;
	}

	if (m_LatestRevision.GetRevisionID() == latestRevision.GetRevisionID())
	{
		GetConnection().Log().Debug() << "RefreshRemote skipped" << Endl;
		return true;
	}
	
	m_LatestRevision = latestRevision;
	if (m_SnapShotRevision == kLocalRevison)
	{
		// If no snapshot was taken, treats latest revision as remote changes
		if (!m_AES->GetRevision(m_LatestRevision.GetRevisionID(), m_RemoteChangesEntries))
		{
			GetConnection().Log().Debug() << "Cannot get remote changes from AES, reason: " << m_AES->GetLastMessage() << Endl;
			return false;
		}
	}
	else if (m_SnapShotRevision != m_LatestRevision.GetRevisionID())
	{
		// Get delta from snapshot revision and latest revision
		if (!m_AES->GetRevisionDelta(m_LatestRevision.GetRevisionID(), m_SnapShotRevision, m_RemoteChangesEntries))
		{
			GetConnection().Log().Debug() << "Cannot get remote changes from AES, reason: " << m_AES->GetLastMessage() << Endl;
			return false;
		}
	}
	
	ELAPSED_CLOCK("RefreshRemote")
	
	return true;
}

typedef struct {
	JSONArray* array;
	int ignoredState;
	int removeState;
} EntryToJSONCallBackData;

int AESPlugin::EntryToJSONCallBack(void *data, const string& key, AESEntry *entry)
{
	EntryToJSONCallBackData* d = (EntryToJSONCallBackData*) data;
	if (d->ignoredState != kNone && entry->HasState(d->ignoredState))
	{
		return 0;
	}

	int state = entry->GetState();
	state &= ~(d->removeState);

	JSONObject elt;
	elt["path"] = new JSONValue(key);
	elt["hash"] = new JSONValue(entry->GetHash());
	elt["directory"] = new JSONValue(entry->IsDir());
	elt["state"] = new JSONValue((double)state);
	elt["size"] = new JSONValue((double)entry->GetSize());
	elt["mtime"] = new JSONValue((double)entry->GetTimeStamp());

	d->array->push_back(new JSONValue(elt));
	return 0;
}

bool AESPlugin::SaveSnapShotToFile(const string& path)
{
	GetConnection().Log().Debug() << "SaveSnapShotToFile " << path << Endl;
	
	// Build JSON array from snapshot entries
	JSONArray entries;
	EntryToJSONCallBackData data;
	data.array = &entries;
	data.ignoredState = kNone;
	data.removeState = kMarkUseMine | kMarkUseTheirs | kAddedLocal | kDeletedLocal | kCheckedOutLocal;
	int res = m_SnapShotEntries.iterate(EntryToJSONCallBack, (void*)&data);
	if (res != 0)
	{
		GetConnection().Log().Debug() << "EntryToJSON iterate callback failed" << Endl;
		return false;
	}

	// Build JSON array from local changes
	JSONArray changes;
	data.array = &changes;
	data.ignoredState = kSynced;
	data.removeState = kMarkUseMine | kMarkUseTheirs;
	res = m_LocalChangesEntries.iterate(EntryToJSONCallBack, (void*)&data);
	if (res != 0)
	{
		GetConnection().Log().Debug() << "EntryToJSON iterate callback failed" << Endl;
		return false;
	}

	// Build JSON object
	JSONObject obj;
	obj["entries"] = new JSONValue(entries);
	obj["changes"] = new JSONValue(changes);
	obj["snapshot"] = new JSONValue(m_SnapShotRevision);
	
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
	
	m_SnapShotRevision = kLocalRevison;
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
			if (m_SnapShotRevision.empty())
				m_SnapShotRevision = kLocalRevison;
		}
		
		if (obj.find("changes") != obj.end())
		{
			// Get local changes
			const JSONArray& arr = *(obj.at("changes"));
			for (vector<JSONValue*>::const_iterator i = arr.begin() ; i != arr.end() ; i++)
			{
				const JSONObject& entry = (*i)->AsObject();

				string path = *(entry.at("path"));
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "00000000000000000000000000000000";
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
				string hash = (entry.find("hash") != entry.end()) ? *(entry.at("hash")) : "00000000000000000000000000000000";
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

bool AESPlugin::RefreshSnapShot()
{
	START_CLOCK()
	GetConnection().Log().Debug() << "RefreshSnapShot" << Endl;

	if (!m_IsConnected)
		return false;

    m_SnapShotRevision = kLocalRevison;
	m_SnapShotEntries.clear();
	m_LocalChangesEntries.clear();

	string repo = m_Fields[kAESRepository].GetValue();
	if (!repo.empty())
	{
		string aesCache = GetProjectPath();
		aesCache.append(kCacheFileName);
		aesCache.append(repo);
		aesCache.append(kCacheFileNameExt);
		if (PathExists(aesCache))
		{
			if (!RestoreSnapShotFromFile(aesCache))
			{
				StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh snapshot from AES cache file"));
				return false;
			}
		}
	}

	ELAPSED_CLOCK("RefreshSnapShot")

	return true;
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

	string repo = m_Fields[kAESRepository].GetValue();
	if (!repo.empty())
	{
		string aesCache = GetProjectPath();
		aesCache.append(kCacheFileName);
		aesCache.append(repo);
		aesCache.append(kCacheFileNameExt);
		if (!SaveSnapShotToFile(aesCache))
		{
			GetConnection().Log().Debug() << "Cannot save snapshot to AES cache file" << Endl;
		}
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
	
	// Get remote changes
	if (!RefreshRemote())
	{
		GetConnection().Log().Debug() << "Cannot refresh remote" << Endl;
	}
	
	// Apply local changes
	if (!RefreshLocal())
	{
		GetConnection().Log().Debug() << "Cannot refresh local" << Endl;
	}

    return 0;
}

bool AESPlugin::EnsureConnected()
{
    GetConnection().Log().Debug() << "EnsureConnected" << Endl;
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

bool AESPlugin::AddAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "AddAssets" << Endl;
    
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
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
		AESEntry* entry = m_LocalChangesEntries.search(path);

		uint64_t size = 0;
		time_t ts = 0;
		string md4 = "00000000000000000000000000000000";

		GetAFileInfo(asset.GetPath(), &size, NULL, &ts);
		GetAFileHash(asset.GetPath(), md4);

		if (entry != NULL)
		{
			// Update entry
			entry->SetHash(md4);
			entry->SetSize(size);
			entry->SetTimeStamp(ts);
			entry->AddState(kAddedLocal);
			continue;
		}

		m_LocalChangesEntries.insert(path, AESEntry(kLocalRevison, md4, UpdateStateForMeta(path, kAddedLocal), size, false, ts));
	}
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::RemoveAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RemoveAssets" << Endl;
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
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
		AESEntry* entry = m_LocalChangesEntries.search(path);

		if (entry == NULL)
		{
			// Not in changes
			continue;
		}

		m_LocalChangesEntries.erase(path);
	}
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    GetConnection().Log().Debug() << "MoveAssets" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    toAssetList.clear();
    return true;
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
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
		AESEntry* entry = m_LocalChangesEntries.search(path);

		if (entry == NULL)
		{
			// Not in changes
			GetConnection().Log().Debug() << "CheckoutAssets asset '" << path << "' not in local changes" << Endl;
			continue;
		}

		entry->AddState(kCheckedOutLocal);
	}
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

bool AESPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

bool AESPlugin::DownloadAssets(const string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

int AESPlugin::ApplyRemoteChangesCallBack(void *data, const string& key, AESEntry *entry)
{
	AESPlugin* plugin = (AESPlugin*)data;

	// Handle added and deleted files
	if (entry->HasState(kAddedRemote))
	{
		// Download file
		string target = plugin->GetProjectPath() + "/" + key;
		if (!plugin->m_AES->Download(entry, key, target))
		{
			plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << plugin->m_AES->GetLastMessage() << Endl;
			return 1;
		}
		
		// Update Snapshot
		string localPath = plugin->GetProjectPath() + "/" + key;
		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(localPath, &size, NULL, &ts);
		
		plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
		plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), UpdateStateForMeta(key, kSynced), size, false, ts));
	}
	else if (entry->HasState(kDeletedRemote))
	{
		// Delete local file
		string localPath = plugin->GetProjectPath() + "/" + key;
		plugin->GetConnection().Log().Debug() << "Delete file from remote change: " << localPath << Endl;
		if (!DeleteRecursive(localPath))
		{
			plugin->StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot delete ") + localPath));
			return 1;
		}
		
		// Update snapshot
		plugin->m_SnapShotEntries.erase(key);
	}

	return 0;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	if (!RefreshRemote())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh remote"));
		return true;
	}
	
	GetConnection().Log().Debug() << "GetAssets apply remote changes (RevisionID: " << m_LatestRevision.GetRevisionID() << ")" << Endl;
	int res = m_RemoteChangesEntries.iterate(ApplyRemoteChangesCallBack, (void*)this);
	if (res != 0)
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Apply remote changes iterate callback failed."));
		return true;
	}

	m_SnapShotRevision = m_LatestRevision.GetRevisionID();
	m_RemoteChangesEntries.clear();
	
	return GetAssetsStatus(assetList);
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	VersionedAssetList::iterator i = assetList.begin();
	while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
            // Skip folders
            i = assetList.erase(i);
            continue;
        }

        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry == NULL)
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, string("Asset not found in local changes ") + path));
			return true;
		}

		switch (method)
		{
			case VersionControlPlugin::kMine:
				{
					GetConnection().Log().Debug() << "ResolveAssets mine for " << path << Endl;
					entry->SetState(UpdateStateForMeta(path, kCheckedOutLocal | kMarkUseMine));
					asset.RemoveState(kConflicted);
				}
				break;

			case VersionControlPlugin::kTheirs:
				{
					AESEntry* remoteEntry = m_RemoteChangesEntries.search(path);
					if (remoteEntry != NULL)
					{
						if (remoteEntry->HasState(kAddedRemote) || remoteEntry->HasState(kCheckedOutRemote))
						{
							GetConnection().Log().Debug() << "ResolveAssets download " << path << Endl;
							string target = GetProjectPath()+ "/" + path;
							if (!m_AES->Download(remoteEntry, path, target))
							{
								StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
								assetList.clear();
								return true;
							}

							m_SnapShotEntries.insert(path, AESEntry(remoteEntry->GetRevisionID(), remoteEntry->GetHash(), UpdateStateForMeta(path, kSynced), remoteEntry->GetSize(), remoteEntry->IsDir(), remoteEntry->GetTimeStamp()));

							entry->SetState(UpdateStateForMeta(path, kSynced | kMarkUseTheirs));
							entry->SetHash(remoteEntry->GetHash());
							entry->SetRevisionID(remoteEntry->GetRevisionID());
							entry->SetSize(remoteEntry->GetSize());
							entry->SetTimeStamp(remoteEntry->GetTimeStamp());
						}
						else if (remoteEntry->HasState(kDeletedRemote))
						{
							if (!DeleteRecursive(asset.GetPath()))
							{
								StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot delete ") + path));
								assetList.clear();
								return true;
							}

							m_LocalChangesEntries.erase(path);
						}

						asset.RemoveState(kConflicted);
					}
					else
					{
						StatusAdd(VCSStatusItem(VCSSEV_Error, string("Asset not found in remote changes ") + path));
						assetList.clear();
						return true;
					}
				}
				break;

			case VersionControlPlugin::kMerged:
				{
					StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
					assetList.clear();
					return true;
				}
				break;
		}

		i++;
	}

	return true;
}

bool AESPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SetRevision" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

typedef struct {
	AESPlugin* plugin;
	string revisionID;
} ApplySubmitChangesCallBackData;

int AESPlugin::ApplySubmitChangesCallBack(void *data, const string& key, AESEntry *entry)
{
	ApplySubmitChangesCallBackData* d = (ApplySubmitChangesCallBackData*)data;
	if ((entry->GetState() & kSynced) == kSynced || (entry->GetState() & kAddedLocal) == kAddedLocal || (entry->GetState() & kCheckedOutLocal) == kCheckedOutLocal)
	{
		d->plugin->GetConnection().Log().Debug() << "Add/Update in snapshot " << key << Endl;
		d->plugin->m_SnapShotEntries.insert(key, AESEntry(d->revisionID, entry->GetHash(), UpdateStateForMeta(key, kSynced), entry->GetSize(), entry->IsDir(), entry->GetTimeStamp()));
		AESEntry* localEntry = d->plugin->m_LocalChangesEntries.search(key);
		if (localEntry != NULL)
		{
			localEntry->SetState(UpdateStateForMeta(key, kSynced));
		}
	}
	else if ((entry->GetState() & kDeletedLocal) == kDeletedLocal)
	{
		d->plugin->GetConnection().Log().Debug() << "Delete in snapshot " << key << Endl;
		d->plugin->m_SnapShotEntries.erase(key);
		d->plugin->m_LocalChangesEntries.erase(key);
	}
	return 0;
}

bool AESPlugin::SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList, bool saveOnly)
{
    GetConnection().Log().Debug() << "SubmitAssets" << Endl;
    
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	if (!saveOnly && m_RemoteChangesEntries.size() > 0)
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Incoming changes still pending. Pull changes before submitting again."));
		GetConnection().WarnLine(IntToString(m_RemoteChangesEntries.size()) + " incoming change(s) not applied.", MAGeneral);
		//assetList.clear();
        return true;
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
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry == NULL)
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, string("Asset not found in local changes ") + path));
			return true;
		}
		
		if (entry->HasState(kAddedLocal) || entry->HasState(kCheckedOutLocal))
		{
			GetConnection().Log().Debug() << "Add/Update " << path << Endl;
			entriesToAddOrMod.insert(path, AESEntry(kLocalRevison, entry->GetHash(), entry->GetState(), entry->GetSize(), false, entry->GetTimeStamp()));
		}
		else if (entry->HasState(kDeletedLocal))
		{
			GetConnection().Log().Debug() << "Remove " << path << Endl;
			entriesToDelete.insert(path, AESEntry(kLocalRevison, entry->GetHash(), entry->GetState(), entry->GetSize(), false, entry->GetTimeStamp()));
		}
    }
	
	int succeededEntries;
	if (!m_AES->ApplyChanges(GetProjectPath(), entriesToAddOrMod, entriesToDelete, changeList.GetDescription(), &succeededEntries))
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
		//assetList.clear();
        return true;
	}
	else
	{
		if (succeededEntries != (entriesToAddOrMod.size() + entriesToDelete.size()))
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, "AES ApplyChanges failed, not all files were submitted"));
			//assetList.clear();
			return true;
		}
		GetConnection().Log().Debug() << "AES ApplyChanges success" << Endl;
	}

	AESRevision latestRevision;
	if (!m_AES->GetLatestRevision(latestRevision))
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
		//assetList.clear();
		return true;
	}

	ApplySubmitChangesCallBackData data;
	data.plugin = this;
	data.revisionID = latestRevision.GetRevisionID();

	int res = entriesToAddOrMod.iterate(ApplySubmitChangesCallBack, (void*)&data);
	if (res != 0)
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Apply submit changes iterate callback failed."));
		return true;
	}

	res = entriesToDelete.iterate(ApplySubmitChangesCallBack, (void*)&data);
	if (res != 0)
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Apply submit changes iterate callback failed."));
		return true;
	}

	if (!RefreshLocal())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh local"));
		return true;
	}

	if (!saveOnly)
	{
		m_SnapShotRevision = latestRevision.GetRevisionID();
	}

    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode)
{
    GetConnection().Log().Debug() << "SetAssetsFileMode" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

bool AESPlugin::GetAssetsStatus(VersionedAssetList& assetList, bool recursive)
{
    GetConnection().Log().Debug() << "GetAssetsStatus (" << (recursive ? "" : "Non-") << "Recursive)" << Endl;
    
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

    VersionedAssetList::iterator i = assetList.begin();
    while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
		
		if (asset.IsFolder())
		{
			if (recursive)
			{
				ResetTimer();
				GetConnection().Progress(-1, 0, "secs. while scanning local changes...");
				ScanDirectory(asset.GetPath(), false, ScanLocalChangeCallBack, (void*)this);
			}

            // Skip folders
            i = assetList.erase(i);
            continue;
		}

        int state = kNone;
		string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* localEntry = m_LocalChangesEntries.search(path);
		AESEntry* remoteEntry = m_RemoteChangesEntries.search(path);

		if (asset.IsFolder())
		{
			// Look for .meta file instead.
			path = path.substr(0, path.size()-1);
			path.append(kMetaExtension);

			GetConnection().Log().Debug() << "GetAssetsStatus folder, look for .meta file instead " << path << Endl;
			localEntry = m_LocalChangesEntries.search(path);
			remoteEntry = m_RemoteChangesEntries.search(path);
		}

		if (localEntry == NULL)
		{
			state = kLocal;
			if (remoteEntry != NULL)
			{
				state = remoteEntry->GetState() | kOutOfSync;
			}
		}
		else
		{
			state = localEntry->GetState();
			if (remoteEntry != NULL)
			{
				state |= remoteEntry->GetState();
				if (localEntry->GetHash() != remoteEntry->GetHash())
				{
					state |= kConflicted;
				}
				else
				{
					state |= kSynced;
				}
			}
		}
		
		state = UpdateStateForMeta(path, state);
        asset.SetState(state);
        
        i++;
    }
    
    return true;
}

bool AESPlugin::GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssetsChangeStatus (" << revision << ")" << Endl;
    
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	if (!RefreshLocal())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, "Cannot refresh local"));
		return true;
	}
    
    assetList.clear();
    if (revision == kNewListRevision)
    {
		EntriesToAssets(m_LocalChangesEntries, assetList);
    }

	bool res = GetAssetsStatus(assetList);
	if (res)
	{
		// Skip synced assets
		VersionedAssetList::iterator i = assetList.begin();
		while (i != assetList.end())
		{
			VersionedAsset& asset = (*i);

			if (asset.IsFolder() || asset.HasState(kSynced))
			{
				i = assetList.erase(i);
				continue;
			}

			i++;
		}

	}
	return res;
}

bool AESPlugin::GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetIncomingAssetsChangeStatus" << Endl;
    
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	string searchedRevision = revision;
	if (searchedRevision == kLatestRevison)
    {
		searchedRevision = m_LatestRevision.GetRevisionID();
	}
    assetList.clear();
	ChangelistRevisions::const_iterator i = find(m_Revisions.begin(), m_Revisions.end(), searchedRevision);
	if (i == m_Revisions.end())
	{
		StatusAdd(VCSStatusItem(VCSSEV_Error, string("Cannot find revision ") + searchedRevision));
		return true;
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
			return true;
		}
	}
	else
	{
		GetConnection().Log().Debug() << "Compare revision " << searchedRevision << " with " << compareTo << Endl;
		if (m_AES->GetRevisionDelta(searchedRevision, compareTo, entries))
		{
			EntriesToAssets(entries, assetList);
		}
		else
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, m_AES->GetLastMessage()));
			return true;
		}
	}

    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::GetAssetsChanges(Changes& changes)
{
    GetConnection().Log().Debug() << "GetAssetsChanges" << Endl;
    
    if (!EnsureConnected())
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
    
    if (!EnsureConnected())
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

	if (!EnsureConnected())
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
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    return true;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    assetList.clear();
    return true;
}

typedef struct {
	AESPlugin* plugin;
	size_t size;
	size_t iter;
	TreeOfEntries* ignoredEntries;
} UpdateToRevisionCallBackData;

int AESPlugin::UpdateToRevisionCheckCallBack(void *data, const string& key, AESEntry *entry)
{
	UpdateToRevisionCallBackData* d = (UpdateToRevisionCallBackData*) data;
	if (!entry->IsDir())
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;

		// Find if ignored or not
		AESEntry* ignoredEntry = d->ignoredEntries->search(key);
		if (ignoredEntry != NULL)
		{
			d->iter = d->iter+1;
			return 0;
		}
	}

	// Search in local changes
	AESEntry* localEntry = d->plugin->m_LocalChangesEntries.search(key);
	if (localEntry != NULL)
	{
		if (localEntry->GetHash() != entry->GetHash() && !(localEntry->HasState(kMarkUseMine) || localEntry->HasState(kMarkUseTheirs)))
		{
			d->plugin->GetConnection().Log().Debug() << "Asset '" << key << "' no decision made for it."
				<< "LocalHash: " << localEntry->GetHash()
				<< ", RemoteHash: " << entry->GetHash()
				<< Endl;
			return -1;
		}
	}

	d->iter = d->iter+1;
	return 0;
}

int AESPlugin::UpdateToRevisionDownloadCallBack(void *data, const string& key, AESEntry *entry)
{
	UpdateToRevisionCallBackData* d = (UpdateToRevisionCallBackData*) data;
	if (!entry->IsDir())
	{
		string localPath = d->plugin->GetProjectPath() + "/" + key;

		// Find if ignored or not
		AESEntry* ignoredEntry = d->ignoredEntries->search(key);
		if (ignoredEntry != NULL)
		{
			d->iter = d->iter+1;
			return 0;
		}

		// Search in local changes
		AESEntry* localEntry = d->plugin->m_LocalChangesEntries.search(key);
		if (localEntry != NULL)
		{
			if (localEntry->HasState(kMarkUseMine))
			{
				d->plugin->GetConnection().Log().Debug() << "Asset '" << key << "' is marked as mine." << Endl;
				d->iter = d->iter+1;

				// Update Snapshot
				uint64_t size = 0;
				time_t ts = 0;
				GetAFileInfo(localPath, &size, NULL, &ts);
				
				d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
				d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), UpdateStateForMeta(key, kSynced), size, false, ts));

				return 0;
			}
			else  if (localEntry->HasState(kMarkUseTheirs))
			{
				d->plugin->GetConnection().Log().Debug() << "Asset '" << key << "' is marked as theirs." << Endl;
			}
			else
			{
				if (localEntry->GetHash() != entry->GetHash())
				{
					d->plugin->GetConnection().Log().Debug() << "Asset '" << key << "' no decision made for it."
						<< "LocalHash: " << localEntry->GetHash()
						<< ", RemoteHash: " << entry->GetHash()
					<< Endl;
					return -1;
				}
				else
				{
					d->plugin->GetConnection().Log().Debug() << "Asset '" << key << "' is in synced, skip it." << Endl;
					d->iter = d->iter+1;

					// Update Snapshot
					uint64_t size = 0;
					time_t ts = 0;
					GetAFileInfo(localPath, &size, NULL, &ts);

					d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
					d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), UpdateStateForMeta(key, kSynced), size, false, ts));

					return 0;
				}
			}
		}

		// Delete file
		::DeleteRecursive(localPath);

		// Download file
		string targetPath = d->plugin->GetProjectPath() + "/" + key;
		//int pct = (int)(d->iter*100/d->size);
		d->plugin->GetConnection().Progress(-1, d->plugin->GetTimerSoFar(), string("Downloading file ") + key);
		if (!d->plugin->m_AES->Download(entry, key, targetPath))
		{
			d->plugin->GetConnection().Log().Debug() << "Cannot download " << key << ", reason:" << d->plugin->m_AES->GetLastMessage() << Endl;
			return 1;
		}

		// Remove from remote changes
		d->plugin->m_RemoteChangesEntries.erase(key);

		// Update Snapshot
		uint64_t size = 0;
		time_t ts = 0;
		GetAFileInfo(localPath, &size, NULL, &ts);

		d->plugin->GetConnection().Log().Debug() << "Add file from remote change: " << localPath << " (Size: " << size << ", TS: " << ToTime(ts) << ")" << Endl;
		d->plugin->m_SnapShotEntries.insert(key, AESEntry(entry->GetRevisionID(), entry->GetHash(), UpdateStateForMeta(key, kSynced), size, false, ts));
	}

	d->iter = d->iter+1;
	return 0;
}

bool AESPlugin::UpdateToRevision(const ChangelistRevision& revision, const VersionedAssetList& ignoredAssetList, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UpdateToRevision " << Endl;

    if (!EnsureConnected())
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
			AESEntry* entry = m_LocalChangesEntries.search(path);
			if (entry == NULL)
			{
				entry = m_SnapShotEntries.search(path);
				if (entry == NULL)
				{
					StatusAdd(VCSStatusItem(VCSSEV_Error, string("Asset not found in local changes or snapshot ") + path));
					return true;
				}
			}
			ignoredEntries.insert(path, AESEntry(entry->GetRevisionID(), entry->GetHash(), entry->GetState(), entry->GetSize(), entry->IsDir(), entry->GetTimeStamp()));
		}

		TreeOfEntries entries;
		ChangelistRevision newRevision = (revision == kLatestRevison) ? lastRevision : revision;
		if (m_AES->GetRevision(newRevision, entries))
		{
			if (entries.size() > 0)
			{
				UpdateToRevisionCallBackData data;
				data.plugin = this;
				data.size = entries.size();
				data.iter = 0;
				data.ignoredEntries = &ignoredEntries;

				int res = entries.iterate(UpdateToRevisionCheckCallBack, (void*)&data);
				if (res != 0)
				{
					StatusAdd(VCSStatusItem(VCSSEV_Error, "Unable to update to revision, still conflicts."));
					return true;
				}

				data.iter = 0;
				res = entries.iterate(UpdateToRevisionDownloadCallBack, (void*)&data);
				if (res != 0)
				{
					StatusAdd(VCSStatusItem(VCSSEV_Error, "Unable to update to revision, download failed."));
					return true;
				}
			}

			m_SnapShotRevision = newRevision;

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

bool AESPlugin::MarkAssets(VersionedAssetList& assetList, MarkMethod method)
{
    GetConnection().Log().Debug() << "MarkAssets" << Endl;
    if (!EnsureConnected())
    {
        //assetList.clear();
        return false;
    }

	VersionedAssetList::iterator i = assetList.begin();
	while (i != assetList.end())
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder())
        {
            // Skip folders
            i = assetList.erase(i);
            continue;
        }

        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		AESEntry* entry = m_LocalChangesEntries.search(path);
		if (entry == NULL)
		{
			StatusAdd(VCSStatusItem(VCSSEV_Error, string("Asset not found in local changes ") + path));
			return true;
		}

		switch (method)
		{
			case VersionControlPlugin::kUseMine:
				{
					GetConnection().Log().Debug() << "MarkAssets mine for " << path << Endl;
					entry->AddState(kMarkUseMine);
					asset.AddState(kMarkUseMine);
				}
				break;

			case VersionControlPlugin::kUseTheirs:
				{
					GetConnection().Log().Debug() << "MarkAssets theirs for " << path << Endl;
					entry->AddState(kMarkUseTheirs);
					asset.AddState(kMarkUseTheirs);
				}
				break;
		}

		i++;
	}

	return true;
}

bool AESPlugin::GetCurrentRevision(string& revisionID)
{
    GetConnection().Log().Debug() << "GetCurrentRevision" << Endl;

    if (!EnsureConnected())
    {
        return false;
    }

	revisionID = m_SnapShotRevision;
	if (revisionID == kLocalRevison)
	{
		revisionID = kDefaultListRevision;
		revisionID.append(" local by ");
		revisionID.append(m_Fields[kAESUserName].GetValue());
		revisionID.append(" on ");
		revisionID.append("0");

		return true;
	}

	AESRevision revision;
	if (!m_AES->GetRevision(m_SnapShotRevision, revision))
	{
		GetConnection().Log().Debug() << "Cannot get revision from AES, reason: " << m_AES->GetLastMessage() << Endl;
		return false;
	}

	revisionID = revision.GetRevisionID();
	revisionID.append(" ");
	revisionID.append(revision.GetComment());
	revisionID.append(" by ");
	revisionID.append(revision.GetComitterEmail());
	revisionID.append(" on ");
	revisionID.append(ToTimeLong(revision.GetTimeStamp()));

	return true;
}

bool AESPlugin::GetLatestRevision(string& revisionID)
{
    GetConnection().Log().Debug() << "GetLatestRevision" << Endl;

    if (!EnsureConnected())
    {
        return false;
    }

	AESRevision latestRevision;
	if (!m_AES->GetLatestRevision(latestRevision))
	{
		GetConnection().Log().Debug() << "Cannot get latest revision from AES, reason: " << m_AES->GetLastMessage() << Endl;
		return false;
	}

	revisionID = latestRevision.GetRevisionID();
	revisionID.append(" ");
	revisionID.append(latestRevision.GetComment());
	revisionID.append(" by ");
	revisionID.append(latestRevision.GetComitterEmail());
	revisionID.append(" on ");
	revisionID.append(ToTimeLong(latestRevision.GetTimeStamp()));

	return true;
}

void AESPlugin::GetCurrentVersion(string& version)
{
	version = kAESPluginVersion;
}

bool AESPlugin::PerformCustomCommand(const string& command, const CommandArgs& args)
{
    GetConnection().Log().Debug() << "PerformCustomCommand with parameters: ";
	StatusAdd(VCSStatusItem(VCSSEV_Error, "### NOT SUPPORTED ###"));
    return true;
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
		int state = entry->GetState();
		d->list->push_back(VersionedAsset(localPath, state));
	}
	return 0;
}

void AESPlugin::EntriesToAssets(TreeOfEntries& entries, VersionedAssetList& assetList, int ignoredState)
{
	EntryToAssetCallBackData data;
	data.plugin = this;
	data.list = &assetList;
	data.ignoredState = ignoredState;
	int res = entries.iterate(EntryToAssetCallBack, (void*)&data);
	if (res != 0)
	{
		GetConnection().Log().Debug() << "EntryToAsset callback" << Endl;
	}
}