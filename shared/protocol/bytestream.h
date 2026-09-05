#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace bh::proto {

// Little-endian byte writer/reader. All wire encoding in BLOODHOLLOW goes
// through these two classes (generated serializers from messages.md).
class Writer {
 public:
  explicit Writer(std::vector<std::uint8_t>& out) : b_(out) {}

  void u8(std::uint8_t v) { b_.push_back(v); }
  void u16(std::uint16_t v) {
    b_.push_back(static_cast<std::uint8_t>(v));
    b_.push_back(static_cast<std::uint8_t>(v >> 8));
  }
  void u32(std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b_.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
  }
  void u64(std::uint64_t v) {
    for (int i = 0; i < 8; ++i) b_.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
  }
  void i8(std::int8_t v) { u8(static_cast<std::uint8_t>(v)); }
  void i16(std::int16_t v) { u16(static_cast<std::uint16_t>(v)); }
  void i32(std::int32_t v) { u32(static_cast<std::uint32_t>(v)); }
  void i64(std::int64_t v) { u64(static_cast<std::uint64_t>(v)); }
  void f32(float v) {
    std::uint32_t u;
    static_assert(sizeof(u) == sizeof(v));
    std::memcpy(&u, &v, 4);
    u32(u);
  }
  void boolean(bool v) { u8(v ? 1 : 0); }
  void str(const std::string& s) {
    const auto len = s.size() > 65535 ? 65535 : s.size();
    u16(static_cast<std::uint16_t>(len));
    b_.insert(b_.end(), s.data(), s.data() + len);
  }

  size_t size() const { return b_.size(); }
  std::uint8_t* raw() { return b_.data(); }

 private:
  std::vector<std::uint8_t>& b_;
};

class Reader {
 public:
  Reader(const std::uint8_t* p, size_t n) : p_(p), n_(n) {}

  bool u8(std::uint8_t& v) {
    if (at_ + 1 > n_) return false;
    v = p_[at_++];
    return true;
  }
  bool u16(std::uint16_t& v) {
    std::uint8_t a, b;
    if (!u8(a) || !u8(b)) return false;
    v = static_cast<std::uint16_t>(a | (static_cast<std::uint16_t>(b) << 8));
    return true;
  }
  bool u32(std::uint32_t& v) {
    std::uint32_t r = 0;
    for (int i = 0; i < 4; ++i) {
      std::uint8_t b;
      if (!u8(b)) return false;
      r |= static_cast<std::uint32_t>(b) << (8 * i);
    }
    v = r;
    return true;
  }
  bool u64(std::uint64_t& v) {
    std::uint64_t r = 0;
    for (int i = 0; i < 8; ++i) {
      std::uint8_t b;
      if (!u8(b)) return false;
      r |= static_cast<std::uint64_t>(b) << (8 * i);
    }
    v = r;
    return true;
  }
  bool i8(std::int8_t& v) {
    std::uint8_t u;
    if (!u8(u)) return false;
    v = static_cast<std::int8_t>(u);
    return true;
  }
  bool i16(std::int16_t& v) {
    std::uint16_t u;
    if (!u16(u)) return false;
    v = static_cast<std::int16_t>(u);
    return true;
  }
  bool i32(std::int32_t& v) {
    std::uint32_t u;
    if (!u32(u)) return false;
    v = static_cast<std::int32_t>(u);
    return true;
  }
  bool i64(std::int64_t& v) {
    std::uint64_t u;
    if (!u64(u)) return false;
    v = static_cast<std::int64_t>(u);
    return true;
  }
  bool f32(float& v) {
    std::uint32_t u;
    if (!u32(u)) return false;
    std::memcpy(&v, &u, 4);
    return true;
  }
  bool boolean(bool& v) {
    std::uint8_t u;
    if (!u8(u)) return false;
    v = u != 0;
    return true;
  }
  bool str(std::string& v, size_t cap = 4096) {
    std::uint16_t len16;
    if (!u16(len16)) return false;
    if (len16 > cap || at_ + len16 > n_) return false;
    v.assign(reinterpret_cast<const char*>(p_ + at_), len16);
    at_ += len16;
    return true;
  }

  size_t remaining() const { return n_ - at_; }

 private:
  const std::uint8_t* p_;
  size_t n_;
  size_t at_ = 0;
};

}  // namespace bh::proto
