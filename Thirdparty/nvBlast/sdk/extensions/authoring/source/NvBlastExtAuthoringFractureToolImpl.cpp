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

#include "NvBlastExtAuthoringFractureToolImpl.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringMeshUtils.h"

// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif
#include <queue>
#include <vector>
#include <map>
#include <stack>
#include <functional>
#include "NvBlastExtAuthoringVSA.h"
#include <float.h>
#include "NvBlastExtAuthoringTriangulator.h"
#include "NvBlastExtAuthoringBooleanTool.h"
#include "NvBlastExtAuthoringAccelerator.h"
#include "NvBlastExtAuthoringCutout.h"
#include "NvBlast.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <NvBlastAssert.h>
#include <NvBlastPxSharedHelpers.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)                                                                                                 \
	{                                                                                                                  \
		if (p)                                                                                                         \
		{                                                                                                              \
			delete (p);                                                                                                \
			(p) = NULL;                                                                                                \
		}                                                                                                              \
	}
#endif

#define DEFAULT_BB_ACCELARATOR_RES 10
#define SLICING_INDEXER_OFFSET (1ll << 32)

namespace Nv
{
namespace Blast
{

struct Halfspace_partitioning : public VSA::VS3D_Halfspace_Set
{
	std::vector<NvcPlane> planes;
	VSA::real farthest_halfspace(VSA::real plane[4], const VSA::real point[4])
	{
		float biggest_d = -FLT_MAX;
		for (uint32_t i = 0; i < planes.size(); ++i)
		{
			float d =
			    planes[i].n.x * point[0] + planes[i].n.y * point[1] + planes[i].n.z * point[2] + planes[i].d * point[3];
			if (d > biggest_d)
			{
				biggest_d = d;
				plane[0]  = planes[i].n.x;
				plane[1]  = planes[i].n.y;
				plane[2]  = planes[i].n.z;
				plane[3]  = planes[i].d;
			}
		}
		return biggest_d;
	};
};


void findCellBasePlanes(const std::vector<NvcVec3>& sites, std::vector<std::vector<int32_t> >& neighboors)
{
	Halfspace_partitioning prt;
	std::vector<NvcPlane>& planes = prt.planes;
	neighboors.resize(sites.size());
	for (uint32_t cellId = 0; cellId + 1 < sites.size(); ++cellId)
	{
		planes.clear();
		planes.resize(sites.size() - 1 - cellId);
		std::vector<NvcVec3> midpoints(sites.size() - 1);
		int32_t collected = 0;

		for (uint32_t i = cellId + 1; i < sites.size(); ++i)
		{
			NvcVec3 midpoint     = 0.5 * (sites[i] + sites[cellId]);
			NvcVec3 direction    = fromPxShared(toPxShared(sites[i] - sites[cellId]).getNormalized());
			planes[collected].n  = direction;
			planes[collected].d  = -dot(direction, midpoint);
			midpoints[collected] = midpoint;
			++collected;
		}
		for (uint32_t i = 0; i < planes.size(); ++i)
		{
			planes[i].n = neg(planes[i].n);
			planes[i].d = -planes[i].d;

			if (VSA::vs3d_test(prt))
			{
				neighboors[cellId].push_back(i + cellId + 1);
				neighboors[i + cellId + 1].push_back(cellId);
			};
			planes[i].n = neg(planes[i].n);
			planes[i].d = -planes[i].d;
		}
	}
}


#define SITE_BOX_SIZE 4
#define CUTTING_BOX_SIZE 40

Mesh* getCellMesh(BooleanEvaluator& eval, int32_t planeIndexerOffset, int32_t cellId, const std::vector<NvcVec3>& sites,
                  std::vector<std::vector<int32_t> >& neighboors, int32_t interiorMaterialId, NvcVec3 origin)
{
	Mesh* cell        = getBigBox(toPxShared(origin), SITE_BOX_SIZE, interiorMaterialId);
	Mesh* cuttingMesh = getCuttingBox(PxVec3(0, 0, 0), PxVec3(1, 1, 1), CUTTING_BOX_SIZE, 0, interiorMaterialId);

	for (uint32_t i = 0; i < neighboors[cellId].size(); ++i)
	{
		int32_t nCell    = neighboors[cellId][i];
		PxVec3 midpoint  = 0.5 * toPxShared(sites[nCell] + sites[cellId]);
		PxVec3 direction = toPxShared(sites[nCell] - sites[cellId]).getNormalized();
		int32_t planeIndex =
		    static_cast<int32_t>(sites.size()) * std::min(cellId, nCell) + std::max(cellId, nCell) + planeIndexerOffset;
		if (nCell < cellId)
			planeIndex = -planeIndex;
		setCuttingBox(midpoint, -direction, cuttingMesh, CUTTING_BOX_SIZE, planeIndex);
		eval.performFastCutting(cell, cuttingMesh, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* newCell = eval.createNewMesh();
		delete cell;
		cell = newCell;
		if (cell == nullptr)
			break;
	}
	delete cuttingMesh;
	return cell;
}


bool blastBondComparator(const NvBlastBondDesc& a, const NvBlastBondDesc& b)
{
	if (a.chunkIndices[0] == b.chunkIndices[0])
		return a.chunkIndices[1] < b.chunkIndices[1];
	else
		return a.chunkIndices[0] < b.chunkIndices[0];
}


#define MAX_VORONOI_ATTEMPT_NUMBER 450

VoronoiSitesGeneratorImpl::VoronoiSitesGeneratorImpl(const Mesh* mesh, RandomGeneratorBase* rnd)
{
	mMesh        = mesh;
	mRnd         = rnd;
	mAccelerator = new BBoxBasedAccelerator(mMesh, DEFAULT_BB_ACCELARATOR_RES);
	mStencil     = nullptr;
}

void VoronoiSitesGeneratorImpl::setBaseMesh(const Mesh* m)
{
	mGeneratedSites.clear();
	delete mAccelerator;
	mMesh        = m;
	mAccelerator = new BBoxBasedAccelerator(mMesh, DEFAULT_BB_ACCELARATOR_RES);
}

VoronoiSitesGeneratorImpl::~VoronoiSitesGeneratorImpl()
{
	delete mAccelerator;
	mAccelerator = nullptr;
}

void VoronoiSitesGeneratorImpl::release()
{
	delete this;
}

void VoronoiSitesGeneratorImpl::setStencil(const Mesh* stencil)
{
	mStencil = stencil;
}


void VoronoiSitesGeneratorImpl::clearStencil()
{
	mStencil = nullptr;
}


void VoronoiSitesGeneratorImpl::uniformlyGenerateSitesInMesh(const uint32_t sitesCount)
{
	BooleanEvaluator voronoiMeshEval;
	NvcVec3 mn              = mMesh->getBoundingBox().minimum;
	NvcVec3 mx              = mMesh->getBoundingBox().maximum;
	NvcVec3 vc              = mx - mn;
	uint32_t attemptNumber  = 0;
	uint32_t generatedSites = 0;
	while (generatedSites < sitesCount && attemptNumber < MAX_VORONOI_ATTEMPT_NUMBER)
	{
		float rn1 = mRnd->getRandomValue() * vc.x;
		float rn2 = mRnd->getRandomValue() * vc.y;
		float rn3 = mRnd->getRandomValue() * vc.z;
		if (voronoiMeshEval.isPointContainedInMesh(mMesh, NvcVec3{ rn1, rn2, rn3 } + mn) &&
		    (mStencil == nullptr || voronoiMeshEval.isPointContainedInMesh(mStencil, NvcVec3{ rn1, rn2, rn3 } + mn)))
		{
			generatedSites++;
			mGeneratedSites.push_back(NvcVec3{ rn1, rn2, rn3 } + mn);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
}


void VoronoiSitesGeneratorImpl::clusteredSitesGeneration(const uint32_t numberOfClusters,
                                                         const uint32_t sitesPerCluster, float clusterRadius)
{
	BooleanEvaluator voronoiMeshEval;
	NvcVec3 mn              = mMesh->getBoundingBox().minimum;
	NvcVec3 mx              = mMesh->getBoundingBox().maximum;
	NvcVec3 middle          = (mx + mn) * 0.5;
	NvcVec3 vc              = (mx - mn) * 0.5;
	uint32_t attemptNumber  = 0;
	uint32_t generatedSites = 0;
	std::vector<NvcVec3> tempPoints;
	while (generatedSites < numberOfClusters)
	{
		float rn1 = mRnd->getRandomValue() * 2 - 1;
		float rn2 = mRnd->getRandomValue() * 2 - 1;
		float rn3 = mRnd->getRandomValue() * 2 - 1;
		NvcVec3 p = { middle.x + rn1 * vc.x, middle.y + rn2 * vc.y, middle.z + rn3 * vc.z };

		if (voronoiMeshEval.isPointContainedInMesh(mMesh, p) &&
		    (mStencil == nullptr || voronoiMeshEval.isPointContainedInMesh(mStencil, p)))
		{
			generatedSites++;
			tempPoints.push_back(p);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
	int32_t totalCount = 0;
	for (; tempPoints.size() > 0; tempPoints.pop_back())
	{
		uint32_t unif  = sitesPerCluster;
		generatedSites = 0;
		while (generatedSites < unif)
		{
			NvcVec3 p =
			    tempPoints.back() + fromPxShared(PxVec3(mRnd->getRandomValue() * 2 - 1, mRnd->getRandomValue() * 2 - 1,
			                                         mRnd->getRandomValue() * 2 - 1)
			                                      .getNormalized()) *
			                            (mRnd->getRandomValue() + 0.001f) * clusterRadius;
			if (voronoiMeshEval.isPointContainedInMesh(mMesh, p) &&
			    (mStencil == nullptr || voronoiMeshEval.isPointContainedInMesh(mStencil, p)))
			{
				totalCount++;
				generatedSites++;
				mGeneratedSites.push_back(p);
				attemptNumber = 0;
			}
			else
			{
				attemptNumber++;
				if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
					break;
			}
		}
	}
}


#define IN_SPHERE_ATTEMPT_NUMBER 20

void VoronoiSitesGeneratorImpl::addSite(const NvcVec3& site)
{
	mGeneratedSites.push_back(site);
}


void VoronoiSitesGeneratorImpl::generateInSphere(const uint32_t count, const float radius, const NvcVec3& center)
{
	BooleanEvaluator voronoiMeshEval;
	uint32_t attemptNumber  = 0;
	uint32_t generatedSites = 0;
	std::vector<NvcVec3> tempPoints;
	float radiusSquared = radius * radius;

	while (generatedSites < count && attemptNumber < MAX_VORONOI_ATTEMPT_NUMBER)
	{
		float rn1     = (mRnd->getRandomValue() - 0.5f) * 2.f * radius;
		float rn2     = (mRnd->getRandomValue() - 0.5f) * 2.f * radius;
		float rn3     = (mRnd->getRandomValue() - 0.5f) * 2.f * radius;
		NvcVec3 point = { rn1, rn2, rn3 };
		if (toPxShared(point).magnitudeSquared() < radiusSquared &&
		    voronoiMeshEval.isPointContainedInMesh(mMesh, point + center) &&
		    (mStencil == nullptr || voronoiMeshEval.isPointContainedInMesh(mStencil, point + center)))
		{
			generatedSites++;
			mGeneratedSites.push_back(point + center);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
}


void VoronoiSitesGeneratorImpl::deleteInSphere(const float radius, const NvcVec3& center, float deleteProbability)
{
	float r2 = radius * radius;
	for (uint32_t i = 0; i < mGeneratedSites.size(); ++i)
	{
		if (toPxShared(mGeneratedSites[i] - center).magnitudeSquared() < r2 && mRnd->getRandomValue() <= deleteProbability)
		{
			std::swap(mGeneratedSites[i], mGeneratedSites.back());
			mGeneratedSites.pop_back();
			--i;
		}
	}
}


void VoronoiSitesGeneratorImpl::radialPattern(const NvcVec3& center, const NvcVec3& normal, float radius,
                                              int32_t angularSteps, int32_t radialSteps, float angleOffset,
                                              float variability)
{
	//	mGeneratedSites.push_back(center);
	PxVec3 t1, t2;
	if (std::abs(normal.z) < 0.9)
	{
		t1 = toPxShared(normal).cross(PxVec3(0, 0, 1));
	}
	else
	{
		t1 = toPxShared(normal).cross(PxVec3(1, 0, 0));
	}
	t2 = t1.cross(toPxShared(normal));
	t1.normalize();
	t2.normalize();

	float radStep = radius / radialSteps;
	int32_t cCr   = 0;

	float angleStep = physx::PxPi * 2 / angularSteps;
	for (float cRadius = radStep; cRadius < radius; cRadius += radStep)
	{
		float cAngle = angleOffset * cCr;
		for (int32_t i = 0; i < angularSteps; ++i)
		{
			float angVars = mRnd->getRandomValue() * variability + (1.0f - 0.5f * variability);
			float radVars = mRnd->getRandomValue() * variability + (1.0f - 0.5f * variability);

			NvcVec3 nPos = fromPxShared(std::cos(cAngle * angVars) * t1 + std::sin(cAngle * angVars) * t2) * cRadius * radVars + center;
			mGeneratedSites.push_back(nPos);
			cAngle += angleStep;
		}
		++cCr;
	}
}

uint32_t VoronoiSitesGeneratorImpl::getVoronoiSites(const NvcVec3*& sites)
{
	if (mGeneratedSites.size())
	{
		sites = &mGeneratedSites[0];
	}
	return (uint32_t)mGeneratedSites.size();
}

int32_t
FractureToolImpl::voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const NvcVec3* cellPointsIn, bool replaceChunk)
{
	if (chunkId == 0 && replaceChunk)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1 || cellCount < 2)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	Mesh* mesh = mChunkData[chunkIndex].meshData;

	std::vector<NvcVec3> cellPoints(cellCount);
	for (uint32_t i = 0; i < cellCount; ++i)
	{
		cellPoints[i] = (cellPointsIn[i] - mOffset) * (1.0f / mScaleFactor);
	}

	/**
	Prebuild accelerator structure
	*/
	BooleanEvaluator eval;
	BooleanEvaluator voronoiMeshEval;

	BBoxBasedAccelerator spAccel = BBoxBasedAccelerator(mesh, DEFAULT_BB_ACCELARATOR_RES);

	std::vector<std::vector<int32_t> > neighboors;
	findCellBasePlanes(cellPoints, neighboors);

	/**
	Fracture
	*/
	int32_t parentChunk = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<uint32_t> newlyCreatedChunksIds;
	for (uint32_t i = 0; i < cellPoints.size(); ++i)
	{
		Mesh* cell =
		    getCellMesh(eval, mPlaneIndexerOffset, i, cellPoints, neighboors, mInteriorMaterialId, cellPoints[i]);

		if (cell == nullptr)
		{
			continue;
		}
		DummyAccelerator dmAccel(cell->getFacetCount());
		voronoiMeshEval.performBoolean(mesh, cell, &spAccel, &dmAccel, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* resultMesh = voronoiMeshEval.createNewMesh();
		if (resultMesh)
		{
			uint32_t ncidx             = createNewChunk(parentChunk);
			mChunkData[ncidx].isLeaf   = true;
			mChunkData[ncidx].meshData = resultMesh;
			newlyCreatedChunksIds.push_back(mChunkData[ncidx].chunkId);
		}
		eval.reset();
		delete cell;
	}
	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
		deleteChunkSubhierarchy(chunkId, true);
	}
	mPlaneIndexerOffset += static_cast<int32_t>(cellPoints.size() * cellPoints.size());

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}

Mesh* FractureToolImpl::createChunkMesh(int32_t chunkId)
{
	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex < 0 || static_cast<size_t>(chunkIndex) >= mChunkData.size())
	{
		return nullptr;
	}

	auto temp = new MeshImpl(*reinterpret_cast<MeshImpl*>(mChunkData[chunkIndex].meshData));
	for (uint32_t i = 0; i < temp->getVerticesCount(); ++i)
	{
		temp->getVerticesWritable()[i].p = temp->getVertices()[i].p * mScaleFactor + mOffset;
	}
	temp->recalculateBoundingBox();

	return temp;
}

bool FractureToolImpl::isMeshContainOpenEdges(const Mesh* input)
{
	std::map<NvcVec3, int32_t, VrtPositionComparator> vertexMapping;
	std::vector<int32_t> vertexRemappingArray(input->getVerticesCount());
	std::vector<Edge> remappedEdges(input->getEdgesCount());
	/**
	    Remap vertices
	*/

	const Vertex* vrx = input->getVertices();
	for (uint32_t i = 0; i < input->getVerticesCount(); ++i)
	{
		auto it = vertexMapping.find(vrx->p);
		if (it == vertexMapping.end())
		{
			vertexMapping[vrx->p]   = i;
			vertexRemappingArray[i] = i;
		}
		else
		{
			vertexRemappingArray[i] = it->second;
		}
		++vrx;
	}

	const Edge* ed = input->getEdges();
	for (uint32_t i = 0; i < input->getEdgesCount(); ++i)
	{
		remappedEdges[i].s = vertexRemappingArray[ed->s];
		remappedEdges[i].e = vertexRemappingArray[ed->e];
		if (remappedEdges[i].e < remappedEdges[i].s)
		{
			std::swap(remappedEdges[i].s, remappedEdges[i].e);
		}
		++ed;
	}

	std::sort(remappedEdges.begin(), remappedEdges.end());

	int32_t collected = 1;
	for (uint32_t i = 1; i < remappedEdges.size(); ++i)
	{
		if (remappedEdges[i - 1].s == remappedEdges[i].s && remappedEdges[i - 1].e == remappedEdges[i].e)
		{
			collected++;
		}
		else
		{
			if (collected & 1)
			{
				return true;
			}
			else
			{
				collected = 1;
			}
		}
	}
	return collected & 1;
}

int32_t FractureToolImpl::voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const NvcVec3* cellPointsIn,
                                            const NvcVec3& scale, const NvcQuat& rotation, bool replaceChunk)
{
	if (chunkId == 0 && replaceChunk)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1 || cellCount < 2)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	Mesh* mesh = mChunkData[chunkIndex].meshData;

	std::vector<NvcVec3> cellPoints(cellCount);
	for (uint32_t i = 0; i < cellCount; ++i)
	{
		cellPoints[i] = (cellPointsIn[i] - mOffset) * (1.0f / mScaleFactor);

		toPxShared(cellPoints[i]) = toPxShared(rotation).rotateInv(toPxShared(cellPoints[i]));

		cellPoints[i].x *= (1.0f / scale.x);
		cellPoints[i].y *= (1.0f / scale.y);
		cellPoints[i].z *= (1.0f / scale.z);
	}

	/**
	Prebuild accelerator structure
	*/
	BooleanEvaluator eval;
	BooleanEvaluator voronoiMeshEval;

	BBoxBasedAccelerator spAccel = BBoxBasedAccelerator(mesh, DEFAULT_BB_ACCELARATOR_RES);

	std::vector<std::vector<int32_t> > neighboors;
	findCellBasePlanes(cellPoints, neighboors);

	/**
	Fracture
	*/
	int32_t parentChunk = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<uint32_t> newlyCreatedChunksIds;

	for (uint32_t i = 0; i < cellPoints.size(); ++i)
	{
		Mesh* cell =
		    getCellMesh(eval, mPlaneIndexerOffset, i, cellPoints, neighboors, mInteriorMaterialId, cellPoints[i]);

		if (cell == nullptr)
		{
			continue;
		}

		for (uint32_t v = 0; v < cell->getVerticesCount(); ++v)
		{
			cell->getVerticesWritable()[v].p.x *= scale.x;
			cell->getVerticesWritable()[v].p.y *= scale.y;
			cell->getVerticesWritable()[v].p.z *= scale.z;
			toPxShared(cell->getVerticesWritable()[v].p) = toPxShared(rotation).rotate(toPxShared(cell->getVerticesWritable()[v].p));
		}
		cell->recalculateBoundingBox();
		DummyAccelerator dmAccel(cell->getFacetCount());
		voronoiMeshEval.performBoolean(mesh, cell, &spAccel, &dmAccel, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* resultMesh = voronoiMeshEval.createNewMesh();
		if (resultMesh)
		{
			uint32_t ncidx             = createNewChunk(parentChunk);
			mChunkData[ncidx].isLeaf   = true;
			mChunkData[ncidx].meshData = resultMesh;
			newlyCreatedChunksIds.push_back(mChunkData[ncidx].chunkId);
		}
		eval.reset();
		delete cell;
	}
	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
        deleteChunkSubhierarchy(chunkId, true);
	}
	mPlaneIndexerOffset += static_cast<int32_t>(cellPoints.size() * cellPoints.size());

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}

int32_t FractureToolImpl::slicing(uint32_t chunkId, const SlicingConfiguration& conf, bool replaceChunk,
                                  RandomGeneratorBase* rnd)
{
	if (conf.noise.amplitude != 0)
	{
		return slicingNoisy(chunkId, conf, replaceChunk, rnd);
	}

	if (replaceChunk && chunkId == 0)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);


	Mesh* mesh = new MeshImpl(*reinterpret_cast<MeshImpl*>(mChunkData[chunkIndex].meshData));

	BooleanEvaluator bTool;

	int32_t x_slices = conf.x_slices;
	int32_t y_slices = conf.y_slices;
	int32_t z_slices = conf.z_slices;

	const physx::PxBounds3 sourceBBox = toPxShared(mesh->getBoundingBox());

	PxVec3 center = {mesh->getBoundingBox().minimum.x, 0, 0};


	float x_offset = (sourceBBox.maximum.x - sourceBBox.minimum.x) * (1.0f / (x_slices + 1));
	float y_offset = (sourceBBox.maximum.y - sourceBBox.minimum.y) * (1.0f / (y_slices + 1));
	float z_offset = (sourceBBox.maximum.z - sourceBBox.minimum.z) * (1.0f / (z_slices + 1));

	center.x += x_offset;

	PxVec3 dir = {1, 0, 0};

	Mesh* slBox = getCuttingBox(center, dir, 20, 0, mInteriorMaterialId);

	ChunkInfo ch;
	ch.isLeaf    = true;
	ch.isChanged = true;
	ch.flags     = ChunkInfo::NO_FLAGS;
	ch.parent    = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<ChunkInfo> xSlicedChunks;
	std::vector<ChunkInfo> ySlicedChunks;
	std::vector<uint32_t> newlyCreatedChunksIds;
	/**
	Slice along x direction
	*/
	for (int32_t slice = 0; slice < x_slices; ++slice)
	{
		PxVec3 randVect =
		    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
		PxVec3 lDir = dir + randVect * conf.angle_variations;

		setCuttingBox(center, -lDir, slBox, 20, mPlaneIndexerOffset + SLICING_INDEXER_OFFSET);
		bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
		ch.meshData = bTool.createNewMesh();

		if (ch.meshData != 0)
		{
			xSlicedChunks.push_back(ch);
		}
		inverseNormalAndIndices(slBox);
		++mPlaneIndexerOffset;
		bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
		Mesh* result = bTool.createNewMesh();
		delete mesh;
		mesh = result;
		if (mesh == nullptr)
		{
			break;
		}
		center.x += x_offset + (rnd->getRandomValue()) * conf.offset_variations * x_offset;
	}
	if (mesh != 0)
	{
		ch.meshData = mesh;
		xSlicedChunks.push_back(ch);
	}


	for (uint32_t chunk = 0; chunk < xSlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, sourceBBox.minimum.y, 0);
		center.y += y_offset;
		dir  = PxVec3(0, 1, 0);
		mesh = xSlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < y_slices; ++slice)
		{
			PxVec3 randVect =
			    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;


			setCuttingBox(center, -lDir, slBox, 20, mPlaneIndexerOffset + SLICING_INDEXER_OFFSET);
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ySlicedChunks.push_back(ch);
			}
			inverseNormalAndIndices(slBox);
			++mPlaneIndexerOffset;
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.y += y_offset + (rnd->getRandomValue()) * conf.offset_variations * y_offset;
		}
		if (mesh != 0)
		{
			ch.meshData = mesh;
			ySlicedChunks.push_back(ch);
		}
	}


	for (uint32_t chunk = 0; chunk < ySlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, 0, sourceBBox.minimum.z);
		center.z += z_offset;
		dir  = PxVec3(0, 0, 1);
		mesh = ySlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < z_slices; ++slice)
		{
			PxVec3 randVect =
			    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;
			setCuttingBox(center, -lDir, slBox, 20, mPlaneIndexerOffset + SLICING_INDEXER_OFFSET);
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ch.chunkId = mChunkIdCounter++;
				newlyCreatedChunksIds.push_back(ch.chunkId);
				mChunkData.push_back(ch);
			}
			inverseNormalAndIndices(slBox);
			++mPlaneIndexerOffset;
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.z += z_offset + (rnd->getRandomValue()) * conf.offset_variations * z_offset;
		}
		if (mesh != 0)
		{
			ch.chunkId  = mChunkIdCounter++;
			ch.meshData = mesh;
			mChunkData.push_back(ch);
			newlyCreatedChunksIds.push_back(ch.chunkId);
		}
	}


	delete slBox;

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
        deleteChunkSubhierarchy(chunkId, true);
	}

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}

int32_t FractureToolImpl::slicingNoisy(uint32_t chunkId, const SlicingConfiguration& conf, bool replaceChunk,
                                       RandomGeneratorBase* rnd)
{
	if (replaceChunk && chunkId == 0)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);


	Mesh* mesh = new MeshImpl(*reinterpret_cast<MeshImpl*>(mChunkData[chunkIndex].meshData));

	BooleanEvaluator bTool;

	int32_t x_slices = conf.x_slices;
	int32_t y_slices = conf.y_slices;
	int32_t z_slices = conf.z_slices;

	const physx::PxBounds3 sourceBBox = toPxShared(mesh->getBoundingBox());

	PxVec3 center = PxVec3(mesh->getBoundingBox().minimum.x, 0, 0);


	float x_offset = (sourceBBox.maximum.x - sourceBBox.minimum.x) * (1.0f / (x_slices + 1));
	float y_offset = (sourceBBox.maximum.y - sourceBBox.minimum.y) * (1.0f / (y_slices + 1));
	float z_offset = (sourceBBox.maximum.z - sourceBBox.minimum.z) * (1.0f / (z_slices + 1));

	PxVec3 resolution(mScaleFactor / conf.noise.samplingInterval.x, mScaleFactor / conf.noise.samplingInterval.y,
	                   mScaleFactor / conf.noise.samplingInterval.z);

	center.x += x_offset;

	PxVec3 dir(1, 0, 0);

	Mesh* slBox = nullptr;

	ChunkInfo ch;
	ch.isLeaf    = true;
	ch.isChanged = true;
	ch.flags     = ChunkInfo::NO_FLAGS;
	ch.parent    = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<ChunkInfo> xSlicedChunks;
	std::vector<ChunkInfo> ySlicedChunks;
	std::vector<uint32_t> newlyCreatedChunksIds;
	float noisyPartSize = 1.2f;
	//	int32_t acceleratorRes = 8;
	/**
	    Slice along x direction
	*/
	for (int32_t slice = 0; slice < x_slices; ++slice)
	{
		PxVec3 randVect =
		    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
		PxVec3 lDir = dir + randVect * conf.angle_variations;
		slBox        = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, resolution,
                                       mPlaneIndexerOffset + SLICING_INDEXER_OFFSET, conf.noise.amplitude,
                                       conf.noise.frequency, conf.noise.octaveNumber, rnd->getRandomValue(),
                                       mInteriorMaterialId);
		//	DummyAccelerator accel(mesh->getFacetCount());
		SweepingAccelerator accel(mesh);
		SweepingAccelerator dummy(slBox);
		bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
		ch.meshData = bTool.createNewMesh();
		if (ch.meshData != 0)
		{
			xSlicedChunks.push_back(ch);
		}
		inverseNormalAndIndices(slBox);
		++mPlaneIndexerOffset;
		bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* result = bTool.createNewMesh();
		delete slBox;
		delete mesh;
		mesh = result;
		if (mesh == nullptr)
		{
			break;
		}
		center.x += x_offset + (rnd->getRandomValue()) * conf.offset_variations * x_offset;
	}
	if (mesh != 0)
	{
		ch.meshData = mesh;
		xSlicedChunks.push_back(ch);
	}
	slBox                    = getCuttingBox(center, dir, 20, 0, mInteriorMaterialId);
	uint32_t slicedChunkSize = xSlicedChunks.size();
	for (uint32_t chunk = 0; chunk < slicedChunkSize; ++chunk)
	{
		center = PxVec3(0, sourceBBox.minimum.y, 0);
		center.y += y_offset;
		dir  = PxVec3(0, 1, 0);
		mesh = xSlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < y_slices; ++slice)
		{
			PxVec3 randVect =
			    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;

			slBox = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, resolution,
			                               mPlaneIndexerOffset + SLICING_INDEXER_OFFSET, conf.noise.amplitude,
			                               conf.noise.frequency, conf.noise.octaveNumber, rnd->getRandomValue(),
			                               mInteriorMaterialId);
			//	DummyAccelerator accel(mesh->getFacetCount());
			SweepingAccelerator accel(mesh);
			SweepingAccelerator dummy(slBox);
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ySlicedChunks.push_back(ch);
			}
			inverseNormalAndIndices(slBox);
			++mPlaneIndexerOffset;
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete slBox;
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.y += y_offset + (rnd->getRandomValue()) * conf.offset_variations * y_offset;
		}
		if (mesh != 0)
		{
			ch.meshData = mesh;
			ySlicedChunks.push_back(ch);
		}
	}

	for (uint32_t chunk = 0; chunk < ySlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, 0, sourceBBox.minimum.z);
		center.z += z_offset;
		dir  = PxVec3(0, 0, 1);
		mesh = ySlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < z_slices; ++slice)
		{
			PxVec3 randVect =
			    PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;
			slBox        = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, resolution,
                                           mPlaneIndexerOffset + SLICING_INDEXER_OFFSET, conf.noise.amplitude,
                                           conf.noise.frequency, conf.noise.octaveNumber, rnd->getRandomValue(),
                                           mInteriorMaterialId);
			//		DummyAccelerator accel(mesh->getFacetCount());
			SweepingAccelerator accel(mesh);
			SweepingAccelerator dummy(slBox);
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ch.chunkId = mChunkIdCounter++;
				mChunkData.push_back(ch);
				newlyCreatedChunksIds.push_back(ch.chunkId);
			}
			inverseNormalAndIndices(slBox);
			++mPlaneIndexerOffset;
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			delete slBox;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.z += z_offset + (rnd->getRandomValue()) * conf.offset_variations * z_offset;
		}
		if (mesh != 0)
		{
			ch.chunkId  = mChunkIdCounter++;
			ch.meshData = mesh;
			mChunkData.push_back(ch);
			newlyCreatedChunksIds.push_back(ch.chunkId);
		}
	}

	//	delete slBox;

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
        deleteChunkSubhierarchy(chunkId, true);
	}

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}
int32_t FractureToolImpl::cut(uint32_t chunkId, const NvcVec3& normal, const NvcVec3& point,
                              const NoiseConfiguration& noise, bool replaceChunk, RandomGeneratorBase* rnd)
{
	if (replaceChunk && chunkId == 0)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	Mesh* mesh = new MeshImpl(*reinterpret_cast<MeshImpl*>(mChunkData[chunkIndex].meshData));
	BooleanEvaluator bTool;

	ChunkInfo ch;
	ch.chunkId          = -1;
	ch.isLeaf           = true;
	ch.isChanged        = true;
	ch.flags            = ChunkInfo::NO_FLAGS;
	ch.parent           = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	float noisyPartSize = 1.2f;

	PxVec3 resolution(mScaleFactor / noise.samplingInterval.x, mScaleFactor / noise.samplingInterval.y,
	                   mScaleFactor / noise.samplingInterval.z);

	// Perform cut
	Mesh* slBox = getNoisyCuttingBoxPair(toPxShared(point - mOffset) / mScaleFactor, toPxShared(normal), 40, noisyPartSize, resolution,
	                                     mPlaneIndexerOffset + SLICING_INDEXER_OFFSET, noise.amplitude, noise.frequency,
	                                     noise.octaveNumber, rnd->getRandomValue(), mInteriorMaterialId);
	SweepingAccelerator accel(mesh);
	SweepingAccelerator dummy(slBox);
	bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
	ch.meshData = bTool.createNewMesh();
	inverseNormalAndIndices(slBox);
	++mPlaneIndexerOffset;
	bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
	Mesh* result = bTool.createNewMesh();
	delete slBox;
	delete mesh;
	mesh = result;

	if (mesh == 0)  // Return if it doesn't cut specified chunk
	{
		return 1;
	}

	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	int32_t firstChunkId = -1;
	if (ch.meshData != 0)
	{
		ch.chunkId = mChunkIdCounter++;
		mChunkData.push_back(ch);
		firstChunkId = ch.chunkId;
	}
	if (mesh != 0)
	{
		ch.chunkId  = mChunkIdCounter++;
		ch.meshData = mesh;
		mChunkData.push_back(ch);
	}

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
        deleteChunkSubhierarchy(chunkId, true);
	}

	if (mRemoveIslands && firstChunkId >= 0)
	{
		islandDetectionAndRemoving(firstChunkId);
		if (mesh != 0)
		{
			islandDetectionAndRemoving(ch.chunkId);
		}
	}

	return 0;
}


bool CmpVec::operator()(const PxVec3& v1, const PxVec3& v2) const
{
	auto v = (v2 - v1).abs();
	if (v.x < 1e-5)
	{
		if (v.y < 1e-5)
		{
			return v1.z < v2.z;
		}
		return v1.y < v2.y;
	}
	return v1.x < v2.x;
}


int32_t FractureToolImpl::cutout(uint32_t chunkId, CutoutConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd)
{
	if ((replaceChunk && chunkId == 0) || conf.cutoutSet == nullptr)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
        deleteChunkSubhierarchy(chunkId);
	}
	chunkIndex                      = getChunkIndex(chunkId);
	Nv::Blast::CutoutSet& cutoutSet = *conf.cutoutSet;

	Mesh* mesh            = new MeshImpl(*reinterpret_cast<MeshImpl*>(mChunkData[chunkIndex].meshData));
	float extrusionLength = toPxShared(mesh->getBoundingBox()).getDimensions().magnitude();
	auto scale            = toPxShared(conf.scale) / mScaleFactor;
	conf.transform.p      = (conf.transform.p - mOffset) / mScaleFactor;
	if (scale.x < 0.f || scale.y < 0.f)
	{
		scale = { extrusionLength, extrusionLength };
	}
	if (conf.isRelativeTransform)
	{
		toPxShared(conf.transform.p) += toPxShared(mesh->getBoundingBox()).getCenter() / mScaleFactor;
	}
	toPxShared(conf.noise.samplingInterval) /= mScaleFactor;
	float xDim = cutoutSet.getDimensions().x;
	float yDim = cutoutSet.getDimensions().y;

	if (conf.cutoutSet->isPeriodic())  // cutout with periodic boundary do not support noise and conicity
	{
		conf.aperture        = 0.f;
		conf.noise.amplitude = 0.f;
	}

	BooleanEvaluator bTool;
	ChunkInfo ch;
	ch.isLeaf    = true;
	ch.isChanged = true;
	ch.flags     = ChunkInfo::NO_FLAGS;
	ch.parent    = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<uint32_t> newlyCreatedChunksIds;

	SharedFacesMap sharedFacesMap;
	std::vector<std::vector<PxVec3> > verts;
	std::vector<std::set<int32_t> > smoothingGroups;
	std::vector<uint32_t> cutoutStarts;

	for (uint32_t c = 0; c < cutoutSet.getCutoutCount(); c++)
	{
		cutoutStarts.push_back(verts.size());
		for (uint32_t l = 0; l < cutoutSet.getCutoutLoopCount(c); l++)
		{
			uint32_t vertCount = cutoutSet.getCutoutVertexCount(c, l);
			verts.push_back(std::vector<PxVec3>(vertCount));
			smoothingGroups.push_back(std::set<int32_t>());
			for (uint32_t v = 0; v < vertCount; v++)
			{
				auto vert       = cutoutSet.getCutoutVertex(c, l, v);
				vert.x          = (vert.x / xDim - 0.5f) * scale.x;
				vert.y          = (vert.y / yDim - 0.5f) * scale.y;
				verts.back()[v] = toPxShared(vert);

				if (cutoutSet.isCutoutVertexToggleSmoothingGroup(c, l, v))
				{
					smoothingGroups.back().insert(v);
				}
			}
		}
	}

	float dimension = scale.magnitude();
	float conicityMultiplierBot =
	    1.f + 2.f * extrusionLength / dimension *
	              physx::PxTan(physx::PxClamp(conf.aperture, -179.f, 179.f) * physx::PxPi / 360.f);
	float conicityMultiplierTop = 2.f - conicityMultiplierBot;
	float heightBot = extrusionLength, heightTop = extrusionLength;
	if (conicityMultiplierBot < 0.f)
	{
		conicityMultiplierBot = 0.f;
		heightBot             = 0.5f * dimension / std::abs(physx::PxTan(conf.aperture * physx::PxPi / 360.f));
	}
	if (conicityMultiplierTop < 0.f)
	{
		conicityMultiplierTop = 0.f;
		heightTop             = 0.5f * dimension / std::abs(physx::PxTan(conf.aperture * physx::PxPi / 360.f));
	}

	uint32_t seed = rnd->getRandomValue();
	buildCuttingConeFaces(conf, verts, heightBot, heightTop, conicityMultiplierBot, conicityMultiplierTop,
	                      mPlaneIndexerOffset, seed, mInteriorMaterialId, sharedFacesMap);

	std::vector<std::vector<Mesh*> > cutoutMeshes;
	for (uint32_t c = 0; c < cutoutSet.getCutoutCount(); c++)
	{
		cutoutMeshes.push_back(std::vector<Mesh*>());
		for (uint32_t l = 0; l < cutoutSet.getCutoutLoopCount(c); l++)
		{
			if (verts[cutoutStarts[c] + l].size() < 4)
			{
				continue;
			}
			cutoutMeshes.back().push_back(
			    getCuttingCone(conf, verts[cutoutStarts[c] + l], smoothingGroups[cutoutStarts[c] + l], heightBot,
			                   heightTop, conicityMultiplierBot, conicityMultiplierTop, mPlaneIndexerOffset, seed,
			                   mInteriorMaterialId, sharedFacesMap, l != 0));
		}
	}

	std::stack<std::pair<int32_t, int32_t> > cellsStack;
	std::set<std::pair<int32_t, int32_t> > visited;
	cellsStack.push(std::make_pair(0, 0));

	while (!cellsStack.empty())
	{
		auto cell            = cellsStack.top();
		auto transformedCell = toPxShared(conf.transform).rotate(PxVec3(cell.first * scale.x, cell.second * scale.y, 0));
		cellsStack.pop();
		if (visited.find(cell) != visited.end())
		{
			continue;
		}
		visited.insert(cell);


		bool hasCutout = false;
		for (uint32_t c = 0; c < cutoutMeshes.size(); c++)
		{
			ch.meshData = 0;
			for (uint32_t l = 0; l < cutoutMeshes[c].size(); l++)
			{
				Mesh* cutoutMesh = cutoutMeshes[c][l];
				if (cutoutMesh == nullptr)
				{
					continue;
				}
				auto vertices = cutoutMesh->getVerticesWritable();
				for (uint32_t v = 0; v < cutoutMesh->getVerticesCount(); v++)
				{
					toPxShared(vertices[v].p) += transformedCell;
				}
				toPxShared(cutoutMesh->getBoundingBoxWritable().minimum) += transformedCell;
				toPxShared(cutoutMesh->getBoundingBoxWritable().maximum) += transformedCell;
				if (l == 0)
				{
					SweepingAccelerator accel(mesh);
					SweepingAccelerator dummy(cutoutMesh);
					bTool.performBoolean(mesh, cutoutMesh, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());

					ch.meshData = bTool.createNewMesh();
				}
				else
				{
					SweepingAccelerator accel(ch.meshData);
					SweepingAccelerator dummy(cutoutMesh);
					bTool.performBoolean(ch.meshData, cutoutMesh, &accel, &dummy,
					                     BooleanConfigurations::BOOLEAN_DIFFERENCE());

					ch.meshData = bTool.createNewMesh();
				}
				for (uint32_t v = 0; v < cutoutMesh->getVerticesCount(); v++)
				{
					toPxShared(vertices[v].p) -= transformedCell;
				}
				toPxShared(cutoutMesh->getBoundingBoxWritable().minimum )-= transformedCell;
				toPxShared(cutoutMesh->getBoundingBoxWritable().maximum) -= transformedCell;
			}
			if (ch.meshData != 0)
			{
				ch.chunkId = mChunkIdCounter++;
				newlyCreatedChunksIds.push_back(ch.chunkId);
				mChunkData.push_back(ch);
				hasCutout = true;
			}
		}

		if (hasCutout && cutoutSet.isPeriodic())
		{
			for (int32_t i = 0; i < 4; ++i)
			{
				const int32_t i0 = i & 1;
				const int32_t i1 = (i >> 1) & 1;
				auto newCell     = std::make_pair(cell.first + i0 - i1, cell.second + i0 + i1 - 1);
				if (visited.find(newCell) == visited.end())
				{
					cellsStack.push(newCell);
				}
			}
		}
	}

	for (uint32_t c = 0; c < cutoutMeshes.size(); c++)
	{
		for (uint32_t l = 0; l < cutoutMeshes[c].size(); l++)
		{
			SAFE_DELETE(cutoutMeshes[c][l]);
		}
	}
	SAFE_DELETE(mesh);

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
        deleteChunkSubhierarchy(chunkId, true);
	}

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}

int32_t FractureToolImpl::getChunkIndex(int32_t chunkId)
{
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (mChunkData[i].chunkId == chunkId)
		{
			return i;
		}
	}
	return -1;
}

int32_t FractureToolImpl::getChunkDepth(int32_t chunkId)
{
	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return -1;
	}

	int32_t depth = 0;

	while (mChunkData[chunkIndex].parent != -1)
	{
		++depth;
		chunkIndex = getChunkIndex(mChunkData[chunkIndex].parent);
	}
	return depth;
}

uint32_t FractureToolImpl::getChunksIdAtDepth(uint32_t depth, int32_t*& chunkIds)
{
	std::vector<int32_t> _chunkIds;

	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (getChunkDepth(mChunkData[i].chunkId) == (int32_t)depth)
		{
			_chunkIds.push_back(mChunkData[i].chunkId);
		}
	}
	chunkIds = new int32_t[_chunkIds.size()];
	memcpy(chunkIds, _chunkIds.data(), _chunkIds.size() * sizeof(int32_t));

	return (uint32_t)_chunkIds.size();
}


void FractureToolImpl::getTransformation(NvcVec3& offset, float& scale)
{
	offset = mOffset;
	scale  = mScaleFactor;
}

void FractureToolImpl::setSourceMesh(const Mesh* meshInput)
{
	if (meshInput == nullptr)
	{
		return;
	}
	reset();

	if (isMeshContainOpenEdges(meshInput))
	{
		NVBLAST_LOG_WARNING("Input mesh contains open edges, it may lead to wrong fractruing results!. \n");
	}


	// mChunkData.resize(1);
	// mChunkData[0].meshData = new MeshImpl(*reinterpret_cast <const MeshImpl*>(meshInput));
	// mChunkData[0].parent = -1;
	// mChunkData[0].isLeaf = true;
	// mChunkData[0].chunkId = mChunkIdCounter++;
	// Mesh* mesh = mChunkData[0].meshData;

	/**
	Move to origin and scale to unit cube
	*/

	mOffset         = (meshInput->getBoundingBox().maximum + meshInput->getBoundingBox().minimum) * 0.5f;
	NvcVec3 bbSizes = (meshInput->getBoundingBox().maximum - meshInput->getBoundingBox().minimum);

	mScaleFactor = std::max(bbSizes.x, std::max(bbSizes.y, bbSizes.z));

	setChunkMesh(meshInput, -1);

	// Vertex* verticesBuffer = mesh->getVerticesWritable();
	// for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
	//{
	//	verticesBuffer[i].p = (verticesBuffer[i].p - mOffset) * (1.0f / mScaleFactor);
	//}

	// mesh->getBoundingBoxWritable().minimum = (mesh->getBoundingBox().minimum - mOffset) * (1.0f / mScaleFactor);
	// mesh->getBoundingBoxWritable().maximum = (mesh->getBoundingBox().maximum - mOffset) * (1.0f / mScaleFactor);


	// for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	//{
	//	mesh->getFacetWritable(i)->userData = 0; // Mark facet as initial boundary facet
	//}
}

int32_t FractureToolImpl::setChunkMesh(const Mesh* meshInput, int32_t parentId)
{
	ChunkInfo* parent = nullptr;
	for (size_t i = 0; i < mChunkData.size(); i++)
	{
		if (mChunkData[i].chunkId == parentId)
		{
			parent = &mChunkData[i];
		}
	}
	if (meshInput == nullptr || (parent == nullptr && parentId != -1))
	{
		return -1;
	}

	mChunkData.push_back(ChunkInfo());
	auto& chunk     = mChunkData.back();
	chunk.meshData  = new MeshImpl(*reinterpret_cast<const MeshImpl*>(meshInput));
	chunk.parent    = parentId;
	chunk.isLeaf    = true;
	chunk.isChanged = true;
	chunk.flags     = ChunkInfo::NO_FLAGS;
	if ((size_t)parentId < mChunkData.size())
	{
		mChunkData[parentId].isLeaf = false;
	}
	chunk.chunkId = mChunkIdCounter++;
	Mesh* mesh    = chunk.meshData;

	/**
	Move to origin and scale to unit cube
	*/

	Vertex* verticesBuffer = mesh->getVerticesWritable();
	for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
	{
		verticesBuffer[i].p = (verticesBuffer[i].p - mOffset) * (1.0f / mScaleFactor);
	}

	mesh->getBoundingBoxWritable().minimum = (mesh->getBoundingBox().minimum - mOffset) * (1.0f / mScaleFactor);
	mesh->getBoundingBoxWritable().maximum = (mesh->getBoundingBox().maximum - mOffset) * (1.0f / mScaleFactor);

	if (parentId == -1)  // We are setting root mesh. Set all facets as boundary.
	{
		for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
		{
			mesh->getFacetWritable(i)->userData = 0;  // Mark facet as initial boundary facet
		}
	}

	return chunk.chunkId;
}

void FractureToolImpl::release()
{
	delete this;
}


void FractureToolImpl::reset()
{
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		delete mChunkPostprocessors[i];
	}
	mChunkPostprocessors.clear();
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		delete mChunkData[i].meshData;
	}
	mChunkData.clear();
	mPlaneIndexerOffset = 1;
	mChunkIdCounter     = 0;
	mInteriorMaterialId = kMaterialInteriorId;
}


void FractureToolImpl::setInteriorMaterialId(int32_t materialId)
{
	mInteriorMaterialId = materialId;
}

bool FractureToolImpl::isAncestorForChunk(int32_t ancestorId, int32_t chunkId)
{
	if (ancestorId == chunkId)
	{
		return false;
	}
	while (chunkId != -1)
	{
		if (ancestorId == chunkId)
		{
			return true;
		}
		chunkId = getChunkIndex(chunkId);
		if (chunkId == -1)
		{
			return false;
		}
		chunkId = mChunkData[chunkId].parent;
	}
	return false;
}

bool FractureToolImpl::deleteChunkSubhierarchy(int32_t chunkId, bool deleteRoot /*= false*/)
{
	std::vector<int32_t> chunkToDelete;
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (isAncestorForChunk(chunkId, mChunkData[i].chunkId) || (deleteRoot && chunkId == mChunkData[i].chunkId))
		{
			chunkToDelete.push_back(i);
		}
	}
	for (int32_t i = (int32_t)chunkToDelete.size() - 1; i >= 0; --i)
	{
		int32_t m = chunkToDelete[i];
		delete mChunkData[m].meshData;
		std::swap(mChunkData.back(), mChunkData[m]);
		mChunkData.pop_back();
	}
    markLeaves();
	return chunkToDelete.size() > 0;
}

void FractureToolImpl::finalizeFracturing()
{
	std::vector<Triangulator*> oldTriangulators = mChunkPostprocessors;
	std::map<int32_t, int32_t> chunkIdToTriangulator;
	std::set<uint32_t> newChunkMask;
	for (uint32_t i = 0; i < oldTriangulators.size(); ++i)
	{
		chunkIdToTriangulator[oldTriangulators[i]->getParentChunkId()] = i;
	}
	mChunkPostprocessors.clear();
	mChunkPostprocessors.resize(mChunkData.size());
	newChunkMask.insert(0xffffffff);  // To trigger masking mode, if newChunkMask will happen to be empty, all UVs will
	                                  // be updated.
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{

		auto it = chunkIdToTriangulator.find(mChunkData[i].chunkId);
		if (mChunkData[i].isChanged || it == chunkIdToTriangulator.end())
		{
			if (it != chunkIdToTriangulator.end())
			{
				delete oldTriangulators[it->second];
				oldTriangulators[it->second] = nullptr;
			}
			mChunkPostprocessors[i] = new Triangulator();
			mChunkPostprocessors[i]->triangulate(mChunkData[i].meshData);
			mChunkPostprocessors[i]->getParentChunkId() = mChunkData[i].chunkId;
			newChunkMask.insert(mChunkData[i].chunkId);
			mChunkData[i].isChanged = false;
		}
		else
		{
			mChunkPostprocessors[i] = oldTriangulators[it->second];
		}
	}

	std::vector<int32_t> badOnes;
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		if (mChunkPostprocessors[i]->getBaseMesh().empty())
		{
			badOnes.push_back(i);
		}
	}
	for (int32_t i = (int32_t)badOnes.size() - 1; i >= 0; --i)
	{
		int32_t chunkId = mChunkData[badOnes[i]].chunkId;
		for (uint32_t j = 0; j < mChunkData.size(); ++j)
		{
			if (mChunkData[j].parent == chunkId)
				mChunkData[j].parent = mChunkData[badOnes[i]].parent;
		}
		std::swap(mChunkPostprocessors[badOnes[i]], mChunkPostprocessors.back());
		mChunkPostprocessors.pop_back();
		std::swap(mChunkData[badOnes[i]], mChunkData.back());
		mChunkData.pop_back();
	}
    if (!mChunkPostprocessors.empty())  // Failsafe to prevent infinite loop (leading to stack overflow)
    {
        fitAllUvToRect(1.0f, newChunkMask);
    }
}

uint32_t FractureToolImpl::getChunkCount() const
{
	return (uint32_t)mChunkData.size();
}

const ChunkInfo& FractureToolImpl::getChunkInfo(int32_t chunkIndex)
{
	return mChunkData[chunkIndex];
}

uint32_t FractureToolImpl::getBaseMesh(int32_t chunkIndex, Triangle*& output)
{
	NVBLAST_ASSERT(mChunkPostprocessors.size() > 0);
	if (mChunkPostprocessors.size() == 0)
	{
		return 0;  // finalizeFracturing() should be called before getting mesh!
	}
	auto& baseMesh = mChunkPostprocessors[chunkIndex]->getBaseMesh();
	output         = new Triangle[baseMesh.size()];
	memcpy(output, baseMesh.data(), baseMesh.size() * sizeof(Triangle));

	/* Scale mesh back */

	for (uint32_t i = 0; i < baseMesh.size(); ++i)
	{
		Triangle& triangle = output[i];
		triangle.a.p       = triangle.a.p * mScaleFactor + mOffset;
		triangle.b.p       = triangle.b.p * mScaleFactor + mOffset;
		triangle.c.p       = triangle.c.p * mScaleFactor + mOffset;
	}

	return baseMesh.size();
}

uint32_t FractureToolImpl::updateBaseMesh(int32_t chunkIndex, Triangle* output)
{
	NVBLAST_ASSERT(mChunkPostprocessors.size() > 0);
	if (mChunkPostprocessors.size() == 0)
	{
		return 0;  // finalizeFracturing() should be called before getting mesh!
	}
	auto& baseMesh = mChunkPostprocessors[chunkIndex]->getBaseMesh();
	memcpy(output, baseMesh.data(), baseMesh.size() * sizeof(Triangle));

	/* Scale mesh back */

	for (uint32_t i = 0; i < baseMesh.size(); ++i)
	{
		Triangle& triangle = output[i];
		triangle.a.p       = triangle.a.p * mScaleFactor + mOffset;
		triangle.b.p       = triangle.b.p * mScaleFactor + mOffset;
		triangle.c.p       = triangle.c.p * mScaleFactor + mOffset;
	}
	return baseMesh.size();
}


float getVolume(std::vector<Triangle>& triangles)
{
    if (triangles.size() == 0)
    {
        return 0.0f;
    }

    // Find an approximate centroid for a more accurate calculation
    NvcVec3 centroid = { 0.0f, 0.0f, 0.0f };
    for (size_t i = 0; i < triangles.size(); ++i)
    {
        centroid = centroid + triangles[i].a.p + triangles[i].b.p + triangles[i].c.p;
    }
    centroid = centroid / (3 * triangles.size());

	float volume = 0.0f;

	for (size_t i = 0; i < triangles.size(); ++i)
	{
		const NvcVec3 a = triangles[i].a.p - centroid;
		const NvcVec3 b = triangles[i].b.p - centroid;
		const NvcVec3 c = triangles[i].c.p - centroid;
		volume +=
		    (a.x * b.y * c.z - a.x * b.z * c.y - a.y * b.x * c.z + a.y * b.z * c.x + a.z * b.x * c.y - a.z * b.y * c.x);
	}
	return (1.0f / 6.0f) * std::abs(volume);
}

float FractureToolImpl::getMeshOverlap(const Mesh& meshA, const Mesh& meshB)
{
	BooleanEvaluator bTool;
	bTool.performBoolean(&meshA, &meshB, BooleanConfigurations::BOOLEAN_INTERSECION());
	Mesh* result = bTool.createNewMesh();
	if (result == nullptr)
	{
		return 0.0f;
	}

	Triangulator postProcessor;
	postProcessor.triangulate(&meshA);

	float baseVolume = getVolume(postProcessor.getBaseMesh());
	if (baseVolume == 0)
	{
		return 0.0f;
	}
	postProcessor.triangulate(result);
	float intrsVolume = getVolume(postProcessor.getBaseMesh());

	delete result;

	return intrsVolume / baseVolume;
}

void weldVertices(std::map<Vertex, uint32_t, VrtComp>& vertexMapping, std::vector<Vertex>& vertexBuffer,
                  std::vector<uint32_t>& indexBuffer, std::vector<Triangle>& trb)
{
	for (uint32_t i = 0; i < trb.size(); ++i)
	{
		auto it = vertexMapping.find(trb[i].a);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].a] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].a);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
		it = vertexMapping.find(trb[i].b);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].b] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].b);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
		it = vertexMapping.find(trb[i].c);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].c] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].c);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
	}
}

void FractureToolImpl::setRemoveIslands(bool isRemoveIslands)
{
	mRemoveIslands = isRemoveIslands;
}

int32_t FractureToolImpl::islandDetectionAndRemoving(int32_t chunkId, bool createAtNewDepth)
{
	if (chunkId == 0 && createAtNewDepth == false)
	{
		return 0;
	}
	int32_t chunkIndex = getChunkIndex(chunkId);
	Triangulator prc;
	prc.triangulate(mChunkData[chunkIndex].meshData);

	Mesh* chunk = mChunkData[chunkIndex].meshData;

	std::vector<uint32_t>& mapping    = prc.getBaseMapping();
	std::vector<TriangleIndexed>& trs = prc.getBaseMeshIndexed();

	std::vector<std::vector<uint32_t> > graph(prc.getWeldedVerticesCount());
	std::vector<int32_t>& pm = prc.getPositionedMapping();
	if (pm.size() == 0)
	{
		return 0;
	}

	/**
	    Chunk graph
	*/
	for (uint32_t i = 0; i < trs.size(); ++i)
	{
		graph[pm[trs[i].ea]].push_back(pm[trs[i].eb]);
		graph[pm[trs[i].ea]].push_back(pm[trs[i].ec]);

		graph[pm[trs[i].ec]].push_back(pm[trs[i].eb]);
		graph[pm[trs[i].ec]].push_back(pm[trs[i].ea]);

		graph[pm[trs[i].eb]].push_back(pm[trs[i].ea]);
		graph[pm[trs[i].eb]].push_back(pm[trs[i].ec]);
	}
	for (uint32_t i = 0; i < chunk->getEdgesCount(); ++i)
	{
		int v1 = chunk->getEdges()[i].s;
		int v2 = chunk->getEdges()[i].e;

		v1 = pm[mapping[v1]];
		v2 = pm[mapping[v2]];

		graph[v1].push_back(v2);
		graph[v2].push_back(v1);
	}


	/**
	    Walk graph, mark components
	*/

	std::vector<int32_t> comps(prc.getWeldedVerticesCount(), -1);
	std::queue<uint32_t> que;
	int32_t cComp = 0;

	for (uint32_t i = 0; i < prc.getWeldedVerticesCount(); ++i)
	{
		int32_t to = pm[i];
		if (comps[to] != -1)
			continue;
		que.push(to);
		comps[to] = cComp;

		while (!que.empty())
		{
			int32_t c = que.front();
			que.pop();

			for (uint32_t j = 0; j < graph[c].size(); ++j)
			{
				if (comps[graph[c][j]] == -1)
				{
					que.push(graph[c][j]);
					comps[graph[c][j]] = cComp;
				}
			}
		}
		cComp++;
	}
	for (uint32_t i = 0; i < prc.getWeldedVerticesCount(); ++i)
	{
		int32_t to = pm[i];
		comps[i]   = comps[to];
	}
	std::vector<uint32_t> longComps(chunk->getVerticesCount());
	for (uint32_t i = 0; i < chunk->getVerticesCount(); ++i)
	{
		int32_t to   = mapping[i];
		longComps[i] = comps[to];
	}

	if (cComp > 1)
	{
		std::vector<std::vector<Vertex> > compVertices(cComp);
		std::vector<std::vector<Facet> > compFacets(cComp);
		std::vector<std::vector<Edge> > compEdges(cComp);


		std::vector<uint32_t> compVertexMapping(chunk->getVerticesCount(), 0);
		const Vertex* vrts = chunk->getVertices();
		for (uint32_t v = 0; v < chunk->getVerticesCount(); ++v)
		{
			int32_t vComp        = comps[mapping[v]];
			compVertexMapping[v] = static_cast<uint32_t>(compVertices[vComp].size());
			compVertices[vComp].push_back(vrts[v]);
		}

		const Facet* fcb = chunk->getFacetsBuffer();
		const Edge* edb  = chunk->getEdges();

		for (uint32_t fc = 0; fc < chunk->getFacetCount(); ++fc)
		{
			std::vector<uint32_t> edgesPerComp(cComp, 0);
			for (uint32_t ep = fcb[fc].firstEdgeNumber; ep < fcb[fc].firstEdgeNumber + fcb[fc].edgesCount; ++ep)
			{
				int32_t vComp = comps[mapping[edb[ep].s]];
				edgesPerComp[vComp]++;
				compEdges[vComp].push_back({compVertexMapping[edb[ep].s], compVertexMapping[edb[ep].e]});
			}
			for (int32_t c = 0; c < cComp; ++c)
			{
				if (edgesPerComp[c] == 0)
				{
					continue;
				}
				compFacets[c].push_back(*chunk->getFacet(fc));
				compFacets[c].back().edgesCount      = edgesPerComp[c];
				compFacets[c].back().firstEdgeNumber = static_cast<int32_t>(compEdges[c].size()) - edgesPerComp[c];
			}
		}

		if (createAtNewDepth == false || chunkId != 0)
		{
			delete mChunkData[chunkIndex].meshData;
			mChunkData[chunkIndex].meshData =
			    new MeshImpl(compVertices[0].data(), compEdges[0].data(), compFacets[0].data(),
			                 static_cast<uint32_t>(compVertices[0].size()), static_cast<uint32_t>(compEdges[0].size()),
			                 static_cast<uint32_t>(compFacets[0].size()));
			;
			for (int32_t i = 1; i < cComp; ++i)
			{
				mChunkData.push_back(ChunkInfo(mChunkData[chunkIndex]));
				mChunkData.back().chunkId = mChunkIdCounter++;
				mChunkData.back().meshData =
				    new MeshImpl(compVertices[i].data(), compEdges[i].data(), compFacets[i].data(),
				                 static_cast<uint32_t>(compVertices[i].size()),
				                 static_cast<uint32_t>(compEdges[i].size()), static_cast<uint32_t>(compFacets[i].size()));
			}
		}
		else
		{
            deleteChunkSubhierarchy(chunkId);
			for (int32_t i = 0; i < cComp; ++i)
			{
				uint32_t nc             = createNewChunk(chunkId);
				mChunkData[nc].isLeaf   = true;
				mChunkData[nc].flags    = ChunkInfo::APPROXIMATE_BONDING;
				mChunkData[nc].meshData = new MeshImpl(compVertices[i].data(), compEdges[i].data(), compFacets[i].data(),
				                                       static_cast<uint32_t>(compVertices[i].size()),
				                                       static_cast<uint32_t>(compEdges[i].size()),
				                                       static_cast<uint32_t>(compFacets[i].size()));
			}
            mChunkData[chunkIndex].isLeaf = false;
        }
		return cComp;
	}
	return 0;
}

uint32_t
FractureToolImpl::getBufferedBaseMeshes(Vertex*& vertexBuffer, uint32_t*& indexBuffer, uint32_t*& indexBufferOffsets)
{
	std::map<Vertex, uint32_t, VrtComp> vertexMapping;
	std::vector<Vertex> _vertexBuffer;
	std::vector<std::vector<uint32_t> > _indexBuffer(mChunkPostprocessors.size());

	indexBufferOffsets = new uint32_t[mChunkPostprocessors.size() + 1];

	uint32_t totalIndices = 0;
	for (uint32_t ch = 0; ch < mChunkPostprocessors.size(); ++ch)
	{
		std::vector<Triangle>& trb = mChunkPostprocessors[ch]->getBaseMesh();

		weldVertices(vertexMapping, _vertexBuffer, _indexBuffer[ch], trb);

		indexBufferOffsets[ch] = totalIndices;
		totalIndices += _indexBuffer[ch].size();
	}
	indexBufferOffsets[mChunkPostprocessors.size()] = totalIndices;

	for (uint32_t i = 0; i < _vertexBuffer.size(); ++i)
	{
		_vertexBuffer[i].p = _vertexBuffer[i].p * mScaleFactor + mOffset;
	}

	vertexBuffer = new Vertex[_vertexBuffer.size()];
	indexBuffer  = new uint32_t[totalIndices];

	memcpy(vertexBuffer, _vertexBuffer.data(), _vertexBuffer.size() * sizeof(Vertex));
	for (uint32_t ch = 0; ch < _indexBuffer.size(); ++ch)
	{
		memcpy(indexBuffer + indexBufferOffsets[ch], _indexBuffer[ch].data(), _indexBuffer[ch].size() * sizeof(uint32_t));
	}

	return _vertexBuffer.size();
}

int32_t FractureToolImpl::getChunkId(int32_t chunkIndex)
{
	if (chunkIndex < 0 || static_cast<uint32_t>(chunkIndex) >= mChunkData.size())
	{
		return -1;
	}
	return mChunkData[chunkIndex].chunkId;
}

int32_t FractureToolImpl::getInteriorMaterialId() const
{
	return mInteriorMaterialId;
}


void FractureToolImpl::replaceMaterialId(int32_t oldMaterialId, int32_t newMaterialId)
{
	for (auto& chunkData : mChunkData)
	{
		if (chunkData.meshData)
		{
			chunkData.meshData->replaceMaterialId(oldMaterialId, newMaterialId);
		}
	}
}

uint32_t FractureToolImpl::stretchGroup(const std::vector<uint32_t>& grp, std::vector<std::vector<uint32_t> >& graph)
{
	uint32_t parent        = mChunkData[grp[0]].parent;
	uint32_t newChunkIndex = createNewChunk(parent);
	graph.push_back(std::vector<uint32_t>());


	std::vector<Vertex> nVertices;
	std::vector<Edge> nEdges;
	std::vector<Facet> nFacets;

	uint32_t offsetVertices = 0;
	uint32_t offsetEdges    = 0;

	for (uint32_t i = 0; i < grp.size(); ++i)
	{
		mChunkData[grp[i]].parent = mChunkData[newChunkIndex].chunkId;

		auto vr = mChunkData[grp[i]].meshData->getVertices();
		auto ed = mChunkData[grp[i]].meshData->getEdges();
		auto fc = mChunkData[grp[i]].meshData->getFacetsBuffer();


		for (uint32_t v = 0; v < mChunkData[grp[i]].meshData->getVerticesCount(); ++v)
		{
			nVertices.push_back(vr[v]);
		}
		for (uint32_t v = 0; v < mChunkData[grp[i]].meshData->getEdgesCount(); ++v)
		{
			nEdges.push_back(ed[v]);
			nEdges.back().s += offsetVertices;
			nEdges.back().e += offsetVertices;
		}
		for (uint32_t v = 0; v < mChunkData[grp[i]].meshData->getFacetCount(); ++v)
		{
			nFacets.push_back(fc[v]);
			nFacets.back().firstEdgeNumber += offsetEdges;
		}
		offsetEdges    = nEdges.size();
		offsetVertices = nVertices.size();

        if (mChunkData[grp[i]].flags & ChunkInfo::APPROXIMATE_BONDING)
        {
            mChunkData[newChunkIndex].flags |= ChunkInfo::APPROXIMATE_BONDING;
        }
    }
	std::vector<Facet> finalFacets;
	std::set<int64_t> hasCutting;
	for (uint32_t i = 0; i < nFacets.size(); ++i)
	{
		if (nFacets[i].userData != 0)
			hasCutting.insert(nFacets[i].userData);
	}
	for (uint32_t i = 0; i < nFacets.size(); ++i)
	{
		if (nFacets[i].userData == 0 || (hasCutting.find(-nFacets[i].userData) == hasCutting.end()) ||
		    std::abs(nFacets[i].userData) >= SLICING_INDEXER_OFFSET)
		{
			finalFacets.push_back(nFacets[i]);
		}
	}
	mChunkData[newChunkIndex].meshData =
	    new MeshImpl(nVertices.data(), nEdges.data(), finalFacets.data(), static_cast<uint32_t>(nVertices.size()),
	                 static_cast<uint32_t>(nEdges.size()), static_cast<uint32_t>(finalFacets.size()));

	return newChunkIndex;
}

uint32_t FractureToolImpl::createNewChunk(uint32_t parent)
{
	mChunkData.push_back(ChunkInfo());
	mChunkData.back().parent    = parent;
	mChunkData.back().chunkId   = mChunkIdCounter++;
	mChunkData.back().meshData  = nullptr;
	mChunkData.back().isLeaf    = false;
	mChunkData.back().isChanged = true;
	mChunkData.back().flags     = ChunkInfo::NO_FLAGS;

	return mChunkData.size() - 1;
}


void FractureToolImpl::fitUvToRect(float side, uint32_t chunk)
{
	int32_t index = getChunkIndex(chunk);
	if (mChunkPostprocessors.empty())  // It seems finalize have not been called, call it here.
	{
		finalizeFracturing();
	}
	if (index == -1 || (int32_t)mChunkPostprocessors.size() <= index)
	{
		return;  // We dont have such chunk tringulated;
	}
	physx::PxBounds3 bnd;
	bnd.setEmpty();

	std::vector<Triangle>& ctrs   = mChunkPostprocessors[index]->getBaseMesh();
	std::vector<Triangle>& output = mChunkPostprocessors[index]->getBaseMesh();

	for (uint32_t trn = 0; trn < ctrs.size(); ++trn)
	{
		if (ctrs[trn].userData == 0)
			continue;
		bnd.include(PxVec3(ctrs[trn].a.uv[0].x, ctrs[trn].a.uv[0].y, 0.0f));
		bnd.include(PxVec3(ctrs[trn].b.uv[0].x, ctrs[trn].b.uv[0].y, 0.0f));
		bnd.include(PxVec3(ctrs[trn].c.uv[0].x, ctrs[trn].c.uv[0].y, 0.0f));
	}

	float xscale = side / (bnd.maximum.x - bnd.minimum.x);
	float yscale = side / (bnd.maximum.y - bnd.minimum.y);
	xscale       = std::min(xscale, yscale);  // To have uniform scaling

	for (uint32_t trn = 0; trn < ctrs.size(); ++trn)
	{
		if (ctrs[trn].userData == 0)
			continue;
		output[trn].a.uv[0].x = (ctrs[trn].a.uv[0].x - bnd.minimum.x) * xscale;
		output[trn].b.uv[0].x = (ctrs[trn].b.uv[0].x - bnd.minimum.x) * xscale;
		output[trn].c.uv[0].x = (ctrs[trn].c.uv[0].x - bnd.minimum.x) * xscale;

		output[trn].a.uv[0].y = (ctrs[trn].a.uv[0].y - bnd.minimum.y) * xscale;
		output[trn].b.uv[0].y = (ctrs[trn].b.uv[0].y - bnd.minimum.y) * xscale;
		output[trn].c.uv[0].y = (ctrs[trn].c.uv[0].y - bnd.minimum.y) * xscale;
	}
}

void FractureToolImpl::fitAllUvToRect(float side)
{
	std::set<uint32_t> mask;
	fitAllUvToRect(side, mask);
}

void FractureToolImpl::fitAllUvToRect(float side, std::set<uint32_t>& mask)
{
	if (mChunkPostprocessors.empty())  // It seems finalize have not been called, call it here.
	{
		finalizeFracturing();
	}
	if (mChunkPostprocessors.empty())
	{
		return;  // We dont have triangulated chunks.
	}
	physx::PxBounds3 bnd;
	bnd.setEmpty();

	for (uint32_t chunk = 0; chunk < mChunkData.size(); ++chunk)
	{
		Mesh* m                = mChunkData[chunk].meshData;
		const Edge* edges      = m->getEdges();
		const Vertex* vertices = m->getVertices();

		for (uint32_t trn = 0; trn < m->getFacetCount(); ++trn)
		{
			if (m->getFacet(trn)->userData == 0)
				continue;
			for (uint32_t ei = 0; ei < m->getFacet(trn)->edgesCount; ++ei)
			{
				int32_t v1 = edges[m->getFacet(trn)->firstEdgeNumber + ei].s;
				int32_t v2 = edges[m->getFacet(trn)->firstEdgeNumber + ei].e;
				bnd.include(PxVec3(vertices[v1].uv[0].x, vertices[v1].uv[0].y, 0.0f));
				bnd.include(PxVec3(vertices[v2].uv[0].x, vertices[v2].uv[0].y, 0.0f));
			}
		}
	}
	float xscale = side / (bnd.maximum.x - bnd.minimum.x);
	float yscale = side / (bnd.maximum.y - bnd.minimum.y);
	xscale       = std::min(xscale, yscale);  // To have uniform scaling

	for (uint32_t chunk = 0; chunk < mChunkPostprocessors.size(); ++chunk)
	{
		if (!mask.empty() && mask.find(mChunkPostprocessors[chunk]->getParentChunkId()) == mask.end())
			continue;
		std::vector<Triangle>& ctrs   = mChunkPostprocessors[chunk]->getBaseMeshNotFitted();
		std::vector<Triangle>& output = mChunkPostprocessors[chunk]->getBaseMesh();

		for (uint32_t trn = 0; trn < ctrs.size(); ++trn)
		{
			if (ctrs[trn].userData == 0)
				continue;
			output[trn].a.uv[0].x = (ctrs[trn].a.uv[0].x - bnd.minimum.x) * xscale;
			output[trn].b.uv[0].x = (ctrs[trn].b.uv[0].x - bnd.minimum.x) * xscale;
			output[trn].c.uv[0].x = (ctrs[trn].c.uv[0].x - bnd.minimum.x) * xscale;

			output[trn].a.uv[0].y = (ctrs[trn].a.uv[0].y - bnd.minimum.y) * xscale;
			output[trn].b.uv[0].y = (ctrs[trn].b.uv[0].y - bnd.minimum.y) * xscale;
			output[trn].c.uv[0].y = (ctrs[trn].c.uv[0].y - bnd.minimum.y) * xscale;
		}
	}
}

void FractureToolImpl::markLeaves()
{
    for (ChunkInfo& info : mChunkData)
    {
        info.isLeaf = true;
    }

    for (ChunkInfo& info : mChunkData)
    {
        const int32_t index = getChunkIndex(info.parent);
        if (index >= 0)
        {
            mChunkData[index].isLeaf = false;
        }
    }
}

void FractureToolImpl::rebuildAdjGraph(const std::vector<uint32_t>& chunks, const NvcVec2i* adjChunks,
                                       uint32_t adjChunksSize, std::vector<std::vector<uint32_t> >& chunkGraph)
{
	std::vector<std::pair<uint64_t, uint32_t> > planeChunkIndex;

	for (uint32_t i = 0; i < chunks.size(); ++i)
	{
		for (uint32_t fc = 0; fc < mChunkData[chunks[i]].meshData->getFacetCount(); ++fc)
		{
			if (mChunkData[chunks[i]].meshData->getFacet(fc)->userData != 0)
			{
				planeChunkIndex.push_back(
				    std::make_pair(std::abs(mChunkData[chunks[i]].meshData->getFacet(fc)->userData), chunks[i]));
			}
		}
	}
	{
		std::sort(planeChunkIndex.begin(), planeChunkIndex.end());
		auto it = std::unique(planeChunkIndex.begin(), planeChunkIndex.end());
		planeChunkIndex.resize(it - planeChunkIndex.begin());
	}

	uint32_t a = 0;

	for (uint32_t i = 1; i < planeChunkIndex.size(); ++i)
	{
		if (planeChunkIndex[a].first != planeChunkIndex[i].first)
		{
			uint32_t b = i;

			for (uint32_t p1 = a; p1 < b; ++p1)
			{
				for (uint32_t p2 = p1 + 1; p2 < b; ++p2)
				{
					if (planeChunkIndex[p1].second == planeChunkIndex[p2].second ||
					    mChunkData[planeChunkIndex[p1].second].parent != mChunkData[planeChunkIndex[p2].second].parent)
					{
						continue;
					}
					bool has = false;
					for (uint32_t k = 0; k < chunkGraph[planeChunkIndex[p1].second].size(); ++k)
					{
						if (chunkGraph[planeChunkIndex[p1].second][k] == planeChunkIndex[p2].second)
						{
							has = true;
							break;
						}
					}
					if (!has)
					{
						chunkGraph[planeChunkIndex[p1].second].push_back(planeChunkIndex[p2].second);
					}
					has = false;
					for (uint32_t k = 0; k < chunkGraph[planeChunkIndex[p2].second].size(); ++k)
					{
						if (chunkGraph[planeChunkIndex[p2].second][k] == planeChunkIndex[p1].second)
						{
							has = true;
							break;
						}
					}
					if (!has)
					{
						chunkGraph[planeChunkIndex[p2].second].push_back(planeChunkIndex[p1].second);
					}
				}
			}
			a = b;
		}
	}

    // Add in extra adjacency info, if we have it
    if (adjChunks && adjChunksSize)
    {
		std::set<uint32_t> chunkSet(chunks.begin(), chunks.end());

#if NV_DEBUG || NV_CHECKED  // Make sure these arrays are sorted
        for (std::vector<uint32_t>& adj : chunkGraph)
        {
			const bool isSorted = std::is_sorted(adj.begin(), adj.end());
			if (!isSorted)
            {
			    NVBLAST_ASSERT(0);
			    NvBlastGlobalGetErrorCallback()->reportError(Nv::Blast::ErrorCode::eDEBUG_WARNING, "Adjacency array not sorted; subsequent code assumes it is.", __FILE__, __LINE__);
            }
        }
#endif
        for (uint32_t i = 0; i < adjChunksSize; ++i)
        {
			const NvcVec2i& pair = adjChunks[i];
			if (chunkSet.find((uint32_t)pair.x) == chunkSet.end() || chunkSet.find((uint32_t)pair.y) == chunkSet.end())
			{
				continue;
            }

            {
                std::vector<uint32_t>& adj0 = chunkGraph[pair.x];
			    std::vector<uint32_t>::iterator it0 = std::lower_bound(adj0.begin(), adj0.end(), (uint32_t)pair.y);
                if (it0 == adj0.end() || *it0 != (uint32_t)pair.y)
                {
				    adj0.insert(it0, (uint32_t)pair.y);
                }
            }

            {
                std::vector<uint32_t>& adj1 = chunkGraph[pair.y];
			    std::vector<uint32_t>::iterator it1 = std::lower_bound(adj1.begin(), adj1.end(), (uint32_t)pair.x);
                if (it1 == adj1.end() || *it1 != (uint32_t)pair.x)
                {
				    adj1.insert(it1, (uint32_t)pair.x);
                }
            }
        }
    }
}

bool VecIntComp(const std::pair<NvcVec3, uint32_t>& a, const std::pair<NvcVec3, uint32_t>& b)
{
	if (a.first.x < b.first.x)
		return true;
	if (a.first.x > b.first.x)
		return false;
	if (a.first.y < b.first.y)
		return true;
	if (a.first.y > b.first.y)
		return false;
	if (a.first.z < b.first.z)
		return true;
	if (a.first.z > b.first.z)
		return false;

	return a.second < b.second;
}

void FractureToolImpl::uniteChunks(uint32_t threshold, uint32_t targetClusterSize, const uint32_t* chunksToMerge, uint32_t mergeChunkCount,
                                   const NvcVec2i* adjChunks, uint32_t adjChunksSize, bool removeOriginalChunks /*= false*/)
{
	std::vector<int32_t> depth(mChunkData.size(), 0);

	std::vector<std::vector<uint32_t> > chunkGraph(mChunkData.size());

	std::vector<uint32_t> atEachDepth;
	std::vector<uint32_t> childNumber(mChunkData.size(), 0);

    std::vector<uint32_t> chunksToRemove;

    enum ChunkFlags
    {
        Mergeable   = (1 << 0),
        Merged      = (1 << 1)
    };

    std::vector<uint32_t> chunkFlags(mChunkData.size());

    if (chunksToMerge == nullptr)
    {
        std::fill(chunkFlags.begin(), chunkFlags.end(), Mergeable);
    }
    else
    {
        // Seed all mergeable chunks with Mergeable flag
        for (uint32_t chunkN = 0; chunkN < mergeChunkCount; ++chunkN)
        {
            const uint32_t chunkIndex = chunksToMerge[chunkN];
            chunkFlags[chunkIndex] |= Mergeable;
        }

        // Make all descendants mergable too
        std::vector<int32_t> treeWalk;
        for (uint32_t chunkIndex = 0; chunkIndex < mChunkData.size(); ++chunkIndex)
        {
            treeWalk.clear();
            int32_t walkIndex = (int32_t)chunkIndex;
            do
            {
                if (chunkFlags[walkIndex] & Mergeable)
                {
                    std::for_each(treeWalk.begin(), treeWalk.end(), [&chunkFlags](int32_t index) {chunkFlags[index] |= Mergeable; });
                    break;
                }
                treeWalk.push_back(walkIndex);
            } while ((walkIndex = getChunkIndex(mChunkData[walkIndex].parent)) >= 0);
        }
    }

    int32_t maxDepth = 0;

    for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (mChunkData[i].parent != -1)
			childNumber[getChunkIndex(mChunkData[i].parent)]++;
		depth[i] = getChunkDepth(mChunkData[i].chunkId);
		NVBLAST_ASSERT(depth[i] >= 0);
        maxDepth = std::max(maxDepth, depth[i]);
	}

    for (int32_t level = maxDepth; level > 0; --level)  // go from leaves to trunk and rebuild hierarchy
	{
		std::vector<uint32_t> cGroup;
		std::vector<uint32_t> chunksToUnify;

		NvcVec3 minPoint = {MAXIMUM_EXTENT, MAXIMUM_EXTENT, MAXIMUM_EXTENT};
		VrtPositionComparator posc;

		for (uint32_t ch = 0; ch < depth.size(); ++ch)
		{
			if (depth[ch] == level && childNumber[getChunkIndex(mChunkData[ch].parent)] > threshold && (chunkFlags[ch] & Mergeable) != 0)
			{
				chunksToUnify.push_back(ch);
				NvcVec3 cp = fromPxShared(toPxShared(mChunkData[ch].meshData->getBoundingBox()).getCenter());
				if (posc(cp, minPoint))
				{
					minPoint = cp;
				}
			}
		}

		std::vector<std::pair<float, uint32_t> > distances;
		for (uint32_t i = 0; i < chunksToUnify.size(); ++i)
		{
			float d = (toPxShared(minPoint) - toPxShared(mChunkData[chunksToUnify[i]].meshData->getBoundingBox()).getCenter()).magnitude();
			distances.push_back(std::make_pair(d, chunksToUnify[i]));
		}
		std::sort(distances.begin(), distances.end());

		for (uint32_t i = 0; i < chunksToUnify.size(); ++i)
		{
			chunksToUnify[i] = distances[i].second;
		}
		rebuildAdjGraph(chunksToUnify, adjChunks, adjChunksSize, chunkGraph);

		for (uint32_t iter = 0; iter < 32 && chunksToUnify.size() > threshold; ++iter)
		{
			std::vector<uint32_t> newChunksToUnify;

			for (uint32_t c = 0; c < chunksToUnify.size(); ++c)
			{
				if ((chunkFlags[chunksToUnify[c]] & Mergeable) == 0)
					continue;
				chunkFlags[chunksToUnify[c]] &= ~Mergeable;
				cGroup.push_back(chunksToUnify[c]);
				for (uint32_t sc = 0; sc < cGroup.size() && cGroup.size() < targetClusterSize; ++sc)
				{
					uint32_t sid = cGroup[sc];
					for (uint32_t neighbN = 0; neighbN < chunkGraph[sid].size() && cGroup.size() < targetClusterSize; ++neighbN)
					{
                        const uint32_t chunkNeighb = chunkGraph[sid][neighbN];
                        if (mChunkData[chunkNeighb].parent != mChunkData[sid].parent)
                            continue;
						if ((chunkFlags[chunkNeighb] & Mergeable) == 0)
							continue;
						chunkFlags[chunkNeighb] &= ~Mergeable;
                        cGroup.push_back(chunkNeighb);
                    }
				}
				if (cGroup.size() > 1)
				{
					uint32_t newChunk = stretchGroup(cGroup, chunkGraph);
                    for (uint32_t chunk : cGroup)
                    {
						if (removeOriginalChunks  && !(chunkFlags[chunk] & Merged))
                        {
							chunksToRemove.push_back(chunk);
                        }
                    }
					cGroup.clear();
					newChunksToUnify.push_back(newChunk);
					chunkFlags.push_back(Merged);
				}
				else
				{
					cGroup.clear();
				}
			}
			chunksToUnify = newChunksToUnify;
			rebuildAdjGraph(chunksToUnify, adjChunks, adjChunksSize, chunkGraph);
		}
	}

    // Remove chunks
    std::vector<uint32_t> remap(mChunkData.size(), 0xFFFFFFFF);
    std::sort(chunksToRemove.begin(), chunksToRemove.end());
    std::vector<uint32_t>::iterator removeIt = chunksToRemove.begin();
    size_t chunkWriteIndex = 0;
    for (size_t chunkReadIndex = 0; chunkReadIndex < mChunkData.size(); ++chunkReadIndex)
    {
        if (removeIt < chunksToRemove.end())
        {
            if (*removeIt == chunkReadIndex)
            {
                ++removeIt;
                continue;
            }
        }
        if (chunkReadIndex != chunkWriteIndex)
        {
            mChunkData[chunkWriteIndex] = mChunkData[chunkReadIndex];
        }
        remap[chunkReadIndex] = chunkWriteIndex++;
    }
    mChunkData.resize(chunkWriteIndex);
    for (ChunkInfo& chunkInfo : mChunkData)
    {
        if (chunkInfo.parent >= 0)
        {
            const uint32_t mappedParentIndex = remap[getChunkIndex(chunkInfo.parent)];
            NVBLAST_ASSERT(mappedParentIndex < mChunkData.size());
            if (mappedParentIndex < mChunkData.size())
            {
                chunkInfo.parent = mChunkData[mappedParentIndex].chunkId;
            }
        }
    }
}

bool FractureToolImpl::setApproximateBonding(uint32_t chunkIndex, bool useApproximateBonding)
{
    if ((size_t)chunkIndex >= mChunkData.size())
    {
        return false;
    }

    if (useApproximateBonding)
    {
        mChunkData[chunkIndex].flags |= (uint32_t)ChunkInfo::APPROXIMATE_BONDING;
    }
    else
    {
        mChunkData[chunkIndex].flags &= ~(uint32_t)ChunkInfo::APPROXIMATE_BONDING;
    }

    return true;
}

}  // namespace Blast
}  // namespace Nv
