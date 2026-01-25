#pragma once

#include "frg_descriptor.hpp"
#include "frg_device.hpp"
#include "frg_game_object.hpp"
#include "frg_particle_dispenser.hpp"
#include "frg_gbuffer.hpp"
#include "frg_lighting.hpp"
#include "frg_renderer.hpp"
#include "frg_ssao.hpp"
#include "frg_window.hpp"
#include "scene_loader.hpp"
#include "ssao_render_system.hpp"

// std
#include <memory>
#include <string>
#include <vector>

namespace frg {
class FirstApp {
  public:
    static constexpr int WIDTH = 1920;
    static constexpr int HEIGHT = 1080;

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
    FrgParticleDispenser frgParticleDispenser{
        frgDevice, 131072, HEIGHT, WIDTH, {1.3, -0.2, -1.8, 0.0}
    };

    std::vector<VkDescriptorImageInfo> get_descriptors_of_game_objects();
    std::vector<FrgGameObject> gameObjects;
    std::vector<VkCommandBuffer> computeCommandBuffers;
    FrgGameObject viewerObject{FrgGameObject::createGameObject()};
    LightManager lightManager;
    SceneSettings sceneSettings;
    uint32_t globalTextureIndex{0};
};
} // namespace frg