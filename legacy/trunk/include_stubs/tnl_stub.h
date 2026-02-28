/// \file
/// \brief TNL stub header for builds without TNL library
///
/// This provides stub implementations of TNL classes to allow compilation
/// when the TNL library is not available. The stubs are no-ops that allow
/// the code to compile but networking won't work.

#ifndef TNL_STUB_H
#define TNL_STUB_H 1

// Guard to ensure this is only included when TNL is not available
#ifdef TNL_STUB_BUILD

// Include the stub headers
#include "tnl/tnlNetBase.h"
#include "tnl/tnlBitStream.h"
#include "tnl/tnlNetInterface.h"
#include "tnl/tnlGhostConnection.h"
#include "tnl/tnlNetObject.h"
#include "tnl/tnlAsymmetricKey.h"
#include "tnl/tnlAssert.h"

#endif // TNL_STUB_BUILD

#endif // TNL_STUB_H