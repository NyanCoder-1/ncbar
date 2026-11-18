#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"

enum ncbar_vulkan_error vulkan_renderer_render_pass_create(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->swapchain_image_format == VK_FORMAT_UNDEFINED) {
        return NCBAR_VULKAN_RENDER_PASS_INVALID_ARGUMENT;
    }

    const VkAttachmentDescription color_attachment = {
        .format = vk_renderer->swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,     /* previous contents are irrelevant, we clear */
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, /* ready for vkQueuePresentKHR */
    };
    const VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };
    /* Pairs with the acquire semaphore: the submit must wait at COLOR_ATTACHMENT_OUTPUT. */
    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    const VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkResult result = vk_renderer->context->vk_loader_device.vkCreateRenderPass(vk_renderer->context->device, &create_info, NULL, &render_pass);
    if (result != VK_SUCCESS) {
        return NCBAR_VULKAN_RENDER_PASS_CREATE_FAILED;
    }
    vk_renderer->render_pass = render_pass;
    return NCBAR_VULKAN_OK;
}

void vulkan_renderer_render_pass_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->render_pass == VK_NULL_HANDLE) {
        return;
    }
    vk_renderer->context->vk_loader_device.vkDestroyRenderPass(vk_renderer->context->device, vk_renderer->render_pass, NULL);
    vk_renderer->render_pass = VK_NULL_HANDLE;
}