
/*
* 创建窗口的基础代码模板
*
*/
#ifdef __linux__
#include <unistd.h>
#endif

#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include "CommonDefines.h"
#include "DirectX12Window.h"

void WinTick(float Duration)
{

}

void WinGUI(float Duration)
{

}

int main(int argc, char** argv)
{
    Dx12Window_CreateInstance(900, 600, 0, 0, WinTick, WinGUI);

    return EXIT_SUCCESS;
}