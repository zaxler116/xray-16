////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_goal.cpp
//	Created 	: 06.10.2026
//	Description : "Life goal" of a human NPC (stalker, not a monster)
////////////////////////////////////////////////////////////////////////////

#include "ai_stalker_goal.h"
#include "StdAfx.h"
#include "ai_stalker.h"
#include "alife_object_registry.h"
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

// goal 1: pick a random hostile NPC to hunt down, then leave the Zone
void CNpcLifeGoal::pick_kill_leave(CSE_ALifeHumanAbstract *self) {
  // collect candidates
  ALife::_OBJECT_ID candidates[64];
  u32 count = 0;

  ALife::D_OBJECT_P_MAP::const_iterator I =
      ai().alife().objects().objects().begin();
  ALife::D_OBJECT_P_MAP::const_iterator E =
      ai().alife().objects().objects().end();
  for (; I != E; ++I) {
    CSE_ALifeHumanAbstract *h = smart_cast<CSE_ALifeHumanAbstract *>(I->second);
    if (!h)
      continue;
    if (h->ID == self->ID)
      continue; // not self
    if (!h->g_Alive())
      continue; // alive only
    if (h->Community() == self->Community())
      continue; // not own faction
    if (count < 64)
      candidates[count++] = I->first;
  }

  if (count == 0) {
    m_type = eGoalCount;
    Msg("NPC [%s] has no valid kill targets", self->name());
    return;
  }

  m_target_npc = candidates[::Random32.random(count)];
  m_target_killed = false;
  m_type = eGoalKillNpcAndLeave;

  // log
  CSE_ALifeDynamicObject *target = ai().alife().objects().object(m_target_npc);
  CSE_ALifeHumanAbstract *target_h =
      target ? smart_cast<CSE_ALifeHumanAbstract *>(target) : nullptr;
  LPCSTR target_name = target_h ? target_h->name() : "unknown";
  Msg("NPC [%s] goal: kill [%s] and leave", self->name(), target_name);
}

// pick one random goal (weights from [life_goal] in system.ltx)
void CNpcLifeGoal::pick_random(CSE_ALifeHumanAbstract *self) {
  if (!is_none())
    return;

  // read [life_goal] section from system.ltx (pSettings)
  u32 weights[eGoalCount];
  u32 total_weight = 0;
  for (u8 i = 0; i < eGoalCount; ++i)
    weights[i] = 0;

  if (pSettings && pSettings->section_exist("life_goal")) {
    u32 count = pSettings->line_count("life_goal");
    for (u32 i = 0; i < count; ++i) {
      LPCSTR name, value;
      if (!pSettings->r_line("life_goal", i, &name, &value))
        continue;
      // parse "goal_name=weight"
      u32 w = (u32)atol(value);
      for (u8 g = 0; g < eGoalCount; ++g) {
        if (xr_stricmp(name, npc_life_goal_name(g)) == 0) {
          weights[g] = w;
          total_weight += w;
          break;
        }
      }
    }
  }

  // fallback: only goal 1 if no config or zero weights
  if (total_weight == 0) {
    weights[eGoalKillNpcAndLeave] = 1;
    total_weight = 1;
  }

  // weighted random pick
  u32 roll = ::Random32.random(total_weight);
  u32 acc = 0;
  u8 chosen = eGoalKillNpcAndLeave;
  for (u8 i = 0; i < eGoalCount; ++i) {
    acc += weights[i];
    if (roll < acc) {
      chosen = i;
      break;
    }
  }

  switch (chosen) {
  case eGoalKillNpcAndLeave:
    pick_kill_leave(self);
    break;
  case eGoalEarnMoney:
  case eGoalExploreZone:
  case eGoalCollectArtefacts:
  case eGoalJoinFaction:
  case eGoalSquadExplore:
    // TODO: implement other goals; fallback to kill for now
    Msg("NPC [%s] goal %s not implemented yet, fallback to kill", self->name(),
        npc_life_goal_name(chosen));
    pick_kill_leave(self);
    break;
  default:
    m_type = eGoalCount;
    break;
  }
}

// per-frame update (called from CAI_Stalker::Think)
void CNpcLifeGoal::tick(CAI_Stalker *self) {
  if (is_none())
    return;

  if (type() == eGoalKillNpcAndLeave && !m_target_killed) {
    // check if target is still alive
    CSE_ALifeDynamicObject *target =
        ai().alife().objects().object(m_target_npc);
    CSE_ALifeHumanAbstract *target_h =
        target ? smart_cast<CSE_ALifeHumanAbstract *>(target) : nullptr;

    if (!target_h || !target_h->g_Alive()) {
      // target is dead or gone
      m_target_killed = true;
      LPCSTR target_name = target_h ? target_h->name() : "unknown";
      Msg("NPC [%s] goal: target [%s] is dead, leaving the Zone",
          self->cName(), target_name);
      // TODO: phase 2 - navigate to exit and leave the Zone
    } else {
      // target is alive, navigate to it
      Fvector target_pos = target_h->position();
      ALife::_OBJECT_ID target_game_vertex =
          ALife::_OBJECT_ID(target_h->m_tGraphID);
      // use level_vertex = 0 for now (will be refined in phase 2)
      go_to(self, target_game_vertex, ALife::_OBJECT_ID(0), target_pos);
    }
  }
  // TODO: other goal types
}

// TODO: force_type / artefact_collected / save / load /
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
