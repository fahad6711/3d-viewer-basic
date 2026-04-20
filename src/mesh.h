#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

// One GPU-side vertex.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// Loads an OBJ file and owns the OpenGL VAO/VBO/EBO.
class Mesh {
public:
    Mesh();
    ~Mesh();

    // Returns false if the file cannot be opened.
    bool loadOBJ(const std::string& path);

    void draw() const;

    glm::vec3 getCenter()          const { return m_center; }
    float     getBoundingRadius()  const { return m_radius; }

private:
    GLuint   m_vao;
    GLuint   m_vbo;
    GLuint   m_ebo;
    GLsizei  m_indexCount;

    glm::vec3 m_center;
    float     m_radius;

    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices);

    void computeBounds(const std::vector<Vertex>& vertices);

    // Compute flat normals for meshes that have none.
    static void computeFlatNormals(std::vector<Vertex>& vertices,
                                   const std::vector<unsigned int>& indices);
};
