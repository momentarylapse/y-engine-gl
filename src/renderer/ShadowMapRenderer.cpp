/*
 * ShadowMapRenderer.cpp
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */

#include "ShadowMapRenderer.h"


ShadowMapRenderer::ShadowMapRenderer() {
	width = 512;
	height = 512;

	depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);

	_default_render_pass = new vulkan::RenderPass({depth_buffer->format}, true, false);
	frame_buffer = new vulkan::FrameBuffer(width, height, _default_render_pass, {depth_buffer->view});


	//shader_into_gbuf = vulkan::Shader::load("3d-multi.shader");
	shader = vulkan::Shader::load("3d-shadow2.shader");
	pipeline = new vulkan::Pipeline(shader, _default_render_pass, 1);

}
ShadowMapRenderer::~ShadowMapRenderer() {
	delete _default_render_pass;
	delete pipeline;
	delete shader;
	delete frame_buffer;
	delete depth_buffer;
}

bool ShadowMapRenderer::start_frame() {
	//in_flight_fence->wait();
	cb->begin();
	return true;

}
void ShadowMapRenderer::end_frame() {
	//cb->barrier({tex_color, tex_emission, tex_normal, tex_pos, depth_buffer}, 0);
	cb->barrier({depth_buffer}, 0);
	cb->end();
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	in_flight_fence->wait();
}
