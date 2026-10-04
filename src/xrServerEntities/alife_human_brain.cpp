////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_brain.cpp
//	Created 	: 06.10.2005
//  Modified 	: 06.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human brain class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_human_brain.h"
#include "Common/object_broker.h"
#include "xrServer_Objects_ALife_Monsters.h"

#include "alife_human_object_handler.h"
#include "alife_monster_movement_manager.h"
#include "alife_monster_detail_path_manager.h"
#include "alife_monster_patrol_path_manager.h"
#include "ai_space.h"
#include "ef_storage.h"
#include "ef_primary.h"
#include "alife_simulator.h"
#include "alife_graph_registry.h"
#include "movement_manager_space.h"
#include "alife_smart_terrain_registry.h"
#include "alife_time_manager.h"
#include "date_time.h"
#ifdef DEBUG
#include "Level.h"
#include "map_location.h"
#include "map_manager.h"
#endif

CALifeHumanBrain::CALifeHumanBrain(object_type* object) : inherited(object)
{
    VERIFY(object);
    m_object = object;

    m_object_handler = xr_new<CALifeHumanObjectHandler>(object);

    m_dwTotalMoney = 0;
    m_cpEquipmentPreferences.resize(5);
    m_cpMainWeaponPreferences.resize(4);

    m_cpEquipmentPreferences.resize(iFloor(ai().ef_storage().m_pfEquipmentType->ffGetMaxResultValue() + .5f));
    m_cpMainWeaponPreferences.resize(iFloor(ai().ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f));
    R_ASSERT2((iFloor(ai().ef_storage().m_pfEquipmentType->ffGetMaxResultValue() + .5f) == 5) &&
            (iFloor(ai().ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f) == 4),
        "Recompile Level Editor and xrAI and rebuild file \"game.spawn\"!");

    for (int i = 0, n = m_cpEquipmentPreferences.size(); i < n; ++i)
        m_cpEquipmentPreferences[i] = u8(::Random.randI(3));

    for (int i = 0, n = m_cpMainWeaponPreferences.size(); i < n; ++i)
        m_cpMainWeaponPreferences[i] = u8(::Random.randI(3));
}

CALifeHumanBrain::~CALifeHumanBrain()
{
    xr_delete(m_object_handler);
}

bool CALifeHumanBrain::perform_attack()
{
    return (m_object->bfPerformAttack());
}

// Stage 1.3: human meet-action logic, restored from the 2003 code
// (CSE_ALifeHumanAbstract::tfGetActionType, alife_human_brain_save.h L48-62).
// A human meeting a friend in a monster-vs-monster combat type gets Interact
// (the 2003 human/monster difference vs CALifeMonsterBrain::action_type);
// otherwise the 2003 human logic is: mutual detection or an Attack combat
// action means Attack, else Ignore. Other combat types always mean Attack.
ALife::EMeetActionType
CALifeHumanBrain::action_type(CSE_ALifeSchedulable *tpALifeSchedulable,
                              const int &iGroupIndex,
                              const bool &bMutualDetection) {
  if (ALife::eCombatTypeMonsterMonster == ai().alife().combat_type()) {
    CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
        smart_cast<CSE_ALifeMonsterAbstract *>(tpALifeSchedulable);
    R_ASSERT2(l_tpALifeMonsterAbstract, "Inconsistent meet action type");
    return (ALife::eRelationTypeFriend ==
                    ai().alife().relation_type(m_object,
                                               l_tpALifeMonsterAbstract)
                ? ALife::eMeetActionTypeInteract
                : ((bMutualDetection ||
                    const_cast<CALifeSimulator &>(ai().alife())
                            .choose_combat_action(iGroupIndex) ==
                        ALife::eCombatActionAttack)
                       ? ALife::eMeetActionTypeAttack
                       : ALife::eMeetActionTypeIgnore));
  } else
    return (ALife::eMeetActionTypeAttack);
}

void CALifeHumanBrain::on_state_write(NET_Packet& packet)
{
    if (packet.inistream == nullptr)
    {
        save_data(m_cpEquipmentPreferences, packet);
        save_data(m_cpMainWeaponPreferences, packet);
    }
}

void CALifeHumanBrain::on_state_read(NET_Packet& packet)
{
    if (object().m_wVersion <= 19)
        return;

    if (object().m_wVersion < 110)
    {
        {
            xr_vector<u32> temp;
            load_data(temp, packet);
        }
        {
            xr_vector<bool> temp;
            load_data(temp, packet);
        }
    }

    if (object().m_wVersion <= 35)
        return;

    if (object().m_wVersion < 110)
    {
        shared_str temp;
        packet.r_stringZ(temp);
    }

    if (object().m_wVersion < 118)
    {
        ALife::OBJECT_VECTOR temp;
        load_data(temp, packet);
    }

    if (packet.inistream == nullptr)
    {
        load_data(m_cpEquipmentPreferences, packet);
        load_data(m_cpMainWeaponPreferences, packet);
    }
}
