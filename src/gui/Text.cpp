/*
 * Text.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Text.h"
#include "Font.h"
#include "../lib/math/vector.h"
#include "../lib/image/image.h"
#include "../lib/nix/nix.h"

#include "../y/EngineData.h"


namespace gui {


Text::Text(const string &t, float h, const vec2 &p) : Picture(rect(p.x,p.x,p.y,p.y), nullptr) {//rect::ID
	type = Type::TEXT;
	//margin = rect(x, h/6, y, h/10);
	font = Font::_default;
	font_size = h;
	if (t != ":::fake:::")
		set_text(t);
}

Text::~Text() {
}

void Text::__init2__(const string &t, float h) {
	new(this) Text(t, h, vec2::ZERO);//h/6, h/6);
}

void Text::__init4__(const string &t, float h, const vec2 &p) {
	new(this) Text(t, h, p);
}

void Text::__delete__() {
	this->Text::~Text();
}

void Text::rebuild() {
	if (!font)
		return;
	Image im;
	font->render_text(text, align, im);
	if (im.width == 0 or im.height == 0)
		im.create(1,1, color(0,0,0,0));

	if (texture == nullptr)
		texture = new nix::Texture();

	texture->overwrite(im);
	//texture->set_options("magfilter=nearest,wrap=clamp");
	texture->set_options("magfilter=linear,wrap=clamp");

	height = font_size * font->get_height_rel(text);
	width = height * (float)im.width / (float)im.height;
	if (align & Align::NONSQUARE)
		 width /= engine.physical_aspect_ratio;
}

void Text::set_text(const string &t) {
	text = t;
	rebuild();
}

}
