#ifndef TNL_NET_OBJECT_H
#define TNL_NET_OBJECT_H

// Stub NetObject
class NetObject
{
  public:
    NetObject() {}
    virtual ~NetObject() {}

    virtual U32 packUpdate(GhostConnection *c, U32 mask, BitStream *stream) { return 0; }
    virtual void unpackUpdate(GhostConnection *c, BitStream *stream) {}
};

#endif // TNL_NET_OBJECT_H