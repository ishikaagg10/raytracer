#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>
#include <string>
#include <vector>

class Scene;
class ray;
class isect;

using std::string;

class TextureMap {
public:
  TextureMap(string filename);
  glm::dvec3 getMappedValue(const glm::dvec2 &coord) const;
  glm::dvec3 getPixelAt(int x, int y) const;
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  ~TextureMap() {}

protected:
  int width;
  int height;
  std::vector<uint8_t> data;
};

class TextureMapException {
public:
  TextureMapException(string errorMsg) : _errorMsg(errorMsg) {}
  string message() { return _errorMsg; }
private:
  string _errorMsg;
};

class MaterialParameter {
public:
  explicit MaterialParameter(const glm::dvec3 &par)
      : _value(par), _textureMap(0) {}
  explicit MaterialParameter(const double par)
      : _value(par, par, par), _textureMap(0) {}
  explicit MaterialParameter(TextureMap *tex) : _textureMap(tex) {}
  MaterialParameter() : _value(0.0, 0.0, 0.0), _textureMap(0) {}

  MaterialParameter &operator*=(const MaterialParameter &rhs) {
    _value[0] *= rhs._value[0];
    _value[1] *= rhs._value[1];
    _value[2] *= rhs._value[2];
    return *this;
  }

  glm::dvec3 &operator*=(const glm::dvec3 &rhs) {
    _value[0] *= rhs[0];
    _value[1] *= rhs[1];
    _value[2] *= rhs[2];
    return _value;
  }

  glm::dvec3 &operator*=(const double rhs) {
    _value[0] *= rhs;
    _value[1] *= rhs;
    _value[2] *= rhs;
    return _value;
  }

  MaterialParameter &operator+=(const MaterialParameter &rhs) {
    _value += rhs._value;
    return *this;
  }

  void setValue(const glm::dvec3 &rhs) {
    _value = rhs;
    _textureMap = 0;
  }

  void setValue(const double rhs) {
    _value[0] = rhs;
    _value[1] = rhs;
    _value[2] = rhs;
    _textureMap = 0;
  }

  bool isZero() const { return glm::length(_value) == 0.0 && _textureMap == nullptr; }
  bool mapped() const { return _textureMap != 0; }

  glm::dvec3 &operator+=(const glm::dvec3 &rhs) {
    _value += rhs;
    return _value;
  }

  glm::dvec3 value(const isect &is) const;
  double intensityValue(const isect &is) const;

private:
  glm::dvec3 _value;
  TextureMap *_textureMap;
};

class Material {
public:
  Material()
      : _ke(glm::dvec3(0.0, 0.0, 0.0)), _ka(glm::dvec3(0.0, 0.0, 0.0)),
        _ks(glm::dvec3(0.0, 0.0, 0.0)), _kd(glm::dvec3(0.0, 0.0, 0.0)),
        _kr(glm::dvec3(0.0, 0.0, 0.0)), _kt(glm::dvec3(0.0, 0.0, 0.0)),
        _kn(glm::dvec3(0.5, 0.5, 1.0)),
        _refl(0), _trans(0), _recur(0), _spec(0), _both(0), _shininess(0.0),
        _index(1.0) {}

  virtual ~Material();

  Material(const glm::dvec3 &e, const glm::dvec3 &a, const glm::dvec3 &s,
           const glm::dvec3 &d, const glm::dvec3 &r, const glm::dvec3 &t,
           double sh, double in)
      : _ke(e), _ka(a), _ks(s), _kd(d), _kr(r), _kt(t), _kn(glm::dvec3(0.5, 0.5, 1.0)),
        _shininess(glm::dvec3(sh, sh, sh)), _index(glm::dvec3(in, in, in)) {
    setBools();
  }

  virtual glm::dvec3 shade(Scene *scene, const ray &r, const isect &i) const;

  Material &operator+=(const Material &m) {
    _ke += m._ke;
    _ka += m._ka;
    _ks += m._ks;
    _kd += m._kd;
    _kr += m._kr;
    _kt += m._kt;
    _index += m._index;
    _shininess += m._shininess;
    return *this;
  }

  friend Material operator*(double d, Material m);

  glm::dvec3 ke(const isect &i) const { return _ke.value(i); }
  glm::dvec3 ka(const isect &i) const { return _ka.value(i); }
  glm::dvec3 ks(const isect &i) const { return _ks.value(i); }
  glm::dvec3 kd(const isect &i) const { return _kd.value(i); }
  glm::dvec3 kr(const isect &i) const { return _kr.value(i); }
  glm::dvec3 kt(const isect &i) const { return _kt.value(i); }
  glm::dvec3 kn(const isect &i) const { return _kn.value(i); }

  double shininess(const isect &i) const {
    return _shininess.mapped() ? 128.0 * _shininess.intensityValue(i) : _shininess.intensityValue(i);
  }
  double index(const isect &i) const { return _index.intensityValue(i); }

  void setEmissive(const glm::dvec3 &ke) { _ke.setValue(ke); }
  void setAmbient(const glm::dvec3 &ka) { _ka.setValue(ka); }
  void setSpecular(const glm::dvec3 &ks) { _ks.setValue(ks); setBools(); }
  void setDiffuse(const glm::dvec3 &kd) { _kd.setValue(kd); }
  void setReflective(const glm::dvec3 &kr) { _kr.setValue(kr); setBools(); }
  void setTransmissive(const glm::dvec3 &kt) { _kt.setValue(kt); setBools(); }
  void setNormalMap(const glm::dvec3 &kn) { _kn.setValue(kn); }
  void setShininess(double shininess) { _shininess.setValue(shininess); }
  void setIndex(double index) { _index.setValue(index); }

  void setEmissive(const MaterialParameter &ke) { _ke = ke; }
  void setAmbient(const MaterialParameter &ka) { _ka = ka; }
  void setSpecular(const MaterialParameter &ks) { _ks = ks; }
  void setDiffuse(const MaterialParameter &kd) { _kd = kd; }
  void setReflective(const MaterialParameter &kr) { _kr = kr; setBools(); }
  void setTransmissive(const MaterialParameter &kt) { _kt = kt; setBools(); }
  void setNormalMap(const MaterialParameter &kn) { _kn = kn; }
  void setShininess(const MaterialParameter &shininess) { _shininess = shininess; }
  void setIndex(const MaterialParameter &index) { _index = index; }

  bool hasNormalMap() const { return _kn.mapped(); }
  bool Refl() const { return _refl; }
  bool Trans() const { return _trans; }
  bool Recur() const { return _recur; }
  bool Spec() const { return _spec; }
  bool Both() const { return _both; }

private:
  MaterialParameter _ke;
  MaterialParameter _ka;
  MaterialParameter _ks;
  MaterialParameter _kd;
  MaterialParameter _kr;
  MaterialParameter _kt;
  MaterialParameter _kn; // Normal Map

  bool _refl;
  bool _trans;
  bool _recur;
  bool _spec;
  bool _both;

  MaterialParameter _shininess;
  MaterialParameter _index;

  void setBools() {
    _refl = !_kr.isZero();
    _trans = !_kt.isZero();
    _recur = _refl || _trans;
    _spec = _refl || !_ks.isZero();
    _both = _refl && _trans;
  }
};

inline Material operator*(double d, Material m) {
  m._ke *= d; m._ka *= d; m._ks *= d; m._kd *= d;
  m._kr *= d; m._kt *= d; m._index *= d; m._shininess *= d;
  return m;
}

#endif