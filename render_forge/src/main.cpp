#include <stdexcept>
#include <iostream>

#include "Engine/Engine.h"



int main() {
    try {
        Engine engine{ 1280, 720, "RenderForge" };
        engine.setup_vulkan();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}