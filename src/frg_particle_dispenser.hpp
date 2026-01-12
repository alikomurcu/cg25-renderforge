#pragma once

#define GLFW_INCLUDE_VULKAN
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include <GLFW/glfw3.h>
#include <random>
#include <time.h>

#include "frg_device.hpp"

namespace frg {
struct Particle {
  public:
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 col;
};
class FrgParticleDispenser {
  public:
    FrgParticleDispenser(FrgDevice &device, uint32_t particle_count, int h, int w);
    ~FrgParticleDispenser();

    const uint32_t particle_count() { return m_particle_count; }
    const VkBuffer *staging_buffer() { return &m_staging_buff; }
    const VkDeviceSize *staging_buffer_size() { return &m_staging_buff_size; }
    void cpy_host2dev(std::vector<VkBuffer> &buffers, std::vector<VkDeviceMemory> &buffers_memory);

  private:
    FrgDevice &m_device;
    uint32_t m_particle_count;
    VkBuffer m_staging_buff;
    VkDevice Memory m_staging_buff_mem;
    VkDeviceSize m_staging_buff_size;

    std::vector<Particle> m_particles;
};
} // namespace frg
