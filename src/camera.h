#pragma once

#include <glm/glm.hpp>

// Orbit camera that circles a target point.
//
// Controls (managed by the Viewer):
//   Left-drag  -> orbit (azimuth / elevation)
//   Right-drag -> pan
//   Scroll     -> zoom
class Camera {
public:
    Camera(float fov    = 45.0f,
           float aspect = 16.0f / 9.0f,
           float nearP  = 0.1f,
           float farP   = 1000.0f);

    glm::mat4 getViewMatrix()       const;
    glm::mat4 getProjectionMatrix() const;

    void orbit(float deltaX, float deltaY);
    void zoom(float delta);
    void pan(float deltaX, float deltaY);

    void setAspect(float aspect);

    // Reposition the camera so the mesh fills the view.
    void fitToMesh(const glm::vec3& center, float radius);

    glm::vec3 getPosition() const;

private:
    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;

    glm::vec3 m_target;
    float     m_distance;
    float     m_azimuth;   // degrees, horizontal
    float     m_elevation; // degrees, vertical

    glm::vec3 computePosition() const;
};
