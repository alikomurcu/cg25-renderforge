#include "first_app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// TODO: make particles go only upwards
int main() {
    try {
        frg::FirstApp app{};
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
