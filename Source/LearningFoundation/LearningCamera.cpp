#include "LearningCamera.h"

#include <cstdio>
#include <ctype.h>
#include "foundation/PxMat33.h"

using namespace physx;

LearningCamera::LearningCamera(const PxVec3& eye, const PxVec3& dir)
{
	mEye = eye;
	mDir = dir.getNormalized();
	mMouseX = 0;
	mMouseY = 0;
	mFOV = 60;
	mClipNear = 1.f;
	mClipFar = 10000.f;
}

void LearningCamera::handleMouse(int button, int state, int x, int y)
{
	PX_UNUSED(state);
	PX_UNUSED(button);
	mMouseX = x;
	mMouseY = y;
	mFOV = 60;
}

bool LearningCamera::handleKey(unsigned char key, int x, int y, float speed)
{
	PX_UNUSED(x);
	PX_UNUSED(y);

	PxVec3 viewY = mDir.cross(PxVec3(0,1,0)).getNormalized();
	//PxVec3 viewX = viewY.cross(PxVec3())
	switch(toupper(key))
	{
	case 'W':	mEye += mDir * 2.0f * speed;		break;
	case 'S':	mEye -= mDir * 2.0f * speed;		break;
	case 'A':	mEye -= viewY * 2.0f * speed;		break;
	case 'D':	mEye += viewY * 2.0f * speed;		break;
	case 'Q':   mEye += PxVec3{0.f, 1.f, 0.f} * 2.f * speed; break;
	case 'E':   mEye -= PxVec3{0.f, 1.f, 0.f} * 2.f * speed; break;
	default:							return false;
	}

	printf("after key input, eye: (%f, %f, %f)\n", mEye.x, mEye.y, mEye.z);
	return true;
}

void LearningCamera::handleAnalogMove(float x, float y)
{
	PxVec3 viewY = mDir.cross(PxVec3(0,1,0)).getNormalized();
	mEye += mDir*y;
	mEye += viewY*x;
}

void LearningCamera::handleMotion(int x, int y)
{
	int dx = mMouseX - x;
	int dy = mMouseY - y;

	PxVec3 viewY = mDir.cross(PxVec3(0,1,0)).getNormalized();

	PxQuat qx(PxPi * dx / 180.0f, PxVec3(0,1,0));
	mDir = qx.rotate(mDir);
	PxQuat qy(PxPi * dy / 180.0f, viewY);
	mDir = qy.rotate(mDir);

	mDir.normalize();

	mMouseX = x;
	mMouseY = y;

	printf("after motion, x: %d, y: %d\n", x, y);
}

PxTransform LearningCamera::getTransform() const
{
	PxVec3 viewY = mDir.cross(PxVec3(0,1,0));

	if(viewY.normalize()<1e-6f) 
		return PxTransform(mEye);

	PxMat33 m(mDir.cross(viewY), viewY, -mDir);
	return PxTransform(mEye, PxQuat(m));
}

PxVec3 LearningCamera::getEye() const
{ 
	return mEye; 
}

PxVec3 LearningCamera::getDir() const
{ 
	return mDir; 
}
