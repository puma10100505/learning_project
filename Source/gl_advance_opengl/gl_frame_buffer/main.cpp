#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "CommonDefines.h"
#include "learning.h"

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
        solution_base_path + "Assets/Shaders/multilights.object.vs", 
        solution_base_path + "Assets/Shaders/multilights.object.fs");

    cube->SetDiffuseTexture(solution_base_path + "Assets/Textures/marble.jpg");
    cube->SetSpecularTexture(solution_base_path + "Assets/Textures/container2_specular.png");

    GLQuad* quad = mainScene.AddQuad(solution_base_path + "Assets/Shaders/multilights.object.vs", 
        solution_base_path + "Assets/Shaders/multilights.object.fs");
   
    quad->SetDiffuseTexture(solution_base_path + "Assets/Textures/container2.png");

    GLCubic* cube2 = mainScene.AddCube(
        solution_base_path + "Assets/Shaders/multilights.object.vs", 
        solution_base_path + "Assets/Shaders/multilights.object.fs");

    cube2->SetDiffuseTexture(solution_base_path + "Assets/Textures/marble.jpg");
    cube2->SetSpecularTexture(solution_base_path + "Assets/Textures/container2_specular.png");

    // glEnable(GL_MULTISAMPLE);
    glEnable(GL_STENCIL_TEST);  // 启用模板缓冲
    // glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 0, 0xff);
    // glStencilMask(0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    // 使用帧缓冲
    // unsigned int fbo;
    // glGenFramebuffers(1, &fbo);
    // glBindFramebuffer(GL_FRAMEBUFFER, fbo); // 绑定帧缓冲

    // unsigned int texture;
    // glGenTextures(1, &texture);
    // glBindTexture(GL_TEXTURE_2D, texture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800,600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // // 附加到帧缓冲
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

    // // 渲染缓冲对象
    // unsigned int rbo;
    // glGenRenderbuffers(1, &rbo);
    // glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {

    // }

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    unsigned int texColorBuffer;
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER::Framebuffer is not completed!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_window_loop(
        [&]() -> void{
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glEnable(GL_DEPTH_TEST);

            glClearColor(.1f, .1f, .1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            mainScene.Render();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // glDisable(GL_DEPTH_TEST);
            
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

    glDeleteFramebuffers(1, &framebuffer);
    mainScene.Destroy();
    
    gl_destroy_gui();
    gl_destroy_window();

    return EXIT_SUCCESS;
}