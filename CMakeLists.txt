cmake_minimum_required(VERSION 3.10)
project(3DViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# GLM is header-only; try find_package, fall back to system include
find_package(glm QUIET)

set(SOURCES
    src/main.cpp
    src/viewer.cpp
    src/camera.cpp
    src/mesh.cpp
    src/shader.cpp
)

add_executable(3d-viewer ${SOURCES})

target_include_directories(3d-viewer PRIVATE src)

target_link_libraries(3d-viewer PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
)

if(glm_FOUND)
    target_link_libraries(3d-viewer PRIVATE glm::glm)
endif()

# Copy runtime assets to the build directory
add_custom_command(TARGET 3d-viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:3d-viewer>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/models  $<TARGET_FILE_DIR:3d-viewer>/models
    COMMENT "Copying shaders and models to build directory"
)
