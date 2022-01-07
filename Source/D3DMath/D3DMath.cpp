#include <Windows.h>
#include <string>
#include <memory>
#include <vector>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    plog::init(plog::debug, __FILE__ ".log");

    PLOG_DEBUG << "Entry of DXMath";

    XMFLOAT3 Pos1 {200, 32, 455};
    XMFLOAT3 Pos2 {200, 28, 455};

    XMVECTOR Vec = XMLoadFloat3(&Pos1) + XMLoadFloat3(&Pos2);

    return EXIT_SUCCESS;
}