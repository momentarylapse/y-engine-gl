/*
 * WorldRendererVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkanForward.h"
#ifdef USING_VULKAN
#include "pass/ShadowPassVulkan.h"
#include "../../graphics-impl.h"
#include "../base.h"
#include "../../lib/os/msg.h"

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../helper/Scheduler.h"
#include "../../plugins/PluginManager.h"
#include "../../gui/gui.h"
#include "../../gui/Node.h"
#include "../../gui/Picture.h"
#include "../../gui/Text.h"
#include "../../fx/Particle.h"
#include "../../fx/Beam.h"
#include "../../fx/ParticleManager.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../y/Entity.h"
#include "../../Config.h"
#include "../../meta.h"


WorldRendererVulkanForward::WorldRendererVulkanForward(Renderer *parent, vulkan::Device *_device) : WorldRendererVulkan("fw", parent, RenderPathType::FORWARD) {
	device = _device;


	auto tex1 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto tex2 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto shadow_depth1 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	auto shadow_depth2 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	shadow_pass = new ShadowPassVulkan(this, tex1, shadow_depth1);
	fb_shadow1 = new vulkan::FrameBuffer(shadow_pass->render_pass(), {tex1, shadow_depth1});
	fb_shadow2 = new vulkan::FrameBuffer(shadow_pass->render_pass(), {tex2, shadow_depth2});

	material_shadow = new Material;
	material_shadow->shader_path = "shadow.shader";



	shader_fx = ResourceManager::load_shader("vulkan/3d-fx.shader");
	pipeline_fx = new vulkan::GraphicsPipeline(shader_fx.get(), render_pass(), 0, "triangles", "3f,4f,2f");
	pipeline_fx->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline_fx->set_z(true, false);
	pipeline_fx->set_culling(CullMode::NONE);
	pipeline_fx->rebuild();
}

static int cur_query_offset;

void WorldRendererVulkanForward::prepare() {


	static int pool_no = 0;
	pool_no = (pool_no + 1) % 16;
	cur_query_offset = pool_no * 8;
	device->reset_query_pool(cur_query_offset, 8);

	auto cb = command_buffer();

	cb->timestamp(cur_query_offset + 0);




	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(cb, cube_map.get(), world.ego->pos);
		_frame = 0;
	}
	prepare_lights(cam_main, rvd_def);
	prepare_instanced_matrices();

	if (shadow_index >= 0) {
		render_shadow_map(cb, fb_shadow1.get(), 4, rvd_shadow1);
		render_shadow_map(cb, fb_shadow2.get(), 1, rvd_shadow2);
	}
	cb->timestamp(cur_query_offset + 1);
}

void WorldRendererVulkanForward::draw() {

	auto cb = command_buffer();
	auto rp = render_pass();

	auto &rvd = rvd_def;

	draw_skyboxes(cb, rp, cam_main, (float)width / (float)height, rvd);

	UBO ubo;
	ubo.p = cam_main->m_projection;
	ubo.v = cam_main->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	draw_terrains(cb, rp, ubo, true, rvd);
	draw_objects_opaque(cb, rp, ubo, true, rvd);
	draw_objects_instanced(cb, rp, ubo, true, rvd);
	draw_objects_transparent(cb, rp, ubo, rvd);

	draw_particles(cb, rp, cam_main, rvd);

	cb->timestamp(cur_query_offset + 2);
}

void WorldRendererVulkanForward::render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) {

	prepare_lights(cam, rvd);

	rp->clear_color[0] = background();

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(rect(0, fb->width, 0, fb->height));


	draw_skyboxes(cb, rp, cam, (float)fb->width / (float)fb->height, rvd);

	UBO ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	draw_terrains(cb, rp, ubo, true, rvd);
	draw_objects_opaque(cb, rp, ubo, true, rvd);
	draw_objects_transparent(cb, rp, ubo, rvd);

	draw_particles(cb, rp, cam, rvd);

	cb->end_render_pass();

}

void WorldRendererVulkanForward::render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) {

	cb->begin_render_pass(shadow_pass->render_pass(), sfb);
	cb->set_viewport(rect(0, sfb->width, 0, sfb->height));

	shadow_pass->set(shadow_proj, scale, &rvd);
	shadow_pass->draw();

	cb->end_render_pass();
}

#endif

