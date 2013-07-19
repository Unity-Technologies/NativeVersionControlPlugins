#include "SvnTask.h"
#include "AllSvnCommands.h"
#include "Dispatch.h"
 
#include "stdio.h" // popen
#include "ctype.h" // isdigit
#include "time.h"
#include <algorithm>
#include <map>

using namespace std;

static map<int, SvnLogResult::Entry> g_LogCache;
static size_t g_LogCacheAssetsCount = 0;

static time_t g_StatusNextCacheUpdateTime = 0;
static VersionedAssetList g_StatusResultAssetsCache;
static std::set<string> g_StatusResultPathsCache;

SvnTask::SvnTask() : m_Connection(NULL), m_IsOnline(false)
{
	SetSvnExecutable(""); // Set default svn executable
}

SvnTask::~SvnTask()
{
	delete m_Connection;
}

void SvnTask::SetRepository(const std::string& p)
{
	if (m_RepositoryConfig != p)
	{
		m_RepositoryConfig = p;
		g_LogCacheAssetsCount = 0;
		g_LogCache.clear();
		g_StatusNextCacheUpdateTime = time(NULL);
	}
	m_IsOnline = false;		
}

const std::string& SvnTask::GetRepository() const
{
	return m_RepositoryConfig;
}

void SvnTask::SetUser(const std::string& p)
{
	if (m_UserConfig != p)
	{
		m_UserConfig = p;
		g_LogCacheAssetsCount = 0;
		g_LogCache.clear();
		g_StatusNextCacheUpdateTime = time(NULL);
	}	
	m_IsOnline = false;		
}

const std::string& SvnTask::GetUser() const
{
	return m_UserConfig;
}

void SvnTask::SetPassword(const std::string& p)
{
	if (m_PasswordConfig != p)
	{
		m_PasswordConfig = p;		
		g_LogCacheAssetsCount = 0;
		g_LogCache.clear();
		g_StatusNextCacheUpdateTime = time(NULL);
	}
	m_IsOnline = false;		
}

const std::string& SvnTask::GetPassword() const
{
	return m_PasswordConfig;
}

void SvnTask::SetOptions(const std::string& p)
{
	if (m_OptionsConfig != p)
	{
		m_OptionsConfig = p;
		g_LogCacheAssetsCount = 0;
		g_LogCache.clear();
		g_StatusNextCacheUpdateTime = time(NULL);
	}
	m_IsOnline = false;		
}

const std::string& SvnTask::GetOptions() const
{
	return m_OptionsConfig;
}

void SvnTask::SetSvnExecutable(const std::string& e)
{
	m_IsOnline = false;		
	if (!e.empty())
	{
		if (PathExists(e))
		{
			m_SvnPath = e;
			return;
		}
		m_Connection->WarnLine(string("No svn executable at path '") + e + "'");
	}

#if defined(_WINDOWS)
	m_SvnPath = PluginPath();
	m_SvnPath = m_SvnPath.substr(0, m_SvnPath.rfind('\\', m_SvnPath.rfind('\\') - 1)) + "\\svn\\svn.exe";
#else // posix
	m_SvnPath = "/usr/bin/svn";
#endif
}

const std::string& SvnTask::GetSvnExecutable() const
{
	return m_SvnPath;
}

std::string SvnTask::GetCredentials() const
{
	string c;
	if (!m_UserConfig.empty())
		c += ToString("--username ", m_UserConfig) + " ";
	if (!m_PasswordConfig.empty())
		c += ToString("--password ", m_PasswordConfig) + " ";
	if (!m_OptionsConfig.empty())
		c+= m_OptionsConfig + " ";
	return c;
}

void SvnTask::SetAssetsPath(const string& p)
{
	if (m_AssetsPath != p)
	{
		m_AssetsPath = p;
		g_LogCacheAssetsCount = 0;
		g_LogCache.clear();
		g_StatusNextCacheUpdateTime = time(NULL);
	}
	m_IsOnline = false;		
}

const string& SvnTask::GetAssetsPath() const
{
	return m_AssetsPath;
}

string SvnTask::GetProjectPath() const
{
	return m_AssetsPath.substr(0, m_AssetsPath.rfind('/'));
}

int SvnTask::Run()
{
	m_Connection = new Connection("./Library/svnplugin.log");

	UnityCommand cmd;
	CommandArgs args;

	try 
	{
		for ( ;; )
		{
			cmd = m_Connection->ReadCommand(args);
			
			try 
			{
				if (cmd == UCOM_Invalid)
					return 1; // error
				else if (cmd == UCOM_Shutdown)
					return 0; // ok 
				else if (!Dispatch<SvnCommand>(*m_Connection, *this, cmd, args))
					return 0; // ok
			}
			catch (SvnException& ex)
			{
				m_Connection->Log().Fatal() << "Fatal 1: " << ex.what() << Endl;
				m_Connection->ErrorLine(ex.what());
				m_Connection->EndResponse();
				return 1;
			}
			catch (PluginException& ex)
			{
				m_Connection->Log().Fatal() << "Fatal 2: " << ex.what() << Endl;
				m_Connection->ErrorLine(ex.what());
				m_Connection->EndResponse();
				return 1;
			}
		}
	} 
	catch (std::exception& e)
	{
		m_Connection->Log().Fatal() << "Fatal: " << e.what() << Endl;
	}
	
	return 1;
}

APOpen SvnTask::RunCommand(const std::string& cmd)
{
	string cred = GetCredentials();
	string cmdline = "\"";
	cmdline += m_SvnPath + "\" " + cred + cmd;
	m_Connection->Log().Info() << cmdline << Endl;
	try
	{
		return APOpen(new POpen(cmdline));
	}
	catch (PluginException e)
	{
		if (PathExists(m_SvnPath))
			throw;
		throw PluginException(string("No svn executable at path '") + m_SvnPath + "'");
	}
}

int ParseChangeState(int state, char c)
{
	switch (c)
	{
	case ' ': // not modified
		break;
	case 'A':
		state |= kAddedLocal; 
		break;
	case 'D':
		state |= kDeletedLocal; 
		break;
	case 'M': // modified
	case 'R': // removed and the added (replaced)
		state |= kCheckedOutLocal; 
		break;
	case 'C':
		state |= kConflicted; 
		break;
	case 'X':
	case 'I':
		break; // external or ignored file. Just skip
	case '?':
		state |= kLocal; // not in version control
		break;
	case '!':
		state |= kOutOfSync; 
		break;
	case '~':
		state |= kCheckedOutLocal; // Changed type (e.g. file -> dir) 
		break;
	default:
		throw SvnException(string("Invalid status 'change' flag ") + ToString((int)c));
	}
		
	return state;
}

int ParsePropertyState(int state, char c)
{
	switch (c)
	{
	case ' ':
		break; // not modified
	case 'M':
		state |= kCheckedOutLocal;
		break;
	case 'C':
		state |= kConflicted;
		break;
	default:
		throw SvnException(string("Invalid status 'property' flag ") + ToString((int)c));
	}
	return state;
}

int ParseWorkingDirLockState(int state, char c)
{
	switch (c)
	{
	case ' ':
		break; // not locked
	case 'L':
		state |= kLockedLocal;
		break;
	default:
		throw SvnException(string("Invalid status 'workdirlock' flag ") + ToString((int)c));
	}
	return state;
}

int ParseLockState(int state, char c)
{
	switch (c)
	{
	case ' ':
		Enforce<SvnException>(! (state & (kLockedLocal | kLockedRemote)),
							  "Property flagged logged but lock state in not locked");
		break; // not locked
	case 'K':
		state |= kLockedLocal;
		break;
	case 'O': // locked remote
	case 'T': // stolen lock and locked remote
		state |= kLockedRemote;
		break;
	case 'B': // Broken local locked and not locked
		state &= ~kLockedLocal; // remote lock flags
		state &= ~kLockedRemote; 
		break;
	default:
		throw SvnException(string("Invalid status 'lock' flag ") + ToString((int)c));
	}
	return state;
}

int ParseConflictState(int state, char c)
{
	switch (c)
	{
	case ' ':
		break; // no conflict
	case 'C':
		state |= kConflicted;
		break;
	default:
		throw SvnException(string("Invalid status 'conflict' flag ") + ToString((int)c));
	}
	return state;
}

int ParseSyncedState(int state, char c)
{
	switch (c)
	{
	case ' ': // no updates on server. Local file is up to date
		state |= kSynced;
		break; 
	case '*': // updates available on server
		state |= kOutOfSync;
		break;
	default:
		throw SvnException(string("Invalid status 'synced' flag ") + ToString((int)c));
	}
	return state;
}

void SvnTask::GetStatus(const VersionedAssetList& assets, VersionedAssetList& result, 
						bool recursive, bool queryRemote)
{
	GetStatusWithChangelists(assets, result, recursive, queryRemote);
}

bool IsDescendant(const VersionedAsset& parent, const VersionedAsset& desc)
{
	size_t len = parent.GetPath().length();
	return parent.GetPath() == desc.GetPath().substr(0, len) && !EndsWith(desc.GetPath(), ".meta");
}

void SvnTask::GetStatusWithChangelists(const VersionedAssetList& inAssets, 
									   VersionedAssetList& result, 
									   bool recursive, bool queryRemote)
{
	// Fetch status for entire workspace since getting status of a range of files will make
	// a request per file to the server. 
	VersionedAssetList tmp;
	std::set<string> resultPaths;
	VersionedAssetList assets(inAssets.begin(), inAssets.end());
	
	// String project path from asset to ease matching
	string projPath = GetProjectPath() + "/";
	size_t prefixLen = projPath.length();
	for (VersionedAssetList::iterator i = assets.begin(); i != assets.end(); ++i)
	{
		if (i->GetPath().substr(0, prefixLen) == projPath)
			i->SetPath(i->GetPath().substr(prefixLen));
	}

	// Reuse results for approx one sec
	time_t timeNow = time(NULL);
	if (timeNow >= g_StatusNextCacheUpdateTime)
	{
		g_StatusResultAssetsCache.clear();
		g_StatusResultPathsCache.clear();
		VersionedAssetList empty;
		bool ok = queryRemote && GetStatusWithChangelists(empty, tmp, resultPaths, "infinity", true);
		if (!ok)
			GetStatusWithChangelists(empty, tmp, resultPaths, "infinity", false);
		g_StatusResultAssetsCache = tmp;
		g_StatusResultPathsCache = resultPaths;
		g_StatusNextCacheUpdateTime = time(NULL) + 1;
	}
	else
	{
		tmp = g_StatusResultAssetsCache;
		resultPaths = g_StatusResultPathsCache;
	}

	// We have all stuff that are in version control in result. Filter away the assets that we are not interested in.
	if (recursive)
	{
		if (assets.empty())
		{
			// Empty means all in recursive mode
			result = tmp;
			return;
		}

		VersionedAssetSet res;
		for (VersionedAssetList::const_iterator i = tmp.begin(); i != tmp.end(); ++i)
		{
			for (VersionedAssetList::const_iterator j = assets.begin(); j != assets.end(); ++j)
			{
				if (IsDescendant(*j,*i))
					res.insert(*i);
			}
		}
		result.insert(result.end(), res.begin(), res.end());
	}
	else
	{
		VersionedAssetSet aset(assets.begin(), assets.end());
		for (VersionedAssetList::const_iterator i = tmp.begin(); i != tmp.end(); ++i)
		{
			if (aset.find(*i) != aset.end())
				result.push_back(*i);
		}
	}

	// Make sure that we have status for local only assets as well
	for (VersionedAssetList::const_iterator i = assets.begin(); i != assets.end(); ++i)
	{
		const string& p = i->GetPath();
		if (resultPaths.find(p) == resultPaths.end())
			result.push_back(VersionedAsset(p, kLocal, "0"));
	}
}

#ifdef DEPRECATED
void SvnTask::GetStatusWithChangelists(const VersionedAssetList& assets, 
									   VersionedAssetList& result, 
									   vector<string>& changelistAssoc,
									   bool recursive, bool queryRemote)
{
	// Svn version 1.7.6 fixed that --depth=empty could be used with files
	// but until that is a common version we need to run partition the
	// assets into files and directories and only use depth=empty on the
	// directories.
	if (recursive)
	{
		bool ok = queryRemote && GetStatusWithChangelists(assets, result, changelistAssoc, "infinity", true);
		if (!ok)
			GetStatusWithChangelists(assets, result, changelistAssoc, "infinity", false);
		return;
	}
	
	VersionedAssetList files;
	VersionedAssetList dirs;
	
	for (VersionedAssetList::const_iterator i = assets.begin(); i != assets.end(); ++i)
	{
		if (IsDirectory(i->GetPath()))
			dirs.push_back(*i);
		else
			files.push_back(*i);
	}
	
	bool bothOk = true;
	if (!dirs.empty())
		bothOk = queryRemote && GetStatusWithChangelists(dirs, result, changelistAssoc, "empty", true);
	if (!files.empty() && bothOk)
		bothOk = queryRemote && GetStatusWithChangelists(files, result, changelistAssoc, "files", true);

	if (!bothOk)
	{
		// try offline status
		result.clear();
		changelistAssoc.clear();
		
		if (!dirs.empty())
			GetStatusWithChangelists(dirs, result, changelistAssoc, "empty", false);
		if (!files.empty())
			GetStatusWithChangelists(files, result, changelistAssoc, "files", false);
	}
}
#endif // DEPRECATED

bool SvnTask::HandleConnectErrorLine(const std::string& line)
{
	bool connectError = line.find("Unable to connect to a repository") != string::npos;
	bool svnVersionError = 
		line.find("appears to be part of a Subversion") != string::npos &&
		EndsWith(line, " or greater");

	if (connectError)
	{
		string msg = "Could not connect subversion to repository ";
		string::size_type si = line.find('\'');
		if (si != string::npos)
			msg += line.substr(si);
		m_Connection->WarnLine(msg, MAProtocol);
		NotifyOffline(msg);
	} 
	else if (svnVersionError)
	{
		string::size_type i = line.find("appears to be part of a Subversion");
		string msg = "Project ";
		msg += line.substr(i);
		msg += ". The unity builtin svn is a lower version. You can specify a custom svn path in Editor Settings.";
		m_Connection->WarnLine(msg, MAProtocol);
		NotifyOffline(msg);
	}
	
	return connectError || svnVersionError;
}

bool SvnTask::GetStatusWithChangelists(const VersionedAssetList& assets, 
									   VersionedAssetList& result, 
									   set<string>& resultPaths,
									   const char* depth, bool queryRemote)
{
	const int MIN_STATUS_LINE_LENGTH = 32;
	string cmd = "status --verbose ";
	if (queryRemote)
		cmd += "--show-updates ";
	cmd += "--depth ";
	cmd += depth;
	cmd += " ";
	cmd += Join(Paths(assets), " ", "\"");

	m_Connection->Progress(-1, 0, "secs. while reading status...");

	APOpen ppipe = RunCommand(cmd);

	string line;
	string currentChangelist;
	bool reposVerified = false;

	time_t lastTick = time(NULL);
	time_t startTick = lastTick;

	// TODO: parse XML instead of this not so stable output

	while (ppipe->ReadLine(line))
	{
		time_t thisTick = time(NULL);
		if (lastTick != thisTick)
		{
			m_Connection->Progress(-1, thisTick - startTick, "secs. while reading status...");
			lastTick = thisTick;
		}

		m_Connection->Log().Debug() << line << Endl;
		if (EndsWith(line, " was not found."))
			continue;

		if (EndsWith(line, "is not a working copy"))
		{
			// Special case: If "EditorSettings.asset" is local if means that the repository is not valid and we mark it as such
			if (EndsWith(line, "\\ProjectSettings\\EditorSettings.asset' is not a working copy") || EndsWith(line, "/ProjectSettings/EditorSettings.asset' is not a working copy"))
			{
				m_Connection->WarnLine("EditorSettings.asset file not part of a subversion working copy\nRemember to add at least the 'Assets' folder and meta file to subversion");
				NotifyOffline("Project is not in a subversion working folder", true);
				return true;
			}

			string::size_type i1 = line.find("'");
			string::size_type i2 = line.find("'", i1+1); 
			string filename = line.substr(i1+1, i2-i1-1);
			char endchar = filename[filename.length()-1];
			if (endchar != '/' && endchar != '\'' && IsDirectory(filename))
				filename += "/";
			VersionedAsset newAsset(Replace(filename, "\\", "/"), kLocal, "");
			result.push_back(newAsset);
			resultPaths.insert(newAsset.GetPath());
			continue;
		}
		
		if (!reposVerified)
		{ 
			if (HandleConnectErrorLine(line))
				return false; // connect error
		
			if (queryRemote)
				NotifyOnline();

			reposVerified = true;
		}

		// Each line is a file status. The first 9 chars indicates
		// status for different areas. Following at are space separated
		// fields in order: working revision, last commited revision, last commited author
		// The last field is the file path and can include spaces
		if (StartsWith(line, "--- Changelist"))
		{
			const int prefixLen = 15; // "-- Changelist '" length
			currentChangelist = Trim(TrimEnd(line.substr(15), ':'), '\'');
			continue;
		}

		if (line.length() < MIN_STATUS_LINE_LENGTH || StartsWith(line, "Status against"))
			continue; // 

		int state = kNone;

		try 
		{
			state = ParseChangeState(state, line[0]);
			if ((state & kLocal) == 0)
			{
				state = ParsePropertyState(state, line[1]);
				state = ParseWorkingDirLockState(state, line[2]);
				//state = ParseHistoryState(state, line[3]);
				//state = ParseSwitchState(state, line[4]);
				state = ParseLockState(state, line[5]);
				state = ParseConflictState(state, line[6]);
				// line[7] always blank
				state = ParseSyncedState(state, line[8]);
			}
		}
		catch (SvnException& ex)
		{
			throw SvnException(string("Parsing: ") + line + " Got: " + ex.what());
		}

		/*
		// Skip the first two fields of each 8 char and a separator
		// Then skip the username and the spaces after that in order
		// to get the filename
		string::size_type i = 28; // start of username
		i = str.find(' ', 28);
		Enforce<SvnException>(i != string::npos, 
							  string("Invalid status line: ") + line);
		i = str.find_first_not_of(' ', i);

		Enforce<SvnException>(i != string::npos, 
							  string("Invalid status line : ") + line);
		string filename = TrimEnd(line.substr(i), '\n');
		*/

		// Revision is idx 11-19
		string revision = TrimStart(line.substr(10, 8), ' ');

		// Filename is after usename which starts at idx 28 or 29
		string::size_type i1 = line.find(' ', line[28] == ' ' ? 29 : 28);
		i1 = line.find_first_not_of(' ', i1);
		string filename = line.substr(i1);
		char endchar = filename[filename.length()-1];
		if (endchar != '/' && endchar != '\'' && IsDirectory(filename))
			filename += "/";
		VersionedAsset theAsset(Replace(filename, "\\", "/"), state, revision);
		theAsset.SetChangeListID(currentChangelist);
		result.push_back(theAsset);
		resultPaths.insert(theAsset.GetPath());
		/*

?                                        Library
A                -       ?   ?           Assets/Red.mat.meta
?                    p4plugin.log		
M                4        4 nevin@unity3d.com Assets/test.unity
                 1        1 jonasd       Assets/NewBehaviourScript.cs		
A                0       ?   ?           hello.txt		
        *        1        1 jonasd       Assets
?                                        ProjectSettings
        *                                Assets/hello.txt

		*/
	}
	
	if (queryRemote)
		NotifyOnline();

	return true;
}

bool isDigit(char c)
{
	return isdigit((int)c) != 0;
}


void SvnTask::GetLog(SvnLogResult& result, const std::string& from, const std::string& to, bool includeAssets)
{
	// Check cache first
	int cacheRev = 0;
	if (from == to)
	{
		cacheRev = atoi(from.c_str());
		if (cacheRev)
		{
			m_Connection->Log().Debug() << "Checking cache for single log revision " << cacheRev << Endl;
			std::map<int, SvnLogResult::Entry>::iterator i = g_LogCache.find(cacheRev);
			if (i != g_LogCache.end())
			{
				m_Connection->Log().Debug() << "  Found in cache" << Endl;
				result.entries.push_back(i->second);
				return;
			}
			else 
			{
				m_Connection->Log().Debug() << "  Not in cache" << Endl;
			}
		}
	}

	// Make sure log cache is not too big
	const size_t MAX_LOG_CACHE_ASSETS = 3000;
	while (g_LogCacheAssetsCount > MAX_LOG_CACHE_ASSETS && !g_LogCache.empty()) 
	{
		std::map<int, SvnLogResult::Entry>::reverse_iterator i = g_LogCache.rbegin();
		g_LogCacheAssetsCount -= i->second.assets.size();
		g_LogCache.erase(i->first);
	}
	
	const int MIN_HEADER_LINE_LENGTH = 30;
	string cmd = "log -r ";
	cmd += from;
	cmd += ":";
	cmd += to;

	if (includeAssets)
		cmd += " -v ";

	m_Connection->Progress(-1, 0, "secs. while reading log...");

	APOpen ppipe = RunCommand(cmd);

	string line;
	
	vector<string> toks;
	VersionedAsset asset;
	bool reposVerified = false;

	time_t lastTick = time(NULL);
	time_t startTick = lastTick;

	while (ppipe->ReadLine(line))
	{
		time_t thisTick = time(NULL);
		if (lastTick != thisTick)
		{
			m_Connection->Progress(-1, thisTick - startTick, "secs. while reading log...");
			lastTick = thisTick;
		}

		m_Connection->Log().Debug() << line << Endl;
		if (EndsWith(line, "is not a working copy"))
		{
			NotifyOffline("Project is not in a subversion working folder", true);
			return;
		}
		
		if (!reposVerified)
		{ 
			if (HandleConnectErrorLine(line))
				return; // connect error
		
			reposVerified = true;
		}

		Enforce<SvnException>(StartsWith(line, "--------------"), string("Invalid log header top: ") + line);
		
		NotifyOnline();

		// Skip first line of "------"
		if (!ppipe->ReadLine(line)) 
			break; 
		
		m_Connection->Log().Debug() << line << Endl;
		Enforce<SvnException>(line.length() >= MIN_HEADER_LINE_LENGTH && line[0] == 'r',
							  string("Invalid log header: ") + line);
		
		toks.clear();
		size_t size = Tokenize(toks, line, "|");
		Enforce<SvnException>(size == 4, "Wrong number of fields in log header");
		
		SvnLogResult::Entry& entry = result.CreateEntry();
		entry.revision = TrimEnd(toks[0]).substr(1); // strip first 'r'
		entry.committer = Trim(toks[1]);
		entry.timestamp = Trim(toks[2]);
		std::string linesField = TrimStart(toks[3]);
		linesField = linesField.substr(0, linesField.find(' '));
		long messageLineCount = atol(linesField.c_str());
		
		if (includeAssets)
		{
		
			// Skip first line which is "Changed paths:"
			if (!ppipe->ReadLine(line)) 
				break;
			
			m_Connection->Log().Debug() << line << Endl;
			
			// Read until blank line which delimits asset files
			while (ppipe->ReadLine(line))
			{
				if (line.empty())
					break;
				
				m_Connection->Log().Debug() << line << Endl;

				asset.Reset();
				string::size_type i1 = line.find_first_not_of(' ');
				string::size_type i2 = line.find(' ', i1);
				Enforce<SvnException>(i1 != string::npos && i2 != string::npos,
									  "Malformed changed file path during svn log");

				string mod = TrimStart(line.substr(i1, i2 - i1));
				Enforce<SvnException>(!mod.empty(), "Empty changed file flag during svn log");
				
				bool checkCopy = false;
				switch (mod[0])
				{
				case 'A':
					asset.SetState(kAddedRemote);
					checkCopy = true;
					break;
				case 'D':
					asset.SetState(kDeletedRemote);
					break;
				case 'M':
					asset.SetState(kCheckedOutRemote);
					break;
				case 'R':
					asset.SetState(kCheckedOutRemote);
					checkCopy = true;
					break;
				default:
					throw SvnException("Invalid changed file state during svn log");
				}
				
				line = line.substr(i2+1);
				
				// Since this is verbose mode the copied_from file is listed
				// after the listed files that have been copied into. Strip that part.
				i1 = checkCopy ? line.rfind(':') : string::npos;
				if (i1 != string::npos)
				{
					// make sure the part from i1 to the end is only digits
					string copyRev = line.substr(i1+1, line.length() - i1 - 2); // eat first : and last )
					
					i2 = line.rfind(" from ", i1);
					i1 = i2 == string::npos ? line.rfind("(from ", i1) : i2;
					if (i1 != string::npos && 
						partition(copyRev.begin(), copyRev.end(), isDigit) == copyRev.end())
					{
						// all is digits and we have a " from " string.
						line = line.substr(0, i1);
					}
				}

				string filename = line.substr(1);
				char endchar = filename[filename.length()-1];
				if (endchar != '/' && endchar != '\'' && IsDirectory(filename))
					filename += "/";

				asset.SetPath(TrimStart(Replace(filename, "\\", "/"), '/'));
				asset.SetRevision(entry.revision);
				entry.assets.push_back(asset);
			}

		}
		else
		{
			// Skip first line of message which is blank
			if (!ppipe->ReadLine(line)) 
				break;
			m_Connection->Log().Debug() << line << Endl;
		}
		
		for (long i = 0; i < messageLineCount; i++)
		{
			if (!ppipe->ReadLine(line)) 
				break;
			m_Connection->Log().Debug() << line << Endl;
			entry.message += line + "\n";
		}

		int rev = atoi(entry.revision.c_str());
		if (rev && includeAssets)
		{
			g_LogCache[rev] = entry;
			g_LogCacheAssetsCount += entry.assets.size();
		}
	}
}

void SvnTask::NotifyOffline(const std::string& reason, bool invalidWorkingCopy)
{
	const char* disableCmdsWorkingCopy[]  = { 
		/*"add",  
		"changeDescription", "changeMove",
		"changes", "changeStatus", "checkout",
		"deleteChanges", 
		"delete", */ "download",
		"getLatest", "incomingChangeAssets", "incoming",
		"lock", /* "move", "resolve",
		"revertChanges", "revert", "status", */
		"submit", "unlock", 
		0
	};
	const char* disableCmdsNonWorkingCopy[]  = { 
		"add",  
		"changeDescription", "changeMove",
		"changes", "changeStatus", "checkout",
		"deleteChanges", 
		"delete", "download",
		"getLatest", "incomingChangeAssets", "incoming",
		"lock", "move", "resolve",
		"revertChanges", "revert", "status",
		"submit", "unlock", 
		0
	};

	const char** disableCmds = invalidWorkingCopy ? disableCmdsNonWorkingCopy : disableCmdsWorkingCopy;

	m_IsOnline = false;

	int i = 0;
	while (disableCmds[i])
	{
		m_Connection->Command(string("disableCommand ") + disableCmds[i], MAProtocol);
		++i;
	}

	m_Connection->Command(string("offline ") + reason, MAProtocol);

	g_LogCacheAssetsCount = 0;
	g_LogCache.clear();
}

void SvnTask::NotifyOnline()
{
	const char* enableCmds[]  = { 
		"add",  
		"changeDescription", "changeMove",
		"changes", "changeStatus", /* "checkout", */
		/* "deleteChanges", */ 
		"delete", "download",
		"getLatest", "incomingChangeAssets", "incoming",
		"lock", "move", "resolve",
		"revertChanges", "revert", "status", 
		"submit", "unlock", 
		0
	};

	if (m_IsOnline) 
		return; // nothing to notify

	int i = 0;
	while (enableCmds[i])
	{
		m_Connection->Command(string("enableCommand ") + enableCmds[i], MAProtocol);
		++i;
	}

	const char* disableCmds[]  = { 
		"checkout",
		"deleteChanges", 
		0
	};
	i = 0;
	while (disableCmds[i])
	{
		m_Connection->Command(string("disableCommand ") + disableCmds[i], MAProtocol);
		++i;
	}
	m_IsOnline = true;
	m_Connection->Command("online");
	g_LogCacheAssetsCount = 0;
	g_LogCache.clear();
}

SvnException::SvnException(const std::string& about) : m_What(about) {}

const char* SvnException::what() const throw()
{
	return m_What.c_str();
}
