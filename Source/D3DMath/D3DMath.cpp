#include <Windows.h>
#include <string>
#include <memory>
#include <vector>

#define GLOG_NO_ABBREVIATED_SEVERITIES 1
#include "logging.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    google::InitGoogleLogging(__FILE__);

    LOG(INFO) << "Entry of app, glog is started...";

    XMFLOAT3 Pos1 {200, 32, 455};
    XMFLOAT3 Pos2 {200, 28, 455};

    XMVECTOR Vec = XMLoadFloat3(&Pos1) + XMLoadFloat3(&Pos2);

    return EXIT_SUCCESS;
}