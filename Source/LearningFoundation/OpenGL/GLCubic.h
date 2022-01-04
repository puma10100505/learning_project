#pragma once
#include <cstdio>
#include <string>
#include <functional>
#include "defines.h"
#include "thirdparty_header.h"

#include "GLObject.h"

class GLScene;
class GLObject;
class GLTexture;
class Shader;
class GLDirectionLight;
class GLSpotLight;
class GLPointLight;

class GLCubic final: public GLObject
{
public:
    GLCubic(){}
    GLCubic(GLScene* sc, const std::string& vs_path, 
        const std::string& fs_path);

    virtual ~GLCubic(){}
    
    virtual void Draw() override;
    virtual void Destroy() override;
};
