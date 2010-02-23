#ifndef HANDLE_HPP
#define HANDLE_HPP

// Ripped from Noel Llopis article at http://www.gamasutra.com/php-bin/article_display.php?story=4015

#include <stdint.h>

namespace HandleType {
  enum Enum {
    InvalidHandle,
    Effect,
    Buffer,
    InputLayout,
  };
}

struct Effect;
struct Buffer;
struct InputLayout;
template<class T> struct handle_type_from_type {};
template<> struct handle_type_from_type<Effect> { enum { type = HandleType::Effect }; };
template<> struct handle_type_from_type<Buffer> { enum { type = HandleType::Buffer }; };
template<> struct handle_type_from_type<InputLayout> { enum { type = HandleType::InputLayout }; };



class HandleManager;
class Handle
{
public:
  Handle() : index_(0), counter_(0), type_(HandleType::InvalidHandle) 
  {}

  Handle(const uint32_t index, const uint32_t counter, const HandleType::Enum type)
    : index_(index), counter_(counter), type_(type)
  {}

  inline operator uint32_t() const;
  inline bool is_valid() const;

private:
  friend class HandleManager;
  uint32_t index_ : 12;
  uint32_t counter_ : 15;
  uint32_t type_ : 5;
};

Handle::operator uint32_t() const {
  return type_ << 27 | counter_ << 12 | index_;
}

bool Handle::is_valid() const {
  return type_ != HandleType::InvalidHandle;
}

#endif // #ifndef HANDLE_HPP
