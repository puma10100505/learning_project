#include "GLScene.h"

#include "GLObject.h"
#include "GLCubic.h"
#include "GLCamera.h"
#include "GLQuad.h"
#include "GLPointLight.h"
#include "GLDirectionLight.h"
#include "GLSpotLight.h"

void GLScene::Render() {
    for (GLObject* sceneObject: scene_objects) {
        if (sceneObject == nullptr) {
            continue;
        }

        sceneObject->BeforeDraw();
        sceneObject->Draw();
    }
}

GLCubic* GLScene::AddCube(const std::string& vs_path, const std::string& fs_path) {
    scene_objects.push_back(std::move(new GLCubic(this, vs_path, fs_path)));
    return dynamic_cast<GLCubic*>(scene_objects[scene_objects.size() - 1]);
}

GLQuad* GLScene::AddQuad(const std::string& vs_path, const std::string& fs_path) {
    scene_objects.push_back(std::move(new GLQuad(this, vs_path, fs_path)));
    return dynamic_cast<GLQuad*>(scene_objects[scene_objects.size() - 1]);
}

template<class T>
T* GLScene::AddSceneObject(const std::string& vs_path, const std::string& fs_path) {
    scene_objects.push_back(new T(this, vs_path, fs_path));
    return dynamic_cast<T*>(std::move(scene_objects[scene_objects.size() - 1]));
}

void GLScene::Destroy() {
    for (GLObject* obj: scene_objects) {
        if (obj != nullptr) {
            delete obj;
            obj = nullptr;
        }
    }
}

GLDirectionLight* GLScene::FirstDirectionLight() {
    if (direction_lights.size() <= 0) {
        direction_lights.push_back(std::move(new GLDirectionLight()));
    }

    return direction_lights[0];
}

GLSpotLight* GLScene::FirstSpotLight() {
    if (spot_lights.size() <= 0) {
        spot_lights.push_back(std::move(new GLSpotLight()));
    }

    return spot_lights[0];
}

GLPointLight* GLScene::FirstPointLight() {
    if (point_lights.size() <= 0) {
        point_lights.push_back(std::move(new GLPointLight()));
    }

    return point_lights[0];
}