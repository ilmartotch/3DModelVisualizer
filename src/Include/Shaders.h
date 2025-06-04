#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

GLuint LoadShader(const char* vertexPath, const char* fragmentPath);

void SetUniformMat4(GLuint shader, const char* name, const glm::mat4& matrix);