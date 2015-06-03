using System;
using System.Collections.Generic;
using System.Linq;

namespace TfsPlugin
{
    public enum ConfigKey
    {
        CK_Traits,
        CK_Versions,
        CK_ProjectPath,
        CK_LogLevel,
        CK_End,
        CK_Unknown,
    }

    public class ConfigRequest : BaseRequest
    {
        public ConfigKey Key { get; set; }
        public string KeyStr { get; set; }
        public string[] Values { get; set; }

        public ConfigRequest(CommandArgs args, Connection conn)
            : base(args, conn)
        {
            if (args.Count < 2)
            {
                string msg = "Plugin got invalid config setting :";

                foreach (var item in args)
                {
                    msg += " ";
                    msg += item;
                }

                conn.WarnLine(msg, MessageArea.MAConfig);
                conn.EndResponse();
                Invalid = true;
                return;
            }

            KeyStr = args[1];
            Key = ConfigKey.CK_Unknown;

            if (KeyStr == "pluginTraits")
                Key = ConfigKey.CK_Traits;
            else if (KeyStr == "pluginVersions")
                Key = ConfigKey.CK_Versions;
            else if (KeyStr == "projectPath")
                Key = ConfigKey.CK_ProjectPath;
            else if (KeyStr == "vcSharedLogLevel")
                Key = ConfigKey.CK_LogLevel;
            else if (KeyStr == "end")
                Key = ConfigKey.CK_End;

            Values = new string[args.Count - 2];
            args.CopyTo(2, Values, 0, Values.Length);
        }

        public LogLevel GetLogLevel()
        {
            string val = GetValues();
            LogLevel level = LogLevel.LOG_DEBUG;
            if (val == "info")
                level = LogLevel.LOG_INFO;
            else if (val == "notice")
                level = LogLevel.LOG_NOTICE;
            else if (val == "fatal")
                level = LogLevel.LOG_FATAL;
            return level;
        }

        public string GetValues()
        {
            return string.Join(" ", Values);
        }
    };

    class ConfigResponse : BaseResponse
    {
        public bool RequiresNetwork { get; set; }
        public bool EnablesCheckout { get; set; }
        public bool EnablesLocking { get; set; }
        public bool EnablesRevertUnchanged { get; set; }
        public bool EnablesChangelists { get; set; }
        public bool EnablesGetLatestOnChangeSetSubset { get; set; }
        List<int> overlays = new List<int>();
        public List<int> Overlays { get { return overlays; } }

        List<PluginTrait> traits = new List<PluginTrait>();
        HashSet<int> supportedVersions = new HashSet<int>();

        ConfigRequest request;
        Connection conn;

        [Flags]
        public enum TraitFlag
        {
            TF_None = 0,
            TF_Required = 1,
            TF_Password = 2
        };

        struct PluginTrait
        {
            public string id; // e.g. "vcSubversionUsername"
            public string label; // e.g. "Username"
            public string description; // e.g. "The user name"
            public string defaultValue; // e.g. "anonymous"
            public TraitFlag flags;
        };

        public ConfigResponse(ConfigRequest req)
        {
            request = req;
            conn = req.Conn;
            RequiresNetwork = false;
            EnablesCheckout = true;
            EnablesLocking = true;
            EnablesRevertUnchanged = true;
        }

        public void addTrait(string id, string label, string desc, string def, TraitFlag flags)
        {
            PluginTrait t = new PluginTrait { id = id, label = label, description = desc, defaultValue = def, flags = flags };
            traits.Add(t);
        }

        public override void Write()
        {
            if (request.Invalid)
            {
                conn.EndResponse();
                return;
            }

            switch (request.Key)
            {
                case ConfigKey.CK_Versions:
                    {
                        int v = SelectVersion(request.Values);
                        conn.DataLine(v, MessageArea.MAConfig);
                        conn.LogInfo("Selected plugin protocol version " + v);
                        break;
                    }
                case ConfigKey.CK_Traits:
                    {
                        // First mandatory traits
                        int count =
                            (RequiresNetwork ? 1 : 0) +
                            (EnablesCheckout ? 1 : 0) +
                            (EnablesLocking ? 1 : 0) +
                            (EnablesChangelists ? 1 : 0) +
                            (EnablesGetLatestOnChangeSetSubset ? 1 : 0) +
                            (EnablesRevertUnchanged ? 1 : 0);

                        conn.DataLine(count);

                        if (RequiresNetwork)
                            conn.DataLine("requiresNetwork", MessageArea.MAConfig);
                        if (EnablesCheckout)
                            conn.DataLine("enablesCheckout", MessageArea.MAConfig);
                        if (EnablesLocking)
                            conn.DataLine("enablesLocking", MessageArea.MAConfig);
                        if (EnablesRevertUnchanged)
                            conn.DataLine("enablesRevertUnchanged", MessageArea.MAConfig);
                        if (EnablesChangelists)
                            conn.DataLine("enablesChangelists", MessageArea.MAConfig);
                        if (EnablesGetLatestOnChangeSetSubset)
                            conn.DataLine("enablesGetLatestOnChangeSetSubset", MessageArea.MAConfig);

                        // The per plugin defined traits
                        conn.DataLine(traits.Count);

                        foreach (var i in traits)
                        {
                            conn.DataLine(i.id);
                            conn.DataLine(i.label, MessageArea.MAConfig);
                            conn.DataLine(i.description, MessageArea.MAConfig);
                            conn.DataLine(i.defaultValue);
                            conn.DataLine((int)i.flags);
                        }
                        if (Overlays.Count != 0)
                        {
                            conn.DataLine("overlays");
                            conn.DataLine(Overlays.Count.ToString());

                            foreach (var item in Overlays)
                            {
                                conn.DataLine(item.ToString());
                                conn.DataLine("default");
                            }
                        }

                        break;
                    }
            }
            conn.EndResponse();
        }

        public void AddSupportedVersion(int v)
        {
            supportedVersions.Add(v);
        }


        int SelectVersion(string[] args)
        {
            HashSet<int> unitySupportedVersions = new HashSet<int>();
            HashSet<int> pluginSupportedVersions = supportedVersions;

            pluginSupportedVersions.Add(1);

            // Read supported versions from unity
            foreach (var i in args)
            {
                unitySupportedVersions.Add(int.Parse(i));

            }

            HashSet<int> candidates = unitySupportedVersions;
            candidates.IntersectWith(pluginSupportedVersions);

            if (candidates.Count == 0)
                return -1;

            return candidates.Max();
        }
    };
}
