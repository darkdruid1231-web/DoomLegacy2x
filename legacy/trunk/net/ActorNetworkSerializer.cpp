#include "ActorNetworkSerializer.h"
#include "g_actor.h"
#include "doomdef.h"

namespace DoomLegacy::Network
{

#define ACTOR_AR 8 // angle resolution for netcode (in bits)

void ActorNetworkSerializer::Serialize(void* obj, DoomLegacy::ISerializer& s, uint32_t mask)
{
    Actor* actor = static_cast<Actor*>(obj);

    if (s.isWriting())
    {
        // Writing: pack data
        if (mask & Actor::M_MOVE)
        {
            SerializeMove(actor, s);
        }

        if (mask & Actor::M_PRES)
        {
            SerializePresentation(actor, s);
            mask &= ~Actor::M_ANIM; // already included
        }

        if (mask & Actor::M_ANIM)
        {
            SerializeAnimation(actor, s);
        }

        // TODO: floorclip, team
    }
    else
    {
        // Reading: unpack data
        if (mask & Actor::M_MOVE)
        {
            DeserializeMove(actor, s);
        }

        if (mask & Actor::M_PRES)
        {
            DeserializePresentation(actor, s);
        }

        if (mask & Actor::M_ANIM)
        {
            DeserializeAnimation(actor, s);
        }
    }
}

void ActorNetworkSerializer::Deserialize(void* obj, DoomLegacy::ISerializer& s, uint32_t mask)
{
    // For symmetry, but since ISerializer has isWriting(), we handle in Serialize
    Serialize(obj, s, mask);
}

void ActorNetworkSerializer::SerializeMove(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Position and velocity
    actor->pos.Pack(s);
    actor->vel.Pack(s);

    // Yaw and pitch with reduced resolution
    s.write(static_cast<uint32_t>(actor->yaw >> (32 - ACTOR_AR)));
    s.write(static_cast<uint32_t>(actor->pitch >> (32 - ACTOR_AR)));
}

void ActorNetworkSerializer::SerializePresentation(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Presentation data (model/sprite info, color, drawing flags)
    if (actor->pres)
    {
        actor->pres->Pack(s);
    }
}

void ActorNetworkSerializer::SerializeAnimation(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Animation sequence and interpolation/state
    if (actor->pres)
    {
        actor->pres->PackAnim(s);
    }
}

void ActorNetworkSerializer::DeserializeMove(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Movement data - unpacked to apos/avel for interpolation
    actor->apos.Unpack(s);
    actor->avel.Unpack(s);

    // Yaw and pitch - shift back to full angle
    actor->yaw = static_cast<angle_t>(s.readUInt32() << (32 - ACTOR_AR));
    actor->pitch = static_cast<angle_t>(s.readUInt32() << (32 - ACTOR_AR));
}

void ActorNetworkSerializer::DeserializePresentation(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Get model/sprite info
    if (actor->pres)
        delete actor->pres;

    // TODO: This needs to be abstracted - presentation creation
    // For now, assume spritepres_t
    actor->pres = new spritepres_t(s);
}

void ActorNetworkSerializer::DeserializeAnimation(Actor* actor, DoomLegacy::ISerializer& s)
{
    // Get animation sequence
    if (actor->pres)
    {
        actor->pres->UnpackAnim(s);
    }
}

} // namespace DoomLegacy::Network