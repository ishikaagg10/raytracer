/*
 * portal.cpp
 */

#include "portal.h"
#include "scene/material.h"
#include "scene/ray.h"
#include <algorithm>
#include <cmath>

// ── Thread-local side-channel definition ─────────────────────────────────────
// One record per thread; safe for multithreaded rendering.
thread_local Portal::HitRecord Portal::lastHit;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Portal::Portal(Scene*             scene,
               Material*          mat,
               const glm::dvec3&  centerIn,
               const glm::dvec3&  normalIn,
               const glm::dvec3&  upHint,
               double             radiusIn,
               const glm::dvec3&  color,
               const glm::dvec3&  rimColorIn)
    : SceneObject(scene, mat),
      center(centerIn),
      radius(radiusIn),
      portalColor(color),
      rimColor(rimColorIn)
{
    buildFrame(glm::normalize(normalIn), upHint);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFrame  –  construct orthonormal {tangent, bitangent, portalNormal}
// ─────────────────────────────────────────────────────────────────────────────
void Portal::buildFrame(const glm::dvec3& normalIn, const glm::dvec3& upHint)
{
    portalNormal = glm::normalize(normalIn);

    glm::dvec3 up = glm::normalize(upHint);
    // If up is nearly parallel to normal, choose a safe fallback
    if (std::abs(glm::dot(up, portalNormal)) > 0.99) {
        up = (std::abs(portalNormal.x) < 0.9)
             ? glm::dvec3(1, 0, 0)
             : glm::dvec3(0, 1, 0);
    }

    tangent   = glm::normalize(glm::cross(up, portalNormal));
    bitangent = glm::normalize(glm::cross(portalNormal, tangent));
}

// ─────────────────────────────────────────────────────────────────────────────
// ComputeLocalBoundingBox
// ─────────────────────────────────────────────────────────────────────────────
BoundingBox Portal::ComputeLocalBoundingBox()
{
    // Portal uses an identity transform, so local == world.
    // Pad along all axes by radius so the box is never degenerate.
    double pad = radius * 0.01;
    BoundingBox bb;
    bb.setMin(center - glm::dvec3(radius + pad));
    bb.setMax(center + glm::dvec3(radius + pad));
    return bb;
}

// ─────────────────────────────────────────────────────────────────────────────
// intersectLocal
// ─────────────────────────────────────────────────────────────────────────────
// Portal uses an identity MatrixTransform, so "local space" == world space.
// We do the full disc intersection here.
bool Portal::intersectLocal(ray& r, isect& i) const
{
    // ── 1. Ray–plane intersection ─────────────────────────────────────────────
    // Only intersect rays hitting the front face (coming against the normal).
    double denom = glm::dot(r.getDirection(), portalNormal);
    if (denom > -1e-8) return false;   // parallel or back-face

    double t = glm::dot(center - r.getPosition(), portalNormal) / denom;
    if (t < 1e-7) return false;         // behind ray origin

    // ── 2. Clip to disc radius ────────────────────────────────────────────────
    glm::dvec3 hit  = r.at(t);
    glm::dvec3 diff = hit - center;
    double dist2 = glm::dot(diff, diff);
    if (dist2 > radius * radius) return false;

    // ── 3. Fill isect ─────────────────────────────────────────────────────────
    i.setT(t);
    i.setN(portalNormal);
    i.setObject(this);      // so setObject is still correct

    // UV: map disc position to [0,1]² centred at (0.5, 0.5)
    double u = glm::dot(diff, tangent)   / radius * 0.5 + 0.5;
    double v = glm::dot(diff, bitangent) / radius * 0.5 + 0.5;
    i.setUVCoordinates(glm::dvec2(u, v));

    // Bake appearance into a per-hit material copy
    double normDist = std::sqrt(dist2) / radius;
    Material m = getMaterial();
    if (normDist > 0.92) {
        // Rim ring – bright emissive so it glows
        m.setDiffuse(rimColor);
        m.setAmbient(rimColor);
        m.setEmissive(rimColor * 0.7);
    } else {
        // Interior – show portal tint while traceRay shoots through it
        m.setDiffuse(portalColor);
        m.setAmbient(portalColor);
        m.setEmissive(portalColor * 0.8);
    }
    i.setMaterial(m);

    // ── 4. Write side-channel so traceRay can find us ─────────────────────────
    // We update lastHit unconditionally here; the BVH will call intersectLocal
    // for every candidate, but scene::intersect() returns the *minimum-t* hit.
    // We therefore update lastHit whenever t is smaller than any previous write
    // in this ray's traversal.  traceRay confirms by comparing t values.
    if (lastHit.t < 0.0 || t < lastHit.t) {
        lastHit.t      = t;
        lastHit.portal = const_cast<Portal*>(this);
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// teleportRay
// ─────────────────────────────────────────────────────────────────────────────
// Decomposes the incoming direction into the entry portal's local frame then
// re-expresses it in the exit portal's frame so the ray "walks through" the
// portal naturally (180° handedness flip around the disc normal).
bool Portal::teleportRay(const ray& incoming, ray& outgoing) const
{
    if (!partner) return false;

    // Entry frame: ray comes IN through this portal (against normal)
    glm::dvec3 entryX =  tangent;
    glm::dvec3 entryY =  bitangent;
    glm::dvec3 entryZ = -portalNormal;   // "into" this portal

    // Exit frame: ray goes OUT through partner (along partner normal)
    glm::dvec3 exitX =  partner->tangent;
    glm::dvec3 exitY =  partner->bitangent;
    glm::dvec3 exitZ =  partner->portalNormal;

    // Decompose incoming direction in entry frame
    glm::dvec3 d  = glm::normalize(incoming.getDirection());
    double dx = glm::dot(d, entryX);
    double dy = glm::dot(d, entryY);
    double dz = glm::dot(d, entryZ);

    // Reconstruct in exit frame
    glm::dvec3 newDir = glm::normalize(dx * exitX + dy * exitY + dz * exitZ);

    // Map the 2-D disc position from entry to exit
    glm::dvec3 hitPt  = incoming.getPosition();   // caller sets origin = hit point
    glm::dvec3 diff   = hitPt - center;
    double px = glm::dot(diff, entryX);
    double py = glm::dot(diff, entryY);

    glm::dvec3 newOrigin = partner->center
                         + px * exitX
                         + py * exitY
                         + newDir * 1e-4;   // nudge to avoid self-intersection

    outgoing = ray(newOrigin, newDir, incoming.getAtten(), ray::VISIBILITY);
    return true;
}