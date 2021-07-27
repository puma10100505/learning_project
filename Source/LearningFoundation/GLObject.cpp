#include "GLObject.h"

#include "GLScene.h"
#include "GLTexture.h"
#include "shader.h"
#include "GLPointLight.h"
#include "GLDirectionLight.h"
#include "GLSpotLight.h"

GLObject::GLObject(GLScene* scene, 
    const std::string& vs_path, const std::string& fs_path) {

    shader.reset(new Shader(vs_path.c_str(), fs_path.c_str()));
}

void GLObject::SetDiffuseTexture(const std::string& texture_path) {
    diffuse_texture.reset(std::move(new GLTexture(texture_path, GL_TEXTURE0)));
    shader->use();
    shader->setInt("material.diffuse", 0);
}

void GLObject::SetSpecularTexture(const std::string& texture_path) {
    specular_texture.reset(std::move(new GLTexture(texture_path, GL_TEXTURE1)));
    shader->use();
    shader->setInt("material.specular", 1);
}

void GLObject::PrepareShader() {
    if (shader == nullptr) {
        std::cout << "shader is null\n";
        return; 
    }

    if (scene == nullptr) {
        std::cout << "scene is null\n";
        return;
    }

    shader->use();

    GLCamera* camera = scene->MainCamera.get();

    // 视点位置 
    shader->setVec3("viewPos", camera->Position);

    // 冯氏模型
    shader->setVec3("phong_model.ambient", scene->phong_model.ambient);
    shader->setVec3("phong_model.diffuse", scene->phong_model.diffuse);
    shader->setVec3("phong_model.specular", scene->phong_model.specular);

    // 材质
    shader->setFloat("material.shininess", material.shininess);

    // 点光源
    shader->setVec3("pointLight.position", scene->FirstPointLight()->transform.position);
    shader->setFloat("pointLight.constant", scene->FirstPointLight()->constant);
    shader->setFloat("pointLight.linear", scene->FirstPointLight()->linear);
    shader->setFloat("pointLight.quadratic", scene->FirstPointLight()->quadratic);

    // 方向光源
    shader->setVec3("directionLight.direction", scene->FirstDirectionLight()->direction);

    // 聚光光源 
    shader->setVec3("spotLight.position", scene->FirstSpotLight()->transform.position);
    shader->setVec3("spotLight.direction", scene->FirstSpotLight()->direction);
    shader->setFloat("spotLight.cutoff", scene->FirstSpotLight()->cutoff);
    shader->setFloat("spotLight.outerCutoff", scene->FirstSpotLight()->outerCutoff);
    shader->setFloat("spotLight.constant", scene->FirstSpotLight()->constant);
    shader->setFloat("spotLight.linear", scene->FirstSpotLight()->linear);
    shader->setFloat("spotLight.quadratic", scene->FirstSpotLight()->quadratic);

    shader->setMat4("view", camera->GetViewMatrix());

    float zoom = camera->Zoom;
    glm::mat4 projection = glm::perspective(glm::radians(zoom), scene->WindowRatio, 0.1f, 100.0f);
    shader->setMat4("projection", projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform.position);

    model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, transform.scale);
    shader->setMat4("model", model);
}

void GLObject::BeforeDraw() {
    glBindVertexArray(VAO);
    bindVAO = true;
} 