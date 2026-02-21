#ifndef TNL_NET_BASE_H
#define TNL_NET_BASE_H

#include <cstdint>

namespace TNL {

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef float F32;
typedef double F64;

const U32 NetClassRepInitialized = 0;
const U32 NetClassGroupMask = 0;

// BIT macro
#define BIT(n) (1 << (n))

class NetClassRep {
public:
    const char* className;
    U32 group;
    U32 bit;
    
    NetClassRep() : className(nullptr), group(0), bit(0) {}
};

} // namespace TNL

#endif // TNL_NET_BASE_H
