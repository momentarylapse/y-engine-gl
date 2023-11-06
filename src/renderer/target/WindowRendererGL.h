/*
 * WindowRendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "TargetRenderer.h"
#ifdef USING_OPENGL

struct GLFWwindow;


class WindowRendererGL : public TargetRenderer {
public:
	WindowRendererGL(GLFWwindow* win);


	bool start_frame() override;
	void end_frame() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	RenderParams create_params(float aspect_ratio);

	GLFWwindow* window;

	DepthBuffer* _depth_buffer;
	FrameBuffer* _frame_buffer;
};

#endif
