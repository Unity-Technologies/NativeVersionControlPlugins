using System.IO;
using System.Xml;
using System.Xml.Serialization;

namespace TfsPlugin
{
    /// <summary>
    /// Settings for how to handle the TFS version control.
    /// </summary>
    [XmlRoot]
    public class TfsSettings
    {
        private static TfsSettings defaultInstance = new TfsSettings();

        private string tfsNoLock;
        private string exclusiveCheckoutFileTypes;
        private string lockLevels;
        private bool? allowSavingLockedExclusiveFiles;
        private string connectionString;
        private string tfsIgnore;
        private string ignoreUsers;
        private string shareBinaryFileCheckout;

        /// <summary>
        /// Initializes a new instance of the <see cref="TfsSettings"/> class.
        /// </summary>
        public TfsSettings()
        {
            this.ResetSettings();
        }

        /// <summary>
        /// Gets the default settings for the version control.
        /// </summary>
        [XmlIgnore]
        public static TfsSettings Default
        {
            get
            {
                return defaultInstance;
            }
        }

        /// <summary>
        /// Gets the path to the file from which the settings were loaded.
        /// </summary>
        [XmlIgnore]
        public string LoadedSettingsPath { get; private set; }

        /// <summary>
        /// Gets or sets a semicolon delimited list of file path wildcards to never lock on checkout ex. "*/Some.prefab;*/SomeOther.prefab".
        /// </summary>
        [XmlElement]
        public string TfsNoLock
        {
            get
            {
                if (this.tfsNoLock == null)
                {
                    return Settings.Default.tfsnolock;
                }

                return this.tfsNoLock;
            }

            set
            {
                this.tfsNoLock = value;
            }
        }

        /// <summary>
        /// Gets or sets a pipe delimited list of file path extensions to lock on checkout. Most useful when using binary format. ex. ".unity|.prefab|.anim|.controller".
        /// </summary>
        [XmlElement]
        public string ExclusiveCheckoutFileTypes
        {
            get
            {
                if (this.exclusiveCheckoutFileTypes == null)
                {
                    return Settings.Default.ExclusiveCheckoutFileTypes;
                }

                return this.exclusiveCheckoutFileTypes;
            }

            set
            {
                this.exclusiveCheckoutFileTypes = value;
            }
        }

        /// <summary>
        /// Gets or sets a pipe delimited list of colon delimited TFS server path:locklevel pairs.      
        /// </summary>
        /// <remarks>
        /// A check-out lock prevents other users from checking out and making changes to the locked item in their workspaces.
        /// A check-in lock is less restrictive than a check-out lock. If you apply a check-in lock, users can continue to make local changes to the item in other workspaces.
        /// However, those changes cannot be checked in until you remove the lock or check in your changes (if the file is non-mergable, one users version will be stomped).
        /// Can be specified per server path. Valid values are None, Checkin or Checkout. The default value is Checkout.
        /// ex. "$/:Checkout|$/ProjectA:Checkin".
        /// </remarks>
        [XmlElement]
        public string LockLevels
        {
            get
            {
                if (this.lockLevels == null)
                {
                    return Settings.Default.LockLevels;
                }

                return this.lockLevels;
            }

            set
            {
                this.lockLevels = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether or not to allow saving locked exclusive files.
        /// </summary>
        /// <remarks>
        /// When True Unity will allow the user to save unsaved changes if a ExclusiveCheckoutFileTypes file is unable to be checked out and locked.
        /// When False Unity will NOT allow the user to save unsaved changes if a ExclusiveCheckoutFileTypes file is unable to be checked out and locked.
        /// </remarks>
        [XmlElement]
        public bool AllowSavingLockedExclusiveFiles
        {
            get
            {
                if (!this.allowSavingLockedExclusiveFiles.HasValue)
                {
                    return Settings.Default.AllowSavingLockedExclusiveFiles;
                }

                return this.allowSavingLockedExclusiveFiles.Value;
            }

            set
            {
                this.allowSavingLockedExclusiveFiles = value;
            }
        }

        /// <summary>
        /// Gets or sets the TFS server connection string.
        /// </summary>
        [XmlElement]
        public string TfsServerConnectionString
        {
            get
            {
                if (this.connectionString == null)
                {
                    return Settings.Default.DefaultURL;
                }

                return this.connectionString;
            }

            set
            {
                this.connectionString = value;
            }
        }

        /// <summary>
        /// Gets or sets a semicolon delimited list of file path wildcards to ignore from version control ex. "*/Some.dll;*/Some.pdb".
        /// </summary>
        [XmlElement]
        public string TfsIgnore
        {
            get
            {
                if (this.tfsIgnore == null)
                {
                    return Settings.Default.tfsignore;
                }

                return this.tfsIgnore;
            }

            set
            {
                this.tfsIgnore = value;
            }
        }

        /// <summary>
        /// Gets or sets a pipe delimited list of users that will always be forced to work offline. Most useful for automated build user accounts. ex. "serviceAccount1|serviceAccount2".
        /// </summary>
        [XmlElement]
        public string IgnoreUsers
        {
            get
            {
                if (this.ignoreUsers == null)
                {
                    return Settings.Default.ignoreUsers;
                }

                return this.ignoreUsers;
            }

            set
            {
                this.ignoreUsers = value;
            }
        }

        /// <summary>
        /// Gets or sets the share binary file checkout.
        /// </summary>
        /// <remarks>
        /// Pipe delimited list of colon separated TFS server path:boolean pairs. 
        /// When true, the plugin will checkout ExclusiveCheckoutFileTypes files without a lock if they are currently checked out but not locked.
        /// When false, the plugin will NOT checkout ExclusiveCheckoutFileTypes files without a lock if they are currently checked out but not locked.
        /// ex. "$/:false|$/ProjectA:true".</remarks>
        [XmlElement]
        public string ShareBinaryFileCheckout
        {
            get
            {
                if (this.shareBinaryFileCheckout == null)
                {
                    return Settings.Default.ShareBinaryFileCheckout;
                }

                return this.shareBinaryFileCheckout;
            }

            set
            {
                this.shareBinaryFileCheckout = value;
            }
        }

        /// <summary>
        /// Deserialize the TFS settings from the specified configuration file.
        /// </summary>
        /// <param name="path">The TFS settings file to load.</param>
        public static void Load(string path)
        {
            if (!File.Exists(path))
            {
                TfsSettings.Default.ResetSettings();
                return;
            }

            FileStream fileStream = null;
            try
            {
                fileStream = File.OpenRead(path);

                using (XmlReader reader = XmlReader.Create(fileStream))
                {
                    fileStream = null;
                    XmlSerializer xml = new XmlSerializer(typeof(TfsSettings));
                    TfsSettings settings = (TfsSettings)xml.Deserialize(reader);
                    settings.LoadedSettingsPath = Path.GetFullPath(path);
                    reader.Close();

                    TfsSettings.Default.CopySettings(settings);
                }
            }
            finally
            {
                if (fileStream != null)
                {
                    fileStream.Dispose();
                    fileStream = null;
                }
            }
        }

        private void ResetSettings()
        {
            this.LoadedSettingsPath = string.Empty;
            this.allowSavingLockedExclusiveFiles = null;
            this.connectionString = null;
            this.exclusiveCheckoutFileTypes = null;
            this.ignoreUsers = null;
            this.lockLevels = null;
            this.shareBinaryFileCheckout = null;
            this.tfsIgnore = null;
            this.tfsNoLock = null;
        }

        private void CopySettings(TfsSettings settings)
        {
            this.LoadedSettingsPath = settings.LoadedSettingsPath;
            this.allowSavingLockedExclusiveFiles = settings.allowSavingLockedExclusiveFiles;
            this.connectionString = settings.connectionString;
            this.exclusiveCheckoutFileTypes = settings.exclusiveCheckoutFileTypes;
            this.ignoreUsers = settings.ignoreUsers;
            this.lockLevels = settings.lockLevels;
            this.shareBinaryFileCheckout = settings.shareBinaryFileCheckout;
            this.tfsIgnore = settings.tfsIgnore;
            this.tfsNoLock = settings.tfsNoLock;
        }
    }
}
