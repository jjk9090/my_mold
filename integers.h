// This file defines integral types for file input/output. We need to use
// these types instead of the plain integers (such as uint32_t or int32_t)
// when reading from/writing to an mmap'ed file area for the following
// reasons:
//
// 1. mold is always a cross linker and should not depend on what host it
//    is running on. Users should be able to run mold on a big-endian
//    SPARC machine to create a little-endian RV64 binary, for example.
//
// 2. Even though data members in all ELF data strucutres are naturally
//    aligned, they are not guaranteed to be aligned on memory. Because
//    archive file (.a file) aligns each member only to a 2 byte boundary,
//    anything larger than 2 bytes may be unaligned in an mmap'ed memory.
//    Unaligned access is an undefined behavior in C/C++, so we shouldn't
//    cast an arbitrary pointer to a uint32_t, for example, to read a
//    32-bits value.
//
// The data types defined in this file don't depend on host byte order and
// don't do unaligned access.
#ifndef INTEGER_H
#define INTEGER_H
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#endif
// namespace mold
