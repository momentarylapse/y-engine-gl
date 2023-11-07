/*
 * ShadowRendererVulkan.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRendererVulkan.h"

#include <GLFW/glfw3.h>
#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../geometry/GeometryRendererVulkan.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../world/Light.h"
#include "../../../world/Material.h"
#include "../../../Config.h"
#include <lib/nix/nix.h>
#include <helper/PerformanceMonitor.h>


ShadowRendererVulkan::ShadowRendererVulkan(Renderer *parent) : Renderer("shdw", parent) {
	int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	auto tex1 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto tex2 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto depth1 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	auto depth2 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	_render_pass = new vulkan::RenderPass({tex1, depth1}, "clear");
	fb[0] = new vulkan::FrameBuffer(_render_pass, {tex1, depth1});
	fb[1] = new vulkan::FrameBuffer(_render_pass, {tex2, depth2});


	rvd[0].ubo_light = new UniformBuffer(3 * sizeof(UBOLight)); // just to fill the dset
	rvd[1].ubo_light = new UniformBuffer(3 * sizeof(UBOLight));

	material = new Material(resource_manager);
	material->pass0.shader_path = "shadow.shader";



	geo_renderer = new GeometryRendererVulkan(RenderPathType::FORWARD, scene_view, this);
	geo_renderer->flags = GeometryRenderer::Flags::SHADOW_PASS;
	geo_renderer->material_shadow = material;
	//scene_view.ubo_light = new UniformBuffer(8); // dummy
}

void ShadowRendererVulkan::render(vulkan::CommandBuffer *cb, SceneView &parent_scene_view) {
	scene_view.cam = parent_scene_view.cam;
	proj = parent_scene_view.shadow_proj;
	auto params = RenderParams::WHATEVER;
	params.command_buffer = cb;
	params.render_pass = _render_pass;
	prepare(params);
}

void ShadowRendererVulkan::prepare(const RenderParams& params) {
	auto cb = params.command_buffer;

	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(cb, ch_prepare);
	render_shadow_map(cb, fb[0].get(), 4, rvd[0]);
	render_shadow_map(cb, fb[1].get(), 1, rvd[1]);
	gpu_timestamp_end(cb, ch_prepare);
	PerformanceMonitor::end(ch_prepare);
}

void ShadowRendererVulkan::render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) {
	geo_renderer->prepare(RenderParams::into_texture(sfb, 1.0f));

	cb->begin_render_pass(_render_pass, sfb);
	cb->set_viewport(rect(0, sfb->width, 0, sfb->height));

	//shadow_pass->set(shadow_proj, scale, &rvd);
	//shadow_pass->draw();

	auto m = mat4::scale(scale, -scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);

	UBO ubo;
	ubo.p = m * proj;
	ubo.v = mat4::ID;
	ubo.num_lights = 0;
	ubo.shadow_index = -1;

	geo_renderer->draw_terrains(cb, _render_pass, ubo, rvd);
	geo_renderer->draw_objects_opaque(cb, _render_pass, ubo, rvd);
	geo_renderer->draw_objects_instanced(cb, _render_pass, ubo, rvd);
	geo_renderer->draw_user_meshes(cb, _render_pass, ubo, false, rvd);


	cb->end_render_pass();
}

#endif
