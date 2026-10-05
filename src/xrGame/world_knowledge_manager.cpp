////////////////////////////////////////////////////////////////////////////
//	Module 		: world_knowledge_manager.cpp
//	Created 	: 05.10.2026
//	Description : V0.1: persistent "world knowledge" of a stalker
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "world_knowledge_manager.h"
#include "memory_space.h"
#include "CustomMonster.h"
#include "ai/stalker/ai_stalker.h"
#include "relation_registry.h"
#include "xrEngine/IGame_Level.h"

CWorldKnowledgeManager::CWorldKnowledgeManager(const CCustomMonster* object, CAI_Stalker* stalker)
    : m_object(object), m_stalker(stalker), m_ttl_ms(6 * 60 * 60 * 1000), m_max_points(64)
{
    VERIFY(object);
}

void CWorldKnowledgeManager::reload(LPCSTR section)
{
    pSettings->read_if_exists(m_ttl_ms, "memory", "world_knowledge_ttl");
    pSettings->read_if_exists(m_max_points, "memory", "world_knowledge_max_points");
    if (!m_ttl_ms)
        m_ttl_ms = 6 * 60 * 60 * 1000;
    if (!m_max_points)
        m_max_points = 64;
}

void CWorldKnowledgeManager::reinit()
{
    // intentionally empty: knowledge must survive net_Spawn
}

void CWorldKnowledgeManager::update()
{
    if (m_points.empty())
        return;

    u32 now = Device.dwTimeGlobal;
    u32 expire_line = now > m_ttl_ms ? now - m_ttl_ms : 0;
    m_points.erase(
        std::remove_if(m_points.begin(), m_points.end(),
            [expire_line](const SWorldKnowledgePoint& p) { return (p.last_seen_time < expire_line); }),
        m_points.end());
}

void CWorldKnowledgeManager::add_creature_seen(const CEntityAlive* creature, const Fvector& position,
    u32 level_vertex_id)
{
    VERIFY(creature);
    if (creature == m_object)
        return;
    if (!creature->g_Alive())
        return;

    u32 now = Device.dwTimeGlobal;

    // relation at the moment of seeing; if the creature is an
    // InventoryOwner (stalker/trader) the registry knows it precisely
    ALife::ERelationType relation_to_me = ALife::eRelationTypeNeutral;
    ALife::ERelationType relation_for_me = ALife::eRelationTypeNeutral;
    const CInventoryOwner* other_io = smart_cast<const CInventoryOwner*>(creature);
    const CInventoryOwner* my_io = smart_cast<const CInventoryOwner*>(m_object);
    if (other_io && my_io)
    {
        relation_to_me = RELATION_REGISTRY().GetRelationType(other_io, my_io);
        relation_for_me = RELATION_REGISTRY().GetRelationType(my_io, other_io);
    }
    else
    {
        ALife::ERelationType rel = m_object->tfGetRelationType(creature);
        relation_for_me = rel;
        relation_to_me = rel;
    }

    for (SWorldKnowledgePoint& point : m_points)
    {
        if (point.creature_object_id == creature->ID())
        {
            point.position = position;
            point.level_vertex_id = level_vertex_id;
            point.relation_to_me = relation_to_me;
            point.relation_for_me = relation_for_me;
            point.last_seen_time = now;
            point.seen_level_time = Level().GetGameTime();
            return;
        }
    }

    SWorldKnowledgePoint point;
    point.position = position;
    point.level_vertex_id = level_vertex_id;
    point.creature_object_id = creature->ID();
    point.relation_to_me = relation_to_me;
    point.relation_for_me = relation_for_me;
    point.last_seen_time = now;
    point.seen_level_time = Level().GetGameTime();
    m_points.push_back(point);

    if (m_points.size() > m_max_points)
        m_points.erase(m_points.begin());
}
