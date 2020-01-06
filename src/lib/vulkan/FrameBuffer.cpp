/*
 * FrameBuffer.cpp
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#include "vulkan.h"
#include "FrameBuffer.h"
#include "helper.h"
#include <iostream>

namespace vulkan {


DepthBuffer::DepthBuffer(int w, int h, VkFormat _format, bool _with_sampler) {
	with_sampler = _with_sampler;
	create(w, h, _format);
}

void DepthBuffer::create(int w, int h, VkFormat _format) {
	width = w;
	height = h;
	depth = 1;
	mip_levels = 1;
	format = _format;

	auto usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (!with_sampler)
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	create_image(width, height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
	view = create_image_view(image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	transition_image_layout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);


	if (with_sampler)
		_create_sampler();
}




FrameBuffer::FrameBuffer(int w, int h, RenderPass *rp, const Array<VkImageView> &attachments) {
	frame_buffer = nullptr;
	create(w, h, rp, attachments);
}

FrameBuffer::~FrameBuffer() {
	destroy();
}


void FrameBuffer::create(int w, int h, RenderPass *rp, const Array<VkImageView> &attachments) {
	width = w;
	height = h;

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = rp->render_pass;
	info.attachmentCount = attachments.num;
	info.pAttachments = &attachments[0];
	info.width = w;
	info.height = h;
	info.layers = 1;

	if (vkCreateFramebuffer(device, &info, nullptr, &frame_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void FrameBuffer::destroy() {
	if (frame_buffer)
		vkDestroyFramebuffer(device, frame_buffer, nullptr);
	frame_buffer = nullptr;
}


} /* namespace vulkan */