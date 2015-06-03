
namespace TfsPlugin
{

    class TfsConfigCommand : ICommand<TfsTask, ConfigRequest, ConfigResponse>
    {
        public bool Run(TfsTask task, ConfigRequest req, ConfigResponse resp)
        {
            string val = req.GetValues();
            switch (req.Key)
            {
                case ConfigKey.CK_Traits:
                    resp.RequiresNetwork = true;
                    resp.EnablesCheckout = true;
                    resp.EnablesLocking = true;
                    resp.EnablesRevertUnchanged = true;
                    
                    resp.addTrait("vcTfsServer", "Server", "The TFS server using format <protocol>://<url>:<port>/<path>/<Team project collection>", "", ConfigResponse.TraitFlag.TF_None);
                    resp.addTrait("vcTfsPartialOffline", "Work Partially Offline", "Files marked as writable will not be checked out. Enter True or False. Default is False.", "", ConfigResponse.TraitFlag.TF_None);
                    resp.addTrait("vcTfsVersion", "v" + TfsTask.GetAssemblyFileVersion(), "This is the version of the tfs plugin, the text box should be left empty.", "empty", ConfigResponse.TraitFlag.TF_None);

                    resp.Overlays.AddRange(new[] 
                    { 
                        (int)State.kLocal,
                        (int)State.kOutOfSync,
                        (int)State.kCheckedOutLocal,
                        (int)State.kCheckedOutRemote,
                        (int)State.kDeletedLocal,
                        (int)State.kDeletedRemote,
                        (int)State.kAddedLocal,
                        (int)State.kAddedRemote,
                        (int)State.kConflicted,
                        (int)State.kLockedLocal,
                        (int)State.kLockedRemote,
                    });
                    break;

                case ConfigKey.CK_Versions:
                    resp.AddSupportedVersion(2);
                    break;

                case ConfigKey.CK_ProjectPath:
                    req.Conn.LogInfo("Set projectPath to " + val);
                    task.SetProjectPath(val);
                    break;

                case ConfigKey.CK_LogLevel:
                    req.Conn.LogInfo("Set log level to " + val);
                    req.Conn.SetLogLevel(req.GetLogLevel());
                    break;
                case ConfigKey.CK_End:
                    break;
                case ConfigKey.CK_Unknown:
                    if (req.KeyStr == "vcTfsServer")
                    {
                        task.SetTfsServer(val);
                    }
                    else if (req.KeyStr == "vcTfsPartialOffline")
                    {
                        task.SetPartialOffline(val);
                    }
                    else
                    {
                        string msg = "Unknown config field set on tfs plugin: ";
                        msg += req.KeyStr;
                        req.Conn.WarnLine(msg, MessageArea.MAConfig);
                        req.Invalid = true;
                    }
                    break;
            }

            resp.Write();
            return true;
        }
    };
}
