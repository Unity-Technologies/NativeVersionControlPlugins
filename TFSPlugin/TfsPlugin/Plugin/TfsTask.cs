using Microsoft.TeamFoundation.Client;
using Microsoft.TeamFoundation.Framework.Client;
using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;

namespace TfsPlugin
{
    public class ChangesetData
    {
        public ChangesetData(string comment, VersionedAssetList assets)
        {
            this.Comment = comment;
            Changes = assets;
        }
        public string Comment { get; set; }
        public VersionedAssetList Changes { get; set; }
    }

    public class TfsTask : IDisposable
    {
        class NativeMethods
        {
            [DllImport("user32.dll", CharSet = CharSet.Unicode)]
            public static extern int MessageBox(IntPtr hWnd, String text, String caption, uint type);
        }

        const uint MB_ICONEXCLAMATION = unchecked((int)0x00000030L);

        const uint MB_OK = unchecked((int)0x00000000L);

        // TFS
        TeamFoundationIdentity identity;
        TfsTeamProjectCollection projectCollection;
        Workspace workspace;

        Connection m_Connection;
        bool? isOnline;

        // unity config
        string projectPath;
        string tpcPath;

        public bool IsPartiallyOffline { get; set; }

        // plugin settings cache
        List<string> exclusiveCheckoutTypes = new List<string>();
        Lazy<List<Regex>> nolockRegexCache;
        Lazy<List<Regex>> ignoreRegexCache;
        Lazy<LockLevel> lockLevel;
        Lazy<bool> shareBinaryFileCheckout;

        // status cache
        Dictionary<int, ChangesetData> changesetCache = new Dictionary<int, ChangesetData>();
        TfsFileStatusCache tfsFileStatusCache = new TfsFileStatusCache();

        // unit test helpers
        public event Action<string> WarningDialogDisplayed;

        public void ClearTFSReferences()
        {
            identity = null;
            projectCollection = null;
            workspace = null;
        }

        public void Close()
        {
            if (m_Connection != null)
            {
                m_Connection.Close();
                m_Connection = null;
            }

            if (projectCollection != null)
            {
                projectCollection.Dispose();
                projectCollection = null;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                Close();
            }
        }

        public string ExclusiveCheckoutTypesToString()
        {
            return string.Join(", ", exclusiveCheckoutTypes.Where(each => !each.EndsWith(".meta", StringComparison.CurrentCultureIgnoreCase)));
        }

        public TfsTask(Connection conn, string tpcPath, string projectPath)
        {
            m_Connection = conn;
            this.tpcPath = tpcPath;

            if (!string.IsNullOrEmpty(Settings.Default.ExclusiveCheckoutFileTypes))
            {
            foreach (var item in Settings.Default.ExclusiveCheckoutFileTypes.Split('|'))
            {
                exclusiveCheckoutTypes.Add(item);
                exclusiveCheckoutTypes.Add(item + ".meta");
            }
            }

            if (!string.IsNullOrEmpty(projectPath))
            {
                SetProjectPath(projectPath);
            }
            else
            {
                this.projectPath = string.Empty;
            }

        }

        public TfsFileStatusCache TfsFileStatusCache
        {
            get { return tfsFileStatusCache; }
        }

        private void FillWildcardList(string content, char endline, List<Regex> result)
        {
            try
            {
                if (!string.IsNullOrEmpty(content))
                {
                    var lines = content.Split(new[] { endline }, StringSplitOptions.RemoveEmptyEntries);

                    AddFileWildcards(result, lines);
                }
            }
            catch (Exception e)
            {
                m_Connection.LogDebug(e.Message);
            }
        }

        private void FillWildcardList(string file, List<Regex> result)
        {
            if (File.Exists(file))
            {
                try
                {
                    var lines = File.ReadAllLines(file);

                    AddFileWildcards(result, lines);
                }
                catch (Exception e)
                {
                    m_Connection.LogDebug(e.Message);
                }
            }
        }

        private void AddFileWildcards(List<Regex> result, string[] lines)
        {
            foreach (var item in lines)
            {
                if (item.Trim().StartsWith("#"))
                {
                    continue;
                }

                result.Add(new Regex(WildcardToRegex(item), RegexOptions.IgnoreCase));
            }
        }

        public static string WildcardToRegex(string pattern)
        {
            return "^" + Regex.Escape(pattern).Replace("\\*", ".*") + "$";
        }

        public string ProjectPath
        {
            get
            {
                return projectPath;
            }
        }


        public TeamFoundationIdentity Identity
        {
            get
            {
                lock (this)
                {
                    if (identity == null)
                    {
                        this.ProjectCollection.GetAuthenticatedIdentity(out identity);
                    }

                    return identity;
                }
            }
        }


        public TfsTeamProjectCollection ProjectCollection
        {
            get
            {
                lock (this)
                {
                    if (projectCollection == null)
                    {
                        var url = TfsUrl();
                        try
                        {
                            this.projectCollection = new TfsTeamProjectCollection(new Uri(url));
                            this.projectCollection.EnsureAuthenticated();
                        }
                        finally
                        {
                            if (projectCollection == null || projectCollection.HasAuthenticated == false)
                            {
                                NotifyOffline("Unable to connect to " + url);
                            }
                            else
                            {
                                NotifyOnline();
                            }
                        }
                    }

                    return projectCollection;
                }
            }
        }

        public VersionControlServer VersionControlServer
        {
            get
            {
                return this.ProjectCollection.GetService<VersionControlServer>();
            }
        }

        public Workspace Workspace
        {
            get
            {
                lock (this)
                {
                    if (workspace == null)
                    {
                        workspace = VersionControlServer.GetWorkspace(projectPath);
                    }

                    return workspace;
                }
            }
        }

        bool IsOnline() { return isOnline == true; }

        public int Run()
        {
            if (m_Connection == null)
                m_Connection = new Connection("./Library/tfsplugin.log");

            UnityCommand cmd;
            CommandArgs commandArgs = new CommandArgs();
            bool isIgnoreUser = IsIgnoringUser();

            try
            {
                m_Connection.LogInfo("\nBegin Connection - " + DateTime.Now.ToString() + " v" + GetAssemblyFileVersion());

                while (true)
                {
                    cmd = m_Connection.ReadCommand(commandArgs);

                    if (cmd == UnityCommand.UCOM_Invalid)
                    {
                        return 1; // error
                    }
                    else if (cmd == UnityCommand.UCOM_Shutdown)
                    {
                        m_Connection.EndResponse();
                        return 0; // ok 
                    }
                    else if (isIgnoreUser)
                    {
                        m_Connection.WarnLine("ignoring user");
                        m_Connection.EndResponse();
                        return 0;
                    }
                    else
                    {
                        Dispatch(m_Connection, this, cmd, commandArgs);
                    }

                    ClearTFSReferences();
                }
            }
            catch (Exception e)
            {
                m_Connection.WarnLine(("Fatal: " + e.ToString()));
            }

            return 1;
        }

        private bool IsIgnoringUser()
        {
            foreach (var item in Settings.Default.ignoreUsers.Split('|'))
            {
                if (Environment.UserName == item)
                {
                    return true;
                }
            }

            return false;
        }

        private List<Regex> InitializeIgnoreFiles()
        {
            var result = new List<Regex>();

            FillWildcardList(Settings.Default.tfsignore, ';', result);

            if (!string.IsNullOrEmpty(projectPath) && Directory.Exists(projectPath))
            {
                foreach (var item in Directory.GetFiles(projectPath, "*.tfsignore", SearchOption.TopDirectoryOnly))
                {
                    FillWildcardList(item, result);
                }
            }

            return result;
        }

        private List<Regex> InitializeNolockFiles()
        {
            var result = new List<Regex>();

            FillWildcardList(Settings.Default.tfsnolock, ';', result);

            if (!string.IsNullOrEmpty(projectPath) && Directory.Exists(projectPath))
            {
                foreach (var item in Directory.GetFiles(projectPath, "*.tfsnolock", SearchOption.TopDirectoryOnly))
                {
                    FillWildcardList(item, result);
                }
            }

            return result;
        }


        public static string GetAssemblyFileVersion()
        {
            var assembly = Assembly.GetExecutingAssembly();
            FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(assembly.Location);
            return fvi.FileVersion;
        }

        private string TfsUrl()
        {
            return string.IsNullOrEmpty(tpcPath) ? Settings.Default.DefaultURL : tpcPath;
        }

        void NotifyOffline(string reason)
        {
            if (isOnline == false)
            {
                return;
            }

            isOnline = false;

            if (string.IsNullOrEmpty(tpcPath))
            {

            }
            else
            {
                ShowWarningMessageBox(tpcPath + " Offline", "Working offline", true);
            }

            string[] disableCmds = {
                "add",
                "changeDescription", "changeMove",
                "changes", "changeStatus", "checkout",
                "deleteChanges",
                "delete", "download",
                "getLatest", "incomingChangeAssets", "incoming",
                "lock", "move", "resolve",
                "revertChanges", "revert", "status",
                "submit", "unlock"
            };


            foreach (var item in disableCmds)
            {
                m_Connection.Command("disableCommand " + item, MessageArea.MAProtocol);
            }

            m_Connection.Command("offline " + reason, MessageArea.MAProtocol);
        }

        private void NotifyOnline()
        {
            if (isOnline == true)
            {
                return;
            }

            isOnline = true;

            string[] enableCmds = {
                "add",
                "changeDescription",              
                "changes",
                "changeStatus",
                "checkout",               
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
                "unlock"
            };

            m_Connection.Command("online", MessageArea.MAProtocol);

            foreach (var item in enableCmds)
            {
                m_Connection.Command("enableCommand " + item, MessageArea.MAProtocol);
            }
        }


        public static bool Dispatch(Connection conn, TfsTask session, UnityCommand cmd, CommandArgs args)
        {
            conn.LogInfo(args[0] + "::Run()");

            var timer = new Stopwatch();
            timer.Start();

            try
            {
                switch (cmd)
                {
                    case UnityCommand.UCOM_Config:
                        return RunCommand<TfsConfigCommand, ConfigRequest, ConfigResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Add:
                        return RunCommand<TfsAddCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Move:
                        return RunCommand<TfsMoveCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_ChangeMove:
                        return RunCommand<TfsMoveChangelistCommand, MoveChangelistRequest, MoveChangelistResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Delete:
                        return RunCommand<TfsDeleteCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_GetLatest:
                        return RunCommand<TfsGetLatestCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Resolve:
                        return RunCommand<TfsResolveCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Lock:
                        return RunCommand<TfsLockCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Unlock:
                        return RunCommand<TfsUnlockCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Revert:
                        return RunCommand<TfsRevertCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_RevertChanges:
                        return RunCommand<TfsRevertChangesCommand, RevertChangesRequest, RevertChangesResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Submit:
                        return RunCommand<TfsSubmitCommand, SubmitRequest, SubmitResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Download:
                        return RunCommand<TfsDownloadCommand, DownloadRequest, DownloadResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_ChangeDescription:
                        return RunCommand<TfsChangeDescriptionCommand, ChangeDescriptionRequest, ChangeDescriptionResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Status:
                        return RunCommand<TfsStatusCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Incoming:
                        return RunCommand<TfsIncomingCommand, IncomingRequest, IncomingResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_IncomingChangeAssets:
                        return RunCommand<TfsIncomingAssetsCommand, IncomingAssetsRequest, IncomingAssetsResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Changes:
                        return RunCommand<TfsOutgoingCommand, OutgoingRequest, OutgoingResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_ChangeStatus:
                        return RunCommand<TfsChangeStatusCommand, OutgoingAssetsRequest, OutgoingAssetsResponse, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_Checkout:
                        return RunCommand<TfsCheckoutCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    case UnityCommand.UCOM_FileMode:
                        return RunCommand<TfsFileModeCommand, BaseFileSetRequest, BaseFileSetResponse<BaseFileSetRequest>, TfsTask>(conn, session, args);
                    default:
                        break;
                }
            }
            finally
            {
                conn.LogDebug("\n------------ " + timer.Elapsed.ToString() + " --------------\n", false);
            }

            return false;
        }

        public static bool RunCommand<Cmd, Req, Resp, Sess>(Connection conn, Sess session, CommandArgs args)

            where Req : BaseRequest
            where Resp : BaseResponse
            where Cmd : ICommand<Sess, Req, Resp>, new()
        {
            Req req = (Req)Activator.CreateInstance(typeof(Req), args, conn);

            // Invalid requests will send warnings/error to unity and we can just continue.
            // If something fatal happens an exception is thrown when reading the request.
            if (req.Invalid) return true;

            Resp resp = (Resp)Activator.CreateInstance(typeof(Resp), req);

            Cmd found = new Cmd();

            try
            {
                return found.Run(session, req, resp);
            }
            catch (Exception ex)
            {
                conn.WarnLine(ex.Message);
                conn.Log(ex.ToString());

                resp.Write();
            }

            return false;
        }

        public void SetProjectPath(string val)
        {
            projectPath = val;

            // need to reset online flag to allow unity reconnecting
            isOnline = null;

            // need to reset project specific settings
            lockLevel = new Lazy<LockLevel>(InitializeLockLevel);
            shareBinaryFileCheckout = new Lazy<bool>(InitializeShareBinaryFileCheckout);
            nolockRegexCache = new Lazy<List<Regex>>(InitializeNolockFiles);
            ignoreRegexCache = new Lazy<List<Regex>>(InitializeIgnoreFiles);
        }

        private bool InitializeShareBinaryFileCheckout()
        {
            try
            {
                var settingValue = FindBestMatchForProjectSetting(Settings.Default.ShareBinaryFileCheckout);

                bool parsedValue;
                if (settingValue != null && bool.TryParse(settingValue, out parsedValue))
                {
                    return parsedValue;
                }
            }
            catch
            {

            }

            return false;
        }

        private LockLevel InitializeLockLevel()
        {
            LockLevel result = LockLevel.CheckOut;

            try
            {
                var settingValue = FindBestMatchForProjectSetting(Settings.Default.LockLevels);

                LockLevel parsedLevel;
                if (settingValue != null && Enum.TryParse<LockLevel>(settingValue, true, out parsedLevel))
                {
                    result = parsedLevel;
                }
            }
            catch
            {

            }

            if (Workspace.Location == Microsoft.TeamFoundation.VersionControl.Common.WorkspaceLocation.Local && result == LockLevel.CheckOut)
            {
                result = LockLevel.Checkin;
            }

            return result;
        }

        string FindBestMatchForProjectSetting(string setting)
        {
            if (string.IsNullOrEmpty(setting))
            {
                return null;
            }

            var split = setting.Split('|');

            var path = Workspace.TryGetServerItemForLocalItem(projectPath);

            if (string.IsNullOrEmpty(path))
            {
                return null;
            }

            var bestMatch = split
                .Select(each => each.Split(new[] { ':' }, 2))
                .Where(each => each != null && each.Length == 2 && path.StartsWith(each[0], StringComparison.CurrentCultureIgnoreCase))
                .OrderByDescending(each => each[0].Length)
                .FirstOrDefault();

            if (bestMatch == null)
            {
                return null;
            }

            return bestMatch[1];
        }

        public VersionedAssetList GetStatus()
        {
            VersionedAssetList result = new VersionedAssetList();

            var projectFolder = new VersionedAsset(GetAssetPath(projectPath, Workspace.GetServerItemForLocalItem(projectPath), ItemType.Folder, Workspace));

            GetStatus(new VersionedAssetList { projectFolder }, result, true, true);

            return result;
        }

        public void GetStatus(IEnumerable<VersionedAsset> assets, VersionedAssetList result, bool recursive, bool queryRemote)
        {
            if (string.IsNullOrEmpty(tpcPath))
            {
                m_Connection.LogInfo("Status requested without server.");
            }

            GetStatus(assets.Select(each => each.GetPath()).ToArray(), result, recursive, queryRemote);
        }


        internal void GetStatus(string[] assets, VersionedAssetList result, bool recursive, bool queryRemote)
        {
            var timer = new Stopwatch();
            timer.Start();
            var time = DateTime.Now;

            // if we are not connected to tfs 
            if (this.Workspace == null)
            {
                return;
            }

            if (recursive)
            {
                if (assets.Length == 0)
                {
                    // Empty means all in recursive mode

                    var status = GetProjectExtendedStatus();

                    tfsFileStatusCache.RefreshExtendedStatus(status, Workspace, time);
                    ToVersionedAssetList(status, result);
                }
                else
                {
                    var status = GetProjectExtendedStatus(assets, true);
                    tfsFileStatusCache.RefreshExtendedStatus(status, Workspace, time);
                    ToVersionedAssetList(status, result);
                }
            }
            else
            {
                if (assets.Length != 0)
                {
                    var status = GetProjectExtendedStatus(assets, false);
                    tfsFileStatusCache.RefreshExtendedStatus(status, Workspace, time);
                    ToVersionedAssetList(status, result);
                }
            }

            // Make sure that we have status for local only assets as well
            foreach (var i in assets)
            {
                if (i.Contains("*"))
                {
                    continue;
                }

                if (!result.Any(each => each.GetPath().Equals(i, StringComparison.CurrentCultureIgnoreCase)))
                {
                    result.Add(new VersionedAsset(i, State.kLocal, "0"));
                }
            }


            timer.Restart();
            MergeInMoreDetailedPendingChanges(result, recursive);

            if (IsPartiallyOffline)
            {
                MergeInWorkLocalStatus(result);
            }
        }

        /// <summary>
        /// a user can manually make a file not readonly to notify unity it is "checked out" locally and can be modified.
        /// </summary>
        /// <param name="result"></param>
        private void MergeInWorkLocalStatus(VersionedAssetList result)
        {
            foreach (var item in result)
            {
                if (!item.HasPendingLocalChange && item.IsKnownByVersionControl && item.IsWritable)
                {
                    item.AddState(State.kOutOfSync | State.kCheckedOutLocal);
                }
            }
        }

        internal void MergeInMoreDetailedPendingChanges(VersionedAssetList result, bool isRecursive)
        {
            if (result.Count == 0)
            {
                return;
            }

            var conflicts = GetConflicts(result);

            foreach (var item in conflicts)
            {
                var found = FindVersionedAsset(item, result);

                if (found != null)
                {
                    found.AddState(State.kConflicted);
                    found.RemoveState(State.kSynced);
                    found.AddState(State.kOutOfSync);
                }
            }

            if (Workspace.Location == Microsoft.TeamFoundation.VersionControl.Common.WorkspaceLocation.Server)
            {
                var time = DateTime.Now;
                var pending = GetProjectPendingSets(result, isRecursive);

                var localPending = pending.Where(item => item.Type == PendingSetType.Workspace && item.OwnerName == Workspace.OwnerName && item.Name == Workspace.Name).ToArray();
                var remotePending = pending.Where(item => item.Type == PendingSetType.Workspace && !(item.OwnerName == Workspace.OwnerName && item.Name == Workspace.Name)).ToArray();

                TfsFileStatusCache.RefreshPendingChanges(result.Select(each => each.GetPath()), Workspace, remotePending, localPending, time);

                foreach (var item in pending)
                {
                    if (item.Type != PendingSetType.Workspace)
                    {
                        continue;
                    }

                    if (item.OwnerName == Workspace.OwnerName && item.Name == Workspace.Name)
                    {
                        ApplyStatusFromPendingSet(result, item, true);
                    }
                    else
                    {
                        ApplyStatusFromPendingSet(result, item, false);
                    }
                }
            }
        }

        private void ApplyStatusFromPendingSet(VersionedAssetList result, PendingSet item, bool isLocal)
        {
            // each local pending change
            foreach (var eachPendingChange in item.PendingChanges)
            {
                var asset = FindVersionedAsset(eachPendingChange, result);

                if (asset == null)
                {
                    continue;
                }

                if (eachPendingChange.IsLock)
                {
                    asset.AddState(isLocal ? State.kLockedLocal : State.kLockedRemote);
                }

                if (eachPendingChange.IsAdd)
                {
                    asset.AddState(isLocal ? State.kAddedLocal : State.kAddedRemote);
                }
                else if (eachPendingChange.IsDelete)
                {
                    asset.AddState(isLocal ? State.kDeletedLocal : State.kDeletedRemote);

                    if (isLocal)
                    {
                        asset.RemoveState(State.kMissing);
                        asset.AddState(State.kSynced);
                    }
                }
                else if (eachPendingChange.IsEdit || eachPendingChange.IsRename || eachPendingChange.IsBranch || eachPendingChange.IsMerge || eachPendingChange.IsRollback || eachPendingChange.IsUndelete || eachPendingChange.IsEncoding)
                {
                    asset.AddState(isLocal ? State.kCheckedOutLocal : State.kCheckedOutRemote);
                }
            }
        }

        void ApplyStatusFromExtendedItem(VersionedAsset asset, ExtendedItem eachItem)
        {
            if (asset == null)
            {
                return;
            }

            var isLocal = true;

            if (eachItem.ChangeType.HasFlag(ChangeType.Lock))
            {
                asset.AddState(isLocal ? State.kLockedLocal : State.kLockedRemote);
            }

            if (eachItem.ChangeType.HasFlag(ChangeType.Add))
            {
                asset.AddState(isLocal ? State.kAddedLocal : State.kAddedRemote);
            }
            else if (eachItem.ChangeType.HasFlag(ChangeType.Delete))
            {
                asset.AddState(isLocal ? State.kDeletedLocal : State.kDeletedRemote);

                if (isLocal)
                {
                    asset.RemoveState(State.kMissing);
                    asset.AddState(State.kSynced);
                }
            }
            else if (eachItem.ChangeType.HasFlag(ChangeType.Edit) || eachItem.ChangeType.HasFlag(ChangeType.Rename) || eachItem.ChangeType.HasFlag(ChangeType.Branch) || eachItem.ChangeType.HasFlag(ChangeType.Merge) || eachItem.ChangeType.HasFlag(ChangeType.Rollback) || eachItem.ChangeType.HasFlag(ChangeType.Undelete) || eachItem.ChangeType.HasFlag(ChangeType.Encoding))
            {
                asset.AddState(isLocal ? State.kCheckedOutLocal : State.kCheckedOutRemote);
            }
        }

        public VersionedAssetList ToVersionedAssetList(Changeset changes)
        {
            return new VersionedAssetList(changes.Changes.Select(each => new VersionedAsset(GetAssetPath(each, Workspace))));
        }

        public VersionedAssetList ToVersionedAssetList(PendingChange[] changes, bool useSourceItemWhenRename)
        {
            return new VersionedAssetList(changes.Select(each => new VersionedAsset(GetAssetPath(each, Workspace, useSourceItemWhenRename))));
        }

        public VersionedAsset ToVersionedAsset(ExtendedItem eachItem)
        {
            VersionedAsset asset = new VersionedAsset(GetAssetPath(eachItem, Workspace));
            asset.SetChangeListID(eachItem.VersionLocal.ToString());
            asset.SetRevision(eachItem.VersionLocal.ToString());

            if (asset.ExistsOnDisk)
            {
                asset.AddState(State.kLocal);
            }
            else
            {
                asset.AddState(State.kMissing);
            }

            if (eachItem.IsInWorkspace)
            {
                if (eachItem.IsLatest)
                {
                    asset.AddState(State.kSynced);
                }
                else
                {
                    asset.AddState(State.kOutOfSync);
                }
            }

            if (Workspace.Location == Microsoft.TeamFoundation.VersionControl.Common.WorkspaceLocation.Local)
            {
                ApplyStatusFromExtendedItem(asset, eachItem);
            }

            return asset;
        }


        public bool IsIgnored(VersionedAsset asset)
        {
            foreach (var item in ignoreRegexCache.Value)
            {
                if (item.IsMatch(asset.GetPath()))
                {
                    return true;
                }
            }

            return false;
        }

        private bool IsNoLocked(VersionedAsset asset)
        {
            foreach (var item in nolockRegexCache.Value)
            {
                if (item.IsMatch(asset.GetPath()))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// replace paths to a folder and its meta file, with a wildcard path that TFS will resolve to all of that folders children one level deep
        /// </summary>
        /// <param name="assets"></param>
        public static void ReplaceFolderPathsWithChildrenWildcard(List<string> assets)
        {
            foreach (var item in assets.ToArray())
            {
                if (IsFolder(item))
                {
                    assets.Remove(item);
                    assets.Remove(TfsTask.GetMeta(item));
                    assets.Add(item + "*");
                }
            }
        }

        /// <summary>
        /// for each folder in the given assets, returns a wildcard path that will resolve to the folders children one level deep
        /// </summary>
        /// <param name="assets"></param>
        /// <returns></returns>
        public static IEnumerable<string> GetFolderContentsWildcardPaths(IEnumerable<string> assets)
        {
            foreach (var item in assets)
            {
                if (IsFolder(item))
                {
                    yield return item + "*";
                }
            }
        }

        public static bool IsFolder(VersionedAsset asset)
        {
            return IsFolder(asset.Path);
        }

        public static bool IsFolder(string asset)
        {
            return asset.EndsWith("/", StringComparison.CurrentCultureIgnoreCase);
        }

        public static bool IsMeta(VersionedAsset asset)
        {
            return asset.Path.EndsWith(".meta", StringComparison.CurrentCultureIgnoreCase);
        }
        public static string GetMeta(string asset)
        {
            return asset.TrimEnd('/') + ".meta";
        }

        public static bool IsDescendant(VersionedAsset parent, VersionedAsset desc)
        {
            return desc.Path.StartsWith(parent.Path, StringComparison.CurrentCultureIgnoreCase);
        }

        void ToVersionedAssetList(ExtendedItem[][] items, VersionedAssetList result)
        {
            foreach (var item in items)
            {
                foreach (var eachItem in item)
                {
                    VersionedAsset asset = ToVersionedAsset(eachItem);

                    result.Add(asset);
                }
            }
        }

        VersionedAsset FindVersionedAsset(PendingChange item, VersionedAssetList list)
        {
            var path = GetAssetPath(item, Workspace, false);

            return list.FirstOrDefault(each => each.Path.Equals(path, StringComparison.CurrentCultureIgnoreCase));
        }

        VersionedAsset FindVersionedAsset(Conflict item, VersionedAssetList list)
        {
            var path = GetAssetPath(item, Workspace);

            return list.FirstOrDefault(each => each.Path.Equals(path, StringComparison.CurrentCultureIgnoreCase));
        }

        public static string GetAssetPath(PendingChange item, Workspace workspace, bool useSourceItemWhenRename)
        {
            return GetAssetPath(null, (item.IsRename && useSourceItemWhenRename) ? item.SourceServerItem : item.ServerItem, item.ItemType, workspace);
        }

        public static string GetAssetPath(Conflict item, Workspace workspace)
        {
            return GetAssetPath(item.LocalPath, item.BaseServerItem, item.BaseItemType, workspace);
        }

        public static string GetAssetPath(Change item, Workspace workspace)
        {
            return GetAssetPath(null, item.Item.ServerItem, item.Item.ItemType, workspace);
        }

        public static string GetAssetPath(string localItem, string sourceServerItem, ItemType itemType, Workspace workspace)
        {
            var result = string.Empty;

            if (localItem != null)
            {
                result = localItem.Replace('\\', '/');
            }
            else
            {
                // this can happen if the item is open for delete
                result = workspace.GetLocalItemForServerItem(sourceServerItem).Replace('\\', '/');

            }

            if (itemType == ItemType.Folder)
                result += "/";

            return result;
        }

        public static string GetAssetPath(ExtendedItem item, Workspace workspace)
        {
            return GetAssetPath(item.LocalItem, item.SourceServerItem, item.ItemType, workspace);
        }

        public static Changelist ConvertToChangelist(Changeset cs)
        {
            return new Changelist
            {
                Committer = cs.Committer,
                Description = cs.Comment.Replace("\r\n", " ").Replace("\n", " "),
                Revision = new ChangelistRevision(cs.ChangesetId.ToString()),
                Timestamp = cs.CreationDate.ToString()
            };
        }

        public ChangesetData GetChangesetWithChanges(int id)
        {
            ChangesetData found;

            if (!changesetCache.TryGetValue(id, out found))
            {
                var cs = VersionControlServer.GetChangeset(id, true, false);

                found = new ChangesetData(cs.Comment, ToVersionedAssetList(cs));
                changesetCache[id] = found;
            }
            else if (found.Changes == null)
            {
                // if the cache was populated with a changeset without changes, fill it  now

                var cs = VersionControlServer.GetChangeset(id, true, false);

                found.Changes = ToVersionedAssetList(cs);
            }

            return found;
        }

        public LockLevel GetLockLevel()
        {
            return lockLevel.Value;
        }

        public bool ShouldLock(VersionedAsset asset)
        {
            return asset.IsOfType(exclusiveCheckoutTypes) && !IsNoLocked(asset);
        }

        public void SplitByLockPolicy(IEnumerable<VersionedAsset> assets, out string[] needLocks, out string[] noLocks)
        {
            List<string> toLock = new List<string>();
            List<string> notToLock = new List<string>();

            foreach (var item in assets)
            {
                if (ShouldLock(item))
                {
                    toLock.Add(Path.GetFullPath(item.GetPath()));
                }
                else
                {
                    notToLock.Add(Path.GetFullPath(item.GetPath()));
                }
            }

            // check the currently lock status of binary files, if they are checked out, or have a checkin lock, dont lock them
            if (toLock.Count != 0 && shareBinaryFileCheckout.Value)
            {
                var lockLevel = GetLockLevel();

                List<TfsFileStatus> status = new List<TfsFileStatus>();
                TfsFileStatusCache.RefreshExtendedStatus(Workspace, toLock, status);

                foreach (var item in status)
                {
                    if (!item.ExtendedItem.HasOtherPendingChange)
                    {
                        continue;
                    }

                    if (lockLevel == LockLevel.Checkin && item.ExtendedItem.LockStatus == LockLevel.None)
                    {
                        continue;
                    }

                    var fullpath = item.LocalFullPath;
                    toLock.RemoveAll(each => each.Equals(fullpath, StringComparison.CurrentCultureIgnoreCase));
                    notToLock.Add(fullpath);

                }
            }

            needLocks = toLock.ToArray();
            noLocks = notToLock.ToArray();
        }

        internal List<ChangesetData> GetChangesets(HashSet<int> changeSets)
        {
            List<ChangesetData> result = new List<ChangesetData>(changeSets.Count);
            HashSet<int> changeSetsCopy = new HashSet<int>(changeSets);

            foreach (var item in changeSets)
            {
                ChangesetData found;
                if (changesetCache.TryGetValue(item, out found))
                {
                    result.Add(found);
                    changeSetsCopy.Remove(item);
                }
            }

            if (changeSetsCopy.Count == 0)
            {
                return result;
            }

            var min = new ChangesetVersionSpec(changeSetsCopy.Min());
            var max = new ChangesetVersionSpec(changeSetsCopy.Max());

            var changes = VersionControlServer.QueryHistory(ProjectPath, VersionSpec.Latest, 0, RecursionType.Full, null, min, max, Int32.MaxValue, false, false);

            foreach (Changeset item in changes)
            {
                ChangesetData found;
                if (!changesetCache.TryGetValue(item.ChangesetId, out found))
                {
                    found = new ChangesetData(item.Comment, null);
                    changesetCache[item.ChangesetId] = found;
                }

                if (changeSetsCopy.Contains(item.ChangesetId))
                {
                    result.Add(found);
                }
            }

            return result;
        }

        internal PendingSet[] GetProjectPendingSets(VersionedAssetList versionedAssetList, bool isRecursive = true)
        {
            var list = versionedAssetList.Select(each => each.GetPath()).ToArray();
            return VersionControlServer.GetPendingSets(list, isRecursive ? RecursionType.Full : RecursionType.None);
        }

        internal PendingChange[] GetProjectPendingChanges(VersionedAssetList versionedAssetList)
        {
            return Workspace.GetPendingChanges(versionedAssetList.Select(each => each.GetPath()).ToArray());
        }

        internal PendingChange[] GetProjectPendingChanges()
        {
            return Workspace.GetPendingChanges(ProjectPath, RecursionType.Full);
        }

        public static bool PendingChangesAreTheSame(PendingChange[] a1, PendingChange[] a2)
        {
            if (a1.Length == a2.Length)
            {
                for (int i = 0; i < a1.Length; i++)
                {
                    if (a1[i].FileName != a2[i].FileName || a1[i].LocalItem != a2[i].LocalItem || a1[i].Version != a2[i].Version || a1[i].ChangeType != a2[i].ChangeType)
                    {
                        return false;
                    }
                }
                return true;
            }
            return false;
        }



        internal ExtendedItem[][] GetProjectExtendedStatus(string[] paths, bool resursive)
        {
            var itemSpecs = paths.Select(each => new ItemSpec(each, resursive && IsFolder(each) ? RecursionType.Full : RecursionType.None)).ToArray();
            return Workspace.GetExtendedItems(itemSpecs, DeletedState.Any, ItemType.Any, GetItemsOptions.None);
        }

        internal ExtendedItem[][] GetProjectExtendedStatus()
        {
            return Workspace.GetExtendedItems(new[] { new ItemSpec(ProjectPath, RecursionType.Full) }, DeletedState.Any, ItemType.Any, GetItemsOptions.None);
        }

        internal Conflict[] GetConflicts(VersionedAssetList versionedAssetList)
        {
            var conflicts = Workspace.QueryConflicts(versionedAssetList.Select(each => each.GetPath()).ToArray(), true);

            return conflicts;
        }

        internal void SetTfsServer(string val)
        {
            this.tpcPath = val;

            this.changesetCache = new Dictionary<int, ChangesetData>();
            this.projectCollection = null;
            this.workspace = null;
            this.identity = null;
            this.isOnline = null;
        }

        public string ProjectName
        {
            get
            {
                return Path.GetFileName(ProjectPath);
            }
        }


        public void ShowWarningMessageBox(string text, string caption, bool blocking = false)
        {
            if (string.IsNullOrEmpty(text))
            {
                return;
            }

            if (WarningDialogDisplayed != null)
            {
                WarningDialogDisplayed(text);
            }

            foreach (var item in Process.GetProcessesByName("Unity"))
            {
                if (item.MainWindowTitle.Contains(string.Format(" {0} ", this.ProjectName)))
                {
                    if (blocking)
                    {
                        NativeMethods.MessageBox(item.MainWindowHandle, text, caption, MB_ICONEXCLAMATION | MB_OK);
                    }
                    else
                    {
                        ThreadPool.QueueUserWorkItem((obj) => NativeMethods.MessageBox(item.MainWindowHandle, text, caption, MB_ICONEXCLAMATION | MB_OK));
                    }
                    break;
                }
            }
        }

        public void WarnAboutFailureToPendChange(IEnumerable<VersionedAsset> asset)
        {
            if (asset.Any() == false)
            {
                return;
            }

            var caption = "Version Control Warning";
            var messageHeader = "Unable to checkout to the following files";
            string footer = string.Empty;

            var assets = TfsFileStatusCache.Find(asset.Select(each => each.GetPath()));

            if (assets.Any(each => each.IsRemoteLocked || each.IsRemoteCheckedOut))
            {
                footer = string.Format("Only one user can hold a lock for check-in at a time, locks protect the lock owners work on non-mergable files ({0}).", ExclusiveCheckoutTypesToString());
            }

            ShowWarningMessageBox(assets, caption, messageHeader, footer);
        }

        public void WarnAboutOutOfDateFiles(IEnumerable<VersionedAsset> asset)
        {
            var assets = TfsFileStatusCache.Find(asset.Select(each => each.GetPath())).Where(each => each.IsLocalLatest == false);

            if (assets.Any() == false)
            {
                return;
            }

            var caption = "Version Control Warning";
            var messageHeader = "You do not have the latest versions of the following files";
            string footer = string.Empty;

            footer = string.Format("Changes made to a non-mergable file ({0}) that is not the latest version will not be able to be checked in, resulting in lost work.", ExclusiveCheckoutTypesToString());

            ShowWarningMessageBox(assets, caption, messageHeader, footer);
        }

        public void UserWantsToGoPartiallyOfflineFor(IEnumerable<VersionedAsset> asset)
        {
            if (asset.Any() == false)
            {
                return;
            }

            var assets = TfsFileStatusCache.Find(asset.Select(each => each.GetPath()));

            var caption = "Working Partially Offline";
            string footer = "Be sure to resolve your offline modifications with TFS when finished.";

            var messageHeader = string.Format("You are now working with the following files offline, they can be edited and will appear checked out.");

            ShowWarningMessageBox(assets, caption, messageHeader, footer);
        }

        public void WarnAboutBinaryFilesCheckedOutByOthers(IEnumerable<VersionedAsset> asset)
        {
            var assets = TfsFileStatusCache.Find(asset.Where(each => ShouldLock(each)).Select(each => each.GetPath())).Where(each => (each.IsRemoteLocked || each.IsRemoteCheckedOut));

            if (assets.Any() == false)
            {
                return;
            }

            var caption = "Version Control Warning";
            var messageHeader = "Other users are also working on the same non-mergable files";
            string footer = string.Empty;

            footer = string.Format("The first user to submit their change to a non-mergable file ({0}) will prevent all other users who had the file checked out from submitting their change, resulting in lost work", ExclusiveCheckoutTypesToString());

            ShowWarningMessageBox(assets, caption, messageHeader, footer);
        }

        private void ShowWarningMessageBox(IEnumerable<TfsFileStatus> assets, string caption, string messageHeader, string footer)
        {
            var msgLines = string.Join(Environment.NewLine, assets.Select(item => item.PartialPath(projectPath) + Environment.NewLine + item.ToRemoteStatusString() + Environment.NewLine));

            const int MaxMessageLength = 2000;

            if (msgLines.Length > MaxMessageLength)
            {
                msgLines = msgLines.Substring(0, MaxMessageLength) + "...";
            }

            var msg = messageHeader + Environment.NewLine + Environment.NewLine + msgLines + Environment.NewLine + footer;

            ShowWarningMessageBox(msg, caption);
        }

        void RunTfptScortchOnProject()
        {
            if (projectPath == null || Directory.Exists(projectPath) == false)
            {
                return;
            }

            var processPath = "tfpt";
            var command = "scorch";
            var path = Path.Combine(projectPath, "Assets");
            var args = "/diff /recursive";
            Process.Start(new ProcessStartInfo { FileName = processPath, Arguments = string.Format(@"{0} ""{1}"" {2}", command, path, args) });
        }

        internal void SetPartialOffline(string val)
        {
            var newValue = !string.IsNullOrEmpty(val) && (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase) || val.Trim().Equals("yes", StringComparison.OrdinalIgnoreCase) || val.Trim().Equals("y", StringComparison.OrdinalIgnoreCase));

            if (newValue != IsPartiallyOffline)
            {
                IsPartiallyOffline = newValue;

                if (IsPartiallyOffline == false)
                {
                    RunTfptScortchOnProject();
                }
            }
        }
    }
}
