/*
 * Controller.h
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#ifndef SRC_PLUGINS_CONTROLLER_H_
#define SRC_PLUGINS_CONTROLLER_H_

#include "../y/Entity.h"

class Controller : public Entity {
public:
	Controller();
	~Controller() override {};

	void __init__();
	void __delete__() override;

	virtual void _cdecl on_input() {}
	virtual void _cdecl on_key_down(int k) {}
	virtual void _cdecl on_key_up(int k) {}
	virtual void _cdecl on_key(int k) {}
	virtual void _cdecl on_left_button_down() {}
	virtual void _cdecl on_left_button_up() {}
	virtual void _cdecl on_middle_button_down() {}
	virtual void _cdecl on_middle_button_up() {}
	virtual void _cdecl on_right_button_down() {}
	virtual void _cdecl on_right_button_up() {}
	virtual void _cdecl on_iterate_pre(float dt) {}
	virtual void _cdecl on_draw_pre() {}

	virtual void _cdecl on_render_inject() {}
	virtual void _cdecl on_render_inject2() {}
};

#endif /* SRC_PLUGINS_CONTROLLER_H_ */
