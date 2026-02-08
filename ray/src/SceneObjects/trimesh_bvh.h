#ifndef TRIMESH_BVH_H__
#define TRIMESH_BVH_H__

#include <vector>
#include "../scene/bbox.h"
#include "../scene/ray.h"

class TrimeshFace;

// Simple BVH node for Trimesh acceleration
class TrimeshBVHNode {
public:
  BoundingBox bounds;
  TrimeshBVHNode* left;
  TrimeshBVHNode* right;
  std::vector<TrimeshFace*> faces;

  TrimeshBVHNode();
  ~TrimeshBVHNode();

  bool isLeaf() const { return left == nullptr && right == nullptr; }
};

class TrimeshBVH {
public:
  TrimeshBVH();
  ~TrimeshBVH();

  void build(const std::vector<TrimeshFace*>& faces);
  bool intersect(ray& r, isect& i) const;

private:
  TrimeshBVHNode* root;

  TrimeshBVHNode* buildRecursive(std::vector<TrimeshFace*>& faces, int depth);
  bool intersectNode(TrimeshBVHNode* node, ray& r, isect& i) const;
};

#endif // TRIMESH_BVH_H__