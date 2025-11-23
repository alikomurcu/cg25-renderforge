#pragma once

#include "frg_descriptor.hpp"
#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_renderer.hpp"
#include "frg_window.hpp"

// std
#include <memory>
#include <string>
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

    FrgWindow frgWindow{WIDTH, HEIGHT, "RenderForge"};
    FrgDevice frgDevice{frgWindow};
    FrgDescriptor frgDescriptor{frgDevice};
    FrgRenderer frgRenderer{frgWindow, frgDevice};

    std::vector<VkDescriptorImageInfo> get_descriptors_of_game_objects();
    std::vector<FrgGameObject> gameObjects;
    uint32_t globalTextureIndex{0};
  };
} // namespace frg