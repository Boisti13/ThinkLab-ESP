// Minimal Arduino API for rendering the e-paper pages on a PC (tools/render).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef ARDUINO
#define ARDUINO 10819
#endif
#define PROGMEM
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))
class __FlashStringHelper;
using std::isnan;
typedef bool boolean;
typedef uint8_t byte;

uint32_t millis();
inline void delay(uint32_t) {}
inline float radians(float d) { return d * 0.017453292519943295f; }
inline float degrees(float r) { return r * 57.29577951308232f; }
template <class T> inline T constrain(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

class String {
public:
  String() {}
  String(const char *s) : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  String(const __FlashStringHelper *s) : s_(reinterpret_cast<const char *>(s)) {}
  String(char c) : s_(1, c) {}
  String(int v) : s_(std::to_string(v)) {}
  String(unsigned v) : s_(std::to_string(v)) {}
  String(long v) : s_(std::to_string(v)) {}
  String(unsigned long v) : s_(std::to_string(v)) {}
  String(long long v) : s_(std::to_string(v)) {}
  String(unsigned long long v) : s_(std::to_string(v)) {}
  String(float v, unsigned dec = 2) : s_(fmt(v, dec)) {}
  String(double v, unsigned dec = 2) : s_(fmt(v, dec)) {}
  const char *c_str() const { return s_.c_str(); }
  unsigned length() const { return (unsigned)s_.size(); }
  int indexOf(char c, unsigned from = 0) const {
    auto p = s_.find(c, from);
    return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const String &t, unsigned from = 0) const {
    auto p = s_.find(t.s_, from);
    return p == std::string::npos ? -1 : (int)p;
  }
  String substring(unsigned a) const { return a >= s_.size() ? String() : String(s_.substr(a)); }
  String substring(unsigned a, unsigned b) const {
    if (a > b) std::swap(a, b);
    if (a >= s_.size()) return String();
    return String(s_.substr(a, b - a));
  }
  void reserve(unsigned n) { s_.reserve(n); }
  void remove(unsigned i) { if (i < s_.size()) s_.erase(i); }
  void remove(unsigned i, unsigned n) { if (i < s_.size()) s_.erase(i, n); }
  char operator[](unsigned i) const { return s_[i]; }
  char charAt(unsigned i) const { return s_[i]; }
  bool operator==(const String &o) const { return s_ == o.s_; }
  bool operator!=(const String &o) const { return s_ != o.s_; }
  String &operator+=(const String &o) { s_ += o.s_; return *this; }
  String &operator+=(const char *o) { s_ += o; return *this; }
  String &operator+=(char c) { s_ += c; return *this; }
  friend String operator+(const String &a, const String &b) { return String(a.s_ + b.s_); }
  friend String operator+(const String &a, const char *b) { return String(a.s_ + b); }
  friend String operator+(const char *a, const String &b) { return String(a + b.s_); }
  friend String operator+(const String &a, char b) { return String(a.s_ + b); }
  friend String operator+(const String &a, const __FlashStringHelper *b) {
    return String(a.s_ + reinterpret_cast<const char *>(b));
  }

private:
  static std::string fmt(double v, unsigned dec) {
    char b[48];
    snprintf(b, sizeof b, "%.*f", (int)dec, v);
    return b;
  }
  std::string s_;
};

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *b, size_t n) {
    size_t k = 0;
    while (n--) k += write(*b++);
    return k;
  }
  size_t write(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
  size_t print(const char *s) { return write(s); }
  size_t print(const String &s) { return write(s.c_str()); }
  size_t print(const __FlashStringHelper *s) { return write(reinterpret_cast<const char *>(s)); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) { return print(String(v)); }
  size_t print(unsigned v) { return print(String(v)); }
  size_t print(long v) { return print(String(v)); }
  size_t print(unsigned long v) { return print(String(v)); }
  size_t print(double v, int dec = 2) { return print(String(v, dec)); }
  size_t println(const String &s = String()) { return print(s) + print('\n'); }
};
