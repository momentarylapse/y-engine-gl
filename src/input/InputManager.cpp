/*
 * InputManager.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "InputManager.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Gamepad.h"

#include <GLFW/glfw3.h>


namespace input {



void init(GLFWwindow *window) {
	init_mouse(window);
	init_keyboard(window);
}

void iterate() {
	glfwPollEvents();

	iterate_mouse();
	iterate_keyboard();
	iterate_pads();
}


}
