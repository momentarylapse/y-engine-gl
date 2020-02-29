/*----------------------------------------------------------------------------*\
| Camera                                                                       |
| -> representing the camera (view port)                                       |
| -> can be controlled by a camera script                                      |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.11.22 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(CAMERA_H__INCLUDED_)
#define CAMERA_H__INCLUDED_


#include "../lib/base/base.h"
#include "../lib/math/math.h"
#include "../meta.h"


class Camera : public XContainer {
public:
	Camera(const vector &pos, const quaternion &ang, const rect &dest);
	~Camera() override;
	void reset();
	
	rect dest;

	bool enabled;
	bool show;

	float min_depth, max_depth;
	
	vector pos;
	quaternion ang;
	float zoom;
	float exposure;
	float bloom_radius;
	float bloom_factor;

	void set_view(float aspect_ratio);

	matrix m_projection, m_view;
	matrix m_all, im_all;
	vector _cdecl project(const vector &v);
	vector _cdecl unproject(const vector &v);

	void _cdecl on_iterate(float dt) override;

	void _cdecl __init__(const vector &pos, const quaternion &ang, const rect &dest);
	void _cdecl __delete__() override;
};

void CameraInit();
void CameraReset();
void CameraCalcMove(float dt);
void CameraShiftAll(const vector &dpos);

extern Array<Camera*> cameras;
extern Camera *cam; // "camera"
extern Camera *cur_cam; // currently rendering

#endif

