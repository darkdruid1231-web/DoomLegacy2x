#ifndef TNL_NET_OBJECT_H
#define TNL_NET_OBJECT_H

#include "tnlBitStream.h"

namespace TNL {

class NetInterface;

class NetObject {
public:
    NetObject() : mNetInterface(nullptr), mGhostIndex(0) {}
    virtual ~NetObject() {}
    
    virtual void onGhostAdded() {}
    virtual void onGhostRemoved() {}
    virtual void onGhostUpdate(BitStream* stream) {}
    virtual void onGhostReceive(BitStream* stream) {}
    
    virtual void packUpdate(BitStream* stream) {}
    virtual bool unpackUpdate(BitStream* stream) { return true; }
    
    NetInterface* getNetInterface() { return mNetInterface; }
    void setNetInterface(NetInterface* iface) { mNetInterface = iface; }
    
    int getGhostIndex() const { return mGhostIndex; }
    void setGhostIndex(int idx) { mGhostIndex = idx; }
    
private:
    NetInterface* mNetInterface;
    int mGhostIndex;
};

} // namespace TNL

#endif // TNL_NET_OBJECT_H
