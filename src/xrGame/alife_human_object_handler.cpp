////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler.cpp
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_human_object_handler.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"


// Stage 2.2: needed by best_detector() to reach member->brain().objects()
#include "alife_human_brain.h"
#include "alife_graph_registry.h"

// Stage 2.1: count the ammo of the given weapon section the object carries
// (2003 CSE_ALifeHumanAbstract::get_available_ammo_count, save L171-185).
// No ammo section -> unlimited (-1); weapons without ammo (slot 0/3) rely
// on this too.
u16 CALifeHumanObjectHandler::get_available_ammo_count(
    const CSE_ALifeItemWeapon *weapon, ALife::OBJECT_VECTOR &objects) {
  if (!weapon->m_caAmmoSections)
    return (u16(-1));

  u32 l_dwResult = 0;
  ALife::OBJECT_IT I = objects.begin();
  ALife::OBJECT_IT E = objects.end();
  for (; I != E; ++I) {
    CSE_ALifeItemAmmo *l_tpALifeItemAmmo =
        smart_cast<CSE_ALifeItemAmmo *>(ai().alife().objects().object(*I));
    if (l_tpALifeItemAmmo &&
        strstr(weapon->m_caAmmoSections, l_tpALifeItemAmmo->s_name.c_str()))
      l_dwResult += l_tpALifeItemAmmo->a_elapsed;
  }
  return (u16(l_dwResult));
}

// Stage 3.1: 2003 CSE_ALifeHumanAbstract::get_available_ammo_count
// (save L187-205) for items gathered from the graph. The 2005 code has no
// m_dwTotalMoney on the human, so the money check is dropped (Stage 4
// communication will carry the money logic).
u16 CALifeHumanObjectHandler::get_available_ammo_count(
    const CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR &items,
    ALife::OBJECT_VECTOR *objects) {
  if (!weapon->m_caAmmoSections)
    return (u16(-1));

  u32 l_dwResult = 0;
  ALife::ITEM_P_VECTOR::const_iterator I = items.begin();
  ALife::ITEM_P_VECTOR::const_iterator E = items.end();
  for (; I != E; ++I) {
    CSE_ALifeItemAmmo *l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo *>(*I);
    if (l_tpALifeItemAmmo &&
        strstr(weapon->m_caAmmoSections, l_tpALifeItemAmmo->s_name.c_str()) &&
        (!objects || (std::find(objects->begin(), objects->end(),
                                l_tpALifeItemAmmo->ID) == objects->end())))
      l_dwResult += l_tpALifeItemAmmo->a_elapsed;
  }
  return (u16(l_dwResult));
}

// Stage 3.1: 2003 CSE_ALifeHumanAbstract::attach_available_ammo
// (save L207-232). Attaches up to MAX_AMMO_ATTACH_COUNT boxes from the
// gathered items to the graph vertex (objects==0) or to the children list
// (objects!=0). Money check dropped (see above).
void CALifeHumanObjectHandler::attach_available_ammo(
    CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR &items,
    ALife::OBJECT_VECTOR *objects) {
  if (!weapon || !weapon->m_caAmmoSections)
    return;

  constexpr u32 MAX_AMMO_ATTACH_COUNT = 1;
  u32 l_dwCount = 0;
  ALife::ITEM_P_VECTOR::const_iterator I = items.begin();
  ALife::ITEM_P_VECTOR::const_iterator E = items.end();
  for (; I != E; ++I) {
    CSE_ALifeItemAmmo *l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo *>(*I);
    if (l_tpALifeItemAmmo &&
        strstr(weapon->m_caAmmoSections, l_tpALifeItemAmmo->s_name.c_str()) &&
        can_take_item(l_tpALifeItemAmmo) &&
        (!objects || (std::find(objects->begin(), objects->end(),
                                l_tpALifeItemAmmo->ID) == objects->end()))) {
      if (!objects)
        const_cast<CALifeSimulator &>(ai().alife()).graph().attach(*m_object, l_tpALifeItemAmmo,
                                    l_tpALifeItemAmmo->m_tGraphID);
      else
        m_object->children.push_back(l_tpALifeItemAmmo->ID);
      ++l_dwCount;
      if (l_dwCount >= MAX_AMMO_ATTACH_COUNT)
        break;
    }
  }
}

// Stage 3.1: 2003 CSE_ALifeHumanAbstract::bfCanGetItem (save L327-333).
// 2005 CSE_ALifeInventoryItem has no m_iVolume, so the volume check is
// dropped (m_iCumulativeItemVolume stays 0 for now).
bool CALifeHumanObjectHandler::can_take_item(
    CSE_ALifeInventoryItem *inventory_item) {
  if (inventory_item &&
      ((m_object->m_fCumulativeItemMass + inventory_item->m_fMass >
        m_object->m_fMaxItemMass)))
    return (false);
  return (true);
}
// Stage 2.2: merge ammo boxes of the same section (2003 vfCollectAmmoBoxes,
// save L79-134). Local marks vector instead of alife().m_temp_marks (Stage 4).
void CALifeHumanObjectHandler::collect_ammo_boxes() {
  object_type &object = *m_object;
  int n = (int)object.children.size();
  xr_vector<bool> marks(n, false);

  for (int i = 0; i < n; ++i) {
    if (marks[i])
      continue;
    marks[i] = true;

    CSE_ALifeItemAmmo *box = smart_cast<CSE_ALifeItemAmmo *>(
        ai().alife().objects().object(object.children[i]));
    if (!box)
      continue;

    for (int j = i + 1; j < n; ++j) {
      if (marks[j])
        continue;

      CSE_ALifeItemAmmo *box1 = smart_cast<CSE_ALifeItemAmmo *>(
          ai().alife().objects().object(object.children[j]));
      if (!box1) {
        marks[j] = true;
        continue;
      }

      if (!strstr(box->s_name.c_str(), box1->s_name.c_str()))
        continue;

      marks[j] = true;

      if (box->a_elapsed + box1->a_elapsed > box->m_boxSize) {
        box1->a_elapsed = box->a_elapsed + box1->a_elapsed - box->m_boxSize;
        box->a_elapsed = box->m_boxSize;
        box = box1;
      } else {
        box->a_elapsed = box->a_elapsed + box1->a_elapsed;
        box1->a_elapsed = 0;
      }
    }
  }

  for (int i = 0, j = 0; i < n; ++i, ++j) {
    marks[j] = false;
    CSE_ALifeItemAmmo *box = smart_cast<CSE_ALifeItemAmmo *>(
        ai().alife().objects().object(object.children[i]));
    if (!box || box->a_elapsed)
      continue;
    const_cast<CALifeSimulator &>(ai().alife()).release(box, true);
    --i;
    --n;
  }
}
int CALifeHumanObjectHandler::choose_equipment(ALife::OBJECT_VECTOR *objects) {
  return (-1);
}
int CALifeHumanObjectHandler::choose_weapon(
    const ALife::EWeaponPriorityType &weapon_priority_type,
    ALife::OBJECT_VECTOR *objects) {
  return (-1);
}

int CALifeHumanObjectHandler::choose_food(ALife::OBJECT_VECTOR *objects) {
  return (-1);
}
int CALifeHumanObjectHandler::choose_medikit(ALife::OBJECT_VECTOR *objects) {
  return (-1);
}
int CALifeHumanObjectHandler::choose_detector(ALife::OBJECT_VECTOR *objects) {
  return (-1);
}
int CALifeHumanObjectHandler::choose_valuables() { return (-1); }
bool CALifeHumanObjectHandler::choose_fast() { return (false); }
void CALifeHumanObjectHandler::choose_group(
    CSE_ALifeGroupAbstract *group_abstract) {}
void CALifeHumanObjectHandler::detach_all(bool fictitious) {}
// Stage 2.2: after combat, trim the ammo of the current best weapon (2003
// vfUpdateWeaponAmmo, save L136-169). Slot 0/3 (no external ammo) kept.
void CALifeHumanObjectHandler::update_weapon_ammo() {
  object_type &object = *m_object;
  CSE_ALifeItemWeapon *wpn = object.m_tpCurrentBestWeapon;
  if (!wpn)
    return;

  switch (wpn->get_slot()) {
  case 0:
  case 3:
    break;
  default: {
    int n = (int)object.children.size();
    for (int i = 0; i < n; ++i) {
      CSE_ALifeItemAmmo *ammo = smart_cast<CSE_ALifeItemAmmo *>(
          ai().alife().objects().object(object.children[i]));
      if (ammo && strstr(wpn->m_caAmmoSections, ammo->s_name.c_str())) {
        if (wpn->m_dwAmmoAvailable > ammo->a_elapsed) {
          wpn->m_dwAmmoAvailable -= ammo->a_elapsed;
          continue;
        }
        if (wpn->m_dwAmmoAvailable) {
          ammo->a_elapsed = (u16)wpn->m_dwAmmoAvailable;
          wpn->m_dwAmmoAvailable = 0;
          continue;
        }
        const_cast<CALifeSimulator &>(ai().alife()).release(ammo, true);
        --i;
        --n;
      }
    }
    object.m_tpCurrentBestWeapon = 0;
    break;
  }
  }
  collect_ammo_boxes();
}
// Stage 3.2: 2003 CSE_ALifeHumanAbstract::vfProcessItems (save
// L234-262): scan the items lying on the current graph point, keep
// the useful offline ones (detection probability fixed at 1.0f - the
// 2003 m_detect_probability field has no 2005 counterpart), then
// attach the collected items via attach_items().
void CALifeHumanObjectHandler::process_items()
{
  ALife::ITEM_P_VECTOR& items = const_cast<ALife::ITEM_P_VECTOR&>(ai().alife().m_temp_item_vector);
  items.clear();

  const CALifeSimulator& sim = ai().alife();
  const CSE_ALifeDynamicObject* self = m_object;
  const GameGraph::_GRAPH_ID gid = self->m_tGraphID;

  const CALifeGraphRegistry::OBJECT_REGISTRY& reg =
      const_cast<CALifeGraphRegistry::GRAPH_REGISTRY&>(
          ai().alife().graph().objects())[gid]
          .objects();
  for (auto& pair : reg.objects()) {
    CSE_ALifeInventoryItem* item =
        smart_cast<CSE_ALifeInventoryItem*>(pair.second);
    if (!item || !item->bfUseful() || pair.second->m_bOnline)
      continue;

    // 2003 m_detect_probability is absent in 2005: detection is
    // always successful (probability 1.0f).
    items.push_back(item);
  }

  if (!items.empty())
    attach_items();
}
// Stage 2.2: choose the best detector (2003 tpfGetBestDetector, save
// L281-325). 2005: detectors are CSE_ALifeItemDetector, no VISUAL clsid.
CSE_ALifeDynamicObject *CALifeHumanObjectHandler::best_detector() {
  object_type &object = *m_object;
  object.m_tpBestDetector = 0;

  CSE_ALifeGroupAbstract *group = smart_cast<CSE_ALifeGroupAbstract *>(&object);
  if (group) {
    u32 l_dwBestValue = 0;
    if (!group->m_wCount)
      return (0);
    for (u32 i = 0, n = group->m_tpMembers.size(); i < n; ++i) {
      CSE_ALifeHumanAbstract *member = smart_cast<CSE_ALifeHumanAbstract *>(
          ai().alife().objects().object(group->m_tpMembers[i]));
      if (!member)
        continue;
      CSE_ALifeDynamicObject *det = member->brain().objects().best_detector();
      u32 value = det ? det->ef_detector_type() : 0;
      if (value > l_dwBestValue) {
        l_dwBestValue = value;
        object.m_tpBestDetector = det;
      }
    }
    return (object.m_tpBestDetector);
  }

  for (u32 i = 0, n = object.children.size(); i < n; ++i) {
    CSE_ALifeDynamicObject *obj =
        ai().alife().objects().object(object.children[i]);
    if (!smart_cast<CSE_ALifeItemDetector *>(obj))
      continue;
    object.m_tpBestDetector = obj;
  }
  return (object.m_tpBestDetector);
}

// Stage 2.1: choose the best weapon the object carries (2003
// CSE_ALifeHumanAbstract::tpfGetBestWeapon, save L47-77). A weapon is a
// candidate if it has ammo, or is a knife (slot 0) / secondary (slot 3). The
// best (highest ef_weapon_type) candidate is stored in the object's
// m_tpCurrentBestWeapon and reported back.
CSE_ALifeItemWeapon *CALifeHumanObjectHandler::best_weapon() {
  object_type &object = *m_object;
  object.m_tpCurrentBestWeapon = 0;
  u32 l_dwBestWeapon = 0;

  ALife::OBJECT_IT I = object.children.begin();
  ALife::OBJECT_IT E = object.children.end();
  for (; I != E; ++I) {
    CSE_ALifeItemWeapon *l_tpALifeItemWeapon =
        smart_cast<CSE_ALifeItemWeapon *>(ai().alife().objects().object(*I));
    if (!l_tpALifeItemWeapon)
      continue;

    l_tpALifeItemWeapon->m_dwAmmoAvailable =
        get_available_ammo_count(l_tpALifeItemWeapon, object.children);
    if (l_tpALifeItemWeapon->m_dwAmmoAvailable ||
        (!l_tpALifeItemWeapon->get_slot()) ||
        (3 == l_tpALifeItemWeapon->get_slot())) {
      u32 l_dwCurrentBestWeapon = l_tpALifeItemWeapon->ef_weapon_type();
      if (l_dwCurrentBestWeapon > l_dwBestWeapon) {
        l_dwBestWeapon = l_dwCurrentBestWeapon;
        object.m_tpCurrentBestWeapon = l_tpALifeItemWeapon;
      }
    }
  }
  return (object.m_tpCurrentBestWeapon);
}

void CALifeHumanObjectHandler::attach_items() {}
