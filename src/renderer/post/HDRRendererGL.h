/*
 * HDRRendererGL.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include "../Renderer.h"
#ifdef USING_OPENGL

class vec2;

class HDRRendererGL : public Renderer {
public:
	HDRRendererGL(Renderer *parent);
	virtual ~HDRRendererGL();

	void prepare() override;
	void draw() override;


	void process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const vec2 &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	void render_out(FrameBuffer *source, Texture *bloom);

	shared<FrameBuffer> fb_main;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;

	FrameBuffer *current_frame_buffer() const { return fb_main.get(); };
	DepthBuffer *current_depth_buffer() const { return depth_buffer; };

	DepthBuffer *depth_buffer = nullptr;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;

	VertexBuffer *vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
