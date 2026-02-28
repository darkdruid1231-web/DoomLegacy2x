#ifndef TNL_GHOST_CONNECTION_H
#define TNL_GHOST_CONNECTION_H

#include "tnlNetInterface.h"

// Stub NetConnection
class NetConnection
{
  public:
    enum Reason
    {
        ReasonSelfDisconnect,
        ReasonRemoteDisconnect,
        ReasonTimeout,
        ReasonFailedConnect,
        ReasonError
    };

    NetConnection() {}
    virtual ~NetConnection() {}

    virtual void connect(NetInterface *net, const Address &address) {}
    virtual void disconnect(Reason reason, const char *reasonString = nullptr) {}

    U32 getConnectionState() const { return 0; } // Connected
};

// Stub GhostConnection
class GhostConnection : public NetConnection
{
  public:
    GhostConnection() {}
    virtual ~GhostConnection() {}

    virtual S32 getGhostIndex(void *obj) { return 0; }
    virtual void *resolveGhost(S32 index) { return nullptr; }

    void objectInScope(void *obj) {} // Stub
    void postNetEvent(void *event) {} // Stub
};

#endif // TNL_GHOST_CONNECTION_H