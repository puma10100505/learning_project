#pragma once
#include <cstdio>
#include <functional>
#include <string>

#include "defines.h"
#include "GLObject.h"
#include "thirdparty_header.h"
#include "GLScene.h"

class GLScene;
class GLObject;

class GLQuad final: public GLObject {
public:
    GLQuad();
    GLQuad(GLScene* sc, const std::string& vs_path, const std::string& fs_path);

    virtual ~GLQuad(){}
    
    virtual void Draw() override;
    virtual void Destroy() override;

    
};
