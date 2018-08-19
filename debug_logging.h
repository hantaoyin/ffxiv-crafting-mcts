// -*- mode:c++; c-basic-offset:2 -*-
// ==================================================================================================
#ifndef INCLUDE_GUARD_DEBUG_LOGGING_H__
#define INCLUDE_GUARD_DEBUG_LOGGING_H__

#include<iostream>

namespace check_impl {
struct Terminate {
  ~Terminate() {
    std::cerr << "\n";
    std::exit(-1);
  }
};

struct NewLine {
  ~NewLine() {
    std::cout << "\n";
  }
};
}  // namespace check

// This is meant to be a check always in place.
//
// Usage:
// Check if an integer a is even:
// CHECK(a % 2 == 0) << a;
//
// Obviously, a more useful implementation will also print a stack trace.
#define CHECK(condition) __builtin_expect((condition), 1) ? \
  std::cerr :                                               \
  (check_impl::Terminate(), std::cerr)                      \
  << "Condition `" #condition "` failed in "                \
  << __FILE__ << " line " << __LINE__ << ", msg = "

// This is meant to be a check only in place in debug mode.  Unlike CHECK(condition),
// ASSERT(condition) doesn't evaluate condition in NDEBUG mode.
#ifdef NDEBUG
#  define ASSERT(condition) true || std::cerr
#else
#  define ASSERT(condition) CHECK((condition))
#endif  // NDEBUG

// If condition is true, then LOG(condition) behaves like std::cout, otherwise does nothing and
// discards any following << ... construction without evaluating them.
//
// This is meant to reduce the frequency of logging messages, so condition is expected to be false
// most of the time.
#define LOG(condition) __builtin_expect(!(condition), 1) ? \
  std::cout : (check_impl::NewLine(), std::cout)

#endif  // #ifndef INCLUDE_GUARD_DEBUG_LOGGING_H__
