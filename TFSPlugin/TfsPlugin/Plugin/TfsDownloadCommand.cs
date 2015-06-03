using Microsoft.TeamFoundation.VersionControl.Client;
using System;
using System.IO;

namespace TfsPlugin
{
    class TfsDownloadCommand : ICommand<TfsTask, DownloadRequest, DownloadResponse>
    {
        public bool Run(TfsTask task, DownloadRequest req, DownloadResponse resp)
        {
            int idx = 0;

            foreach (var i in req.Assets)
            {
                string assetPath = i.GetPath();

                string tmpFile = req.TargetDir + "/" + idx.ToString() + "_";

                foreach (var j in req.Revisions)
                {
                    string revision = j;

                    try
                    {
                        if (revision == ChangelistRevision.kDefaultListRevision || revision == "head")
                        {
                            if (!EnsureDirectory(req.TargetDir))
                            {
                                string msg = "Could not create temp dir: " + req.TargetDir;
                                req.Conn.ErrorLine(msg);
                                resp.Assets.Clear();
                                resp.Write();
                                return true;
                            }

                            string f = tmpFile + "_" + revision;
                            Download(task, assetPath, f, VersionSpec.Latest);
                            VersionedAsset asset = new VersionedAsset();
                            asset.SetPath(f);
                            resp.Assets.Add(asset);
                        }
                        else if (revision == "mineAndConflictingAndBase")
                        {
                            if (!EnsureDirectory(req.TargetDir))
                            {
                                string msg = "Could not create temp dir: " + req.TargetDir;
                                req.Conn.ErrorLine(msg);
                                resp.Assets.Clear();
                                resp.Write();
                                return true;
                            }

                            VersionedAsset mine, conflict, baseAsset;
                            GetConflictInfo(task, assetPath, tmpFile, out mine, out conflict, out baseAsset);

                            // If we cannot locate the working file we do not 
                            // include this asset in the response.
                            if (mine == null)
                            {
                                req.Conn.ErrorLine("No 'mine' file for " + assetPath);
                                continue;
                            }

                            if (conflict == null)
                            {
                                req.Conn.ErrorLine("No 'conflict' file for " + assetPath);
                                continue;
                            }

                            if (baseAsset == null)
                            {
                                req.Conn.ErrorLine("No 'base' file for " + assetPath);
                                continue;
                            }

                            resp.Assets.Add(mine);
                            resp.Assets.Add(conflict);
                            resp.Assets.Add(baseAsset);
                        }

                        idx += 1;
                    }
                    catch (Exception e)
                    {
                        req.Conn.LogNotice(e.Message);
                        req.Conn.ErrorLine("Error downloading file through svn");
                        resp.Assets.Clear();
                        resp.Write();
                        return true;
                    }
                }
            }
            resp.Write();
            return true;
        }

        private void GetConflictInfo(TfsTask task, string assetPath, string tmpFile, out VersionedAsset mine, out VersionedAsset theirs, out VersionedAsset baseAsset)
        {
            mine = null;
            theirs = null;
            baseAsset = null;

            var conflicts = task.Workspace.QueryConflicts(new[] { assetPath }, false);

            if (conflicts.Length > 0)
            {
                var conflict = conflicts[0];

                mine = new VersionedAsset(assetPath);

                theirs = new VersionedAsset(tmpFile + "_" + conflict.TheirVersion.ToString());
                theirs.SetRevision(conflict.TheirVersion.ToString());
                conflict.DownloadTheirFile(theirs.GetPath());

                baseAsset = new VersionedAsset(tmpFile + "_" + conflict.BaseVersion.ToString());
                baseAsset.SetRevision(conflict.BaseVersion.ToString());
                conflict.DownloadBaseFile(baseAsset.GetPath());
            }
        }

        private void Download(TfsTask task, string assetPath, string targetPath, VersionSpec revision)
        {
            var serverItem = task.Workspace.GetServerItemForLocalItem(assetPath);

            task.VersionControlServer.DownloadFile(serverItem, 0, revision, targetPath);
        }

        private bool EnsureDirectory(string p)
        {
            if (Directory.Exists(p))
            {
                return true;
            }

            Directory.CreateDirectory(p);

            return Directory.Exists(p);
        }
    }
}
