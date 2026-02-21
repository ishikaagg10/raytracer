#pragma once

#include "bbox.h"
#include <vector>

class Geometry;
class ray;
class isect;

class SceneBVHNode {
public:
    BoundingBox bounds;
    SceneBVHNode* left;
    SceneBVHNode* right;
    std::vector<Geometry*> objects;

    SceneBVHNode() : left(nullptr), right(nullptr) {}
    ~SceneBVHNode() { 
        delete left; 
        delete right; 
    }
    
    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

class SceneBVH {
public:
    SceneBVH() : root(nullptr) {}
    ~SceneBVH() { delete root; }

    void build(const std::vector<Geometry*>& objects);
    bool intersect(ray& r, isect& i) const;

private:
    SceneBVHNode* root;
    SceneBVHNode* buildRecursive(std::vector<Geometry*>& objects, int depth);
    bool intersectNode(SceneBVHNode* node, ray& r, isect& i) const;
};