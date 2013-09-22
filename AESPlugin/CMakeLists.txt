IF (WIN32)
	CM_CONFIGURE_MSVC_RUNTIME()
ENDIF (WIN32)

CM_TARGET_BEGIN(
	AESPlugin
	EXECUTABLE
	.
	AESPlugin
)	
	# Dependencies
	# -----------------------------------------------
	
	# Includes
	# -----------------------------------------------
	CM_TARGET_ADD_INCLUDES(../Common)
	CM_TARGET_ADD_INCLUDES(./External/openSSL/include)
	CM_TARGET_ADD_INCLUDES(./External/simpleJSON)
	
	IF (APPLE)
		CM_TARGET_ADD_INCLUDES(./External/curl/macosx32/include)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_INCLUDES(./External/curl/win32/include)
	ENDIF (WIN32)
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_INCLUDES(./External/curl/linux32/include)
	ENDIF (UNIX AND NOT APPLE)
	
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
		${CMAKE_CURRENT_SOURCE_DIR}/External/simpleJSON/JSON.h
		${CMAKE_CURRENT_SOURCE_DIR}/External/simpleJSON/JSON.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/External/simpleJSON/JSONValue.h
		${CMAKE_CURRENT_SOURCE_DIR}/External/simpleJSON/JSONValue.cpp
	)
	
	IF (APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/AESPlugin_OSX.cpp
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/AESPlugin_WIN.cpp
		)
	ENDIF (WIN32)
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_SOURCES( 
			"Sources"
			./Source/AESPlugin_Posix.cpp
		)
	ENDIF (UNIX AND NOT APPLE)
	
	# Libraries
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LIBRARIES(
			${CMAKE_CURRENT_SOURCE_DIR}/External/curl/macosx32/lib/libcurl.a
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/macosx32/lib/libcrypto.a
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/macosx32/lib/libssl.a
		)
	ENDIF (APPLE)
	
	IF (WIN32)
		CM_TARGET_ADD_LIBRARIES(
			shlwapi.lib
			${CMAKE_CURRENT_SOURCE_DIR}/External/curl/win32/lib/libcurl_a_X86.lib
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/macosx32/lib/libeay32.lib
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/macosx32/lib/ssleay32.lib
		)
	ENDIF (WIN32)
	
	IF (UNIX AND NOT APPLE)
		CM_TARGET_ADD_LIBRARIES(
			${CMAKE_CURRENT_SOURCE_DIR}/External/curl/linux32/lib/libcurl.a
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/linux32/lib/libcrypto.a
			${CMAKE_CURRENT_SOURCE_DIR}/External/openSSL/linux32/lib/libssl.a
		)
	ENDIF (UNIX AND NOT APPLE)
	
	# Link flags
	# -----------------------------------------------
	IF (APPLE)
		CM_TARGET_ADD_LINK_FLAGS ( "-arch i386 -g -framework Cocoa -lstdc++ -lz" )
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