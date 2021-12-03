#pragma once

#include "foundation/PxTransform.h"

class LearningCamera
{
public:
	LearningCamera(const physx::PxVec3 &eye, const physx::PxVec3& dir);

	void				handleMouse(int button, int state, int x, int y);
	bool				handleKey(unsigned char key, int x, int y, float speed = 1.0f);
	void				handleMotion(int x, int y);
	void				handleAnalogMove(float x, float y);

	physx::PxVec3		getEye()	const;
	physx::PxVec3		getDir()	const;
	physx::PxTransform	getTransform() const;
public:
	physx::PxVec3	mEye;
	physx::PxVec3	mDir;
	int				mMouseX;
	int				mMouseY;
};