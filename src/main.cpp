#include <iostream>
#include "viewer.h"

int main(int argc, char* argv[]) {
    const std::string modelPath = (argc > 1) ? argv[1] : "models/cube.obj";

    std::cout << "3D Viewer — loading: " << modelPath << "\n"
              << "Controls:\n"
              << "  Left-drag   Orbit\n"
              << "  Right-drag  Pan\n"
              << "  Scroll      Zoom\n"
              << "  W           Toggle wireframe\n"
              << "  R           Reset camera\n"
              << "  ESC         Quit\n\n";

    try {
        Viewer viewer(1280, 720, "3D Viewer");

        if (!viewer.loadModel(modelPath)) {
            std::cerr << "Error: could not load \"" << modelPath << "\"\n";
            return 1;
        }

        viewer.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
