CM_TARGET_BEGIN(
	PlasticSCMPlugin
	CUSTOM
	.
	PlasticSCMPlugin
)	
	# Copy
	# -----------------------------------------------
	#CM_TARGET_ADD_COPY_FILES(${CMAKE_CURRENT_SOURCE_DIR}/PlasticSCMPlugin ${CM_ROOT_DIRECTORY}/${CONFIGURATION}/PlasticSCMPlugin)
	CM_TARGET_ADD_COPY_FILES(${CMAKE_CURRENT_SOURCE_DIR}/PlasticSCMPlugin ${CM_ROOT_DIRECTORY}/Debug/PlasticSCMPlugin)
	
# End of target definition
# -----------------------------------------------
CM_TARGET_END()