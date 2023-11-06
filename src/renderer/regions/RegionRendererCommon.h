/*
 * RegionRendererCommon.h
 *
 *  Created on: 06 Nov 2023
 *      Author: michi
 */

#ifndef SRC_RENDERER_REGIONS_REGIONRENDERERCOMMON_H_
#define SRC_RENDERER_REGIONS_REGIONRENDERERCOMMON_H_

#include "../Renderer.h"

#include <lib/math/rect.h>

class RegionRendererCommon : public Renderer {
public:
	RegionRendererCommon(Renderer *parent);

	void prepare(const RenderParams& params) override;

	Renderer *add_region(const rect &dest, int z);

	struct Region {
		rect dest;
		int z;
		Renderer *renderer;
	};

	Array<Region> regions;
	Array<Region*> sorted_regions;
};

#endif /* SRC_RENDERER_REGIONS_REGIONRENDERERCOMMON_H_ */
