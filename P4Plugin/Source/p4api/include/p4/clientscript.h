/*
 * Copyright 1995, 2019 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

# ifdef HAS_EXTENSIONS

// This enum represents the result of a client-side Extension function.

enum class ClientScriptAction
{
	UNKNOWN,      // Script misbehaving / crashed / etc.
	FAIL,         // Script says 'no'.
	PASS,         // Script says 'ok'.
	REPLACE,      // Script does something instead of what would happen.
	PRE_DEBUG,    // Non-functional divider between normal user-facing
	              // scripts and internal debug code.
	ABORT,        // Tell the caller to abort or otherwise exit immediately.
	EARLY_RETURN  // Tell the caller to return control to its parent func.
};

class ClientScript
{
	public:

	    ClientScript( Client* c );
	    virtual ~ClientScript();

	    bool CanLoad() const;
	    bool BuildCheck() const;
	    void SetClient( Client* c );
	    void SetSearchPath( const char* where );
	    void SetSearchPattern( const char* what );

	    std::vector< std::unique_ptr< Extension > >& GetExts();

	    virtual void LoadScripts( const bool search, Error* e );

	    virtual std::tuple< ClientScriptAction, int >
	             Run( const char* cmd, const char* func,
	                  ClientUser* u, const bool noReplace,
	                  Error* e );

	    static SCR_VERSION scrVerFromFileName( const char* file );

	private:

	    std::vector< std::tuple< std::string, SCR_VERSION > >
	    FindLooseExts( const StrPtr& start, const bool search, Error* e );

	    std::vector< std::unique_ptr< Extension > > exts;

	    std::vector< std::string > patterns;
	    StrBuf path;
	    Client* client;
};

# else

struct ClientScriptAction
{
	static const int UNKNOWN = 0;
	static const int FAIL = 1;
	static const int PASS = 2;
	static const int REPLACE = 3;

	bool operator ==( const int o ) const
	{
	    // Ensure that we can only ever pass in the stub.
	    return o == PASS;
	}
};

class ClientScript
{
	public:

	    ClientScript( Client* c ){}
	    ~ClientScript(){}

	    bool CanLoad() const { return false; }
	    bool BuildCheck() const { return false; }

	    int& GetExts();

	    void LoadScripts( const bool search, Error* e ){}

	    ClientScriptAction
	    Run( const char* cmd, const char* func,
	         ClientUser *u, const bool noReplace,
	         Error* e );

	    void SetClient( Client* c );
	    void SetSearchPath( const char* where );
	    void SetSearchPattern( const char* what );

};

# endif
