//
// Created by Michael Ankele on 2025-01-08.
//

#include "Raytracing.h"

#include <renderer/world/geometry/SceneView.h>

#include "../base.h"
#include <lib/os/msg.h>
#include <lib/base/iter.h>
#include <renderer/target/WindowRendererVulkan.h>

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../gui/Node.h"
#include "../../fx/Particle.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../y/ComponentManager.h"
#include "../../y/Entity.h"
#include "../../y/EngineData.h"
#include "../../Config.h"
#include "../../graphics-impl.h"
#ifdef USING_VULKAN

static const int MAX_RT_TRIAS = 65536;
static const int MAX_RT_MESHES = 1024;
static const int MAX_RT_REQUESTS = 4096*16;

void rt_setup(SceneView& scene_view) {
	scene_view.ray_tracing_data = new RayTracingData(engine.window_renderer->device);
}

void rt_update_frame(SceneView& scene_view) {
	scene_view.ray_tracing_data->update_frame();
}

RayTracingData::RayTracingData(vulkan::Device *device) {
	auto resource_manager = engine.resource_manager;

	/*if (device->has_rtx() and config.allow_rtx)
		mode = Mode::RTX;
	else*/
	if (device->has_compute())
		mode = Mode::COMPUTE;
	else
		throw Exception("no compute shader support");

	buffer_meshes = new UniformBuffer(sizeof(MeshDescription) * MAX_RT_MESHES);
	buffer_requests = new UniformBuffer(sizeof(RayRequest) * MAX_RT_REQUESTS);
	buffer_reply = new vulkan::StorageBuffer(sizeof(RayReply) * MAX_RT_REQUESTS);

	if (mode == Mode::RTX) {
		msg_error("RTX!!!");
		rtx.pool = new vulkan::DescriptorPool("acceleration-structure:1,image:1,storage-buffer:1,buffer:1024,sampler:1024", 1024);

		//rtx.buffer_cam = new UniformBuffer(sizeof(PushConst));

		rtx.dset = rtx.pool->create_set("acceleration-structure,image,buffer,buffer,buffer,buffer");
		rtx.dset->set_uniform_buffer(5, buffer_meshes.get());

		auto shader_gen = resource_manager->load_shader("vulkan/gen.shader");
		auto shader1 = resource_manager->load_shader("vulkan/group1.shader");
		auto shader2 = resource_manager->load_shader("vulkan/group2.shader");
		rtx.pipeline = new vulkan::RayPipeline("[[acceleration-structure,image,buffer,buffer,buffer,buffer]]", {shader_gen.get(), shader1.get(), shader2.get()}, 2);
		rtx.pipeline->create_sbt();


	} else if (mode == Mode::COMPUTE) {
		//msg_error("COMPUTE!!!");

		compute.pool = new vulkan::DescriptorPool("image:1,storage-buffer:1,buffer:8,sampler:1", 1);

		auto shader = resource_manager->load_shader("compute/raytracing.shader");
		compute.pipeline = new vulkan::ComputePipeline(shader.get());
		compute.dset = compute.pool->create_set("buffer,buffer,storage-buffer");
		compute.dset->set_uniform_buffer(0, buffer_requests.get());
		compute.dset->set_uniform_buffer(1, buffer_meshes.get());
		compute.dset->set_storage_buffer(2, buffer_reply.get());
		compute.dset->update();

		compute.command_buffer = device->command_pool->create_command_buffer();
		compute.fence = new Fence(device);
	}
}


void RayTracingData::update_frame() {

	auto& models = ComponentManager::get_list_family<Model>();
	auto& terrains = ComponentManager::get_list_family<Terrain>();


	Array<MeshDescription> meshes;

	for (auto m: models) {
		m->update_matrix();
		for (int i=0; i<m->material.num; i++) {
			auto material = m->material[i];

			MeshDescription md;
			md.matrix = m->_matrix;
			md.num_triangles = m->mesh[0]->sub[i].triangle_index.num / 3;
			md.albedo = material->albedo.with_alpha(material->roughness);
			md.emission = material->emission.with_alpha(material->metal);
			md.address_vertices = m->mesh[0]->sub[i].vertex_buffer->vertex_buffer.get_device_address();
			//md.address_indices = m->mesh[0]->sub[i].vertex_buffer->index_buffer.get_device_address();
			meshes.add(md);
		}
	}
	for (auto *t: terrains) {
		auto o = t->owner;

		MeshDescription md;
		md.matrix = mat4::translation(o->pos);
		md.albedo = t->material->albedo.with_alpha(t->material->roughness);
		md.emission = t->material->emission.with_alpha(t->material->metal);
		md.num_triangles = t->vertex_buffer->output_count / 3;
		md.address_vertices = t->vertex_buffer->vertex_buffer.get_device_address();
		meshes.add(md);
	}


	buffer_meshes->update_array(meshes, 0);

	num_meshes = meshes.num;



	if (mode == Mode::RTX) {

		Array<mat4> matrices;

		if (rtx.tlas) {
			// update
			for (auto m: models) {
				m->update_matrix();
				for (int i=0; i<m->material.num; i++)
					matrices.add(m->owner->get_matrix().transpose());
			}
			for (auto *t: terrains) {
				auto o = t->owner;
				matrices.add(mat4::translation(o->pos).transpose());
			}
			rtx.tlas->update_top(rtx.blas, matrices);

		} else {

			auto make_indexed = [] (VertexBuffer *vb) {
				if (!vb->is_indexed()) {
					Array<int> index;
					for (int i=0; i<vb->output_count; i++)
						index.add(i);
					vb->update_index(index);
				}
			};

			for (auto m: models) {
				m->update_matrix();
				for (int i=0; i<m->material.num; i++) {
					m->update_matrix();
					auto vb = m->mesh[0]->sub[i].vertex_buffer;
					make_indexed(vb);
					rtx.blas.add(vulkan::AccelerationStructure::create_bottom(device, vb));
					matrices.add(m->owner->get_matrix().transpose());
				}
			}

			for (auto *t: terrains) {
				auto o = t->owner;
				make_indexed(t->vertex_buffer.get());
				rtx.blas.add(vulkan::AccelerationStructure::create_bottom(device, t->vertex_buffer.get()));
				matrices.add(mat4::translation(o->pos).transpose());
			}

			rtx.tlas = vulkan::AccelerationStructure::create_top(device, rtx.blas, matrices);
		}

	} else if (mode == Mode::COMPUTE) {
	}
}

//Array<base::optional<RayHitInfo>>
Array<RayReply> vtrace(SceneView& scene_view, const Array<RayRequest>& requests) {
	if (requests.num > MAX_RT_REQUESTS) {
		msg_error("too many rt requests");
		return {};
	}
	//msg_write("upd " + str(requests.num));
	scene_view.ray_tracing_data->buffer_requests->update_array(requests);
	auto cb = scene_view.ray_tracing_data->compute.command_buffer;
	auto fence = scene_view.ray_tracing_data->compute.fence;
	auto device = engine.window_renderer->device;
	//msg_write("cb");
	cb->begin();
	cb->set_bind_point(vulkan::PipelineBindPoint::COMPUTE);
	cb->bind_pipeline(scene_view.ray_tracing_data->compute.pipeline);
	cb->bind_descriptor_set(0, scene_view.ray_tracing_data->compute.dset);
	int n = 1 + (requests.num - 1) / 256;
	cb->dispatch(n,1,1);
	cb->end();
	//msg_write("submit");
	device->compute_queue.submit(cb, {}, {}, fence);
	//msg_write("wait");

	fence->wait();

	Array<RayReply> replies;
	replies.resize(requests.num);
	auto p = scene_view.ray_tracing_data->buffer_reply->map_part(0, sizeof(RayReply) * requests.num);
	memcpy(replies.data, p, sizeof(RayReply) * requests.num);
	scene_view.ray_tracing_data->buffer_reply->unmap();
	//msg_write(bytes(replies.data, 100).hex());

	return replies;
}
#else

void rt_setup(SceneView& scene_view) {
	msg_error("raytracing only supported on vulkan!");
}

void rt_update_frame(SceneView& scene_view) {
}

Array<RayReply> vtrace(SceneView& scene_view, const Array<RayRequest>& requests) {
	return {};
}
#endif
