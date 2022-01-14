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


#include "SubmeshParameters_0p0.h"
#include <string.h>
#include <stdlib.h>

using namespace NvParameterized;

namespace nvidia
{
namespace parameterized
{

using namespace SubmeshParameters_0p0NS;

const char* const SubmeshParameters_0p0Factory::vptr =
    NvParameterized::getVptr<SubmeshParameters_0p0, SubmeshParameters_0p0::ClassAlignment>();

const uint32_t NumParamDefs = 8;
static NvParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 4, 6, 3, 5, 7,
};

#define TENUM(type) nvidia::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NvParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 4 },
	{ TYPE_REF, false, (size_t)(&((ParametersStruct*)0)->vertexBuffer), NULL, 0 }, // vertexBuffer
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->indexBuffer), CHILDREN(4), 1 }, // indexBuffer
	{ TYPE_U32, false, 1 * sizeof(uint32_t), NULL, 0 }, // indexBuffer[]
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->vertexPartition), CHILDREN(5), 1 }, // vertexPartition
	{ TYPE_U32, false, 1 * sizeof(uint32_t), NULL, 0 }, // vertexPartition[]
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->indexPartition), CHILDREN(6), 1 }, // indexPartition
	{ TYPE_U32, false, 1 * sizeof(uint32_t), NULL, 0 }, // indexPartition[]
};


bool SubmeshParameters_0p0::mBuiltFlag = false;
NvParameterized::MutexType SubmeshParameters_0p0::mBuiltFlagMutex;

SubmeshParameters_0p0::SubmeshParameters_0p0(NvParameterized::Traits* traits, void* buf, int32_t* refCount) :
	NvParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &SubmeshParameters_0p0FactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

SubmeshParameters_0p0::~SubmeshParameters_0p0()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void SubmeshParameters_0p0::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NvParameterized::Traits* traits = mParameterizedTraits;
	int32_t* refCount = mRefCount;
	void* buf = mBuffer;

	this->~SubmeshParameters_0p0();

	NvParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NvParameterized::DefinitionImpl* SubmeshParameters_0p0::getParameterDefinitionTree(void)
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

const NvParameterized::DefinitionImpl* SubmeshParameters_0p0::getParameterDefinitionTree(void) const
{
	SubmeshParameters_0p0* tmpParam = const_cast<SubmeshParameters_0p0*>(this);

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

NvParameterized::ErrorType SubmeshParameters_0p0::getParameterHandle(const char* long_name, Handle& handle) const
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

NvParameterized::ErrorType SubmeshParameters_0p0::getParameterHandle(const char* long_name, Handle& handle)
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

void SubmeshParameters_0p0::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<SubmeshParameters_0p0::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void SubmeshParameters_0p0::freeParameterDefinitionTable(NvParameterized::Traits* traits)
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

void SubmeshParameters_0p0::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="vertexBuffer"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("vertexBuffer", TYPE_REF, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("INCLUDED", uint64_t(1), true);
		ParamDefTable[1].setHints((const NvParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("INCLUDED", uint64_t(1), true);
		HintTable[1].init("longDescription", "This is the vertex buffer included with this submesh.  The submesh is defined\nby a vertex buffer and an index buffer (see indexBuffer).  The vertices for\ndifferent mesh parts are stored in contiguous subsets of the whole vertex buffer.\nThe vertexPartition array holds the offsets into the vertexBuffer for each part.\n", true);
		HintTable[2].init("shortDescription", "The vertex buffer for this submesh", true);
		ParamDefTable[1].setHints((const NvParameterized::Hint**)HintPtrTable, 3);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */


		static const char* const RefVariantVals[] = { "VertexBufferParameters" };
		ParamDefTable[1].setRefVariantVals((const char**)RefVariantVals, 1);



	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="indexBuffer"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("indexBuffer", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "This is the vertex buffer included with this submesh.  The submesh is defined\nby a index buffer and an vertex buffer (see vertexBuffer).  The indices for\ndifferent mesh parts are stored in contiguous subsets of the whole index buffer.\nThe indexPartition array holds the offsets into the indexBuffer for each part.\n", true);
		HintTable[1].init("shortDescription", "The index buffer for this submesh", true);
		ParamDefTable[2].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="indexBuffer[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("indexBuffer", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "This is the vertex buffer included with this submesh.  The submesh is defined\nby a index buffer and an vertex buffer (see vertexBuffer).  The indices for\ndifferent mesh parts are stored in contiguous subsets of the whole index buffer.\nThe indexPartition array holds the offsets into the indexBuffer for each part.\n", true);
		HintTable[1].init("shortDescription", "The index buffer for this submesh", true);
		ParamDefTable[3].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="vertexPartition"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("vertexPartition", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Index offset into vertexBuffer for each part.  The first vertex index for part\ni is vertexPartition[i]. The vertexPartition array size is N+1, where N = the\nnumber of mesh parts, and vertexPartition[N] = vertexBuffer.vertexCount (the\nsize of the vertex buffer). This way, the number of vertices for part i can be\nalways be obtained with vertexPartition[i+1]-vertexPartition[i].\n", true);
		HintTable[1].init("shortDescription", "Part lookup into vertexBuffer", true);
		ParamDefTable[4].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="vertexPartition[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("vertexPartition", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Index offset into vertexBuffer for each part.  The first vertex index for part\ni is vertexPartition[i]. The vertexPartition array size is N+1, where N = the\nnumber of mesh parts, and vertexPartition[N] = vertexBuffer.vertexCount (the\nsize of the vertex buffer). This way, the number of vertices for part i can be\nalways be obtained with vertexPartition[i+1]-vertexPartition[i].\n", true);
		HintTable[1].init("shortDescription", "Part lookup into vertexBuffer", true);
		ParamDefTable[5].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="indexPartition"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("indexPartition", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Index offset into indexBuffer for each part.  The first index location in\nindexPartition for part i is indexPartition[i].  The indexPartition array\nsize is N+1, where N = the number of mesh parts, and indexPartition[N] =\nthe size of the indexBuffer.  This way, the number of indices for part i\ncan be always be obtained with indexPartition[i+1]-indexPartition[i].\n", true);
		HintTable[1].init("shortDescription", "Part lookup into indexBuffer", true);
		ParamDefTable[6].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=7, longName="indexPartition[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[7];
		ParamDef->init("indexPartition", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Index offset into indexBuffer for each part.  The first index location in\nindexPartition for part i is indexPartition[i].  The indexPartition array\nsize is N+1, where N = the number of mesh parts, and indexPartition[N] =\nthe size of the indexBuffer.  This way, the number of indices for part i\ncan be always be obtained with indexPartition[i+1]-indexPartition[i].\n", true);
		HintTable[1].init("shortDescription", "Part lookup into indexBuffer", true);
		ParamDefTable[7].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[4];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);
		Children[2] = PDEF_PTR(4);
		Children[3] = PDEF_PTR(6);

		ParamDefTable[0].setChildren(Children, 4);
	}

	// SetChildren for: nodeIndex=2, longName="indexBuffer"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(3);

		ParamDefTable[2].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=4, longName="vertexPartition"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(5);

		ParamDefTable[4].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=6, longName="indexPartition"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(7);

		ParamDefTable[6].setChildren(Children, 1);
	}

	mBuiltFlag = true;

}
void SubmeshParameters_0p0::initStrings(void)
{
}

void SubmeshParameters_0p0::initDynamicArrays(void)
{
	indexBuffer.buf = NULL;
	indexBuffer.isAllocated = true;
	indexBuffer.elementSize = sizeof(uint32_t);
	indexBuffer.arraySizes[0] = 0;
	vertexPartition.buf = NULL;
	vertexPartition.isAllocated = true;
	vertexPartition.elementSize = sizeof(uint32_t);
	vertexPartition.arraySizes[0] = 0;
	indexPartition.buf = NULL;
	indexPartition.isAllocated = true;
	indexPartition.elementSize = sizeof(uint32_t);
	indexPartition.arraySizes[0] = 0;
}

void SubmeshParameters_0p0::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();

	initDynamicArrays();
	initStrings();
	initReferences();
}

void SubmeshParameters_0p0::initReferences(void)
{
	vertexBuffer = NULL;

}

void SubmeshParameters_0p0::freeDynamicArrays(void)
{
	if (indexBuffer.isAllocated && indexBuffer.buf)
	{
		mParameterizedTraits->free(indexBuffer.buf);
	}
	if (vertexPartition.isAllocated && vertexPartition.buf)
	{
		mParameterizedTraits->free(vertexPartition.buf);
	}
	if (indexPartition.isAllocated && indexPartition.buf)
	{
		mParameterizedTraits->free(indexPartition.buf);
	}
}

void SubmeshParameters_0p0::freeStrings(void)
{
}

void SubmeshParameters_0p0::freeReferences(void)
{
	if (vertexBuffer)
	{
		vertexBuffer->destroy();
	}

}

} // namespace parameterized
} // namespace nvidia
