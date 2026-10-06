////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_goal.cpp
//	Created 	: 06.10.2026
//	Description : "Life goal" of a human NPC (stalker, not a monster)
////////////////////////////////////////////////////////////////////////////

#include "ai_stalker_goal.h"
#include "StdAfx.h"
#include "ai_stalker.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"


LPCSTR npc_life_goal_name(u8 goal_type) {
  switch (goal_type) {
  case eGoalKillNpcAndLeave:
    return ("kill_npc_and_leave");
  case eGoalEarnMoney:
    return ("earn_money");
  case eGoalExploreZone:
    return ("explore_zone");
  case eGoalCollectArtefacts:
    return ("collect_artefacts");
  case eGoalJoinFaction:
    return ("join_faction");
  case eGoalSquadExplore:
    return ("squad_explore");
  default:
    return ("none");
  }
}

CNpcLifeGoal::CNpcLifeGoal()
    : m_type(eGoalCount), m_target_npc(ALife::_OBJECT_ID(0xffff)),
      m_target_killed(false), m_money_target(0), m_money_at_spawn(0),
      m_target_faction(NO_COMMUNITY_INDEX), m_squad_target(0),
      m_current_dest(ALife::_OBJECT_ID(0xffff)), m_current_dest_pos() {}

// TODO: pick_random / force_type / tick / artefact_collected / save / load /
//       debug_info / complete_to_wander

///////////////////////////////////////////////////////////////////////////////
// Movement helpers
//
// go_to mirrors the navigation part of CStalkerActionSmartTerrain::execute:
// route on the game graph until we reach the destination's game vertex, then
// switch to a level path toward the exact position. The destination is kept
// in m_current_dest so we only re-issue the path when it actually changes.
///////////////////////////////////////////////////////////////////////////////

void CNpcLifeGoal::go_to(CAI_Stalker *ai, ALife::_OBJECT_ID game_vertex,
                         ALife::_OBJECT_ID level_vertex,
                         const Fvector &position) {
  if (m_current_dest == level_vertex && m_current_dest_pos == position)
    return;

  m_current_dest = level_vertex;
  m_current_dest_pos = position;

  // still in another area of the zone -> route through the game graph first
  if (ai->ai_location().game_vertex_id() != game_vertex) {
    ai->movement().set_path_type(MovementManager::ePathTypeGamePath);
    ai->movement().set_game_dest_vertex(game_vertex);
    return;
  }

  ai->movement().set_path_type(MovementManager::ePathTypeLevelPath);
  if (ai->movement().accessible(level_vertex)) {
    ai->movement().set_level_dest_vertex(level_vertex);
    ai->movement().set_desired_position(&m_current_dest_pos);
  } else {
    ai->movement().set_nearest_accessible_position(m_current_dest_pos,
                                                   level_vertex);
  }
}

void CNpcLifeGoal::set_destination(ALife::_OBJECT_ID level_vertex,
                                   const Fvector &position) {
  m_current_dest = level_vertex;
  m_current_dest_pos = position;
}

bool CNpcLifeGoal::at_destination(CAI_Stalker *ai, float radius) const {
  if (m_current_dest == ALife::_OBJECT_ID(0xffff))
    return (false);
  if (m_current_dest_pos.distance_to_sqr(ai->Position()) > radius * radius)
    return (false);
  return (ai->ai_location().level_vertex_id() == m_current_dest);
}
