# 3d-viewer-basic

A basic 3D model viewer written in C++ using OpenGL 3.3 (Core Profile), GLFW, GLEW and GLM.

## Features

- Loads Wavefront **OBJ** files (vertices, normals, texture-coordinates, fan-triangulated polygons)
- **Orbit camera** – rotate around the model with left-drag
- **Pan** – middle- or right-drag
- **Zoom** – mouse scroll wheel
- Blinn-Phong shading with a camera-relative point light
- **Wireframe** toggle (`W` key)
- Camera **reset** (`R` key)
- 4× MSAA anti-aliasing
- VSync enabled

## Dependencies

| Library | Version tested | Purpose |
|---------|---------------|---------|
| [GLFW 3](https://www.glfw.org/) | 3.3+ | Window & input |
| [GLEW](https://glew.sourceforge.net/) | 2.x | OpenGL extension loader |
| [GLM](https://github.com/g-truc/glm) | 0.9.9+ | Math (vectors, matrices) |
| CMake | 3.10+ | Build system |

### Ubuntu / Debian

```bash
sudo apt-get install libglfw3-dev libglew-dev libglm-dev
```

### macOS (Homebrew)

```bash
brew install glfw glew glm
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
# Default sample model (cube)
./build/3d-viewer

# Custom OBJ file
./build/3d-viewer path/to/model.obj
```

## Keyboard & Mouse Controls

| Input | Action |
|-------|--------|
| Left-drag | Orbit camera |
| Right-drag / Middle-drag | Pan camera |
| Scroll wheel | Zoom in/out |
| `W` | Toggle wireframe |
| `R` | Reset camera to fit model |
| `ESC` | Quit |

## Project Structure

```
3d-viewer-basic/
├── CMakeLists.txt          # Build configuration
├── README.md
├── src/
│   ├── main.cpp            # Entry point
│   ├── viewer.h / .cpp     # Window, render loop, input handling
│   ├── camera.h / .cpp     # Orbit camera
│   ├── mesh.h   / .cpp     # OBJ loader + GPU mesh (VAO/VBO/EBO)
│   └── shader.h / .cpp     # GLSL shader program management
├── shaders/
│   ├── vertex.glsl         # MVP transform, normal transform
│   └── fragment.glsl       # Blinn-Phong lighting
└── models/
    └── cube.obj            # Sample unit cube
```
