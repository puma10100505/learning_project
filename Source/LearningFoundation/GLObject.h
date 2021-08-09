#pragma once
# pragma warning (disable:4819)

#include <memory>
#include <cstdio>
#include <functional>
#include <vector>

#include "defines.h"
#include "thirdparty_header.h"
#include "LearningShader.h"

class GLScene;
class Shader;
class GLTexture;
class GLQuad;
// class GLPointLight;
// class GLSpotLight;
// class GLDirectionLight;

class GLObject{
public:
    GLObject(){}
    GLObject(GLScene* scene, const std::string& vs_path, const std::string& fs_path);

    virtual ~GLObject() {}

public:
    virtual void Draw() = 0;
    virtual void Destroy() = 0;
    virtual void PrepareShader() ;

    virtual void BeforeDraw();

    // 色彩模式一定要根据纹理文件格式指定对应的参数，否则纹理贴图会出错
    // 例如为png文件指定GL_RGB格式就会出错，因为png文件有Alpha通道
    void SetDiffuseTexture(const std::string& texture_path);
    void SetSpecularTexture(const std::string& texture_path);


public:
    // 着色器
    std::unique_ptr<Shader> shader;
    
    // 变换参数
    Transform transform;
    
    // 材质
    MaterialSettings material;

    // 纹理
    std::unique_ptr<GLTexture> diffuse_texture;
    std::unique_ptr<GLTexture> specular_texture;

    // 主场景引用
    GLScene* scene = nullptr;

protected:
    uint32_t VBO;
    uint32_t VAO;
    uint32_t EBO;
    bool bindVAO = false;
};
