#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;

    Shader(const std::string& InShaderName);
    Shader(const std::string& vsPath, const std::string& fsPath, const std::string& geoPath = "");
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

    /* ------------------------------- Old Version of Methods ------------------------------- */
    void use();
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w);
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

    /* ------------------------------- New Version of Methods ------------------------------- */
    void Activate();
    void SetBoolValue(const std::string& Name, const bool Val) const;
    void SetIntValue(const std::string &Name, const int Value) const;
    void SetFloatValue(const std::string &Name, const float Val) const;
    void SetVector2Value(const std::string &Name, const glm::vec2 &Value) const;
    void SetVector2Value(const std::string &Name, const float x, const float y) const;
    void SetVector3Value(const std::string &Name, const glm::vec3 &Value) const;
    void SetVector3Value(const std::string &Name, const float x, const float y, const float z) const;
    void SetVector4Value(const std::string &Name, const glm::vec4 &Value) const;
    void SetVector4Value(const std::string &Name, const float x, const float y, const float z, const float w) const;
    void SetMatrix2Value(const std::string &Name, const glm::mat2 &MatVal) const;
    void SetMatrix3Value(const std::string &Name, const glm::mat3 &MatVal) const;
    void SetMatrix4Value(const std::string &Name, const glm::mat4 &MatVal) const;


private:
    void Init(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    void CheckCompileErrors(GLuint shader, std::string type);
};
#endif