IF (WIN32)
	CM_CONFIGURE_MSVC_RUNTIME()
ENDIF (WIN32)

CM_TARGET_BEGIN(
	SvnPlugin
	EXECUTABLE
	.
	SubversionPlugin
)	
	# Dependencies
	# -----------------------------------------------
	
	# Includes
	# -----------------------------------------------
	CM_TARGET_ADD_INCLUDES(../Common)
	
	# Defines
	# -----------------------------------------------
	
	# Sources
	# -----------------------------------------------
	CM_TARGET_ADD_SOURCES( 
		"Common"
		../Common/Changes.h
	    ../Common/Changes.cpp
	    ../Common/Command.h
	    ../Common/Command.cpp
	    ../Common/CommandLine.h
	    ../Common/CommandLine.cpp
	    ../Common/Connection.h
	    ../Common/Connection.cpp
	    ../Common/Dispatch.h
	    ../Common/FileSystem.h
	    ../Common/FileSystem.cpp
	    ../Common/Log.h
	    ../Common/Log.cpp
	    ../Common/Pipe.h
	    ../Common/Pipe.cpp
	    ../Common/POpen.h
	    ../Common/POpen.cpp
	    ../Common/Status.h
	    ../Common/Status.cpp
	    ../Common/Utility.h
	    ../Common/Utility.cpp
	    ../Common/VersionedAsset.h
	    ../Common/VersionedAsset.cpp
	)
	
	CM_TARGET_ADD_SOURCES( 
		"Sources"
		./Source/SvnTask.cpp
		./Source/AllSvnCommands.h
		./Source/SvnAddCommand.h
		./Source/SvnChangeDescription.h
		./Source/SvnCheckoutCommand.h
		./Source/SvnCommand.h
		./Source/SvnConfigCommand.h
		./Source/SvnDeleteCommand.h
		./Source/SvnDownloadCommand.h
		./Source/SvnGetLatestCommand.h
		./Source/SvnIncomingAssetsCommand.h
		./Source/SvnIncomingCommand.h
		./Source/SvnLockCommand.h
		./Source/SvnMoveChangelistCommand.h
		./Source/SvnMoveCommand.h
		./Source/SvnOutgoingAssetsCommand.h
		./Source/SvnOutgoingCommand.h
		./Source/SvnResolveCommand.h
		./Source/SvnRevertCommand.h
		./Source/SvnStatusCommand.h
		./Source/SvnSubmitCommand.h
		./Source/SvnTask.h
		./Source/SvnUnlockCommand.h
	)
	
	IF (APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_OSX.cpp
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_OSX.cpp
			./Source/SvnPlugin_WIN.cpp
		)
	ENDIF (WIN32)
	
	IF (UNIX)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_OSX.cpp
		)
	ENDIF (UNIX)
	
	# Libraries
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LIBRARIES(
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_LIBRARIES(
			shlwapi.lib
		)
	ENDIF (WIN32)
	
	IF (UNIX)
		CM_TARGET_ADD_LIBRARIES(
		)
	ENDIF (UNIX)
	
	# Link flags
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LINK_FLAGS ( "-arch i386 -g -framework Cocoa -lstdc++" )
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_LINK_FLAGS ( "" )
	ENDIF (WIN32)

	IF (UNIX)
		CM_TARGET_ADD_LINK_FLAGS ( "-g -fpermissive -lrt -ldl -lstdc++" )
	ENDIF (UNIX)
	
# End of target definition
# -----------------------------------------------
CM_TARGET_END()