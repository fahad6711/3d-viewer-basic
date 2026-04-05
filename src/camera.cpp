#include "camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float fov, float aspect, float nearP, float farP)
    : m_fov(fov), m_aspect(aspect), m_near(nearP), m_far(farP),
      m_target(0.0f, 0.0f, 0.0f),
      m_distance(3.0f),
      m_azimuth(45.0f),
      m_elevation(30.0f) {}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 pos = computePosition();
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    // Avoid gimbal lock near poles
    if (std::abs(m_elevation) > 89.0f) {
        up = glm::vec3(0.0f, 0.0f, m_elevation > 0.0f ? -1.0f : 1.0f);
    }
    return glm::lookAt(pos, m_target, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}

void Camera::orbit(float deltaX, float deltaY) {
    m_azimuth   += deltaX * 0.5f;
    m_elevation += deltaY * 0.5f;
    m_elevation  = std::clamp(m_elevation, -89.0f, 89.0f);
}

void Camera::zoom(float delta) {
    m_distance -= delta * m_distance * 0.1f;
    m_distance  = std::max(m_distance, 0.01f);
}

void Camera::pan(float deltaX, float deltaY) {
    glm::vec3 pos     = computePosition();
    glm::vec3 forward = glm::normalize(m_target - pos);
    glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up      = glm::normalize(glm::cross(right, forward));

    float scale = m_distance * 0.001f;
    m_target -= right * deltaX * scale;
    m_target += up    * deltaY * scale;
}

void Camera::setAspect(float aspect) {
    m_aspect = aspect;
}

void Camera::fitToMesh(const glm::vec3& center, float radius) {
    m_target   = center;
    float fovRad = glm::radians(m_fov);
    m_distance = (radius / std::sin(fovRad * 0.5f)) * 1.2f;
    m_near     = m_distance * 0.001f;
    m_far      = m_distance * 100.0f;
}

glm::vec3 Camera::getPosition() const {
    return computePosition();
}

glm::vec3 Camera::computePosition() const {
    float azRad = glm::radians(m_azimuth);
    float elRad = glm::radians(m_elevation);

    glm::vec3 offset;
    offset.x = m_distance * std::cos(elRad) * std::sin(azRad);
    offset.y = m_distance * std::sin(elRad);
    offset.z = m_distance * std::cos(elRad) * std::cos(azRad);

    return m_target + offset;
}
