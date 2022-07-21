/*
 * Config.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "Config.h"
#include "lib/os/msg.h"

Config config;

Config::Config() {
}

void Config::load(const Array<string> &arg) {
	// fixed config
	Configuration::load("game.ini");

	// local config override
	Configuration local;
	local.load("game-local.ini");
	for (auto &k: local.keys())
		set_str(k, local.get_str(k, ""));

	// cli arguments override
	for (auto &a: arg.sub_ref(1)) {
		if (a.head(2).lower() == "-c") {
			auto xx = a.sub_ref(2).explode("=");
			set_str(xx[0].trim(), xx[1].trim());
		} else if (a == "--uncapped" or a == "-u") {
			set_bool("renderer.uncapped-framerate", true);
		} else if (a == "--debug" or a == "-D") {
			set_int("debug.level", 2);
		} else if (a.head(11) == "--game-dir=") {
			game_dir = a.sub_ref(11);
		} else if (a == "--fw") {
			set_str("renderer.path", "forward");
		} else if (a == "--def") {
			set_str("renderer.path", "deferred");
		} else if (a == "--direct") {
			set_str("renderer.path", "direct");
		} else if (a == "--msaa") {
			set_str("renderer.antialiasing", "MSAA");
		} else if (a.head(8) == "--scale=") {
			set_str("renderer.resolution-scale-min", a.sub_ref(7));
			set_str("renderer.resolution-scale-max", a.sub_ref(7));
		} else if (a.head(1) != "-") {
			set_str("default.world", a);
		}
	}

	// deprecated
	if (has("default-world") and !has("default.world"))
		set_str("default.world", get_str("default-world", ""));
	if (has("second-world") and !has("default.second-world"))
		set_str("default.second-world", get_str("second-world", ""));
	if (has("default-font") and !has("default.font"))
		set_str("default.font", get_str("default-font", ""));
	if (has("default-material") and !has("default.material"))
		set_str("default.material", get_str("default-material", ""));
	if (has("main-script") and !has("default.main-script"))
		set_str("default.main-script", get_str("main-script", ""));

	default_world = get_str("default.world", "");
	second_world = get_str("default.second-world", "");
	main_script = get_str("default.main-script", "");
	default_font = get_str("default.font", "");
	default_material = get_str("default.material", "");
	debug = get_int("debug.levelS", 1);

	string aa = get_str("renderer.antialiasing", "");
	if (aa == "") {
	} else if (aa == "MSAA") {
		antialiasing_method = AntialiasingMethod::MSAA;
	} else if (aa == "TAA") {
		antialiasing_method = AntialiasingMethod::TAA;
	} else {
		msg_error("unknown antialiasing method: " + aa);
	}

	resolution_scale_min = get_float("renderer.resolution-scale-min", 0.5f);
	resolution_scale_max = get_float("renderer.resolution-scale-max", 1.0f);
	target_framerate = get_float("renderer.target-framerate", 60.0f);

	ambient_occlusion_radius = get_float("renderer.ssao.radius", 10);
	if (!get_bool("renderer.ssao.enabled", true))
		ambient_occlusion_radius = -1;
}
