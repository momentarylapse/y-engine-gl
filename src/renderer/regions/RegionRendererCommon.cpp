/*
 * RegionRendererCommon.cpp
 *
 *  Created on: 06 Nov 2023
 *      Author: michi
 */

#include "RegionRendererCommon.h"

RegionRendererCommon::RegionRendererCommon(Renderer *parent) : Renderer("region", parent) {
}

void RegionRendererCommon::prepare(const RenderParams& params) {
	for (int i=0; i<children.num; i++)
		regions[i].renderer = children[i];

	for (auto r: sorted_regions) {
		if (r->renderer) {
			auto sub_params = params;
			sub_params.desired_aspect_ratio *= r->dest.width() / r->dest.height();
			r->renderer->prepare(sub_params);
		}
	}
}

Renderer* RegionRendererCommon::add_region(const rect &dest, int z) {
	regions.add({dest, z, nullptr});

	// resort
	sorted_regions.clear();
	for (auto &r: regions)
		sorted_regions.add(&r);
	for (int i=0; i<sorted_regions.num; i++)
		for (int k=i+1; k<sorted_regions.num; k++)
			if (sorted_regions[i]->z > sorted_regions[k]->z)
				sorted_regions.swap(i, k);

	return this;
}



