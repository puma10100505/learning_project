#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "CommonDefines.h"
#include "learning.h"


/////////////////////

static MaterialSettings material_parameters;
static PhongModelSettings light_parameters;

static glm::vec3 camera_pos = glm::vec3(0.0f, 6.0f, 0.0f);
static Camera_Movement camera_direction;

static GLCamera camera(camera_pos);
static float tmp_camera_speed = 10.0f;

static void gl_on_gui() {
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

int main() {

    int iRet = 0;

    // Initialize the window and GUI
    do {
        if ((iRet = gl_init_window()) < 0) {
            std::cout << "init window failed\n";
            break;
        }
 
        if ((iRet = gl_create_window(1280, 720, "Learning Light Window", false, 16)) < 0) {
            std::cout << "create window failed\n";
            break;
        }

        if ((iRet = gl_init_gui()) < 0) {
            std::cout << "init gui failed\n";
            break;
        }
    } while(false);

    // -----------------------

    GLCubic cube;
    GLLight light_box;

    //glm::vec3 light_pos(1.0f, 2.5f, 2.0f);

    Shader light_shader((solution_base_path + "assets/shaders/simple.light.vs").c_str(), 
        (solution_base_path + "assets/shaders/simple.light.fs").c_str());
    Shader object_shader((solution_base_path + "assets/shaders/simple.object.vs").c_str(), 
        (solution_base_path + "assets/shaders/simple.object.fs").c_str());

    object_shader.use();
    object_shader.setVec3("object_color", glm::vec3(1.0f, 0.5f, 0.3f));
    object_shader.setVec3("light_color", glm::vec3(1.0f, 1.0f, 1.0f));
    object_shader.setVec3("view_pos", camera.Position);

    object_shader.setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
    object_shader.setVec3("material.diffuse", 1.0f, 0.5f, 0.31f);
    object_shader.setVec3("material.specular", 0.5f, 0.5f, .5f);
    object_shader.setFloat("material.shininess", 32.0f);
    object_shader.setVec3("light.ambient", .2f, .2f, .2f);
    object_shader.setVec3("light.diffuse", .5f, .5f, .5f);
    object_shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
    object_shader.setVec3("light.position", light_parameters.light_pos);
    object_shader.setFloat("light.cutoff", light_parameters.cutoff);
    object_shader.setFloat("light.outerCutoff", light_parameters.outerCutoff);

    unsigned int texture_conainer2 = load_texture((solution_base_path + "resources/container2.png").c_str());
    unsigned int texture_container2_spec = load_texture((solution_base_path + "resources/container2_specular.png").c_str());
    object_shader.use();
    // set the texture unit
    object_shader.setInt("material.diffuse", 0);
    object_shader.setInt("material.specular", 1);

    // Dot Light Settings:
    object_shader.setFloat("light.constant", 1.0f);
    object_shader.setFloat("light.linear", 0.09f);
    object_shader.setFloat("light.quadratic", 0.032f);

    // calc object transform
    glm::mat4 object_view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(55.0f), WINDOW_RATIO, 0.1f, 100.0f);

    //light_parameters.light_pos = light_pos;
    // camera.MovementSpeed = 1.0f;

    gl_window_loop([&]()->void{
        
        projection = glm::perspective(glm::radians(camera.Zoom), WINDOW_RATIO, 0.1f, 100.0f);

        // camera.Position = camera_pos;
        glm::mat4 object_view = camera.GetViewMatrix();
        glm::mat4 light_view = camera.GetViewMatrix();

        // draw light
        light_shader.use();
        glm::mat4 light_model = glm::mat4(1.0f);
        light_model = glm::translate(light_model, light_parameters.light_pos);
        light_model = glm::scale(light_model, glm::vec3(0.1f));
        light_shader.setMat4("model", light_model);
        light_shader.setMat4("view", light_view);
        light_shader.setMat4("projection", projection);

        light_box.Draw();

        // draw object

        object_shader.use();
        // glm::mat4 object_model = glm::mat4(1.0f);
        // object_model = glm::translate(object_model, glm::vec3(0.0f, 0.0f, 0.0f));
        // object_model = glm::rotate(object_model, glm::radians(60.0f) * (float)glfwGetTime(), 
        //     glm::vec3(1.0f, 0.5f, 1.0f));
        // object_shader.setMat4("model", object_model);
       

        object_shader.setVec3("material.ambient", 
            material_parameters.ambient.x, material_parameters.ambient.y, material_parameters.ambient.z);
        object_shader.setVec3("material.diffuse", 
            material_parameters.diffuse.x, material_parameters.diffuse.y, material_parameters.diffuse.z);
        object_shader.setVec3("material.specular", 
            material_parameters.specular.x, material_parameters.specular.y, material_parameters.specular.z);
        object_shader.setFloat("material.shininess", material_parameters.shininess);

        object_shader.setVec3("light.ambient", light_parameters.ambient.x, light_parameters.ambient.y, 
            light_parameters.ambient.z);
        object_shader.setVec3("light.diffuse", light_parameters.diffuse.x, light_parameters.diffuse.y, 
            light_parameters.diffuse.z);
        object_shader.setVec3("light.specular", light_parameters.specular.x, light_parameters.specular.y,
            light_parameters.specular.z);
        // object_shader.setVec3("light.position", light_parameters.light_pos);
        // object_shader.setVec3("light.direction", light_parameters.direction);
        object_shader.setVec3("light.position", camera.Position);
        object_shader.setVec3("light.direction", camera.Front);
        object_shader.setFloat("light.cutoff", glm::cos(glm::radians(light_parameters.cutoff)));
        object_shader.setFloat("light.outerCutoff", glm::cos(glm::radians(light_parameters.outerCutoff)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_conainer2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_container2_spec);

        object_shader.setMat4("view", object_view);
        object_shader.setMat4("projection", projection);

        cube.BeforeDraw();
        for (glm::vec3& pos: cubePositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(60.0f) * (float)glfwGetTime(), 
                glm::vec3(1.0f, 0.2f, .8f));
            object_shader.setMat4("model", model);

            cube.Draw();
        }

        // glm::mat4 model = glm::mat4(1.0f);;
        // model = glm::translate(model, cubePositions[0]);
        // model = glm::rotate(model, glm::radians(60.0f) * (float)glfwGetTime(), 
        //     glm::vec3(1.0f, 0.2f, .8f));
        // object_shader.setMat4("model", model);

        // cube.Draw();
        
        // cube.Draw();    
        // camera.MovementSpeed = 0.0f;    
    }, gl_on_gui);


    light_box.Destroy();
    cube.Destroy();

    // Cleanup
    gl_destroy_gui();

    gl_destroy_window();

    return 0;
}