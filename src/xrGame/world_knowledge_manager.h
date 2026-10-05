////////////////////////////////////////////////////////////////////////////
//	Module 		: world_knowledge_manager.h
//	Created 	: 05.10.2026
//	Description : V0.1: persistent "world knowledge" of a stalker: where
//					creatures were seen (position, level vertex, relation
//					at the moment of seeing). Unlike visual memory it is NOT
//					reinit-ed on net_Spawn and survives level transitions;
//					records expire by TTL (Device.dwTimeGlobal based).
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CCustomMonster;
class CAI_Stalker;
class CEntityAlive;

class CWorldKnowledgeManager
{
public:
    struct SWorldKnowledgePoint
    {
        Fvector position;                 // world position at last sight
        u32 level_vertex_id;              // vertex of the current level graph
        u16 creature_object_id;           // ALife object id of the creature
        ALife::ERelationType relation_to_me;   // how it related to me at sight
        ALife::ERelationType relation_for_me;  // how I related to it at sight
        u32 last_seen_time;               // Device.dwTimeGlobal at last sight
        u32 seen_level_time;              // level game time at last sight
    };

    typedef xr_vector<SWorldKnowledgePoint> POINTS;

private:
    POINTS m_points;
    const CCustomMonster* m_object;
    CAI_Stalker* m_stalker;

    u32 m_ttl_ms;        // knowledge lifetime, ms of Device.dwTimeGlobal
    u32 m_max_points;    // hard limit of stored points (oldest evicted)

public:
    CWorldKnowledgeManager(const CCustomMonster* object, CAI_Stalker* stalker);
    void reload(LPCSTR section);
    void reinit();   // NO-OP: knowledge must survive net_Spawn by design
    void update();   // drops expired points

    // called by CMemoryManager for every visible creature (not self, not squad)
    void add_creature_seen(const CEntityAlive* creature, const Fvector& position,
        u32 level_vertex_id);

    IC const POINTS& points() const { return (m_points); }
    void save(NET_Packet& packet) const;
    void load(IReader& packet);
    IC u32 ttl_ms() const { return (m_ttl_ms); }
    IC u32 max_points() const { return (m_max_points); }
    IC const CCustomMonster* object() const { return (m_object); }
};

