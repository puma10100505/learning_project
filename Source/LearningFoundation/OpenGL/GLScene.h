#pragma once
#include <memory>
#include <cstdio>
#include <functional>
#include <vector>
#include <string>

#include "defines.h"
#include "learning.h"

#include "GLCamera.h"
#include "GLTexture.h"

class GLCamera;
class GLCubic;
class GLObject;
class GLQuad;
class GLPointLight;
class GLSpotLight;
class GLDirectionLight;

class GLScene {
public:
    GLScene(const glm::vec3& cameraPos, int32_t wwidth = WINDOW_WIDTH, 
        int32_t wheight = WINDOW_HEIGHT) {
        WindowWidth = wwidth;
        WindowHeight = wheight;
        WindowRatio = (float) wwidth / (float) wheight;
        MainCamera.reset(std::move(new GLCamera(cameraPos)));
    }

    virtual ~GLScene() {}

    void Render();

    GLCubic* AddCube(const std::string& vs_path, const std::string& fs_path);
    GLQuad* AddQuad(const std::string& vs_path, const std::string& fs_path);

    template<class T>
    T* AddSceneObject(const std::string& vs_path, const std::string& fs_path);

    GLDirectionLight* FirstDirectionLight();

    GLSpotLight* FirstSpotLight();

    GLPointLight* FirstPointLight();

    void Destroy();
private:
    std::vector<GLObject*> scene_objects;

public:
    int32_t WindowWidth;
    int32_t WindowHeight;
    float WindowRatio;
    std::unique_ptr<GLCamera> MainCamera;

    // 光源
    PhongModelSettings phong_model;
    std::vector<GLDirectionLight*> direction_lights;
    std::vector<GLSpotLight*> spot_lights;
    std::vector<GLPointLight*> point_lights;

    
};