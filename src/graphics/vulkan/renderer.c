#include "graphics/vulkan/renderer.h"
#include "graphics/vulkan/context.h"
#include <stb_ds.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct vulkan_renderer *vulkan_renderer_init(struct vulkan_context *ctx, struct vulkan_renderer_params *params) {
    struct vulkan_renderer *vk_renderer = (struct vulkan_renderer *)calloc(1, sizeof(struct vulkan_renderer));
    if (vk_renderer == NULL) {
        return NULL;
    }
    vk_renderer->context = ctx;
    vk_renderer->swapchain_healthy = false;
    enum ncbar_vulkan_error error = vulkan_renderer_surface_create(vk_renderer, params->wl_display, params->wl_surface);
    if (error != NCBAR_VULKAN_OK) {
        vulkan_renderer_destroy(vk_renderer);
        return NULL;
    }
    if (params->width != 0 && params->height != 0) {
        error = vulkan_renderer_resize(vk_renderer, params->width, params->height);
        if (error != NCBAR_VULKAN_OK) {
            fprintf(stderr, "vulkan_renderer_init: vulkan_renderer_swapchain_create failed with %d", (int)error);
            // TODO: report swapchain create error, but it's not critical, so do not interrupt renderer creation
        }
    }
    return vk_renderer;
}
void vulkan_renderer_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL)
        return;
    vk_renderer->context->vk_loader_device.vkDeviceWaitIdle(vk_renderer->context->device);
    vulkan_renderer_process_retirement(vk_renderer, true);
    vulkan_renderer_render_pass_destroy(vk_renderer);
    vulkan_renderer_swapchain_destroy(vk_renderer);
    vulkan_renderer_surface_destroy(vk_renderer);
    free(vk_renderer);
}
enum ncbar_vulkan_error vulkan_renderer_resize(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height) {
    vk_renderer->context->vk_loader_device.vkDeviceWaitIdle(vk_renderer->context->device);
    vk_renderer->swapchain_healthy = false;
    enum ncbar_vulkan_error error = vulkan_renderer_swapchain_create(vk_renderer, width, height);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    error = vulkan_renderer_swapchain_image_views_create(vk_renderer);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    if (vk_renderer->render_pass == VK_NULL_HANDLE) {
        error = vulkan_renderer_render_pass_create(vk_renderer);
        if (error != NCBAR_VULKAN_OK) {
            return error;
        }
    }
    error = vulkan_renderer_swapchain_framebuffers_create(vk_renderer);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    vk_renderer->swapchain_healthy = true;
    return NCBAR_VULKAN_OK;
}

void vulkan_renderer_process_retirement(struct vulkan_renderer *vk_renderer, bool free_arrays) {
    if (vk_renderer == NULL)
        return;

    // framebuffers
    for (size_t i = 0; i < arrlenu(vk_renderer->swapchain_framebuffer_retire_queue); ++i) {
        if (vk_renderer->swapchain_framebuffer_retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroyFramebuffer(vk_renderer->context->device, vk_renderer->swapchain_framebuffer_retire_queue[i], NULL);
            vk_renderer->swapchain_framebuffer_retire_queue[i] = VK_NULL_HANDLE;
        }
    }
    if (free_arrays) {
        arrfree(vk_renderer->swapchain_framebuffer_retire_queue);
    } else {
        arrclear(vk_renderer->swapchain_framebuffer_retire_queue);
    }

    // image_views
    for (size_t i = 0; i < arrlenu(vk_renderer->swapchain_image_view_retire_queue); ++i) {
        if (vk_renderer->swapchain_image_view_retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroyImageView(vk_renderer->context->device, vk_renderer->swapchain_image_view_retire_queue[i], NULL);
            vk_renderer->swapchain_image_view_retire_queue[i] = VK_NULL_HANDLE;
        }
    }
    if (free_arrays) {
        arrfree(vk_renderer->swapchain_image_view_retire_queue);
    } else {
        arrclear(vk_renderer->swapchain_image_view_retire_queue);
    }

    // swapchains
    for (size_t i = 0; i < arrlenu(vk_renderer->swapchain_retire_queue); ++i) {
        if (vk_renderer->swapchain_retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->swapchain_retire_queue[i], NULL);
            vk_renderer->swapchain_retire_queue[i] = VK_NULL_HANDLE;
        }
    }
    if (free_arrays) {
        arrfree(vk_renderer->swapchain_retire_queue);
    } else {
        arrclear(vk_renderer->swapchain_retire_queue);
    }
}
