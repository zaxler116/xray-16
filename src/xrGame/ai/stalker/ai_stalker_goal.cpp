////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_goal.cpp
//	Created 	: 06.10.2026
//	Description : "Life goal" of a human NPC (stalker, not a monster)
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai_stalker.h"
#include "alife_space.h"
#include "character_info_defs.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"

// NOTE: enum and class are defined inline here (not in a separate header)
// to work around an unresolved MSBuild issue where ai_stalker_goal.h
// contents were not visible to the compiler.

class CAI_Stalker;
class CSE_ALifeHumanAbstract;
class IReader;
class IWriter;
struct HLOG;
class shared_str;

enum ENpcLifeGoal : u8 {
  eGoalKillNpcAndLeave = 0,
  eGoalEarnMoney = 1,
  eGoalExploreZone = 2,
  eGoalCollectArtefacts = 3,
  eGoalJoinFaction = 4,
  eGoalSquadExplore = 5,
  eGoalCount
};

// returns a short printable name for logs / debug / console
LPCSTR npc_life_goal_name(u8 goal_type);

class CNpcLifeGoal {
private:
  u8 m_type; // eGoalCount == "no goal yet"

  // eGoalKillNpcAndLeave
  ALife::_OBJECT_ID m_target_npc; // who to hunt
  bool m_target_killed;

  // eGoalEarnMoney
  u32 m_money_target;   // absolute money amount to hold
  u32 m_money_at_spawn; // capital at spawn (for "earned" reports)

  // eGoalJoinFaction
  s32 m_target_faction; // CHARACTER_COMMUNITY_INDEX

  // eGoalSquadExplore
  u32 m_squad_target; // desired squad size

  // current sub-goal destination (avoid re-picking every frame)
  ALife::_OBJECT_ID m_current_dest; // level vertex id of current destination
  Fvector m_current_dest_pos;       // world position of current destination

public:
  CNpcLifeGoal();

  // --- selection ---------------------------------------------------------
  // pick one random goal (weights from [life_goal]) and initialise its fields.
  // Must be called exactly once, on first entity creation.
  void pick_random(CSE_ALifeHumanAbstract *self);
  // force a goal type (console command / debugging); initialises fields too
  void force_type(CSE_ALifeHumanAbstract *self, u8 type);

  IC bool is_none() const { return (m_type == eGoalCount); }
  IC u8 type() const { return (m_type); }

  // --- per-frame update (called from CAI_Stalker::Think) ------------------
  void tick(CAI_Stalker *self);

  // --- per-goal state queries (used by tick / debug) ---------------------
  IC ALife::_OBJECT_ID target_npc() const { return (m_target_npc); }
  IC u32 money_target() const { return (m_money_target); }
  IC u32 squad_target() const { return (m_squad_target); }
  IC s32 target_faction() const { return (m_target_faction); }

  // --- serialization (survives level transitions + savegames) ------------
  void save(IWriter &F) const;
  void load(IReader &F);

  // --- debug -------------------------------------------------------------
  void debug_info(CSE_ALifeHumanAbstract *self, HLOG &log) const;

  // --- helpers (implemented in ai_stalker_goal.cpp) ----------------------
  // navigate the stalker to a vertex/position (mirrors
  // CStalkerActionSmartTerrain)
  void go_to(CAI_Stalker *ai, ALife::_OBJECT_ID game_vertex,
             ALife::_OBJECT_ID level_vertex, const Fvector &position);
  void set_destination(ALife::_OBJECT_ID level_vertex, const Fvector &position);
  IC bool at_destination(CAI_Stalker *ai, float radius) const;

  // switch to the "wander afterwards" behaviour (goal 2,3,4,5 completion)
  void complete_to_wander();
};

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
