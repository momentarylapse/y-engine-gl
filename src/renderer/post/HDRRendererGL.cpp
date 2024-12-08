/*
 * HDRRendererGL.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRendererGL.h"

#include <lib/image/image.h>
#include <renderer/target/TextureRendererGL.h>
#ifdef USING_OPENGL
#include "ThroughShaderRenderer.h"
#include "../base.h"
#include "../helper/ComputeTask.h"
#include <lib/nix/nix.h>
#include <lib/math/vec2.h>
#include <lib/math/rect.h>
#include <lib/os/msg.h>
#include <lib/any/any.h>
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../Config.h"
#include "../../y/EngineData.h"
#include "../../world/Camera.h"

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

static int BLOOM_LEVEL_SCALE = 4;

namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}


HDRRendererGL::HDRRendererGL(Camera *_cam, int width, int height) : PostProcessorStage("hdr") {
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);

	cam = _cam;

	_depth_buffer = new nix::DepthBuffer(width, height, "d24s8");
	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		msg_error("yes msaa");

		auto tex_ms = new nix::TextureMultiSample(width, height, 4, "rgba:f16");
		auto depth_ms = new nix::RenderBuffer(width, height, 4, "d24s8");
		// _depth_buffer
		texture_renderer = new TextureRenderer({tex_ms, depth_ms});
		fb_main_ms = texture_renderer->frame_buffer;

		shader_resolve_multisample = resource_manager->load_shader("forward/resolve-multisample.shader");

		fb_main = new nix::FrameBuffer({
				new nix::Texture(width, height, "rgba:f16"),
				_depth_buffer});
	} else {
		msg_error("no msaa");

		auto tex = new nix::Texture(width, height, "rgba:f16");
		texture_renderer = new TextureRenderer({tex, _depth_buffer});
		fb_main = texture_renderer->frame_buffer;
	}

	shader_blur = resource_manager->load_shader("forward/blur.shader");
	int bloomw = width, bloomh = height;
	for (int i=0; i<MAX_BLOOM_LEVELS; i++) {
		auto& bl = bloom_levels[i];
		bloomw /= BLOOM_LEVEL_SCALE;
		bloomh /= BLOOM_LEVEL_SCALE;
		bl.tex_temp = new nix::Texture(bloomw, bloomh, "rgba:f16");
		bl.tex_out = new nix::Texture(bloomw, bloomh, "rgba:f16");
		bl.tex_temp->set_options("wrap=clamp");
		bl.tex_out->set_options("wrap=clamp");
	//	bl.tsr[0] = new ThroughShaderRenderer({bl.tex_temp}, shader_blur);
	//	bl.tsr[1] = new ThroughShaderRenderer({bl.tex_temp}, shader_blur);
		bl.renderer[0] = new TextureRenderer({bl.tex_temp});
		bl.renderer[1] = new TextureRenderer({bl.tex_out});
		bl.fb_temp = bl.renderer[0]->frame_buffer;
		bl.fb_out = bl.renderer[1]->frame_buffer;
	}

	fb_main->color_attachments[0]->set_options("wrap=clamp,minfilter=nearest");
	fb_main->color_attachments[0]->set_options("magfilter=" + config.resolution_scale_filter);

	vb_2d = new nix::VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);


	shader_out = resource_manager->load_shader("forward/hdr.shader");
	out_renderer = new ThroughShaderRenderer({fb_main->color_attachments[0], bloom_levels[0].tex_out, bloom_levels[1].tex_out, bloom_levels[2].tex_out, bloom_levels[3].tex_out}, shader_out);

	light_meter.init(resource_manager, fb_main.get(), channel);
}

HDRRendererGL::~HDRRendererGL() = default;

void HDRRendererGL::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	if (!cam)
		cam = cam_main;

	for (auto c: children)
		c->prepare(params);

	texture_renderer->children = children;
	auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
	texture_renderer->render(scaled_params);

	vb_2d->create_quad(rect::ID_SYM, dynamicly_scaled_source());
	out_renderer->set_source(dynamicly_scaled_source());

	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		// resolve
		if (true) {
			shader_resolve_multisample->set_float("width", fb_main_ms->width);
			shader_resolve_multisample->set_float("height", fb_main_ms->height);
			process({fb_main_ms->color_attachments[0].get()}, fb_main.get(), shader_resolve_multisample.get());
		} else {
			// not sure, why this does not work... :(
			nix::resolve_multisampling(fb_main.get(), fb_main_ms.get());
		}
	}

	PerformanceMonitor::begin(ch_post_blur);
	gpu_timestamp_begin(ch_post_blur);
	//float r = cam->bloom_radius * engine.resolution_scale_x;
	float r = 3;//max(5 * engine.resolution_scale_x, 2.0f);
	auto bloom_input = fb_main.get();
	float threshold = 1.0f;
	for (int i=0; i<4; i++) {
		process_blur(bloom_input, bloom_levels[i].fb_temp.get(), r*BLOOM_LEVEL_SCALE, threshold, {1,0});
		process_blur(bloom_levels[i].fb_temp.get(), bloom_levels[i].fb_out.get(), r, 0.0f, {0,1});
		bloom_input = bloom_levels[i].fb_out.get();
		r = 3;//max(5 * engine.resolution_scale_x, 3.0f);
		threshold = 0;
	}
	//glGenerateTextureMipmap(fb_small2->color_attachments[0]->texture);
	gpu_timestamp_end(ch_post_blur);
	PerformanceMonitor::end(ch_post_blur);

	light_meter.measure(fb_main.get());
	if (cam->auto_exposure)
		light_meter.adjust_camera(cam);

	PerformanceMonitor::end(ch_prepare);
}

void HDRRendererGL::draw(const RenderParams& params) {
	Any data;
	data.dict_set("exposure", cam->exposure);
	data.dict_set("bloom_factor", cam->bloom_factor);
	data.dict_set("scale_x", resolution_scale_x);
	data.dict_set("scale_y", resolution_scale_y);

	out_renderer->data = data;
	out_renderer->draw(params);
}

void HDRRendererGL::process_blur(FrameBuffer *source, FrameBuffer *target, float r, float threshold, const vec2 &axis) {
	shader_blur->set_float("radius", r);
	shader_blur->set_float("threshold", threshold / cam->exposure);
	shader_blur->set_floats("axis", &axis.x, 2);
	process(weak(source->color_attachments), target, shader_blur.get());
}

void HDRRendererGL::process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader) {
	nix::bind_frame_buffer(target);
	nix::set_viewport(dynamicly_scaled_area(target));
	//nix::set_scissor(rect(0, target->width*resolution_scale_x, 0, target->height*resolution_scale_y));
	nix::set_z(false, false);
	//nix::set_projection_ortho_relative();
	//nix::set_view_matrix(matrix::ID);
	//nix::set_model_matrix(matrix::ID);
	shader->set_floats("resolution_scale", &resolution_scale_x, 2);
	nix::set_shader(shader);

	nix::bind_textures(source);
	nix::draw_triangles(vb_2d.get());
	//nix::set_scissor(rect::EMPTY);
}


void HDRRendererGL::LightMeter::init(ResourceManager* resource_manager, FrameBuffer* frame_buffer, int channel) {
	ch_post_brightness = PerformanceMonitor::create_channel("expo", channel);
	compute = new ComputeTask(resource_manager->load_shader("compute/brightness.shader"));
	params = new UniformBuffer();
	buf = new ShaderStorageBuffer();
	compute->bind_texture(0, frame_buffer->color_attachments[0].get());
	compute->bind_storage_buffer(1, buf);
	compute->bind_uniform_buffer(2, params);
}

void HDRRendererGL::LightMeter::measure(FrameBuffer* frame_buffer) {
	PerformanceMonitor::begin(ch_post_brightness);
	gpu_timestamp_begin(ch_post_brightness);

	int NBINS = 256;
	histogram.resize(NBINS);
	memset(&histogram[0], 0, NBINS * sizeof(int));
	buf->update(&histogram[0], NBINS * sizeof(int));

	int pp[2] = {frame_buffer->width, frame_buffer->height};
	params->update(&pp, sizeof(pp));
	const int NSAMPLES = 256;
	compute->dispatch(NSAMPLES, 1, 1);

	buf->read(&histogram[0], NBINS*sizeof(int));
	//msg_write(str(histogram));

	gpu_timestamp_end(ch_post_brightness);

	/*int s = 0;
	for (int i=0; i<NBINS; i++)
		s += histogram[i];
	msg_write(format("%d  %d", s, NSAMPLES*256));*/

	int thresh = (NSAMPLES * 16 * 16) / 200 * 199;
	int n = 0;
	int ii = 0;
	for (int i=0; i<NBINS; i++) {
		n += histogram[i];
		if (n > thresh) {
			ii = i;
			break;
		}
	}
	brightness = pow(2.0f, ((float)ii / (float)NBINS) * 20.0f - 10.0f);
	PerformanceMonitor::end(ch_post_brightness);
}

void HDRRendererGL::LightMeter::adjust_camera(Camera *cam) {
	float exposure = clamp(pow(1.0f / brightness, 0.8f), cam->auto_exposure_min, cam->auto_exposure_max);
	if (exposure > cam->exposure)
		cam->exposure *= 1.05f;
	if (exposure < cam->exposure)
		cam->exposure /= 1.05f;
}



#endif
