#include "portal.h"
#include "scene/material.h"
#include "scene/ray.h"
#include <algorithm>
#include <cmath>

thread_local Portal::HitRecord Portal::lastHit;

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

void Portal::buildFrame(const glm::dvec3& normalIn, const glm::dvec3& upHint)
{
    portalNormal = glm::normalize(normalIn);

    glm::dvec3 up = glm::normalize(upHint);
    if (std::abs(glm::dot(up, portalNormal)) > 0.99) {
        up = (std::abs(portalNormal.x) < 0.9)
             ? glm::dvec3(1, 0, 0)
             : glm::dvec3(0, 1, 0);
    }

    tangent   = glm::normalize(glm::cross(up, portalNormal));
    bitangent = glm::normalize(glm::cross(portalNormal, tangent));
}

BoundingBox Portal::ComputeLocalBoundingBox()
{
    double pad = radius * 0.01;
    BoundingBox bb;
    bb.setMin(center - glm::dvec3(radius + pad));
    bb.setMax(center + glm::dvec3(radius + pad));
    return bb;
}

bool Portal::intersectLocal(ray& r, isect& i) const
{
    double denom = glm::dot(r.getDirection(), portalNormal);
    if (denom > -1e-8) return false;

    double t = glm::dot(center - r.getPosition(), portalNormal) / denom;
    if (t < 1e-7) return false;

    glm::dvec3 hit  = r.at(t);
    glm::dvec3 diff = hit - center;
    double dist2 = glm::dot(diff, diff);
    if (dist2 > radius * radius) return false;

    i.setT(t);
    i.setN(portalNormal);
    i.setObject(this);

    double u = glm::dot(diff, tangent)   / radius * 0.5 + 0.5;
    double v = glm::dot(diff, bitangent) / radius * 0.5 + 0.5;
    i.setUVCoordinates(glm::dvec2(u, v));

    double normDist = std::sqrt(dist2) / radius;
    Material m = getMaterial();
    if (normDist > 0.92) {
        m.setDiffuse(rimColor);
        m.setAmbient(rimColor);
        m.setEmissive(rimColor * 0.7);
    } else {
        m.setDiffuse(portalColor);
        m.setAmbient(portalColor);
        m.setEmissive(portalColor * 0.8);
    }
    i.setMaterial(m);

    if (lastHit.t < 0.0 || t < lastHit.t) {
        lastHit.t      = t;
        lastHit.portal = const_cast<Portal*>(this);
    }

    return true;
}

bool Portal::teleportRay(const ray& incoming, ray& outgoing) const
{
    if (!partner) return false;

    glm::dvec3 entryX =  tangent;
    glm::dvec3 entryY =  bitangent;
    glm::dvec3 entryZ = -portalNormal;

    glm::dvec3 exitX =  partner->tangent;
    glm::dvec3 exitY =  partner->bitangent;
    glm::dvec3 exitZ =  partner->portalNormal;

    glm::dvec3 d  = glm::normalize(incoming.getDirection());
    double dx = glm::dot(d, entryX);
    double dy = glm::dot(d, entryY);
    double dz = glm::dot(d, entryZ);

    glm::dvec3 newDir = glm::normalize(dx * exitX + dy * exitY + dz * exitZ);

    glm::dvec3 hitPt  = incoming.getPosition();
    glm::dvec3 diff   = hitPt - center;
    double px = glm::dot(diff, entryX);
    double py = glm::dot(diff, entryY);

    glm::dvec3 newOrigin = partner->center
                         + px * exitX
                         + py * exitY
                         + newDir * 1e-4;

    outgoing = ray(newOrigin, newDir, incoming.getAtten(), ray::VISIBILITY);
    return true;
}