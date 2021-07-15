/*
 * Entity3D.h
 *
 *  Created on: Jul 15, 2021
 *      Author: michi
 */

#pragma once

#include "../y/Entity.h"
#include "../lib/math/vector.h"
#include "../lib/math/quaternion.h"

class matrix;


class Entity3D : public Entity {
public:
	Entity3D(Type type);

	vector pos;
	quaternion ang;
	matrix get_matrix() const;
};

