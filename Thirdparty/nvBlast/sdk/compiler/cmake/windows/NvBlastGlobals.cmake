#
# Build NvBlastGlobals Windows
#

SET(BLASTGLOBALS_COMPILE_DEFS
	# Common to all configurations
	${BLAST_SLN_COMPILE_DEFS};_CONSOLE
	
	$<$<CONFIG:debug>:${BLAST_SLN_DEBUG_COMPILE_DEFS}>
	$<$<CONFIG:checked>:${BLAST_SLN_CHECKED_COMPILE_DEFS}>
	$<$<CONFIG:profile>:${BLAST_SLN_PROFILE_COMPILE_DEFS}>
	$<$<CONFIG:release>:${BLAST_SLN_RELEASE_COMPILE_DEFS}>
)
