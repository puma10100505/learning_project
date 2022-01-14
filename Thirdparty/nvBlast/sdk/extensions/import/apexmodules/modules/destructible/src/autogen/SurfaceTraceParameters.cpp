// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.

// This file was generated by NvParameterized/scripts/GenParameterized.pl


#include "SurfaceTraceParameters.h"
#include <string.h>
#include <stdlib.h>

using namespace NvParameterized;

namespace nvidia
{
namespace destructible
{

using namespace SurfaceTraceParametersNS;

const char* const SurfaceTraceParametersFactory::vptr =
    NvParameterized::getVptr<SurfaceTraceParameters, SurfaceTraceParameters::ClassAlignment>();

const uint32_t NumParamDefs = 6;
static NvParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 3, 5, 2, 4,
};

#define TENUM(type) nvidia::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NvParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 3 },
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->submeshIndices), CHILDREN(3), 1 }, // submeshIndices
	{ TYPE_U8, false, 1 * sizeof(uint8_t), NULL, 0 }, // submeshIndices[]
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->vertexIndices), CHILDREN(4), 1 }, // vertexIndices
	{ TYPE_U32, false, 1 * sizeof(uint32_t), NULL, 0 }, // vertexIndices[]
	{ TYPE_VEC3, false, (size_t)(&((ParametersStruct*)0)->defaultNormal), NULL, 0 }, // defaultNormal
};


bool SurfaceTraceParameters::mBuiltFlag = false;
NvParameterized::MutexType SurfaceTraceParameters::mBuiltFlagMutex;

SurfaceTraceParameters::SurfaceTraceParameters(NvParameterized::Traits* traits, void* buf, int32_t* refCount) :
	NvParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &SurfaceTraceParametersFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

SurfaceTraceParameters::~SurfaceTraceParameters()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void SurfaceTraceParameters::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NvParameterized::Traits* traits = mParameterizedTraits;
	int32_t* refCount = mRefCount;
	void* buf = mBuffer;

	this->~SurfaceTraceParameters();

	NvParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NvParameterized::DefinitionImpl* SurfaceTraceParameters::getParameterDefinitionTree(void)
{
	if (!mBuiltFlag) // Double-checked lock
	{
		NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

const NvParameterized::DefinitionImpl* SurfaceTraceParameters::getParameterDefinitionTree(void) const
{
	SurfaceTraceParameters* tmpParam = const_cast<SurfaceTraceParameters*>(this);

	if (!mBuiltFlag) // Double-checked lock
	{
		NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			tmpParam->buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

NvParameterized::ErrorType SurfaceTraceParameters::getParameterHandle(const char* long_name, Handle& handle) const
{
	ErrorType Ret = NvParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

NvParameterized::ErrorType SurfaceTraceParameters::getParameterHandle(const char* long_name, Handle& handle)
{
	ErrorType Ret = NvParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

void SurfaceTraceParameters::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<SurfaceTraceParameters::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void SurfaceTraceParameters::freeParameterDefinitionTable(NvParameterized::Traits* traits)
{
	if (!traits)
	{
		return;
	}

	if (!mBuiltFlag) // Double-checked lock
	{
		return;
	}

	NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);

	if (!mBuiltFlag)
	{
		return;
	}

	for (uint32_t i = 0; i < NumParamDefs; ++i)
	{
		ParamDefTable[i].~DefinitionImpl();
	}

	traits->free(ParamDefTable);

	mBuiltFlag = false;
}

#define PDEF_PTR(index) (&ParamDefTable[index])

void SurfaceTraceParameters::buildTree(void)
{

	uint32_t allocSize = sizeof(NvParameterized::DefinitionImpl) * NumParamDefs;
	ParamDefTable = (NvParameterized::DefinitionImpl*)(mParameterizedTraits->alloc(allocSize));
	memset(ParamDefTable, 0, allocSize);

	for (uint32_t i = 0; i < NumParamDefs; ++i)
	{
		NV_PARAM_PLACEMENT_NEW(ParamDefTable + i, NvParameterized::DefinitionImpl)(*mParameterizedTraits);
	}

	// Initialize DefinitionImpl node: nodeIndex=0, longName=""
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[0];
		ParamDef->init("", TYPE_STRUCT, "STRUCT", true);






	}

	// Initialize DefinitionImpl node: nodeIndex=1, longName="submeshIndices"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("submeshIndices", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The submeshes to which the corresponding vertices (in vertexIndices) belong.\n					The surface traces are defined as a set of vertex indices, but these indices are relative to\n					a submesh.  The size of submeshIndices must be equal to the size of vertexIndices.", true);
		HintTable[1].init("shortDescription", "The submeshes to which the corresponding vertices (in vertexIndices) belong", true);
		ParamDefTable[1].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="submeshIndices[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("submeshIndices", TYPE_U8, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The submeshes to which the corresponding vertices (in vertexIndices) belong.\n					The surface traces are defined as a set of vertex indices, but these indices are relative to\n					a submesh.  The size of submeshIndices must be equal to the size of vertexIndices.", true);
		HintTable[1].init("shortDescription", "The submeshes to which the corresponding vertices (in vertexIndices) belong", true);
		ParamDefTable[2].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="vertexIndices"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("vertexIndices", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The vertex indices defining the surface trace.  The surface traces are defined\n					as a set of vertex indices, but these indices are relative to a submesh.  The size of submeshIndices\n					must be equal to the size of vertexIndices.", true);
		HintTable[1].init("shortDescription", "The vertex indices defining the surface trace.", true);
		ParamDefTable[3].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="vertexIndices[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("vertexIndices", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The vertex indices defining the surface trace.  The surface traces are defined\n					as a set of vertex indices, but these indices are relative to a submesh.  The size of submeshIndices\n					must be equal to the size of vertexIndices.", true);
		HintTable[1].init("shortDescription", "The vertex indices defining the surface trace.", true);
		ParamDefTable[4].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="defaultNormal"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("defaultNormal", TYPE_VEC3, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The average surface normal for this trace.  A surface trace is a single closed loop\n					on the surface of the mesh, along triangle boundaries.  This normal is the normalized average of the\n					surface triangles enclosed by the loop.", true);
		HintTable[1].init("shortDescription", "The average surface normal for this trace", true);
		ParamDefTable[5].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[3];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(3);
		Children[2] = PDEF_PTR(5);

		ParamDefTable[0].setChildren(Children, 3);
	}

	// SetChildren for: nodeIndex=1, longName="submeshIndices"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(2);

		ParamDefTable[1].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=3, longName="vertexIndices"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(4);

		ParamDefTable[3].setChildren(Children, 1);
	}

	mBuiltFlag = true;

}
void SurfaceTraceParameters::initStrings(void)
{
}

void SurfaceTraceParameters::initDynamicArrays(void)
{
	submeshIndices.buf = NULL;
	submeshIndices.isAllocated = true;
	submeshIndices.elementSize = sizeof(uint8_t);
	submeshIndices.arraySizes[0] = 0;
	vertexIndices.buf = NULL;
	vertexIndices.isAllocated = true;
	vertexIndices.elementSize = sizeof(uint32_t);
	vertexIndices.arraySizes[0] = 0;
}

void SurfaceTraceParameters::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();

	initDynamicArrays();
	initStrings();
	initReferences();
}

void SurfaceTraceParameters::initReferences(void)
{
}

void SurfaceTraceParameters::freeDynamicArrays(void)
{
	if (submeshIndices.isAllocated && submeshIndices.buf)
	{
		mParameterizedTraits->free(submeshIndices.buf);
	}
	if (vertexIndices.isAllocated && vertexIndices.buf)
	{
		mParameterizedTraits->free(vertexIndices.buf);
	}
}

void SurfaceTraceParameters::freeStrings(void)
{
}

void SurfaceTraceParameters::freeReferences(void)
{
}

} // namespace destructible
} // namespace nvidia
