#
# Build NvBlast common
#

SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)
SET(SOLVER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/lowlevel/source)
SET(PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lowlevel/include)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlast.cmake)

SET(COMMON_FILES
	${BLAST_PLATFORM_COMMON_FILES}
	
	${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	${COMMON_SOURCE_DIR}/NvBlastAssert.h
	${COMMON_SOURCE_DIR}/NvBlastAtomic.cpp
	${COMMON_SOURCE_DIR}/NvBlastAtomic.h
	${COMMON_SOURCE_DIR}/NvBlastDLink.h
	${COMMON_SOURCE_DIR}/NvBlastFixedArray.h
	${COMMON_SOURCE_DIR}/NvBlastFixedBitmap.h
	${COMMON_SOURCE_DIR}/NvBlastFixedBoolArray.h
	${COMMON_SOURCE_DIR}/NvBlastFixedPriorityQueue.h
	${COMMON_SOURCE_DIR}/NvBlastGeometry.h
#	${COMMON_SOURCE_DIR}/NvBlastIndexFns.cpp
	${COMMON_SOURCE_DIR}/NvBlastIndexFns.h
	${COMMON_SOURCE_DIR}/NvBlastIteratorBase.h
	${COMMON_SOURCE_DIR}/NvBlastMath.h
	${COMMON_SOURCE_DIR}/NvBlastMemory.h
	${COMMON_SOURCE_DIR}/NvBlastPreprocessorInternal.h
	${COMMON_SOURCE_DIR}/NvBlastTime.cpp
	${COMMON_SOURCE_DIR}/NvBlastTime.h
	${COMMON_SOURCE_DIR}/NvBlastTimers.cpp
)

SET(PUBLIC_FILES
	${PUBLIC_INCLUDE_DIR}/NvBlast.h
	${PUBLIC_INCLUDE_DIR}/NvBlastPreprocessor.h
	${PUBLIC_INCLUDE_DIR}/NvBlastTypes.h
	${PUBLIC_INCLUDE_DIR}/NvCTypes.h
	${PUBLIC_INCLUDE_DIR}/NvPreprocessor.h
)

SET(SOLVER_FILES
	${SOLVER_SOURCE_DIR}/NvBlastActor.cpp
	${SOLVER_SOURCE_DIR}/NvBlastActor.h
	${SOLVER_SOURCE_DIR}/NvBlastFamilyGraph.cpp
	${SOLVER_SOURCE_DIR}/NvBlastFamilyGraph.h
	${SOLVER_SOURCE_DIR}/NvBlastActorSerializationBlock.cpp
	${SOLVER_SOURCE_DIR}/NvBlastActorSerializationBlock.h
	${SOLVER_SOURCE_DIR}/NvBlastAsset.cpp
	${SOLVER_SOURCE_DIR}/NvBlastAssetHelper.cpp
	${SOLVER_SOURCE_DIR}/NvBlastAsset.h
	${SOLVER_SOURCE_DIR}/NvBlastSupportGraph.h
	${SOLVER_SOURCE_DIR}/NvBlastChunkHierarchy.h
	${SOLVER_SOURCE_DIR}/NvBlastFamily.cpp
	${SOLVER_SOURCE_DIR}/NvBlastFamily.h
)

ADD_LIBRARY(NvBlast ${BLAST_LIB_TYPE} 
	${COMMON_FILES}
	${PUBLIC_FILES}
	${SOLVER_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("solver" FILES ${SOLVER_FILES})

# Target specific compile options


TARGET_INCLUDE_DIRECTORIES(NvBlast 
	PRIVATE ${BLAST_PLATFORM_INCLUDES}

	PUBLIC ${PUBLIC_INCLUDE_DIR}
	PRIVATE ${COMMON_SOURCE_DIR}
)

TARGET_COMPILE_DEFINITIONS(NvBlast 
	PRIVATE ${BLAST_COMPILE_DEFS}
)

TARGET_COMPILE_OPTIONS(NvBlast
	PRIVATE ${BLAST_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlast PROPERTIES 
	PDB_NAME_DEBUG "NvBlast${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlast${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlast${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlast${CMAKE_RELEASE_POSTFIX}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BL_LIB_OUTPUT_DIR}/debug"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BL_DLL_OUTPUT_DIR}/debug"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BL_EXE_OUTPUT_DIR}/debug"
    ARCHIVE_OUTPUT_DIRECTORY_CHECKED "${BL_LIB_OUTPUT_DIR}/checked"
    LIBRARY_OUTPUT_DIRECTORY_CHECKED "${BL_DLL_OUTPUT_DIR}/checked"
    RUNTIME_OUTPUT_DIRECTORY_CHECKED "${BL_EXE_OUTPUT_DIR}/checked"
    ARCHIVE_OUTPUT_DIRECTORY_PROFILE "${BL_LIB_OUTPUT_DIR}/profile"
    LIBRARY_OUTPUT_DIRECTORY_PROFILE "${BL_DLL_OUTPUT_DIR}/profile"
    RUNTIME_OUTPUT_DIRECTORY_PROFILE "${BL_EXE_OUTPUT_DIR}/profile"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BL_LIB_OUTPUT_DIR}/release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BL_DLL_OUTPUT_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BL_EXE_OUTPUT_DIR}/release"
)
