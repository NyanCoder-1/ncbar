#include "graphics/vulkan/renderer.h"
#include "graphics/vulkan/context.h"
#include <stb_ds.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

constexpr unsigned long long ACQUIRE_TIMEOUT_NS = 1000000000ULL;

static enum ncbar_vulkan_error record_clear_commands(struct vulkan_renderer *vk_renderer, uint32_t image_index);

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
    error = vulkan_renderer_frame_resources_create(vk_renderer);
    if (error != NCBAR_VULKAN_OK) {
        vulkan_renderer_destroy(vk_renderer);
        return NULL;
    }
    error = vulkan_renderer_sync_objects_create(vk_renderer);
    if (error != NCBAR_VULKAN_OK) {
        vulkan_renderer_destroy(vk_renderer);
        return NULL;
    }
    return vk_renderer;
}
void vulkan_renderer_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL)
        return;
    vk_renderer->context->vk_loader_device.vkDeviceWaitIdle(vk_renderer->context->device);
    vulkan_renderer_process_retirement(vk_renderer, true);
    vulkan_renderer_sync_objects_destroy(vk_renderer);
    vulkan_renderer_frame_resources_destroy(vk_renderer);
    vulkan_renderer_swapchain_framebuffers_destroy(vk_renderer);
    vulkan_renderer_swapchain_image_views_destroy(vk_renderer);
    vulkan_renderer_render_pass_destroy(vk_renderer);
    vulkan_renderer_swapchain_destroy(vk_renderer);
    vulkan_renderer_surface_destroy(vk_renderer);
    free(vk_renderer);
}
enum ncbar_vulkan_error vulkan_renderer_resize(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height) {
    vk_renderer->swapchain_healthy = false;
    enum ncbar_vulkan_error error = vulkan_renderer_swapchain_create(vk_renderer, width, height);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    error = vulkan_renderer_swapchain_image_views_create(vk_renderer);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    if (vk_renderer->render_pass_image_format != vk_renderer->swapchain_image_format) {
        if (vk_renderer->render_pass != VK_NULL_HANDLE) {
            arrput(vk_renderer->render_pass_retire_queue, vk_renderer->render_pass);
        }
        vk_renderer->render_pass = VK_NULL_HANDLE;
        vk_renderer->render_pass_image_format = VK_FORMAT_UNDEFINED;
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

    // semaphores
    for (size_t i = 0; i < arrlenu(vk_renderer->vk_semaphore_retire_queue); i++) {
        if (vk_renderer->vk_semaphore_retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroySemaphore(vk_renderer->context->device, vk_renderer->vk_semaphore_retire_queue[i], NULL);
            vk_renderer->vk_semaphore_retire_queue[i] = VK_NULL_HANDLE;
        }
    }
    if (free_arrays) {
        arrfree(vk_renderer->vk_semaphore_retire_queue);
    } else {
        arrclear(vk_renderer->vk_semaphore_retire_queue);
    }

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

    // render-pass
    for (size_t i = 0; i < arrlenu(vk_renderer->render_pass_retire_queue); ++i) {
        if (vk_renderer->render_pass_retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroyRenderPass(vk_renderer->context->device, vk_renderer->render_pass_retire_queue[i], NULL);
            vk_renderer->render_pass_retire_queue[i] = VK_NULL_HANDLE;
        }
    }
    if (free_arrays) {
        arrfree(vk_renderer->render_pass_retire_queue);
    } else {
        arrclear(vk_renderer->render_pass_retire_queue);
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

enum ncbar_vulkan_error vulkan_renderer_render(struct vulkan_renderer *vk_renderer) {
    printf("render start\n");
    if (vk_renderer == NULL || !vk_renderer->swapchain_healthy) {
        return NCBAR_VULKAN_RENDER_SKIPPED;
    }
    printf("render 1\n");
    const struct vulkan_loader_device *vk = &vk_renderer->context->vk_loader_device;
    const VkDevice device = vk_renderer->context->device;

    /* 1. The previous frame's GPU work must be finished before reusing its command buffer and semaphore. */
    if (vk->vkWaitForFences(device, 1, &vk_renderer->in_flight, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    printf("render 2\n");

    /* 2. Acquire. Signals image_available when the image is really ready. */
    uint32_t image_index = 0;
    VkResult result = vk->vkAcquireNextImageKHR(device, vk_renderer->swapchain, ACQUIRE_TIMEOUT_NS, vk_renderer->image_available, VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_renderer->swapchain_healthy = false; /* Stage 4 recreates it */
        return NCBAR_VULKAN_RENDER_SKIPPED;
    }
    printf("render 3\n");
    if (result == VK_TIMEOUT || result == VK_NOT_READY) {
        return NCBAR_VULKAN_RENDER_SKIPPED;
    }
    printf("render 4\n");
    /* VK_SUBOPTIMAL_KHR still acquired an image and signaled the semaphore, so keep going. */
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    printf("render 5\n");
    if (image_index >= arrlenu(vk_renderer->render_finished) || image_index >= arrlenu(vk_renderer->swapchain_framebuffers)) {
        return NCBAR_VULKAN_RENDER_FAIL; /* per-image arrays out of sync with the swapchain: a bug */
    }
    printf("render 6\n");

    /* 3. Record. */
    enum ncbar_vulkan_error error = record_clear_commands(vk_renderer, image_index);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    printf("render 7\n");

    /* 4. Submit. Reset the fence only now, right before the submit that will signal it again. */
    if (vk->vkResetFences(device, 1, &vk_renderer->in_flight) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    printf("render 8\n");
    /* Must match the render pass's external dependency (srcStageMask). */
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vk_renderer->image_available,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_renderer->command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vk_renderer->render_finished[image_index],
    };
    if (vk->vkQueueSubmit(vk_renderer->context->graphics_queue, 1, &submit_info, vk_renderer->in_flight) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    printf("render 9\n");

    /* 5. Present. This is also what commits the wl_surface. */
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vk_renderer->render_finished[image_index],
        .swapchainCount = 1,
        .pSwapchains = &vk_renderer->swapchain,
        .pImageIndices = &image_index,
    };
    result = vk->vkQueuePresentKHR(vk_renderer->context->graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_renderer->swapchain_healthy = false;
        return NCBAR_VULKAN_RENDER_SKIPPED;
    }
    printf("render 10\n");
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    printf("render good\n");
    return NCBAR_VULKAN_OK;
}

static enum ncbar_vulkan_error record_clear_commands(struct vulkan_renderer *vk_renderer, uint32_t image_index) {
    const struct vulkan_loader_device *vk = &vk_renderer->context->vk_loader_device;
    const VkCommandBuffer cmd = vk_renderer->command_buffer;

    if (vk->vkResetCommandBuffer(cmd, 0) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    if (vk->vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }

    /* The color lives here, not in the render pass. Opaque for now (alpha = 1). */
    const VkClearValue clear_value = {.color = {.float32 = {1.0f, 1.0f, 1.0f, 1.0f}}};
    const VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk_renderer->render_pass,
        .framebuffer = vk_renderer->swapchain_framebuffers[image_index],
        .renderArea = {.offset = {0, 0}, .extent = vk_renderer->swapchain_extent},
        .clearValueCount = 1,
        .pClearValues = &clear_value,
    };
    vk->vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vk->vkCmdEndRenderPass(cmd);

    if (vk->vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_FAIL;
    }
    return NCBAR_VULKAN_OK;
}
