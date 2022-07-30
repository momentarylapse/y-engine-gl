/*
 * Beam.h
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#pragma once

#include "Particle.h"

class Beam : public Particle{
public:
	Beam(const vec3 &pos, const vec3 &length, float r, Texture *tex, float ttl);

	void __init_beam__(const vec3 &pos, const vec3 &length, float r, Texture *tex, float ttl);

	vec3 length;
};
