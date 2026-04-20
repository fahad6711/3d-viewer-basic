#include "viewer.h"

#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

Viewer::Viewer(int width, int height, const std::string& title)
    : m_window(nullptr),
      m_width(width), m_height(height),
      m_lastMouseX(0.0), m_lastMouseY(0.0),
      m_leftMouseDown(false), m_middleMouseDown(false), m_rightMouseDown(false),
      m_wireframe(false) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // 4x MSAA

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSwapInterval(1);  // vsync

    // Register GLFW callbacks
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error(
            std::string("GLEW init failed: ") +
            reinterpret_cast<const char*>(glewGetErrorString(err)));
    }

    initGL();

    m_camera = std::make_unique<Camera>(
        45.0f, static_cast<float>(width) / static_cast<float>(height),
        0.1f, 1000.0f);

    m_shader = std::make_unique<Shader>("shaders/vertex.glsl",
                                        "shaders/fragment.glsl");

    m_mesh = std::make_unique<Mesh>();
}

Viewer::~Viewer() {
    // Destroy GL resources before the context goes away.
    m_shader.reset();
    m_mesh.reset();

    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool Viewer::loadModel(const std::string& path) {
    if (!m_mesh->loadOBJ(path)) return false;
    m_camera->fitToMesh(m_mesh->getCenter(), m_mesh->getBoundingRadius());
    return true;
}

void Viewer::run() {
    while (!glfwWindowShouldClose(m_window)) {
        render();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

void Viewer::initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
}

void Viewer::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);

    const glm::mat4 model(1.0f);
    const glm::mat4 view = m_camera->getViewMatrix();
    const glm::mat4 proj = m_camera->getProjectionMatrix();
    const glm::mat3 normalMat =
        glm::transpose(glm::inverse(glm::mat3(model)));

    m_shader->use();
    m_shader->setMat4("uModel",      model);
    m_shader->setMat4("uView",       view);
    m_shader->setMat4("uProjection", proj);
    m_shader->setMat3("uNormalMatrix", normalMat);

    // Slightly offset light so it is above-right of the camera.
    const glm::vec3 camPos = m_camera->getPosition();
    m_shader->setVec3("uLightPos",    camPos + glm::vec3(1.0f, 2.0f, 1.0f));
    m_shader->setVec3("uLightColor",  glm::vec3(1.0f, 1.0f, 1.0f));
    m_shader->setVec3("uViewPos",     camPos);
    m_shader->setVec3("uObjectColor", glm::vec3(0.72f, 0.72f, 0.80f));

    m_mesh->draw();
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void Viewer::onResize(int width, int height) {
    m_width  = width;
    m_height = height;
    glViewport(0, 0, width, height);
    if (height > 0)
        m_camera->setAspect(static_cast<float>(width) /
                            static_cast<float>(height));
}

void Viewer::onMouseButton(int button, int action, int /*mods*/) {
    bool pressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_LEFT)   m_leftMouseDown   = pressed;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) m_middleMouseDown = pressed;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)  m_rightMouseDown  = pressed;

    if (pressed) {
        glfwGetCursorPos(m_window, &m_lastMouseX, &m_lastMouseY);
    }
}

void Viewer::onMouseMove(double x, double y) {
    double dx = x - m_lastMouseX;
    double dy = y - m_lastMouseY;
    m_lastMouseX = x;
    m_lastMouseY = y;

    if (m_leftMouseDown) {
        m_camera->orbit(static_cast<float>(dx), static_cast<float>(dy));
    }
    if (m_middleMouseDown || m_rightMouseDown) {
        m_camera->pan(static_cast<float>(dx), static_cast<float>(dy));
    }
}

void Viewer::onScroll(double /*xOffset*/, double yOffset) {
    m_camera->zoom(static_cast<float>(yOffset));
}

void Viewer::onKey(int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            break;
        case GLFW_KEY_W:
            m_wireframe = !m_wireframe;
            std::cout << "Wireframe: " << (m_wireframe ? "ON" : "OFF") << "\n";
            break;
        case GLFW_KEY_R:
            m_camera->fitToMesh(m_mesh->getCenter(), m_mesh->getBoundingRadius());
            std::cout << "Camera reset\n";
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Static GLFW callbacks
// ---------------------------------------------------------------------------

void Viewer::framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    static_cast<Viewer*>(glfwGetWindowUserPointer(w))->onResize(width, height);
}

void Viewer::mouseButtonCallback(GLFWwindow* w, int btn, int action, int mods) {
    static_cast<Viewer*>(glfwGetWindowUserPointer(w))->onMouseButton(btn, action, mods);
}

void Viewer::cursorPosCallback(GLFWwindow* w, double x, double y) {
    static_cast<Viewer*>(glfwGetWindowUserPointer(w))->onMouseMove(x, y);
}

void Viewer::scrollCallback(GLFWwindow* w, double xo, double yo) {
    static_cast<Viewer*>(glfwGetWindowUserPointer(w))->onScroll(xo, yo);
}

void Viewer::keyCallback(GLFWwindow* w, int key, int sc, int action, int mods) {
    static_cast<Viewer*>(glfwGetWindowUserPointer(w))->onKey(key, sc, action, mods);
}
