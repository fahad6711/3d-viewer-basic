#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

// Loads, compiles and links a GLSL vertex + fragment shader pair.
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;

    GLuint getID() const { return m_programID; }

private:
    GLuint m_programID;

    static std::string loadFile(const std::string& path);
    static GLuint compileShader(GLenum type, const std::string& source);
};
