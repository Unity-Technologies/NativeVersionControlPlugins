#pragma once
#include <vector>
#include <string>
#include <set>
#include "Connection.h"
#include "P4Task.h"
#include "VersionedAsset.h"

typedef std::vector<std::string> CommandArgs;
Connection& operator<<(Connection& p, const VCSStatus& v);
Connection& SendToConnection(Connection& p, const VCSStatus& st, MessageArea ma);

/* 
 * Base class for all commands that unity can issue and is supported
 * by the perforce plugin
 */

class P4Command : public ClientUser
{
public:
	virtual bool Run(P4Task& task, const CommandArgs& args) = 0;
	
	const VCSStatus& GetStatus() const;
	VCSStatus& GetStatus();

	// Error* GetError();
	bool HasErrors() const;
	void ClearStatus();
	
	std::string GetStatusMessage() const;

	bool ConnectAllowed(); // is this command allowed to initiate a connect
	
	// ClientUser overides
	void HandleError( Error *err );
	void OutputError( const char *errBuf );
	void ErrorPause( char* errBuf, Error* e);
	void OutputInfo( char level, const char *data );
	void OutputStat( StrDict *varList );
	void OutputText( const char *data, int length);
	void OutputBinary( const char *data, int length);
	void InputData( StrBuf *buf, Error *err );
	void Prompt( const StrPtr &msg, StrBuf &buf, int noEcho ,Error *e );
	void Finished();

	static Connection& Conn() { return *s_Conn; }
	
protected:
	P4Command(const char* name);
	static bool HandleOnlineStatusOnError(Error *err);

	// Many of the derived classes need to send updated
	// state of a list. This is a convenience method to do that.
	static void RunAndSendStatus(P4Task& task, const VersionedAssetList& assetList);
	static void RunAndGetStatus(P4Task& task, const VersionedAssetList& assetList, VersionedAssetList& result);
	bool m_AllowConnect;

	struct Mapping 
	{
		std::string depotPath;
		std::string clientPath;
	};

	static const std::vector<Mapping>& GetMappings(P4Task& task, const VersionedAssetList& assetList);
	static bool MapToLocal(P4Task& task, VersionedAssetList& assets);

	friend class P4Task;
	static Connection* s_Conn;
private:
	VCSStatus m_Status;
};

// Lookup command handler by name
P4Command* LookupCommand(const std::string& name);
