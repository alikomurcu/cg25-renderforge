#include <stdexcept>
#include <iostream>

#include "Engine/Engine.h"



int main() {
    try {
        Engine engine{ 1920, 1080, "RenderForge" };
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}