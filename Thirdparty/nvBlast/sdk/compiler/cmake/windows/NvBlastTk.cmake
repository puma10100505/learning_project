#
# Build NvBlastTk Windows
#

SET(BLASTTK_PLATFORM_COMMON_FILES
	${COMMON_SOURCE_DIR}/NvBlastIncludeWindows.h
)

SET(BLASTTK_PLATFORM_INCLUDES
)

SET(BLASTTK_COMPILE_DEFS
	# Common to all configurations
	${BLAST_SLN_COMPILE_DEFS};_CONSOLE
	
	$<$<CONFIG:debug>:${BLAST_SLN_DEBUG_COMPILE_DEFS}>
	$<$<CONFIG:checked>:${BLAST_SLN_CHECKED_COMPILE_DEFS}>
	$<$<CONFIG:profile>:${BLAST_SLN_PROFILE_COMPILE_DEFS}>
	$<$<CONFIG:release>:${BLAST_SLN_RELEASE_COMPILE_DEFS}>
)

SET(BLASTTK_LIBTYPE "SHARED")

SET(BLASTTK_PLATFORM_LINKED_LIBS Rpcrt4)
