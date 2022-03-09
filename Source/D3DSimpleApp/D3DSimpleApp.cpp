#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>

#include "d3dx12.h"
#include "stdafx.h"

#include "CommonDefines.h"

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

#include "d3dApp.h"
#include <DirectXColors.h>

using namespace LearningD3D;

class InitDirect3DApp: public D3DApp 
{
    public:
        InitDirect3DApp(HINSTANCE hInstance);
        ~InitDirect3DApp();

        virtual bool Initialize() override;

    private:
        virtual void OnResize() override;
        virtual void Update(const GameTimer& InGameTimer) override;
        virtual void Draw(const GameTimer& InGameTimer) override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3DHello.log").c_str());

    PLOGD << "Entry of application..." << " CmdLine: " << lpCmdLine << ", file: " << __FILE__;




    return EXIT_SUCCESS;
}