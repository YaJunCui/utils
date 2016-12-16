// noncopyable.hpp header file  Edit by cyj 2016.12.6

// Private copy constructor and copy assignment ensure classes derived from
// class noncopyable cannot be copied.
// use a C++11 compiler

#ifndef __CYJ_NONCOPYABLE_HPP__
#define __CYJ_NONCOPYABLE_HPP__

namespace cyj
{

class noncopyable
{
protected:
  noncopyable() = default;
  ~noncopyable() = default;
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
};

} // namespace cyj

#endif // __CYJ_NONCOPYABLE_HPP__
