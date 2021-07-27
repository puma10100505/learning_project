#pragma once

#include "GLLightBase.h"

class GLDirectionLight: public GLLightBase {
public:
    GLDirectionLight(): direction(__zero) {}

    virtual ~GLDirectionLight() {}

public:
    glm::vec3 direction;
};