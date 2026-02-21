#ifndef TNL_GHOST_CONNECTION_H
#define TNL_GHOST_CONNECTION_H

#include "tnlNetInterface.h"

namespace TNL {

class GhostConnection : public NetConnection {
public:
    GhostConnection() {}
    virtual ~GhostConnection() {}
    
    // Ghost interface stubs
    void addGhost(NetObject* obj) {}
    void removeGhost(NetObject* obj) {}
    void updateGhost(NetObject* obj) {}
    
    virtual void onGhostAdded(NetObject* obj) {}
    virtual void onGhostRemoved(NetObject* obj) {}
    virtual void onGhostUpdate(NetObject* obj) {}
};

} // namespace TNL

#endif // TNL_GHOST_CONNECTION_H
