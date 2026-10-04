////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_brain.cpp
//	Created 	: 06.10.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster brain class
////////////////////////////////////////////////////////////////////////////

#include "alife_monster_brain.h"
#include "Common/object_broker.h"
#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"

#include "ai_space.h"
#include "alife_graph_registry.h"
#include "alife_monster_detail_path_manager.h"
#include "alife_monster_movement_manager.h"
#include "alife_monster_patrol_path_manager.h"
#include "alife_simulator.h"
#include "alife_smart_terrain_registry.h"
#include "alife_time_manager.h"
#include "date_time.h"
#include "ef_primary.h"
#include "ef_storage.h"
#include "movement_manager_space.h"

#ifdef DEBUG
#include "Level.h"
#include "map_location.h"
#include "map_manager.h"
#endif

CALifeMonsterBrain::CALifeMonsterBrain(object_type *object) {
  VERIFY(object);
  m_object = object;
  m_last_search_time = 0;
  m_smart_terrain = nullptr;

  m_movement_manager = xr_new<CALifeMonsterMovementManager>(object);

  u32 hours, minutes, seconds;
  sscanf(pSettings->r_string(this->object().name(),
                             "smart_terrain_choose_interval"),
         "%d:%d:%d", &hours, &minutes, &seconds);
  m_time_interval = (u32)generate_time(1, 1, 1, hours, minutes, seconds);

  m_can_choose_alife_tasks = true;
}

CALifeMonsterBrain::~CALifeMonsterBrain() { xr_delete(m_movement_manager); }

void CALifeMonsterBrain::on_state_write(NET_Packet & /*packet*/) {}
void CALifeMonsterBrain::on_state_read(NET_Packet & /*packet*/) {}

bool CALifeMonsterBrain::perform_attack() { return (true); }

// Copy of CSE_ALifeMonsterAbstract::tfGetActionType
// (alife_monster_abstract.cpp)
// - monsters fight as in the 2003 code.
ALife::EMeetActionType
CALifeMonsterBrain::action_type(CSE_ALifeSchedulable *tpALifeSchedulable,
                                const int &iGroupIndex,
                                const bool &bMutualDetection) {
  if (ALife::eCombatTypeMonsterMonster == ai().alife().combat_type()) {
    CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
        smart_cast<CSE_ALifeMonsterAbstract *>(tpALifeSchedulable);
    R_ASSERT2(l_tpALifeMonsterAbstract, "Inconsistent meet action type");
    return (ALife::eRelationTypeFriend ==
                    ai().alife().relation_type(
                        smart_cast<CSE_ALifeMonsterAbstract *>(
                            this->object().base()),
                        l_tpALifeMonsterAbstract)
                ? ALife::eMeetActionTypeIgnore
                : ((bMutualDetection ||
                    const_cast<CALifeSimulator &>(ai().alife())
                            .choose_combat_action(iGroupIndex) ==
                        ALife::eCombatActionAttack)
                       ? ALife::eMeetActionTypeAttack
                       : ALife::eMeetActionTypeIgnore));
  } else if (ALife::eCombatTypeSmartTerrain == ai().alife().combat_type()) {
    CSE_ALifeSmartZone *smart_zone =
        smart_cast<CSE_ALifeSmartZone *>(tpALifeSchedulable);
    VERIFY(smart_zone);
    return (smart_zone->tfGetActionType(this->object().cast_schedulable(),
                                        iGroupIndex ? 0 : 1, bMutualDetection));
  } else
    return (ALife::eMeetActionTypeAttack);
}

void CALifeMonsterBrain::on_register() {}
void CALifeMonsterBrain::on_unregister() {}
void CALifeMonsterBrain::on_location_change() {}
CSE_ALifeSmartZone &CALifeMonsterBrain::smart_terrain() {
  VERIFY(object().m_smart_terrain_id != 0xffff);
  if (m_smart_terrain && (object().m_smart_terrain_id == m_smart_terrain->ID))
    return (*m_smart_terrain);

  m_smart_terrain =
      ai().alife().smart_terrains().object(object().m_smart_terrain_id);
  VERIFY(m_smart_terrain);
  return (*m_smart_terrain);
}

void CALifeMonsterBrain::process_task() {
  CALifeSmartTerrainTask *task = smart_terrain().task(&object());
  THROW3(task, "smart terrain returned nil task, while npc is registered in it",
         smart_terrain().name_replace());
  movement().path_type(MovementManager::ePathTypeGamePath);
  movement().detail().target(*task);
}

void CALifeMonsterBrain::select_task(const bool forced) {
  if (object().m_smart_terrain_id != 0xffff)
    return;

  if (!can_choose_alife_tasks())
    return;

  ALife::_TIME_ID current_time = ai().alife().time_manager().game_time();

  if (!forced && m_last_search_time + m_time_interval > current_time)
    return;

  m_last_search_time = current_time;

  float best_value = flt_min;
  CALifeSmartTerrainRegistry::OBJECTS::const_iterator I =
      ai().alife().smart_terrains().objects().begin();
  CALifeSmartTerrainRegistry::OBJECTS::const_iterator E =
      ai().alife().smart_terrains().objects().end();
  for (; I != E; ++I) {
    if (!(*I).second->enabled(&object()))
      continue;

    float value = (*I).second->suitable(&object());
    if (value > best_value) {
      best_value = value;
      object().m_smart_terrain_id = (*I).second->ID;
    }
  }

  if (object().m_smart_terrain_id != 0xffff) {
    smart_terrain().register_npc(&object());
    m_last_search_time = 0;
  }
}

void CALifeMonsterBrain::update(const bool forced) {
#if 0 // def DEBUG
    if (!Level().MapManager().HasMapLocation("debug_stalker",object().ID)) {
        CMapLocation				*map_location =
            Level().MapManager().AddMapLocation(
                "debug_stalker",
                object().ID
            );

        map_location->SetHint		(object().name_replace());
    }
#endif

  select_task(forced);

  if (object().m_smart_terrain_id != 0xffff)
    process_task();
  else
    default_behaviour();

  movement().update();
}

void CALifeMonsterBrain::default_behaviour() {
  movement().path_type(MovementManager::ePathTypeNoPath);
}
void CALifeMonsterBrain::on_switch_online() { movement().on_switch_online(); }
void CALifeMonsterBrain::on_switch_offline() { movement().on_switch_offline(); }
