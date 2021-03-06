/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef MODULE_CONVERSIONDESTRUCTIBLEACTORPARAM_0P30_0P31H_H
#define MODULE_CONVERSIONDESTRUCTIBLEACTORPARAM_0P30_0P31H_H

#include "NvParamConversionTemplate.h"
#include "DestructibleActorParam_0p30.h"
#include "DestructibleActorParam_0p31.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::DestructibleActorParam_0p30, 
						nvidia::parameterized::DestructibleActorParam_0p31, 
						nvidia::parameterized::DestructibleActorParam_0p30::ClassVersion, 
						nvidia::parameterized::DestructibleActorParam_0p31::ClassVersion>
						ConversionDestructibleActorParam_0p30_0p31Parent;

class ConversionDestructibleActorParam_0p30_0p31: public ConversionDestructibleActorParam_0p30_0p31Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleActorParam_0p30_0p31));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleActorParam_0p30_0p31)(t) : 0;
	}

protected:
	ConversionDestructibleActorParam_0p30_0p31(NvParameterized::Traits* t) : ConversionDestructibleActorParam_0p30_0p31Parent(t) {}

	const NvParameterized::PrefVer* getPreferredVersions() const
	{
		static NvParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (uint32_t)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		//TODO:
		//	Write custom conversion code here using mNewData and mLegacyData members.
		//
		//	Note that
		//		- mNewData has already been initialized with default values
		//		- same-named/same-typed members have already been copied
		//			from mLegacyData to mNewData
		//		- included references were moved to mNewData
		//			(and updated to preferred versions according to getPreferredVersions)
		//
		//	For more info see the versioning wiki.
		physx::PxMat33 tm(physx::PxVec3(mLegacyData->globalPose[0], mLegacyData->globalPose[1], mLegacyData->globalPose[2]),
			physx::PxVec3(mLegacyData->globalPose[3], mLegacyData->globalPose[4], mLegacyData->globalPose[5]),
			physx::PxVec3(mLegacyData->globalPose[6], mLegacyData->globalPose[7], mLegacyData->globalPose[8]));

		mNewData->globalPose.q = physx::PxQuat(tm);
		mNewData->globalPose.p = physx::PxVec3(mLegacyData->globalPose[9], mLegacyData->globalPose[10], mLegacyData->globalPose[11]);
		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
