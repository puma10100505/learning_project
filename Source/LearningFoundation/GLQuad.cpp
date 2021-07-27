#include "GLQuad.h"

#include "GLObject.h"
#include "GLScene.h"

GLQuad::GLQuad(){

}

GLQuad::GLQuad(GLScene* sc, const std::string& vs_path, const std::string& fs_path)
    :GLObject(sc, vs_path, fs_path){
    float vertices[] = {
        0.5f, 0.5f, 0.0f, 0.0f,  1.0f,  0.0f, 1.0f, 1.0f, 
        0.5f, -0.5f, 0.0f,0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,0.0f,  1.0f,  0.0f, 0.0f, 0.0f, 
        -0.5f, 0.5f, 0.0f,0.0f,  1.0f,  0.0f, 0.0f, 1.0f
    };

    int indices[] = {
        0, 1, 3, 1, 2, 3
    };

    // unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // set vertices data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // descript the usage of vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    scene = sc;
}

void GLQuad::Draw() {
    if (diffuse_texture != nullptr) {
        diffuse_texture->Active();
    }

    if (specular_texture != nullptr) {
        specular_texture->Active();
    }

    //std::cout << "begin to draw cubic" << std::endl;
    
    if (bindVAO == false) {
        glBindVertexArray(VAO);
        bindVAO = true;
    }

    PrepareShader();

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GLQuad::Destroy() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}
