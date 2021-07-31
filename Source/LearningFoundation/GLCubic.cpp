#include "GLCubic.h"

#include "shader.h"
#include "GLObject.h"
#include "GLScene.h"
#include "GLDirectionLight.h"
#include "GLSpotLight.h"
#include "GLPointLight.h"
#include "GLTexture.h"


GLCubic::GLCubic(GLScene* sc, const std::string& vs_path, const std::string& fs_path)
    :GLObject(sc, vs_path, fs_path) {
    
    // Vertices with normal vector
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // int indices[] = {
    //     0, 1, 3, 1, 2, 3
    // };

    // unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // set vertices data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // descript the usage of vertex
    // vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    scene = sc;
    std::cout << "after constructor cubic" << std::endl;
}

void GLCubic::Draw() {
    
    if (diffuse_texture != nullptr) {
        diffuse_texture->Active();
    }

    if (specular_texture != nullptr) {
        specular_texture->Active();
    }

    //std::cout << "begin to draw cubic" << std::endl;
    
    if (bindVAO == false) {
        glBindVertexArray(VAO);
        bindVAO = true;
    }

    PrepareShader();

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void GLCubic::Destroy() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// void GLCubic::PrepareShader() {
//     if (shader == nullptr) {
//         std::cout << "shader is null\n";
//         return; 
//     }

//     if (scene == nullptr) {
//         std::cout << "scene is null\n";
//         return;
//     }

//     shader->use();

//     GLCamera* camera = scene->MainCamera.get();

//     // 视点位置 
//     shader->setVec3("viewPos", camera->Position);

//     // 冯氏模型
//     shader->setVec3("phong_model.ambient", scene->phong_model.ambient);
//     shader->setVec3("phong_model.diffuse", scene->phong_model.diffuse);
//     shader->setVec3("phong_model.specular", scene->phong_model.specular);

//     // 材质
//     shader->setFloat("material.shininess", material.shininess);

//     // 点光源
//     shader->setVec3("pointLight.position", scene->FirstPointLight()->transform.position);
//     shader->setFloat("pointLight.constant", scene->FirstPointLight()->constant);
//     shader->setFloat("pointLight.linear", scene->FirstPointLight()->linear);
//     shader->setFloat("pointLight.quadratic", scene->FirstPointLight()->quadratic);

//     // 方向光源
//     shader->setVec3("directionLight.direction", scene->FirstDirectionLight()->direction);

//     // 聚光光源 
//     shader->setVec3("spotLight.position", scene->FirstSpotLight()->transform.position);
//     shader->setVec3("spotLight.direction", scene->FirstSpotLight()->direction);
//     shader->setFloat("spotLight.cutoff", scene->FirstSpotLight()->cutoff);
//     shader->setFloat("spotLight.outerCutoff", scene->FirstSpotLight()->outerCutoff);
//     shader->setFloat("spotLight.constant", scene->FirstSpotLight()->constant);
//     shader->setFloat("spotLight.linear", scene->FirstSpotLight()->linear);
//     shader->setFloat("spotLight.quadratic", scene->FirstSpotLight()->quadratic);

//     shader->setMat4("view", camera->GetViewMatrix());

//     float zoom = camera->Zoom;
//     glm::mat4 projection = glm::perspective(glm::radians(zoom), 
//         scene->WindowRatio, 0.1f, 100.0f);
//     shader->setMat4("projection", projection);

//     glm::mat4 model = glm::mat4(1.0f);
//     model = glm::translate(model, transform.position);

//     model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
//     model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
//     model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
//     model = glm::scale(model, transform.scale);
//     shader->setMat4("model", model);
// }



