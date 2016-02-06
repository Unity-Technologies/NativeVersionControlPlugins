using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using CheckoutRequest = TfsPlugin.BaseFileSetRequest;
using CheckoutResponse = TfsPlugin.BaseFileSetResponse<TfsPlugin.BaseFileSetRequest>;


namespace TfsPlugin
{
    class TfsCheckoutCommand : ICommand<TfsTask, CheckoutRequest, CheckoutResponse>
    {
        public bool Run(TfsTask task, CheckoutRequest req, CheckoutResponse resp)
        {
            ExpandFoldersIntoFiles(task, req.Conn, req.Assets);

            PendEdit(task, req.Conn, req.Assets);

            task.GetStatus(req.Assets, resp.Assets, false, true);
            
            if (task.IsPartiallyOffline)
            {
                task.UserWantsToGoPartiallyOfflineFor(resp.Assets.Where(item => !item.HasState(State.kCheckedOutLocal)));
               
                foreach (var item in resp.Assets.Where(item => !item.HasState(State.kCheckedOutLocal)))
                {                      
                    // make file writable
                    item.MakeWritable();

                    // add the working locally state to the item manually
                    item.AddState(State.kOutOfSync | State.kCheckedOutLocal);   

                    req.Conn.InfoLine("Now working on the following file offline: " + item.GetPath());                       
                }                
            }
            else
            {
                task.WarnAboutFailureToPendChange(resp.Assets.Where(item => !item.HasState(State.kCheckedOutLocal)));
                task.WarnAboutBinaryFilesCheckedOutByOthers(resp.Assets.Where(item => item.HasState(State.kCheckedOutLocal)));
            }
         
            task.WarnAboutOutOfDateFiles(req.Assets);

            if (!TfsSettings.Default.AllowSavingLockedExclusiveFiles)
            {
                foreach (var item in resp.Assets)
                {
                    if (!item.HasState(State.kCheckedOutLocal))
                    {                    
                        req.Conn.ErrorLine("Unable to checkout " + item.GetPath());
                    }
                }
            }

            resp.Write();
            return true;
        }

        public static void PendEdit(TfsTask task, Connection connection, VersionedAssetList req)
        {
            string[] toLock;
            string[] notToLock;
            task.SplitByLockPolicy(req, out toLock, out notToLock);

            if (toLock != null && toLock.Length != 0)
            {
                try
                {
                    var level = task.GetLockLevel();
                    task.Workspace.PendEdit(toLock, RecursionType.None, null, level);
                }
                catch (Exception e)
                {
                    connection.WarnLine(e.Message);
                }
            }

            if (notToLock != null && notToLock.Length != 0)
            {
                try
                {
                    task.Workspace.PendEdit(notToLock, RecursionType.None, null, LockLevel.None);
                }
                catch (Exception e)
                {
                    connection.WarnLine(e.Message);
                }
            }
        }

        private static void ExpandFoldersIntoFiles(TfsTask task, Connection connection, VersionedAssetList versionedAssets)
        {
            List<string> enumeratedFiles = new List<string>();

            //Check each asset to see if it is a folder
            for (int i = versionedAssets.Count - 1; i >= 0; i--)
            {
                VersionedAsset asset = versionedAssets[i];

                if (!asset.IsFolder())
                { continue; }

                try
                {
                    //Get all files in the folder
                    enumeratedFiles.AddRange(Directory.GetFiles(asset.Path, "*.*", SearchOption.AllDirectories));
                    connection.LogInfo("Expanding Folder: " + asset.Path);
                }
                catch (Exception e)
                {
                    connection.WarnLine(e.Message);
                }

                //Remove the folder from the checkout
                versionedAssets.RemoveAt(i);
            }

            //Get the status of the enumerated files
            VersionedAssetList newVersionedAssets = new VersionedAssetList();
            task.GetStatus(enumeratedFiles.ToArray(), newVersionedAssets, false, true);

            //Remove any local-only files
            for (int i = newVersionedAssets.Count - 1; i >= 0; i--)
            {
                VersionedAsset asset = newVersionedAssets[i];
                if (!asset.IsKnownByVersionControl || asset.HasPendingLocalChange)
                {
                    newVersionedAssets.RemoveAt(i);
                }
            }

            //Add the new assets to the asset list
            versionedAssets.AddRange(newVersionedAssets);
        }
    }
}
