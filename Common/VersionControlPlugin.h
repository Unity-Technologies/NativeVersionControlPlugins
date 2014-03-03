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

/*
 * VersionControl Plugin configuration field.
 */
class VersionControlPluginCfgField {
public:
    enum FieldFlags
    {
        kNoneField = 0,
        kRequiredField = 1,
        kPasswordField = 2,
		kToggleField = 4,
		kHiddenField = 8
    };

    VersionControlPluginCfgField()
    {
        m_Prefix = "";
        m_Name = "";
        m_Label = "";
        m_Description = "";
        m_DefaultValue = "";
        m_Flags = kNoneField;
        m_Value = "";
    }
    
    VersionControlPluginCfgField(const std::string& prefix, const std::string& name, const std::string& label, const std::string& description, const std::string& defaultValue, FieldFlags flags)
    {
        m_Prefix = prefix;
        m_Name = name;
        m_Label = label;
        m_Description = description;
        m_DefaultValue = defaultValue;
        m_Flags = flags;
        m_Value = m_DefaultValue;
    }
    
    VersionControlPluginCfgField(const std::string& prefix, const std::string& name, const std::string& label, const std::string& description, bool defaultValue, FieldFlags flags)
	{
        m_Prefix = prefix;
        m_Name = name;
        m_Label = label;
        m_Description = description;
        m_DefaultValue = (defaultValue ? "true" : "false");
        m_Flags = flags;
        m_Value = m_DefaultValue;
	}

    VersionControlPluginCfgField(const VersionControlPluginCfgField& other)
    {
        m_Prefix = other.GetPrefix();
        m_Name = other.GetName();
        m_Label = other.GetLabel();
        m_Description = other.GetDescription();
        m_DefaultValue = other.GetDefaultValue();
        m_Flags = (FieldFlags)other.GetFlags();
        m_Value = other.GetValue();
    }
    
	const std::string& GetPrefix() const { return m_Prefix; }
	const std::string& GetName() const { return m_Name; }
    const std::string& GetLabel() const { return m_Label; }
	const std::string& GetDescription() const { return m_Description; }
    const std::string& GetDefaultValue() const { return m_DefaultValue; }
    const std::string& GetValue() const { return m_Value; }
	void SetValue(const std::string& newValue) { m_Value = newValue; }
    int GetFlags() const { return m_Flags; }

    VersionControlPluginCfgField& operator= (const VersionControlPluginCfgField& rhs)
    {
        m_Prefix = rhs.GetPrefix();
        m_Name = rhs.GetName();
        m_Label = rhs.GetLabel();
        m_Description = rhs.GetDescription();
        m_DefaultValue = rhs.GetDefaultValue();
        m_Flags = (FieldFlags)rhs.GetFlags();
        m_Value = rhs.GetValue();
        
        return (*this);
    }

private:
	std::string m_Prefix;
	std::string m_Name;
	std::string m_Label;
	std::string m_Description;
	std::string m_DefaultValue;
	std::string m_Value;
	FieldFlags m_Flags;
};

ENUM_FLAGS(VersionControlPluginCfgField::FieldFlags);

Connection& operator<<(Connection& p, const VersionControlPluginCfgField& field);

class VersionControlPluginCustomCommand
{
public:
	VersionControlPluginCustomCommand()
	{
		m_Name = "";
		m_Label = "";
		m_Context = "";
	}
	
	VersionControlPluginCustomCommand(const std::string& name, const std::string& label)
	{
		m_Name = name;
		m_Label = label;
		m_Context = "global";
	}
	
	const std::string& GetName() const { return m_Name; }
	const std::string& GetLabel() const { return m_Label; }
	const std::string& GetContext() const { return m_Context; }

private:
	std::string m_Name;
	std::string m_Label;
	std::string m_Context;
};

/*
 * VersionControl Plugin abstract class
 */
class VersionControlPlugin
{
public:
    typedef std::map<std::string, std::string> VersionControlPluginMapOfArguments;
    typedef std::set<int> VersionControlPluginVersions;
    typedef std::vector<VersionControlPluginCfgField> VersionControlPluginCfgFields;
    typedef std::map<State, std::string> VersionControlPluginOverlays;
    typedef std::vector<VersionControlPluginCustomCommand> VersionControlPluginCustomCommands;
    
    // Plugin configuration traits flags
    enum TraitsFlags {
        kRequireNetwork = 1 << 0,
        kEnablesCheckout = 1 << 1,
        kEnablesLocking = 1 << 2,
        kEnablesRevertUnchanged = 1 << 3,
        kEnablesVersioningFolders = 1 << 4,
        kEnablesChangelists = 1 << 5,
        kEnablesGetLatestOnChangeSetSubset = 1 << 6,
        kEnablesConflictHandlingByPlugin = 1 << 7,
		kEnablesAll = (1 << 8) - 2
    };
    
    // Plugin UI commands flags
    enum CommandsFlags {
        kNone = 0,
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
    
    // Plugin resolve method
    enum ResolveMethod {
        kMine = 1 << 0,
        kTheirs = 1 << 1,
        kMerged = 1 << 2
    };
    
    // Plugin file mode
    enum FileMode {
        kUnused = 0,
        kBinary = 1 << 0,
        kText = 1 << 1
    };
    
    // Plugin mark method
    enum MarkMethod {
        kUseMine = 1 << 0,
        kUseTheirs = 1 << 1,
    };

    // Main loop, read command from input and process it until EOF.
    int Run();

    // Get plugin connection with Unity.
    Connection& GetConnection() { return (*m_Connection); }
	
	virtual int Test() { return 0; }
	
    // Get ProjectPath
    inline const std::string& GetProjectPath() const { return m_ProjectPath; }

protected:
    
    // Constructors
    VersionControlPlugin(const std::string& name, int argc, char** argv);
    VersionControlPlugin(const std::string& name, const char* args);

	// Arguments
	const std::string GetArg(const std::string& name);
	bool HasArg(const std::string& name);

    // Online/Offline
    inline bool IsOnline() const { return m_IsOnline; }
    inline void SetOnline() { m_IsOnline = true; }
    inline void SetOffline() { m_IsOnline = false; }
    void NotifyOffline(const std::string& reason);
    void NotifyOnline();
    
    // Set ProjectPath
    void SetProjectPath(const std::string& path);
    
    // Plugin name
    inline const std::string& GetPluginName() const { return m_PluginName; }

    // VCStatus
	const VCSStatus& GetStatus() const;
	void ClearStatus();
    VCSStatus& StatusAdd(VCSStatusItem item);

	inline void SetDirty() { m_IsDirty = true; }
	inline void ResetDirty() { m_IsDirty = false; }
	inline bool IsDirty() { return m_IsDirty; }

    /*
     * Get the log file name.
     * Returns:
     *  - string that reperesent the log file name.
     */
    virtual const char* GetLogFileName() = 0;
    
    /*
     * Get the supported Unity versions.
     * Returns:
     *  - set of supported Unity versions.
     */
    virtual const VersionControlPluginVersions& GetSupportedVersions() = 0;
    
    /*
     * Get the supported plugin traits.
     * Returns:
     *  - The supported plugin traits.
     */
    virtual const TraitsFlags GetSupportedTraitFlags() = 0;
    
    /*
     * Get the plugin configuration fields.
     * Returns:
     *  - list of plugin configuration fields.
     */
    virtual VersionControlPluginCfgFields& GetConfigFields() = 0;
    
    /*
     * Get the plugin custom commands.
     * Returns:
     *  - list of plugin custom commands.
     */
    virtual const VersionControlPluginCustomCommands& GetCustomCommands() = 0;
    
    /*
     * Get the commands that should be enabled when plugin is online.
     * Returns:
     *  - the commands to enable when plugin is online.
     */
    virtual CommandsFlags GetOnlineUICommands() = 0;

    /*
     * Get the commands that should be enabled when plugin is offline.
     * Returns:
     *  - the commands to enable when plugin is online.
     */
    virtual CommandsFlags GetOfflineUICommands() = 0;
    
    /*
     * Get the plugin overlays (with icons if not default).
     * Returns:
     *  - the plugin overlays.
     */
    virtual const VersionControlPluginOverlays& GetOverlays() { return s_emptyOverlays; };
    
    /*
     * Connect to underlying VC system.
     * Returns:
     * - 0 if success, other values on failure (VCStatus contains errors).
     */
    virtual int Connect() = 0;
    
    /*
     * Disconnect from underlying VC system.
     * Returns:
     * - 0 if success, other values on failure (VCStatus contains errors).
     */
    virtual void Disconnect() = 0;
    
    /*
     * Check whether or not plugin is connected to underlying VC system.
     * Returns:
     * - true if connected, false otherwise.
     */
    virtual bool IsConnected() { return true; }
    
    /*
     * Ensure plugin is connected to underlying VC system.
     * Returns:
     * - true if connected, false otherwise.
     */
    virtual bool EnsureConnected() = 0;

    /*
     * Login to underlying VC system.
     * Returns:
     * - 0 if logged in, other values on failure (VCStatus contains errors).
     */
    virtual int Login() { return 0; }
    
    /*
     * Add assets to VC.
     * Parameters: 
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be added.
     * On output, assetList contains assets that have been added with appropriate status.
     */
    virtual bool AddAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Checkout assets in VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be checked out.
     * On output, assetList contains assets that have been checked out with appropriate status.
     */
    virtual bool CheckoutAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Download assets from VC.
     * Parameters:
     *  - targetDir: IN target directory.
     *  - changes: IN list of changes.
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to download.
     * On output, assetList contains assets that have been downloaded from VC with appropriate status.
     */
    virtual bool DownloadAssets(const std::string& targetDir, const ChangelistRevisions& changes, VersionedAssetList& assetList) = 0;
    
    /*
     * Get assets from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to get.
     * On output, assetList contains assets that have been get from VC with appropriate status.
     */
    virtual bool GetAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Revert assets from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be reverted.
     * On output, assetList contains assets that have been reverted with appropriate status.
     */
    virtual bool RevertAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Resolve assets from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     *  - method: IN resolve method.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be resolved.
     * On output, assetList contains assets that have been resolved with appropriate status.
     */
    virtual bool ResolveAssets(VersionedAssetList& assetList, ResolveMethod method = kMine) = 0;
    
    /*
     * Remove assets from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be removed.
     * On output, assetList contains assets that have been removed with appropriate status.
     */
    virtual bool RemoveAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Move assets in VC.
     * Parameters:
     *  - fromAssetList: IN list of versioned asset.
     *  - toAssetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, fromAssetList contains assets to be moved and toAssetList the corresponding asset.
     * On output, toAssetList contains assets that have been moved with appropriate status.
     */
    virtual bool MoveAssets(const VersionedAssetList& fromAssetList, VersionedAssetList& toAssetList) = 0;
    
    /*
     * Lock assets in VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be locked.
     * On output, assetList contains assets that have been locked with appropriate status.
     */
    virtual bool LockAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Unlock assets in VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be unlocked.
     * On output, assetList contains assets that have been unlocked with appropriate status.
     */
    virtual bool UnlockAssets(VersionedAssetList& assetList) = 0;
    
    /*
     * Submit assets in VC.
     * Parameters:
     *  - changeList: IN change list (revision, description, timestamp and commiter).
     *  - assetList: IN/OUT list of versioned asset.
	 *  - saveOnly: save changes only.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be submitted.
     * On output, assetList contains assets that have been submitted with appropriate status.
     */
    virtual bool SubmitAssets(const Changelist& changeList, VersionedAssetList& assetList, bool saveOnly = false) = 0;
    
    /*
     * Set assets file mode in VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     *  - mode: file mode (Text or Binary)
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets for which file mode needs to be changes.
     * On output, assetList contains assets with file mode changed.
     */
    virtual bool SetAssetsFileMode(VersionedAssetList& assetList, FileMode mode) = 0;
    
    /*
     * Get assets status from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     *  - recursive: IN boolean, true if status is made recursively
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets for which status is asked.
     * On output, assetList contains assets with appropriate status.
     */
    virtual bool GetAssetsStatus(VersionedAssetList& assetList, bool recursive = false) = 0;
    
    /*
     * Get assets status from VC for a specific revision.
     * Parameters:
     *  - revision: IN specific revision.
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets for which status is asked.
     * On output, assetList contains assets with appropriate status.
     */
    virtual bool GetAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
    
    /*
     * Get assets incomig status from VC for a specific revision.
     * Parameters:
     *  - revision: IN specific revision.
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets for which status is asked.
     * On output, assetList contains assets with appropriate status.
     */
    virtual bool GetIncomingAssetsChangeStatus(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
    
    /*
     * Get changes from VC.
     * Parameters:
     *  - changes: OUT list of changes.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On output, changes contains list of outgoing changes.
     */
    virtual bool GetAssetsChanges(Changes& changes) = 0;
    
    /*
     * Get incoming changes from VC.
     * Parameters:
     *  - changes: OUT list of changes.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On output, changes contains list of incoming changes.
     */
    virtual bool GetAssetsIncomingChanges(Changes& changes) = 0;
    
    /*
     * Set assest revision in VC.
     * Parameters:
     *  - revision: IN specific revision.
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets for which revision should be set.
     * On output, assetList contains assets with appropriate status.
     */
    virtual bool SetRevision(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
    
    /*
     * Update specific revision in VC.
     * Parameters:
     *  - revision: IN specific revision.
     *  - description: OUT new description.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     */
    virtual bool UpdateRevision(const ChangelistRevision& revision, std::string& description) = 0;
    
    /*
     * Delete specific revision from VC.
     * Parameters:
     *  - revision: IN specific revision.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     */
    virtual bool DeleteRevision(const ChangelistRevision& revision) = 0;
    
    /*
     * Revert assets to a specific revision from VC.
     * Parameters:
     *  - revision: IN specific revision.
     *  - assetList: IN/OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be reverted.
     * On output, assetList contains assets that have been reverted with appropriate status.
     */
    virtual bool RevertChanges(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;
	
	/**
	 * Perform custom command
     * Parameters:
     *  - command: IN specific command.
     *  - CommandArgs: IN command arguments.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
	 */
	virtual bool PerformCustomCommand(const std::string& command, const CommandArgs& args) = 0;

	/**
	 * List a specifc revision
     * Parameters:
     *  - revisionID: IN specific revision.
     *  - assetList: OUT list of versioned asset.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
	 */
	virtual bool ListRevision(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;

	/**
	 * Update to a specifc revision
     * Parameters:
     *  - revisionID: IN specific revision.
     *  - ignoredAssetList: IN list of versioned asset to be ignored.
     *  - assetList: OUT list of versioned asset updated.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).

     * On output, assetList contains assets updated with appropriate status.
	 */
	virtual bool UpdateToRevision(const ChangelistRevision& revision, const VersionedAssetList& ignoredAssetList, VersionedAssetList& assetList) = 0;

	/**
	 * Apply revision specific changes
     * Parameters:
     *  - revisionID: IN specific revision.
     *  - assetList: IN/OUT versioned asset to be fetched.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
	 *
     * On input, assetList contains assets changes to be applied.
     * On output, assetList contains assets changes applied with appropriate status.
	 */
	virtual bool ApplyRevisionChanges(const ChangelistRevision& revision, VersionedAssetList& assetList) = 0;

	/**
	 * Get current revision
     * Parameters:
     *  - revision: OUT current revision.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
	 */
	virtual bool GetCurrentRevision(std::string& revision) = 0;

	/**
	 * Get latest revision
     * Parameters:
     *  - revision: OUT latest revision.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
	 */
	virtual bool GetLatestRevision(std::string& revision) = 0;

	/**
	 * Get current version
     * Parameters:
     *  - version: OUT current version.
	 */
	virtual void GetCurrentVersion(std::string& version) = 0;

    /*
     * Mask assets from VC.
     * Parameters:
     *  - assetList: IN/OUT list of versioned asset.
     *  - method: IN mark method.
     * Returns:
     *  - True if operation succeeded, false otherwise (VCStatus contains errors).
     *
     * On input, assetList contains assets to be resolved.
     * On output, assetList contains assets that have been marked with appropriate status.
     */
    virtual bool MarkAssets(VersionedAssetList& assetList, MarkMethod method = kUseMine) = 0;

private:
    void InitializeArguments(int argc, char** argv);
    
    bool PreHandleCommand();
    void PostHandleCommand(bool wasOnline);

    bool HandleAdd();
    bool HandleChangeDescription();
    bool HandleChangeMove();
    bool HandleChanges();
    bool HandleChangeStatus();
    bool HandleCheckout();
    bool HandleConfig(const CommandArgs& args);
    bool HandleConfigTraits();
    bool HandleCustomCommand(const CommandArgs& args);
    bool HandleDelete();
    bool HandleDeleteChanges();
    bool HandleDownload();
    bool HandleExit();
    bool HandleFileMode(const CommandArgs& args);
    bool HandleGetlatest();
    bool HandleIncoming();
    bool HandleIncomingChangeAssets();
    bool HandleLock();
    bool HandleLogin();
    bool HandleMove(const CommandArgs& args);
    bool HandleQueryConfigParameters();
    bool HandleResolve(const CommandArgs& args);
    bool HandleRevert();
    bool HandleRevertChanges();
    bool HandleSetConfigParameters(const CommandArgs& args);
    bool HandleStatus(const CommandArgs& args);
    bool HandleSubmit(const CommandArgs& args);
    bool HandleUnlock();
    bool HandleListRevision();
    bool HandleUpdateToRevision();
    bool HandleApplyRevisionChanges();
	bool HandleCurrentRevision();
	bool HandleLatestRevision();
	bool HandleCurrentVersion();
	bool HandleMark(const CommandArgs& args);

    bool HandleVersionedAssetsCommand(UnityCommand cmd);
    bool Dispatch(UnityCommand command, const CommandArgs& args);
    
    int SelectVersion(const VersionControlPluginVersions& unitySupportedVersions);

    std::string m_PluginName;
	std::string m_ProjectPath;
    VCSStatus m_Status;
    VersionControlPluginMapOfArguments m_Arguments;
    
    bool m_IsOnline;
	bool m_IsDirty;
    Connection* m_Connection;
    
    static VersionControlPluginOverlays s_emptyOverlays;
};

ENUM_FLAGS(VersionControlPlugin::TraitsFlags);

ENUM_FLAGS(VersionControlPlugin::CommandsFlags);

#endif // VERSIONCONTROLPLUGIN_H
