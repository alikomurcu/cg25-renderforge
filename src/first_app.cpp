#include "first_app.hpp"

#include "frg_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace frg {
FirstApp::FirstApp() {
    loadGameObjects();
    frgDescriptor.write_descriptor_sets(get_descriptors_of_game_objects());
    computeCommandBuffers = frgDevice.createComputeCommandBuffers(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
}

FirstApp::~FirstApp() {}

std::vector<VkDescriptorImageInfo> FirstApp::get_descriptors_of_game_objects() {
    std::vector<VkDescriptorImageInfo> all_descriptor_infos{};
    for (const auto &obj : gameObjects) {
        std::vector<VkDescriptorImageInfo> tmp_infos = obj.model->get_descriptors();
        all_descriptor_infos.insert(all_descriptor_infos.end(), tmp_infos.begin(), tmp_infos.end());
    }

    return all_descriptor_infos;
}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{frgDevice, frgRenderer.getSwapChainRenderPass(), frgDescriptor};
    simpleRenderSystem.setup_ssbos(frgParticleDispenser);
    simpleRenderSystem.set_up_compute_desc_sets(frgParticleDispenser.particle_count() * sizeof(Particle));
    FrgCamera camera{};
    // example camera setup
    camera.setViewTarget(glm::vec3{0.f, 0.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});
    auto viewerObject = FrgGameObject::createGameObject();
    KeyboardMovementController cameraController;

    auto curTime = std::chrono::high_resolution_clock::now();

    // Add a point light to the scene
    simpleRenderSystem.getLightManager().addPointLight(glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 1.f, 1.f), 1.f, 3.f);
    while (!frgWindow.shouldClose()) {
        glfwPollEvents();
        float aspect = frgRenderer.getAspectRatio();
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - curTime).count();
        curTime = newTime;

        // frameTime = glm::min(frameTime, 0.1f); // avoid large dt values

        cameraController.moveInPlaneXZ(frgWindow.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        if (auto commandBuffer = frgRenderer.beginFrame()) {
            // make the render passes here, for example: shadow pass, post
            // processing pass, etc.
            frgRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera, frameTime);
            simpleRenderSystem.bindComputeGraphicsPipeline(commandBuffer);
            UniformBufferObject ubo{};
            ubo.deltaTime = frameTime;
            SimplePushConstantData push{};
            auto projView = camera.getProjectionMatrix() * camera.getViewMatrix();
            auto modelMat = frgParticleDispenser.transform.mat4();
            push.transform = projView * modelMat;
            vkCmdPushConstants(
                commandBuffer,
                simpleRenderSystem.getComputeGraphicsPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push
            );

            ubo.w_parent_pos = {frgParticleDispenser.transform.translation, .6f};
            frgRenderer.renderComputePipeline(
                computeCommandBuffers,
                frgDescriptor,
                simpleRenderSystem.getComputePipelineLayout(),
                simpleRenderSystem.getComputePipeline(),
                frgParticleDispenser.particle_count(),
                ubo,
                simpleRenderSystem.getUbosMapped()
            );
            frgRenderer.delegateComputeBindAndDraw(
                commandBuffer,
                simpleRenderSystem.getSSBOS(),
                frgParticleDispenser.particle_count()
            );
            frgRenderer.endSwapChainRenderPass(commandBuffer);
            frgRenderer.endFrame(true);
        }
    }

    vkDeviceWaitIdle(frgDevice.device());
}

void FirstApp::loadGameObjects() {
    std::array<std::string, 1> file_paths = {"../resources/models/psx_garage/scene.gltf"};

    for (const auto &path : file_paths) {
        auto g_obj = FrgGameObject::createGameObject();
        g_obj.model = std::make_shared<FrgModel>(frgDevice, path);

        std::cout << "# of vertices: " << g_obj.model->vertex_count() << std::endl;

        gameObjects.emplace_back(std::move(g_obj));
    }

    // gameObjects[0].transform.scale = glm::vec3{.01f, .01f, .01f};
    gameObjects[0].transform.rotation.x = glm::pi<float>();
    gameObjects[0].transform.rotation.y = glm::pi<float>() / 2.0f;
    gameObjects[0].transform.translation.z = 0.7f;
    // gameObjects[1].transform.translation.y = -0.2f;
    // gameObjects[1].transform.translation.z = 0.7f;

    // // gameObjects[1].transform.scale = glm::vec3{0.1f, 0.1f, 0.1f};
    // gameObjects[1].transform.rotation.x = -glm::pi<float>();
    // gameObjects[1].transform.rotation.y = glm::pi<float>() / 4.0f;
}
} // namespace frg