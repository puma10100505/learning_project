#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "learning/learning.h"

int main() {
    int iRet = 0;

    do {
        if ((iRet = gl_init_window()) < 0) {
            std::cout << "init window failed\n";
            break;
        }

        if ((iRet = gl_create_window(WINDOW_WIDTH, WINDOW_HEIGHT,  "Learning OPENGL - Multilights", false, 16)) < 0) {
            std::cout << "create window failed \n";
            break;
        }

        if ((iRet = gl_init_gui()) < 0) {
            std::cout << "init gui failed\n";
            break;
        }
    } while (false);

    if (iRet < 0) {
        std::cout << "init application failed\n";
        return EXIT_FAILURE;
    }

    gl_window_loop(
        [&]() -> void{
            // Render Code
        }, 
        
        [&]() -> void {
            // ImGui::Begin(u8"材质 & 光照 & Camera");  
            // ImGui::Text("Material:");               // Display some text (you can use a format strings too)
            // ImGui::SliderFloat("material.shininess", &(cube->material.shininess), 0.0f, 256.0f);
            // ImGui::DragFloat3("point-light.position", (float*)&(mainScene.FirstPointLight()->transform.position), .1f);
            // ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color
            // ImGui::End();
        });

    // mainScene.Destroy();
    
    gl_destroy_gui();
    gl_destroy_window();

    return 0;
}