#pragma once

#include "frg_device.hpp"
#include "frg_swap_chain.hpp"

// std
#include <string>
#include <vector>

namespace frg {
struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo &) = delete;
    PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

    // Vertex input (optional - if empty, uses default Vertex class)
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    // Multiple color blend attachments (for MRT)
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};

    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class FrgPipeline {
  public:
    FrgPipeline(
        FrgDevice &device, const std::string &vertFilePath, const std::string &fragFilePath,
        const PipelineConfigInfo &configInfo
    );

    FrgPipeline(
        FrgDevice &device, const std::string &vertFilePath, const std::string &fragFilePath,
        const std::string &compFilePath, const PipelineConfigInfo &configInfo,
        std::vector<VkDescriptorSetLayout> &desc_set_layouts
    );

    ~FrgPipeline();

    // Delete copy constructor and copy assignment operator
    FrgPipeline(const FrgPipeline &) = delete;
    FrgPipeline &operator=(const FrgPipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void bindCompute(VkCommandBuffer commandBuffer);
    static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo, bool compute = false);
    void create_shader_storage_buffers();
    VkPipelineLayout getComputePipelineLayout() { return computePipelineLayout; }
    VkPipeline getComputePipeline() { return computePipeline; }
    VkPipeline getGraphicsPipeline() { return graphicsPipeline; }
    std::vector<VkBuffer> &getShaderStorageBuffers() { return shader_storage_buffers; }
    std::vector<VkDeviceMemory> &getShaderStorageBuffersMemory() { return shader_storage_buffers_memory; }

  private:
    static std::vector<char> readFile(const std::string &filePath);
    void createComputePipeline(const std::string &compFilePath, std::vector<VkDescriptorSetLayout> &layouts);
    void createGraphicsPipeline(
        const std::string &vertFilePath, const std::string &fragFilePath, const PipelineConfigInfo &configInfo,
        std::vector<VkVertexInputBindingDescription> input_binding_desc,
        std::vector<VkVertexInputAttributeDescription> attr_desc
    );

    void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    FrgDevice &frgDevice;
    VkPipeline graphicsPipeline;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkShaderModule compShaderModule = VK_NULL_HANDLE;
    std::vector<VkBuffer> shader_storage_buffers;
    std::vector<VkDeviceMemory> shader_storage_buffers_memory;
};
} // namespace frg