#include "cel_vulkan.h"

#include "cel_define.h"

#include <assert.h>
#include <string.h>

#include <GLFW/glfw3.h>

#if defined(CEL_DEBUG) || defined(CELVK_VALIDATION_LAYERS)
    #define CELVK_USE_VALIDATION_LAYERS
#endif

#if defined(CELVK_USE_VALIDATION_LAYERS) && (defined(CELVK_VALIDATION_LAYERS_GPU_ASSISTED) || defined(CELVK_VALIDATION_LAYERS_BEST_PRACTICES) || defined(CELVK_VALIDATION_LAYERS_SYNCHRONIZATION))
    #define CELVK_USE_VALIDATION_LAYERS_FEATURES
#endif

#define CELVK_MAX_FRAME_OVERLAP 3

#define CELVK_TEXTURE_BINDING 0
#define CELVK_SAMPLER_BINDING 1

#define CELVK_STORAGE_SIZE (64 * 1024 * 1024)

#define CELVK_MAX_BINDLESS_RESOURCE_COUNT 16536
#define CELVK_MAX_BUFFER_COUNT 1024
#define CELVK_MAX_SAMPLER_COUNT 32
#define CELVK_MAX_IMAGE_COUNT 1024
#define CELVK_MAX_PROGRAM_COUNT 256

#define CELVK_MAX_EXTENSION_COUNT 32
#define CELVK_MAX_LAYER_COUNT 32

typedef struct CELvk_physical_device CELvk_physical_device;
struct CELvk_physical_device {
    VkPhysicalDevice handle;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkQueueFamilyProperties *queue_family_properties;
    uint32_t queue_family_property_count;
    VkExtensionProperties *extension_properties;
    uint32_t extension_property_count;
};

typedef struct CELvk_device CELvk_device;
struct CELvk_device {
    VkDevice handle;
    uint32_t graphics_queue_family_index;
    uint32_t graphics_queue_mode;
    VkQueue graphics_queue;
    VkQueue present_queue;
};

typedef struct CELvk_surface CELvk_surface;
struct CELvk_surface {
    VkSurfaceKHR handle;
    VkBool32 surface_supported;
};

typedef struct CELvk_swapchain CELvk_swapchain;
struct CELvk_swapchain {
    VkSwapchainKHR handle;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkExtent2D swapchain_extent;
    uint32_t image_array_layers;
    CELimage_handle *swapchain_images;
    bool dirty;
};

typedef struct CELvk_bindless_descriptor CELvk_bindless_descriptor;
struct CELvk_bindless_descriptor {
    VkDescriptorPool pool;
    VkDescriptorSet set;
    VkDescriptorSetLayout set_layout;
    CELsampler_handle nearest_sampler;
    CELsampler_handle linear_sampler;
    CELsampler_handle shadow_map_sampler;
};

typedef struct CELvk_immediate_command CELvk_immediate_command;
struct CELvk_immediate_command {
    VkCommandBuffer command_buffer;
    VkCommandPool command_pool;
    VkFence fence;
};

typedef struct CELvk_ctx CELvk_ctx;
struct CELvk_ctx {
    VkInstance instance;
    CELvk_physical_device physical_device;
    CELvk_device device;
    VmaAllocator allocator;
    CELvk_surface surface;
    CELvk_swapchain swapchain;
#if defined(CELVK_USE_VALIDATION_LAYERS)
    VkDebugUtilsMessengerEXT debug_utils_messenger;
#endif
    CELvk_frame_data *frames;
    CELvk_immediate_command immediate_command;
    CELvk_bindless_descriptor descriptor;

    size_t frame_count;

    const char *engine_path;

    bool raytracing_supported;
    bool mesh_shading_supported;
};

GlobalVariable CELvk_ctx vk_ctx = {};

GlobalVariable unsigned char vkbuf[CELVK_STORAGE_SIZE];
GlobalVariable CELarena vk_arena;

GlobalVariable const char *enabled_extensions[32];
GlobalVariable uint32_t enabled_extension_count = 0;

GlobalVariable CELvk_buffer vk_buffers[CELVK_MAX_BUFFER_COUNT];
GlobalVariable uint32_t vk_buffer_count = 0;

GlobalVariable CELvk_image vk_images[CELVK_MAX_IMAGE_COUNT];
GlobalVariable uint32_t vk_image_count = 0;

GlobalVariable CELvk_sampler vk_samplers[CELVK_MAX_SAMPLER_COUNT];
GlobalVariable uint32_t vk_sampler_count = 0;

GlobalVariable CELvk_program vk_programs[CELVK_MAX_PROGRAM_COUNT];
GlobalVariable uint32_t vk_program_count = 0;

Internal bool celvk_enable_extension(const char *req_ext, VkExtensionProperties *available_exts, uint32_t available_exts_count, const char **enabled_exts, uint32_t *enabled_exts_count);
Internal bool celvk_enable_layer(const char *req_layer, VkLayerProperties *supported_layers, uint32_t supported_layer_count, const char **enabled_layers, uint32_t *enabled_layer_count);

Internal VkInstance instance_create();
Internal void instance_destroy(VkInstance *instance);

Internal VmaAllocator allocator_create(VkInstance *instance, VkPhysicalDevice *physical_device, VkDevice *device);
Internal void allocator_destroy(VmaAllocator *allocator);

Internal uint32_t physical_device_count_get(VkInstance *instance);
Internal VkPhysicalDevice physical_device_get(VkInstance *instance);
Internal void physical_device_destroy(VkInstance *instance, VkPhysicalDevice *physical_device);

Internal uint32_t queue_family_count_get(VkPhysicalDevice *physical_device);
Internal VkQueueFamilyProperties *queue_family_properties_get(VkPhysicalDevice *physical_device, uint32_t queue_family_count);
Internal uint32_t graphics_queue_family_index_get(VkQueueFamilyProperties *queue_family_properties, uint32_t queue_family_count);
Internal uint32_t graphics_queue_mode_get(VkQueueFamilyProperties *queue_family_properties, uint32_t graphics_queue_family_index);
Internal VkQueue graphics_queue_get(VkDevice *device, uint32_t graphics_queue_family_index);
Internal VkQueue present_queue_get(VkDevice *device, uint32_t graphics_queue_family_index, uint32_t graphics_queue_mode);

Internal VkDevice device_create(VkPhysicalDevice *physical_device, uint32_t queue_family_count, VkQueueFamilyProperties *queue_family_properties);
Internal void device_destroy(VkDevice *device);

Internal VkSurfaceKHR window_surface_create(GLFWwindow *window, VkInstance *instance);
Internal void window_surface_destroy(VkSurfaceKHR *surface, VkInstance *instance);
Internal VkBool32 window_surface_support_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device, uint32_t graphics_queue_family_index);

Internal VkSurfaceCapabilitiesKHR window_surface_capabilities_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device);
Internal VkSurfaceFormatKHR prefer_surface_format_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device);
Internal VkPresentModeKHR prefer_present_mode_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device);
Internal VkExtent2D prefer_swapchain_extent_get(VkSurfaceCapabilitiesKHR *surface_capabilities, GLFWwindow *window);

Internal VkSwapchainKHR swapchain_create(VkDevice *device, VkSurfaceKHR *surface, VkSurfaceCapabilitiesKHR *surface_capabilities, VkSurfaceFormatKHR *surface_format, VkExtent2D *swapchain_extent, const VkPresentModeKHR *present_mode, uint32_t image_array_layers, uint32_t graphics_queue_index);
Internal void swapchain_destroy(VkDevice *device, CELvk_swapchain *swapchain);
Internal uint32_t swapchain_image_count_get(VkDevice *device, VkSwapchainKHR *swapchain);
Internal VkImage *swapchain_images_get(VkDevice *device, VkSwapchainKHR *swapchain, uint32_t swapchain_image_count);
Internal CELimage_handle swapchain_acquire_next_image(VkDevice *device, uint32_t current_frame_index, uint32_t *image_index);
Internal void submit_and_present(VkCommandBuffer cmd, uint32_t current_frame_index, uint32_t image_index);

Internal CELvk_frame_data *perframes_create(VkDevice *device, uint32_t queue_family_index);
Internal void perframes_destroy(VkDevice *device, CELvk_frame_data *frame_data);

Internal CELvk_immediate_command immediate_command_create(VkDevice *device, uint32_t queue_family_index);
Internal void immediate_command_destroy(VkDevice *device, CELvk_immediate_command *im_cmd);

Internal CELvk_bindless_descriptor bindless_descriptor_create(VkDevice *device);
Internal void bindless_descriptor_destroy(VkDevice *device, CELvk_bindless_descriptor *descriptor);

Internal void vk_images_destroy(VkDevice *device, VmaAllocator *allocator);
Internal void vk_buffers_destroy(VmaAllocator *allocator);
Internal void vk_samplers_destroy(VkDevice *device);

#if defined(CELVK_USE_VALIDATION_LAYERS)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);
#endif

bool cel_vulkan_init(GLFWwindow *window, CELvk_state *state) {
    vk_ctx.engine_path = state->engine_path;

    cel_arena_init(&vk_arena, vkbuf, CELVK_STORAGE_SIZE);

    VK_CHECK(volkInitialize());

    uint32_t available_ext_count;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &available_ext_count, NULL));

    VkExtensionProperties *available_exts = cel_arena_alloc(&vk_arena, sizeof(VkExtensionProperties) * available_ext_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &available_ext_count, available_exts));

#if defined(CELVK_USE_VALIDATION_LAYERS)
    const bool has_debug_utils = celvk_enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, available_exts, available_ext_count, enabled_extensions, &enabled_extension_count);
#endif

#if defined(CELVK_USE_VALIDATION_LAYERS_FEATURES)
#endif

    celvk_enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_exts, available_ext_count, enabled_extensions, &enabled_extension_count);

    uint32_t glfw_ext_count = 0;
    const char **glfw_exts  = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    if (!glfw_exts)
    {
        CEL_ERROR("glfw required vulkan extensions not available");
        return false;
    }

    for (uint32_t i = 0; i < glfw_ext_count; ++i)
    {
        bool added = celvk_enable_extension(glfw_exts[i], available_exts, available_ext_count, enabled_extensions, &enabled_extension_count);
        if (!added)
        {
            CEL_ERROR("glfw required extension %s is not available", glfw_exts[i]);
            return false;
        }
    }

    uint32_t supported_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL));

    VkLayerProperties *supported_layer = cel_arena_alloc(&vk_arena, sizeof(VkLayerProperties) * supported_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layer));

    const char *enabled_layers[CELVK_MAX_LAYER_COUNT];
    uint32_t enable_layer_count = 0;
#if defined(CELVK_USE_VALIDATION_LAYERS)
    celvk_enable_layer("VK_LAYER_KHRONOS_validation", supported_layer, supported_layer_count, enabled_layers, &enable_layer_count);
#endif

    VkApplicationInfo app_info  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName   = state->app_name;
    app_info.applicationVersion = 0;
    app_info.pEngineName        = "CEL Engine";
    app_info.engineVersion      = 0;
    app_info.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info    = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo        = &app_info;
    instance_info.enabledExtensionCount   = enabled_extension_count;
    instance_info.ppEnabledExtensionNames = enabled_extensions;
    instance_info.enabledLayerCount       = enable_layer_count;
    instance_info.ppEnabledLayerNames     = enabled_layers;

#if defined(CLVK_USE_VALIDATION_LAYERS)
#endif
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    if (has_debug_utils)
    {
        debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

        instance_info.pNext = &debug_utils_create_info;
    }

    VkResult result = vkCreateInstance(&instance_info, NULL, &vk_ctx.instance);
    if (result != VK_SUCCESS)
    {
        CEL_FATAL("vulkan error: failed to create vulkan instance");
        abort();
    }

    volkLoadInstanceOnly(vk_ctx.instance);

#if defined(CELVK_USE_VALIDATION_LAYERS)
    if (has_debug_utils)
    {
        result = vkCreateDebugUtilsMessengerEXT(vk_ctx.instance, &debug_utils_create_info, NULL, &vk_ctx.debug_utils_messenger);
        if (result != VK_SUCCESS)
        {
            CEL_ERROR("vulkan error: failed to create vulkan debug utils messenger");
            return false;
        }
    }
#endif

    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vk_ctx.instance, &physical_device_count, NULL));

    if (physical_device_count < 1)
    {
        CEL_ERROR("vulkan error: failed to find a physical device that supports vulkan");
        return false;
    }

    VkPhysicalDevice *physical_devices = arena_alloc(&vk_arena, sizeof(VkPhysicalDevice) * physical_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vk_ctx.instance, &physical_device_count, physical_devices));

    assert(physical_devices != NULL && "vulkan error: no physical devices were found on the system");

    vk_ctx.physical_device.handle = physical_devices[0];
    vkGetPhysicalDeviceProperties(vk_ctx.physical_device.handle, &vk_ctx.physical_device.properties);
    vkGetPhysicalDeviceFeatures(vk_ctx.physical_device.handle, &vk_ctx.physical_device.features);

    vkGetPhysicalDeviceQueueFamilyProperties(vk_ctx.physical_device.handle, &vk_ctx.physical_device.queue_family_property_count, NULL);
    vk_ctx.physical_device.queue_family_properties = cel_arena_alloc(&vk_arena, sizeof(VkQueueFamilyProperties) * vk_ctx.physical_device.queue_family_property_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_ctx.physical_device.handle, &vk_ctx.physical_device.queue_family_property_count, vk_ctx.physical_device.queue_family_properties);

    vkEnumerateDeviceExtensionProperties(vk_ctx.physical_device.handle, NULL, &vk_ctx.physical_device.extension_property_count, NULL);
    vk_ctx.physical_device.extension_properties = cel_arena_alloc(&vk_arena, sizeof(VkExtensionProperties) * vk_ctx.physical_device.extension_property_count);
    vkEnumerateDeviceExtensionProperties(vk_ctx.physical_device.handle, NULL, &vk_ctx.physical_device.extension_property_count, vk_ctx.physical_device.extension_properties);

    CEL_INFO("selected GPU: %s ", vk_ctx.physical_device.properties.deviceName);
    CELvk_physical_device *selected_physical_device = &vk_ctx.physical_device;

    uint32_t queue_family_count                      = queue_family_count_get(&selected_physical_device->handle);
    VkQueueFamilyProperties *queue_family_properties = queue_family_properties_get(&selected_physical_device->handle, queue_family_count);

    vk_ctx.device.handle = device_create(&selected_physical_device->handle, queue_family_count, queue_family_properties);
    ASSERT_VK_HANDLE(vk_ctx.device.handle);

    volkLoadDevice(vk_ctx.device.handle);
    vk_ctx.allocator = allocator_create(&vk_ctx.instance, &vk_ctx.physical_device.handle, &vk_ctx.device.handle);

    vk_ctx.device.graphics_queue_family_index = graphics_queue_family_index_get(queue_family_properties, queue_family_count);
    vk_ctx.device.graphics_queue_mode         = graphics_queue_mode_get(queue_family_properties, vk_ctx.device.graphics_queue_family_index);

    vk_ctx.device.graphics_queue = graphics_queue_get(&vk_ctx.device.handle, vk_ctx.device.graphics_queue_family_index);
    vk_ctx.device.present_queue  = present_queue_get(&vk_ctx.device.handle, vk_ctx.device.graphics_queue_family_index, vk_ctx.device.graphics_queue_mode);

    vk_ctx.surface.handle            = window_surface_create(window, &vk_ctx.instance);
    vk_ctx.surface.surface_supported = window_surface_support_get(&vk_ctx.surface.handle, &vk_ctx.physical_device.handle, vk_ctx.device.graphics_queue_family_index);
    if (!vk_ctx.surface.surface_supported) { return false; }

    vk_ctx.swapchain.surface_capabilities = window_surface_capabilities_get(&vk_ctx.surface.handle, &vk_ctx.physical_device.handle);
    vk_ctx.swapchain.surface_format       = prefer_surface_format_get(&vk_ctx.surface.handle, &vk_ctx.physical_device.handle);
    vk_ctx.swapchain.present_mode         = prefer_present_mode_get(&vk_ctx.surface.handle, &vk_ctx.physical_device.handle);
    vk_ctx.swapchain.swapchain_extent     = prefer_swapchain_extent_get(&vk_ctx.swapchain.surface_capabilities, window);
    vk_ctx.swapchain.image_array_layers   = 1;

    vk_ctx.swapchain.handle = swapchain_create(&vk_ctx.device.handle, &vk_ctx.surface.handle, &vk_ctx.swapchain.surface_capabilities, &vk_ctx.swapchain.surface_format, &vk_ctx.swapchain.swapchain_extent, &vk_ctx.swapchain.present_mode, vk_ctx.swapchain.image_array_layers, vk_ctx.device.graphics_queue_family_index);

    uint32_t swapchain_image_count = swapchain_image_count_get(&vk_ctx.device.handle, &vk_ctx.swapchain.handle);
    VkImage *swapchain_images      = swapchain_images_get(&vk_ctx.device.handle, &vk_ctx.swapchain.handle, swapchain_image_count);

    vk_ctx.swapchain.swapchain_images = cel_arena_alloc(&vk_arena, sizeof(CELvk_image) * swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; ++i)
    {
        CELvk_image_create_info create_info = {
            .format            = vk_ctx.swapchain.surface_format.format,
            .usages            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .base_array_layers = vk_ctx.swapchain.image_array_layers,
            .extent            = (VkExtent3D){vk_ctx.swapchain.swapchain_extent.width, vk_ctx.swapchain.swapchain_extent.height, 1},
        };
        CELimage_handle *image_handle = &vk_ctx.swapchain.swapchain_images[i];
        *image_handle                 = celvk_image_create_w_handle(&vk_ctx.device.handle, &vk_ctx.allocator, &create_info, swapchain_images[i]);
    }

    vk_ctx.frames            = perframes_create(&vk_ctx.device.handle, vk_ctx.device.graphics_queue_family_index);
    vk_ctx.immediate_command = immediate_command_create(&vk_ctx.device.handle, vk_ctx.device.graphics_queue_family_index);

    vk_ctx.descriptor                               = bindless_descriptor_create(&vk_ctx.device.handle);
    VkSamplerCreateInfo nearest_sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    nearest_sampler_create_info.magFilter           = VK_FILTER_NEAREST;
    nearest_sampler_create_info.minFilter           = VK_FILTER_NEAREST;

    VkSamplerCreateInfo linear_sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    nearest_sampler_create_info.magFilter          = VK_FILTER_LINEAR;
    nearest_sampler_create_info.minFilter          = VK_FILTER_LINEAR;

    VkSamplerCreateInfo shadow_sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    nearest_sampler_create_info.magFilter          = VK_FILTER_LINEAR;
    nearest_sampler_create_info.minFilter          = VK_FILTER_LINEAR;
    nearest_sampler_create_info.mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    nearest_sampler_create_info.anisotropyEnable   = VK_TRUE;
    nearest_sampler_create_info.maxAnisotropy      = 16;

    vk_ctx.descriptor.nearest_sampler    = celvk_sampler_create(&vk_ctx.device.handle, &nearest_sampler_create_info);
    vk_ctx.descriptor.linear_sampler     = celvk_sampler_create(&vk_ctx.device.handle, &linear_sampler_create_info);
    vk_ctx.descriptor.shadow_map_sampler = celvk_sampler_create(&vk_ctx.device.handle, &shadow_sampler_create_info);

    return true;
}

void cel_vulkan_fini() {
    vkDeviceWaitIdle(vk_ctx.device.handle);

    bindless_descriptor_destroy(&vk_ctx.device.handle, &vk_ctx.descriptor);
    perframes_destroy(&vk_ctx.device.handle, vk_ctx.frames);
    immediate_command_destroy(&vk_ctx.device.handle, &vk_ctx.immediate_command);

    vk_samplers_destroy(&vk_ctx.device.handle);
    vk_images_destroy(&vk_ctx.device.handle, &vk_ctx.allocator);

    allocator_destroy(&vk_ctx.allocator);

    swapchain_destroy(&vk_ctx.device.handle, &vk_ctx.swapchain);
    window_surface_destroy(&vk_ctx.surface.handle, &vk_ctx.instance);
    device_destroy(&vk_ctx.device.handle);

#if defined(CELVK_USE_VALIDATION_LAYERS)
    ASSERT_VK_HANDLE(vk_ctx.debug_utils_messenger);
    vkDestroyDebugUtilsMessengerEXT(vk_ctx.instance, vk_ctx.debug_utils_messenger, NULL);
#endif

    vkDestroyInstance(vk_ctx.instance, NULL);
    volkFinalize();
    cel_arena_free_all(&vk_arena);
}

VkCommandBuffer celvk_begin_draw() {
    CELvk_frame_data *frame = &vk_ctx.frames[vk_ctx.frame_count % CELVK_MAX_FRAME_OVERLAP];

    VK_CHECK(vkWaitForFences(vk_ctx.device.handle, 1, &frame->render_fence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(vk_ctx.device.handle, 1, &frame->render_fence));

    VK_CHECK(vkResetCommandBuffer(frame->primary_command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(frame->primary_command_buffer, &begin_info));

    return frame->primary_command_buffer;
}

void celvk_draw() {
}

void celvk_end_draw(VkCommandBuffer cmd, CELimage_handle render_texture_handle) {
    uint32_t image_index;
    uint32_t current_frame_index = vk_ctx.frame_count % CELVK_MAX_FRAME_OVERLAP;

    CELimage_handle swapchain_image = swapchain_acquire_next_image(&vk_ctx.device.handle, current_frame_index, &image_index);

    celvk_clear_background(cmd, &swapchain_image, (CELrgba){1.0f, 0.0f, 1.0f, 1.0f});
    celvk_transition_image(cmd, &swapchain_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    submit_and_present(cmd, current_frame_index, image_index);
    vk_ctx.frame_count++;
}

void celvk_clear_background(VkCommandBuffer cmd, const CELimage_handle *handle, CELrgba color) {
    VkClearColorValue clearColorValue  = {{color.r, color.g, color.b, color.a}};
    VkImageSubresourceRange clearRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .layerCount     = 1,
        .baseArrayLayer = 0,
        .levelCount     = 1,
    };

    CELvk_image *image = &vk_images[handle->idx];
    celvk_transition_image(cmd, handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vkCmdClearColorImage(cmd, image->handle, VK_IMAGE_LAYOUT_GENERAL, &clearColorValue, 1, &clearRange);
    celvk_transition_image(cmd, handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void celvk_transition_image(VkCommandBuffer cmd, const CELimage_handle *handle, VkImageLayout old_layout, VkImageLayout new_layout) {
    CELvk_image *image = &vk_images[handle->idx];

    VkImageAspectFlags aspect_flags           = (old_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || new_layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageSubresourceRange subresource_range = {
        .aspectMask     = aspect_flags,
        .baseMipLevel   = 0,
        .layerCount     = 1,
        .baseArrayLayer = 0,
        .levelCount     = 1,
    };

    VkImageMemoryBarrier2 barrier2 = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier2.srcStageMask          = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier2.srcAccessMask         = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier2.dstStageMask          = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier2.dstAccessMask         = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier2.oldLayout             = old_layout;
    barrier2.newLayout             = new_layout;
    barrier2.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image                 = image->handle;
    barrier2.subresourceRange      = subresource_range;

    VkDependencyInfo dependency_info        = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers    = &barrier2;

    vkCmdPipelineBarrier2(cmd, &dependency_info);
}

VmaAllocator allocator_create(VkInstance *instance, VkPhysicalDevice *physical_device, VkDevice *device) {
    VmaAllocator allocator;

    VmaVulkanFunctions vk_functions = {
        .vkGetInstanceProcAddr               = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                 = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                    = vkAllocateMemory,
        .vkFreeMemory                        = vkFreeMemory,
        .vkMapMemory                         = vkMapMemory,
        .vkUnmapMemory                       = vkUnmapMemory,
        .vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                  = vkBindBufferMemory,
        .vkBindImageMemory                   = vkBindImageMemory,
        .vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements,
        .vkCreateBuffer                      = vkCreateBuffer,
        .vkDestroyBuffer                     = vkDestroyBuffer,
        .vkCreateImage                       = vkCreateImage,
        .vkDestroyImage                      = vkDestroyImage,
        .vkCmdCopyBuffer                     = vkCmdCopyBuffer,
    };

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocator_create_info.instance               = *instance;
    allocator_create_info.physicalDevice         = *physical_device;
    allocator_create_info.device                 = *device;
    allocator_create_info.pVulkanFunctions       = &vk_functions;
    allocator_create_info.pAllocationCallbacks   = NULL;

    vmaCreateAllocator(&allocator_create_info, &allocator);
    return allocator;
}

void allocator_destroy(VmaAllocator *allocator) {
    vmaDestroyAllocator(*allocator);
}

bool celvk_enable_extension(const char *req_ext, VkExtensionProperties *available_exts, uint32_t available_ext_count, const char **enabled_exts, uint32_t *enabled_exts_count) {
    bool is_available = false;
    for (uint32_t i = 0; i < available_ext_count; ++i)
    {
        if (strcmp(req_ext, available_exts[i].extensionName) == 0)
        {
            is_available = true;
            break;
        }
    }

    if (is_available)
    {
        bool is_already_enabled = false;
        for (uint32_t i = 0; i < *enabled_exts_count; ++i)
        {
            if (strcmp(req_ext, enabled_exts[i]) == 0)
            {
                is_already_enabled = true;
                break;
            }
        }

        if (!is_already_enabled)
        {
            CEL_INFO("vulkan extension %s available, enable ...", req_ext);
            if (*enabled_exts_count < CELVK_MAX_EXTENSION_COUNT)
            {
                enabled_exts[*enabled_exts_count] = req_ext;
                (*enabled_exts_count)++;
            }
            else
            {
                CEL_WARN("vulkan max enabled extensions reached, cannot add %s", req_ext);
            }
        }
    }
    else
    {
        CEL_INFO("vulkan extension %s not available", req_ext);
    }
    return is_available;
}

bool celvk_enable_layer(const char *req_layer, VkLayerProperties *supported_layers, uint32_t supported_layer_count, const char **enabled_layers, uint32_t *enabled_layer_count) {
    bool is_available = false;
    for (uint32_t i = 0; i < supported_layer_count; ++i)
    {
        if (strcmp(req_layer, supported_layers[i].layerName) == 0)
        {
            is_available = true;
            break;
        }
    }

    if (is_available)
    {
        bool is_already_enabled = false;
        for (uint32_t i = 0; i < *enabled_layer_count; ++i)
        {
            if (strcmp(req_layer, enabled_layers[i]) == 0)
            {
                is_already_enabled = true;
                break;
            }
        }

        if (!is_already_enabled)
        {
            CEL_INFO("vulkan layer %s available, enable ...", req_layer);
            if (*enabled_layer_count < CELVK_MAX_EXTENSION_COUNT)
            {
                enabled_layers[*enabled_layer_count] = req_layer;
                (*enabled_layer_count)++;
            }
            else
            {
                CEL_WARN("vulkan max enabled layers reached, cannot add %s", req_layer);
            }
        }
    }
    else
    {
        CEL_INFO("vulkan layer %s not available", req_layer);
    }
    return is_available;
}

uint32_t queue_family_count_get(VkPhysicalDevice *physical_device) {
    uint32_t queue_family_index = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*physical_device, &queue_family_index, NULL);
    return queue_family_index;
}

VkQueueFamilyProperties *queue_family_properties_get(VkPhysicalDevice *physical_device, uint32_t queue_family_count) {
    VkQueueFamilyProperties *queue_family_properties = cel_arena_alloc(&vk_arena, sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(*physical_device, &queue_family_count, queue_family_properties);
    return queue_family_properties;
}

uint32_t graphics_queue_family_index_get(VkQueueFamilyProperties *queue_family_properties, uint32_t queue_family_count) {
    uint32_t graphics_queue_family_count    = 0;
    uint32_t *graphics_queue_family_indices = cel_arena_alloc(&vk_arena, sizeof(uint32_t) * queue_family_count);

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            graphics_queue_family_indices[graphics_queue_family_count] = i;
            graphics_queue_family_count++;
        }
    }

    uint32_t best_graphics_queue_family_queue_count = 0;
    uint32_t best_graphics_queue_family_queue_index = 0;

    for (uint32_t i = 0; i < graphics_queue_family_count; ++i)
    {
        if (queue_family_properties[graphics_queue_family_indices[i]].queueCount > best_graphics_queue_family_queue_count)
        {
            best_graphics_queue_family_queue_count = queue_family_properties[graphics_queue_family_indices[i]].queueCount;
            best_graphics_queue_family_queue_index = i;
        }
    }

    return best_graphics_queue_family_queue_index;
}

uint32_t graphics_queue_mode_get(VkQueueFamilyProperties *queue_family_properties, uint32_t graphics_queue_family_index) {
    if (queue_family_properties[graphics_queue_family_index].queueCount == 1) { return 0; }
    else if (queue_family_properties[graphics_queue_family_index].queueCount > 1) { return 1; }
    else { return 2; }
}

VkQueue graphics_queue_get(VkDevice *device, uint32_t graphics_queue_family_index) {
    VkQueue graphics_queue = NULL;
    vkGetDeviceQueue(*device, graphics_queue_family_index, 0, &graphics_queue);
    return graphics_queue;
}

VkQueue present_queue_get(VkDevice *device, uint32_t graphics_queue_family_index, uint32_t graphics_queue_mode) {
    VkQueue present_queue = NULL;
    if (graphics_queue_mode == 0) { vkGetDeviceQueue(*device, graphics_queue_family_index, 0, &present_queue); }
    else if (graphics_queue_mode == 1) { vkGetDeviceQueue(*device, graphics_queue_family_index, 1, &present_queue); }
    return present_queue;
}

VkDevice device_create(VkPhysicalDevice *physical_device, uint32_t queue_family_count, VkQueueFamilyProperties *queue_family_properties) {
    VkDeviceQueueCreateInfo *device_queue_create_infos = cel_arena_alloc(&vk_arena, sizeof(VkDeviceQueueCreateInfo) * queue_family_count);
    float **queue_priorities                           = cel_arena_alloc(&vk_arena, sizeof(float *) * queue_family_count);

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        queue_priorities[i] = cel_arena_alloc(&vk_arena, sizeof(float) * queue_family_properties[i].queueCount);
        for (uint32_t j = 0; j < queue_family_properties[i].queueCount; ++j)
        {
            queue_priorities[i][j] = 1.0f;
        }

        device_queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[i].pNext            = NULL;
        device_queue_create_infos[i].flags            = 0;
        device_queue_create_infos[i].queueFamilyIndex = i;
        device_queue_create_infos[i].queueCount       = queue_family_properties[i].queueCount;
        device_queue_create_infos[i].pQueuePriorities = queue_priorities[i];
    }

    const char extension_list[][VK_MAX_EXTENSION_NAME_SIZE] = {"VK_KHR_swapchain"};
    const char *extensions[CELVK_MAX_EXTENSION_COUNT]       = {extension_list[0]};
    uint32_t extensions_count                               = 0;

    if (vk_ctx.raytracing_supported)
    {
        extensions[extensions_count++] = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME;
        extensions[extensions_count++] = VK_KHR_RAY_QUERY_EXTENSION_NAME;
        extensions[extensions_count++] = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
    }

    if (vk_ctx.mesh_shading_supported)
    {
        extensions[extensions_count++] = VK_EXT_MESH_SHADER_EXTENSION_NAME;
    }

    VkPhysicalDeviceFeatures physical_device_features;
    vkGetPhysicalDeviceFeatures(*physical_device, &physical_device_features);

    VkPhysicalDeviceVulkan13Features features_1_3 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features_1_3.dynamicRendering                 = true;
    features_1_3.synchronization2                 = true;

    VkPhysicalDeviceVulkan12Features features_1_2             = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features_1_2.pNext                                        = &features_1_3;
    features_1_2.bufferDeviceAddress                          = true;
    features_1_2.descriptorIndexing                           = true;
    features_1_2.descriptorBindingSampledImageUpdateAfterBind = true;
    features_1_2.descriptorBindingPartiallyBound              = true;
    features_1_2.descriptorBindingUpdateUnusedWhilePending    = true;
    features_1_2.runtimeDescriptorArray                       = true;
    if (vk_ctx.raytracing_supported) { features_1_2.bufferDeviceAddress = true; }

    VkPhysicalDeviceVulkan11Features features_1_1 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    features_1_1.pNext                            = &features_1_2;

    VkPhysicalDeviceFeatures2 features_2  = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features_2.features.samplerAnisotropy = true;
    features_2.pNext                      = &features_1_1;

    VkDeviceCreateInfo device_create_info      = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.pNext                   = &features_2;
    device_create_info.flags                   = 0;
    device_create_info.queueCreateInfoCount    = queue_family_count;
    device_create_info.pQueueCreateInfos       = device_queue_create_infos;
    device_create_info.enabledExtensionCount   = 1;
    device_create_info.ppEnabledExtensionNames = extensions;
    device_create_info.pEnabledFeatures        = NULL;

    VkDevice device = NULL;
    vkCreateDevice(*physical_device, &device_create_info, NULL, &device);

    return device;
}

void device_destroy(VkDevice *device) {
    ASSERT_VK_HANDLE(*device);
    vkDestroyDevice(*device, NULL);
}

VkSurfaceKHR window_surface_create(GLFWwindow *window, VkInstance *instance) {
    VkSurfaceKHR surface = NULL;
    VkResult result      = glfwCreateWindowSurface(*instance, window, NULL, &surface);
    if (result != VK_SUCCESS)
    {
        CEL_ERROR("vulkan error failed to create glfw window surface");
        abort();
    }

    return surface;
}

void window_surface_destroy(VkSurfaceKHR *surface, VkInstance *instance) {
    ASSERT_VK_HANDLE(*surface);
    vkDestroySurfaceKHR(*instance, *surface, NULL);
}

VkBool32 window_surface_support_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device, uint32_t graphics_queue_family_index) {
    VkBool32 surface_support = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(*physical_device, graphics_queue_family_index, *surface, &surface_support);
    return surface_support;
}

VkSurfaceCapabilitiesKHR window_surface_capabilities_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physical_device, *surface, &surface_capabilities);
    return surface_capabilities;
}

VkSurfaceFormatKHR prefer_surface_format_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device) {
    uint32_t surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physical_device, *surface, &surface_format_count, NULL);

    VkSurfaceFormatKHR *surface_formats = cel_arena_alloc(&vk_arena, sizeof(VkSurfaceKHR) * surface_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physical_device, *surface, &surface_format_count, surface_formats);

    VkSurfaceFormatKHR prefer_surface_format = surface_formats[0];
    return prefer_surface_format;
}

VkPresentModeKHR prefer_present_mode_get(VkSurfaceKHR *surface, VkPhysicalDevice *physical_device) {
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physical_device, *surface, &present_mode_count, NULL);

    VkPresentModeKHR *present_modes = cel_arena_alloc(&vk_arena, sizeof(VkPresentModeKHR) * present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physical_device, *surface, &present_mode_count, present_modes);

    VkPresentModeKHR prefer_present_mode = VK_PRESENT_MODE_FIFO_KHR;// VSYNC
    for (uint32_t i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { prefer_present_mode = VK_PRESENT_MODE_MAILBOX_KHR; }
    }

    return prefer_present_mode;
}

VkExtent2D prefer_swapchain_extent_get(VkSurfaceCapabilitiesKHR *surface_capabilities, GLFWwindow *window) {
    int fbo_width  = 0;
    int fbo_height = 0;
    glfwGetFramebufferSize(window, &fbo_width, &fbo_height);

    VkExtent2D prefer_swapchain_extent;
    if (surface_capabilities->currentExtent.width < fbo_width) { prefer_swapchain_extent.width = surface_capabilities->currentExtent.width; }
    else { prefer_swapchain_extent.width = fbo_width; }

    if (surface_capabilities->currentExtent.height < fbo_height) { prefer_swapchain_extent.height = surface_capabilities->currentExtent.height; }
    else { prefer_swapchain_extent.height = fbo_height; }

    return prefer_swapchain_extent;
}

VkSwapchainKHR swapchain_create(VkDevice *device, VkSurfaceKHR *surface, VkSurfaceCapabilitiesKHR *surface_capabilities, VkSurfaceFormatKHR *surface_format, VkExtent2D *swapchain_extent, const VkPresentModeKHR *present_mode, uint32_t image_array_layers, uint32_t graphics_queue_mode) {
    VkSharingMode image_sharing_mode  = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queue_family_index_count = 0;
    uint32_t *p_queue_family_indices  = NULL;
    uint32_t que_family_indices[]     = {0, 1};

    if (graphics_queue_mode == 1)
    {
        image_sharing_mode       = VK_SHARING_MODE_CONCURRENT;
        queue_family_index_count = 2;
        p_queue_family_indices   = que_family_indices;
    }

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.pNext                    = NULL;
    create_info.flags                    = 0;
    create_info.surface                  = *surface;
    create_info.minImageCount            = surface_capabilities->minImageCount + 1;
    create_info.imageFormat              = surface_format->format;
    create_info.imageColorSpace          = surface_format->colorSpace;
    create_info.imageExtent              = *swapchain_extent;
    create_info.imageArrayLayers         = image_array_layers;
    create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.imageSharingMode         = image_sharing_mode;
    create_info.queueFamilyIndexCount    = queue_family_index_count;
    create_info.pQueueFamilyIndices      = p_queue_family_indices;
    create_info.preTransform             = surface_capabilities->currentTransform;
    create_info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode              = *present_mode;
    create_info.clipped                  = VK_TRUE;
    create_info.oldSwapchain             = NULL;

    VkSwapchainKHR swapchain = NULL;
    VK_CHECK(vkCreateSwapchainKHR(*device, &create_info, NULL, &swapchain));
    return swapchain;
}

void swapchain_destroy(VkDevice *device, CELvk_swapchain *swapchain) {
    ASSERT_VK_HANDLE(swapchain->handle);
    vkDestroySwapchainKHR(*device, swapchain->handle, NULL);
}

uint32_t swapchain_image_count_get(VkDevice *device, VkSwapchainKHR *swapchain) {
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(*device, *swapchain, &swapchain_image_count, NULL);
    return swapchain_image_count;
}

VkImage *swapchain_images_get(VkDevice *device, VkSwapchainKHR *swapchain, uint32_t swapchain_image_count) {
    VkImage *swapchain_images = cel_arena_alloc(&vk_arena, sizeof(VkImage) * swapchain_image_count);
    vkGetSwapchainImagesKHR(*device, *swapchain, &swapchain_image_count, swapchain_images);
    return swapchain_images;
}

CELimage_handle swapchain_acquire_next_image(VkDevice *device, uint32_t current_frame_index, uint32_t *image_index) {
    CELvk_frame_data *frame = &vk_ctx.frames[current_frame_index];
    VkResult result         = vkAcquireNextImageKHR(*device, vk_ctx.swapchain.handle, UINT64_MAX, frame->swapchain_semaphore, NULL, image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        vk_ctx.swapchain.dirty = true;
    }
    else if (result != VK_SUCCESS)
    {
        CEL_ERROR("vulkan error: failed to acquire next swapchain image");
        abort();
    }
    return vk_ctx.swapchain.swapchain_images[*image_index];
}

void submit_and_present(VkCommandBuffer cmd, uint32_t current_frame_index, uint32_t image_index) {
    const CELvk_frame_data *frame = &vk_ctx.frames[current_frame_index];

    VkCommandBufferSubmitInfo buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    buffer_submit_info.commandBuffer             = cmd;

    VkSemaphoreSubmitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_info.semaphore             = frame->swapchain_semaphore;
    wait_info.stageMask             = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signal_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_info.semaphore             = frame->render_semaphore;
    signal_info.stageMask             = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    VkSubmitInfo2 submit_info_2            = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info_2.waitSemaphoreInfoCount   = 1;
    submit_info_2.pWaitSemaphoreInfos      = &wait_info;
    submit_info_2.commandBufferInfoCount   = 1;
    submit_info_2.pCommandBufferInfos      = &buffer_submit_info;
    submit_info_2.signalSemaphoreInfoCount = 1;
    submit_info_2.pSignalSemaphoreInfos    = &signal_info;

    VK_CHECK(vkQueueSubmit2(vk_ctx.device.graphics_queue, 1, &submit_info_2, frame->render_fence));

    VkPresentInfoKHR present_info   = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &frame->render_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &vk_ctx.swapchain.handle;
    present_info.pImageIndices      = &image_index;

    VkResult result = vkQueuePresentKHR(vk_ctx.device.present_queue, &present_info);
    if (result != VK_SUCCESS)
    {
        CEL_ERROR("vulkan error: fail to present image");
        vk_ctx.swapchain.dirty = true;
    }
}

CELvk_frame_data *perframes_create(VkDevice *device, uint32_t family_queue_index) {
    CELvk_frame_data *frames = cel_arena_alloc(&vk_arena, sizeof(CELvk_frame_data) * CELVK_MAX_FRAME_OVERLAP);

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_create_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex        = family_queue_index;

    for (uint32_t i = 0; i < CELVK_MAX_FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateFence(*device, &fence_create_info, NULL, &frames[i].render_fence));

        VK_CHECK(vkCreateSemaphore(*device, &semaphore_create_info, NULL, &frames[i].render_semaphore));
        VK_CHECK(vkCreateSemaphore(*device, &semaphore_create_info, NULL, &frames[i].swapchain_semaphore));

        VK_CHECK(vkCreateCommandPool(*device, &pool_create_info, NULL, &frames[i].primary_command_pool));

        VkCommandBufferAllocateInfo buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        buffer_allocate_info.pNext                       = NULL;
        buffer_allocate_info.commandPool                 = frames[i].primary_command_pool;
        buffer_allocate_info.commandBufferCount          = 1;
        buffer_allocate_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VK_CHECK(vkAllocateCommandBuffers(*device, &buffer_allocate_info, &frames[i].primary_command_buffer));
    }

    return frames;
}

void perframes_destroy(VkDevice *device, CELvk_frame_data *frames) {
    for (uint32_t i = 0; i < CELVK_MAX_FRAME_OVERLAP; ++i)
    {
        ASSERT_VK_HANDLE(frames[i].render_fence);
        vkDestroyFence(*device, frames[i].render_fence, NULL);

        ASSERT_VK_HANDLE(frames[i].render_semaphore);
        ASSERT_VK_HANDLE(frames[i].swapchain_semaphore);
        vkDestroySemaphore(*device, frames[i].render_semaphore, NULL);
        vkDestroySemaphore(*device, frames[i].swapchain_semaphore, NULL);

        ASSERT_VK_HANDLE(frames[i].primary_command_buffer);
        ASSERT_VK_HANDLE(frames[i].primary_command_pool);
        vkFreeCommandBuffers(*device, frames[i].primary_command_pool, 1, &frames[i].primary_command_buffer);
        vkDestroyCommandPool(*device, frames[i].primary_command_pool, NULL);
    }
}

Internal CELvk_immediate_command immediate_command_create(VkDevice *device, uint32_t queue_family_index) {
    CELvk_immediate_command im_cmd = {};

    VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex        = queue_family_index;
    VK_CHECK(vkCreateCommandPool(*device, &pool_create_info, NULL, &im_cmd.command_pool));

    VkCommandBufferAllocateInfo buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    buffer_allocate_info.pNext                       = NULL;
    buffer_allocate_info.commandPool                 = im_cmd.command_pool;
    buffer_allocate_info.commandBufferCount          = 1;
    buffer_allocate_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(*device, &buffer_allocate_info, &im_cmd.command_buffer));

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_create_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(*device, &fence_create_info, NULL, &im_cmd.fence));

    return im_cmd;
}

Internal void immediate_command_destroy(VkDevice *device, CELvk_immediate_command *im_cmd) {
    ASSERT_VK_HANDLE(im_cmd->fence);
    vkDestroyFence(*device, im_cmd->fence, NULL);
    ASSERT_VK_HANDLE(im_cmd->command_pool);
    ASSERT_VK_HANDLE(im_cmd->command_buffer);
    vkFreeCommandBuffers(*device, im_cmd->command_pool, 1, &im_cmd->command_buffer);
    vkDestroyCommandPool(*device, im_cmd->command_pool, NULL);
}

CELvk_bindless_descriptor bindless_descriptor_create(VkDevice *device) {
    CELvk_bindless_descriptor descriptor = {0};

#define CELVK_DESCRIPTOR_COUNT 2
    VkDescriptorPoolSize descriptor_pool_sizes[CELVK_DESCRIPTOR_COUNT] = {
        {.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = CELVK_MAX_BINDLESS_RESOURCE_COUNT},
        {.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = CELVK_MAX_SAMPLER_COUNT},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_pool_create_info.pNext                      = NULL;
    descriptor_pool_create_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    descriptor_pool_create_info.maxSets                    = CELVK_MAX_BINDLESS_RESOURCE_COUNT * CELVK_DESCRIPTOR_COUNT;
    descriptor_pool_create_info.poolSizeCount              = CELVK_DESCRIPTOR_COUNT;
    descriptor_pool_create_info.pPoolSizes                 = descriptor_pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(*device, &descriptor_pool_create_info, NULL, &descriptor.pool));

    VkDescriptorSetLayoutBinding bindings[CELVK_DESCRIPTOR_COUNT] = {
        {.binding = CELVK_TEXTURE_BINDING, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL, .descriptorCount = 1},
        {.binding = CELVK_SAMPLER_BINDING, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL, .descriptorCount = 1},
    };

    VkDescriptorBindingFlags binding_flags[CELVK_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo descriptor_set_layout_binding_flags_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    descriptor_set_layout_binding_flags_create_info.pNext                                       = NULL;
    descriptor_set_layout_binding_flags_create_info.bindingCount                                = CELVK_DESCRIPTOR_COUNT;
    descriptor_set_layout_binding_flags_create_info.pBindingFlags                               = binding_flags;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_set_layout_create_info.pNext                           = &descriptor_set_layout_binding_flags_create_info;
    descriptor_set_layout_create_info.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    descriptor_set_layout_create_info.bindingCount                    = CELVK_DESCRIPTOR_COUNT;
    descriptor_set_layout_create_info.pBindings                       = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(*device, &descriptor_set_layout_create_info, NULL, &descriptor.set_layout));

    VkDescriptorSetAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate_info.pNext                       = NULL;
    allocate_info.descriptorPool              = descriptor.pool;
    allocate_info.descriptorSetCount          = 1;
    allocate_info.pSetLayouts                 = &descriptor.set_layout;

    VK_CHECK(vkAllocateDescriptorSets(*device, &allocate_info, &descriptor.set));
#undef CELVK_DESCRIPTOR_COUNT

    return descriptor;
}

void bindless_descriptor_destroy(VkDevice *device, CELvk_bindless_descriptor *descriptor) {
    ASSERT_VK_HANDLE(descriptor->set_layout);
    ASSERT_VK_HANDLE(descriptor->set);
    ASSERT_VK_HANDLE(descriptor->pool);

    vkFreeDescriptorSets(*device, descriptor->pool, 1, &descriptor->set);
    vkDestroyDescriptorSetLayout(*device, descriptor->set_layout, NULL);
    vkDestroyDescriptorPool(*device, descriptor->pool, NULL);
}

CELbuffer_handle celvk_staging_buffer_create(VmaAllocator *allocator, VkDeviceSize size, VkBufferUsageFlags usages) {
    CELvk_buffer buffer;

    VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.pNext              = NULL;
    buffer_create_info.size               = size;
    buffer_create_info.usage              = usages | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocation_create_info.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

    vmaCreateBuffer(*allocator, &buffer_create_info, &allocation_create_info, &buffer.handle, &buffer.allocation, &buffer.allocation_info);

    uint32_t index    = vk_buffer_count++;
    vk_buffers[index] = buffer;
    return (CELbuffer_handle){.idx = index};
}

CELbuffer_handle celvk_gpu_buffer_create(VmaAllocator *allocator, VkDeviceSize size, VkBufferUsageFlags usages) {
    CELvk_buffer buffer;

    VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.pNext              = NULL;
    buffer_create_info.size               = size;
    buffer_create_info.usage              = usages | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBuffer(*allocator, &buffer_create_info, &allocation_create_info, &buffer.handle, &buffer.allocation, &buffer.allocation_info);
    uint32_t index    = vk_buffer_count++;
    vk_buffers[index] = buffer;
    return (CELbuffer_handle){.idx = index};
}

void celvk_buffer_destroy(VmaAllocator *allocator, const CELbuffer_handle *handle) {
    CELvk_buffer *buffer = &vk_buffers[handle->idx];
    vmaDestroyBuffer(*allocator, buffer->handle, buffer->allocation);
    *buffer = (CELvk_buffer){0};
}

CELimage_handle celvk_image_create(const CELvk_image_create_info *create_info, const VmaAllocationCreateInfo *allocation_info) {
    assert(vk_image_count < CELVK_MAX_IMAGE_COUNT && "vulkan error: exceeded max image count");

    CELvk_image image = {};
    image.own_image   = true;

    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_create_info.pNext             = NULL;
    image_create_info.flags             = create_info->flags;
    image_create_info.imageType         = VK_IMAGE_TYPE_2D;
    image_create_info.format            = create_info->format;
    image_create_info.extent            = create_info->extent;
    image_create_info.arrayLayers       = create_info->base_array_layers;
    image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.mipLevels         = 1;
    image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage             = create_info->usages;
    image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK(vmaCreateImage(vk_ctx.allocator, &image_create_info, allocation_info, &image.handle, &image.allocation, NULL));

    VkImageViewCreateInfo image_view_create_info           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_create_info.pNext                           = NULL;
    image_view_create_info.flags                           = 0;
    image_view_create_info.image                           = image.handle;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = create_info->format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
    image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    VK_CHECK(vkCreateImageView(vk_ctx.device.handle, &image_view_create_info, NULL, &image.image_view));

    uint32_t index   = vk_image_count++;
    vk_images[index] = image;
    return (CELimage_handle){.idx = index};
}

CELimage_handle celvk_image_create_w_handle(VkDevice *device, VmaAllocator *allocator, const CELvk_image_create_info *create_info, VkImage handle) {
    assert(vk_image_count < CELVK_MAX_IMAGE_COUNT && "vulkan error: exceeded max image count");
    CELvk_image image = {};
    image.handle      = handle;
    image.own_image   = false;

    VkImageViewCreateInfo image_view_create_info           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_create_info.pNext                           = NULL;
    image_view_create_info.flags                           = 0;
    image_view_create_info.image                           = image.handle;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = create_info->format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
    image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(*device, &image_view_create_info, NULL, &image.image_view));

    uint32_t index   = vk_image_count++;
    vk_images[index] = image;
    return (CELimage_handle){.idx = index};
}

void celvk_image_destroy(VkDevice *device, VmaAllocator *allocator, const CELimage_handle *handle) {
    CELvk_image *image = &vk_images[handle->idx];
    vkDestroyImageView(*device, image->image_view, NULL);
    if (image->own_image) { vmaDestroyImage(*allocator, image->handle, image->allocation); }
    *image = (CELvk_image){0};
}

CELsampler_handle celvk_sampler_create(VkDevice *device, const VkSamplerCreateInfo *create_info) {
    assert(vk_sampler_count < CELVK_MAX_SAMPLER_COUNT && "vulkan error: exceeded max sampler count");

    CELvk_sampler sampler = {0};

    VK_CHECK(vkCreateSampler(*device, create_info, NULL, &sampler.handle));
    uint32_t index     = vk_sampler_count++;
    vk_samplers[index] = sampler;
    return (CELsampler_handle){.idx = index};
}

void celvk_sampler_destroy(VkDevice *device, const CELsampler_handle *handle) {
    CELvk_sampler *sampler = &vk_samplers[handle->idx];
    vkDestroySampler(*device, sampler->handle, NULL);
    *sampler = (CELvk_sampler){0};
}

void vk_images_destroy(VkDevice *device, VmaAllocator *allocator) {
    for (uint32_t i = 0; i < vk_image_count; ++i)
    {
        celvk_image_destroy(device, allocator, &(CELimage_handle){.idx = i});
    }
}

void vk_buffers_destroy(VmaAllocator *allocator) {
    for (uint32_t i = 0; i < vk_buffer_count; ++i)
    {
        celvk_buffer_destroy(allocator, &(CELbuffer_handle){.idx = i});
    }
}

void vk_samplers_destroy(VkDevice *device) {
    for (uint32_t i = 0; i < vk_sampler_count; ++i)
    {
        celvk_sampler_destroy(device, &(CELsampler_handle){.idx = i});
    }
}

uint32_t *celvk_load_shader_w_spv(const char *path, size_t *size) {
    char cwd[FS_PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    printf("current workider: %s\n", cwd);
    FILE *file;
    fopen_s(&file, path, "rb");
    if (!file)
    {
        perror("io error: failed to open .spv file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t s = ftell(file);
    rewind(file);

    if (s % 4 != 0)
    {
        CEL_ERROR("io error: spirv size is not a multiple of 4");
        fclose(file);
        return NULL;
    }

    uint32_t *buffer = cel_arena_alloc(&vk_arena, s);
    if (!buffer)
    {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, s, file);
    fclose(file);

    if (read_size != s)
    {
        CEL_ERROR("io error: failed read entire .spv file");
        return NULL;
    }

    *size = s;
    return buffer;
}

VkPipeline celvk_graphics_pipeline_create(VkDevice *device, const VkPipelineRenderingCreateInfo *rendering_create_info, const CELprogram_handle *program_handle) {
    VkGraphicsPipelineCreateInfo create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

    CELvk_program *program = &vk_programs[program_handle->idx];
    assert(program->stage_count > 0 && "failed shader stage should larger than 0");
    if (program->stage_count <= 0) { return NULL; }

    VkPipelineShaderStageCreateInfo *stages = cel_arena_alloc(&vk_arena, sizeof(VkPipelineShaderStageCreateInfo) * program->stage_count);
    for (uint32_t i = 0; i < program->stage_count; ++i)
    {
        stages[i] = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = program->shader_stages[i],
            .module = program->shader_modules[i],
            .pName  = "main",
        };
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state.vertexBindingDescriptionCount        = 0;
    vertex_input_state.pVertexBindingDescriptions           = NULL;
    vertex_input_state.vertexAttributeDescriptionCount      = 0;
    vertex_input_state.pVertexAttributeDescriptions         = NULL;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable                 = false;

    VkPipelineTessellationStateCreateInfo tessellation_state = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    tessellation_state.patchControlPoints                    = 1;

    VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount                     = 1;
    viewport_state.pViewports                        = NULL;
    viewport_state.scissorCount                      = 1;
    viewport_state.pScissors                         = NULL;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state.depthClampEnable                       = false;
    rasterization_state.rasterizerDiscardEnable                = false;
    rasterization_state.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode                               = VK_CULL_MODE_NONE;
    rasterization_state.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable                        = false;
    rasterization_state.depthBiasConstantFactor                = 0.0f;
    rasterization_state.depthBiasClamp                         = 0.0f;
    rasterization_state.depthBiasSlopeFactor                   = 0.0f;
    rasterization_state.lineWidth                              = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable                  = false;
    multisample_state.minSampleShading                     = 1.0f;
    multisample_state.pSampleMask                          = NULL;
    multisample_state.alphaToCoverageEnable                = false;
    multisample_state.alphaToOneEnable                     = false;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_state.depthTestEnable                       = false;
    depth_stencil_state.depthWriteEnable                      = false;
    depth_stencil_state.depthCompareOp                        = VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable                 = false;
    depth_stencil_state.stencilTestEnable                     = false;
    depth_stencil_state.minDepthBounds                        = 0.0f;
    depth_stencil_state.maxDepthBounds                        = 1.0f;

    VkPipelineColorBlendAttachmentState color_attachment = {
        .blendEnable         = false,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state.logicOpEnable                       = false;
    color_blend_state.logicOp                             = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount                     = 1;
    color_blend_state.pAttachments                        = &color_attachment;
    color_blend_state.blendConstants[0]                   = 0.0f;
    color_blend_state.blendConstants[1]                   = 0.0f;
    color_blend_state.blendConstants[2]                   = 0.0f;
    color_blend_state.blendConstants[3]                   = 0.0f;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount                = 2;
    dynamic_state.pDynamicStates                   = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.pNext                        = rendering_create_info;
    pipeline_create_info.stageCount                   = program->stage_count;
    pipeline_create_info.pStages                      = stages;
    pipeline_create_info.pVertexInputState            = &vertex_input_state;
    pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
    pipeline_create_info.pTessellationState           = &tessellation_state;
    pipeline_create_info.pViewportState               = &viewport_state;
    pipeline_create_info.pRasterizationState          = &rasterization_state;
    pipeline_create_info.pMultisampleState            = &multisample_state;
    pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
    pipeline_create_info.pColorBlendState             = &color_blend_state;
    pipeline_create_info.pDynamicState                = &dynamic_state;
    pipeline_create_info.layout                       = program->layout;

    VkPipeline pipeline = NULL;

    for (uint32_t i = 0; i < program->stage_count; ++i) { vkDestroyShaderModule(*device, program->shader_modules[i], NULL); }

    return pipeline;
}

void celvk_pipeline_destroy(VkDevice *device, VkPipeline *pipeline) {
}

CELprogram_handle celvk_program_create(VkDevice *device, VkPipelineBindPoint bind_point, size_t push_constant_size, const char **shader_paths, uint32_t shader_count) {
    assert(vk_program_count < CELVK_MAX_PROGRAM_COUNT && "vulkan error: exceeded max program count");

    CELvk_program program = {0};
    program.stage_count   = shader_count;

    program.shader_modules = cel_arena_alloc(&vk_arena, sizeof(VkShaderModule) * shader_count);
    uint32_t **shader_code = cel_arena_alloc(&vk_arena, sizeof(uint32_t *) * shader_count);
    size_t *shader_sizes   = cel_arena_alloc(&vk_arena, sizeof(size_t) * shader_count);
    for (uint32_t i = 0; i < shader_count; ++i)
    {
        shader_code[i] = celvk_load_shader_w_spv(shader_paths[i], &shader_sizes[i]);
        assert(shader_code[i] && "vulkan error: failed to load shader from path");

        VkShaderModuleCreateInfo shader_module_create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader_module_create_info.codeSize                 = shader_sizes[i];
        shader_module_create_info.pCode                    = shader_code[i];
        VK_CHECK(vkCreateShaderModule(*device, &shader_module_create_info, NULL, &program.shader_modules[i]));
    }

    program.bind_point = bind_point;

    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset     = 0,
        .size       = (uint32_t) push_constant_size};

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.pNext                      = NULL;
    pipeline_layout_create_info.flags                      = 0;
    pipeline_layout_create_info.setLayoutCount             = 1;
    pipeline_layout_create_info.pSetLayouts                = NULL;
    pipeline_layout_create_info.pushConstantRangeCount     = push_constant_size > 0 ? 1 : 0;
    pipeline_layout_create_info.pPushConstantRanges        = push_constant_size > 0 ? &push_constant_range : NULL;

    VK_CHECK(vkCreatePipelineLayout(*device, &pipeline_layout_create_info, NULL, &program.layout));
    uint32_t index     = vk_program_count++;
    vk_programs[index] = program;

    program.pipeline = celvk_graphics_pipeline_create(device, &(VkPipelineRenderingCreateInfo){}, &(CELprogram_handle){.idx = index});

    return (CELprogram_handle){.idx = index};
}

CELAPI CELprogram_handle celvk_sprite_renderer_create(VkFormat format) {
    char *vert = cel_arena_alloc(&vk_arena, FS_PATH_MAX);
    char *frag = cel_arena_alloc(&vk_arena, FS_PATH_MAX);
    snprintf(vert, sizeof(vert), "%s/%s", vk_ctx.engine_path, "/builtin_sprite.vert.glsl.spv");
    snprintf(vert, sizeof(frag), "%s/%s", vk_ctx.engine_path, "/builtin_sprite.frag.glsl.spv");
    const char *shader_paths[2] = {vert, frag};

    return celvk_program_create(&vk_ctx.device.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, sizeof(CELsprite_renderer_pc), shader_paths, 2);
}

CELAPI void celvk_program_destroy(VkDevice *device, const CELprogram_handle *handle) {
    CELvk_program *program = &vk_programs[handle->idx];
    vkDestroyPipelineLayout(*device, program->layout, NULL);
    *program = (CELvk_program){0};
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data) {
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        CEL_WARN("vulkan debug: %d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        CEL_ERROR("vulkan debug: %d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    return VK_FALSE;
}

const char *vk_result_string(VkResult result) {
    switch (result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED: return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT: return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR: return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR: return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        default: return "VK_UNKNOWN_ERROR";
    }
}
