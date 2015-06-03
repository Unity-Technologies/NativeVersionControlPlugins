//
// Copyright (C) Microsoft. All rights reserved.
//

using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace TfsPlugin.UnitTestProject
{
    /// <summary>
    /// TfsTempWorkspace creates a new TFS workspace mapped to the users temp folder.
    /// </summary>
    public class TfsTempWorkspace : IDisposable
    {
        private Workspace workspace;
        private VersionControlServer vcs;
        public bool IsFrozen { get; set; }

        public TfsTempWorkspace(VersionControlServer vcs, string workspaceName, string userName, bool isFrozen = false)
        {
            this.vcs = vcs;
            this.IsFrozen = isFrozen;
            this.workspace = vcs.QueryWorkspaces(workspaceName, userName, null).FirstOrDefault();

            if (isFrozen)
            {
                return;
            }

            if (this.workspace != null)
            {
                this.workspace.Undo("$/", RecursionType.Full);

                this.workspace.Delete();
            }

            this.workspace = vcs.CreateWorkspace(workspaceName);

            CleanDirectory();
        }


        public Workspace Workspace
        {
            get
            {
                return this.workspace;
            }
        }


        public string Map(string serverPath, string localRelativePath)
        {
            if (this.IsFrozen)
            {
                return this.workspace.GetLocalItemForServerItem(serverPath);
            }

            string localPath = Path.Combine(Path.GetTempPath(), this.workspace.Name, localRelativePath);
            this.workspace.Map(serverPath, localPath);
            return localPath;
        }

        public void CleanDirectory()
        {
            if (this.IsFrozen)
            {
                return;
            }

            string rootPath = Path.Combine(Path.GetTempPath(), this.workspace.Name);
            if (Directory.Exists(rootPath))
            {
                DeleteDirectory(rootPath);
            }
        }


        public void Dispose()
        {
            if (this.IsFrozen)
            {
                return;
            }

            this.workspace.Delete();
        }

        public static void DeleteDirectory(string target_dir)
        {
            string[] files = Directory.GetFiles(target_dir);
            string[] dirs = Directory.GetDirectories(target_dir);

            foreach (string file in files)
            {
                File.SetAttributes(file, FileAttributes.Normal);
                File.SetAttributes(file, File.GetAttributes(file) & ~FileAttributes.Hidden);
                File.Delete(file);
            }

            foreach (string dir in dirs)
            {
                DeleteDirectory(dir);
            }

            var i = new DirectoryInfo(target_dir);
            i.Attributes = i.Attributes & ~FileAttributes.Hidden;
            File.SetAttributes(target_dir, FileAttributes.Normal);

            Directory.Delete(target_dir, true);
        }
    }
}
