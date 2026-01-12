#include "frg_particle_dispenser.hpp"

namespace frg {
FrgParticleDispenser::FrgParticleDispenser(FrgDevice &device, uint32_t particle_count, int h, int w)
    : m_device{device}, m_particle_count{particle_count} {

    std::default_random_engine rnd_eng(static_cast<unsigned>(time(nullptr)));
    std::uniform_real_distribution<float> rnd_dist(0.0f, 1.0f);
    m_particles.resize(m_particle_count);
    for (auto &particle : m_particles) {
        float r = 0.25f * sqrt(rnd_dist(rnd_eng));
        float theta = rnd_dist(rnd_eng) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * h / w;
        float y = r * sin(theta);
        particle.pos = glm::vec2(x, y);
        particle.vel = glm::normalize(glm::vec2(x, y)) * 0.00025f;
        particle.col = glm::vec4(rnd_dist(rnd_eng), rnd_dist(rnd_eng), rnd_dist(rnd_eng), 1.0f);
    }

    m_staging_buff_size = sizeof(Particle) * m_particle_count;
    m_device.createBuffer(
        m_staging_buff_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_staging_buff,
        m_staging_buff_mem
    );

    void *data;
    vkMapMemory(m_device.device(), m_staging_buff_mem, 0, m_staging_buff_size, 0, &data);
    memcpy(data, m_particles.data(), static_cast<size_t>(m_staging_buff_size));
    vkUnmapMemory(m_device.device(), m_staging_buff_mem);
}
FrgParticleDispenser::~FrgParticleDispenser() {
    vkDestroyBuffer(m_device.device(), m_staging_buff, nullptr);
    vkFreeMemory(m_device.device(), m_staging_buff_mem, nullptr);
}
void FrgParticleDispenser::cpy_host2dev(std::vector<VkBuffer> &buffers, std::vector<VkDeviceMemory> &buffers_memory) {
    assert(buffers.size() > 0 && "Shader buffer array has to have at least one buffer!");
    assert(buffers.size() == buffers_memory.size() && "Shader buffer and shader buffer memory sizes have to be equal!");

    for (size_t i = 0; i < buffers.size(); ++i) {
        m_device.createBuffer(
            m_staging_buff_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffers[i],
            buffers_memory[i]
        );
        m_device.copyBuffer(m_staging_buff, buffers[i], m_staging_buff_size);
    }
}
} // namespace frg
