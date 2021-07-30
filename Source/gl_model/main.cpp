#ifdef __linux__
    #include <unistd.h>
#endif 
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

        if ((iRet = gl_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Learning OPENGL - Multilights", false, 16)) < 0) {
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


    GLScene mainScene(glm::vec3(0.0f, -1.70f, 6.0f));
    Shader model_shader(
        (solution_base_path + "Assets/Shaders/multilights.object.vs").c_str(), 
        (solution_base_path + "Assets/Shaders/multilights.object.fs").c_str());

    GLModel our_model(solution_base_path + "Assets/Meshes/cyborg/cyborg.obj");

    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 trans_mat = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale_mat = glm::vec3(.2f, .2f, .2f);
    glm::vec3 rotate_mat = glm::vec3(180.0f, 180.0f, 0.0f);

    float scale = .2f;

    float shininess = 0.01f;
    int draw_mode = GL_FILL;

    gl_window_loop(
        [&]() -> void{
            model_shader.use();

            glm::mat4 projection = glm::perspective(glm::radians(
                mainScene.MainCamera->Zoom), mainScene.WindowRatio, .1f, 100.0f);
            glm::mat4 view = mainScene.MainCamera->GetViewMatrix();
            model_shader.setMat4("projection", projection);
            model_shader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, trans_mat);
            model = glm::scale(model, glm::vec3(scale, scale, scale));
            model = glm::rotate(model, glm::radians(rotate_mat.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotate_mat.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotate_mat.z), glm::vec3(1.0f, 0.0f, 1.0f));

            model_shader.setMat4("model", model);

            // 冯氏模型
            model_shader.setVec3("phong_model.ambient", mainScene.phong_model.ambient);
            model_shader.setVec3("phong_model.diffuse", mainScene.phong_model.diffuse);
            model_shader.setVec3("phong_model.specular", mainScene.phong_model.specular);

            // 材质
            model_shader.setFloat("material.shininess", shininess);

            // 点光源
            model_shader.setVec3("pointLight.position", mainScene.FirstPointLight()->transform.position);
            model_shader.setFloat("pointLight.constant", mainScene.FirstPointLight()->constant);
            model_shader.setFloat("pointLight.linear", mainScene.FirstPointLight()->linear);
            model_shader.setFloat("pointLight.quadratic", mainScene.FirstPointLight()->quadratic);

            // 方向光源
            model_shader.setVec3("directionLight.direction", mainScene.FirstDirectionLight()->direction);

            // 聚光光源 
            model_shader.setVec3("spotLight.position", mainScene.FirstSpotLight()->transform.position);
            model_shader.setVec3("spotLight.direction", mainScene.FirstSpotLight()->direction);
            model_shader.setFloat("spotLight.cutoff", mainScene.FirstSpotLight()->cutoff);
            model_shader.setFloat("spotLight.outerCutoff", mainScene.FirstSpotLight()->outerCutoff);
            model_shader.setFloat("spotLight.constant", mainScene.FirstSpotLight()->constant);
            model_shader.setFloat("spotLight.linear", mainScene.FirstSpotLight()->linear);
            model_shader.setFloat("spotLight.quadratic", mainScene.FirstSpotLight()->quadratic);
            our_model.Draw(model_shader, draw_mode);

            mainScene.Render();
        }, 
        
        [&]() -> void {
            ImGui::Begin(u8"Model"); 
            ImGui::Text("Draw Mode: ");
            if (ImGui::Button("Fill Mode")) {
                draw_mode = GL_FILL;    
            } else if (ImGui::Button("Frame Mode")) {
                draw_mode = GL_LINE;
            }

            ImGui::Text("Material:");               // Display some text (you can use a format strings too)
            ImGui::SliderFloat("material.shininess", &shininess, 0.0f, 256.0f);

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

            ImGui::Text("Camera:");
            ImGui::SliderFloat("FOV", &(mainScene.MainCamera.get()->Zoom), 0.0f, 170.0f);
            ImGui::DragFloat3("camera.position", (float*)&(mainScene.MainCamera.get()->Position), .1f);

            ImGui::Text("Transform:");
            ImGui::DragFloat3("transform.position", (float*)&trans_mat, .1f);
            ImGui::DragFloat3("transform.rotate", (float*)&rotate_mat, .1f);
            // ImGui::DragFloat3("transform.scale", (float*)&scale_mat, .1f);
            ImGui::SliderFloat("Locked Scale: ", &scale, 0.1f, 1.0f);

            ImGui::Text("Background Color:");
            ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

            ImGui::LabelText("Model Position: ", " (%f, %f, %f)", trans_mat.x, trans_mat.y, trans_mat.z);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        });

    mainScene.Destroy();
    
    gl_destroy_gui();
    gl_destroy_window();

    return 0;
}