using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace TfsPlugin
{
    [Flags]
    public enum State
    {
        kNone = 0,
        kLocal = 1 << 0, // Asset in on local disk. If none of the sync flags below are set it means vcs doesn't know about this local file.
        kSynced = 1 << 1, // Asset is known to vcs and in sync locally
        kOutOfSync = 1 << 2, // Asset is known to vcs and outofsync locally
        kMissing = 1 << 3,
        kCheckedOutLocal = 1 << 4,
        kCheckedOutRemote = 1 << 5,
        kDeletedLocal = 1 << 6,
        kDeletedRemote = 1 << 7,
        kAddedLocal = 1 << 8,
        kAddedRemote = 1 << 9,
        kConflicted = 1 << 10,
        kLockedLocal = 1 << 11,
        kLockedRemote = 1 << 12,
        kUpdating = 1 << 13,
        kReadOnly = 1 << 14,
        kMetaFile = 1 << 15,
        kMovedLocal = 1 << 16,
        kMovedRemote = 1 << 17,
    };

    public class VersionedAsset : IComparable<VersionedAsset>, IWritesToConnection, IReadsFromConnection, IEquatable<VersionedAsset>
    {
        public VersionedAsset()
        {
            m_State = State.kNone;
            SetPath("");
        }
        public VersionedAsset(string path)
        {
            m_State = State.kNone;
            SetPath(path);
        }
        public VersionedAsset(string path, State state, string revision = "")
        {
            m_State = state;
            m_Revision = revision;
            SetPath(path);
        }

        public override string ToString()
        {
            return System.IO.Path.GetFileName(Path);
        }
        public State GetState() { return m_State; }
        public void SetState(State newState) { m_State = newState; }
        public void AddState(State state) { m_State |= state; }
        public void RemoveState(State state) { m_State &= ~state; }
        public bool HasState(State state) { return m_State.HasFlag(state); }

        public bool IsKnownByVersionControl
        {
            get
            {
                return HasState(State.kSynced) || HasState(State.kOutOfSync);
            }
        }

        public bool HasPendingLocalChange
        {
            get
            {
                return HasState(State.kAddedLocal) || HasState(State.kCheckedOutLocal) || HasState(State.kDeletedLocal) || HasState(State.kMovedLocal) || HasState(State.kConflicted);
            }
        }

        public string Path
        {
            get
            {
                return m_Path;
            }
        }

        public string GetPath() { return m_Path; }
        public void SetPath(string path)
        {
            m_Path = path;

            if (path.Length > 5 && path.Substring(path.Length - 5, 5) == ".meta")
                AddState(State.kMetaFile);
        }
        public string GetMovedPath() { return m_MovedPath; }
        public void SetMovedPath(string path) { m_MovedPath = path; }
        
        public string GetRevision() { return m_Revision; }
        public void SetRevision(string r) { m_Revision = r; }
        public string GetChangeListID() { return m_ChangeListID; }
        public void SetChangeListID(string c) { m_ChangeListID = c; }

        public void Reset()
        {
            m_State = State.kNone;
            SetPath("");
            m_MovedPath = string.Empty;
            m_Revision = string.Empty;
        }
        public bool IsFolder()
        {
            return m_Path[m_Path.Count() - 1] == '/';
        }
        public bool IsMeta() { return m_State.HasFlag(State.kMetaFile); }

        public int CompareTo(VersionedAsset other)
        {
            return GetPath().CompareTo(other.GetPath());
        }

        public bool ExistsOnDisk
        {
            get
            {
                return File.Exists(Path) || Directory.Exists(Path);
            }
        }

        public void MakeReadonly()
        {
            if (IsFolder())
            {
                var di = new DirectoryInfo(Path);
                di.Attributes &= FileAttributes.ReadOnly;
            }
            else
            {
                FileInfo file = new FileInfo(Path);
                file.IsReadOnly = true;
            }
        }

        public bool IsWritable
        {
            get
            {
                if (IsFolder())
                {
                    var di = new DirectoryInfo(Path);
                    return di.Exists && (di.Attributes & FileAttributes.ReadOnly) != FileAttributes.ReadOnly;
                }
                else
                {
                    FileInfo file = new FileInfo(Path);
                    return file.Exists && file.IsReadOnly == false;
                }
            }
        }

        public void MakeWritable()
        {
            if (IsFolder())
            {
                var di = new DirectoryInfo(Path);
                di.Attributes &= ~FileAttributes.ReadOnly;
            }
            else
            {
                FileInfo file = new FileInfo(Path);
                file.IsReadOnly = false;
            }
        }


        public bool IsOfType(List<string> types)
        {
            foreach (var item in types)
            {
                if (m_Path.EndsWith(item, StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
            }

            return false;
        }

        public bool IsSceneFile
        {
            get
            {
                return m_Path.EndsWith(".unity", StringComparison.CurrentCultureIgnoreCase) || m_Path.EndsWith(".unity.meta", StringComparison.CurrentCultureIgnoreCase);
            }
        }
        public bool IsPrefab
        {
            get
            {
                return m_Path.EndsWith(".prefab", StringComparison.CurrentCultureIgnoreCase) || m_Path.EndsWith(".prefab.meta", StringComparison.CurrentCultureIgnoreCase);
            }
        }

        public bool IsEditorSetting
        {
            get
            {
                return m_Path.EndsWith(".asset", StringComparison.CurrentCultureIgnoreCase);
            }
        }

        public static List<string> Paths(VersionedAssetList assets)
        {
            List<string> result = new List<string>(assets.Count);

            foreach (var i in assets)
            {
                string p = i.GetPath();
                if (i.IsFolder())
                    p = p.Remove(p.Length - 1, 1); // strip ending /
                result.Add(p);
            }
            return result;
        }

        public void Write(Connection p)
        {
            p.DataLine(GetPath());
            p.DataLine((int)GetState(), logNote: GetState().ToString());
        }

        public void Read(Connection p)
        {
            string line;
            line = p.ReadLine();
            SetPath(line);
            State parsedState = p.ReadLine<State>((s) => (State)int.Parse(s));
            SetState(parsedState);
        }

        State m_State;
        string m_Path = string.Empty;
        string m_MovedPath = string.Empty; // Only used for moved files. May be src or dst file depending on the kDeletedLocal/kAddedLocal flag
        string m_Revision = string.Empty;
        string m_ChangeListID = string.Empty; // Some VCS doesn't support this so it is optional

        public bool Equals(VersionedAsset other)
        {
            return m_State == other.m_State
                && m_Path.Equals(other.m_Path, StringComparison.CurrentCultureIgnoreCase)
                && m_MovedPath.Equals(other.m_MovedPath, StringComparison.CurrentCultureIgnoreCase)
                && m_Revision == other.m_Revision
                && m_ChangeListID == other.m_ChangeListID;
        }

        // override object.Equals
        public override bool Equals(object obj)
        {
            if (obj == null || GetType() != obj.GetType())
            {
                return false;
            }

            return Equals((VersionedAsset)obj);
        }

        // override object.GetHashCode
        public override int GetHashCode()
        {
            return m_State.GetHashCode() ^ m_Path.GetHashCode() ^ m_Revision.GetHashCode();
        }
    }

    public class VersionedAssetList : List<VersionedAsset>
    {
        public VersionedAssetList()
        {

        }
        public VersionedAssetList(IEnumerable<VersionedAsset> collection)
            : base(collection)
        {

        }
    }
}
