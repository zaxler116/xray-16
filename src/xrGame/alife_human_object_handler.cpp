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

// Stage 2.1: count the ammo of the given weapon section the object carries
// (2003 CSE_ALifeHumanAbstract::get_available_ammo_count, save L171-185).
// No ammo section -> unlimited (-1); weapons without ammo (slot 0/3) rely
// on this too.
u16 CALifeHumanObjectHandler::get_available_ammo_count(const CSE_ALifeItemWeapon* weapon, ALife::OBJECT_VECTOR& objects)
{
    if (!weapon->m_caAmmoSections)
        return (u16(-1));

    u32 l_dwResult = 0;
    ALife::OBJECT_IT I = objects.begin();
    ALife::OBJECT_IT E = objects.end();
    for ( ; I != E; ++I) {
        CSE_ALifeItemAmmo* l_tpALifeItemAmmo =
            smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(*I));
        if (l_tpALifeItemAmmo && strstr(weapon->m_caAmmoSections, l_tpALifeItemAmmo->s_name.c_str()))
            l_dwResult += l_tpALifeItemAmmo->a_elapsed;
    }
    return (u16(l_dwResult));
}

u16 CALifeHumanObjectHandler::get_available_ammo_count(
    const CSE_ALifeItemWeapon* weapon, ALife::ITEM_P_VECTOR& items, ALife::OBJECT_VECTOR* objects)
{
    return (0);
}

void CALifeHumanObjectHandler::attach_available_ammo(
    CSE_ALifeItemWeapon* weapon, ALife::ITEM_P_VECTOR& items, ALife::OBJECT_VECTOR* objects)
{
}

bool CALifeHumanObjectHandler::can_take_item(CSE_ALifeInventoryItem* inventory_item) { return (false); }
void CALifeHumanObjectHandler::collect_ammo_boxes() {}
int CALifeHumanObjectHandler::choose_equipment(ALife::OBJECT_VECTOR* objects) { return (-1); }
int CALifeHumanObjectHandler::choose_weapon(
    const ALife::EWeaponPriorityType& weapon_priority_type, ALife::OBJECT_VECTOR* objects)
{
    return (-1);
}

int CALifeHumanObjectHandler::choose_food(ALife::OBJECT_VECTOR* objects) { return (-1); }
int CALifeHumanObjectHandler::choose_medikit(ALife::OBJECT_VECTOR* objects) { return (-1); }
int CALifeHumanObjectHandler::choose_detector(ALife::OBJECT_VECTOR* objects) { return (-1); }
int CALifeHumanObjectHandler::choose_valuables() { return (-1); }
bool CALifeHumanObjectHandler::choose_fast() { return (false); }
void CALifeHumanObjectHandler::choose_group(CSE_ALifeGroupAbstract* group_abstract) {}
void CALifeHumanObjectHandler::detach_all(bool fictitious) {}
void CALifeHumanObjectHandler::update_weapon_ammo() {}
void CALifeHumanObjectHandler::process_items() {}
CSE_ALifeDynamicObject* CALifeHumanObjectHandler::best_detector() { return (0); }

// Stage 2.1: choose the best weapon the object carries (2003
// CSE_ALifeHumanAbstract::tpfGetBestWeapon, save L47-77). A weapon is a
// candidate if it has ammo, or is a knife (slot 0) / secondary (slot 3). The
// best (highest ef_weapon_type) candidate is stored in the object's
// m_tpCurrentBestWeapon and reported back.
CSE_ALifeItemWeapon* CALifeHumanObjectHandler::best_weapon()
{
    object_type& object = *m_object;
    object.m_tpCurrentBestWeapon = 0;
    u32 l_dwBestWeapon = 0;

    ALife::OBJECT_IT I = object.children.begin();
    ALife::OBJECT_IT E = object.children.end();
    for ( ; I != E; ++I) {
        CSE_ALifeItemWeapon* l_tpALifeItemWeapon =
            smart_cast<CSE_ALifeItemWeapon*>(ai().alife().objects().object(*I));
        if (!l_tpALifeItemWeapon)
            continue;

        l_tpALifeItemWeapon->m_dwAmmoAvailable = get_available_ammo_count(l_tpALifeItemWeapon, object.children);
        if (l_tpALifeItemWeapon->m_dwAmmoAvailable || (!l_tpALifeItemWeapon->get_slot()) || (3 == l_tpALifeItemWeapon->get_slot())) {
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
