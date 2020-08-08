/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

#include "RenderPathGL.h"
#include "../lib/nix/nix.h"

#include "../helper/PerformanceMonitor.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../world/camera.h"
#include "../world/material.h"
#include "../world/world.h"
#include "../world/model.h"
#include "../world/terrain.h"
#include "../Config.h"
#include "../meta.h"

extern bool SHOW_SHADOW;

void break_point() {
	if (config.debug) {
		glFlush();
		glFinish();
	}
}

RenderPathGL::RenderPathGL(GLFWwindow* w, PerformanceMonitor *pm) {
	window = w;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &width, &height);

	perf_mon = pm;

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);
	SHOW_SHADOW = config.get_bool("shadow.debug", false);

	nix::Init();

	fb = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"),
		new nix::DepthBuffer(width, height)});
	fb2 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb3 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb4 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb5 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb_shadow = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});

	try {
		auto sd = nix::shader_dir;
		shader_blur = nix::Shader::load(hui::Application::directory_static << "forward/blur.shader");
		shader_depth = nix::Shader::load(hui::Application::directory_static << "forward/depth.shader");
		shader_out = nix::Shader::load(hui::Application::directory_static << "forward/hdr.shader");
		shader_3d = nix::Shader::load(hui::Application::directory_static << "forward/3d.shader");
		shader_fx = nix::Shader::load(hui::Application::directory_static << "forward/3d-fx.shader");
		//nix::default_shader_3d = shader_3d;
		shader_shadow = nix::Shader::load(hui::Application::directory_static << "forward/3d-shadow.shader");
		shader_skybox = nix::Shader::load(hui::Application::directory_static << "forward/3d-skybox.shader");
		nix::shader_dir = sd;
	} catch(Exception &e) {
		msg_error(e.message());
		throw e;
	}
	ubo_light = new nix::UniformBuffer();
	tex_white = new nix::Texture();
	tex_black = new nix::Texture();
	Image im;
	im.create(16, 16, White);
	tex_white->overwrite(im);
	im.create(16, 16, Black);
	tex_black->overwrite(im);

	vb_2d = new nix::VertexBuffer("3f,3f,2f");
	vb_2d->create_rect(rect(-1,1, -1,1));

	AllowXContainer = false;
	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);
	AllowXContainer = true;
}
void RenderPathGL::draw() {
	nix::StartFrameGLFW(window);

	_correct_material_shaders();

	prepare_lights();
	perf_mon->tick(0);

	render_shadow_map(fb_shadow, 4);
	render_shadow_map(fb_shadow2, 1);

	render_into_texture(fb);


	auto *source = fb;
	if (cam->focus_enabled) {
		process_depth(source, fb4, fb->depth_buffer, true);
		process_depth(fb4, fb5, fb->depth_buffer, false);
		source = fb5;
	}
	process_blur(source, fb2, 1.0f, true);
	process_blur(fb2, fb3, 0.0f, false);


	nix::BindFrameBuffer(nix::FrameBuffer::DEFAULT);

	render_out(source, fb3->color_attachments[0]);

	draw_gui(source);

	nix::EndFrameGLFW(window);
	perf_mon->tick(5);
}

void RenderPathGL::process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, bool horizontal) {

	nix::SetShader(shader_blur);
	float r = cam->bloom_radius;
	complex ax;
	if (horizontal) {
		ax = complex(2,0);
	} else {
		ax = complex(0,1);
		//r /= 2;
	}
	shader_blur->set_float(shader_blur->get_location("radius"), r);
	shader_blur->set_float(shader_blur->get_location("threshold"), threshold / cam->exposure);
	shader_blur->set_data(shader_blur->get_location("axis"), &ax.x, 8);
	process(source->color_attachments, target, shader_blur);
}

void RenderPathGL::process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, nix::Texture *depth_buffer, bool horizontal) {

	nix::SetShader(shader_depth);
	complex ax = complex(1,0);
	if (!horizontal) {
		ax = complex(0,1);
	}
	shader_depth->set_float(shader_depth->get_location("max_radius"), 50);
	shader_depth->set_float(shader_depth->get_location("focal_length"), cam->focal_length);
	shader_depth->set_float(shader_depth->get_location("focal_blur"), cam->focal_blur);
	shader_depth->set_data(shader_depth->get_location("axis"), &ax.x, 8);
	shader_depth->set_matrix(shader_depth->get_location("invproj"), cam->m_projection.inverse());
	process({source->color_attachments[0], depth_buffer}, target, shader_depth);
}

void RenderPathGL::process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader) {
	nix::BindFrameBuffer(target);
	nix::SetZ(false, false);
	nix::SetProjectionOrtho(true);
	nix::SetViewMatrix(matrix::ID);
	nix::SetWorldMatrix(matrix::ID);
	//nix::SetShader(shader);

	nix::SetTextures(source);
	nix::DrawTriangles(vb_2d);
}

void RenderPathGL::draw_gui(nix::FrameBuffer *source) {
	nix::SetProjectionOrtho(true);
	nix::SetCull(CULL_NONE);
	nix::SetShader(gui::shader);
	nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);

	for (auto *p: gui::pictures) {
		gui::shader->set_float(gui::shader->get_location("blur"), p->bg_blur);
		gui::shader->set_color(gui::shader->get_location("color"), p->col);
		nix::SetTextures({p->texture, source->color_attachments[0]});
		nix::SetWorldMatrix(matrix::translation(vector(p->dest.x1, p->dest.y1, p->z)) * matrix::scale(p->dest.width(), p->dest.height(), 0));
		nix::DrawTriangles(gui::vertex_buffer);
	}
	nix::SetCull(CULL_DEFAULT);

	nix::SetAlpha(ALPHA_NONE);

	break_point();
	perf_mon->tick(4);
}

void RenderPathGL::render_out(nix::FrameBuffer *source, nix::Texture *bloom) {

	nix::SetTextures({source->color_attachments[0], bloom});
	nix::SetShader(shader_out);
	shader_out->set_float(shader_out->get_location("exposure"), cam->exposure);
	shader_out->set_float(shader_out->get_location("bloom_factor"), cam->bloom_factor);
	nix::SetProjectionMatrix(matrix::ID);
	nix::SetViewMatrix(matrix::ID);
	nix::SetWorldMatrix(matrix::ID);

	nix::SetZ(false, false);

	nix::DrawTriangles(vb_2d);

	break_point();
	perf_mon->tick(3);
}

void RenderPathGL::render_into_texture(nix::FrameBuffer *fb) {
	nix::BindFrameBuffer(fb);

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->set_view((float)fb->width / (float)fb->height);
	nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::ResetToColor(world.background);
	nix::ResetZ();

	draw_skyboxes();


	cam->max_depth = max_depth;
	cam->set_view((float)fb->width / (float)fb->height);
	nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::BindUniform(ubo_light, 1);
	nix::SetViewMatrix(cam->m_view);
	nix::SetZ(true, true);

	draw_world(true);

	draw_particles();

	break_point();
	perf_mon->tick(2);
}

void RenderPathGL::draw_particles() {
	matrix r = matrix::rotation_q(cam->ang);
	nix::SetShader(shader_fx);
	nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);
	nix::SetZ(false, true);
	nix::vb_temp->create_rect(rect(-0.5,0.5, -0.5,0.5));
	for (auto g: world.particle_manager->groups) {
		nix::SetTexture(g->texture);
		for (auto p: g->particles) {
			shader_fx->set_color(shader_fx->get_location("color"), p->col);
			shader_fx->set_data(shader_fx->get_location("source"), &p->source.x1, 16);
			nix::SetWorldMatrix(matrix::translation(p->pos) * r * matrix::scale(p->radius, p->radius, p->radius));
			nix::DrawTriangles(nix::vb_temp);
		}
	}
	nix::SetZ(true, true);
	nix::SetAlpha(ALPHA_NONE);
}

void RenderPathGL::_material_set_shader(Material *m, nix::Shader *s) {
	if (m->shader != s) {
		msg_write("SWAP skybox shader");
		if (m->shader)
			m->shader->unref();
		m->shader = s->ref();
	}
}
void RenderPathGL::_correct_material_shaders() {
	for (auto *sb: world.skybox)
		for (int i=0; i<sb->material.num; i++)
			_material_set_shader(sb->material[i], shader_skybox);
}

void RenderPathGL::draw_skyboxes() {
	nix::SetZ(false, false);
	nix::SetCull(CULL_NONE);
	nix::SetViewMatrix(matrix::rotation_q(cam->ang).transpose());
	for (auto *sb: world.skybox) {
		sb->_matrix = matrix::rotation_q(sb->ang);
		nix::SetWorldMatrix(sb->_matrix * matrix::scale(10,10,10));
		for (int i=0; i<sb->material.num; i++) {
			set_material(sb->material[i]);
			nix::DrawTriangles(sb->mesh[0]->sub[i].vertex_buffer);
		}
	}
	nix::SetCull(CULL_DEFAULT);
}
void RenderPathGL::draw_terrains(bool allow_material) {
	for (auto *t: world.terrains) {
		//nix::SetWorldMatrix(matrix::translation(t->pos));
		nix::SetWorldMatrix(matrix::ID);
		if (allow_material)
			set_material(t->material);
		t->draw();
		nix::DrawTriangles(t->vertex_buffer);
	}
}
void RenderPathGL::draw_objects(bool allow_material) {
	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;
		nix::SetWorldMatrix(mtr(m->pos, m->ang));//m->_matrix);
		if (allow_material)
			set_material(s.material);
		//nix::DrawInstancedTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, 200);
		nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
	}
}
void RenderPathGL::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects(allow_material);
}
void RenderPathGL::set_material(Material *m) {
	auto s = m->shader; //shader_3d;
	nix::SetShader(s);
	s->set_data(s->get_location("eye_pos"), &cam->pos.x, 16);
	s->set_int(s->get_location("num_lights"), lights.num);
	if (m->alpha.mode == TRANSPARENCY_FUNCTIONS)
		nix::SetAlpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TRANSPARENCY_COLOR_KEY_HARD)
		nix::SetAlpha(ALPHA_COLOR_KEY_HARD);
	else
		nix::SetAlpha(ALPHA_NONE);

	set_textures(m->textures);
	s->set_data(s->get_location("emission_factor"), &m->emission.r, 16);
}
void RenderPathGL::set_textures(const Array<nix::Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white);
	if (tt.num == 1)
		tt.add(tex_white);
	if (tt.num == 2)
		tt.add(tex_black);
	tt.add(fb_shadow->depth_buffer);
	tt.add(fb_shadow2->depth_buffer);
	nix::SetTextures(tt);
}
void RenderPathGL::prepare_lights() {

	lights.clear();
	for (auto *l: world.lights) {
		if (!l->enabled)
			continue;

		if (l->radius <= 0){
			vector center = cam->pos + cam->ang*vector::EZ * (shadow_box_size / 3.0f);
			float grid = shadow_box_size / 16;
			center.x -= fmod(center.x, grid) - grid/2;
			center.y -= fmod(center.y, grid) - grid/2;
			center.z -= fmod(center.z, grid) - grid/2;
			auto t = matrix::translation(- center);
			auto r = matrix::rotation(l->dir.dir2ang()).transpose();
			float f = 1 / shadow_box_size;
			auto s = matrix::scale(f, f, f);
			// map onto [-1,1]x[-1,1]x[0,1]
			shadow_proj = matrix::translation(vector(0,0,-0.5f)) * s * r * t;
			l->proj = shadow_proj;
		}
		lights.add(*l);
	}
	ubo_light->update_array(lights);
}
void RenderPathGL::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::BindFrameBuffer(sfb);

	nix::SetProjectionMatrix(matrix::scale(scale, scale, 1) * shadow_proj);
	nix::SetViewMatrix(matrix::ID);

	nix::ResetZ();

	nix::SetZ(true, true);
	nix::SetShader(shader_shadow);


	draw_world(false);

	break_point();
	perf_mon->tick(1);
}



