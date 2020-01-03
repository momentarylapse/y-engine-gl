/*
 * Semaphore.h
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_SEMAPHORE_H_
#define SRC_LIB_VULKAN_SEMAPHORE_H_

#include <vulkan/vulkan.h>

namespace vulkan {

class Fence {
public:
	Fence();
	~Fence();

	void __init__();
	void __delete__();

	VkFence fence;

	void reset();
	void wait();
};

class Semaphore {
public:
	Semaphore();
	~Semaphore();

	void __init__();
	void __delete__();

	VkSemaphore semaphore;
};

} /* namespace vulkan */

#endif /* SRC_LIB_VULKAN_SEMAPHORE_H_ */
