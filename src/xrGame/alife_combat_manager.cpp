////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_combat_manager.h
//	Created 	: 12.08.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife combat manager
////////////////////////////////////////////////////////////////////////////

#include "alife_combat_manager.h"
#include "StdAfx.h"
#include "ai_debug.h"
#include "alife_graph_registry.h"
#include "alife_schedule_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"

#include "alife_human_brain.h"
#include "alife_human_object_handler.h"
#include "alife_object_registry.h"
#include "alife_spawn_registry.h"
#include "alife_time_manager.h"
#include "ef_pattern.h"
#include "ef_storage.h"
#include "relation_registry.h"

using namespace ALife;

#define NORMALIZE_VARIABLE(a, b, c, d) a = u32(b % c) + d, b /= c;

void print_time(LPCSTR S, _TIME_ID tTimeID) {
  u32 Milliseconds, Seconds, Minutes, Hours, Days, Week, Months, Years;
  NORMALIZE_VARIABLE(Milliseconds, tTimeID, 1000, 0);
  NORMALIZE_VARIABLE(Seconds, tTimeID, 60, 0);
  NORMALIZE_VARIABLE(Minutes, tTimeID, 60, 0);
  NORMALIZE_VARIABLE(Hours, tTimeID, 24, 0);
  NORMALIZE_VARIABLE(Days, tTimeID, 7, 1);
  NORMALIZE_VARIABLE(Week, tTimeID, 4, 1);
  NORMALIZE_VARIABLE(Months, tTimeID, 12, 1);
  Years = u32(tTimeID) + 1;
  Msg("%s year %d month %d week %d day %d time %d:%d:%d.%d", S, Years, Months,
      Week, Days, Hours, Minutes, Seconds, Milliseconds);
}

CALifeCombatManager::CALifeCombatManager(IPureServer *server, LPCSTR section)
    : CALifeSimulatorBase(server, section) {
  seed(u32(CPU::QPC() & 0xffffffff));
  m_dwMaxCombatIterationCount =
      pSettings->r_u32(section, "max_combat_iteration_count");
  m_combat_type = eCombatTypeMonsterMonster;
  for (int i = 0; i < 2; ++i) {
    m_tpaCombatGroups[i].clear();
    m_tpaCombatGroups[i].reserve(255);
    m_tpaCombatObjects[i] = 0;
  }
}

CALifeCombatManager::~CALifeCombatManager() {}

void CALifeCombatManager::vfFillCombatGroup(
    CSE_ALifeSchedulable *tpALifeSchedulable, int iGroupIndex) {
  EHitType l_tHitType;
  float l_fHitPower;
  SCHEDULE_P_VECTOR &tpGroupVector = m_tpaCombatGroups[iGroupIndex];
  CSE_ALifeGroupAbstract *l_tpALifeGroupAbstract =
      smart_cast<CSE_ALifeGroupAbstract *>(tpALifeSchedulable);
  tpGroupVector.clear();
  if (l_tpALifeGroupAbstract) {
    OBJECT_IT I = l_tpALifeGroupAbstract->m_tpMembers.begin();
    OBJECT_IT E = l_tpALifeGroupAbstract->m_tpMembers.end();
    for (; I != E; ++I) {
      CSE_ALifeSchedulable *l_tpALifeSchedulable =
          smart_cast<CSE_ALifeSchedulable *>(objects().object(*I));
      R_ASSERT2(l_tpALifeSchedulable, "Invalid combat object");
      tpGroupVector.push_back(l_tpALifeSchedulable);
      l_tpALifeSchedulable->tpfGetBestWeapon(l_tHitType, l_fHitPower);
    }
  } else {
    tpGroupVector.push_back(tpALifeSchedulable);
    tpALifeSchedulable->tpfGetBestWeapon(l_tHitType, l_fHitPower);
  }
  m_tpaCombatObjects[iGroupIndex] = tpALifeSchedulable;
  // W.2: the opposing group (if already filled) becomes the combat
  // target for situation-aware weapon selection (W.3)
  vfUpdateCombatTargets(iGroupIndex);
}

// W.2: for every human in group iGroupIndex, store the opposing
// groups position and a representative enemy in the object handler, so
// best_weapon() can score weapons by distance / enemy class / cluster.
void CALifeCombatManager::vfUpdateCombatTargets(int iGroupIndex) {
  const SCHEDULE_P_VECTOR &enemy_group = m_tpaCombatGroups[iGroupIndex ^ 1];
  if (enemy_group.empty())
    return;

  xr_vector<Fvector> enemy_positions;
  enemy_positions.reserve(enemy_group.size());
  CSE_ALifeMonsterAbstract *rep = 0;
  SCHEDULE_P_VECTOR::const_iterator E = enemy_group.begin();
  for (; E != enemy_group.end(); ++E) {
    CSE_ALifeMonsterAbstract *m = smart_cast<CSE_ALifeMonsterAbstract *>(*E);
    if (!m)
      continue;
    if (!rep)
      rep = m;
    enemy_positions.push_back(m->draw_level_position());
  }
  if (!rep)
    return;

  SCHEDULE_P_VECTOR &self_group = m_tpaCombatGroups[iGroupIndex];
  SCHEDULE_P_IT I = self_group.begin();
  for (; I != self_group.end(); ++I) {
    CSE_ALifeHumanAbstract *human = smart_cast<CSE_ALifeHumanAbstract *>(*I);
    if (!human)
      continue;
    human->brain().objects().set_combat_target(rep->draw_level_position(), rep,
                                               &enemy_positions[0],
                                               (int)enemy_positions.size());
  }
}

ECombatAction CALifeCombatManager::choose_combat_action(int iCombatGroupIndex) {
  ai().ef_storage().alife_evaluation(true);
  if (eCombatTypeMonsterMonster != combat_type())
    if (((eCombatTypeAnomalyMonster == combat_type()) && !iCombatGroupIndex) ||
        ((eCombatTypeMonsterAnomaly == combat_type()) && iCombatGroupIndex))
      return (eCombatActionAttack);
    else
      return (eCombatActionRetreat);

  SCHEDULE_P_VECTOR &Members = m_tpaCombatGroups[iCombatGroupIndex];
  SCHEDULE_P_VECTOR &Enemies = m_tpaCombatGroups[iCombatGroupIndex ^ 1];
  int i = 0, j = 0, I = (int)Members.size(), J = (int)Enemies.size();
  float fMinProbability;
  if (!I)
    fMinProbability = 0;
  else {
    CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
        smart_cast<CSE_ALifeMonsterAbstract *>(Members[0]);
    R_ASSERT2(l_tpALifeMonsterAbstract, "Invalid combat object");
    fMinProbability = l_tpALifeMonsterAbstract->m_fRetreatThreshold;
  }
  while ((i < I) && (j < J)) {
    ai().ef_storage().alife().member() =
        smart_cast<CSE_ALifeMonsterAbstract *>(Members[i]);
    ai().ef_storage().alife().enemy() =
        smart_cast<CSE_ALifeMonsterAbstract *>(Enemies[j]);
    float fProbability =
              ai().ef_storage().m_pfVictoryProbability->ffGetValue() / 100.f,
          fCurrentProbability;
    if (fProbability > fMinProbability) {
      fCurrentProbability = fProbability;
      for (++j; (i < I) && (j < J); ++j) {
        ai().ef_storage().alife().enemy() =
            smart_cast<CSE_ALifeMonsterAbstract *>(Enemies[j]);
        fProbability =
            ai().ef_storage().m_pfVictoryProbability->ffGetValue() / 100.f;
        if (fCurrentProbability * fProbability < fMinProbability) {
          ++i;
          break;
        } else
          fCurrentProbability *= fProbability;
      }
    } else {
      fCurrentProbability = 1.0f - fProbability;
      for (++i; (i < I) && (j < J); ++i) {
        ai().ef_storage().alife().member() =
            smart_cast<CSE_ALifeMonsterAbstract *>(Members[i]);
        fProbability =
            1.0f -
            ai().ef_storage().m_pfVictoryProbability->ffGetValue() / 100.f;
        if (fCurrentProbability * fProbability < fMinProbability) {
          ++j;
          break;
        } else
          fCurrentProbability *= fProbability;
      }
    }
  }
  return ((j >= J) ? eCombatActionAttack : eCombatActionRetreat);
}

bool CALifeCombatManager::bfCheckObjectDetection(
    CSE_ALifeSchedulable *tpALifeSchedulable1,
    CSE_ALifeSchedulable *tpALifeSchedulable2) {
  ai().ef_storage().alife_evaluation(true);
  switch (combat_type()) {
  case eCombatTypeMonsterMonster: {
    ai().ef_storage().alife().member_item() =
        smart_cast<CSE_ALifeMonsterAbstract *>(tpALifeSchedulable1);
    ai().ef_storage().alife().member() = tpALifeSchedulable1;
    ai().ef_storage().alife().enemy() = tpALifeSchedulable2;
    return (randF(100) <
            ai().ef_storage().m_pfEnemyDetectProbability->ffGetValue());
  }
  case eCombatTypeAnomalyMonster: {
    ai().ef_storage().alife().enemy() = tpALifeSchedulable1;
    return (randF(100) <
            ai().ef_storage().m_pfAnomalyInteractProbability->ffGetValue());
  }
  case eCombatTypeMonsterAnomaly: {
    ai().ef_storage().alife().member_item() =
        tpALifeSchedulable1->tpfGetBestDetector();
    ai().ef_storage().alife().member() = tpALifeSchedulable1;
    ai().ef_storage().alife().enemy() = tpALifeSchedulable2;
    return (randF(100) <
            ai().ef_storage().m_pfAnomalyDetectProbability->ffGetValue());
  }
  case eCombatTypeSmartTerrain: {
    CSE_ALifeSmartZone *smart_zone =
        smart_cast<CSE_ALifeSmartZone *>(tpALifeSchedulable1);
    return (!smart_zone
                ? false
                : randF(100) < 100.f * smart_zone->detect_probability());
  }
  default:
    NODEFAULT;
  }
#ifdef DEBUG
  return (false);
#endif // DEBUG
}

bool CALifeCombatManager::bfCheckForInteraction(
    CSE_ALifeSchedulable *tpALifeSchedulable1,
    CSE_ALifeSchedulable *tpALifeSchedulable2, int &iCombatGroupIndex,
    bool &bMutualDetection) {
  if (!tpALifeSchedulable1->bfActive() || !tpALifeSchedulable2->bfActive())
    return (false);

  // determine combat type
  CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract1 =
      smart_cast<CSE_ALifeMonsterAbstract *>(tpALifeSchedulable1);
  CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract2 =
      smart_cast<CSE_ALifeMonsterAbstract *>(tpALifeSchedulable2);
  if (!l_tpALifeMonsterAbstract1) {
    if (!l_tpALifeMonsterAbstract2)
      return (false);
    else {
      CSE_ALifeCustomZone *l_tpALifeSpaceRestrictor =
          smart_cast<CSE_ALifeCustomZone *>(tpALifeSchedulable2);
      R_ASSERT2(l_tpALifeSpaceRestrictor, "Unknown schedulable object class");
      m_combat_type = eCombatTypeAnomalyMonster;
    }
  } else {
    if (!l_tpALifeMonsterAbstract2) {
      CSE_ALifeCustomZone *l_tpALifeSpaceRestrictor =
          smart_cast<CSE_ALifeCustomZone *>(tpALifeSchedulable2);
      if (!l_tpALifeSpaceRestrictor) {
        CSE_ALifeSmartZone *smart_zone =
            smart_cast<CSE_ALifeSmartZone *>(tpALifeSchedulable2);
        if (smart_zone)
          m_combat_type = eCombatTypeSmartTerrain;
        else
          R_ASSERT2(false, "Unknown schedulable object class");
      } else
        m_combat_type = eCombatTypeMonsterAnomaly;
    } else {
      m_combat_type = eCombatTypeMonsterMonster;
      if (eRelationTypeFriend ==
          relation_type(l_tpALifeMonsterAbstract1, l_tpALifeMonsterAbstract2)) {
        CSE_ALifeHumanAbstract *l_tpALifeHumanAbstract1 =
            smart_cast<CSE_ALifeHumanAbstract *>(l_tpALifeMonsterAbstract1);
        CSE_ALifeHumanAbstract *l_tpALifeHumanAbstract2 =
            smart_cast<CSE_ALifeHumanAbstract *>(l_tpALifeMonsterAbstract2);
        if (l_tpALifeHumanAbstract1 && l_tpALifeHumanAbstract2) {
          iCombatGroupIndex = 0;
          return (true);
        } else
          return (false);
      }
    }
  }

  // perform interaction
#if defined(DEBUG) || defined(DEBUG_ALIFE)

  if (ALIFE_LOG_ON) {
    GameGraph::_GRAPH_ID l_tGraphID =
        l_tpALifeMonsterAbstract1 ? l_tpALifeMonsterAbstract1->m_tGraphID
                                  : l_tpALifeMonsterAbstract2->m_tGraphID;
    print_time("\n[LSS]", time_manager().game_time());
    Msg("[LSS] %s met %s on the graph point %d (level %s[%d][%d][%d][%d], "
        "[%f][%f][%f])",
        tpALifeSchedulable1->base()->name_replace(),
        tpALifeSchedulable2->base()->name_replace(), l_tGraphID,
        *ai().game_graph()
             .header()
             .levels()
             .find(ai().game_graph().vertex(l_tGraphID)->level_id())
             ->second.name(),
        ai().game_graph().vertex(l_tGraphID)->vertex_type()[0],
        ai().game_graph().vertex(l_tGraphID)->vertex_type()[1],
        ai().game_graph().vertex(l_tGraphID)->vertex_type()[2],
        ai().game_graph().vertex(l_tGraphID)->vertex_type()[3],
        VPUSH(ai().game_graph().vertex(l_tGraphID)->level_point()));
  }
#endif

  bMutualDetection = false;
  iCombatGroupIndex = -1;

  if (bfCheckObjectDetection(tpALifeSchedulable1, tpALifeSchedulable2)) {
    iCombatGroupIndex = 0;
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] %s detected %s", tpALifeSchedulable1->base()->name_replace(),
          tpALifeSchedulable2->base()->name_replace());
    }
#endif
  } else {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] %s didn't detect %s",
          tpALifeSchedulable1->base()->name_replace(),
          tpALifeSchedulable2->base()->name_replace());
    }
#endif
  }

  if (eCombatTypeMonsterAnomaly == combat_type())
    m_combat_type = eCombatTypeAnomalyMonster;
  else if (eCombatTypeAnomalyMonster == combat_type())
    m_combat_type = eCombatTypeMonsterAnomaly;

  if (bfCheckObjectDetection(tpALifeSchedulable2, tpALifeSchedulable1)) {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] %s detected %s", tpALifeSchedulable2->base()->name_replace(),
          tpALifeSchedulable1->base()->name_replace());
    }
#endif
    if (!iCombatGroupIndex)
      bMutualDetection = true;
    else
      iCombatGroupIndex = 1;
  } else {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] %s didn't detect %s",
          tpALifeSchedulable2->base()->name_replace(),
          tpALifeSchedulable1->base()->name_replace());
    }
#endif
  }

  if (eCombatTypeMonsterAnomaly == combat_type())
    m_combat_type = eCombatTypeAnomalyMonster;
  else if (eCombatTypeAnomalyMonster == combat_type())
    m_combat_type = eCombatTypeMonsterAnomaly;

  if (iCombatGroupIndex < 0) {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] There is no interaction");
    }
#endif
    return (false);
  } else
    return (true);
}

bool CALifeCombatManager::bfCheckIfRetreated(int iCombatGroupIndex) {
  ai().ef_storage().alife_evaluation(true);
  ai().ef_storage().alife().member_item() =
      (eCombatTypeMonsterMonster == combat_type())
          ? smart_cast<CSE_ALifeObject *>(
                m_tpaCombatGroups[iCombatGroupIndex][0])
          : m_tpaCombatGroups[iCombatGroupIndex][0]->m_tpBestDetector;
  ai().ef_storage().alife().member() = m_tpaCombatGroups[iCombatGroupIndex][0];
  ai().ef_storage().alife().enemy() =
      m_tpaCombatGroups[iCombatGroupIndex ^ 1][0];
  return (randF(100) <
          ((eCombatTypeMonsterMonster != combat_type())
               ? ai().ef_storage().m_pfAnomalyRetreatProbability->ffGetValue()
               : ai().ef_storage().m_pfEnemyRetreatProbability->ffGetValue()));
}

void CALifeCombatManager::vfPerformAttackAction(int iCombatGroupIndex) {
  // W.2: refresh the combat context before best_weapon() is
  // re-evaluated for the attacking group
  vfUpdateCombatTargets(iCombatGroupIndex);
  ai().ef_storage().alife_evaluation(true);
  SCHEDULE_P_VECTOR &l_tCombatGroup = m_tpaCombatGroups[iCombatGroupIndex];
  SCHEDULE_P_IT I = l_tCombatGroup.begin();
  SCHEDULE_P_IT E = l_tCombatGroup.end();
  for (; I != E; ++I) {
    EHitType l_tHitType = eHitTypeMax;
    float l_fHitPower = 0.f;
    // W.3: while the combat context is known, humans re-evaluate
    // their weapon every round (ammo drops, GL rules, cluster)
    CSE_ALifeHumanAbstract *l_tpHuman =
        smart_cast<CSE_ALifeHumanAbstract *>(*I);
    if (l_tpHuman && l_tpHuman->brain().objects().has_combat_target())
      l_tpHuman->m_tpCurrentBestWeapon = 0;
    if (!(*I)->m_tpCurrentBestWeapon) {
      CSE_ALifeItemWeapon *l_tpALifeItemWeapon =
          (*I)->tpfGetBestWeapon(l_tHitType, l_fHitPower);
      if (!l_tpALifeItemWeapon && (l_fHitPower <= EPS_L))
        continue;
    } else {
      l_tHitType = (*I)->m_tpCurrentBestWeapon->m_tHitType;
      l_fHitPower = (*I)->m_tpCurrentBestWeapon->m_fHitPower;
    }

    ai().ef_storage().alife().member_item() = smart_cast<CSE_ALifeObject *>(*I);
    ai().ef_storage().alife().member() = *I;
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] %s attacks with %s(%d ammo) %d times in a row",
          (*I)->base()->name_replace(),
          (*I)->m_tpCurrentBestWeapon
              ? (*I)->m_tpCurrentBestWeapon->name_replace()
              : "its natural weapon",
          (*I)->m_tpCurrentBestWeapon
              ? (*I)->m_tpCurrentBestWeapon->m_dwAmmoAvailable
              : 0,
          iFloor(ai().ef_storage().m_pfWeaponAttackTimes->ffGetValue() + .5f));
    }
#endif
    for (int i = 0,
             n = iFloor(ai().ef_storage().m_pfWeaponAttackTimes->ffGetValue() +
                        .5f);
         i < n; ++i) {
      if (randF(100) <
          ai().ef_storage().m_pfWeaponSuccessProbability->ffGetValue()) {
        // choose random enemy group member and perform hit with random power
        // multiplied by immunity factor
        int l_iIndex = randI(m_tpaCombatGroups[iCombatGroupIndex ^ 1].size());
        CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
            smart_cast<CSE_ALifeMonsterAbstract *>(
                m_tpaCombatGroups[iCombatGroupIndex ^ 1][l_iIndex]);
        R_ASSERT2(l_tpALifeMonsterAbstract, "Invalid combat object");
        float l_fHit = randF(l_fHitPower * 0.5f, l_fHitPower * 1.5f);
        l_tpALifeMonsterAbstract->set_health(
            l_tpALifeMonsterAbstract->get_health() -
            l_tpALifeMonsterAbstract->m_fpImmunityFactors[l_tHitType] * l_fHit);
#if defined(DEBUG) || defined(DEBUG_ALIFE)

        if (ALIFE_LOG_ON) {
          Msg("[LSS] %s %s %s [power %5.2f][damage %5.2f][health "
              "%5.2f][creatures left %d]",
              (*I)->base()->name_replace(),
              l_tpALifeMonsterAbstract->get_health() <= 0 ? "killed"
                                                          : "attacked",
              l_tpALifeMonsterAbstract->name_replace(), l_fHit,
              l_tpALifeMonsterAbstract->m_fpImmunityFactors[l_tHitType] *
                  l_fHit,
              _max(l_tpALifeMonsterAbstract->get_health(), 0.f),
              l_tpALifeMonsterAbstract->get_health() >= EPS_L
                  ? m_tpaCombatGroups[iCombatGroupIndex ^ 1].size()
                  : m_tpaCombatGroups[iCombatGroupIndex ^ 1].size() - 1);
        }
#endif
        // check if victim became dead
        if (l_tpALifeMonsterAbstract->get_health() <= 0) {
          m_tpaCombatGroups[iCombatGroupIndex ^ 1].erase(
              m_tpaCombatGroups[iCombatGroupIndex ^ 1].begin() + l_iIndex);
          if (m_tpaCombatGroups[iCombatGroupIndex ^ 1].empty())
            return;
        }
      } else {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

        if (ALIFE_LOG_ON) {
          Msg("[LSS] %s missed", (*I)->base()->name_replace());
        }
#endif
      }
      // perform attack (if we use a weapon we should delete ammo we used)
      if (!(*I)->bfPerformAttack())
        break;
    }
  }
}

void CALifeCombatManager::vfFinishCombat(ECombatResult tCombatResult) {
  // W.3: clear the combat context, post-combat decisions (trade,
  // loot) must not use stale enemy data
  for (int i = 0; i < 2; ++i) {
    SCHEDULE_P_IT J = m_tpaCombatGroups[i].begin();
    for (; J != m_tpaCombatGroups[i].end(); ++J) {
      CSE_ALifeHumanAbstract *l_tpHuman =
          smart_cast<CSE_ALifeHumanAbstract *>(*J);
      if (l_tpHuman)
        l_tpHuman->brain().objects().reset_combat_target();
    }
  }
  // processing weapons and dead monsters
  CSE_ALifeDynamicObject *l_tpALifeDynamicObject =
      smart_cast<CSE_ALifeDynamicObject *>(m_tpaCombatObjects[0]);
  R_ASSERT2(l_tpALifeDynamicObject, "Unknown schedulable object class");
  GameGraph::_GRAPH_ID l_tGraphID = l_tpALifeDynamicObject->m_tGraphID;
  m_temp_item_vector.clear();
  for (int i = 0; i < 2; ++i) {
    CSE_ALifeGroupAbstract *l_tpALifeGroupAbstract =
        smart_cast<CSE_ALifeGroupAbstract *>(m_tpaCombatObjects[i]);
    if (l_tpALifeGroupAbstract) {
      for (int I = 0, N = l_tpALifeGroupAbstract->m_tpMembers.size(); I < N;
           ++I) {
        CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
            smart_cast<CSE_ALifeMonsterAbstract *>(
                objects().object(l_tpALifeGroupAbstract->m_tpMembers[I]));
        R_ASSERT2(l_tpALifeMonsterAbstract, "Invalid group member!");
        l_tpALifeMonsterAbstract->vfUpdateWeaponAmmo();
        if (l_tpALifeMonsterAbstract->get_health() <= EPS_L) {
          append_item_vector(l_tpALifeMonsterAbstract->children,
                             m_temp_item_vector);
          l_tpALifeMonsterAbstract->m_bDirectControl = true;
          l_tpALifeGroupAbstract->m_tpMembers.erase(
              l_tpALifeGroupAbstract->m_tpMembers.begin() + I);
          assign_death_position(l_tpALifeMonsterAbstract, l_tGraphID,
                                m_tpaCombatObjects[i ^ 1]);
          l_tpALifeMonsterAbstract->vfDetachAll();
          R_ASSERT(l_tpALifeMonsterAbstract->children.empty());
          register_object(l_tpALifeMonsterAbstract);
          CSE_ALifeInventoryItem *l_tpALifeInventoryItem =
              smart_cast<CSE_ALifeInventoryItem *>(l_tpALifeMonsterAbstract);
          if (l_tpALifeInventoryItem)
            m_temp_item_vector.push_back(l_tpALifeInventoryItem);
          --l_tpALifeGroupAbstract->m_wCount;
          --I;
          --N;
        }
      }
    } else {
      m_tpaCombatObjects[i]->vfUpdateWeaponAmmo();
      CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract =
          smart_cast<CSE_ALifeMonsterAbstract *>(m_tpaCombatObjects[i]);
      if (l_tpALifeMonsterAbstract &&
          (l_tpALifeMonsterAbstract->get_health() <= EPS_L)) {
        kill_entity(l_tpALifeMonsterAbstract, l_tGraphID,
                    m_tpaCombatObjects[i ^ 1]);
      }
    }
  }

  if (m_temp_item_vector.empty() ||
      (eCombatTypeMonsterMonster != combat_type())) {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] There is nothing to take");
    }
#endif
    return;
  }

  int l_iGroupIndex = -1;
  switch (tCombatResult) {
  case eCombatResultBothKilled:
  case eCombatResultRetreat12:
    break;
  case eCombatResultRetreat2:
  case eCombatResult1Kill2: {
    l_iGroupIndex = 0;
    break;
  }
  case eCombatResultRetreat1:
  case eCombatResult2Kill1: {
    l_iGroupIndex = 1;
    break;
  }
  default:
    NODEFAULT;
  }

  if (l_iGroupIndex >= 0) {
#if defined(DEBUG) || defined(DEBUG_ALIFE)

    if (ALIFE_LOG_ON) {
      Msg("[LSS] Starting taking items [%s][%f]",
          m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),
          smart_cast<CSE_ALifeMonsterAbstract *>(
              m_tpaCombatObjects[l_iGroupIndex])
              ->get_health());
    }
#endif
    m_tpaCombatObjects[l_iGroupIndex]->vfAttachItems();
  }
  m_temp_item_vector.clear();
}

ALife::ERelationType CALifeCombatManager::relation_type(
    CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract1,
    CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract2) const {
  CSE_ALifeTraderAbstract *human1 =
      smart_cast<CSE_ALifeTraderAbstract *>(tpALifeMonsterAbstract1);
  CSE_ALifeTraderAbstract *human2 =
      smart_cast<CSE_ALifeTraderAbstract *>(tpALifeMonsterAbstract2);

  if (human1 && human2) {
    // relation registry works on server ALife objects (CSE_*), not client
    // CEntityAlive
    ALife::ERelationType rel = RELATION_REGISTRY().GetRelationBetween(
        (CSE_ALifeTraderAbstract *)tpALifeMonsterAbstract1,
        (CSE_ALifeTraderAbstract *)tpALifeMonsterAbstract2);
    return rel;
  }

  if (tpALifeMonsterAbstract1->g_team() != tpALifeMonsterAbstract2->g_team())
    return (ALife::eRelationTypeEnemy);
  else
    return (ALife::eRelationTypeNeutral);
}

void CALifeCombatManager::kill_entity(
    CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract,
    const GameGraph::_GRAPH_ID &l_tGraphID, CSE_ALifeSchedulable *schedulable) {
  VERIFY(l_tpALifeMonsterAbstract->g_Alive());
  append_item_vector(l_tpALifeMonsterAbstract->children, m_temp_item_vector);
  GameGraph::_GRAPH_ID l_tGraphID1 = l_tpALifeMonsterAbstract->m_tGraphID;
  assign_death_position(l_tpALifeMonsterAbstract, l_tGraphID, schedulable);
  l_tpALifeMonsterAbstract->vfDetachAll();
  R_ASSERT(l_tpALifeMonsterAbstract->children.empty());
  scheduled().remove(l_tpALifeMonsterAbstract);
  if (l_tpALifeMonsterAbstract->m_tGraphID != l_tGraphID1) {
    graph().remove(l_tpALifeMonsterAbstract, l_tGraphID1);
    graph().add(l_tpALifeMonsterAbstract, l_tpALifeMonsterAbstract->m_tGraphID);
  }
  CSE_ALifeInventoryItem *l_tpALifeInventoryItem =
      smart_cast<CSE_ALifeInventoryItem *>(l_tpALifeMonsterAbstract);
  if (l_tpALifeInventoryItem)
    m_temp_item_vector.push_back(l_tpALifeInventoryItem);
}
