// The main ray tracer.

#pragma warning(disable : 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "portal.h"

#include "parser/JsonParser.h"
#include "parser/Parser.h"
#include "parser/Tokenizer.h"
#include <json.hpp>

#include "ui/TraceUI.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h>

#include <fstream>
#include <iostream>

using namespace std;
extern TraceUI *traceUI;

bool debugMode = false;

glm::dvec3 RayTracer::trace(double x, double y) {
  if (TraceUI::m_debug) {
    scene->clearIntersectCache();
  }

  ray r(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::VISIBILITY);
  scene->getCamera().rayThrough(x, y, r);
  double dummy;
  glm::dvec3 ret =
      traceRay(r, glm::dvec3(1.0, 1.0, 1.0), traceUI->getDepth(), dummy);
  ret = glm::clamp(ret, 0.0, 1.0);
  return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j) {
  glm::dvec3 col(0, 0, 0);

  if (!sceneLoaded())
    return col;

  int numSamples = samples;

  if (numSamples <= 1) {
      // anti-aliasing off
      double x = (double(i) + 0.5) / double(buffer_width);
      double y = (double(j) + 0.5) / double(buffer_height);
      col = trace(x, y);
      
  } else {
      // anti-aliasing on: Stochastic (Jittered) Supersampling
      for (int p = 0; p < numSamples; ++p) {
          for (int q = 0; q < numSamples; ++q) {
              double r1 = double(rand()) / RAND_MAX;
              double r2 = double(rand()) / RAND_MAX;

              double xOffset = (double(p) + r1) / double(numSamples);
              double yOffset = (double(q) + r2) / double(numSamples);

              double x = (double(i) + xOffset) / double(buffer_width);
              double y = (double(j) + yOffset) / double(buffer_height);

              col += trace(x, y);
          }
      }

      col /= double(numSamples * numSamples);
  }

  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  pixel[0] = (int)(255.0 * col[0]);
  pixel[1] = (int)(255.0 * col[1]);
  pixel[2] = (int)(255.0 * col[2]);
  
  return col;
}

#define VERBOSE 0

// ─────────────────────────────────────────────────────────────────────────────
// traceRay
// ─────────────────────────────────────────────────────────────────────────────
// Portal detection: isect has no getObject() accessor so we use the
// thread_local Portal::lastHit side-channel written by Portal::intersectLocal().
// Before calling scene->intersect() we reset lastHit.t to -1 so we don't
// confuse hits from prior rays.  After the call, if lastHit.t matches
// i.getT() exactly, the closest intersection was a portal.
// ─────────────────────────────────────────────────────────────────────────────
glm::dvec3 RayTracer::traceRay(ray &r, const glm::dvec3 &thresh, int depth,
                               double &t) {
    if (glm::max(thresh.r, glm::max(thresh.g, thresh.b)) < traceUI->getThreshold()) {
      return glm::dvec3(0.0, 0.0, 0.0);
  }
  isect i;
  glm::dvec3 colorC;
#if VERBOSE
  std::cerr << "== current depth: " << depth << std::endl;
#endif

  // Reset the portal side-channel before each intersection test.
  Portal::lastHit.t      = -1.0;
  Portal::lastHit.portal = nullptr;

  if (scene->intersect(r, i)) {

    // ── Portal hit? ───────────────────────────────────────────────────────────
    // lastHit is valid AND its t matches the winning isect t → portal hit.
    Portal* hitPortal = nullptr;
    if (Portal::lastHit.portal != nullptr &&
        Portal::lastHit.t == i.getT()) {
      hitPortal = Portal::lastHit.portal;
    }

    if (hitPortal && hitPortal->getPartner()) {

      // Per-thread bounce counter to prevent infinite portal loops.
      static thread_local int portalBounces = 0;

      // The material already has rim/interior color baked in by intersectLocal.
      const Material &m = i.getMaterial();
      glm::dvec3 rimContrib = m.shade(scene.get(), r, i);

      if (portalBounces >= PORTAL_MAX_BOUNCES) {
        return glm::clamp(rimContrib, 0.0, 1.0);
      }

      // Determine rim vs interior from UV (set by intersectLocal).
      glm::dvec2 uv = i.getUVCoordinates();
      double du = uv.x - 0.5;
      double dv = uv.y - 0.5;
      double radialFrac = 2.0 * std::sqrt(du * du + dv * dv); // 0=centre,1=rim

      if (radialFrac > 0.92) {
        // Rim ring: just show the rim color, don't teleport.
        return glm::clamp(rimContrib, 0.0, 1.0);
      }

      // Interior: teleport the ray and recurse.
      glm::dvec3 hitPt = r.at(i.getT());
      ray hitRay(hitPt, r.getDirection(), r.getAtten(), r.type());

      ray teleported(glm::dvec3(0), glm::dvec3(0, 0, 1),
                     glm::dvec3(1, 1, 1), ray::VISIBILITY);

      if (hitPortal->teleportRay(hitRay, teleported)) {
        portalBounces++;
        double dummyT;
        glm::dvec3 portalViewColor = traceRay(teleported, thresh, depth, dummyT);
        portalBounces--;

        // Slight tint from portal color (8 %), rest is the true view.
        glm::dvec3 tint   = hitPortal->getColor();
        glm::dvec3 result = portalViewColor * (0.92 * glm::dvec3(1.0) + 0.08 * tint);
        return glm::clamp(result, 0.0, 1.0);
      }

      // teleportRay returned false (no partner) – fall through to normal shading.
    }

    // ── Normal (non-portal) shading ───────────────────────────────────────────
    const Material &m = i.getMaterial();
    colorC = m.shade(scene.get(), r, i);

    // ── Reflection ────────────────────────────────────────────────────────────
    if (depth > 0 && glm::length(m.kr(i)) > 0) {
      // Calculate new threshold: current threshold * reflection coefficient
      glm::dvec3 nextThresh = thresh * m.kr(i);
      glm::dvec3 N = glm::normalize(i.getN());
      glm::dvec3 V = glm::normalize(r.getDirection());
      glm::dvec3 R = glm::normalize(glm::reflect(V, N));
      glm::dvec3 P = r.at(i.getT());

      glm::dvec3 offsetN = (glm::dot(N, V) < 0) ? N : -N;
      ray reflectedRay(P + offsetN * 0.0001, R, glm::dvec3(1.0), ray::REFLECTION);

      double dummyT;
      // colorC += m.kr(i) * traceRay(reflectedRay, thresh, depth - 1, dummyT);
      colorC += m.kr(i) * traceRay(reflectedRay, nextThresh, depth - 1, dummyT);
    }

    // ── Refraction ────────────────────────────────────────────────────────────
    if (depth > 0 && glm::length(m.kt(i)) > 0) {
      // Calculate new threshold: current threshold * transmission coefficient
      glm::dvec3 nextThresh = thresh * m.kt(i);
      glm::dvec3 N = glm::normalize(i.getN());
      glm::dvec3 V = glm::normalize(r.getDirection());

      double eta;
      double nDotV = glm::dot(N, V);
      glm::dvec3 effectiveN;

      if (nDotV < 0) {
        eta      = 1.0 / m.index(i);
        effectiveN = N;
        nDotV    = -nDotV;
      } else {
        eta      = m.index(i) / 1.0;
        effectiveN = -N;
      }

      double discriminant = 1.0 - (eta * eta) * (1.0 - nDotV * nDotV);
      glm::dvec3 P = r.at(i.getT());

      if (discriminant >= 0.0) {
        double cosThetaT = sqrt(discriminant);
        glm::dvec3 T = glm::normalize(
            eta * V + (eta * nDotV - cosThetaT) * effectiveN);
        ray refractedRay(P + T * 0.0001, T, glm::dvec3(1.0), ray::REFRACTION);

        double dummyT;
        glm::dvec3 refractedColor = traceRay(refractedRay, nextThresh, depth - 1, dummyT);

    // --- RADIOACTIVE GOOP ADDITION ---
    // If nDotV > 0, we are EXITING the object, meaning the ray just traveled 
    // through the internal volume of the goop.
    if (glm::dot(i.getN(), V) > 0) {
        // Add emission scaled by distance traveled (dummyT is the distance to the next hit)
        refractedColor += m.ke(i) * dummyT; 
    }
    // ---------------------------------

    colorC += m.kt(i) * refractedColor;
      } else {
        // Total internal reflection
        glm::dvec3 R = glm::normalize(glm::reflect(V, effectiveN));
        ray reflectedRay(P + R * 0.0001, R, glm::dvec3(1.0), ray::REFLECTION);

        double dummyT;
        // colorC += m.kt(i) * traceRay(reflectedRay, thresh, depth - 1, dummyT);
        colorC += m.kt(i) * traceRay(reflectedRay, nextThresh, depth - 1, dummyT);
      }
    }

  } else {
    // ── No intersection – background / cube map ───────────────────────────────
    if (traceUI->cubeMap()) {
      colorC = traceUI->getCubeMap()->getColor(r);
    } else {
      colorC = glm::dvec3(0.0, 0.0, 0.0);
    }
  }

#if VERBOSE
  std::cerr << "== depth: " << depth + 1 << " done, returning: " << colorC
            << std::endl;
#endif
  return colorC;
}

RayTracer::RayTracer()
    : scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0),
      m_bBufferReady(false) {}

RayTracer::~RayTracer() {}

void RayTracer::getBuffer(unsigned char *&buf, int &w, int &h) {
  buf = buffer.data();
  w = buffer_width;
  h = buffer_height;
}

double RayTracer::aspectRatio() {
  return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char *fn) {
  ifstream ifs(fn);
  if (!ifs) {
    string msg("Error: couldn't read scene file ");
    msg.append(fn);
    traceUI->alert(msg);
    return false;
  }

  bool isRay = false;
  const char *ext = strrchr(fn, '.');
  if (ext && !strcmp(ext, ".ray"))
    isRay = true;

  string path(fn);
  if (path.find_last_of("\\/") == string::npos)
    path = ".";
  else
    path = path.substr(0, path.find_last_of("\\/"));

  if (isRay) {
    Tokenizer tokenizer(ifs, false);
    Parser parser(tokenizer, path);
    try {
      scene.reset(parser.parseScene());
    } catch (SyntaxErrorException &pe) {
      traceUI->alert(pe.formattedMessage());
      return false;
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (TextureMapException e) {
      string msg("Texture mapping exception: ");
      msg.append(e.message());
      traceUI->alert(msg);
      return false;
    }
  } else {
    try {
      JsonParser parser(path, ifs);
      scene.reset(parser.parseScene());
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (const json::exception &je) {
      string msg("Invalid JSON encountered ");
      msg.append(je.what());
      traceUI->alert(msg);
      return false;
    }
  }

  if (!sceneLoaded())
    return false;

  return true;
}

void RayTracer::traceSetup(int w, int h) {
  size_t newBufferSize = w * h * 3;
  if (newBufferSize != buffer.size()) {
    bufferSize = newBufferSize;
    buffer.resize(bufferSize);
  }
  buffer_width  = w;
  buffer_height = h;
  std::fill(buffer.begin(), buffer.end(), 0);
  m_bBufferReady = true;

  threads    = traceUI->getThreads();
  block_size = traceUI->getBlockSize();
  thresh     = traceUI->getThreshold();
  samples    = traceUI->getSuperSamples();
  aaThresh   = traceUI->getAaThreshold();
}

void RayTracer::traceImage(int w, int h) {
  traceSetup(w, h);
  for (int i = 0; i < w; ++i)
    for (int j = 0; j < h; ++j)
      tracePixel(i, j);
}

int  RayTracer::aaImage()    { return 0; }
bool RayTracer::checkRender(){ return true; }
void RayTracer::waitRender() {}

glm::dvec3 RayTracer::getPixel(int i, int j) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  return glm::dvec3((double)pixel[0] / 255.0,
                    (double)pixel[1] / 255.0,
                    (double)pixel[2] / 255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  pixel[0] = (int)(255.0 * color[0]);
  pixel[1] = (int)(255.0 * color[1]);
  pixel[2] = (int)(255.0 * color[2]);
}