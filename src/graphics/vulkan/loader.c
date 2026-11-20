#include "graphics/vulkan/loader.h"
#include "graphics/vulkan/context.h"
#include <dlfcn.h>
#include <string.h>

enum ncbar_vulkan_error vulkan_context_init_loader_global(struct vulkan_loader_global *loader) {
    loader->vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (loader->vulkan_library == NULL) {
        return NCBAR_VULKAN_DLOPEN_FAILED;
    }

    *(void **)(&loader->vkGetInstanceProcAddr) = dlsym(loader->vulkan_library, "vkGetInstanceProcAddr");
    if (loader->vkGetInstanceProcAddr == NULL) {
        return NCBAR_VULKAN_LOADER_GLOBAL_INIT_FAILED;
    }
    loader->vkCreateInstance = (PFN_vkCreateInstance)loader->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    loader->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)loader->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties");
    if (loader->vkCreateInstance == NULL || loader->vkEnumerateInstanceLayerProperties == NULL) {
        return NCBAR_VULKAN_LOADER_GLOBAL_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_context_init_loader_instance(VkInstance instance, PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, struct vulkan_loader_instance *loader) {
    loader->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); // optional
    loader->vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    loader->vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
    loader->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"); // optional
    loader->vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    loader->vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    loader->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    loader->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    loader->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
    loader->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    loader->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    loader->vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    loader->vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    loader->vkGetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    if (loader->vkCreateWaylandSurfaceKHR == NULL || loader->vkDestroyInstance == NULL || loader->vkDestroySurfaceKHR == NULL ||
        loader->vkEnumerateDeviceExtensionProperties == NULL || loader->vkEnumeratePhysicalDevices == NULL || loader->vkGetDeviceProcAddr == NULL ||
        loader->vkCreateDevice == NULL || loader->vkGetPhysicalDeviceQueueFamilyProperties == NULL || loader->vkGetPhysicalDeviceSurfaceCapabilitiesKHR == NULL ||
        loader->vkGetPhysicalDeviceSurfaceFormatsKHR == NULL || loader->vkGetPhysicalDeviceSurfacePresentModesKHR == NULL ||
        loader->vkGetPhysicalDeviceWaylandPresentationSupportKHR == NULL) {
        return NCBAR_VULKAN_LOADER_INSTANCE_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_context_init_loader_device(VkDevice device, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, struct vulkan_loader_device *loader) {
    loader->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
    loader->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vkGetDeviceProcAddr(device, "vkAllocateCommandBuffers");
    loader->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(device, "vkBeginCommandBuffer");
    loader->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
    loader->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(device, "vkCmdEndRenderPass");
    loader->vkCreateCommandPool = (PFN_vkCreateCommandPool)vkGetDeviceProcAddr(device, "vkCreateCommandPool");
    loader->vkCreateFence = (PFN_vkCreateFence)vkGetDeviceProcAddr(device, "vkCreateFence");
    loader->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkGetDeviceProcAddr(device, "vkCreateFramebuffer");
    loader->vkCreateImageView = (PFN_vkCreateImageView)vkGetDeviceProcAddr(device, "vkCreateImageView");
    loader->vkCreateRenderPass = (PFN_vkCreateRenderPass)vkGetDeviceProcAddr(device, "vkCreateRenderPass");
    loader->vkCreateSemaphore = (PFN_vkCreateSemaphore)vkGetDeviceProcAddr(device, "vkCreateSemaphore");
    loader->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
    loader->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vkGetDeviceProcAddr(device, "vkDestroyCommandPool");
    loader->vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");
    loader->vkDestroyFence = (PFN_vkDestroyFence)vkGetDeviceProcAddr(device, "vkDestroyFence");
    loader->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkGetDeviceProcAddr(device, "vkDestroyFramebuffer");
    loader->vkDestroyImageView = (PFN_vkDestroyImageView)vkGetDeviceProcAddr(device, "vkDestroyImageView");
    loader->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkGetDeviceProcAddr(device, "vkDestroyRenderPass");
    loader->vkDestroySemaphore = (PFN_vkDestroySemaphore)vkGetDeviceProcAddr(device, "vkDestroySemaphore");
    loader->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
    loader->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkGetDeviceProcAddr(device, "vkDeviceWaitIdle");
    loader->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(device, "vkEndCommandBuffer");
    loader->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
    loader->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
    loader->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
    loader->vkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device, "vkQueueSubmit");
    loader->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)vkGetDeviceProcAddr(device, "vkResetCommandBuffer");
    loader->vkResetFences = (PFN_vkResetFences)vkGetDeviceProcAddr(device, "vkResetFences");
    loader->vkWaitForFences = (PFN_vkWaitForFences)vkGetDeviceProcAddr(device, "vkWaitForFences");
    if (loader->vkAcquireNextImageKHR == NULL || loader->vkAllocateCommandBuffers == NULL || loader->vkBeginCommandBuffer == NULL || loader->vkCmdBeginRenderPass == NULL ||
        loader->vkCmdEndRenderPass == NULL || loader->vkCreateCommandPool == NULL || loader->vkCreateFence == NULL || loader->vkCreateFramebuffer == NULL ||
        loader->vkCreateImageView == NULL || loader->vkCreateRenderPass == NULL || loader->vkCreateSemaphore == NULL || loader->vkCreateSwapchainKHR == NULL ||
        loader->vkDestroyCommandPool == NULL || loader->vkDestroyDevice == NULL || loader->vkDestroyFence == NULL || loader->vkDestroyFramebuffer == NULL ||
        loader->vkDestroyImageView == NULL || loader->vkDestroyRenderPass == NULL || loader->vkDestroySwapchainKHR == NULL || loader->vkDeviceWaitIdle == NULL ||
        loader->vkEndCommandBuffer == NULL || loader->vkGetDeviceQueue == NULL || loader->vkQueuePresentKHR == NULL || loader->vkQueueSubmit == NULL ||
        loader->vkResetCommandBuffer == NULL || loader->vkResetFences == NULL || loader->vkWaitForFences == NULL) {
        return NCBAR_VULKAN_LOADER_DEVICE_INIT_FAILED;
    }

    return NCBAR_VULKAN_OK;
}

void vulkan_context_destroy_loader(struct vulkan_context *self) {
    if (self == NULL || self->vk_loader_global.vulkan_library == NULL)
        return;
    dlclose(self->vk_loader_global.vulkan_library);
    memset(&self->vk_loader_device, 0, sizeof(self->vk_loader_device));
    memset(&self->vk_loader_instance, 0, sizeof(self->vk_loader_instance));
    memset(&self->vk_loader_global, 0, sizeof(self->vk_loader_global));
}