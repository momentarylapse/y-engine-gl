/*
 * DescriptorSet.cpp
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */


#if HAS_LIB_VULKAN


#include "DescriptorSet.h"
#include "AccelerationStructure.h"
#include "Buffer.h"
#include "Device.h"
#include "Shader.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"

namespace vulkan {

const string DESCRIPTOR_NAME_UNIFORM_BUFFER = "buffer";
const string DESCRIPTOR_NAME_UNIFORM_BUFFER_DYNAMIC = "dbuffer";
const string DESCRIPTOR_NAME_SAMPLER = "sampler";
const string DESCRIPTOR_NAME_STORAGE_IMAGE = "image";
const string DESCRIPTOR_NAME_ACCELERATION_STRUCTURE = "acceleration-structure";

VkDescriptorType descriptor_type(const string &s) {
	if (s == DESCRIPTOR_NAME_UNIFORM_BUFFER)
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	if (s == DESCRIPTOR_NAME_UNIFORM_BUFFER_DYNAMIC)
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	if (s == "storage-buffer")
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	if (s == DESCRIPTOR_NAME_SAMPLER)
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	if (s == DESCRIPTOR_NAME_STORAGE_IMAGE)
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	if (s == DESCRIPTOR_NAME_ACCELERATION_STRUCTURE)
		return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	throw Exception("unknown type: " + s);
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

	DescriptorPool::DescriptorPool(const string &s, int max_sets) {
		Array<VkDescriptorPoolSize> pool_sizes;
		for (auto &xx: s.explode(",")) {
			auto y = xx.explode(":");
			pool_sizes.add({descriptor_type(y[0]), (unsigned)y[1]._int()});
		}


		VkDescriptorPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = static_cast<uint32_t>(pool_sizes.num);
		info.pPoolSizes = &pool_sizes[0];
		info.maxSets = max_sets;

		if (vkCreateDescriptorPool(default_device->device, &info, nullptr, &pool) != VK_SUCCESS) {
			throw Exception("failed to create descriptor pool!");
		}
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(default_device->device, pool, nullptr);
	}

	void DescriptorPool::__init__(const string &s, int max_sets) {
		new(this) DescriptorPool(s, max_sets);
	}
	void DescriptorPool::__delete__() {
		this->~DescriptorPool();
	}

	DescriptorSet *DescriptorPool::create_set(const string &s) {
		Array<VkDescriptorType> types;
		Array<int> binding_no;
		DescriptorSet::digest_bindings(s, types, binding_no);

		auto layout = DescriptorSet::create_layout(types, binding_no);
		return create_set_from_layout(layout);
	}

	DescriptorSet *DescriptorPool::create_set_from_layout(VkDescriptorSetLayout layout) {
		return new DescriptorSet(this, layout);
	}

	Array<DescriptorSet*> descriptor_sets;

	DescriptorSet::DescriptorSet(DescriptorPool *pool, VkDescriptorSetLayout _layout) {
		layout = _layout;

		num_dynamic_ubos = 0;
		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = pool->pool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

		if (vkAllocateDescriptorSets(default_device->device, &info, &descriptor_set) != VK_SUCCESS) {
			throw Exception("failed to allocate descriptor sets!");
		}
	}
	DescriptorSet::~DescriptorSet() {
		// no VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT...
		//vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
		destroy_layout(layout);
	}

	void DescriptorSet::__delete__() {
		this->~DescriptorSet();
	}

	template<class T>
	T &get_for_binding(Array<T> &array, int binding, VkDescriptorType type) {
		for (auto &x: array)
			if (x.binding == binding)
				return x;
		array.add({{}, binding, type});
		return array.back();
	}

	void DescriptorSet::set_texture(int binding, Texture *t) {
		auto &i = get_for_binding(images, binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		i.info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		i.info.imageView = t->view;
		i.info.sampler = t->sampler;
	}

	void DescriptorSet::set_storage_image(int binding, Texture *t) {
		//msg_error("storage image");
		auto &i = get_for_binding(images, binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		i.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		i.info.imageView = t->view;
		i.info.sampler = VK_NULL_HANDLE; //t->sampler;
	}

	void DescriptorSet::set_buffer_with_offset(int binding, Buffer *u, int offset) {
		auto type = /*u->is_dynamic() ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :*/ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		auto &i = get_for_binding(buffers, binding, type);
		i.info.buffer = u->buffer;
		i.info.offset = offset;
		i.info.range = u->size;
	}

	void DescriptorSet::set_storage_buffer(int binding, Buffer *u) {
		auto type = /*u->is_dynamic() ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :*/ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		auto &i = get_for_binding(buffers, binding, type);
		i.info.buffer = u->buffer;
		i.info.offset = 0;
		i.info.range = u->size;
	}

	void DescriptorSet::set_acceleration_structure(int binding, AccelerationStructure *a) {
		auto &i = get_for_binding(accelerations, binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);
	    i.info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	    i.info.accelerationStructureCount = 1;
	    i.info.pAccelerationStructures = &a->structure;
	}

	void DescriptorSet::set_buffer(int binding, Buffer *u) {
		set_buffer_with_offset(binding, u, 0);
	}

	void DescriptorSet::update() {

		//std::cout << "update dset with " << buffers.num << " buffers, " << images.num << " images\n";

		Array<VkWriteDescriptorSet> wds;
		for (auto &b: buffers) {
			VkWriteDescriptorSet w;
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = b.binding;
			w.dstArrayElement = 0;
			w.descriptorType = b.type;
			w.descriptorCount = 1;
			w.pBufferInfo = &b.info;
			wds.add(w);
		}

		for (auto &i: images) {
			VkWriteDescriptorSet w;
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = i.binding;
			w.dstArrayElement = 0;
			w.descriptorType = i.type;
			w.descriptorCount = 1;
			w.pImageInfo = &i.info;
			wds.add(w);
		}
		for (auto &a: accelerations) {
			VkWriteDescriptorSet w;
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = a.binding;
			w.dstArrayElement = 0;
			w.descriptorType = a.type;
			w.descriptorCount = 1;
			w.pNext = &a.info;
			wds.add(w);
		}

		vkUpdateDescriptorSets(default_device->device, static_cast<uint32_t>(wds.num), &wds[0], 0, nullptr);
	}

	VkDescriptorSetLayout DescriptorSet::create_layout(const Array<VkDescriptorType> &types, const Array<int> &binding_no) {
		//std::cout << "create dset layout, " << num_ubos << " ubos, " << num_samplers << " samplers\n";
		Array<VkDescriptorSetLayoutBinding> bindings;
		for (int i=0; i<types.num;i++) {
			VkDescriptorSetLayoutBinding lb = {};
			lb.descriptorType = types[i];
			lb.descriptorCount = 1;
			lb.binding = binding_no[i];
			lb.pImmutableSamplers = nullptr;
			if (types[i] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				lb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
			} else {
				lb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV;
			}
			bindings.add(lb);
		}


		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = bindings.num;
		info.pBindings = &bindings[0];

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(default_device->device, &info, nullptr, &layout) != VK_SUCCESS) {
			throw Exception("failed to create descriptor set layout!");
		}
		return layout;
	}

	void DescriptorSet::destroy_layout(VkDescriptorSetLayout layout) {
		vkDestroyDescriptorSetLayout(default_device->device, layout, nullptr);
	}



	Array<VkDescriptorSetLayout> DescriptorSet::parse_bindings(const string &bindings) {
		Array<VkDescriptorSetLayout> rr;
		int i0 = 1;
		while (i0 < bindings.num) {
			int i1 = bindings.find("[", i0);
			if (i1 < 0)
				break;
			int i2 = bindings.find("]", i1);
			if (i2 < 0)
				break;
			string bb = bindings.sub(i1+1, i2);
			Array<VkDescriptorType> types;
			Array<int> binding_no;
			digest_bindings(bb, types, binding_no);
			rr.add(DescriptorSet::create_layout(types, binding_no));

			i0 = i2 + 1;
		}
		return rr;
	}

	void DescriptorSet::digest_bindings(const string &bindings, Array<VkDescriptorType> &types, Array<int> &binding_no) {
		auto x = bindings.explode(",");
		//int num_samplers = 0;
		int cur_binding = 0;
		for (auto &y: x) {
			if (y == "" or y == ".") {
				cur_binding ++;
			} else {
				types.add(descriptor_type(y));
				binding_no.add(cur_binding ++);
			}
		}
	}


};

#endif


