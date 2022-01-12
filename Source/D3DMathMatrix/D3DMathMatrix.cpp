#include <Windows.h>
#include <string>
#include <iostream>

#include "CommonDefines.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
    XMFLOAT4 dst;
    XMStoreFloat4(&dst, v);

    os << "(" << dst.x << ", " << dst.y << ", " << dst.z << ", " << dst.w << ")";

    return os;
}

ostream& XM_CALLCONV operator << (ostream& os, FXMMATRIX m)
{
    for (int i = 0; i < 4; ++i)
    {
        os << XMVectorGetX(m.r[i]) << "\t";
        os << XMVectorGetY(m.r[i]) << "\t";
        os << XMVectorGetZ(m.r[i]) << "\t";
        os << XMVectorGetW(m.r[i]);
        os << std::endl;
    }

    return os;
}

int main(int argc, char** argv)
{
    plog::init(plog::debug, (DefaultLogDirectory + "D3DMathMatrix.log").c_str());

    if (XMVerifyCPUSupport() == false)
    {
        PLOGD << "CPU does not support SSE2";
        return EXIT_FAILURE;
    }

    std::cout << "Hello " << std::endl;

    XMMATRIX OriginalMat(
        1.f, 0.f, 0.f, 0.f, 
        0.f, 2.f, 0.f, 0.f, 
        0.f, 0.f, 4.f, 0.f, 
        1.f, 2.f, 3.f, 1.f);

    XMMATRIX Identity = XMMatrixIdentity();

    std::cout << OriginalMat << std::endl;
    std::cout << Identity << std::endl;

    XMMATRIX ScaleMat = XMMatrixScaling(3.f, 3.f, 3.f);

    std::cout << OriginalMat * ScaleMat << std::endl;

    XMMATRIX RotXMat = XMMatrixRotationX(90.f);

    std::cout << RotXMat << std::endl;

    XMFLOAT4 OriginalVec = {1.f, 0.f, 0.f, 0.f};

    XMVECTOR Vec = XMLoadFloat4(&OriginalVec);

    //std::cout << Vec * RotXMat << std::endl;

    return EXIT_SUCCESS;
}