/*
 * GeometryRendererVulkan.h
 *
 *  Created on: Dec 16, 2022
 *      Author: michi
 */

#pragma once

#include "GeometryRenderer.h"
#ifdef USING_VULKAN
#include "../../../lib/math/mat4.h"

class Camera;
class PerformanceMonitor;
class Material;

enum class RenderPathType;
enum class ShaderVariant;

struct UBO {
	// matrix
	mat4 m,v,p;
	// material
	color albedo, emission;
	float roughness, metal;
	int dummy[2];
	int num_lights;
	int shadow_index;
	int dummy2[2];
};

struct RenderDataVK {
	UniformBuffer* ubo;
	DescriptorSet* dset;
};

struct UBOFx {
	mat4 m,v,p;
};

struct RenderDataFxVK {
	UniformBuffer *ubo;
	DescriptorSet *dset;
	VertexBuffer *vb;
};

struct RenderViewDataVK {
	UniformBuffer *ubo_light = nullptr;
	Array<RenderDataVK> rda_tr;
	Array<RenderDataVK> rda_ob;
	Array<RenderDataVK> rda_ob_trans;
	Array<RenderDataVK> rda_ob_multi;
	Array<RenderDataVK> rda_user;
	Array<RenderDataVK> rda_sky;
	Array<RenderDataFxVK> rda_fx;
};

class GeometryRendererVulkan : public GeometryRenderer {
public:
	GeometryRendererVulkan(RenderPathType type, Renderer *parent);

	void prepare() override;
	void draw() override {}

	GraphicsPipeline *pipeline_fx = nullptr;
	RenderViewDataVK rvd_def;


	GraphicsPipeline *get_pipeline(Shader *s, RenderPass *rp, Material *m, PrimitiveTopology top, VertexBuffer *vb);
	void set_material(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, RenderPathType type, ShaderVariant v, PrimitiveTopology top, VertexBuffer *vb);
	void set_material_x(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, GraphicsPipeline *p);
	void set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex);

	void draw_particles(CommandBuffer *cb, RenderPass *rp, Camera *cam, RenderViewDataVK &rvd);
	void draw_skyboxes(CommandBuffer *cb, RenderPass *rp, Camera *cam, float aspect, RenderViewDataVK &rvd);
	void draw_terrains(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd);
	void draw_objects_opaque(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd);
	void draw_objects_transparent(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd);
	void draw_objects_instanced(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd);
	void draw_user_meshes(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool transparent, RenderViewDataVK &rvd);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam, RenderViewDataVK &rvd);
};

#endif