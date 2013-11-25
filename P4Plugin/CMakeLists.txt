IF (WIN32)
	CM_CONFIGURE_MSVC_RUNTIME()
ENDIF (WIN32)

CM_TARGET_BEGIN(
	P4Plugin
	EXECUTABLE
	.
	PerforcePlugin
)	

	# Dependencies
	# -----------------------------------------------
	
	# Includes
	# -----------------------------------------------
	CM_TARGET_ADD_INCLUDES(../Common)
	CM_TARGET_ADD_INCLUDES(./Source/r12.2/include/p4)
	
	# Defines
	# -----------------------------------------------
	IF (WIN32)
		CM_TARGET_ADD_DEFINES(-D_CRT_SECURE_NO_WARNINGS)
		CM_TARGET_ADD_DEFINES(-D_CRT_SECURE_NODEPRECATE)
		CM_TARGET_ADD_DEFINES(-D_CRT_NONSTDC_NO_DEPRECATE)
		CM_TARGET_ADD_DEFINES(-D_SCL_SECURE_NO_DEPRECATE)
		CM_TARGET_ADD_DEFINES(-D_SECURE_SCL=0)
		CM_TARGET_ADD_DEFINES(-D_WINDOWS)
		CM_TARGET_ADD_DEFINES(-DWIN32)
	ENDIF (WIN32)
	
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
		./Source/P4AddCommand.cpp
		./Source/P4ChangeDescriptionCommand.cpp
		./Source/P4ChangeMoveCommand.cpp
		./Source/P4ChangesCommand.cpp
		./Source/P4ChangeStatusCommand.cpp
		./Source/P4CheckoutCommand.cpp
		./Source/P4Command.h
		./Source/P4Command.cpp
		./Source/P4ConfigCommand.cpp
		./Source/P4DeleteChangesCommand.cpp
		./Source/P4DeleteCommand.cpp
		./Source/P4DownloadCommand.cpp
		./Source/P4ExitCommand.cpp
		./Source/P4FileSetBaseCommand.h
		./Source/P4FileSetBaseCommand.cpp
		./Source/P4GetLatestCommand.cpp
		./Source/P4IncomingChangeAssetsCommand.cpp
		./Source/P4IncomingCommand.cpp
		./Source/P4LockCommand.cpp
		./Source/P4LoginCommand.cpp
		./Source/P4LogoutCommand.cpp
		./Source/P4MoveCommand.cpp
		./Source/P4QueryConfigParametersCommand.cpp
		./Source/P4ResolveCommand.cpp
		./Source/P4RevertChangesCommand.cpp
		./Source/P4RevertCommand.cpp
		./Source/P4SpecCommand.cpp
		./Source/P4StatusBaseCommand.h
		./Source/P4StatusBaseCommand.cpp
		./Source/P4StatusCommand.h
		./Source/P4StatusCommand.cpp
		./Source/P4SubmitCommand.cpp
		./Source/P4FileModeCommand.cpp
		./Source/P4Task.h
		./Source/P4Task.cpp
		./Source/P4UnlockCommand.cpp
		./Source/P4Utility.h
		./Source/P4Utility.cpp
	)
	
	CM_TARGET_ADD_SOURCES( 
		"External"
		./Source/r12.2/include/p4/charcvt.h
		./Source/r12.2/include/p4/clientapi.h
		./Source/r12.2/include/p4/clientmerge.h
		./Source/r12.2/include/p4/clientprog.h
		./Source/r12.2/include/p4/clientresolvea.h
		./Source/r12.2/include/p4/clientuser.h
		./Source/r12.2/include/p4/datetime.h
		./Source/r12.2/include/p4/debug.h
		./Source/r12.2/include/p4/diff.h
		./Source/r12.2/include/p4/echoctl.h
		./Source/r12.2/include/p4/enviro.h
		./Source/r12.2/include/p4/error.h
		./Source/r12.2/include/p4/errorlog.h
		./Source/r12.2/include/p4/errornum.h
		./Source/r12.2/include/p4/filesys.h
		./Source/r12.2/include/p4/handler.h
		./Source/r12.2/include/p4/hostenv.h
		./Source/r12.2/include/p4/i18napi.h
		./Source/r12.2/include/p4/ident.h
		./Source/r12.2/include/p4/keepalive.h
		./Source/r12.2/include/p4/mapapi.h
		./Source/r12.2/include/p4/msgclient.h
		./Source/r12.2/include/p4/msgserver.h
		./Source/r12.2/include/p4/netconnect.h
		./Source/r12.2/include/p4/netportparser.h
		./Source/r12.2/include/p4/ntservice.h
		./Source/r12.2/include/p4/options.h
		./Source/r12.2/include/p4/p4tags.h
		./Source/r12.2/include/p4/pathsys.h
		./Source/r12.2/include/p4/runcmd.h
		./Source/r12.2/include/p4/signaler.h
		./Source/r12.2/include/p4/spec.h
		./Source/r12.2/include/p4/stdhdrs.h
		./Source/r12.2/include/p4/strarray.h
		./Source/r12.2/include/p4/strbuf.h
		./Source/r12.2/include/p4/strdict.h
		./Source/r12.2/include/p4/strops.h
		./Source/r12.2/include/p4/strtable.h
		./Source/r12.2/include/p4/threading.h
		./Source/r12.2/include/p4/vararray.h
	)
	
	IF (APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/P4Plugin_Posix.cpp
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/P4Plugin_WIN.cpp
		)
	ENDIF (WIN32)

	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/P4Plugin_Posix.cpp
		)
	ENDIF (UNIX AND NOT APPLE)
	
	# Libraries
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LIBRARIES(
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/libclient.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/librpc.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/libp4sslstub.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/libsupp.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/libssl.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/osx32/libcrypto.a
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_LIBRARIES(
			shlwapi.lib
			ws2_32.lib
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/win32${CM_CONFIGURATION}/libclient.lib
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/win32${CM_CONFIGURATION}/librpc.lib
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/win32${CM_CONFIGURATION}/libsupp.lib
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/win32${CM_CONFIGURATION}/ssleay32MTd.lib
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/win32${CM_CONFIGURATION}/libeay32MTd.lib
		)
	ENDIF (WIN32)
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_LIBRARIES(
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/libclient.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/librpc.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/libp4sslstub.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/libsupp.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/libssl.a
			${CMAKE_CURRENT_SOURCE_DIR}/Source/r12.2/lib/linux32/libcrypto.a
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