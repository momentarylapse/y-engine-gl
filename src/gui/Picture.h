/*
 * Picture.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#ifndef SRC_GUI_PICTURE_H_
#define SRC_GUI_PICTURE_H_

//#include "../lib/vulkan/vulkan.h"
#include "../lib/nix/nix.h"

class Picture {
public:
	Picture(const rect &r, float z, nix::Texture *tex, nix::Shader *shader);
	Picture(const rect &r, float z, nix::Texture *tex);
	virtual ~Picture();

	void __init__(const rect &r, float z, nix::Texture *tex);
	virtual void __delete__();

	rect dest;
	rect source;
	color col;
	float z;

	float bg_blur;

	nix::Texture *texture;
	/*vulkan::Texture *texture;
	vulkan::UniformBuffer *ubo;
	vulkan::DescriptorSet *dset;
	vulkan::Shader *user_shader;
	vulkan::Pipeline *user_pipeline;*/

	//virtual void rebuild();

	/*static vulkan::Shader *shader;
	static vulkan::Pipeline *pipeline;
	static vulkan::VertexBuffer *vertex_buffer;
	static vulkan::RenderPass *render_pass;*/
};

namespace gui {
	//void init(vulkan::RenderPass *rp);
	void init(nix::Shader *s);
	void reset();
	//void render(vulkan::CommandBuffer *cb, const rect &viewport);
	void update();
	void add(Picture *p);

	extern Array<Picture*> pictures;
	extern nix::Shader *shader;
	extern nix::VertexBuffer *vertex_buffer;
}

#endif /* SRC_GUI_PICTURE_H_ */
