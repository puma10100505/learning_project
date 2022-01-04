#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include "learning.h"
#include "CommonDefines.h"
#include "GlfwWindows.h"


/////////////////////

static MaterialSettings material_parameters;
static PhongModelSettings light_parameters;

static glm::vec3 camera_pos = glm::vec3(0.0f, 6.0f, 0.0f);
static Camera_Movement camera_direction;

static GLCamera camera(camera_pos);
static float tmp_camera_speed = 10.0f;

static std::vector<glm::vec3> cubePositions = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

static std::unique_ptr<Shader> LightShader = nullptr;
static std::unique_ptr<Shader> ObjectShader = nullptr;

static std:unique_ptr<GLCubic> Cube = nullptr;
static std::unique_ptr<GLLight> LightBox = nullptr;

glm::mat4 object_view;
glm::mat4 projection;

static void OnTick(float DeltaTime)
{
    projection = glm::perspective(glm::radians(camera.Zoom), WINDOW_RATIO, 0.1f, 100.0f);

    // camera.Position = camera_pos;
    glm::mat4 object_view = camera.GetViewMatrix();
    glm::mat4 light_view = camera.GetViewMatrix();

    // draw light
    LightShader->Activate();
    glm::mat4 light_model = glm::mat4(1.0f);
    light_model = glm::translate(light_model, light_parameters.light_pos);
    light_model = glm::scale(light_model, glm::vec3(0.1f));
    LightShader->SetMatrix4Value("model", light_model);
    LightShader->SetMatrix4Value("view", light_view);
    LightShader->SetMatrix4Value("projection", projection);

    LightBox->Draw();

    ObjectShader->Activate();
    
    ObjectShader->SetVector3Value("material.ambient", 
        material_parameters.ambient.x, material_parameters.ambient.y, material_parameters.ambient.z);
    ObjectShader->SetVector3Value("material.diffuse", 
        material_parameters.diffuse.x, material_parameters.diffuse.y, material_parameters.diffuse.z);
    ObjectShader->SetVector3Value("material.specular", 
        material_parameters.specular.x, material_parameters.specular.y, material_parameters.specular.z);
    ObjectShader->SetFloatValue("material.shininess", material_parameters.shininess);

    ObjectShader->SetVector3Value("light.ambient", light_parameters.ambient.x, light_parameters.ambient.y, 
        light_parameters.ambient.z);
    ObjectShader->SetVector3Value("light.diffuse", light_parameters.diffuse.x, light_parameters.diffuse.y, 
        light_parameters.diffuse.z);
    ObjectShader->SetVector3Value("light.specular", light_parameters.specular.x, light_parameters.specular.y,
        light_parameters.specular.z);
    // ObjectShader->SetVector3Value("light.position", light_parameters.light_pos);
    // ObjectShader->SetVector3Value("light.direction", light_parameters.direction);
    ObjectShader->SetVector3Value("light.position", camera.Position);
    ObjectShader->SetVector3Value("light.direction", camera.Front);
    ObjectShader->SetFloatValue("light.cutoff", glm::cos(glm::radians(light_parameters.cutoff)));
    ObjectShader->SetFloatValue("light.outerCutoff", glm::cos(glm::radians(light_parameters.outerCutoff)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_conainer2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_container2_spec);

    ObjectShader->SetMatrix4Value("view", object_view);
    ObjectShader->SetMatrix4Value("projection", projection);

    Cube->BeforeDraw();
    for (glm::vec3& pos: cubePositions) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(60.0f) * (float)glfwGetTime(), 
            glm::vec3(1.0f, 0.2f, .8f));
        ObjectShader->SetMatrix4Value("model", model);

        Cube->Draw();
    }
}

static void OnGUI(float DeltaTime)
{
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Material & Light & Camera");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("Material Settings:");               // Display some text (you can use a format strings too)
    ImGui::DragFloat3("material.ambient", (float*)&material_parameters.ambient, .1f, 0.0f, 255.0f);
    ImGui::DragFloat3("material.diffuse", (float*)&material_parameters.diffuse, .1f, 0.0f, 255.0f);
    ImGui::DragFloat3("material.specular", (float*)&material_parameters.specular, .1f, 0.0f, 255.0f);
    ImGui::SliderFloat("material.shininess", &material_parameters.shininess, 0.0f, 256.0f);

    ImGui::Text("Light Settings:");
    ImGui::DragFloat3("light.ambient", (float*)&light_parameters.ambient, .1f, 0.0f, 255.0f);
    ImGui::DragFloat3("light.diffuse", (float*)&light_parameters.diffuse, .1f, 0.0f, 255.0f);
    ImGui::DragFloat3("light.specular", (float*)&light_parameters.specular, .1f, 0.0f, 255.0f);
    ImGui::DragFloat3("light.position", (float*)&light_parameters.light_pos, .1f);
    ImGui::DragFloat3("light.direction", (float*)&light_parameters.direction, .1f);
    ImGui::SliderFloat("light.cutoff", &light_parameters.cutoff, 0.0f, 180.0f);
    ImGui::SliderFloat("light.outerCutoff", &light_parameters.outerCutoff, 0.0f, 180.0f);

    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

    ImGui::Text("Camera Settings:");
    if (ImGui::Button("Forward", std::move(ImVec2(100.0f, 20.0f)))) {
        camera_direction = FORWARD;
        camera.MovementSpeed = tmp_camera_speed;
    }

    if (ImGui::Button("Backward", std::move(ImVec2(100.0f, 20.0f)))) {
        camera_direction = BACKWARD;
        camera.MovementSpeed = tmp_camera_speed;
    }

    if (ImGui::Button("Left", std::move(ImVec2(100.0f, 20.0f)))) {
        camera_direction = LEFT;
        camera.MovementSpeed = tmp_camera_speed;
    }

    if (ImGui::Button("Right", std::move(ImVec2(100.0f, 20.0f)))) {
        camera_direction = RIGHT;
        camera.MovementSpeed = tmp_camera_speed;
    }

    ImGui::SliderFloat("FOV", &camera.Zoom, 0.0f, 170.0f);

    camera.ProcessKeyboard(camera_direction, GetDeltaTime());
    camera.MovementSpeed = 0.0f;

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void Init()
{
    LightShader = std::make_unique<Shader>("simple.light");
    ObjectShader = std::make_unique<Shader>("simple.object");

    ObjectShader->Activate();
    ObjectShader->SetVector3Value("object_color", glm::vec3(1.0f, 0.5f, 0.3f));
    ObjectShader->SetVector3Value("light_color", glm::vec3(1.0f, 1.0f, 1.0f));
    ObjectShader->SetVector3Value("view_pos", camera.Position);

    ObjectShader->SetVector3Value("material.ambient", 1.0f, 0.5f, 0.31f);
    ObjectShader->SetVector3Value("material.diffuse", 1.0f, 0.5f, 0.31f);
    ObjectShader->SetVector3Value("material.specular", 0.5f, 0.5f, .5f);
    ObjectShader->SetFloatValue("material.shininess", 32.0f);
    ObjectShader->SetVector3Value("light.ambient", .2f, .2f, .2f);
    ObjectShader->SetVector3Value("light.diffuse", .5f, .5f, .5f);
    ObjectShader->SetVector3Value("light.specular", 1.0f, 1.0f, 1.0f);
    ObjectShader->SetVector3Value("light.position", light_parameters.light_pos);
    ObjectShader->SetFloatValue("light.cutoff", light_parameters.cutoff);
    ObjectShader->SetFloatValue("light.outerCutoff", light_parameters.outerCutoff);

    unsigned int texture_conainer2 = load_texture((DefaultTextureDirectory + "container2.png").c_str());
    unsigned int texture_container2_spec = load_texture((DefaultTextureDirectory + "container2_specular.png").c_str());
    
    ObjectShader->Activate();
    // set the texture unit
    ObjectShader->SetIntValue("material.diffuse", 0);
    ObjectShader->SetIntValue("material.specular", 1);

    // Dot Light Settings:
    ObjectShader->SetFloatValue("light.constant", 1.0f);
    ObjectShader->SetFloatValue("light.linear", 0.09f);
    ObjectShader->SetFloatValue("light.quadratic", 0.032f);

    // calc object transform
    object_view = camera.GetViewMatrix();
    projection = glm::perspective(glm::radians(55.0f), WINDOW_RATIO, 0.1f, 100.0f);
}

int main() {

    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    Init();

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return 0;
}