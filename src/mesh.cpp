#include "mesh.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <tuple>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

Mesh::Mesh()
    : m_vao(0), m_vbo(0), m_ebo(0), m_indexCount(0),
      m_center(0.0f), m_radius(1.0f) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

// ---------------------------------------------------------------------------
// OBJ loader
// ---------------------------------------------------------------------------

// Holds the three OBJ indices for a single face corner.
struct FaceVertex {
    int posIdx  = -1;
    int texIdx  = -1;
    int normIdx = -1;

    bool operator<(const FaceVertex& o) const {
        return std::tie(posIdx, texIdx, normIdx)
             < std::tie(o.posIdx, o.texIdx, o.normIdx);
    }
};

// Parse a single face-corner token such as "1", "1/2", "1//3", "1/2/3".
// Returns 0-based indices; unspecified components remain -1.
static FaceVertex parseFaceVertex(const std::string& token) {
    FaceVertex fv;
    size_t s1 = token.find('/');
    if (s1 == std::string::npos) {
        fv.posIdx = std::stoi(token) - 1;
        return fv;
    }
    fv.posIdx = std::stoi(token.substr(0, s1)) - 1;

    size_t s2 = token.find('/', s1 + 1);
    if (s2 == std::string::npos) {
        // "v/t"
        std::string t = token.substr(s1 + 1);
        if (!t.empty()) fv.texIdx = std::stoi(t) - 1;
    } else {
        // "v/t/n" or "v//n"
        std::string t = token.substr(s1 + 1, s2 - s1 - 1);
        std::string n = token.substr(s2 + 1);
        if (!t.empty()) fv.texIdx  = std::stoi(t) - 1;
        if (!n.empty()) fv.normIdx = std::stoi(n) - 1;
    }
    return fv;
}

bool Mesh::loadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open OBJ file: " << path << "\n";
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::map<FaceVertex, unsigned int> vertexMap;
    bool hasNormals = false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (token == "vt") {
            glm::vec2 tc;
            ss >> tc.x >> tc.y;
            texCoords.push_back(tc);
        } else if (token == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
            hasNormals = true;
        } else if (token == "f") {
            // Collect face corners
            std::vector<FaceVertex> face;
            std::string vtStr;
            while (ss >> vtStr) {
                face.push_back(parseFaceVertex(vtStr));
            }

            // Fan triangulation
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                for (int j : {0, (int)i, (int)(i + 1)}) {
                    const FaceVertex& fv = face[static_cast<size_t>(j)];
                    auto it = vertexMap.find(fv);
                    if (it != vertexMap.end()) {
                        indices.push_back(it->second);
                    } else {
                        Vertex v{};
                        if (fv.posIdx  >= 0 && fv.posIdx  < (int)positions.size())
                            v.position = positions[static_cast<size_t>(fv.posIdx)];
                        if (fv.texIdx  >= 0 && fv.texIdx  < (int)texCoords.size())
                            v.texCoord = texCoords[static_cast<size_t>(fv.texIdx)];
                        if (fv.normIdx >= 0 && fv.normIdx < (int)normals.size())
                            v.normal   = normals[static_cast<size_t>(fv.normIdx)];

                        auto idx = static_cast<unsigned int>(vertices.size());
                        vertices.push_back(v);
                        vertexMap[fv] = idx;
                        indices.push_back(idx);
                    }
                }
            }
        }
    }

    if (!hasNormals) {
        computeFlatNormals(vertices, indices);
    }

    computeBounds(vertices);
    upload(vertices, indices);
    return true;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Mesh::upload(const std::vector<Vertex>& vertices,
                  const std::vector<unsigned int>& indices) {
    m_indexCount = static_cast<GLsizei>(indices.size());

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    // layout(location = 0) position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // layout(location = 1) normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // layout(location = 2) texCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::computeBounds(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) return;

    glm::vec3 minP = vertices[0].position;
    glm::vec3 maxP = vertices[0].position;

    for (const auto& v : vertices) {
        minP = glm::min(minP, v.position);
        maxP = glm::max(maxP, v.position);
    }

    m_center = (minP + maxP) * 0.5f;

    float maxDist = 0.0f;
    for (const auto& v : vertices) {
        float d = glm::length(v.position - m_center);
        maxDist = std::max(maxDist, d);
    }
    m_radius = maxDist;
}

void Mesh::computeFlatNormals(std::vector<Vertex>& vertices,
                              const std::vector<unsigned int>& indices) {
    // Reset all normals
    for (auto& v : vertices) v.normal = glm::vec3(0.0f);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 n = glm::normalize(
            glm::cross(v1.position - v0.position,
                       v2.position - v0.position));
        v0.normal += n;
        v1.normal += n;
        v2.normal += n;
    }

    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.0f)
            v.normal = glm::normalize(v.normal);
    }
}
