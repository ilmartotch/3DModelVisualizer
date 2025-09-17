#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint LoadShader(const char* vertexPath, const char* fragmentPath);

void SetUniformMat4(GLuint shader, const char* name, const glm::mat4& matrix);

// Funzione helper per impostare un uniform vec3 in uno shader OpenGL
inline void SetUniformVec3(GLuint shader, const char* name, const glm::vec3& value) {
    GLint location = glGetUniformLocation(shader, name);
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

inline void SetUniformFloat(GLuint shader, const char* name, float value) {
    GLint location = glGetUniformLocation(shader, name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}