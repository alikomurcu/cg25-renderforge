#pragma once

#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_window.hpp"
#include "frg_renderer.hpp"

// std
#include <memory>
#include <vector>

namespace frg
{
    class FirstApp
    {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp &) = delete;

        void run();

    private:
        void loadGameObjects();

        FrgWindow frgWindow{WIDTH, HEIGHT, "MERHABA VULKAN!"};
        FrgDevice frgDevice{frgWindow};
        FrgRenderer frgRenderer{frgWindow, frgDevice};

        std::vector<FrgGameObject> gameObjects;
    };
} // namespace frg