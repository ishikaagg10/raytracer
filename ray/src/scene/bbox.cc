#include "bbox.h"
#include "ray.h"

BoundingBox::BoundingBox() : bEmpty(true) {}

BoundingBox::BoundingBox(glm::dvec3 bMin, glm::dvec3 bMax)
    : bEmpty(false), dirty(true), bmin(bMin), bmax(bMax) {}

bool BoundingBox::intersects(const BoundingBox &target) const {
  return ((target.getMin()[0] - RAY_EPSILON <= bmax[0]) &&
          (target.getMax()[0] + RAY_EPSILON >= bmin[0]) &&
          (target.getMin()[1] - RAY_EPSILON <= bmax[1]) &&
          (target.getMax()[1] + RAY_EPSILON >= bmin[1]) &&
          (target.getMin()[2] - RAY_EPSILON <= bmax[2]) &&
          (target.getMax()[2] + RAY_EPSILON >= bmin[2]));
}

bool BoundingBox::intersects(const glm::dvec3 &point) const {
  return ((point[0] + RAY_EPSILON >= bmin[0]) &&
          (point[1] + RAY_EPSILON >= bmin[1]) &&
          (point[2] + RAY_EPSILON >= bmin[2]) &&
          (point[0] - RAY_EPSILON <= bmax[0]) &&
          (point[1] - RAY_EPSILON <= bmax[1]) &&
          (point[2] - RAY_EPSILON <= bmax[2]));
}

bool BoundingBox::intersect(const ray &r, double &tMin, double &tMax) const {
    if (bEmpty) return false;

    glm::dvec3 R0 = r.getPosition();
    glm::dvec3 Rd = r.getDirection();

    double t0 = 0.0;     
    double t1 = 1.0e308;

    for (int i = 0; i < 3; ++i) {
        double invD = 1.0 / Rd[i];
        double tNear = (bmin[i] - R0[i]) * invD;
        double tFar = (bmax[i] - R0[i]) * invD;

        if (std::isnan(tNear) || std::isnan(tFar)) {
             if (R0[i] < bmin[i] || R0[i] > bmax[i]) return false;
             continue;
        }

        if (tNear > tFar) std::swap(tNear, tFar);

        if (tNear > t0) t0 = tNear;
        if (tFar < t1)  t1 = tFar;

        if (t0 > t1) return false;
    }

    tMin = t0;
    tMax = t1;
    return true;
}

double BoundingBox::area() {
  if (bEmpty)
    return 0.0;
  else if (dirty) {
    bArea = 2.0 * ((bmax[0] - bmin[0]) * (bmax[1] - bmin[1]) +
                   (bmax[1] - bmin[1]) * (bmax[2] - bmin[2]) +
                   (bmax[2] - bmin[2]) * (bmax[0] - bmin[0]));
    dirty = false;
  }
  return bArea;
}

double BoundingBox::volume() {
  if (bEmpty)
    return 0.0;
  else if (dirty) {
    bVolume = ((bmax[0] - bmin[0]) * (bmax[1] - bmin[1]) * (bmax[2] - bmin[2]));
    dirty = false;
  }
  return bVolume;
}

void BoundingBox::merge(const BoundingBox &bBox) {
  if (bBox.bEmpty)
    return;
  for (int axis = 0; axis < 3; axis++) {
    if (bEmpty || bBox.bmin[axis] < bmin[axis])
      bmin[axis] = bBox.bmin[axis];
    if (bEmpty || bBox.bmax[axis] > bmax[axis])
      bmax[axis] = bBox.bmax[axis];
  }
  dirty = true;
  bEmpty = false;
}
