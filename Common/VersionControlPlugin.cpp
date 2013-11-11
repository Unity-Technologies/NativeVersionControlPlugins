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
    "enablesVersioningFolders",
    "enablesLocking",
    "enablesRevertUnchanged",
    "enablesChangelists",
    "enablesGetLatestOnChangeSetSubset",
    "enablesConflictHandlingByPlugin",
    0
};

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

VersionControlPlugin::VersionControlPlugin(const std::string& name, const char* args) : m_PluginName(name),  m_ProjectPath(""), m_IsOnline(false), m_Connection(NULL)
{
    int argc = 0;
    char** argv = CommandLineToArgv(args, &argc);
    InitializeArguments(argc, argv);
    CommandLineFreeArgs(argv);
}

VersionControlPlugin::VersionControlPlugin(const std::string& name, int argc, char** argv) : m_PluginName(name), m_ProjectPath(""), m_IsOnline(false), m_Connection(NULL)
{
    InitializeArguments(argc, argv);
}

void VersionControlPlugin::InitializeArguments(int argc, char** argv)
{
    vector<string> pathComp;
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
    
    for (int i = 1 ; i < argc ; )
    {
        if ((argv[i][0] == '-') && (i+1 < argc) && (argv[i+1][0] != '-')) {
            m_arguments[argv[i]] = argv[i+1];
            i += 2;
            continue;
        }
        
        m_arguments[argv[i++]] = "";
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
    m_Status.insert(item);
    return m_Status;
}

int VersionControlPlugin::Run()
{
	VersionControlPluginMapOfArguments::const_iterator i = m_arguments.find("-l");
    m_Connection = new Connection((i != m_arguments.end()) ? i->second : GetLogFileName());
    
    i = m_arguments.find("-v");
    if (i != m_arguments.end())
    {
        GetConnection().Log().SetLogLevel(LOG_DEBUG);
    }
    
    try
	{
		UnityCommand cmd;
		vector<string> args;
		
        i = m_arguments.find("-c");
        if (i != m_arguments.end())
        {
            if (Tokenize(args, i->second) == 0)
            {
                return 1; // error
            }
            
            cmd = StringToUnityCommand(args[0].c_str());
            if (!Dispatch(cmd, args))
				return 1; // error
            
            return 0; // ok
        }
        
		for ( ;; )
		{
            GetConnection().Log().Debug() << "ReadCommand" << Endl;
            
			cmd = GetConnection().ReadCommand(args);
            
			if (cmd == UCOM_Invalid)
            {
                GetConnection().Log().Debug() << "Invalid command" << Endl;
				return 1; // error
			}
            
            if (cmd == UCOM_Shutdown)
			{
                GetConnection().Log().Debug() << "Shutdown" << Endl;
				GetConnection().EndResponse();
				return 0; // ok
			}
			
            if (!Dispatch(cmd, args))
				return 0; // error
		}
	}
	catch (exception& e)
	{
		GetConnection().Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
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
    GetConnection().Log().Debug() << "HandleAdd" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleChangeDescription" << Endl;

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
    GetConnection().Log().Debug() << "HandleChangeMove" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleChanges" << Endl;

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
    GetConnection().Log().Debug() << "HandleChangeStatus" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleCheckout " << Endl;
    
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
    GetConnection().Log().Debug() << "HandleConfig" << Endl;
    
    if (args.size() < 2)
    {
        string msg = "Plugin got invalid config setting :";
        for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i) {
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
        Disconnect();
        GetConnection().EndResponse();
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
    GetConnection().Log().Debug() << "HandleCustomCommand" << Endl;
    
    bool m_WasOnline = PreHandleCommand();
    // TODO
    PostHandleCommand(m_WasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleDelete()
{
    GetConnection().Log().Debug() << "HandleDelete" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleDeleteChanges" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleDownload" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleExit" << Endl;
    
    bool wasOnline = PreHandleCommand();
    // TODO: no protocol documentation found
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleFileMode(const CommandArgs& args)
{
    GetConnection().Log().Debug() << "HandleFileMode" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleGetlatest" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleIncoming" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleIncomingChangeAssets" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleLock" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleLogin" << Endl;
    
    bool m_WasOnline = PreHandleCommand();
    if (Login())
        SetOnline();

    PostHandleCommand(m_WasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleMove(const CommandArgs& args)
{
    GetConnection().Log().Debug() << "HandleMove" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleQueryConfigParameters" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleResolve" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleRevert" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleRevertChanges" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleSubmit" << Endl;
    
    bool wasOnline = PreHandleCommand();
    
    Changelist changelist;
    GetConnection() >> changelist;
    
    VersionedAssetList assetList;
	GetConnection() >> assetList;
    
    if (SubmitAssets(changelist, assetList))
        SetOnline();
    else
        assetList.clear();
    
    GetConnection() << assetList;
    
    PostHandleCommand(wasOnline);
    
    return true;
}

bool VersionControlPlugin::HandleStatus(const CommandArgs& args)
{
    GetConnection().Log().Debug() << "HandleStatus" << Endl;
    
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
    GetConnection().Log().Debug() << "HandleUnlock" << Endl;
    
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
		++i;
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
    
    //TODO: CustomCommands

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
            field.SetValue(TrimEnd(value));
            GetConnection().Log().Info() << key << " set to " << value << Endl;
            return true;
        }
    }
    
    GetConnection().Log().Info() << "Unable to set " << key << " to " << value << Endl;
    return false;
}

bool VersionControlPlugin::Dispatch(UnityCommand command, const CommandArgs& args)
{
    GetConnection().Log().Debug() << "Dispatch " << UnityCommandToString(command) << Endl;
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
	
    m_IsOnline = false;
}

void VersionControlPlugin::NotifyOnline()
{
	if (m_IsOnline)
		return; // nothing to notify
    
	GetConnection().Command("online");

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


