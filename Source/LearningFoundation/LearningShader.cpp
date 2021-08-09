#include "LearningShader.h"
#include "CommonDefines.h"

Shader::Shader(const std::string& InShaderName)
{
    std::string VertexShaderFileName = InShaderName + ".vs";
    std::string FragmentShaderFileName = InShaderName + ".fs";
    Init((DefaultShaderDirectory + VertexShaderFileName).c_str(), 
        (DefaultShaderDirectory + FragmentShaderFileName).c_str());
}

Shader::Shader(const std::string& vsPath, const std::string& fsPath, const std::string& geoPath)
{
    if (geoPath.length() > 0)
    {
        Init(vsPath.c_str(), fsPath.c_str(), geoPath.c_str());
    }
    else
    {
        Init(vsPath.c_str(), fsPath.c_str());
    }
}

Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    Init(vertexPath, fragmentPath, geometryPath);
}

void Shader::Init(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try 
    {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();		
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();			
        // if geometry shader path is present, also load a geometry shader
        if(geometryPath != nullptr)
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char * fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");
    // if geometry shader is given, compile geometry shader
    unsigned int geometry;
    if(geometryPath != nullptr)
    {
        const char * gShaderCode = geometryCode.c_str();
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        CheckCompileErrors(geometry, "GEOMETRY");
    }
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    if(geometryPath != nullptr)
        glAttachShader(ID, geometry);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if(geometryPath != nullptr)
        glDeleteShader(geometry);

}

// activate the shader
// ------------------------------------------------------------------------
void Shader::use() 
{ 
    glUseProgram(ID); 
}


// utility uniform functions
// ------------------------------------------------------------------------
void Shader::setBool(const std::string &name, bool value) const
{         
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
}

// ------------------------------------------------------------------------
void Shader::setInt(const std::string &name, int value) const
{ 
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
}

// ------------------------------------------------------------------------
void Shader::setFloat(const std::string &name, float value) const
{ 
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
}



// ------------------------------------------------------------------------
void Shader::setVec2(const std::string &name, const glm::vec2 &value) const
{ 
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
}

void Shader::setVec2(const std::string &name, float x, float y) const
{ 
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
}
// ------------------------------------------------------------------------
void Shader::setVec3(const std::string &name, const glm::vec3 &value) const
{ 
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
}
void Shader::setVec3(const std::string &name, float x, float y, float z) const
{ 
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
}
// ------------------------------------------------------------------------
void Shader::setVec4(const std::string &name, const glm::vec4 &value) const
{ 
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
}

void Shader::setVec4(const std::string &name, float x, float y, float z, float w) 
{ 
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
}


// ------------------------------------------------------------------------
void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

// -------------------------------------- New Version Interfaces ----------------------------------------

void Shader::Activate()
{
    glUseProgram(ID);
}

void Shader::SetBoolValue(const std::string& Name, const bool Value) const
{
    glUniform1i(glGetUniformLocation(ID, Name.c_str()), (int)Value); 
}

void Shader::SetIntValue(const std::string &Name, const int Value) const
{ 
    glUniform1i(glGetUniformLocation(ID, Name.c_str()), Value); 
}

void Shader::SetFloatValue(const std::string &Name, const float Value) const
{ 
    glUniform1f(glGetUniformLocation(ID, Name.c_str()), Value); 
}

void Shader::SetVector2Value(const std::string &Name, const glm::vec2 &Value) const
{ 
    glUniform2fv(glGetUniformLocation(ID, Name.c_str()), 1, &Value[0]); 
}

void Shader::SetVector2Value(const std::string &Name, const float x, const float y) const
{ 
    glUniform2f(glGetUniformLocation(ID, Name.c_str()), x, y); 
}

void Shader::SetVector3Value(const std::string &Name, const glm::vec3 &Value) const
{ 
    glUniform3fv(glGetUniformLocation(ID, Name.c_str()), 1, &Value[0]); 
}

void Shader::SetVector3Value(const std::string &Name, const float x, const float y, const float z) const
{ 
    glUniform3f(glGetUniformLocation(ID, Name.c_str()), x, y, z); 
}

void Shader::SetVector4Value(const std::string &Name, const glm::vec4 &Value) const
{ 
    glUniform4fv(glGetUniformLocation(ID, Name.c_str()), 1, &Value[0]); 
}

void Shader::SetVector4Value(const std::string &Name, const float x, const float y, const float z, const float w) const
{ 
    glUniform4f(glGetUniformLocation(ID, Name.c_str()), x, y, z, w); 
}

void Shader::SetMatrix2Value(const std::string &Name, const glm::mat2 &MatVal) const
{
    glUniformMatrix2fv(glGetUniformLocation(ID, Name.c_str()), 1, GL_FALSE, &MatVal[0][0]);
}

void Shader::SetMatrix3Value(const std::string &Name, const glm::mat3 &MatVal) const
{
    glUniformMatrix3fv(glGetUniformLocation(ID, Name.c_str()), 1, GL_FALSE, &MatVal[0][0]);
}

void Shader::SetMatrix4Value(const std::string &Name, const glm::mat4 &MatVal) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, Name.c_str()), 1, GL_FALSE, &MatVal[0][0]);
}

void Shader::CheckCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }