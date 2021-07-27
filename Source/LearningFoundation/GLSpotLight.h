#pragma once

#include "GLLightBase.h"

class GLSpotLight: public GLLightBase {
public:
    GLSpotLight() {}
    GLSpotLight(const glm::vec3& position) {
        transform.position = position;
    }
    GLSpotLight(const glm::vec3& position, const glm::vec3& dir, 
        float co, float oco): direction(dir), cutoff(co), outerCutoff(oco) {
        transform.position = position;
    }

    virtual ~GLSpotLight(){}

public:
    glm::vec3 direction = __zero;

    float constant = 0.0f;
    float linear = 0.0f;
    float quadratic = 0.0f;

    float cutoff = 0.0f;
    float outerCutoff = 0.0f;
};