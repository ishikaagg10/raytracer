#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
#include <algorithm>
#include <cmath>
extern TraceUI *traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material() {}

glm::dvec3 Material::shade(Scene *scene, const ray &r, const isect &i) const {
  glm::dvec3 P = r.at(i.getT()); 
  glm::dvec3 N = glm::normalize(i.getN());

  // Normal Mapping
  if (hasNormalMap()) {
      glm::dvec3 mapColor = kn(i);
      glm::dvec3 tangentNormal = glm::normalize(mapColor * 2.0 - 1.0);

      glm::dvec3 up = (std::abs(N.z) < 0.9) ? glm::dvec3(0, 0, 1) : glm::dvec3(1, 0, 0);
      glm::dvec3 T = glm::normalize(glm::cross(up, N));
      glm::dvec3 B = glm::cross(N, T);

      N = glm::normalize(tangentNormal.x * T + tangentNormal.y * B + tangentNormal.z * N);
  }

  glm::dvec3 V = glm::normalize(-r.getDirection());
  glm::dvec3 totalColor = ke(i) + ka(i) * scene->ambient();

  for ( const auto& pLight : scene->getAllLights() ) {
      glm::dvec3 L = glm::normalize(pLight->getDirection(P));
      double nDotL = std::max(0.0, glm::dot(N, L));
      
      glm::dvec3 diffuseTerm = kd(i) * nDotL;
      glm::dvec3 specularTerm(0.0, 0.0, 0.0);
      
      if (nDotL > 0.0) {
          glm::dvec3 R = glm::normalize(glm::reflect(-L, N));
          double rDotV = std::max(0.0, glm::dot(R, V));
          specularTerm = ks(i) * pow(rDotV, shininess(i));
      }

      glm::dvec3 lightIntensity = pLight->getColor();
      double distAtten = pLight->distanceAttenuation(P);
      glm::dvec3 shadowAtten = pLight->shadowAttenuation(r, P);
      totalColor += shadowAtten * distAtten * lightIntensity * (diffuseTerm + specularTerm);
  }

  return totalColor;
}

TextureMap::TextureMap(string filename) {
  data = readImage(filename.c_str(), width, height);
  if (data.empty()) {
    throw TextureMapException("Unable to load texture: " + filename);
  }
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2 &coord) const {
    double x = coord[0] * (width - 1);
    double y = coord[1] * (height - 1);
    int x0 = (int)floor(x); int y0 = (int)floor(y);
    int x1 = std::min(x0 + 1, width - 1); int y1 = std::min(y0 + 1, height - 1);
    double dx = x - x0; double dy = y - y0;

    glm::dvec3 c00 = getPixelAt(x0, y0);
    glm::dvec3 c10 = getPixelAt(x1, y0);
    glm::dvec3 c01 = getPixelAt(x0, y1);
    glm::dvec3 c11 = getPixelAt(x1, y1);

    glm::dvec3 top = c00 * (1.0 - dx) + c10 * dx;
    glm::dvec3 bottom = c01 * (1.0 - dx) + c11 * dx;
    return top * (1.0 - dy) + bottom * dy;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    int index = (y * width + x) * 3;
    return glm::dvec3(data[index]/255.0, data[index+1]/255.0, data[index+2]/255.0);
}

glm::dvec3 MaterialParameter::value(const isect &is) const {
  if (_textureMap) return _textureMap->getMappedValue(is.getUVCoordinates());
  return _value;
}

double MaterialParameter::intensityValue(const isect &is) const {
  glm::dvec3 v = value(is);
  return (0.299 * v[0]) + (0.587 * v[1]) + (0.114 * v[2]);
}