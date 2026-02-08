#include "trimesh_bvh.h"
#include "trimesh.h"
#include <algorithm>

static const int MAX_FACES_PER_LEAF = 4;

TrimeshBVHNode::TrimeshBVHNode() : left(nullptr), right(nullptr) {}

TrimeshBVHNode::~TrimeshBVHNode() {
  delete left;
  delete right;
}

TrimeshBVH::TrimeshBVH() : root(nullptr) {}

TrimeshBVH::~TrimeshBVH() {
  delete root;
}

void TrimeshBVH::build(const std::vector<TrimeshFace*>& faces) {
  delete root;
  std::vector<TrimeshFace*> copy = faces;
  root = buildRecursive(copy, 0);
}

TrimeshBVHNode* TrimeshBVH::buildRecursive(std::vector<TrimeshFace*>& faces, int depth) {
  TrimeshBVHNode* node = new TrimeshBVHNode();

  // Compute bounds
  for (auto f : faces) {
    node->bounds.merge(f->getBoundingBox());
  }

  if (faces.size() <= MAX_FACES_PER_LEAF) {
    node->faces = faces;
    return node;
  }

  // Choose split axis by largest extent
  glm::dvec3 ext = node->bounds.getMax() - node->bounds.getMin();
  int axis = 0;
  if (ext.y > ext.x && ext.y > ext.z) axis = 1;
  else if (ext.z > ext.x) axis = 2;

  std::sort(faces.begin(), faces.end(), [axis](TrimeshFace* a, TrimeshFace* b) {
    return a->getBoundingBox().getMin()[axis] < b->getBoundingBox().getMin()[axis];
  });

  size_t mid = faces.size() / 2;
  std::vector<TrimeshFace*> leftFaces(faces.begin(), faces.begin() + mid);
  std::vector<TrimeshFace*> rightFaces(faces.begin() + mid, faces.end());

  node->left = buildRecursive(leftFaces, depth + 1);
  node->right = buildRecursive(rightFaces, depth + 1);

  return node;
}

bool TrimeshBVH::intersect(ray& r, isect& i) const {
  if (!root) return false;
  return intersectNode(root, r, i);
}

bool TrimeshBVH::intersectNode(TrimeshBVHNode* node, ray& r, isect& i) const {
  double tmin, tmax;
  if (!node->bounds.intersect(r, tmin, tmax)) return false;

  bool hit = false;

  if (node->isLeaf()) {
    for (auto f : node->faces) {
      isect cur;
      if (f->intersectLocal(r, cur)) {
        if (!hit || cur.getT() < i.getT()) {
          i = cur;
          hit = true;
        }
      }
    }
    return hit;
  }

  isect leftI, rightI;
  bool hitLeft = node->left && intersectNode(node->left, r, leftI);
  bool hitRight = node->right && intersectNode(node->right, r, rightI);

  if (hitLeft && hitRight) {
    i = (leftI.getT() < rightI.getT()) ? leftI : rightI;
    return true;
  } else if (hitLeft) {
    i = leftI;
    return true;
  } else if (hitRight) {
    i = rightI;
    return true;
  }

  return false;
}
