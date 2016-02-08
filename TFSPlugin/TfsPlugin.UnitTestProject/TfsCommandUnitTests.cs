//
// Copyright (C) Microsoft. All rights reserved.
//

using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;
using Microsoft.TeamFoundation.Client;
using Microsoft.TeamFoundation.VersionControl.Client;
using Microsoft.TeamFoundation.Framework.Client;
using System.Linq;
using System.Diagnostics;
using System.Threading;
using System.Collections.Generic;
using System.Reflection;

namespace TfsPlugin.UnitTestProject
{
    /// <summary>
    /// to run the tests in this class, edit the UnitTestSettings.Default.ServerPathToUnitTestProject and UnitTestSettings.Default.TFSServerAddress configuration settings
    ///   to match the TFS project you are testing against
    /// </summary>
    [TestClass]
    public class TfsCommandUnitTests
    {
        readonly static string ServerPathToUnitTestProject = UnitTestSettings.Default.ServerPathToUnitTestProject;
        readonly static string TFSServerAddress = UnitTestSettings.Default.TFSServerAddress;

        TfsTeamProjectCollection projectCollection;
        TfsTempWorkspace tempWorkspace;
        TfsTempWorkspace tempWorkspace2;

        string testProjectPath;
        string mainScenePath;
        string mainSceneMovedPath;
        string scenesFolderPath;

        string testProjectPath2;
        string mainScenePath2;

        string mockNamedPipeName;


        Workspace Workspace
        {
            get
            {
                return tempWorkspace.Workspace;
            }
        }

        Workspace Workspace2
        {
            get
            {
                return tempWorkspace2.Workspace;
            }
        }


        static Dictionary<string, Assembly> LoadedAssemblies = InstallChecker.LoadAssemblies();

        [TestInitialize]
        public void Setup()
        {
            mockNamedPipeName = Guid.NewGuid().ToString("D") + "-testpipe.txt";
            this.projectCollection = new TfsTeamProjectCollection(new Uri(TFSServerAddress));
            this.projectCollection.EnsureAuthenticated();

#if TEAMFOUNTATION_14
            this.projectCollection.GetService<VersionControlServer>().NonFatalError += TfsCommandUnitTests_NonFatalError;
#endif
            this.tempWorkspace = new TfsTempWorkspace(this.projectCollection.GetService<VersionControlServer>(), "__TfsUnitTestWorkspace", projectCollection.AuthorizedIdentity.UniqueName, isFrozen: false);

            var mapping = this.tempWorkspace.Map(ServerPathToUnitTestProject + "/TestResources/UnityProjects", "TestResources");

            Workspace.Get();

            this.tempWorkspace2 = new TfsTempWorkspace(this.projectCollection.GetService<VersionControlServer>(), "__TfsUnitTestWorkspace2", projectCollection.AuthorizedIdentity.UniqueName, isFrozen: false);

            var mapping2 = this.tempWorkspace2.Map(ServerPathToUnitTestProject + "/TestResources/UnityProjects", "TestResources");

            Workspace2.Get();

            this.testProjectPath = Path.Combine(mapping, "TestOne");
            this.mainScenePath = Path.Combine(this.testProjectPath, "Assets\\Scenes\\main.unity");
            this.mainSceneMovedPath = Path.Combine(this.testProjectPath, "Assets\\Scenes\\mainMoved.unity");

            this.testProjectPath2 = Path.Combine(mapping2, "TestOne");
            this.mainScenePath2 = Path.Combine(this.testProjectPath2, "Assets\\Scenes\\main.unity");

            this.scenesFolderPath = Path.Combine(this.testProjectPath, "Assets\\Scenes\\");
        }

#if TEAMFOUNTATION_14
        private void TfsCommandUnitTests_NonFatalError(object sender, ExceptionEventArgs e)
        {
           
        }
#endif

        [TestCleanup]
        public void Cleanup()
        {
            if (this.tempWorkspace != null)
            {
                this.tempWorkspace.Dispose();
            }

            if (this.tempWorkspace2 != null)
            {
                this.tempWorkspace2.Dispose();
            }
            GC.Collect();
            // give tfs some time to finish deleting the workspaces
            //Thread.Sleep(500);         
        }

        [TestMethod]
        public void TestStatus()
        {
            TfsTask task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);

            var s = task.GetStatus();
        }

        [TestMethod]
        public void TestPendingStatus()
        {
            TfsTask task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);

            Workspace.PendEdit(new[] { mainScenePath }, RecursionType.None, null, LockLevel.Checkin);

            VersionedAssetList result = new VersionedAssetList();
            task.GetStatus(new VersionedAssetList { new VersionedAsset(mainScenePath.Replace("\\", "/")) }, result, true, true);
            Assert.AreEqual(true, result[0].HasState(State.kCheckedOutLocal));

            var newFile = Path.Combine(this.testProjectPath, "assets", "new.txt");
            File.WriteAllText(newFile, "hi");
            Workspace.PendAdd(newFile);

            result = new VersionedAssetList();
            task.GetStatus(new VersionedAssetList { new VersionedAsset(newFile.Replace("\\", "/")) }, result, true, true);
            Assert.AreEqual(true, result[0].HasState(State.kAddedLocal));
        }

        [TestMethod]
        public void TestBulkStatus()
        {
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);

            List<string> paths = new List<string>();

            foreach (var item in Directory.GetFiles(this.testProjectPath, "*.*", SearchOption.AllDirectories))
            {
                paths.Add(item);
            }

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:status");
            pipe.SendMessageLine(paths.Count.ToString());

            foreach (var item in paths)
            {
                pipe.SendMessageLine(ConvertToUnityPath(item));
                pipe.SendMessageLine("0");
            }
            pipe.SendMessageLine("c:shutdown");

            task.Run();

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestStatus2()
        {
            TfsTask task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            var w = new Stopwatch();
            w.Start();
            var s = task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            task.GetStatus();
            var time = w.ElapsedMilliseconds;
            Assert.IsTrue(time < 10000);
        }

        string ConvertToUnityPath(string path)
        {
            return path.Replace("\\", "/");
        }

        string ConvertToUnityPathAddMeta(string path)
        {
            return path.Replace("\\", "/").TrimEnd('/') + ".meta";
        }

        [TestMethod]
        public void TestDelete()
        {
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:delete");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            task.Run();

            sceneStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.Delete | ChangeType.Lock, sceneStatus.LocalChangeType);


            var scenedeletedStatus = task.TfsFileStatusCache.Find(ConvertToUnityPath(mainScenePath));
            Assert.AreEqual(ChangeType.Delete | ChangeType.Lock, scenedeletedStatus.LocalChangeType);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestCheckout()
        {
            List<string> warnings = new List<string>();
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:checkout");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            task.Run();

            sceneStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.Edit | ChangeType.Lock, sceneStatus.LocalChangeType);

            Assert.AreEqual(0, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestCheckoutFolders()
        {
            List<string> warnings = new List<string>();
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:checkout");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(scenesFolderPath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(scenesFolderPath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            //Check that the main scene got checked out
            var fileStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, fileStatus.LocalChangeType);

            task.Run();

            fileStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.Edit | ChangeType.Lock, fileStatus.LocalChangeType);

            Assert.AreEqual(0, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestBinaryCheckoutWithRemoteCheckoutLock()
        {
            List<string> warnings = new List<string>();

            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:checkout");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            Workspace2.PendEdit(new[] { mainScenePath2 }, RecursionType.None, null, LockLevel.CheckOut);

            task.Run();

            sceneStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            var scenemetaStatus = TfsFileStatus.CreateFromLocalPath(Workspace, ConvertToUnityPathAddMeta(mainScenePath));
            Assert.AreEqual(ChangeType.Edit | ChangeType.Lock, scenemetaStatus.LocalChangeType);


            Assert.AreEqual(1, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestBinaryCheckoutWithRemoteCheckinLock()
        {
            List<string> warnings = new List<string>();
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:checkout");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            Workspace2.PendEdit(new[] { mainScenePath2 }, RecursionType.None, null, LockLevel.Checkin);

            task.Run();

            sceneStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            Assert.AreEqual(1, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestBinaryCheckoutWithRemoteCheckoutNolock()
        {
            List<string> warnings = new List<string>();
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:checkout");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine(ConvertToUnityPathAddMeta(mainScenePath));
            pipe.SendMessageLine("3");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            Workspace2.PendEdit(new[] { mainScenePath2 }, RecursionType.None, null, LockLevel.None);

            task.Run();

            sceneStatus.RefreshExtendedItem(Workspace);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            Assert.AreEqual(1, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestRename()
        {
            List<string> warnings = new List<string>();
            var task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);
            task.WarningDialogDisplayed += (m) => warnings.Add(m);

            var pipe = CreateUnityConnectionPipe();

            pipe.SendMessageLine("c:move");
            pipe.SendMessageLine("2");
            pipe.SendMessageLine(ConvertToUnityPath(mainScenePath));
            pipe.SendMessageLine("0");
            pipe.SendMessageLine(ConvertToUnityPath(mainSceneMovedPath));
            pipe.SendMessageLine("0");
            pipe.SendMessageLine("c:shutdown");

            var sceneStatus = TfsFileStatus.CreateFromLocalPath(Workspace, mainScenePath);
            Assert.AreEqual(ChangeType.None, sceneStatus.LocalChangeType);

            task.Run();

            var sceneRenamedStatus = task.TfsFileStatusCache.Find(ConvertToUnityPath(mainSceneMovedPath));
            Assert.AreEqual(ChangeType.Rename | ChangeType.Lock, sceneRenamedStatus.LocalChangeType);

            Assert.AreEqual(0, warnings.Count);

            pipe.Close();
            task.Close();

            var pipeText = File.ReadAllText(mockNamedPipeName);
        }

        [TestMethod]
        public void TestIncomingCommand()
        {
            TfsTask task = CreateTfsTaskFromLocalProjectPath(TFSServerAddress, this.testProjectPath);

            var history2 = task.VersionControlServer.QueryHistory(task.ProjectPath,
                new WorkspaceVersionSpec(task.Workspace), 0,
                RecursionType.Full, null, new WorkspaceVersionSpec(task.Workspace),
                null, 10, true, true).Cast<Changeset>().ToList();

            var w = new WorkspaceVersionSpec(task.Workspace);

            var history3 = task.VersionControlServer.QueryHistory(new QueryHistoryParameters(task.ProjectPath, RecursionType.Full)
            {
                VersionEnd = w,
                MaxResults = 1
            }).Cast<Changeset>().ToArray();

            var history5 = task.VersionControlServer.QueryHistory(new QueryHistoryParameters(task.ProjectPath, RecursionType.Full)
            {
                VersionStart = new ChangesetVersionSpec(history3[0].ChangesetId + 1),
                VersionEnd = VersionSpec.Latest
            }).Cast<Changeset>().ToArray();
        }

        private Pipe CreateUnityConnectionPipe()
        {
            var result = new Pipe();
            result.Connect(File.Open(mockNamedPipeName, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite));

            return result;
        }

        private TfsTask CreateTfsTaskFromLocalProjectPath(string url, string projectLocalPath)
        {
            File.WriteAllText(mockNamedPipeName, "");

            MemoryStream log = new MemoryStream();

            var conn = new Connection(log, File.Open(mockNamedPipeName, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite));

            TfsTask task = new TfsTask(conn, url, projectLocalPath);

            return task;
        }
    }
}
