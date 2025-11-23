#include "first_app.hpp"

#include "keyboard_movement_controller.hpp"
#include "frg_camera.hpp"
#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <stdexcept>
#include <chrono>
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace frg
{
    FirstApp::FirstApp()
    {
        loadGameObjects();
        frgDescriptor.write_descriptor_sets(get_descriptors_of_game_objects());
    }

    FirstApp::~FirstApp() {}

    std::vector<VkDescriptorImageInfo> FirstApp::get_descriptors_of_game_objects()
    {
        std::vector<VkDescriptorImageInfo> all_descriptor_infos{};
        for (const auto &obj : gameObjects)
        {
            std::vector<VkDescriptorImageInfo> tmp_infos =
                obj.model->get_descriptors();
            all_descriptor_infos.insert(all_descriptor_infos.end(),
                                        tmp_infos.begin(),
                                        tmp_infos.end());
        }

        return all_descriptor_infos;
    }

    void FirstApp::run()
    {
        SimpleRenderSystem simpleRenderSystem{frgDevice,
                                              frgRenderer.getSwapChainRenderPass(),
                                              frgDescriptor};
        FrgCamera camera{};
        // example camera setup
        camera.setViewTarget(glm::vec3{0.f, -.5f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});
        auto viewerObject = FrgGameObject::createGameObject();
        KeyboardMovementController cameraController;

        auto curTime = std::chrono::high_resolution_clock::now();

        // Add a point light to the scene
        simpleRenderSystem.getLightManager().addPointLight(
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(1.f, 1.f, 1.f),
            1.f,
            3.f);
        while (!frgWindow.shouldClose())
        {
            glfwPollEvents();
            float aspect = frgRenderer.getAspectRatio();
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - curTime).count();
            curTime = newTime;

            // frameTime = glm::min(frameTime, 0.1f); // avoid large dt values

            cameraController.moveInPlaneXZ(frgWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

            if (auto commandBuffer = frgRenderer.beginFrame())
            {
                // make the render passes here, for example: shadow pass, post
                // processing pass, etc.
                frgRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer,
                                                     gameObjects,
                                                     camera,
                                                     frameTime);

                frgRenderer.endSwapChainRenderPass(commandBuffer);
                frgRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(frgDevice.device());
    }

    void FirstApp::loadGameObjects()
    {
        // add car model

        std::string file_path = "../resources/models/car/source/car5/car5.obj";
        auto g_obj = FrgGameObject::createGameObject();
        g_obj.model = std::make_shared<FrgModel>(frgDevice, file_path);

        // Load textures from the resources/models/car/textures directory
        std::unique_ptr<Texture> diffuse_tex = std::make_unique<Texture>(
            frgDevice,
            "texture_diffuse",
            "../resources/models/car/textures/TexturesLights.png");
        g_obj.model->add_texture_to_mesh(0, diffuse_tex);
        g_obj.model->set_texture_index_for_mesh(0, globalTextureIndex);
        globalTextureIndex++;

        std::cout << "# of vertices: " << g_obj.model->vertex_count() << std::endl;
        g_obj.transform.scale = glm::vec3{.01f, .01f, .01f};
        // g_obj.transform.rotation.x = glm::pi<float>() / 2.0f;
        // g_obj.transform.rotation.y = glm::pi<float>() / 2.0f;
        g_obj.transform.rotation.z = glm::pi<float>();
        gameObjects.emplace_back(std::move(g_obj));

        // add ground model
        file_path = "../resources/models/ground/ground.obj";
        auto ground_obj = FrgGameObject::createGameObject();
        ground_obj.model = std::make_shared<FrgModel>(frgDevice, file_path);

        std::unique_ptr<Texture> ground_diffuse_tex = std::make_unique<Texture>(
            frgDevice,
            "texture_diffuse",
            "../resources/models/ground/0.jpeg");
        ground_obj.model->add_texture_to_mesh(0, ground_diffuse_tex);
        ground_obj.model->set_texture_index_for_mesh(0, globalTextureIndex);
        globalTextureIndex++;

        std::cout << "# of vertices: " << ground_obj.model->vertex_count() << std::endl;
        ground_obj.transform.scale = glm::vec3{0.4f, 0.4f, 0.4f};
        gameObjects.emplace_back(std::move(ground_obj));
    }
} // namespace frg
