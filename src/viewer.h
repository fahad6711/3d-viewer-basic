#pragma once

#include <memory>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "mesh.h"
#include "shader.h"

// Main application class.  Creates a GLFW window and manages the render loop.
class Viewer {
public:
    Viewer(int width = 1280, int height = 720,
           const std::string& title = "3D Viewer");
    ~Viewer();

    // Load an OBJ model and auto-fit the camera.  Returns false on failure.
    bool loadModel(const std::string& path);

    // Enter the render loop (blocks until the window is closed).
    void run();

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;

    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Mesh>   m_mesh;
    std::unique_ptr<Shader> m_shader;

    // Mouse state
    double m_lastMouseX;
    double m_lastMouseY;
    bool   m_leftMouseDown;
    bool   m_middleMouseDown;
    bool   m_rightMouseDown;

    bool m_wireframe;

    void initGL();
    void render();

    // Event handlers
    void onResize(int width, int height);
    void onMouseButton(int button, int action, int mods);
    void onMouseMove(double x, double y);
    void onScroll(double xOffset, double yOffset);
    void onKey(int key, int scancode, int action, int mods);

    // Static GLFW callbacks that forward to the Viewer instance.
    static void framebufferSizeCallback(GLFWwindow* w, int width, int height);
    static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void scrollCallback(GLFWwindow* w, double xOffset, double yOffset);
    static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
};
