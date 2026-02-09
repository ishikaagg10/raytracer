#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI *traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material() {}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray &r, const isect &i) const {
  // For now, this method just returns the diffuse color of the object.
  // This gives a single matte color for every distinct surface in the
  // scene, and that's it.  Simple, but enough to get you started.
  // (It's also inconsistent with the phong model...)

  // Your mission is to fill in this method with the rest of the phong
  // shading model, including the contributions of all the light sources.
  // You will need to call both distanceAttenuation() and
  // shadowAttenuation()
  // somewhere in your code in order to compute shadows and light falloff.
  //	if( debugMode )
  //		std::cout << "Debugging Phong code..." << std::endl;

  // When you're iterating through the lights,
  // you'll want to use code that looks something
  // like this:
  //
  // for ( const auto& pLight : scene->getAllLights() )
  // {
  //              // pLight has type Light*
  // 		.
  // 		.
  // 		.
  // }
  
  glm::dvec3 P = r.at(i.getT()); 
  glm::dvec3 N = glm::normalize(i.getN());
  glm::dvec3 V = glm::normalize(-r.getDirection());

  glm::dvec3 totalColor = ke(i) + ka(i) * scene->ambient();

  for ( const auto& pLight : scene->getAllLights() ) {
      glm::dvec3 L = pLight->getDirection(P);
      glm::dvec3 L_norm = glm::normalize(L);

      double nDotL = std::max(0.0, glm::dot(N, L_norm));
      glm::dvec3 diffuseTerm = kd(i) * nDotL;

      glm::dvec3 specularTerm(0.0, 0.0, 0.0);
      
      if (nDotL > 0.0) {
          glm::dvec3 R = glm::normalize(glm::reflect(-L_norm, N));
          
          double rDotV = std::max(0.0, glm::dot(R, V));
          double specFactor = pow(rDotV, shininess(i));
          specularTerm = ks(i) * specFactor;
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
    width = 0;
    height = 0;
    string error("Unable to load texture map '");
    error.append(filename);
    error.append("'.");
    throw TextureMapException(error);
  }
}
glm::dvec3 TextureMap::getMappedValue(const glm::dvec2 &coord) const {
    double x = coord[0] * (width - 1);
    double y = coord[1] * (height - 1);

    int x0 = (int)floor(x);
    int y0 = (int)floor(y);

    int x1 = x0 + 1;
    int y1 = y0 + 1;

    double dx = x - x0;
    double dy = y - y0;

    glm::dvec3 c00 = getPixelAt(x0, y0); // Top-Left
    glm::dvec3 c10 = getPixelAt(x1, y0); // Top-Right
    glm::dvec3 c01 = getPixelAt(x0, y1); // Bottom-Left
    glm::dvec3 c11 = getPixelAt(x1, y1); // Bottom-Right

    glm::dvec3 top = c00 * (1.0 - dx) + c10 * dx;
    glm::dvec3 bottom = c01 * (1.0 - dx) + c11 * dx;

    return top * (1.0 - dy) + bottom * dy;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    int index = (y * width + x) * 3;

    if (index < 0 || index + 2 >= data.size()) {
        return glm::dvec3(0, 0, 0);
    }

    double r = data[index] / 255.0;
    double g = data[index + 1] / 255.0;
    double b = data[index + 2] / 255.0;

    return glm::dvec3(r, g, b);
}

glm::dvec3 MaterialParameter::value(const isect &is) const {
  if (0 != _textureMap)
    return _textureMap->getMappedValue(is.getUVCoordinates());
  else
    return _value;
}

double MaterialParameter::intensityValue(const isect &is) const {
  if (0 != _textureMap) {
    glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
    return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
  } else
    return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}
