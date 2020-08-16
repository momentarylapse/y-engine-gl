/*
 * EngineData.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */


#include "EngineData.h"
#include "../lib/nix/nix.h"
//#include "../lib/vulkan/vulkan.h"
#include "../lib/kaba/kaba.h"
#ifdef _X_ALLOW_X_
#include "../world/model.h"
#include "../world/material.h"
#endif

EngineData engine;

EngineData::EngineData() {
	default_font = NULL;

	ZBufferEnabled = true;
	CullingEnabled = false;
	SortingEnabled = false;
	console_enabled = false;
	show_timings = false;
	debug = false;

	shadow_lower_detail = false;
	shadow_level = 0;

	fps_max = 60;
	fps_min = 15;

	detail_level = 100;
	detail_factor_inv = 1.0f;
	mirror_level_max = 1;
	multisampling = 1;

	wire_mode = false;
	physics_enabled = false;
	collisions_enabled = false;
	elapsed = elapsed_rt = 0;

	time_scale = 1.0f;
	file_errors_are_critical = false;
	physical_aspect_ratio = 1.333f;

	num_real_col_tests = 0;
	resetting_game = false;
	first_frame = false;
	game_running = false;
}



void EngineData::set_dirs(const Path &texture_dir, const Path &_map_dir, const Path &_object_dir, const Path &_sound_dir, const Path &_script_dir, const Path &material_dir, const Path &font_dir) {
#if LIB_HAS_VULKAN
	vulkan::Texture::directory = texture_dir;
	vulkan::Shader::directory = material_dir;
#endif
	nix::texture_dir = texture_dir;
	nix::shader_dir = material_dir;

	map_dir = _map_dir;
	object_dir = _object_dir;
	sound_dir = _sound_dir;
	script_dir = _script_dir;
	MaterialDir = material_dir;
#if 0
	Gui::FontDir = font_dir;
#endif
	Kaba::config.directory = script_dir;
}

