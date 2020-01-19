/*
 * RenderPath.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "RenderPath.h"
#include "Renderer.h"
#include "ShadowMapRenderer.h"
#include "../lib/vulkan/vulkan.h"
#include "../world/world.h"
#include "../world/terrain.h"
#include "../world/model.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../gui/Picture.h"


extern vulkan::Shader *shader_fx;
extern vulkan::Texture *tex_white;
extern vulkan::Texture *tex_black;

struct GeoPush {
	alignas(16) matrix model;
	alignas(16) color emission;
	alignas(16) vector eye_pos;
	alignas(16) float xxx[4];
};


matrix mtr(const vector &t, const quaternion &a) {
	auto mt = matrix::translation(t);
	auto mr = matrix::rotation_q(a);
	return mt * mr;
}

RenderPath::RenderPath(Renderer *r, PerformanceMonitor *pm, const string &shadow_shader_filename) {
	renderer = r;
	perf_mon = pm;


	shader_fx = vulkan::Shader::load("fx.shader");
	pipeline_fx = new vulkan::Pipeline(shader_fx, renderer->default_render_pass(), 0, 1);
	pipeline_fx->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeline_fx->set_z(true, false);
	pipeline_fx->set_culling(0);
	pipeline_fx->rebuild();

	particle_vb = new vulkan::VertexBuffer();
	Array<vulkan::Vertex1> vertices;
	vertices.add({vector(-1,-1,0), vector::EZ, 0,0});
	vertices.add({vector(-1, 1,0), vector::EZ, 0,1});
	vertices.add({vector( 1,-1,0), vector::EZ, 1,0});
	vertices.add({vector( 1, 1,0), vector::EZ, 1,1});
	particle_vb->build1i(vertices, {0,2,1, 1,2,3});


	shadow_renderer = new ShadowMapRenderer(shadow_shader_filename);


	AllowXContainer = false;
	light_cam = new Camera(v_0, quaternion::ID, rect::ID);
	AllowXContainer = true;
}

RenderPath::~RenderPath() {
	delete particle_vb;
	delete pipeline_fx;

	delete shadow_renderer;

	delete light_cam;
}

void RenderPath::pick_shadow_source() {

}

void RenderPath::draw_world(vulkan::CommandBuffer *cb, int light_index) {

	GeoPush gp;
	gp.eye_pos = cam->pos;

	for (auto *t: world.terrains) {
		gp.model = matrix::ID;
		gp.emission = Black;
		gp.xxx[0] = 0.0f;
		cb->push_constant(0, sizeof(gp), &gp);
		cb->bind_descriptor_set_dynamic(0, t->dset, {light_index});
		cb->draw(t->vertex_buffer);
	}

	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;
		gp.model = mtr(m->pos, m->ang);
		gp.emission = s.material->emission;
		gp.xxx[0] = 0.2f;
		cb->push_constant(0, sizeof(gp), &gp);

		cb->bind_descriptor_set_dynamic(0, s.dset, {light_index});
		cb->draw(m->mesh[0]->sub[0].vertex_buffer);
	}

}

void RenderPath::render_fx(vulkan::CommandBuffer *cb, Renderer *r) {
	cb->set_pipeline(pipeline_fx);
	cb->set_viewport(r->area());

	cb->push_constant(80, sizeof(color), &world.fog._color);
	float density0 = 0;
	cb->push_constant(96, 4, &density0);

	/*
	for (auto *g: world.particle_manager->groups) {
		g->dset->set({}, {deferred_reenderer->gbuf_ren->depth_buffer, g->texture});
		cb->bind_descriptor_set(0, g->dset);

		for (auto *p: g->particles) {
			matrix m = cam->m_all * matrix::translation(p->pos) * matrix::rotation_q(cam->ang) * matrix::scale(p->radius, p->radius, 1);
			cb->push_constant(0, sizeof(m), &m);
			cb->push_constant(64, sizeof(color), &p->col);
			if (world.fog.enabled) {
				float dist = (cam->pos - p->pos).length(); //(cam->m_view * p->pos).z;
				float fog_density = 1-exp(-dist / world.fog.distance);
				cb->push_constant(96, 4, &fog_density);
			}
			cb->draw(particle_vb);
		}
	}*/

}

void RenderPath::prepare_all(Renderer *r, Camera *c) {

	c->set_view((float)r->width / (float)r->height);

	UBOMatrices u;
	u.proj = c->m_projection;
	u.view = c->m_view;

	UBOFog f;
	f.col = world.fog._color;
	f.distance = world.fog.distance;
	world.ubo_fog->update(&f);

	for (auto *t: world.terrains) {
		u.model = matrix::ID;
		t->ubo->update(&u);
		//t->dset->set({t->ubo, world.ubo_light, world.ubo_fog}, {t->material->textures[0], tex_white, tex_black, shadow_renderer->depth_buffer});

		t->draw(); // rebuild stuff...
	}
	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;

		u.model = mtr(m->pos, m->ang);
		s.ubo->update(&u);
	}

	gui::update();
}


void RenderPath::render_into_shadow(ShadowMapRenderer *r) {
	r->start_frame();
	auto *cb = r->cb;

	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
	cb->set_pipeline(r->pipeline);
	cb->set_viewport(r->area());

	draw_world(cb, 0);
	cb->end_render_pass();

	r->end_frame();
}

