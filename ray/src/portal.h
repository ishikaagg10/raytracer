#ifndef __PORTAL_H__
#define __PORTAL_H__

/*
 * portal.h
 *
 * Portal – a disc-shaped SceneObject that teleports rays to a linked partner.
 *
 * ── How portal detection works ───────────────────────────────────────────────
 * isect has setObject() but no getObject(), so we cannot recover the Portal*
 * from an isect after the fact.  Instead, Portal::intersectLocal() writes
 * {t, Portal*} into a thread_local HitRecord (Portal::lastHit) every time it
 * records a successful intersection.  traceRay reads it back immediately after
 * scene->intersect() returns and confirms the t values match — if they do, the
 * closest hit was a portal.  Thread-local storage makes this safe for
 * multithreaded rendering.
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * JSON scene format — "portals" is a top-level key whose value is an array.
 * Adjacent pairs [0,1], [2,3], … are automatically linked:
 *
 *   {"portals": [
 *     { "center":[x,y,z], "normal":[x,y,z], "up":[x,y,z],
 *       "radius":1.0, "color":[r,g,b], "rimColor":[r,g,b] },
 *     { "center":[x,y,z], "normal":[x,y,z], ... }
 *   ]}
 *
 * Only "center" and "normal" are required.
 */

#include "scene/scene.h"
#include "scene/ray.h"
#include <glm/glm.hpp>

// Maximum portal-through-portal bounces before giving up and returning rim color.
static constexpr int PORTAL_MAX_BOUNCES = 4;

class Portal : public SceneObject {
public:
    // ── Construction ──────────────────────────────────────────────────────────
    Portal(Scene*             scene,
           Material*          mat,
           const glm::dvec3&  center,
           const glm::dvec3&  normal,
           const glm::dvec3&  upHint,
           double             radius,
           const glm::dvec3&  color    = glm::dvec3(0.3, 0.6, 1.0),
           const glm::dvec3&  rimColor = glm::dvec3(0.1, 0.3, 0.9));

    // Link two portals as a pair.  Call once before rendering.
    static void link(Portal* a, Portal* b) {
        a->partner = b;
        b->partner = a;
    }

    Portal* getPartner() const { return partner; }

    // ── Accessors ─────────────────────────────────────────────────────────────
    const glm::dvec3& getCenter()    const { return center; }
    const glm::dvec3& getNormal()    const { return portalNormal; }
    const glm::dvec3& getTangent()   const { return tangent; }
    const glm::dvec3& getBitangent() const { return bitangent; }
    double            getRadius()    const { return radius; }
    const glm::dvec3& getColor()     const { return portalColor; }
    const glm::dvec3& getRimColor()  const { return rimColor; }

    // ── Portal-hit side-channel ───────────────────────────────────────────────
    // Because isect::getObject() does not exist we use a thread_local record
    // written by intersectLocal() and read by RayTracer::traceRay().
    // If lastHit.t matches i.getT() after scene->intersect(), the hit was us.
    struct HitRecord {
        double  t      = -1.0;   // t value of the recorded hit (-1 = none)
        Portal* portal = nullptr; // which portal was hit
    };
    static thread_local HitRecord lastHit;

    // ── Teleportation ─────────────────────────────────────────────────────────
    // incoming.getPosition() must be the world-space hit point on this portal.
    // outgoing is filled with the transformed ray starting just outside the
    // partner disc.  Returns false if there is no partner.
    bool teleportRay(const ray& incoming, ray& outgoing) const;

    // ── Geometry interface ────────────────────────────────────────────────────
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

#endif // __PORTAL_H__