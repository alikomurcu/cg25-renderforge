#include "frg_particle_dispenser.hpp"

namespace frg {
FrgParticleDispenser::FrgParticleDispenser(
    FrgDevice &device, uint32_t particle_count, int h, int w, glm::vec4 position, int angle_of_domain, int angle_var
)
    : m_device{device}, m_particle_count{particle_count} {

    transform.translation = glm::vec3(position.x, position.y, position.z);
    m_particles.resize(m_particle_count);
    m_rand_eng = std::default_random_engine((unsigned)time(nullptr));
    m_dist01 = std::uniform_real_distribution<float>(0.0f, 1.0f);
    m_dist11 = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    angle_of_domain = 1.57;
    for (auto &particle : m_particles) {
        // alfa -> z
        float alfa = (angle_of_domain / 2.0f) * m_dist11(m_rand_eng);
        // gamma -> x
        float gamma = (angle_of_domain / 2.0f) * m_dist11(m_rand_eng);
        // Source:
        // https://en.wikipedia.org/wiki/Rotation_matrix
        glm::mat3 rot_m_x = {
            {1,           0,            0},
            {0, cosf(gamma), -sinf(gamma)},
            {0, sinf(gamma),  cosf(gamma)}
        };

        glm::mat3 rot_m_z = {
            {cosf(alfa), -sinf(alfa), 0},
            {sinf(alfa),  cosf(alfa), 0},
            {         0,           0, 1}
        };

        glm::vec3 up_vector = {0.0, -1.0, 0.0};

        particle.pos = generate_point_in_sphere(position); // generate_point_in_sphere(position);
        particle.vel = {m_dist01(m_rand_eng) * 0.1f, 0, 0, 0};
        // y down in vulkan
        particle.dir = {glm::vec3(rot_m_z * rot_m_x * up_vector), 0};
        int default_ttl = 30000;
        int ttl = std::max(300, static_cast<int>(m_dist01(m_rand_eng) * default_ttl));
        particle.flags = {ttl, 0, 0, ttl};
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
} // namespace frg
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
glm::vec4 FrgParticleDispenser::generate_point_in_sphere(glm::vec4 w_sphere_origin) {
    glm::vec4 pos{};
    do {
        pos = glm::vec4(m_dist11(m_rand_eng), -1 * m_dist11(m_rand_eng), m_dist11(m_rand_eng), 0);
    } while (glm::length(pos) > 1);

    pos /=100.f;
    return pos + w_sphere_origin;
}
VkVertexInputBindingDescription Particle::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, 2> Particle::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attr_desc{};
    attr_desc[0].binding = 0;
    attr_desc[0].location = 0;
    attr_desc[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attr_desc[0].offset = offsetof(Particle, pos);

    attr_desc[1].binding = 0;
    attr_desc[1].location = 1;
    attr_desc[1].format = VK_FORMAT_R32G32B32A32_SINT;
    attr_desc[1].offset = offsetof(Particle, flags);

    return attr_desc;
}
} // namespace frg
