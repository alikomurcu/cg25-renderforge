#pragma once

#define GLFW_INCLUDE_VULKAN
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <random>
#include <time.h>

#include "frg_device.hpp"
#include "frg_game_object.hpp"

namespace frg {

struct UniformBufferObject {
    float deltaTime = 1.0f;
    glm::vec4 w_parent_pos{};
};

//
//  flags contains flags to the compute shader
//  flags.x -> ttl (Time to Live) of the given particle, if its 0 it is considered to be dead
//  flags.y -> DEGREE | variance of the angle that the particle could achieve in relation to the dir vector
//  flags.z -> DEGREE | angle of domain from which dir vec can take values
//  flags.w -> default ttl for the particle
//
// 64 bytes
struct Particle {
  public:
    glm::vec4 pos;    // 16 byte
    glm::vec4 vel;    // 16 byte
    glm::ivec4 flags; // 16 byte
    glm::vec4 dir;    // 16 byte

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};
class FrgParticleDispenser {
  public:
    FrgParticleDispenser(
        FrgDevice &device, uint32_t particle_count, int h, int w, glm::vec4 position = {0.0f, 0.0f, 0.0f, 1.0f},
        int angle_of_domain = 45, int angle_var = 10
    );
    ~FrgParticleDispenser();

    const uint32_t particle_count() { return m_particle_count; }
    const VkBuffer *staging_buffer() { return &m_staging_buff; }
    const VkDeviceSize *staging_buffer_size() { return &m_staging_buff_size; }
    void cpy_host2dev(std::vector<VkBuffer> &buffers, std::vector<VkDeviceMemory> &buffers_memory);
    TransformComponent transform{};

  private:
    std::default_random_engine m_rand_eng;
    std::uniform_real_distribution<float> m_dist01;
    std::uniform_real_distribution<float> m_dist11;
    glm::vec4 generate_point_in_sphere(glm::vec4 w_sphere_origin);
    FrgDevice &m_device;
    uint32_t m_particle_count;
    VkBuffer m_staging_buff;
    VkDeviceMemory m_staging_buff_mem;
    VkDeviceSize m_staging_buff_size;

    std::vector<Particle> m_particles;
};
} // namespace frg
