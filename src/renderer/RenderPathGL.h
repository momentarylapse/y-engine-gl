/*
 * RenderPathGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "RenderPath.h"
#include "../lib/base/pointer.h"

namespace nix {
	class FrameBuffer;
	class Texture;
	class DepthBuffer;
	class CubeMap;
	class VertexBuffer;
	class UniformBuffer;
}
class Material;
class UBOLight;
class GLFWwindow;

namespace kaba {
	class Function;
}

typedef nix::FrameBuffer* post_process_func_t(nix::FrameBuffer* cur);
struct PostProcessor {
	post_process_func_t *func;
};


class RenderPathGL : public RenderPath {
public:
	int width, height;
	GLFWwindow* window;
	shared<nix::Texture> tex_black;
	shared<nix::Texture> tex_white;
	shared<nix::FrameBuffer> fb_main;
	shared<nix::FrameBuffer> fb_small1;
	shared<nix::FrameBuffer> fb_small2;
	shared<nix::FrameBuffer> fb2;
	shared<nix::FrameBuffer> fb3;
	shared<nix::FrameBuffer> fb_shadow;
	shared<nix::FrameBuffer> fb_shadow2;
	nix::DepthBuffer *depth_buffer = nullptr;
	shared<nix::Shader> shader_blur;
	shared<nix::Shader> shader_depth;
	shared<nix::Shader> shader_out;
	shared<nix::Shader> shader_fx;
	//shared<nix::Shader> shader_3d;
	shared<nix::Shader> shader_shadow;
	shared<nix::Shader> shader_resolve_multisample;

	Array<UBOLight> lights;
	nix::UniformBuffer *ubo_light;
	PerformanceMonitor *perf_mon;
	nix::VertexBuffer *vb_2d;

	shared<nix::DepthBuffer> depth_cube;
	shared<nix::FrameBuffer> fb_cube;
	shared<nix::CubeMap> cube_map;

	//Camera *shadow_cam;
	matrix shadow_proj;
	int shadow_index;

	float shadow_box_size;
	int shadow_resolution;

	RenderPathGL(GLFWwindow* win, int w, int h, PerformanceMonitor *pm);

	virtual void render_into_texture(nix::FrameBuffer *fb, Camera *cam) = 0;
	void render_into_cubemap(nix::DepthBuffer *fb, nix::CubeMap *cube, const vector &pos);

	void start_frame() override;
	void end_frame() override;

	void process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, const complex &axis);
	void process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, const complex &axis);
	void process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader);
	nix::FrameBuffer* do_post_processing(nix::FrameBuffer *source);
	nix::FrameBuffer* resolve_multisampling(nix::FrameBuffer *source);
	void set_material(Material *m);
	void set_textures(const Array<nix::Texture*> &tex);
	void draw_gui(nix::FrameBuffer *source);
	void render_out(nix::FrameBuffer *source, nix::Texture *bloom);

	nix::FrameBuffer *next_fb(nix::FrameBuffer *cur);


	Array<PostProcessor> post_processors;
	void kaba_add_post_processor(kaba::Function *f);
};



