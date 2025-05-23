#include <fstream>
#include <sstream>
#include <string>
#include <glad/glad.h>
#include <iostream>

GLuint LoadShader(const char* vertexPath, const char* fragmentPath)
{
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

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertCode.c_str();
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragCode.c_str();
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}
