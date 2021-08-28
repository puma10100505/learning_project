#pragma once
# pragma warning (disable:4819)

#include "thirdparty_header.h"

#include <string>

//const std::string app_base_dir = "D:/Development/mygit/learning_opengl/";

const glm::vec3 __zero = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 __one = glm::vec3(1.0f, 1.0f, 1.0f);

const glm::vec3 __up = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 __down = glm::vec3(0.0f, -1.0f, 0.0f);
const glm::vec3 __left = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 __right = glm::vec3(-1.0f, 0.0f, 0.0f);
const glm::vec3 __front = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 __back = glm::vec3(0.0f, 0.0f, 1.0f);

typedef struct _stTransform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool bIsDirty = false;

    _stTransform() {
        position = __zero;
        rotation = __zero;
        scale = __one;
    }
} Transform;

typedef struct _stMaterial {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    _stMaterial() {
        ambient = __zero;
        diffuse = __zero;
        specular = __zero;
        shininess = 0.5f;
    }
} MaterialSettings;

typedef struct _stPhongModel {
    // 冯氏光照模型
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    _stPhongModel() {
        ambient =  __zero;
        diffuse = __zero;
        specular = __zero;
    }
} PhongModelSettings;

// 点光源参数
typedef struct _stPointLight {
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;

    _stPointLight() {
        position = __zero;
        constant = linear = quadratic = 0.0f;
    }
} PointLightSettings;

typedef struct _stDirectionLight {
    glm::vec3 direction;

    _stDirectionLight() {
        direction = __zero;
    }
} DirectionLightSettings;

typedef struct _stSpotLight {
    glm::vec3 direction;
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;

    float cutoff;
    float outerCutoff;

    _stSpotLight() {
        direction = __zero;
        position = __zero;

        constant = linear = quadratic = 0.0f;

        cutoff = outerCutoff = 0.0f;
    }
} SpotLightSettings;

typedef struct _stVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
} Vertex;

typedef struct _stTexture {
    unsigned int id;
    std::string type;
    std::string path;
} Texture;