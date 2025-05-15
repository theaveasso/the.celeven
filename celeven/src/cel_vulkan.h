#pragma once

#include "cel.h"
#include "cel_log.h"

#include <stdlib.h>

// clang-format off
#include <volk.h>
#include <vk_mem_alloc.h>
// clang-format on

const char *vk_result_string(VkResult result);

#define VK_CHECK(x)                                               \
    do {                                                          \
        VkResult err = x;                                         \
        if (err)                                                  \
        {                                                         \
            CEL_ERROR("vulkan error: %s", vk_result_string(err)); \
            abort();                                              \
        }                                                         \
    } while (0)

#define ASSERT_VK_HANDLE(handle)         \
    do {                                 \
        if ((handle) == VK_NULL_HANDLE)  \
        {                                \
            CEL_ERROR("handle is NULL"); \
            abort();                     \
        }                                \
    } while (0)

struct GLFWwindow;

CEL_HANDLE_DEFINE(buffer_handle);
CEL_HANDLE_DEFINE(image_handle);
CEL_HANDLE_DEFINE(sampler_handle);
CEL_HANDLE_DEFINE(program_handle);

typedef struct CELvk_state CELvk_state;
struct CELvk_state {
    const char *app_name;
    const char *engine_path;
};

typedef struct CELrgba CELrgba;
struct CELrgba {
    float r;
    float g;
    float b;
    float a;
};

typedef struct CELvk_frame_data CELvk_frame_data;
struct CELvk_frame_data {
    VkFence render_fence;
    VkSemaphore swapchain_semaphore;
    VkSemaphore render_semaphore;
    VkCommandBuffer primary_command_buffer;
    VkCommandPool primary_command_pool;
};

typedef struct CELvk_buffer CELvk_buffer;
struct CELvk_buffer {
    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
};

typedef struct CELvk_image_create_info CELvk_image_create_info;
struct CELvk_image_create_info {
    VkFormat format;
    VkImageUsageFlags usages;
    VkImageCreateFlags flags;
    VkExtent3D extent;
    uint32_t base_array_layers;
};

typedef struct CELvk_image CELvk_image;
struct CELvk_image {
    VmaAllocation allocation;

    VkImage handle;
    VkImageView image_view;
    bool own_image;
};

typedef struct CELvk_sampler CELvk_sampler;
struct CELvk_sampler {
    VkSampler handle;
};

typedef struct CELvk_program CELvk_program;
struct CELvk_program {
    VkPipeline pipeline;

    VkPipelineBindPoint bind_point;
    VkPipelineLayout layout;
    VkDescriptorSetLayout set_layout;

    VkShaderModule *shader_modules;
    VkShaderStageFlags *shader_stages;
    uint32_t stage_count;
};

typedef struct CELsprite_renderer_pc CELsprite_renderer_pc;
struct CELsprite_renderer_pc {
    VkDeviceAddress buffer_device_address;
};

CELAPI CELbuffer_handle celvk_staging_buffer_create(VmaAllocator *allocator, VkDeviceSize size, VkBufferUsageFlags usages);
CELAPI CELbuffer_handle celvk_gpu_buffer_create(VmaAllocator *allocator, VkDeviceSize size, VkBufferUsageFlags usages);
CELAPI void celvk_buffer_destroy(VmaAllocator *allocator, const CELbuffer_handle *handle);

CELAPI CELimage_handle celvk_image_create(const CELvk_image_create_info *create_info, const VmaAllocationCreateInfo *allocation_info);
CELAPI CELimage_handle celvk_image_create_w_handle(VkDevice *device, VmaAllocator *allocator, const CELvk_image_create_info *create_info, VkImage image);
CELAPI void celvk_image_destroy(VkDevice *device, VmaAllocator *allocator, const CELimage_handle *image);

CELAPI CELsampler_handle celvk_sampler_create(VkDevice *device, const VkSamplerCreateInfo *create_info);
CELAPI void celvk_sampler_destroy(VkDevice *device, const CELsampler_handle *handle);

CELAPI uint32_t *celvk_load_shader_w_spv(const char *path, size_t *size);

CELAPI VkPipeline celvk_graphics_pipeline_create(VkDevice *device, const VkPipelineRenderingCreateInfo *rendering_create_info, const CELprogram_handle *program);
CELAPI void celvk_pipeline_destroy(VkDevice *device, VkPipeline *pipeline);
CELAPI CELprogram_handle celvk_sprite_renderer_create(VkFormat format);

CELAPI CELprogram_handle celvk_program_create(VkDevice *device, VkPipelineBindPoint bind_point, size_t push_constant_size, const char **shader_paths, uint32_t shader_count);
CELAPI void celvk_program_destroy(VkDevice *device, const CELprogram_handle *handle);

bool cel_vulkan_init(struct GLFWwindow *window, CELvk_state *state);
void cel_vulkan_fini();

CELAPI VkCommandBuffer celvk_begin_draw();
CELAPI void celvk_draw();
CELAPI void celvk_end_draw(VkCommandBuffer cmd, CELimage_handle render_texture_handle);

CELAPI void celvk_clear_background(VkCommandBuffer cmd, const CELimage_handle *handle, CELrgba color);
CELAPI void celvk_transition_image(VkCommandBuffer cmd, const CELimage_handle *handle, VkImageLayout old_layout, VkImageLayout new_layout);
