////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_goal.h
//	Created 	: 06.10.2026
//	Description : "Life goal" of a human NPC (stalker, not a monster).
//					On spawn the NPC picks ONE random goal
//(weighted, from 					[life_goal] config
// section) and pursues it for its whole life. The goal state lives on the
// server
// entity 					(CSE_ALifeHumanAbstract), so it
// survives level 					transitions and
// savegames, exactly like the persistent world knowledge
// (CWorldKnowledgeManager).
//
//					Goals:
//					 0 eGoalKillNpcAndLeave   - find & kill
// a specific NPC, then leave the Zone 					 1
// eGoalEarnMoney         - earn a certain amount of money (min from config)
// 2 eGoalExploreZone       - visit all smart terrains, then just wander
// 3 eGoalCollectArtefacts  - collect artefacts of all (configured) types
// 4 eGoalJoinFaction       - join a faction with relation >= 0
// 5 eGoalSquadExplore      - gather max squad size + explore the Zone
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "character_info_defs.h"

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

  // eGoalExploreZone / eGoalSquadExplore
  // visited smart terrains are kept on the server entity
  // (CSE_ALifeHumanAbstract) so they survive level transitions; not stored
  // here.

  // eGoalCollectArtefacts
  // (the collected-artefact list is added in the artefact-goal phase; it is
  //  not needed yet, and keeping this header free of xr_vector<shared_str> lets
  //  it compile under any precompiled header)

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
  // goal 1: pick a random hostile NPC to hunt down, then leave the Zone
  void pick_kill_leave(CSE_ALifeHumanAbstract *self);

  IC bool is_none() const { return (m_type == eGoalCount); }
  IC u8 type() const { return (m_type); }

  // --- per-frame update (called from CAI_Stalker::Think) ------------------
  void tick(CAI_Stalker *self);
  void tick_kill_leave(CAI_Stalker *ai);
  void leave_zone(CAI_Stalker *ai);

  // --- per-goal state queries (used by tick / debug) ---------------------
  IC ALife::_OBJECT_ID target_npc() const { return (m_target_npc); }
  IC u32 money_target() const { return (m_money_target); }
  IC u32 squad_target() const { return (m_squad_target); }
  IC s32 target_faction() const { return (m_target_faction); }

  // --- serialization (survives level transitions + savegames) ------------
  void save(IWriter &F) const;
  void load(IReader &F);
  void save_net(NET_Packet &P) const;
  void load_net(NET_Packet &P);

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
