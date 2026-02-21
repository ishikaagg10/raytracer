#include <cmath>

#include "../ui/TraceUI.h"
#include "kdTree.h"
#include "light.h"
#include "scene.h"
#include <glm/gtx/extended_min_max.hpp>
#include <glm/gtx/io.hpp>
#include <iostream>
#include <algorithm>

using namespace std;

bool Geometry::intersect(ray &r, isect &i) const {
  double tmin, tmax;
  if (hasBoundingBoxCapability() && !(bounds.intersect(r, tmin, tmax)))
    return false;
  // Transform the ray into the object's local coordinate space
  glm::dvec3 pos = transform.globalToLocalCoords(r.getPosition());
  glm::dvec3 dir =
      transform.globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
  double length = glm::length(dir);
  dir = glm::normalize(dir);
  // Backup World pos/dir, and switch to local pos/dir
  glm::dvec3 Wpos = r.getPosition();
  glm::dvec3 Wdir = r.getDirection();
  r.setPosition(pos);
  r.setDirection(dir);
  bool rtrn = false;
  if (intersectLocal(r, i)) {
    // Transform the intersection point & normal returned back into
    // global space.
    i.setN(transform.localToGlobalCoordsNormal(i.getN()));
    i.setT(i.getT() / length);
    rtrn = true;
  }
  // Restore World pos/dir
  r.setPosition(Wpos);
  r.setDirection(Wdir);
  return rtrn;
}

bool Geometry::hasBoundingBoxCapability() const {
  // by default, primitives do not have to specify a bounding box. If this
  // method returns true for a primitive, then either the ComputeBoundingBox()
  // or the ComputeLocalBoundingBox() method must be implemented.

  // If no bounding box capability is supported for an object, that object will
  // be checked against every single ray drawn. This should be avoided whenever
  // possible, but this possibility exists so that new primitives will not have
  // to have bounding boxes implemented for them.
  return false;
}

void Geometry::ComputeBoundingBox() {
  // take the object's local bounding box, transform all 8 points on it,
  // and use those to find a new bounding box.

  BoundingBox localBounds = ComputeLocalBoundingBox();

  glm::dvec3 min = localBounds.getMin();
  glm::dvec3 max = localBounds.getMax();

  glm::dvec4 v, newMax, newMin;

  v = transform.localToGlobalCoords(glm::dvec4(min[0], min[1], min[2], 1));
  newMax = v;
  newMin = v;
  v = transform.localToGlobalCoords(glm::dvec4(max[0], min[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], max[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], max[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], min[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], min[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], max[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], max[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);

  bounds.setMax(glm::dvec3(newMax));
  bounds.setMin(glm::dvec3(newMin));
}

Scene::Scene() { ambientIntensity = glm::dvec3(0, 0, 0); }

Scene::~Scene() {
  for (auto &obj : objects)
    delete obj;
  for (auto &light : lights)
    delete light;
}

void Scene::add(Geometry *obj) {
  obj->ComputeBoundingBox();
  sceneBounds.merge(obj->getBoundingBox());
  objects.emplace_back(obj);
}

void Scene::add(Light *light) { lights.emplace_back(light); }

void SceneBVH::build(const std::vector<Geometry*>& objects) {
    delete root;
    std::vector<Geometry*> copy = objects;
    root = buildRecursive(copy, 0);
}

SceneBVHNode* SceneBVH::buildRecursive(std::vector<Geometry*>& objects, int depth) {
    SceneBVHNode* node = new SceneBVHNode();
    
    for (auto obj : objects) {
        if (obj->hasBoundingBoxCapability()) {
            node->bounds.merge(obj->getBoundingBox());
        }
    }

    if (objects.size() <= 4 || depth > 20) {
        node->objects = objects;
        return node;
    }

    glm::dvec3 ext = node->bounds.getMax() - node->bounds.getMin();
    int axis = 0;
    if (ext.y > ext.x && ext.y > ext.z) axis = 1;
    else if (ext.z > ext.x && ext.z > ext.y) axis = 2;

    std::sort(objects.begin(), objects.end(), [axis](Geometry* a, Geometry* b) {
        double aMin = a->hasBoundingBoxCapability() ? a->getBoundingBox().getMin()[axis] : 0.0;
        double bMin = b->hasBoundingBoxCapability() ? b->getBoundingBox().getMin()[axis] : 0.0;
        return aMin < bMin;
    });

    size_t mid = objects.size() / 2;
    std::vector<Geometry*> leftObjs(objects.begin(), objects.begin() + mid);
    std::vector<Geometry*> rightObjs(objects.begin() + mid, objects.end());

    node->left = buildRecursive(leftObjs, depth + 1);
    node->right = buildRecursive(rightObjs, depth + 1);

    return node;
}

bool SceneBVH::intersect(ray& r, isect& i) const {
    if (!root) return false;
    return intersectNode(root, r, i);
}

bool SceneBVH::intersectNode(SceneBVHNode* node, ray& r, isect& i) const {
    double tmin, tmax;
    if (!node->bounds.intersect(r, tmin, tmax)) return false;

    bool hit = false;

    if (node->isLeaf()) {
        for (auto obj : node->objects) {
            isect cur;
            if (obj->intersect(r, cur)) {
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

// Get any intersection with an object.  Return information about the
// intersection through the reference parameter.
bool Scene::intersect(ray &r, isect &i) const {
  if (!bvhBuilt) {
      bvh.build(objects);
      bvhBuilt = true;
  }

  bool have_one = bvh.intersect(r, i);
  
  if (!have_one) {
    i.setT(1000.0);
  }
  
  if (TraceUI::m_debug) {
    addToIntersectCache(std::make_pair(new ray(r), new isect(i)));
  }
  
  return have_one;
}

TextureMap *Scene::getTexture(string name) {
  auto itr = textureCache.find(name);
  if (itr == textureCache.end()) {
    textureCache[name].reset(new TextureMap(name));
    return textureCache[name].get();
  }
  return itr->second.get();
}
