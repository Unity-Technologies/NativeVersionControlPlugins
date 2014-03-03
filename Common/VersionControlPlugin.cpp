#include "VersionControlPlugin.h"
#include "CommandLine.h"
#include "Command.h"
#include <algorithm>
#include <iterator>

using namespace std;

static const char* gVersionControlPluginCommands[] =
{
    "add",
    "changeDescription",
    "changeMove",
    "changes",
    "changeStatus",
    "checkout",
    "deleteChanges",
    "delete",
    "download",
    "getLatest",
    "incomingChangeAssets",
    "incoming",
    "lock",
    "move",
    "resolve",
    "revertChanges",
    "revert",
    "status",
    "submit",
    "unlock",
    0
};

static const char* gVersionControlPluginTraits[] =
{
    "requiresNetwork",
    "enablesCheckout",
    "enablesLocking",
    "enablesRevertUnchanged",
    "enablesVersioningFolders",
    "enablesChangelists",
    "enablesGetLatestOnChangeSetSubset",
    "enablesConflictHandlingByPlugin",
    0
};

static const string Empty = "";

#ifndef NDEBUG
static const char* kRecordExtFileName = "Record.txt";
#endif

Connection& operator<<(Connection& p, const VersionControlPluginCfgField& field)
{
    p.DataLine("vc" + field.GetPrefix() + field.GetName());
    p.DataLine(field.GetLabel(), MAConfig);
    p.DataLine(field.GetDescription(), MAConfig);
    p.DataLine(field.GetDefaultValue());
    p.DataLine(field.GetFlags());
    return p;
}

VersionControlPlugin::VersionControlPluginOverlays VersionControlPlugin::s_emptyOverlays;

VersionControlPlugin::VersionControlPlugin(const std::string& name, const char* args) : m_PluginName(name),  m_ProjectPath(""), m_IsOnline(false), m_IsDirty(false), m_Connection(NULL)
{
    int argc = 0;
    char** argv = CommandLineToArgv(args, &argc);
    InitializeArguments(argc, argv);
    CommandLineFreeArgs(argv);
}

VersionControlPlugin::VersionControlPlugin(const std::string& name, int argc, char** argv) : m_PluginName(name), m_ProjectPath(""), m_IsOnline(false), m_IsDirty(false), m_Connection(NULL)
{
    InitializeArguments(argc, argv);
}

inline const string VersionControlPlugin::GetArg(const std::string& name)
{
	VersionControlPluginMapOfArguments::const_iterator i = m_Arguments.find(name);
	if (i == m_Arguments.end())
		return "";

	return i->second;
}

inline bool VersionControlPlugin::HasArg(const std::string& name)
{
	return (m_Arguments.find(name) != m_Arguments.end());
}

void VersionControlPlugin::InitializeArguments(int argc, char** argv)
{
    vector<string> pathComp;
	if (argc > 0) 
	{
#ifdef WIN32
		Tokenize(pathComp, string(argv[0]), "\\");
#else
		Tokenize(pathComp, string(argv[0]), "/");
#endif
	    string tmp = (*pathComp.rbegin());
			tmp = tmp.substr(0, tmp.find("Plugin"));
		if (!tmp.empty())
		{
			m_PluginName = tmp;
		}
	}
    
    for (int i = 0 ; i < argc ; )
    {
        if ((argv[i][0] == '-') && (i+1 < argc) && (argv[i+1][0] != '-')) {
            m_Arguments[argv[i]] = argv[i+1];
            i += 2;
            continue;
        }
        
        m_Arguments[argv[i++]] = "";
    }
}

const VCSStatus& VersionControlPlugin::GetStatus() const
{
	return m_Status;
}

void VersionControlPlugin::ClearStatus()
{
    m_Status.clear();
}

VCSStatus& VersionControlPlugin::StatusAdd(VCSStatusItem item)
{
	if (item.severity == VCSSEV_Error)
		GetConnection().Log().Fatal() << item.message << Endl;
	else if (item.severity == VCSSEV_Warn)
		GetConnection().Log().Notice() << item.message << Endl;
	else if (item.severity == VCSSEV_Info)
		GetConnection().Log().Info() << item.message << Endl;
	else
		GetConnection().Log().Debug() << item.message << Endl;

    m_Status.insert(item);
    return m_Status;
}

void VersionControlPlugin::SetProjectPath(const string& path)
{
	if (path != m_ProjectPath)
		SetDirty();

	m_ProjectPath = path;
}

int VersionControlPlugin::Run()
{
#ifndef NDEBUG
    m_Connection = new Connection(HasArg("-l") ? GetArg("-l") : GetLogFileName(), "./Temp/" + GetPluginName() + kRecordExtFileName);
#else
    m_Connection = new Connection(HasArg("-l") ? GetArg("-l") : GetLogFileName());
#endif

    if (HasArg("-v"))
    {
        GetConnection().Log().SetLogLevel(LOG_DEBUG);
    }
	
    if (HasArg("-p"))
    {
        SetProjectPath(GetArg("-p"));
    }
	
    if (HasArg("-t"))
    {
		return Test();
    }
    
    try
	{
		UnityCommand cmd;
		vector<string> args;
		
        if (HasArg("-c"))
        {
            if (Tokenize(args, GetArg("-c")) == 0)
            {
                return 1; // error
            }
            
            cmd = StringToUnityCommand(args[0].c_str());
            if (!Dispatch(cmd, args))
				return 1; // error
            
			Disconnect();
            return 0; // ok
        }
        
		for ( ;; )
		{
            GetConnection().Log().Debug() << "ReadCommand" << Endl;
#ifndef NDEBUG
            GetConnection().GetRecordStream() << "### ReadCommand ###" << endl;
#endif

			cmd = GetConnection().ReadCommand(args);
            
			if (cmd == UCOM_Invalid)
            {
                GetConnection().Log().Debug() << "Invalid command" << Endl;
#ifndef NDEBUG
				GetConnection().GetRecordStream() << "### Invalid command ###" << endl;
#endif
				return 1; // error
			}
            
            if (cmd == UCOM_Shutdown)
			{
                GetConnection().Log().Debug() << "Shutdown" << Endl;
#ifndef NDEBUG
				GetConnection().GetRecordStream() << "### Shutdown ###" << endl;
#endif
				Disconnect();
				GetConnection().EndResponse();
#ifdef NDEBUG
				return 0; // ok
#else
				continue;
#endif
			}
			
            if (!Dispatch(cmd, args))
				return 0; // error
		}
	}
	catch (exception& e)
	{
		GetConnection().Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
#ifndef NDEBUG
		GetConnection().GetRecordStream() << "### Unhandled exception: " << e.what() << " ###" << endl;
#endif
	}
    
	return 1; // error
}

bool VersionControlPlugin::PreHandleCommand()
{
    GetConnection().Log().Debug() << "PreHandleCommand" << Endl;
    
    ClearStatus();
    return IsOnline();
}

void VersionControlPlugin::PostHandleCommand(bool wasOnline)
{
    GetConnection().Log().Debug() << "PostHandleCommand " << (wasOnline?"WASONLINE":"WASOFFLINE") << Endl;
    
	GetConnection() << GetStatus();

    if (!wasOnline && IsOnline())
    {
        SetOffline();
        NotifyOnline();
    }
    else if (wasOnline && !IsOnline())
    {
        SetOnline();
        NotifyOffline(GetStatus().size() == 0 ? "Offline" : GetStatus().begin()->message);
    }
    
    GetConnection().EndResponse();
}

bool VersionControlPlugin::HandleAdd()
{
    GetConnection().Log().Trace() << "HandleAdd" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (AddAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    GetConnection().EndList();
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleChangeDescription()
{
    GetConnection().Log().Trace() << "HandleChangeDescription" << Endl;

    bool wasOnline = PreHandleCommand();
    
    ChangelistRevision revision;
    GetConnection() >> revision;
    
    string description("");
    if (UpdateRevision(revision, description))
    {
        SetOnline();
        GetConnection().DataLine(description);
    }
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleChangeMove()
{
    GetConnection().Log().Trace() << "HandleChangeMove" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    ChangelistRevision revision;
    GetConnection() >> revision;

    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (SetRevision(revision, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleChanges()
{
    GetConnection().Log().Trace() << "HandleChanges" << Endl;

    bool wasOnline = PreHandleCommand();
    
    Changes changes;
    if (GetAssetsChanges(changes))
        SetOnline();
    else
        changes.clear();
    
    GetConnection() << changes;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleChangeStatus()
{
    GetConnection().Log().Trace() << "HandleChangeStatus" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    ChangelistRevision cl;
    GetConnection() >> cl;
    
    VersionedAssetList assetList;
    if (GetAssetsChangeStatus(cl, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleCheckout()
{
    GetConnection().Log().Trace() << "HandleCheckout " << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (CheckoutAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleConfig(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleConfig" << Endl;
    
    if (args.size() < 2)
    {
        string msg = "Plugin got invalid config setting :";
        for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i)
		{
            msg += " ";
            msg += *i;
        }
        GetConnection().WarnLine(msg, MAConfig);
        GetConnection().EndResponse();
        return true;
    }
    
    string key = args[1];
    string value = Join(args.begin() + 2, args.end(), " ");
    
    GetConnection().Log().Info() << "Got config " << key << " = '" << value << "'" << Endl;
    
    if (key == "projectPath")
    {
        SetProjectPath(TrimEnd(value));
        GetConnection().Log().Info() << "Set " << key << " to " << value << Endl;
        GetConnection().EndResponse();
        return true;
    }
    
    if (key == "vcSharedLogLevel")
    {
        GetConnection().Log().Info() << "Set log level to " << value << Endl;
        LogLevel level = LOG_DEBUG;
        if (value == "verbose")
            level = LOG_DEBUG;
        else if (value == "info")
            level = LOG_INFO;
        else if (value == "notice")
            level = LOG_NOTICE;
        else if (value == "fatal")
            level = LOG_FATAL;

		if (GetConnection().Log().GetLogLevel() != level)
			SetDirty();

        GetConnection().Log().SetLogLevel(level);
        GetConnection().EndResponse();
        return true;
    }
    
    if (key == "pluginVersions")
    {
        VersionControlPluginVersions unitySupportedVersions;
        CommandArgs::const_iterator i = args.begin();
        i += 2;
        for	( ; i != args.end(); ++i)
        {
            unitySupportedVersions.insert(atoi(i->c_str()));
        }
        
        int sel = SelectVersion(unitySupportedVersions);
        GetConnection().DataLine(sel, MAConfig);
        GetConnection().Log().Info() << "Selected plugin protocol version " << sel << Endl;
        GetConnection().EndResponse();
        return true;
    }
    
    if (key == "pluginTraits")
    {
        HandleConfigTraits();
        GetConnection().EndResponse();
        return true;
    }
    
    if (key == "end")
    {
		PreHandleCommand();
		if (IsDirty())
		{
			Disconnect();
		}

		if (EnsureConnected())
		{
			SetOnline();
			PostHandleCommand(false);
		}
		else
		{
			SetOffline();
			PostHandleCommand(true);
		}
        return true;
    }
    
    if (HandleSetConfigParameters(args))
    {
        GetConnection().EndResponse();
        return true;
    }
    
    GetConnection().Log().Notice() << "Unknown config field set on version control plugin: " << key << Endl;
    GetConnection().WarnLine(ToString("Unknown config field set on version control plugin: ", key), MAConfig);
    GetConnection().EndResponse();
    return false;
}

bool VersionControlPlugin::HandleCustomCommand(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleCustomCommand" << Endl;
    
    bool m_WasOnline = PreHandleCommand();
    
	if (args.size() > 1)
	{
		string customCmd = args[1];
		if (!PerformCustomCommand(customCmd, args))
		{
			GetConnection().Log().Notice() << "Unable to perform custom command " << customCmd << Endl;
		}
	}
	
    PostHandleCommand(m_WasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleDelete()
{
    GetConnection().Log().Trace() << "HandleDelete" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (RemoveAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleDeleteChanges()
{
    GetConnection().Log().Trace() << "HandleDeleteChanges" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    ChangelistRevisions changes;
    GetConnection() >> changes;
    
    if (changes.empty())
    {
        GetConnection().WarnLine("Changes to delete is empty");
        PostHandleCommand(wasOnline);
        return true;
    }
    
    for (ChangelistRevisions::const_iterator i = changes.begin() ; i != changes.end() ; i++)
    {
        if (DeleteRevision(*i))
        {
            SetOnline();
        }
    }
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleDownload()
{
    GetConnection().Log().Trace() << "HandleDownload" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    string targetDir;
    ChangelistRevisions changes;
    VersionedAssetList assetList;
    
    GetConnection() >> targetDir;
    GetConnection() >> changes;
    GetConnection() >> assetList;
    
    if (DownloadAssets(targetDir, changes, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleExit()
{
    GetConnection().Log().Trace() << "HandleExit" << Endl;
    
    bool wasOnline = PreHandleCommand();
    // TODO: no protocol documentation found
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleFileMode(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleFileMode" << Endl;
    
    FileMode mode = kUnused;
    if (args.size() == 3)
    {
        if (args[1] == "set" && args[2] == "binary")
        {
            mode = kBinary;
        }
        else if (args[1] == "set" && args[2] == "text")
        {
            mode = kText;
        }
        else
        {
            GetConnection().WarnLine(string("Unknown filemode ") + args[1] + " " + args[2]);
        }
    }

    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (SetAssetsFileMode(assetList, mode))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleGetlatest()
{
    GetConnection().Log().Trace() << "HandleGetlatest" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (GetAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleIncoming()
{
    GetConnection().Log().Trace() << "HandleIncoming" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    Changes changes;
    if (GetAssetsIncomingChanges(changes))
        SetOnline();
    else
        changes.clear();
    
    GetConnection() << changes;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleIncomingChangeAssets()
{
    GetConnection().Log().Trace() << "HandleIncomingChangeAssets" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    ChangelistRevision cl;
    GetConnection() >> cl;
    
    VersionedAssetList assetList;
    if (GetIncomingAssetsChangeStatus(cl, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleLock()
{
    GetConnection().Log().Trace() << "HandleLock" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (LockAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleLogin()
{
    GetConnection().Log().Trace() << "HandleLogin" << Endl;
    
    bool m_WasOnline = PreHandleCommand();
    if (Login())
        SetOnline();

    PostHandleCommand(m_WasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleMove(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleMove" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (MoveAssets(assetList, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleQueryConfigParameters()
{
    GetConnection().Log().Trace() << "HandleQueryConfigParameters" << Endl;
    
    VersionControlPluginCfgFields& fields = GetConfigFields();
    for (VersionControlPluginCfgFields::iterator i = fields.begin() ; i != fields.end() ; i++)
    {
        VersionControlPluginCfgField& field = (*i);
        GetConnection().DataLine(string("text ") + field.GetLabel());
    }
    GetConnection().DataLine("");
    
    return true;
}

bool VersionControlPlugin::HandleResolve(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleResolve" << Endl;
    
    ResolveMethod method = kMine;
    if (args.size() == 2)
    {
        if (args[1] == "mine")
        {
            method = kMine;
        }
        else if (args[1] == "theirs")
        {
            method = kTheirs;
        }
        else if (args[1] == "merged")
        {
            method = kMerged;
        }
    }

    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (ResolveAssets(assetList, method))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleRevert()
{
    GetConnection().Log().Trace() << "HandleRevert" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (RevertAssets(assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleRevertChanges()
{
    GetConnection().Log().Trace() << "HandleRevertChanges" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    ChangelistRevision cl;
    GetConnection() >> cl;
    
    VersionedAssetList assetList;
    if (RevertChanges(cl, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleSubmit(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleSubmit" << Endl;
	
	bool saveOnly = false;
    if (args.size() == 2 && args[1] == "saveOnly")
    {
        saveOnly = true;
    }
	
    bool wasOnline = PreHandleCommand();
    
    Changelist changelist;
    GetConnection() >> changelist;
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (SubmitAssets(changelist, assetList, saveOnly))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleStatus(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleStatus" << Endl;
    
    bool recursive = false;
    if (args.size() == 2 && args[1] == "recursive")
    {
        recursive = true;
    }
    
    bool wasOnline = PreHandleCommand();
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (GetAssetsStatus(assetList, recursive))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleUnlock()
{
    GetConnection().Log().Trace() << "HandleUnlock" << Endl;

    bool wasOnline = PreHandleCommand();

    VersionedAssetList assetList;
	GetConnection() >> assetList;

    if (UnlockAssets(assetList))
        SetOnline();
    else
        assetList.clear();

    GetConnection() << assetList;

    PostHandleCommand(wasOnline);

    return true;
}
bool VersionControlPlugin::HandleConfigTraits()
{
    vector<string> flagsList;
    TraitsFlags flags = GetSupportedTraitFlags();
	int i = 0;
	while (gVersionControlPluginTraits[i])
	{
        TraitsFlags v = (TraitsFlags)(1 << i);
        if ((flags & v) == v)
        {
            flagsList.push_back(gVersionControlPluginTraits[i]);
        }
		i++;
	}

    GetConnection().DataLine(flagsList.size());
    for (vector<string>::const_iterator i = flagsList.begin() ; i != flagsList.end() ; i++)
    {
        GetConnection().DataLine(*i, MAConfig);
    }
    
    const VersionControlPluginCfgFields& fields = GetConfigFields();
    GetConnection().DataLine(fields.size());
    for (VersionControlPluginCfgFields::const_iterator i = fields.begin() ; i != fields.end() ; i++)
    {
        const VersionControlPluginCfgField& field = (*i);
        GetConnection() << field;
    }
    
    const VersionControlPluginOverlays& overlays = GetOverlays();
    if (overlays.size() > 0)
    {
        GetConnection().DataLine("overlays");
        GetConnection().DataLine(overlays.size());
        for (VersionControlPluginOverlays::const_iterator i = overlays.begin() ; i != overlays.end() ; i++)
        {
            GetConnection().DataLine(i->first);
            GetConnection().DataLine(i->second);
        }
    }
    
    const VersionControlPluginCustomCommands& customCommands = GetCustomCommands();
    if (customCommands.size() > 0)
    {
        GetConnection().DataLine("customCommands");
        GetConnection().DataLine(customCommands.size());
        for (VersionControlPluginCustomCommands::const_iterator i = customCommands.begin() ; i != customCommands.end() ; i++)
        {
            GetConnection().DataLine(i->GetName());
            GetConnection().DataLine(i->GetLabel());
            GetConnection().DataLine(i->GetContext());
        }
    }

    return true;
}

bool VersionControlPlugin::HandleSetConfigParameters(const CommandArgs& args)
{
    string key = args[1];
    key = key.substr(string("vc" + GetPluginName()).length());
    
    string value = Join(args.begin() + 2, args.end(), " ");
    
    GetConnection().Log().Info() << "Set " << key << " to " << value << Endl;
    
    VersionControlPluginCfgFields& fields = GetConfigFields();
    for (VersionControlPluginCfgFields::iterator i = fields.begin() ; i != fields.end() ; i++)
    {
        VersionControlPluginCfgField& field = (*i);
        if (field.GetName() == key)
        {
			value = TrimEnd(value);
			if (field.GetValue() != value)
				SetDirty();

            field.SetValue(value);
            GetConnection().Log().Info() << key << " set to " << value << Endl;
            return true;
        }
    }
    
    GetConnection().Log().Info() << "Unknown config " << key << ", skip it" << Endl;
    return true;
}

bool VersionControlPlugin::HandleListRevision()
{
    GetConnection().Log().Trace() << "HandleListRevision" << Endl;

    bool wasOnline = PreHandleCommand();

	ChangelistRevision revision;
	GetConnection().ReadLine(revision);

    VersionedAssetList assetList;
    if (ListRevision(revision, assetList))
        SetOnline();
    else
        assetList.clear();

    GetConnection() << assetList;

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::HandleUpdateToRevision()
{
    GetConnection().Log().Trace() << "HandleUpdateToRevision" << Endl;

    bool wasOnline = PreHandleCommand();

	ChangelistRevision revision;
	GetConnection().ReadLine(revision);

    VersionedAssetList ignoredAssetList;
	GetConnection() >> ignoredAssetList;

    VersionedAssetList assetList;

    if (UpdateToRevision(revision, ignoredAssetList, assetList))
        SetOnline();
    else
        assetList.clear();

    GetConnection() << assetList;

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::HandleApplyRevisionChanges()
{
    GetConnection().Log().Trace() << "HandleApplyRevisionChanges" << Endl;

    bool wasOnline = PreHandleCommand();

	ChangelistRevision revision;
	GetConnection().ReadLine(revision);

    VersionedAssetList assetList;
	GetConnection() >> assetList;

    if (ApplyRevisionChanges(revision, assetList))
        SetOnline();
    else
        assetList.clear();

    GetConnection() << assetList;

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::HandleCurrentRevision()
{
    GetConnection().Log().Trace() << "HandleCurrentRevision" << Endl;

    bool wasOnline = PreHandleCommand();

	string currentRevision;
    if (GetCurrentRevision(currentRevision))
	{
		GetConnection().DataLine(string("current:") + currentRevision, MARemote);
        SetOnline();
	}

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::HandleLatestRevision()
{
    GetConnection().Log().Trace() << "HandleLatestRevision" << Endl;

    bool wasOnline = PreHandleCommand();

	string latestRevision;
    if (GetLatestRevision(latestRevision))
	{
		GetConnection().DataLine(string("latest:") + latestRevision, MARemote);
        SetOnline();
	}

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::HandleCurrentVersion()
{
    GetConnection().Log().Trace() << "HandleCurrentVersion" << Endl;

    bool wasOnline = PreHandleCommand();

	string currentVersion;
    GetCurrentVersion(currentVersion);

	GetConnection().DataLine(string("version:") + currentVersion, MARemote);

	SetOnline();
    PostHandleCommand(wasOnline);

	return true;
}

bool VersionControlPlugin::HandleMark(const CommandArgs& args)
{
    GetConnection().Log().Trace() << "HandleMark" << Endl;

    MarkMethod method = kUseMine;
    if (args.size() == 2)
    {
        if (args[1] == "mine")
        {
            method = kUseMine;
        }
        else if (args[1] == "theirs")
        {
            method = kUseTheirs;
        }
    }

    bool wasOnline = PreHandleCommand();

    VersionedAssetList assetList;
	GetConnection() >> assetList;

    if (MarkAssets(assetList, method))
        SetOnline();
    else
        assetList.clear();

    GetConnection() << assetList;

    PostHandleCommand(wasOnline);

    return true;
}

bool VersionControlPlugin::Dispatch(UnityCommand command, const CommandArgs& args)
{
    GetConnection().Log().Trace() << "Dispatch " << UnityCommandToString(command) << Endl;
#ifndef NDEBUG
    GetConnection().GetRecordStream() << "### DispatchCommand ";
    for (CommandArgs::const_iterator i = args.begin() ; i != args.end() ; i++)
    {
        GetConnection().GetRecordStream() << (*i) << " ";
    }
    GetConnection().GetRecordStream() << "###" << endl;
#endif
    switch (command) {
        case UCOM_Add:
            return HandleAdd();
            
        case UCOM_ChangeDescription:
            return HandleChangeDescription();
            
        case UCOM_ChangeMove:
            return HandleChangeMove();
            
        case UCOM_Changes:
            return HandleChanges();
            
        case UCOM_ChangeStatus:
            return HandleChangeStatus();
                        
        case UCOM_Checkout:
            return HandleCheckout();
            
        case UCOM_Config:
            return HandleConfig(args);
            
        case UCOM_CustomCommand:
            return HandleCustomCommand(args);
            
        case UCOM_Delete:
            return HandleDelete();
            
        case UCOM_DeleteChanges:
            return HandleDeleteChanges();
            
        case UCOM_Download:
            return HandleDownload();
            
        case UCOM_Exit:
            return HandleExit();
            
        case UCOM_FileMode:
            return HandleFileMode(args);
            
        case UCOM_GetLatest:
            return HandleGetlatest();
            
        case UCOM_Incoming:
            return HandleIncoming();
            
        case UCOM_IncomingChangeAssets:
            return HandleIncomingChangeAssets();
            
        case UCOM_Lock:
            return HandleLock();
            
        case UCOM_Login:
            return HandleLogin();
            
        case UCOM_Move:
            return HandleMove(args);
            
        case UCOM_QueryConfigParameters:
            return HandleQueryConfigParameters();
            
        case UCOM_Resolve:
            return HandleResolve(args);
            
        case UCOM_Revert:
            return HandleRevert();
            
        case UCOM_RevertChanges:
            return HandleRevertChanges();
            
        case UCOM_Status:
            return HandleStatus(args);
            
        case UCOM_Submit:
            return HandleSubmit(args);
            
        case UCOM_Unlock:
            return HandleUnlock();

        case UCOM_ListRevision:
            return HandleListRevision();

        case UCOM_UpdateToRevision:
            return HandleUpdateToRevision();

        case UCOM_ApplyRevision:
            return HandleApplyRevisionChanges();

        case UCOM_CurrentRevision:
            return HandleCurrentRevision();

        case UCOM_LatestRevision:
            return HandleLatestRevision();

        case UCOM_CurrentVersion:
            return HandleCurrentVersion();

        case UCOM_Mark:
            return HandleMark(args);

        default:
            GetConnection().Log().Debug() << "Command " << UnityCommandToString(command) << " not handled" << Endl;
            GetConnection().EndResponse();
            return false;
    }
}

void VersionControlPlugin::NotifyOffline(const string& reason)
{
	if (!m_IsOnline)
		return; // nothing to notify

	int i = 0;
    CommandsFlags flags = GetOfflineUICommands();
	while (gVersionControlPluginCommands[i])
	{
        CommandsFlags v = (CommandsFlags)(1 << i);
        if ((flags & v) == v)
            GetConnection().Command(string("enableCommand ") + gVersionControlPluginCommands[i], MAProtocol);
        else
            GetConnection().Command(string("disableCommand ") + gVersionControlPluginCommands[i], MAProtocol);
		++i;
	}
	
	GetConnection().Command(string("offline ") + reason, MAProtocol);
	
    m_IsOnline = false;
}

void VersionControlPlugin::NotifyOnline()
{
	if (m_IsOnline)
		return; // nothing to notify
    
	GetConnection().Command("online", MAProtocol);

	int i = 0;
    CommandsFlags flags = GetOnlineUICommands();
	while (gVersionControlPluginCommands[i])
	{
        CommandsFlags v = (CommandsFlags)(1 << i);
        if ((flags & v) == v)
            GetConnection().Command(string("enableCommand ") + gVersionControlPluginCommands[i], MAProtocol);
        else
            GetConnection().Command(string("disableCommand ") + gVersionControlPluginCommands[i], MAProtocol);
		++i;
	}
	
    m_IsOnline = true;
}

int VersionControlPlugin::SelectVersion(const VersionControlPluginVersions& unitySupportedVersions)
{
    const VersionControlPluginVersions& pluginSupportedVersions = GetSupportedVersions();
    VersionControlPluginVersions candidates;
    set_intersection(unitySupportedVersions.begin(), unitySupportedVersions.end(),
                     pluginSupportedVersions.begin(), pluginSupportedVersions.end(),
                     inserter(candidates, candidates.end()));
    if (candidates.empty())
        return -1;
    
    return *candidates.rbegin();
}


