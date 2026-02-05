#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3 &) const {
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const ray &r,
                                               const glm::dvec3 &p) const {
  // You should implement shadow-handling code here.
  glm::dvec3 attenuation(1.0, 1.0, 1.0);
  glm::dvec3 L = getDirection(p);
  
  ray shadowRay(p + (L * 0.0001), L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
  isect i;

  while (scene->intersect(shadowRay, i)) {
      const Material& m = i.getMaterial();
      
      if (glm::length(m.kt(i)) < 1e-6) {
          return glm::dvec3(0.0, 0.0, 0.0);
      }

      attenuation *= m.kt(i);
      glm::dvec3 hitPoint = shadowRay.at(i.getT());
      shadowRay = ray(hitPoint + (L * 0.0001), L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
  }

  return attenuation;
}

glm::dvec3 DirectionalLight::getColor() const { return color; }

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3 &) const {
  return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3 &P) const {
  // You'll need to modify this method to attenuate the intensity
  // of the light based on the distance between the source and the
  // point P.  For now, we assume no attenuation and just return 1.0

    double d = glm::distance(position, P);
    double denominator = constantTerm + linearTerm * d + quadraticTerm * d * d;

    if (denominator < 1e-7) return 1.0;
    
    return std::min(1.0, 1.0 / denominator);
}

glm::dvec3 PointLight::getColor() const { return color; }

glm::dvec3 PointLight::getDirection(const glm::dvec3 &P) const {
  return glm::normalize(position - P);
}

glm::dvec3 PointLight::shadowAttenuation(const ray &r,
                                         const glm::dvec3 &p) const {
  // You should implement shadow-handling code here.

  glm::dvec3 attenuation(1.0, 1.0, 1.0);
  
  glm::dvec3 D = position - p;
  double distToLight = glm::length(D);
  glm::dvec3 L = glm::normalize(D);

  ray shadowRay(p + (L * 0.0001), L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
  isect i;

  while (scene->intersect(shadowRay, i)) {
      if (i.getT() >= distToLight) {
          break; 
      }

      const Material& m = i.getMaterial();

      if (glm::length(m.kt(i)) < 1e-6) {
          return glm::dvec3(0.0, 0.0, 0.0);
      }

      attenuation *= m.kt(i);
      glm::dvec3 hitPoint = shadowRay.at(i.getT());
      distToLight = glm::distance(position, hitPoint);
      shadowRay = ray(hitPoint + (L * 0.0001), L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
  }

  return attenuation;
}

#define VERBOSE 0

