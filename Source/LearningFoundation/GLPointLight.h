#pragma once

#include "GLLightBase.h"

class GLPointLight: public GLLightBase {
public:
    GLPointLight() {}
    GLPointLight(const glm::vec3& position): 
        constant(0.0f), linear(0.0f), quadratic(0.0f) {
        transform.position = position;
    }

    virtual ~GLPointLight(){}

public:
    float constant = 0.0f;
    float linear = 0.0f;
    float quadratic = 0.0f;
};