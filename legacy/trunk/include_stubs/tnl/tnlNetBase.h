#ifndef TNL_NET_BASE_H
#define TNL_NET_BASE_H

// Basic types
typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned int U32;
typedef signed int S32;
typedef float F32;

// Termination reasons
enum TerminationReason
{
    ReasonSelfDisconnect,
    ReasonRemoteDisconnect,
    ReasonTimeout,
    ReasonFailedConnect,
    ReasonError
};

// Macros
#define BIT(x) (1 << (x))
#define FirstValidInfoPacketId 1

// Class implementation macros - defined as no-ops
#define TNL_IMPLEMENT_CLASS(className)
#define TNL_IMPLEMENT_NETCONNECTION(className, group, isGhosting)

// RPC macros - defined as no-ops
#define TNL_DECLARE_RPC(rpcName, args) // Stub - no-op
#define TNL_RPC_CONSTRUCT_NETEVENT(connection, rpcName, args) nullptr

// Forward declarations
class Address;
class Nonce;
class NetObject;

#endif // TNL_NET_BASE_H