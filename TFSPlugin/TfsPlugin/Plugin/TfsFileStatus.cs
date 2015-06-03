using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace TfsPlugin
{
    public class TfsFileStatusCache
    {
        ConcurrentDictionary<string, TfsFileStatus> cache = new ConcurrentDictionary<string, TfsFileStatus>(StringComparer.CurrentCultureIgnoreCase);

        public TfsFileStatusCache()
        {
        }

        public TfsFileStatus Find(string localPath)
        {
            TfsFileStatus result;
            if (cache.TryGetValue(localPath, out result))
            {
                return result;
            }
            return null;
        }

        public void Clear()
        {
            cache.Clear();
        }

        public IEnumerable<TfsFileStatus> Find(IEnumerable<string> localPaths)
        {
            foreach (var item in localPaths)
            {
                TfsFileStatus result;
                if (cache.TryGetValue(item, out result))
                {
                    yield return result;
                }
            }
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

        public void RefreshExtendedStatus(Workspace ws, IEnumerable<string> localPaths, List<TfsFileStatus> updated = null)
        {
            var time = DateTime.Now;
            var items = ws.GetExtendedItems(localPaths.Select(each => new ItemSpec(each, RecursionType.None)).ToArray(), DeletedState.Any, ItemType.Any);

            RefreshExtendedStatus(items, ws, time, updated);
        }

        public void RefreshExtendedStatus(ExtendedItem[][] status, Workspace ws, DateTime time, List<TfsFileStatus> updated = null)
        {
            foreach (var item in status)
            {
                foreach (var eachItem in item)
                {
                    var assetPath = GetAssetPath(eachItem.LocalItem, eachItem.SourceServerItem, eachItem.ItemType, ws);
                    TfsFileStatus result;
                    if (cache.TryGetValue(assetPath, out result))
                    {
                        result.RefreshExtendedItem(eachItem, time);
                        if (updated != null)
                            updated.Add(result);
                    }
                    else
                    {

                        result = new TfsFileStatus(eachItem.SourceServerItem, assetPath);
                        result.RefreshExtendedItem(eachItem, time);
                        cache[assetPath] = result;
                        if (updated != null)
                            updated.Add(result);
                    }
                }
            }
        }



        public void RefreshPendingChanges(IEnumerable<string> localPaths, Workspace ws, PendingSet[] remote, PendingSet[] local, DateTime time)
        {
            foreach (var item in localPaths)
            {
                TfsFileStatus result;
                if (cache.TryGetValue(item, out result))
                {
                    result.RefreshPendingChanges(remote, local, time);
                }
                else
                {
                    var serverPath = ws.GetServerItemForLocalItem(item);
                    result = new TfsFileStatus(serverPath, item);
                    result.RefreshPendingChanges(remote, local, time);
                    cache[item] = result;
                }
            }
        }
    }

    public class TfsFileStatus
    {
        public string ServerPath { get; private set; }
        public string LocalPath { get; set; }
        public string LocalFullPath { get { return Path.GetFullPath(LocalPath); } }

        PendingSet[] remotePending;
        PendingSet[] localPending;
        public DateTime PendingChangeTimestamp { get; set; }

        ExtendedItem extendedItem;
        public DateTime ExtendedItemTimestamp { get; set; }
        public ExtendedItem ExtendedItem { get { return extendedItem; } }

        public bool IsRemoteLockedCheckout { get { return RemotePendingChanges.Any(each => each.Item2.IsLock && each.Item2.LockLevel == LockLevel.CheckOut); } }
        public bool IsRemoteLockedCheckin { get { return RemotePendingChanges.Any(each => each.Item2.IsLock && each.Item2.LockLevel == LockLevel.Checkin); } }
        public bool IsRemoteLocked { get { return RemotePendingChanges.Any(each => each.Item2.IsLock); } }
        public bool IsLocalLatest { get { return extendedItem != null ? extendedItem.IsLatest : true; } }
        public bool IsRemoteCheckedOut { get { return RemotePendingChanges.Any(each => each.Item2.ChangeType != ChangeType.None); } }
        public bool IsLocalCheckedOut { get { return extendedItem.ChangeType != ChangeType.None; } }
        public ChangeType LocalChangeType { get { return extendedItem.ChangeType; } }

        public TfsFileStatus(string serverPath, string localPath)
        {
            this.ServerPath = serverPath;
            this.LocalPath = localPath;
        }

        public static TfsFileStatus CreateFromLocalPath(Workspace ws, string localPath)
        {
            var items = ws.GetExtendedItems(new[] { new ItemSpec(localPath, RecursionType.None) }, DeletedState.Any, ItemType.Any);

            foreach (var item in items)
            {
                foreach (var each in item)
                {
                    var result = new TfsFileStatus(each.SourceServerItem, localPath);
                    result.RefreshExtendedItem(each, DateTime.Now);
                    return result;
                }
            }

            return null;
        }

        public void RefreshExtendedItem(Workspace ws)
        {
            var items = ws.GetExtendedItems(new[] { new ItemSpec(LocalPath, RecursionType.None) }, DeletedState.Any, ItemType.Any);

            foreach (var item in items)
            {
                foreach (var each in item)
                {
                    extendedItem = each;
                    ExtendedItemTimestamp = DateTime.Now;
                    return;
                }
            }
        }

        public void RefreshExtendedItem(ExtendedItem item, DateTime time)
        {
            this.ExtendedItemTimestamp = time;
            this.extendedItem = item;
        }

        public string PartialPath(string projectPath)
        {
            return Regex.Replace(LocalPath, projectPath.Replace("\\", "/"), "", RegexOptions.IgnoreCase);
        }

        public void RefreshPendingChanges(PendingSet[] remote, PendingSet[] local, DateTime time)
        {
            this.PendingChangeTimestamp = time;
            this.remotePending = remote;
            this.localPending = local;
        }

        public string ToRemoteStatusString()
        {
            var lockedBy = RemotePendingChanges.Where(each => each.Item2.IsLock);
            HashSet<string> checkedOutBy = new HashSet<string>(RemotePendingChanges.Where(each => !each.Item2.IsLock).Select(each => each.Item1.OwnerDisplayName));

            string result = string.Empty;

            foreach (var item in lockedBy)
            {
                result += " *Locked for " + item.Item2.LockLevelName + " by " + item.Item1.OwnerDisplayName + "* ";
            }

            if (checkedOutBy.Count != 0)
            {
                result += " *Checked out by " + string.Join(", ", checkedOutBy) + "* ";
            }

            return result;
        }

        IEnumerable<Tuple<PendingSet, PendingChange>> LocalPendingChanges
        {
            get
            {
                foreach (var item in localPending)
                {
                    foreach (var eachPending in item.PendingChanges)
                    {
                        if (IsOurChange(eachPending))
                        {
                            yield return Tuple.Create(item, eachPending);
                        }
                    }
                }
            }
        }

        private bool IsOurChange(PendingChange eachPending)
        {
            if (eachPending.IsLocalItemDelete)
            {
                return eachPending.SourceServerItem.Equals(this.ServerPath, StringComparison.CurrentCultureIgnoreCase);
            }
            else if (eachPending.IsRename)
            {
                return eachPending.SourceServerItem.Equals(this.ServerPath, StringComparison.CurrentCultureIgnoreCase);
            }
            else if (eachPending.LocalItem != null)
            {
                return eachPending.ServerItem.Equals(this.ServerPath, StringComparison.CurrentCultureIgnoreCase);
            }
            return false;
        }

        IEnumerable<Tuple<PendingSet, PendingChange>> RemotePendingChanges
        {
            get
            {
                foreach (var item in remotePending)
                {
                    foreach (var eachPending in item.PendingChanges)
                    {
                        if (IsOurChange(eachPending))
                        {
                            yield return Tuple.Create(item, eachPending);
                        }
                    }
                }
            }
        }
    }
}
