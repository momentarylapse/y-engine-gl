/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "../y/EngineData.h"
#include "../lib/math/rect.h"
#include "../helper/PerformanceMonitor.h"
#include "world/WorldRenderer.h"

#include "../lib/os/msg.h"


const RenderParams RenderParams::WHATEVER = {};

RenderParams RenderParams::with_target(FrameBuffer *fb) const {
	RenderParams r = *this;
	r.frame_buffer = fb;
	r.target_is_window = false;
	return r;
}

RenderParams RenderParams::into_window(FrameBuffer *frame_buffer, float aspect_ratio) {
	return {aspect_ratio, true, frame_buffer};

}
RenderParams RenderParams::into_texture(FrameBuffer *frame_buffer, float aspect_ratio) {
	return {aspect_ratio, false, frame_buffer};
}


Renderer::Renderer(const string &name, Renderer *_parent) {
	width = engine.width;
	height = engine.height;
	parent = _parent;
	if (parent) {
		width = parent->width;
		height = parent->height;
		parent->add_child(this);
	}

	channel = PerformanceMonitor::create_channel(name, parent ? parent->channel : -1);
	//ch_render = PerformanceMonitor::create_channel("render");
	//ch_end = PerformanceMonitor::create_channel("end", ch_render);
	msg_write("NEW: " + name);
	msg_write(p2s(engine.context));
	msg_write(p2s(engine.resource_manager));
	context = engine.context;
	resource_manager = engine.resource_manager;
}


Renderer::~Renderer() {
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}

void Renderer::add_child(Renderer *child) {
	children.add(child);
}

void Renderer::prepare(const RenderParams& params) {
	for (auto c: children)
		c->prepare(params);
}

color Renderer::background() const {
	return Red;
}

FrameBuffer *Renderer::frame_buffer() const {
	if (!parent)
		return nullptr;
	return parent->frame_buffer();
}

DepthBuffer *Renderer::depth_buffer() const {
	if (!parent)
		return nullptr;
	return parent->depth_buffer();
}

#ifdef USING_VULKAN
RenderPass *Renderer::render_pass() const {
	if (!parent)
		return nullptr;
	return parent->render_pass();
}

CommandBuffer *Renderer::command_buffer() const {
	if (!parent)
		return nullptr;
	return parent->command_buffer();
}
#endif


/*bool Renderer::rendering_into_window() const {
	if (!parent)
		return false;
	return parent->forwarding_into_window();
}

bool Renderer::forwarding_into_window() const {
	if (!parent)
		return false;
	return parent->forwarding_into_window();
}*/
