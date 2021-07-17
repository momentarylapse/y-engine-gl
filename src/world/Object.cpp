/*----------------------------------------------------------------------------*\
| Object                                                                       |
| -> physical entities of a model in the game                                  |
| -> manages physics on its own                                                |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.10.26 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "Object.h"
#include "Material.h"
#include "World.h"
#include "../y/EngineData.h"
#include "../lib/file/file.h"




// neutral object (for terrains,...)
Object::Object() {
	material.add(new Material);
	script_data.name = "-terrain-";
	visible = false;
	prop.radius = 30000000;
	_matrix = matrix::ID;
}


void Object::make_visible(bool _visible_) {
	if (_visible_ == visible)
		return;
	if (_visible_)
		world.register_model(this);
	else
		world.unregister_model(this);
	visible = _visible_;
}
