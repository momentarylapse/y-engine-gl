/*
 * WorldRendererVulkanForward.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRendererVulkan.h"
#ifdef USING_VULKAN

class Camera;
class PerformanceMonitor;
struct UBO;

class WorldRendererVulkanForward : public WorldRendererVulkan {
public:

	WorldRendererVulkanForward(Renderer *parent);

	void prepare() override;
	void draw() override;

	void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void draw_world(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, Array<RenderDataVK> &rda_tr, Array<RenderDataVK> &rda_ob);
	void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale);
};

#endif