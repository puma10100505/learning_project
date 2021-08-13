# pragma warning (disable:4819)

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

extern glm::vec4 BackgroundColor;

static std::unique_ptr<Shader> ModelShader = nullptr;
static std::unique_ptr<GLScene> MainScene = nullptr;
static std::unique_ptr<GLModel> MainModel = nullptr;

static glm::mat4 model = glm::mat4(1.0f);
static glm::vec3 trans_mat = glm::vec3(0.0f, 0.0f, 0.0f);
static glm::vec3 scale_mat = glm::vec3(.2f, .2f, .2f);
static glm::vec3 rotate_mat = glm::vec3(180.0f, 180.0f, 0.0f);
static float scale = .2f;
static float shininess = 0.01f;
static int draw_mode = GL_FILL;

void OnTick(float DeltaTime)
{
    ModelShader->Activate();

    glm::mat4 projection = glm::perspective(glm::radians(
        MainScene->MainCamera->Zoom), MainScene->WindowRatio, .1f, 100.0f);
    glm::mat4 view = MainScene->MainCamera->GetViewMatrix();
    
    ModelShader->setMat4("projection", projection);
    ModelShader->setMat4("view", view);

    model = glm::mat4(1.0f);
    model = glm::translate(model, trans_mat);
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    model = glm::rotate(model, glm::radians(rotate_mat.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotate_mat.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotate_mat.z), glm::vec3(1.0f, 0.0f, 1.0f));

    ModelShader->setMat4("model", model);

    // 冯氏模型
    ModelShader->setVec3("phong_model.ambient", MainScene->phong_model.ambient);
    ModelShader->setVec3("phong_model.diffuse", MainScene->phong_model.diffuse);
    ModelShader->setVec3("phong_model.specular", MainScene->phong_model.specular);

    // 材质
    ModelShader->setFloat("material.shininess", shininess);

    // 点光源
    ModelShader->setVec3("pointLight.position", MainScene->FirstPointLight()->transform.position);
    ModelShader->setFloat("pointLight.constant", MainScene->FirstPointLight()->constant);
    ModelShader->setFloat("pointLight.linear", MainScene->FirstPointLight()->linear);
    ModelShader->setFloat("pointLight.quadratic", MainScene->FirstPointLight()->quadratic);

    // 方向光源
    ModelShader->setVec3("directionLight.direction", MainScene->FirstDirectionLight()->direction);

    // 聚光光源 
    ModelShader->setVec3("spotLight.position", MainScene->FirstSpotLight()->transform.position);
    ModelShader->setVec3("spotLight.direction", MainScene->FirstSpotLight()->direction);
    ModelShader->setFloat("spotLight.cutoff", MainScene->FirstSpotLight()->cutoff);
    ModelShader->setFloat("spotLight.outerCutoff", MainScene->FirstSpotLight()->outerCutoff);
    ModelShader->setFloat("spotLight.constant", MainScene->FirstSpotLight()->constant);
    ModelShader->setFloat("spotLight.linear", MainScene->FirstSpotLight()->linear);
    ModelShader->setFloat("spotLight.quadratic", MainScene->FirstSpotLight()->quadratic);
    MainModel->Draw(*ModelShader, draw_mode);

    MainScene->Render();
}

void OnGUI(float DeltaTime)
{
    ImGui::Begin("Model"); 
    ImGui::Text("Draw Mode: ");
    if (ImGui::Button("Fill Mode")) {
        draw_mode = GL_FILL;    
    } else if (ImGui::Button("Frame Mode")) {
        draw_mode = GL_LINE;
    }

    ImGui::Text("Material:");               // Display some text (you can use a format strings too)
    ImGui::SliderFloat("material.shininess", &shininess, 0.0f, 256.0f);

    ImGui::Text("Light - Phong:");
    ImGui::DragFloat3("phong.ambient", (float*)&(MainScene->phong_model.ambient), .01f, 0.0f, 1.0f);
    ImGui::DragFloat3("phong.diffuse", (float*)&(MainScene->phong_model.diffuse), .01f, 0.0f, 1.0f);
    ImGui::DragFloat3("phong.specular", (float*)&(MainScene->phong_model.specular), .01f, 0.0f, 1.0f);

    ImGui::Text("Light - Direction:");
    ImGui::DragFloat3("direction-light.direction", (float*)&(MainScene->FirstDirectionLight()->direction), .1f);

    ImGui::Text("Light - Point:");
    ImGui::DragFloat3("point-light.position", (float*)&(MainScene->FirstPointLight()->transform.position), .1f);
    ImGui::SliderFloat("point-light.constant", (float*)&(MainScene->FirstPointLight()->constant), 0.001f, 1.0f);
    ImGui::SliderFloat("point-light.linear", (float*)&(MainScene->FirstPointLight()->linear), 0.001f, 1.0f);
    ImGui::SliderFloat("point-light.quadratic", (float*)&(MainScene->FirstPointLight()->quadratic), 0.001f, 1.0f);

    ImGui::Text("Light - Spot:");
    ImGui::DragFloat3("spot-light.direction", (float*)&(MainScene->FirstSpotLight()->direction), .1f);
    ImGui::DragFloat3("spot-light.position", (float*)&(MainScene->FirstSpotLight()->transform.position), .1f);
    ImGui::SliderFloat("spot-light.cutoff", &(MainScene->FirstSpotLight()->cutoff), 0.0f, 179.0f);
    ImGui::SliderFloat("spot-light.outerCutoff", &(MainScene->FirstSpotLight()->outerCutoff), 0.0f, 179.0f);
    ImGui::SliderFloat("spot-light.constant", (float*)&(MainScene->FirstSpotLight()->constant), 0.001f, 1.0f);
    ImGui::SliderFloat("spot-light.linear", (float*)&(MainScene->FirstSpotLight()->linear), 0.001f, 1.0f);
    ImGui::SliderFloat("spot-light.quadratic", (float*)&(MainScene->FirstSpotLight()->quadratic), 0.001f, 1.0f);

    ImGui::Text("Camera:");
    ImGui::SliderFloat("FOV", &(MainScene->MainCamera.get()->Zoom), 0.0f, 170.0f);
    ImGui::DragFloat3("camera.position", (float*)&(MainScene->MainCamera.get()->Position), .1f);

    ImGui::Text("Transform:");
    ImGui::DragFloat3("transform.position", (float*)&trans_mat, .1f);
    ImGui::DragFloat3("transform.rotate", (float*)&rotate_mat, .1f);
    ImGui::SliderFloat("Locked Scale: ", &scale, 0.1f, 1.0f);

    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

    ImGui::LabelText("Model Position: ", " (%f, %f, %f)", trans_mat.x, trans_mat.y, trans_mat.z);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void InitScene()
{
    MainScene = std::make_unique<GLScene>(glm::vec3(0.0f, -1.70f, 6.0f));

    ModelShader = std::make_unique<Shader>("multilights.object");

    MainModel = std::make_unique<GLModel>(DefaultMeshDirectory + "cyborg/cyborg.obj");
}

int main() {
    // 创建窗口
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    InitScene();

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return 0;
}