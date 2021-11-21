/*
 * base.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "base.h"
#include "../graphics-impl.h"
#include "../lib/image/image.h"
#include "../lib/file/msg.h"
#include "../Config.h"

Texture *tex_white = nullptr;
Texture *tex_black = nullptr;


#ifdef USING_VULKAN

vulkan::Instance *instance = nullptr;

void api_init(GLFWwindow* window) {
	instance = vulkan::init(window, {"glfw", "validation", "api=1.2"});

	tex_white = new Texture();
	tex_black = new Texture();

	Image im;
	im.create(16, 16, White);
	tex_white->override(im);
	im.create(16, 16, Black);
	tex_black->override(im);
}

void api_end() {
	delete instance;
}

void break_point() {}

#endif

#ifdef USING_OPENGL


namespace nix {
	extern bool allow_separate_vertex_arrays;
	int total_mem();
	int available_mem();
}


void api_init(GLFWwindow* window) {
	nix::allow_separate_vertex_arrays = true;
	nix::init();

	if (nix::total_mem() > 0) {
		msg_write(format("VRAM: %d mb  of  %d mb available", nix::available_mem() / 1024, nix::total_mem() / 1024));
	}

	tex_white = new nix::Texture(16, 16, "rgba:i8");
	tex_black = new nix::Texture(16, 16, "rgba:i8");
	Image im;
	im.create(16, 16, White);
	tex_white->override(im);
	im.create(16, 16, Black);
	tex_black->override(im);
}

void api_end() {
}

void break_point() {
	if (config.debug) {
		nix::flush();
	}
}

#endif

