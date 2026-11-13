#include "graphics/vulkan/context.h"
#include <stdbool.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

static bool is_device_suitable(struct vulkan_context *self, VkPhysicalDevice device, uint32_t *graphics_family, struct wl_display *wl_display);

static const char *const vk_device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const uint32_t vk_device_extensions_count = sizeof(vk_device_extensions) / sizeof(vk_device_extensions[0]);

enum ncbar_vulkan_error vulkan_context_init_physical_device(struct vulkan_context *self, struct wl_display *wl_display) {
    uint32_t device_count = 0;
    self->vk_loader_instance.vkEnumeratePhysicalDevices(self->instance, &device_count, NULL);
    if (device_count == 0)
        return NCBAR_VULKAN_NO_VULKAN_GPU;
    VkPhysicalDevice physical_devices[device_count];
    self->vk_loader_instance.vkEnumeratePhysicalDevices(self->instance, &device_count, physical_devices);

    VkPhysicalDevice result = VK_NULL_HANDLE;
    uint32_t graphics_family_index = 0;
    for (uint32_t device_num = 0; device_num < device_count; device_num++) {
        const VkPhysicalDevice device = physical_devices[device_num];

        uint32_t graphics_family_index_current = 0;
        if (is_device_suitable(self, device, &graphics_family_index_current, wl_display)) {
            result = device;
            graphics_family_index = graphics_family_index_current;
            break;
        }
    }

    if (result == VK_NULL_HANDLE)
        return NCBAR_VULKAN_NO_SUITABLE_GPU;

    self->physical_device = result;
    self->graphics_queue_family = graphics_family_index;

    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_context_init_device(struct vulkan_context *self) {
    const float queue_priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = self->graphics_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    const VkPhysicalDeviceFeatures device_features = {0};
    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = (const VkDeviceQueueCreateInfo[]){queue_create_info},
        .enabledExtensionCount = vk_device_extensions_count,
        .ppEnabledExtensionNames = vk_device_extensions,
        .pEnabledFeatures = &device_features,
    };
    VkResult result = self->vk_loader_instance.vkCreateDevice(self->physical_device, &create_info, NULL, &self->device);
    if (result != VK_SUCCESS)
        return NCBAR_VULKAN_LOGICAL_DEVICE_CREATE_FAILED;

    return NCBAR_VULKAN_OK;
}

void vulkan_context_destroy_device(struct vulkan_context *self) {
    if (self == NULL || self->device == VK_NULL_HANDLE || self->vk_loader_device.vkDestroyDevice == NULL)
        return;
    self->vk_loader_device.vkDestroyDevice(self->device, NULL);
    self->device = VK_NULL_HANDLE;
}

static uint32_t find_queue_family(struct vulkan_context *self, VkPhysicalDevice device, struct wl_display *wl_display);
static bool check_device_extension_support(struct vulkan_context *self, VkPhysicalDevice device);

static bool is_device_suitable(struct vulkan_context *self, VkPhysicalDevice device, uint32_t *graphics_family, struct wl_display *wl_display) {
    const uint32_t queue_family_result = find_queue_family(self, device, wl_display);
    const bool extensions_supported = check_device_extension_support(self, device);
    if (queue_family_result != UINT32_MAX && extensions_supported) {
        *graphics_family = queue_family_result;
        return true;
    }
    return false;
}

static uint32_t find_queue_family(struct vulkan_context *self, VkPhysicalDevice device, struct wl_display *wl_display) {
    uint32_t queue_family_count = 0;
    self->vk_loader_instance.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_families[queue_family_count];
    self->vk_loader_instance.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    uint32_t result = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count && result == UINT32_MAX; i++) {
        VkBool32 present_support = self->vk_loader_instance.vkGetPhysicalDeviceWaylandPresentationSupportKHR(device, i, wl_display);
        const VkQueueFamilyProperties QUEUE_FAMILY = queue_families[i];
        if (present_support != VK_FALSE && (QUEUE_FAMILY.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result = i;
        }
    }

    return result;
}
static bool check_device_extension_support(struct vulkan_context *self, VkPhysicalDevice device) {
    uint32_t extension_count = 0;
    self->vk_loader_instance.vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    const uint32_t extension_array_size = extension_count == 0 ? 1 : extension_count;
    VkExtensionProperties available_extensions[extension_array_size];
    self->vk_loader_instance.vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);

    bool vk_device_extension_met[vk_device_extensions_count];
    memset(vk_device_extension_met, 0, sizeof(vk_device_extension_met));

    bool result = false;

    for (uint32_t i = 0; i < extension_count && !result; i++) {
        const char *available_extension_name = available_extensions[i].extensionName;
        uint32_t extensions_met_count = 0;
        for (uint32_t j = 0; j < vk_device_extensions_count; j++) {
            const char *required_extension = vk_device_extensions[j];
            if (strcmp(available_extension_name, required_extension) == 0) {
                vk_device_extension_met[j] = true;
            }
            if (vk_device_extension_met[j]) {
                extensions_met_count++;
            }
        }
        result = extensions_met_count == vk_device_extensions_count;
    }

    return result;
}