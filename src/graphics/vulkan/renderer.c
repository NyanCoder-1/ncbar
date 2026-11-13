#include "graphics/vulkan/renderer.h"
#include "graphics/vulkan/context.h"
#include <stdlib.h>

struct vulkan_renderer *vulkan_renderer_init(struct vulkan_context *ctx, struct vulkan_renderer_params *params) {
    struct vulkan_renderer *vk_renderer = (struct vulkan_renderer *)calloc(1, sizeof(struct vulkan_renderer));
    if (vk_renderer == NULL) {
        return NULL;
    }
    vk_renderer->context = ctx;
    enum ncbar_vulkan_error error = vulkan_renderer_surface_create(vk_renderer, params->wl_display, params->wl_surface);
    if (error != NCBAR_VULKAN_OK) {
        vulkan_renderer_destroy(vk_renderer);
        return NULL;
    }
    if (params->width != 0 && params->height != 0) {
        error = vulkan_renderer_swapchain_create(vk_renderer, params->width, params->height);
        if (error != NCBAR_VULKAN_OK) {
            // TODO: report swapchain create error, but it's not critical, so do not interrupt renderer creation
        }
    }
    return vk_renderer;
}
void vulkan_renderer_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL)
        return;
    vulkan_renderer_swapchain_destroy(vk_renderer);
    vulkan_renderer_process_retirement(vk_renderer);
    vulkan_renderer_surface_destroy(vk_renderer);
    free(vk_renderer);
}
