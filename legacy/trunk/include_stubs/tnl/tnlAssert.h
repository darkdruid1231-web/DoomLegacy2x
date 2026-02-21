#ifndef TNL_ASSERT_H
#define TNL_ASSERT_H

#include <cstdio>

// Stub TNL Assert header
#define TNLAssert(expr, msg)
#define TNLAssertMacro(expr, msg)

namespace TNL {

// Basic types
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;
typedef float F32;
typedef double F64;

// BIT macro
#define BIT(n) (1 << (n))

// Stub assert handler
inline void assertHandler(const char* expr, const char* file, int line, const char* msg) {
    std::fprintf(stderr, "Assertion failed: %s at %s:%d\n", expr, file, line);
}

} // namespace TNL

#endif // TNL_ASSERT_H
