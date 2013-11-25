CM_TARGET_BEGIN(
	PlasticSCMPlugin
	CUSTOM
	.
	PlasticSCMPlugin
)	
	# Copy
	# -----------------------------------------------
	CM_TARGET_ADD_COPY_FILES(${CMAKE_CURRENT_SOURCE_DIR}/PlasticSCMPlugin ${CM_BIN_DIR}/PlasticSCMPlugin)
	
# End of target definition
# -----------------------------------------------
CM_TARGET_END()