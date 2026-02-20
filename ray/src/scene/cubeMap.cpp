#include "cubeMap.h"
#include "../scene/material.h"
#include "../ui/TraceUI.h"
#include "ray.h"
extern TraceUI *traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {
  glm::dvec3 dir = r.getDirection();
  
  double x = dir.x;
  double y = dir.y;
  double z = dir.z;

  double absX = std::abs(x);
  double absY = std::abs(y);
  double absZ = std::abs(z);

  int faceIndex = 0;
  double u = 0.0, v = 0.0;
  double maxAxis = 0.0;

  if (absX >= absY && absX >= absZ) {
      maxAxis = absX;
      if (x > 0.0) {
          faceIndex = 0; // +X (Right)
          u = -z;
          v = y;
      } else {
          faceIndex = 1; // -X (Left)
          u = z;
          v = y;
      }
  } else if (absY >= absX && absY >= absZ) {
      maxAxis = absY;
      if (y > 0.0) {
          faceIndex = 2; // +Y (Top)
          u = x;
          v = -z;
      } else {
          faceIndex = 3; // -Y (Bottom)
          u = x;
          v = z;
      }
  } else {
      maxAxis = absZ;
      if (z > 0.0) {
          faceIndex = 4; // +Z (Front)
          u = x;
          v = y;
      } else {
          faceIndex = 5; // -Z (Back)
          u = -x;
          v = y;
      }
  }

  u = 0.5 * (u / maxAxis + 1.0);
  v = 0.5 * (v / maxAxis + 1.0);

  if (tMap[faceIndex]) {
      return tMap[faceIndex]->getMappedValue(glm::dvec2(u, v));
  }

  return glm::dvec3(0.0, 0.0, 0.0);
}

CubeMap::CubeMap() {}

CubeMap::~CubeMap() {}

void CubeMap::setNthMap(int n, TextureMap *m) {
  if (m != tMap[n].get())
    tMap[n].reset(m);
}
