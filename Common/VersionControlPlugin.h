#ifndef VERSIONCONTROLPLUGIN_H
#define VERSIONCONTROLPLUGIN_H

#include "Connection.h"
#include "Command.h"
#include "Status.h"
#include "VersionedAsset.h"
#include "Changes.h"
#include <string>
#include <map>
#include <set>

#define ENUM_FLAGS(T) \
inline T  operator  |(const T s, const T e) { return (T)((unsigned)s | e); } \
inline T &operator |=(T      &s, const T e) { return s = s | e; }            \
inline T  operator  &(const T s, const T e) { return (T)((unsigned)s & e); } \
inline T &operator &=(T      &s, const T e) { return s = s & e; }            \
inline T  operator  ^(const T s, const T e) { return (T)((unsigned)s ^ e); } \
inline T &operator ^=(T      &s, const T e) { return s = s ^ e; }            \
inline T  operator  ~(const T s)            { return (T)~(unsigned)s; }

enum VersionControlUnityVersion { kUnity42 = 1, kUnity43 = 2 };

class VersionControlPluginCfgField {
public:
    enum FieldFlags
    {
        kNoneField = 0,
        kRequiredField = 1,
        kPasswordField = 2
    };

    VersionControlPluginCfgField(const std::string& name, const std::string& label, const std::string& description, const std::string& defaultValue, FieldFlags flags)
    {
        m_Name = name;
        m_Label = label;
        m_Description = description;
        m_DefaultValue = defaultValue;
        m_Flags = flags;
        m_Value = m_DefaultValue;
    }

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
    
	const std::string& GetLabel() const { return m_Label; }
	void SetLabel(const std::string& label) { m_Label = label; }

	const std::string& GetDescription() const { return m_Description; }
    void SetDescription(const std::string& desc) { m_Description = desc; }

    const std::string& GetDefaultValue() const { return m_DefaultValue; }
    void SetDefaultValue(const std::string& defaultValue) { m_DefaultValue = defaultValue; }

    const std::string& GetValue() const { return m_Value; }
	void SetValue(const std::string& newValue) { m_Value = newValue; }
    
	bool IsRequired() const { return (m_Flags & kRequiredField) == kRequiredField; }
	bool IsPassword() const { return (m_Flags & kPasswordField) == kPasswordField; }
	
    int GetFlags() const { return m_Flags; }
    void SetFlags(int flags) { m_Flags = (FieldFlags)flags; }

    VersionControlPluginCfgField& operator=( const VersionControlPluginCfgField& rhs )
    {
        m_Name = rhs.GetName();
        m_Label = rhs.GetLabel();
        m_Description = rhs.GetDescription();
        m_DefaultValue = rhs.GetDefaultValue();
        m_Flags = (FieldFlags)rhs.GetFlags();
        m_Value = rhs.GetValue();
        
        return (*this);
    }

private:
	std::string m_Name;
	std::string m_Label;
	std::string m_Description;
	std::string m_DefaultValue;
	std::string m_Value;
	FieldFlags m_Flags;
};

ENUM_FLAGS(VersionControlPluginCfgField::FieldFlags);

Connection& operator<<(Connection& p, const VersionControlPluginCfgField& field);

class VersionControlPlugin
{
public:
    typedef std::map<std::string, std::string> VersionControlPluginMapOfArguments;
    typedef std::set<int> VersionControlPluginVersions;
    typedef std::vector<VersionControlPluginCfgField> VersionControlPluginCfgFields;
    typedef std::map<State, std::string> VersionControlPluginOverlays;
    
    enum TraitsFlags {
        kRequireNetwork = 1 << 0,
        kEnablesCheckout = 1 << 1,
        kEnablesLocking = 1 << 2,
        kEnablesRevertUnchanged = 1 << 3,
        kEnablesVersioningFolders = 1 << 4,
        kEnablesChangelists = 1 << 5,
        kEnablesGetLatestOnChangeSetSubset = 1 << 6,
        kEnablesConflictHandlingByPlugin = 1 << 7
    };
    
    enum CommandsFlags {
        kAdd = 1 << 0,
		kChangeDescription = 1 << 1,
        kChangeMove = 1 << 2,
		kChanges = 1 << 3,
        kChangeStatus = 1 << 4,
        kCheckout = 1 << 5,
		kDeleteChanges = 1 << 6,
		kDelete = 1 << 7,
        kDownload = 1 << 8,
		kGetLatest = 1 << 9,
        kIncomingChangeAssets = 1 << 10,
        kIncoming = 1 << 11,
		kLock = 1 << 12,
        kMove = 1 << 13,
        kResolve = 1 << 14,
		kRevertChanges = 1 << 15,
        kRevert = 1 << 16,
        kStatus = 1 << 17,
		kSubmit = 1 << 18,
        kUnlock = 1 << 19,
        kAll = (1 << 20) - 1
    };
    
    int Run();
    
protected:
    
    VersionControlPlugin();
    VersionControlPlugin(int argc, char** argv);
    VersionControlPlugin(const char* args);
    ~VersionControlPlugin();
    
    Connection& GetConnection() { return (*m_Connection); }

    inline void SetProjectPath(const std::string& path) { m_ProjectPath = path; }
    inline const std::string& GetProjectPath() const { return m_ProjectPath; }
    
    virtual const char* GetLogFileName() = 0;
    virtual const VersionControlPluginVersions& GetSupportedVersions() = 0;
    virtual const TraitsFlags GetSupportedTraitFlags() = 0;
    virtual VersionControlPluginCfgFields& GetConfigFields() = 0;
    virtual CommandsFlags GetOnlineUICommands() = 0;
    virtual const VersionControlPluginOverlays& GetOverlays() { return s_emptyOverlays; };
    
	const VCSStatus& GetStatus() const;
	VCSStatus& GetStatus();
	void ClearStatus();

    virtual int Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() = 0;
    
    virtual bool AddAssets(VersionedAssetList& assetList) = 0;
    virtual bool CheckoutAssets(VersionedAssetList& assetList) = 0;
    virtual bool GetAssets(VersionedAssetList& assetList) = 0;
    virtual bool RevertAssets(VersionedAssetList& assetList) = 0;
    virtual bool ChangeOrMoveAssets(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
    virtual bool SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList) = 0;
    virtual bool GetAssetsStatus(VersionedAssetList& assetList, bool recursive = false) = 0;
    virtual bool GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
    virtual bool GetAssetsChanges(Changes& changes) = 0;
    virtual bool GetAssetsIncomingChanges(Changes& changes) = 0;

	std::string m_ProjectPath;
    VCSStatus m_Status;
    VersionControlPluginMapOfArguments m_arguments;

    void NotifyOffline(const std::string& reason);
    void NotifyOnline();
    
    inline bool IsOnline() const { return m_IsOnline; }
    inline void SetOnline() { m_IsOnline = true; }
    inline void SetOffline() { m_IsOnline = false; }
    
private:
    void InitializeArguments(int argc, char** argv);

    bool PreHandleCommand();
    void PostHandleCommand(bool wasOnline);

    bool HandleAdd();
    bool HandleChangeDescription(const CommandArgs& args);
    bool HandleChangeMove(const CommandArgs& args);
    bool HandleChanges();
    bool HandleChangeStatus();
    bool HandleCheckout();
    bool HandleConfig(const CommandArgs& args);
    bool HandleConfigTraits();
    bool HandleCustomCommand(const CommandArgs& args);
    bool HandleDelete(const CommandArgs& args);
    bool HandleDeleteChanges(const CommandArgs& args);
    bool HandleDownload(const CommandArgs& args);
    bool HandleExit(const CommandArgs& args);
    bool HandleFileMode(const CommandArgs& args);
    bool HandleGetlatest();
    bool HandleIncoming();
    bool HandleIncomingChangeAssets(const CommandArgs& args);
    bool HandleLock(const CommandArgs& args);
    bool HandleLogin(const CommandArgs& args);
    bool HandleMove(const CommandArgs& args);
    bool HandleQueryConfigParameters(const CommandArgs& args);
    bool HandleResolve(const CommandArgs& args);
    bool HandleRevert(const CommandArgs& args);
    bool HandleRevertChanges(const CommandArgs& args);
    bool HandleSetConfigParameters(const CommandArgs& args);
    bool HandleStatus(const CommandArgs& args);
    bool HandleSubmit(const CommandArgs& args);
    bool HandleUnlock(const CommandArgs& args);

    bool HandleVersionedAssetsCommand(UnityCommand cmd);
    bool Dispatch(UnityCommand command, const CommandArgs& args);
    
    int SelectVersion(const VersionControlPluginVersions& unitySupportedVersions);

    bool m_IsOnline;
    Connection* m_Connection;
    
    static VersionControlPluginOverlays s_emptyOverlays;
};

ENUM_FLAGS(VersionControlPlugin::TraitsFlags);

ENUM_FLAGS(VersionControlPlugin::CommandsFlags);

#endif // VERSIONCONTROLPLUGIN_H
