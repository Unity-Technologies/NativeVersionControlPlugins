/*
 * Copyright 1995, 2018 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

# ifdef HAS_EXTENSIONS

# include <map>
# include <any>
# include <tuple>
# include <string>
# include <vector>
# include <variant>
# include <optional>
# include <functional>
# include <unordered_map>
# include <unordered_set>

# include <p4script.h>

class FileSys;
class Client;
class ClientUser;

struct ExtensionCallerDataC : public ExtensionCallerData
{
	std::string func, sourcePath;

	Client* client;
	ClientUser* ui;

	std::function< int( StrBuf&, StrBuf&, int argc,
	    std::vector< std::string > argv,
	    std::unordered_map< std::string, std::string > ssodict,
	    Error* ) > loginSSO;
} ;

class ExtensionClient : public Extension
{
	public:

	    ExtensionClient( const SCR_VERSION v, const int apiVersion,
	                     std::optional<
	                     std::unique_ptr< ExtensionCallerData > > ecd,
	                     Error* e );

	    virtual ~ExtensionClient();

	    void DisableExtensionBinding();

	protected:

	    class extImpl53client;

};

class ExtensionClient::extImpl53client : public Extension::extImpl53
{
	public:

	     extImpl53client( Extension& p, Error* e );
	    ~extImpl53client();

	    void doBindings( Error* e );

	    void DisableExtensionBinding();

} ;

# else

# include <p4script.h>

struct ExtensionCallerDataC
{
};

class ExtensionClient : public Extension
{
	public:

	    ExtensionClient( const SCR_VERSION v,
	                     const char* ecd,
	                     Error* e )
	    {
	        e->Set( MsgScript::ExtScriptNotInBuild );
	    }

	    void SetMaxMem( int ){}
	    void SetMaxTime( int ){}
	    bool doFile( const char*, Error* ){ return false; }

	    void DisableExtensionBinding();
};

# endif
