/*
 * Particle.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "Particle.h"
#include "../graphics-impl.h"

Shader *shader_fx;

Particle::Particle(const vec3 &p, const color& _col, float r, float ttl) {
	pos = p;
	vel = vec3::ZERO;
	col = _col;
	radius = r;
	time_to_live = ttl;
	suicidal = ttl > 0;
	enabled = true;
}


LegacyParticle::LegacyParticle() : BaseClass(Type::LEGACY_PARTICLE) {
	pos = vec3::ZERO;
	vel = vec3::ZERO;
	col = White;
	radius = 1;
	time_to_live = -1;
	source = rect::ID;
	enabled = true;
}

LegacyParticle::~LegacyParticle() = default;

void LegacyParticle::__init__() {
	new(this) LegacyParticle();
}

void LegacyParticle::__delete__() {
	this->LegacyParticle::~LegacyParticle();
}

