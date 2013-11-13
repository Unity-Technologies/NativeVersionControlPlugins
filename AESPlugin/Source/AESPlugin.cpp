#include "AESPlugin.h"
#include "FileSystem.h"
#include "JSON.h"

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

int CompareHash(const string& file, const AESEntry& entry)
{
    size_t size = GetFileLength(file);
    if ((int)size != entry.GetSize())
        return -1;
    
    return 0;
}

void FillMapOfEntries(const AESEntries& entries, MapOfEntries& map)
{
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end(); i++)
    {
        AESEntry entry = (*i);
        map[entry.GetPath()] = entry;
        if (entry.IsDir())
        {
            FillMapOfEntries(entry.GetChildren(), map);
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
    m_Fields.push_back(VersionControlPluginCfgField(GetPluginName(), "Repository", "Repository", "The AES Repository", "TestGitRepo1", VersionControlPluginCfgField::kRequiredField));
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
    
    m_CurrRevision = "current";
	m_CurrEntries.clear();
}

bool AESPlugin::Refresh()
{
	GetConnection().Log().Debug() << "Refresh" << Endl;
	
	if (!m_IsConnected)
		return false;
	
	// Fetch current enries from AES
	AESEntries entries;
    if (!m_AES->GetRevision(m_CurrRevision, entries))
    {
        GetConnection().Log().Debug() << "Cannot get " << m_CurrRevision << " revision, error: " << m_AES->GetLastMessage() << Endl;
        return false;
    }
	
	m_CurrEntries.clear();
	FillMapOfEntries(entries, m_CurrEntries);
	GetConnection().Log().Debug() << "Found in " << m_CurrRevision << ": " << Endl;
	/*
	for (MapOfEntries::const_iterator j = m_CurrEntries.begin() ; j != m_CurrEntries.end() ; j++)
	{
		GetConnection().Log().Debug() << "- " << j->first << Endl;
	}
	*/
	return true;
}

bool AESPlugin::SaveStateToCacheFile()
{
	string aesCache = GetProjectPath() + "/Library/aesCache.txt";
	GetConnection().Log().Debug() << "SaveStateToCacheFile " << aesCache << Endl;
	
	JSONArray arr;
    for (MapOfEntries::const_iterator i = m_CurrEntries.begin() ; i != m_CurrEntries.end(); i++)
    {
		JSONObject elt;
		const AESEntry& entry = i->second;
		elt["name"] = new JSONValue(entry.GetName());
		elt["path"] = new JSONValue(entry.GetPath());
		elt["ref"] = new JSONValue(entry.GetReference());
		elt["hash"] = new JSONValue(entry.GetHash());
		elt["size"] = new JSONValue((double)entry.GetSize());
		
		arr.push_back(new JSONValue(elt));
	}
	JSONObject obj;
	obj["entries"] = new JSONValue(arr);
	
	JSONValue json(obj);
	GetConnection().Log().Debug() << "SaveState JSON: " << json.Stringify() << Endl;
	return WriteAFile(aesCache, json.Stringify());
}

bool AESPlugin::RestoreStateFromCacheFile()
{
	string aesCache = GetProjectPath() + "/Library/aesCache.txt";
	GetConnection().Log().Debug() << "RestoreStateFromCacheFile " << aesCache << Endl;
	
	string buffer;
	if (!ReadAFile(aesCache, buffer))
	{
		return false;
	}
	
	bool res = false;
    JSONValue* json = JSON::Parse(buffer.c_str());
	if (json != NULL && json->IsObject())
	{
		const JSONObject& obj = json->AsObject();
		
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
	
	if (!SaveStateToCacheFile())
    {
        GetConnection().Log().Debug() << "Cannot save AES cache" << Endl;
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
        GetConnection().Log().Debug() << "Login failed " << m_AES->GetLastError() << Endl;
        return -1;
    }
    
	Refresh();
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
		asset.AddState(kAddedLocal);
		
		GetConnection().Log().Debug() << "Outgoing ADD " << path << Endl;
        m_Outgoing.insert(make_pair(path, asset));
    }
    
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
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder() || asset.HasState(kDeletedLocal) || asset.HasState(kSynced))
        {
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		asset.AddState(kDeletedLocal);
		
		GetConnection().Log().Debug() << "Outgoing REMOVE" << path << Endl;
        m_Outgoing.insert(make_pair(path, asset));
    }
    
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList)
{
    GetConnection().Log().Debug() << "MoveAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::CheckoutAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "CheckoutAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder() || asset.HasState(kCheckedOutLocal))
        {
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		asset.AddState(kCheckedOutLocal);
		
		GetConnection().Log().Debug() << "Outgoing CHECKOUT " << path << Endl;
        m_Outgoing.insert(make_pair(path, asset));
    }
    
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::RevertAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertAssets" << Endl;
    if (!CheckConnectedAndLogged())
    {
        assetList.clear();
        return true;
    }
    
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (asset.IsFolder() || !asset.HasState(kCheckedOutLocal))
        {
            continue;
        }
		
        string path = asset.GetPath().substr(GetProjectPath().size()+1);
		VersionedAssetMap::const_iterator j = m_Outgoing.find(path);
		if (j != m_Outgoing.end())
		{
			GetConnection().Log().Debug() << "Outgoing UNDO-CHECKOUT " << path << Endl;
			m_Outgoing.erase(j);
		}
    }
    
	return GetAssetsStatus(assetList, false);
}

bool AESPlugin::DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "DownloadAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::GetAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "GetAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::ResolveAssets(VersionedAssetList& assetList, ResolveMethod method)
{
    GetConnection().Log().Debug() << "ResolveAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::LockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "LockAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::UnlockAssets(VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "UnlockAssets" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}

bool AESPlugin::SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "SetRevision" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
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
    
	AESEntries entriesToAddOrMod;
	AESEntries entriesToDelete;
    for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
        if (!asset.IsFolder())
		{
			string path = asset.GetPath().substr(GetProjectPath().size()+1);
			string remotePath = BuildRemotePath(asset);
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
	
	for (VersionedAssetList::iterator i = assetList.begin() ; i != assetList.end() ; i++)
    {
        VersionedAsset& asset = (*i);
		if (!asset.IsFolder())
		{
			string path = asset.GetPath().substr(GetProjectPath().size()+1);
			VersionedAssetMap::const_iterator j = m_Outgoing.find(path);
			if (j != m_Outgoing.end())
			{
				GetConnection().Log().Debug() << "Remove " << path << " from outgoing change" << Endl;
				m_Outgoing.erase(j);
			}
		}
	}
	
    return GetAssetsStatus(assetList, false);
}

bool AESPlugin::SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode)
{
    GetConnection().Log().Debug() << "SetAssetsFileMode" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
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
    
	if (!Refresh())
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
        
		MapOfEntries::const_iterator j = m_CurrEntries.find(path);
        if (j != m_CurrEntries.end())
        {
			const AESEntry& entry = j->second;
            GetConnection().Log().Debug() << "Found AES entry " << entry.GetReference() << " (" << entry.GetSize() << ")" << Endl;
            state |= kSynced;
        }

		VersionedAssetMap::const_iterator k = m_Outgoing.find(path);
		if (k != m_Outgoing.end())
		{
			const VersionedAsset& managedAsset = k->second;
            GetConnection().Log().Debug() << "Found Outgoing entry " << managedAsset.GetPath() << Endl;
			state |= managedAsset.GetState();
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
    if (revision == kDefaultListRevision)
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

void EntriesToAssets(const AESEntries& entries, VersionedAssetList& assetList, bool setState = false)
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
			int state = kSynced;
			if (setState)
			{
				int size = entry.GetSize();
				if (size > 0) state = kAddedLocal;
				else if (size < 0) state = kDeletedRemote;
			}
			
            assetList.push_back(VersionedAsset(entry.GetPath(), state));
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
		if (m_AES->GetRevision(revision, entries))
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
    defaultItem.SetDescription(m_CurrRevision);
    defaultItem.SetRevision(kDefaultListRevision);
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
        for (vector<AESRevision>::const_iterator i = revisions.begin() ; i != revisions.end() ; i++)
        {
            const AESRevision& rev = (*i);
			GetConnection().Log().Debug() << "GetAssetsIncomingChanges found revision" << rev.GetRevisionID() << Endl;
            Changelist item;
            item.SetCommitter(rev.GetComitterName());
            item.SetDescription(rev.GetComment());
            item.SetRevision(rev.GetRevisionID());
            //item.SetTimestamp(ToTime(rev.GetTimeStamp()));
            changes.push_back(item);
			m_Revisions.push_back(rev.GetRevisionID());
        }
		//std::reverse(changes.begin(), changes.end());
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
    return true;
}

bool AESPlugin::RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList)
{
    GetConnection().Log().Debug() << "RevertChanges" << Endl;
	GetConnection().Log().Debug() << "### NOT IMPLEMENTED ###" << Endl;
    return true;
}