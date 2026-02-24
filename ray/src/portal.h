#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "scene/scene.h"
#include "scene/ray.h"
#include <glm/glm.hpp>

static constexpr int PORTAL_MAX_BOUNCES = 4;

class Portal : public SceneObject {
public:
    Portal(Scene*             scene,
           Material*          mat,
           const glm::dvec3&  center,
           const glm::dvec3&  normal,
           const glm::dvec3&  upHint,
           double             radius,
           const glm::dvec3&  color    = glm::dvec3(0.3, 0.6, 1.0),
           const glm::dvec3&  rimColor = glm::dvec3(0.1, 0.3, 0.9));

    static void link(Portal* a, Portal* b) {
        a->partner = b;
        b->partner = a;
    }

    Portal* getPartner() const { return partner; }

    const glm::dvec3& getCenter()    const { return center; }
    const glm::dvec3& getNormal()    const { return portalNormal; }
    const glm::dvec3& getTangent()   const { return tangent; }
    const glm::dvec3& getBitangent() const { return bitangent; }
    double            getRadius()    const { return radius; }
    const glm::dvec3& getColor()     const { return portalColor; }
    const glm::dvec3& getRimColor()  const { return rimColor; }

    struct HitRecord {
        double  t      = -1.0;
        Portal* portal = nullptr; // which portal was hit
    };
    static thread_local HitRecord lastHit;

    bool teleportRay(const ray& incoming, ray& outgoing) const;

    bool        hasBoundingBoxCapability() const override { return true; }
    BoundingBox ComputeLocalBoundingBox() override;

protected:
    bool intersectLocal(ray& r, isect& i) const override;

private:
    void buildFrame(const glm::dvec3& normalIn, const glm::dvec3& upHint);

    glm::dvec3 center;
    glm::dvec3 portalNormal;
    glm::dvec3 tangent;
    glm::dvec3 bitangent;
    double     radius;
    glm::dvec3 portalColor;
    glm::dvec3 rimColor;

    Portal* partner = nullptr;
};

#endif