#include "shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertSource = loadFile(vertexPath);
    std::string fragSource = loadFile(fragmentPath);

    GLuint vertShader = compileShader(GL_VERTEX_SHADER,   vertSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource);

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertShader);
    glAttachShader(m_programID, fragShader);
    glLinkProgram(m_programID);

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        throw std::runtime_error(std::string("Shader link error: ") + infoLog);
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

Shader::~Shader() {
    glDeleteProgram(m_programID);
}

void Shader::use() const {
    glUseProgram(m_programID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::setFloat(const std::string& name, float value) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniform1f(loc, value);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string Shader::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open shader file: " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error: ") + infoLog);
    }
    return shader;
}
