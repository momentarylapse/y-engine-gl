/*
 * PluginManager.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "PluginManager.h"
#include "Controller.h"
#include "../lib/kaba/kaba.h"
#include "../world/world.h"
#include "../world/model.h"
#include "../world/terrain.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../helper/InputManager.h"


PluginManager plugin_manager;


void PluginManager::link_kaba() {

	Camera _cam(v_0, quaternion::ID, rect::ID);
	Kaba::declare_class_size("Camera", sizeof(Camera));
	Kaba::declare_class_element("Camera.pos", &Camera::pos);
	Kaba::declare_class_element("Camera.ang", &Camera::ang);
	Kaba::declare_class_element("Camera.dest", &Camera::dest);
	Kaba::declare_class_element("Camera.zoom", &Camera::zoom);
	Kaba::declare_class_element("Camera.enabled", &Camera::enabled);
	Kaba::declare_class_element("Camera.show", &Camera::show);
	Kaba::declare_class_element("Camera.min_depth", &Camera::min_depth);
	Kaba::declare_class_element("Camera.max_depth", &Camera::max_depth);
	Kaba::declare_class_element("Camera.m_view", &Camera::m_view);
	Kaba::link_external_virtual("Camera.__delete__", &Camera::__delete__, &_cam);
	Kaba::link_external_virtual("Camera.on_init", &Camera::on_init, &_cam);
	Kaba::link_external_virtual("Camera.on_delete", &Camera::on_delete, &_cam);
	Kaba::link_external_virtual("Camera.on_iterate", &Camera::on_iterate, &_cam);


	Model model;
	Kaba::declare_class_size("Model", sizeof(Model));
	Kaba::declare_class_element("Model.pos", &Model::pos);
	Kaba::declare_class_element("Model.vel", &Model::vel);
	Kaba::declare_class_element("Model.ang", &Model::ang);
	Kaba::declare_class_element("Model.rot", &Model::rot);
	Kaba::declare_class_element("Model.mesh", &Model::mesh);
	Kaba::declare_class_element("Model.materials", &Model::material);
	Kaba::declare_class_element("Model.bones", &Model::bone);
	//Kaba::declare_class_element("Model.mass", &Model::physics_data.mass);
	Kaba::link_external_virtual("Model.__delete__", &Model::__delete__, &model);
	Kaba::link_external_virtual("Model.on_init", &Model::on_init, &model);
	Kaba::link_external_virtual("Model.on_delete", &Model::on_delete, &model);
	Kaba::link_external_virtual("Model.on_collide_m", &Model::on_collide_m, &model);
	Kaba::link_external_virtual("Model.on_collide_t", &Model::on_collide_t, &model);
	Kaba::link_external_virtual("Model.on_iterate", &Model::on_iterate, &model);


	Kaba::declare_class_element("Material.textures", &Material::textures);
	Kaba::declare_class_element("Material.shader", &Material::shader);
	Kaba::declare_class_element("Material.ambient", &Material::ambient);
	Kaba::declare_class_element("Material.diffuse", &Material::diffuse);
	Kaba::declare_class_element("Material.specular", &Material::specular);
	Kaba::declare_class_element("Material.shininess", &Material::shininess);
	Kaba::declare_class_element("Material.emission", &Material::emission);


	Kaba::declare_class_element("World.objects", &World::objects);
	Kaba::declare_class_element("World.terrains", &World::terrains);
	Kaba::declare_class_element("World.background", &World::background);
	Kaba::declare_class_element("World.skyboxes", &World::skybox);
	Kaba::declare_class_element("World.sun", &World::sun);
	Kaba::declare_class_element("World.lights", &World::lights);
	Kaba::declare_class_element("World.ego", &World::ego);
	Kaba::declare_class_element("World.fog", &World::fog);
	Kaba::link_external_class_func("World.create_object", &World::create_object);
	Kaba::link_external_class_func("World.create_terrain", &World::create_terrain);
	Kaba::link_external_class_func("World.add_light", &World::add_light);
	Kaba::link_external_class_func("World.add_particle", &World::add_particle);

	Kaba::declare_class_element("Fog.color", &Fog::_color);
	Kaba::declare_class_element("Fog.enabled", &Fog::enabled);
	Kaba::declare_class_element("Fog.distance", &Fog::distance);


	Controller con;
	Kaba::declare_class_size("Controller", sizeof(Controller));
	Kaba::link_external_class_func("Controller.__init__", &Controller::__init__);
	Kaba::link_external_virtual("Controller.__delete__", &Controller::__delete__, &con);
	Kaba::link_external_virtual("Controller.on_init", &Controller::on_init, &con);
	Kaba::link_external_virtual("Controller.on_delete", &Controller::on_delete, &con);
	Kaba::link_external_virtual("Controller.on_iterate", &Controller::on_iterate, &con);
	Kaba::link_external_virtual("Controller.on_input", &Controller::on_input, &con);
	Kaba::link_external_virtual("Controller.on_key", &Controller::on_key, &con);
	Kaba::link_external_virtual("Controller.on_key_down", &Controller::on_key_down, &con);
	Kaba::link_external_virtual("Controller.on_key_up", &Controller::on_key_up, &con);
	Kaba::link_external_virtual("Controller.on_left_button_down", &Controller::on_left_button_down, &con);
	Kaba::link_external_virtual("Controller.on_left_button_up", &Controller::on_left_button_up, &con);
	Kaba::link_external_virtual("Controller.on_middle_button_down", &Controller::on_middle_button_down, &con);
	Kaba::link_external_virtual("Controller.on_middle_button_up", &Controller::on_middle_button_up, &con);
	Kaba::link_external_virtual("Controller.on_right_button_down", &Controller::on_right_button_down, &con);
	Kaba::link_external_virtual("Controller.on_right_button_up", &Controller::on_right_button_up, &con);
	Kaba::link_external_virtual("Controller.before_draw", &Controller::before_draw, &con);


	Kaba::declare_class_size("Light", sizeof(Light));
	Kaba::declare_class_element("Light.pos", &Light::pos);
	Kaba::declare_class_element("Light.dir", &Light::dir);
	Kaba::declare_class_element("Light.color", &Light::col);
	Kaba::declare_class_element("Light.radius", &Light::radius);
	Kaba::declare_class_element("Light.theta", &Light::theta);
	Kaba::declare_class_element("Light.harshness", &Light::harshness);
	Kaba::declare_class_element("Light.enabled", &Light::enabled);

	Kaba::link_external_class_func("LightParallel.__init__", &Light::__init_parallel__);
	Kaba::link_external_class_func("LightSpherical.__init__", &Light::__init_spherical__);
	Kaba::link_external_class_func("LightCone.__init__", &Light::__init_cone__);


	Kaba::declare_class_size("Particle", sizeof(Particle));
	Kaba::declare_class_element("Particle.pos", &Particle::pos);
	Kaba::declare_class_element("Particle.radius", &Particle::radius);
	Kaba::declare_class_element("Particle.texture", &Particle::texture);
	Kaba::declare_class_element("Particle.color", &Particle::col);
	Kaba::link_external_class_func("Particle.__init__", &Particle::__init__);

	Kaba::link_external("get_key", (void*)&InputManager::get_key);
	Kaba::link_external("get_key_down", (void*)&InputManager::get_key_down);
	Kaba::link_external("get_key_up", (void*)&InputManager::get_key_up);


	Kaba::link_external("world", &world);
	Kaba::link_external("cam", &cam);
	Kaba::link_external("mouse", &InputManager::mouse);
	Kaba::link_external("dmouse", &InputManager::dmouse);
	Kaba::link_external("scroll", &InputManager::scroll);
}

void PluginManager::reset() {
	controllers.clear();
}

void *PluginManager::create_instance(const string &filename, const string &base_class) {

	try{
		auto *s = Kaba::Load(filename);
		for (auto *c: s->classes()) {
			if (c->is_derived_from_s(base_class)) {
				return c->create_instance();
			}
		}
	}catch(Kaba::Exception &e) {
		msg_error(e.message());
		throw std::runtime_error(e.message().c_str());
	}
	throw std::runtime_error(("script does not contain a class derived from " + base_class).c_str());
	return nullptr;
}

void PluginManager::add_controller(const string &name) {
	auto *c = (Controller*)create_instance(name, "Controller");
	controllers.add(c);
	c->on_init();
}

