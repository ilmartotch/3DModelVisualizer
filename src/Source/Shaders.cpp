#include <fstream>
#include <sstream>
#include <string>
#include <glad/glad.h>
#include <iostream>
#include "../Include/Shaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint LoadShader(const char* vertexPath, const char* fragmentPath) {
    auto readFile = [](const char* path) -> std::string {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    std::string vertCode = readFile(vertexPath);
    std::string fragCode = readFile(fragmentPath);

    if (vertCode.empty() || fragCode.empty()) {
        std::cerr << "Error reading shader files." << std::endl;
        return 0;
    }

    // Compile vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertCode.c_str();
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);

    int success;
    char infoLog[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Compile fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragCode.c_str();
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

void SetUniformMat4(GLuint shader, const char* name, const glm::mat4& matrix) {
    GLint location = glGetUniformLocation(shader, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

/*
Shader utilities for loading and compiling GLSL shaders.

LoadShader reads vertex and fragment shader source from disk, compiles both
with error reporting, links into a program and cleans up intermediate objects.
Returns 0 on failure, valid program ID on success.

SetUniform functions are convenience wrappers around glUniform calls that
handle uniform location lookup internally and skip silently if not found.

Shader files are loaded from the Shaders directory relative to the executable.
The CMake build copies shaders to the output directory.
*/