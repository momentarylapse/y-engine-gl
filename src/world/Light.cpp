/*
 * Light.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "Light.h"
#include "Camera.h"
#include "../y/Entity.h"
#include "../lib/file/msg.h"

const kaba::Class *Light::_class = nullptr;

Light::Light(const color &c, float r, float t) {
	component_type = _class;
	light.pos = vector(0,0,0);
	light.dir = vector::EZ;
	light.col = c;
	light.radius = r;
	light.theta = t;
	light.harshness = 0.8f;
	if (light.radius >= 0)
		light.harshness = 1;
	enabled = true;
	allow_shadow = false;
	user_shadow_control = false;
	user_shadow_theta = -1;
	shadow_dist_min = -1;
	shadow_dist_max = -1;
	type = LightType::DIRECTIONAL;
	if (light.radius > 0) {
		if (light.theta > 0)
			type = LightType::CONE;
		else
			type = LightType::POINT;
	}
}

void Light::on_init() {
	auto o = get_owner<Entity>();
	light.pos = o->pos;
	light.dir = o->ang * vector::EZ;
}

/*void Light::__init_parallel__(const quaternion &ang, const color &c) {
	new(this) Light(v_0, ang, c, -1, -1);
}
void Light::__init_spherical__(const vector &p, const color &c, float r) {
	new(this) Light(p, quaternion::ID, c, r, -1);
}
void Light::__init_cone__(const vector &p, const quaternion &ang, const color &c, float r, float t) {
	new(this) Light(p, ang, c, r, t);
}*/


void Light::set_direction(const vector &dir) {
	get_owner<Entity>()->ang = quaternion::rotation(dir.dir2ang());
}

void Light::update(Camera *cam, float shadow_box_size, bool using_view_space) {
	auto o = get_owner<Entity>();
	if (using_view_space) {
		//light.pos = cam->m_view * o->pos;
		light.pos = cam->view_matrix() * o->pos;
		light.dir = cam->get_owner<Entity>()->ang.bar() * o->ang * vector::EZ;
	} else {
		light.pos = o->pos;
		light.dir = o->ang * vector::EZ;
	}

	if (allow_shadow) {
		if (type == LightType::DIRECTIONAL) {
			//msg_write(format("shadow dir: %s  %s", light.pos.str(), light.dir.str()));
			vector center = cam->get_owner<Entity>()->pos + cam->get_owner<Entity>()->ang*vector::EZ * (shadow_box_size / 3.0f);
			float grid = shadow_box_size / 16;
			center.x -= fmod(center.x, grid) - grid/2;
			center.y -= fmod(center.y, grid) - grid/2;
			center.z -= fmod(center.z, grid) - grid/2;
			//center = vector(0,200,0);
			auto t = matrix::translation(- center);
			//auto r = matrix::rotation({pi/2,0,0}).transpose();
			//o->ang = quaternion(pi/2, {1,0,0});
			auto r = matrix::rotation(o->ang).transpose();
			float f = 1 / shadow_box_size;
			auto s = matrix::scale(f, f, f);
			// map onto [-1,1]x[-1,1]x[0,1]
			shadow_projection = matrix::translation(vector(0,0,0.5f)) * matrix::scale(1,1,0.5f) * matrix::translation(vector(0,0,-0.5f)) * s * r * t;
			//msg_write(shadow_projection.str());
		} else {
			auto t = matrix::translation(- o->pos);
			auto ang = cam->get_owner<Entity>()->ang;
			if (type == LightType::CONE or user_shadow_control)
				ang = o->ang;
			auto r = matrix::rotation(ang).transpose();
			float theta = 1.35f;
			if (type == LightType::CONE)
				theta = light.theta;
			if (user_shadow_theta > 0)
				theta = user_shadow_theta;
			float dist_min = (shadow_dist_min > 0) ? shadow_dist_min : light.radius * 0.01f;
			float dist_max = (shadow_dist_max > 0) ? shadow_dist_max : light.radius;
			auto p = matrix::perspective(2 * theta, 1.0f, dist_min, dist_max, false);
			shadow_projection = p * r * t;
		}
		if (using_view_space)
			light.proj = shadow_projection * cam->view_matrix().inverse();
		else
			light.proj = shadow_projection;
	}
}
