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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#include "NvBlastAtomic.h"

#include <string.h>
#include <stdlib.h>


namespace Nv
{
namespace Blast
{


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Windows/XBOXONE Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if NV_WINDOWS_FAMILY || NV_XBOXONE

#include "NvBlastIncludeWindows.h"

int32_t atomicIncrement(volatile int32_t* val)
{
	return (int32_t)InterlockedIncrement((volatile LONG*)val);
}

int32_t atomicDecrement(volatile int32_t* val)
{
	return (int32_t)InterlockedDecrement((volatile LONG*)val);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Unix/PS4 Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#elif(NV_UNIX_FAMILY || NV_PS4)

int32_t atomicIncrement(volatile int32_t* val)
{
	return __sync_add_and_fetch(val, 1);
}

int32_t atomicDecrement(volatile int32_t* val)
{
	return __sync_sub_and_fetch(val, 1);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Unsupported Platforms
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#else

#error "Platform not supported!"

#endif


} // namespace Blast
} // namespace Nv

