using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace TfsPlugin.UnitTestProject
{
    [TestClass]
    [DeploymentItem(@"TestResources\TfsSettings\", @"TfsSettings\")]
    public class TfsSettingsTests
    {
        [TestMethod]
        public void NoProjectSettingsFile()
        {
            TfsSettings expectedTfsSettings = TfsSettingsTests.ExpectedTfsSettings(true);

            Assert.AreEqual(string.Empty, TfsSettings.Default.LoadedSettingsPath);
            TfsSettingsTests.AssertSettingsAreEuqal(expectedTfsSettings, TfsSettings.Default);
        }

        [TestMethod]
        public void ProjectSettingsFileWithNoOverrides()
        {
            TfsSettings expectedTfsSettings = TfsSettingsTests.ExpectedTfsSettings(true);
            TfsSettings.Load(@"TfsSettings\NoOverrideSettings.xml");

            Assert.AreEqual(Path.GetFullPath(@"TfsSettings\NoOverrideSettings.xml"), TfsSettings.Default.LoadedSettingsPath);
            TfsSettingsTests.AssertSettingsAreEuqal(expectedTfsSettings, TfsSettings.Default);
        }

        [TestMethod]
        public void ProjectSettingsFileWithOverrides()
        {
            TfsSettings expectedTfsSettings = TfsSettingsTests.ExpectedTfsSettings(false);
            TfsSettings.Load(@"TfsSettings\OverrideAllSettings.xml");

            Assert.AreEqual(Path.GetFullPath(@"TfsSettings\OverrideAllSettings.xml"), TfsSettings.Default.LoadedSettingsPath);
            TfsSettingsTests.AssertSettingsAreEuqal(expectedTfsSettings, TfsSettings.Default);
        }

        [TestMethod]
        public void ProjectClearSettings()
        {
            TfsSettings expectedTfsSettings = TfsSettingsTests.ExpectedTfsSettings(true);
            TfsSettings.Load(@"TfsSettings\OverrideAllSettings.xml");
            Assert.AreEqual(Path.GetFullPath(@"TfsSettings\OverrideAllSettings.xml"), TfsSettings.Default.LoadedSettingsPath);

            TfsSettings.Load(null);
            Assert.AreEqual(string.Empty, TfsSettings.Default.LoadedSettingsPath);
            TfsSettingsTests.AssertSettingsAreEuqal(expectedTfsSettings, TfsSettings.Default);
        }

        private static TfsSettings ExpectedTfsSettings(bool defaultValues)
        {
            TfsSettings settings = new TfsSettings();

            if (defaultValues)
            {
                // Because the Settings are not an exposed class, just hardcoding the current values for now.  We can change if we expect to change values later.
                settings.TfsServerConnectionString = string.Empty;
                settings.TfsIgnore = string.Empty;
                settings.TfsNoLock = string.Empty;
                settings.ExclusiveCheckoutFileTypes = ".unity|.prefab|.anim|.controller";
                settings.IgnoreUsers = string.Empty;
                settings.ShareBinaryFileCheckout = "$/:false";
                settings.LockLevels = "$/:Checkout";
                settings.AllowSavingLockedExclusiveFiles = true;
            }
            else
            {
                settings.TfsServerConnectionString = "https://address.com:port/tfs/ProjectName";
                settings.TfsIgnore = "*/Some.dll;*/Some.pdb";
                settings.TfsNoLock = "*/Some.prefab;*/SomeOther.prefab";
                settings.ExclusiveCheckoutFileTypes = string.Empty;
                settings.IgnoreUsers = "serviceAccount1|serviceAccount2";
                settings.ShareBinaryFileCheckout = "$/:true";
                settings.LockLevels = "$/:Checkin";
                settings.AllowSavingLockedExclusiveFiles = false;
            }

            return settings;
        }

        private static void AssertSettingsAreEuqal(TfsSettings expectedTfsSettings, TfsSettings actualTfsSettings)
        {
            Assert.AreEqual(expectedTfsSettings.TfsServerConnectionString, actualTfsSettings.TfsServerConnectionString);
            Assert.AreEqual(expectedTfsSettings.TfsIgnore, actualTfsSettings.TfsIgnore);
            Assert.AreEqual(expectedTfsSettings.TfsNoLock, actualTfsSettings.TfsNoLock);
            Assert.AreEqual(expectedTfsSettings.ExclusiveCheckoutFileTypes, actualTfsSettings.ExclusiveCheckoutFileTypes);
            Assert.AreEqual(expectedTfsSettings.IgnoreUsers, actualTfsSettings.IgnoreUsers);
            Assert.AreEqual(expectedTfsSettings.ShareBinaryFileCheckout, actualTfsSettings.ShareBinaryFileCheckout);
            Assert.AreEqual(expectedTfsSettings.LockLevels, actualTfsSettings.LockLevels);
            Assert.AreEqual(expectedTfsSettings.AllowSavingLockedExclusiveFiles, actualTfsSettings.AllowSavingLockedExclusiveFiles);
        }
    }
}
