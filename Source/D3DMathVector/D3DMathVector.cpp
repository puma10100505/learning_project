#include <Windows.h>
#include <string>
#include <memory>
#include <iostream>
#include <vector>

#include "CommonDefines.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

float XM_CALLCONV DotProduct(FXMVECTOR Param1, FXMVECTOR Param2)
{
    return sqrt(XMVectorGetX(Param1) * XMVectorGetX(Param2) + 
            XMVectorGetY(Param1) * XMVectorGetY(Param2) + 
            XMVectorGetZ(Param1) * XMVectorGetZ(Param2));
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3DMath.log").c_str());

    if (XMVerifyCPUSupport() == false)
    {
        PLOGD << "CPU does not support SSE2";
    }

    PLOG_DEBUG << L"Entry of DXMath";

    XMFLOAT3 Pos1 {200, 32, 455};
    XMFLOAT3 Pos2 {200, 28, 455};

    XMVECTOR Vec = XMLoadFloat3(&Pos1) + XMLoadFloat3(&Pos2);

    PLOGD << L"Added, x: " << XMVectorGetX(Vec) << L", y: " << XMVectorGetY(Vec) << L", z: " << XMVectorGetZ(Vec);

    PLOGD << L"DotProduct: " << DotProduct(XMLoadFloat3(&Pos1), XMLoadFloat3(&Pos2));

    XMVECTOR RetVec = XMVector3Orthogonal(Vec);

    XMVECTOR BetVec = XMVector3AngleBetweenVectors(RetVec, Vec);

    PLOGD << XMVectorGetX(BetVec) << " " << XMVectorGetY(BetVec) << " " << XMVectorGetZ(BetVec);

    XMVECTOR DotRet = XMVector3Dot(RetVec, Vec);

    PLOGD << XMVectorGetX(DotRet);

    // XMFLOAT3 U{1, 1, 0};
    // XMFLOAT3 V{-2, 2, 0};
    // XMVECTOR Dot1 = XMVector3Dot(XMLoadFloat3(&U), XMLoadFloat3(&V));
    
    // PLOGD << XMVectorGetX(Dot1);

    return EXIT_SUCCESS;
}