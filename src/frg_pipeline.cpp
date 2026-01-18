#include "frg_pipeline.hpp"

#include "frg_model.hpp"
// std
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace frg {
FrgPipeline::FrgPipeline(
    FrgDevice &device, const std::string &vertFilePath, const std::string &fragFilePath,
    const PipelineConfigInfo &configInfo
)
    : frgDevice(device) {
  auto attr = Vertex::get_attribute_descriptions();
    std::vector<VkVertexInputAttributeDescription> inp_attr(attr.begin(), attr.end());
    createGraphicsPipeline(vertFilePath, fragFilePath, configInfo, Vertex::get_binding_descriptions(), inp_attr);
}

FrgPipeline::FrgPipeline(
    FrgDevice &device, const std::string &vertFilePath, const std::string &fragFilePath,
    const std::string &compFilePath, const PipelineConfigInfo &configInfo,
    std::vector<VkDescriptorSetLayout> &desc_set_layouts
)
    : frgDevice{device} {
    std::vector<VkVertexInputBindingDescription> desc = {Particle::getBindingDescription()};
  auto attr = Particle::getAttributeDescriptions();
    std::vector<VkVertexInputAttributeDescription> inp_attr(attr.begin(), attr.end());
    createGraphicsPipeline(vertFilePath, fragFilePath, configInfo, desc, inp_attr);
  createComputePipeline(compFilePath, desc_set_layouts);
  create_shader_storage_buffers();
}

FrgPipeline::~FrgPipeline() {
  vkDestroyShaderModule(frgDevice.device(), vertShaderModule, nullptr);
  vkDestroyShaderModule(frgDevice.device(), fragShaderModule, nullptr);
  if (compShaderModule != VK_NULL_HANDLE)
    vkDestroyShaderModule(frgDevice.device(), compShaderModule, nullptr);
  vkDestroyPipeline(frgDevice.device(), graphicsPipeline, nullptr);
  if (computePipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(frgDevice.device(), computePipeline, nullptr);
  if (computePipelineLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(frgDevice.device(), computePipelineLayout, nullptr);
  for (size_t i = 0; i < shader_storage_buffers.size(); ++i) {
    vkDestroyBuffer(frgDevice.device(), shader_storage_buffers[i], nullptr);
    vkFreeMemory(frgDevice.device(), shader_storage_buffers_memory[i], nullptr);
  }
}

std::vector<char> FrgPipeline::readFile(const std::string &filePath) {
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file: " + filePath);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

void FrgPipeline::createComputePipeline(const std::string &compFilePath, std::vector<VkDescriptorSetLayout> &layouts) {
  auto compCode = readFile(compFilePath);
  createShaderModule(compCode, &compShaderModule);

  VkPipelineShaderStageCreateInfo comp_shader_stage_create_info{};
    comp_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  comp_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  comp_shader_stage_create_info.module = compShaderModule;
  comp_shader_stage_create_info.pName = "main";

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(frgDevice.device(), &pipeline_layout_info, nullptr, &computePipelineLayout) !=
        VK_SUCCESS)
    {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  VkComputePipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.layout = computePipelineLayout;
  pipeline_info.stage = comp_shader_stage_create_info;
    if (vkCreateComputePipelines(frgDevice.device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &computePipeline) !=
        VK_SUCCESS)
    {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

void FrgPipeline::createGraphicsPipeline(
    const std::string &vertFilePath, const std::string &fragFilePath, const PipelineConfigInfo &configInfo,
    std::vector<VkVertexInputBindingDescription> input_binding_desc,
    std::vector<VkVertexInputAttributeDescription> attr_desc
) {

    assert(
        configInfo.pipelineLayout != VK_NULL_HANDLE &&
         "Cannot create graphics pipeline: no pipeline layout provided in "
        "configInfo"
    );
    assert(
        configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline: no render pass provided in "
                                                   "configInfo"
    );

  auto vertCode = readFile(vertFilePath);
  auto fragCode = readFile(fragFilePath);

  createShaderModule(vertCode, &vertShaderModule);
  createShaderModule(fragCode, &fragShaderModule);

  VkPipelineShaderStageCreateInfo shaderStages[2];
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule;
  shaderStages[0].pName = "main";
  shaderStages[0].flags = 0;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].pSpecializationInfo = nullptr;

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule;
  shaderStages[1].pName = "main";
  shaderStages[1].flags = 0;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].pSpecializationInfo = nullptr;

  // Use custom vertex input if provided, otherwise use default Vertex class
  std::vector<VkVertexInputBindingDescription> bindingDescriptions;
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

  if (configInfo.bindingDescriptions.empty() &&
      configInfo.attributeDescriptions.empty()) {
    bindingDescriptions = input_binding_desc;
    attributeDescriptions = attr_desc;
  } else {
    bindingDescriptions = configInfo.bindingDescriptions;
    attributeDescriptions = configInfo.attributeDescriptions;
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindingDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
  pipelineInfo.pViewportState = &configInfo.viewportInfo;
  pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
  pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
  pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
  pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
  pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

  pipelineInfo.layout = configInfo.pipelineLayout;
  pipelineInfo.renderPass = configInfo.renderPass;
  pipelineInfo.subpass = configInfo.subpass;

  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(frgDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
        VK_SUCCESS)
    {
    throw std::runtime_error("failed to create graphics pipeline!");
  }
}

void FrgPipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(frgDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
}

void FrgPipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void FrgPipeline::bindCompute(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
}

void FrgPipeline::defaultPipelineConfigInfo(PipelineConfigInfo &configInfo, bool compute) {
    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  configInfo.inputAssemblyInfo.topology =
        compute ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  configInfo.viewportInfo.viewportCount = 1;
  configInfo.viewportInfo.scissorCount = 1;
  configInfo.viewportInfo.pViewports = nullptr;
  configInfo.viewportInfo.pScissors = nullptr;

    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
  configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  configInfo.rasterizationInfo.lineWidth = 1.0f;
  configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
  configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
  configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
  configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
  configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
  configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
  configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
  configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

  configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
  configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
  configInfo.colorBlendInfo.attachmentCount = 1;
  configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
  configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
  configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
  configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
  configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
  configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
  configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
  configInfo.depthStencilInfo.front = {}; // Optional
  configInfo.depthStencilInfo.back = {};  // Optional

    configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
  configInfo.dynamicStateInfo.flags = 0;
}
void FrgPipeline::create_shader_storage_buffers() {
  shader_storage_buffers.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
  shader_storage_buffers_memory.resize(FrgSwapChain::MAX_FRAMES_IN_FLIGHT);
}
} // namespace frg