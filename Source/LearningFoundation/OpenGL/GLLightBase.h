#pragma once

#include "defines.h"

class GLLightBase {
public:
    GLLightBase() {}
    GLLightBase(const glm::vec3& light_pos) {
        transform.position = light_pos;
    }

    virtual ~GLLightBase() {}

public:
    Transform transform;
};