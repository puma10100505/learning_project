#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

//#include "thirdparty_header.h"
#include "CommonDefines.h"
#include "learning/learning.h"

int main() {
    int iRet = 0;

    do {
        if ((iRet = gl_init_window()) < 0) {
            std::cout << "init window failed\n";
            break;
        }

        if ((iRet = gl_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, 
            "Learning OPENGL - Depth Testing", false, 16)) < 0) {
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

    GLScene mainScene(__back * 6.0f);
    GLCubic* cube = mainScene.AddCube(
        solution_base_path + "assets/shaders/multilights.object.vs", 
        solution_base_path + "assets/shaders/multilights.object.fs");

    cube->SetDiffuseTexture(solution_base_path + "/resources/textures/marble.jpg");
    cube->SetSpecularTexture(solution_base_path + "/resources/textures/container2_specular.png");

    GLQuad* quad = mainScene.AddQuad(solution_base_path + "assets/shaders/multilights.object.vs", 
        solution_base_path + "assets/shaders/multilights.object.fs");
   
    quad->SetDiffuseTexture(solution_base_path + "/resources/textures/container2.png");

    GLCubic* cube2 = mainScene.AddCube(
        solution_base_path + "assets/shaders/multilights.object.vs", 
        solution_base_path + "assets/shaders/multilights.object.fs");

    cube2->SetDiffuseTexture(solution_base_path + "/resources/textures/marble.jpg");
    cube2->SetSpecularTexture(solution_base_path + "/resources/textures/container2_specular.png");

    // glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);

    glEnable(GL_MULTISAMPLE);

    gl_window_loop(
        [&]() -> void{
            mainScene.Render();
        }, 
        
        [&]() -> void {
            ImGui::Begin(u8"材质 & 光照 & Camera");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("Material:");               // Display some text (you can use a format strings too)
            ImGui::SliderFloat("cube.material.shininess", &(cube->material.shininess), 0.0f, 256.0f);

            ImGui::Text("Material:");               // Display some text (you can use a format strings too)
            ImGui::SliderFloat("quad.material.shininess", &(quad->material.shininess), 0.0f, 256.0f);

            ImGui::Text("Light - Phong:");
            ImGui::DragFloat3("phong.ambient", (float*)&(mainScene.phong_model.ambient), .01f, 0.0f, 1.0f);
            ImGui::DragFloat3("phong.diffuse", (float*)&(mainScene.phong_model.diffuse), .01f, 0.0f, 1.0f);
            ImGui::DragFloat3("phong.specular", (float*)&(mainScene.phong_model.specular), .01f, 0.0f, 1.0f);
            
            ImGui::Text("Light - Direction:");
            ImGui::DragFloat3("direction-light.direction", (float*)&(mainScene.FirstDirectionLight()->direction), .1f);

            ImGui::Text("Light - Point:");
            ImGui::DragFloat3("point-light.position", (float*)&(mainScene.FirstPointLight()->transform.position), .1f);
            ImGui::SliderFloat("point-light.constant", (float*)&(mainScene.FirstPointLight()->constant), 0.001f, 1.0f);
            ImGui::SliderFloat("point-light.linear", (float*)&(mainScene.FirstPointLight()->linear), 0.001f, 1.0f);
            ImGui::SliderFloat("point-light.quadratic", (float*)&(mainScene.FirstPointLight()->quadratic), 0.001f, 1.0f);

            ImGui::Text("Light - Spot:");
            ImGui::DragFloat3("spot-light.direction", (float*)&(mainScene.FirstSpotLight()->direction), .1f);
            ImGui::DragFloat3("spot-light.position", (float*)&(mainScene.FirstSpotLight()->transform.position), .1f);
            ImGui::SliderFloat("spot-light.cutoff", &(mainScene.FirstSpotLight()->cutoff), 0.0f, 179.0f);
            ImGui::SliderFloat("spot-light.outerCutoff", &(mainScene.FirstSpotLight()->outerCutoff), 0.0f, 179.0f);
            ImGui::SliderFloat("spot-light.constant", (float*)&(mainScene.FirstSpotLight()->constant), 0.001f, 1.0f);
            ImGui::SliderFloat("spot-light.linear", (float*)&(mainScene.FirstSpotLight()->linear), 0.001f, 1.0f);
            ImGui::SliderFloat("spot-light.quadratic", (float*)&(mainScene.FirstSpotLight()->quadratic), 0.001f, 1.0f);

            ImGui::Text("Quad Transform:");
            ImGui::DragFloat3("transform.position", (float*)&(quad->transform.position), .1f);
            ImGui::DragFloat3("transform.rotate", (float*)&(quad->transform.rotation), .1f);
            ImGui::DragFloat3("transform.scale", (float*)&(quad->transform.scale), .1f);

            ImGui::Text("Cube Transform:");
            ImGui::DragFloat3("cube.transform.position", (float*)&(cube->transform.position), .1f);
            ImGui::DragFloat3("cube.transform.rotate", (float*)&(cube->transform.rotation), .1f);
            ImGui::DragFloat3("cube.transform.scale", (float*)&(cube->transform.scale), .1f);

            ImGui::Text("Cube Transform:");
            ImGui::DragFloat3("cube2.transform.position", (float*)&(cube2->transform.position), .1f);
            ImGui::DragFloat3("cube2.transform.rotate", (float*)&(cube2->transform.rotation), .1f);
            ImGui::DragFloat3("cube2.transform.scale", (float*)&(cube2->transform.scale), .1f);


            ImGui::Text("Camera:");
            ImGui::SliderFloat("FOV", &(mainScene.MainCamera.get()->Zoom), 0.0f, 170.0f);
            ImGui::DragFloat3("camera.position", (float*)&(mainScene.MainCamera.get()->Position), .1f);

            ImGui::Text("Background Color:");
            ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        });

    mainScene.Destroy();
    
    gl_destroy_gui();
    gl_destroy_window();

    return EXIT_SUCCESS;
}