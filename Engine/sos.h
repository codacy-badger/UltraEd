#ifndef _SOS_H_
#define _SOS_H_

#include <nusys.h>
#include "upng.h"

#define SCREEN_WD 320
#define SCREEN_HT 240

struct transform {
  Mtx projection;
  Mtx translation;
  Mtx scale;
  Mtx rotation;
};

struct sos_model {
  struct mesh *mesh;
  unsigned short *texture;
  double rotationAngle;
  int visible;
  struct vector3 *position;
  struct vector3 *rotationAxis;
  struct vector3 *scale;
  struct transform transform;
};

struct vector3 {
  double x, y, z;
};

struct mesh {
  int vertex_count;
  Vtx *vertices;
};

struct sos_model *load_sos_model_with_texture(void *data_start, void *data_end,
	void *texture_start, void *texture_end,
	double positionX, double positionY, double positionZ,
	double rotX, double rotY, double rotZ, double angle,
	double scaleX, double scaleY, double scaleZ);

#endif
