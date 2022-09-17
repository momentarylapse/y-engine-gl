/*
 * Queue.cpp
 *
 *  Created on: Sep 21, 2021
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "Queue.h"
#include "Semaphore.h"
#include "vulkan.h"
#include "helper.h"
#include "common.h"

#include <iostream>
#include <set>

namespace vulkan {


bool Queue::submit(CommandBuffer *cb, const Array<Semaphore*> &wait_sem, const Array<Semaphore*> &signal_sem, Fence *fence) {

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	auto wait_semaphores = extract_semaphores(wait_sem);
	auto signal_semaphores = extract_semaphores(signal_sem);
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = wait_semaphores.num;
	submit_info.pWaitSemaphores = &wait_semaphores[0];
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cb->buffer;
	submit_info.signalSemaphoreCount = signal_semaphores.num;
	submit_info.pSignalSemaphores = &signal_semaphores[0];


	if (fence)
		fence->reset();

	VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence_handle(fence));
	return (result == VK_SUCCESS);
	/*if (result != VK_SUCCESS) {
		std::cerr << " SUBMIT ERROR " << result << "\n";
		throw Exception("failed to submit draw command buffer!");
	}*/
}

void Queue::wait_idle() {
	vkQueueWaitIdle(queue);
}



QueueFamilyIndices QueueFamilyIndices::query(VkPhysicalDevice device, VkSurfaceKHR surface) {

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	Array<VkQueueFamilyProperties> queue_families;
	queue_families.resize(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, &queue_families[0]);

	QueueFamilyIndices indices;
	int i = 0;
	for (const auto& family: queue_families) {
		if (family.queueCount == 0)
			continue;

		if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;

		if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.compute_family = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support)
			indices.present_family = i;

		i ++;
	}

	return indices;
}
bool QueueFamilyIndices::is_complete(Requirements req) const {
	if (req & Requirements::GRAPHICS)
		if (!graphics_family)
			return false;
	if (req & Requirements::COMPUTE)
		if (!compute_family)
			return false;
	if (req & Requirements::PRESENT)
		if (!present_family)
			return false;
	return true;
}


/*bool QueueFamilyIndices::is_complete() {
	return graphics_family.has_value() and present_family.has_value();
}*/



} /* namespace vulkan */

#endif
