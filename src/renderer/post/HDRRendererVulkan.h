/*
 * HDRRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */


#pragma once

#include "PostProcessor.h"
#ifdef USING_VULKAN

class HDRRendererVulkan : public PostProcessorStage {
public:
	HDRRendererVulkan(Renderer *parent);
	virtual ~HDRRendererVulkan();

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);

	struct RenderIntoData {
		RenderIntoData() {}
		RenderIntoData(Renderer *r);
		void render_into(Renderer *r, const RenderParams& params);

		shared<FrameBuffer> fb_main;
		DepthBuffer *_depth_buffer = nullptr;

		//shared<RenderPass> render_pass;
		RenderPass *_render_pass = nullptr;
	} into;


	struct RenderOutData {
		RenderOutData(){}
		RenderOutData(Shader *s, Renderer *r, const Array<Texture*> &tex);
		void render_out(CommandBuffer *cb, const Array<float> &data, const RenderParams& params);
		shared<Shader> shader_out;
		GraphicsPipeline* pipeline_out;
		DescriptorSet *dset_out;
		VertexBuffer *vb_2d;
	} out;


	FrameBuffer *fb_main;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;

	FrameBuffer *frame_buffer() const override { return into.fb_main.get(); };
	DepthBuffer *depth_buffer() const override { return into._depth_buffer; };

	shared<Shader> shader_blur;
	shared<Shader> shader_out;

	owned<VertexBuffer> vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
