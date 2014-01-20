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
	IF (APPLE)
		CM_TARGET_ADD_DEFINES(
		)
	ENDIF (APPLE)

	IF (WIN32)
		CM_TARGET_ADD_DEFINES(
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_SECURE_NODEPRECATE
			-D_CRT_NONSTDC_NO_DEPRECATE
			-D_SCL_SECURE_NO_DEPRECATE
			-D_SECURE_SCL=0
			-D_WINDOWS
			-DWIN32
		)
	ENDIF (WIN32)

	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_DEFINES(
		)
	ENDIF (UNIX AND NOT APPLE)
	
	# Sources
	# -----------------------------------------------
	CM_TARGET_ADD_SOURCES( 
		"Common"
        ../Common/Changes.cpp
        ../Common/Changes.h
        ../Common/Command.cpp
        ../Common/Command.h
        ../Common/CommandLine.cpp
        ../Common/CommandLine.h
        ../Common/Connection.cpp
        ../Common/Connection.h
        ../Common/Dispatch.h
        ../Common/FileSystem.cpp
        ../Common/FileSystem.h
        ../Common/Log.cpp
        ../Common/Log.h
        ../Common/Pipe.cpp
        ../Common/Pipe.h
        ../Common/POpen.cpp
        ../Common/POpen.h
        ../Common/Status.cpp
        ../Common/Status.h
        ../Common/Utility.cpp
        ../Common/Utility.h
        ../Common/VersionControlPlugin.cpp
        ../Common/VersionControlPlugin.h
        ../Common/VersionedAsset.cpp
        ../Common/VersionedAsset.h
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
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/SvnPlugin_OSX.cpp
		)
	ENDIF (UNIX AND NOT APPLE)
	
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
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_LIBRARIES(
		)
	ENDIF (UNIX AND NOT APPLE)
	
	# Link flags
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LINK_FLAGS ( "-arch i386 -g -framework Cocoa -lstdc++" )
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_LINK_FLAGS ( "" )
	ENDIF (WIN32)

	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_LINK_FLAGS ( "-g -fpermissive -lrt -ldl -lstdc++" )
	ENDIF (UNIX AND NOT APPLE)
	
# End of target definition
# -----------------------------------------------
CM_TARGET_END()