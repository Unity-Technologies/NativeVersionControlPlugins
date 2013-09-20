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
		./Source/SvnPlugin_OSX.cpp
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
	
	IF (APPLE OR UNIX)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_OSX.cpp
		)
	ENDIF (APPLE OR UNIX)
	
	IF (WIN32 AND NOT UNIX)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_WIN.cpp
		)
	ENDIF (WIN32 AND NOT UNIX)
	
	# Libraries
	# -----------------------------------------------
	CM_TARGET_ADD_LIBRARIES(
	)
	
	# Link flags
	# -----------------------------------------------
	CM_TARGET_ADD_LINK_FLAGS ( "-v -arch i386 -g -framework Cocoa -lstdc++" )
	
# End of target definition
# -----------------------------------------------
CM_TARGET_END()