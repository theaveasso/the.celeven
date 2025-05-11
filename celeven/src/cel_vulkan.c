#include "cel_vulkan.h"

typedef struct CELvulkan_ctx CELvulkan_ctx;
struct CELvulkan_ctx {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSwapchainKHR swapchain;
};

GlobalVariable CELvulkan_ctx vk_ctx = {};

bool cel_vulkan_init() {
    volkInitialize();

    return true;
}

void cel_vulkan_fini() {
    volkFinalize();
}
