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

// Shim between higher level Rh* data and the plumbing.

struct ExtensionCallerData
{
	virtual ~ExtensionCallerData();

	// Extensions

	std::function< void( const char* msg ) > SetClientMsg
	    = []( const char* msg ){};
	std::function< void() > SetExtExecError
	    = [](){};
	StrBuf archDir, dataDir;

	std::string defaultLocale; // manifest
	std::string userLocale, userCharset, userLanguage; // client vars
	std::vector< std::string > supportedLocales; // manifest
	std::string setLocale; // extension-supplied or default
	// locale -> key -> message
	std::unordered_map< std::string, std::unordered_map< std::string,
	                                                     std::string > >
	    translationMap;

	int apiVersion = 0;
} ;

class Extension : public p4script
{
	public:

	     Extension( const SCR_VERSION v, const int apiVersion,
	                std::optional<
	                std::unique_ptr< ExtensionCallerData > > ecd, Error* e,
	                const bool alloc = false );
	    virtual ~Extension();

	    void LoadFile( const char* file, Error *e );
	    virtual void doBindings( Error* e );
	    ExtensionCallerData* GetECD();

	    std::optional< std::any > RunCallBack( const char* name, Error* e );

	protected:

	    class extImpl;
	    class extImpl53;

	    std::unique_ptr< extImpl > rhePimpl;

	private:

	    std::optional< std::unique_ptr< ExtensionCallerData > > ecd;
} ;

class Extension::extImpl
{
	public:

	             extImpl( Extension& p, Error *e );
	    virtual ~extImpl();

	    virtual void doBindings( Error* e ) = 0;

	    virtual std::optional< std::any >
	    RunCallBack( const char* name, Error* e ) = 0;

	protected:

	    Extension& parent;
} ;

class Extension::extImpl53 : public Extension::extImpl
{
	public:

	     extImpl53( Extension& p, Error *e );
	    virtual ~extImpl53();

	    virtual void doBindings( Error* e );

	    std::optional< std::any > RunCallBack( const char* name, Error* e );
} ;

# else

class FileSys;

struct ExtensionCallerData
{
};

class Extension
{
	public:

	    ExtensionCallerData* GetECD() { return &ecd; }

	private:

	    ExtensionCallerData ecd;
} ;

# endif
