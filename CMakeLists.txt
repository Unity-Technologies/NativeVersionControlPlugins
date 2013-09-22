# Project Definition
# ------------------------------
PROJECT( VersionControl )

CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
INCLUDE( "CMakeSettings.txt" )

# Globals
# -------------------------------------------
IF (APPLE)
	SET( CMAKE_OSX_ARCHITECTURES "i386" )
ENDIF (APPLE)

# Applications
# -------------------------------------------
CM_ADD_DIRECTORY( "./AESPlugin" )
CM_ADD_DIRECTORY( "./P4Plugin" )
CM_ADD_DIRECTORY( "./SvnPlugin" )
#CM_ADD_DIRECTORY( "./PlasticSCMPlugin" )
#CM_ADD_DIRECTORY( "./Test" )