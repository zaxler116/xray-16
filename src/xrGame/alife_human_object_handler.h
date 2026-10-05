////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler.h
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CSE_ALifeItemWeapon;
class CSE_ALifeInventoryItem;
class CSE_ALifeGroupAbstract;
class CSE_ALifeHumanAbstract;

class CALifeHumanObjectHandler
{
public:
    typedef CSE_ALifeHumanAbstract object_type;

private:
    object_type* m_object;

public:
    IC CALifeHumanObjectHandler(object_type* object);
    IC object_type& object() const;

public:
    u16 get_available_ammo_count(const CSE_ALifeItemWeapon* weapon, ALife::OBJECT_VECTOR& objects);
    u16 get_available_ammo_count(
        const CSE_ALifeItemWeapon* weapon, ALife::ITEM_P_VECTOR& items, ALife::OBJECT_VECTOR* objects = 0);
    void attach_available_ammo(
        CSE_ALifeItemWeapon* weapon, ALife::ITEM_P_VECTOR& items, ALife::OBJECT_VECTOR* objects = 0);
    bool can_take_item(CSE_ALifeInventoryItem* inventory_item);
    void collect_ammo_boxes();

public:
    void detach_all(bool fictitious);
    void update_weapon_ammo();
    void process_items();
    CSE_ALifeDynamicObject* best_detector();
    CSE_ALifeItemWeapon* best_weapon();

public:
    int choose_equipment(ALife::OBJECT_VECTOR* objects = 0);
    int choose_weapon(const ALife::EWeaponPriorityType& weapon_priority_type, ALife::OBJECT_VECTOR* objects = 0);
    int choose_food(ALife::OBJECT_VECTOR* objects = 0);
    int choose_medikit(ALife::OBJECT_VECTOR* objects = 0);
    int choose_detector(ALife::OBJECT_VECTOR* objects = 0);
    int choose_valuables();
    bool choose_fast();
    void choose_group(CSE_ALifeGroupAbstract* group_abstract);
    void attach_items();
    // Stage 3.6: Min/Rest pick-up order, shared by attach_items() and choose_group()
    void attach_items_pick(ALife::ETakeType tTakeType);

    // N.2: trade "need" core. Decides how many items of a given type the
    // owner must keep (food stock, single medikit, primary weapon + ammo
    // reserve, equipment) and whether a particular item is personal /
    // keepable (i.e. must NOT be sold).
    int  item_keep_count(CSE_ALifeInventoryItem* item, CSE_ALifeHumanAbstract* owner) const;
    int  item_current_count(CSE_ALifeInventoryItem* item, CSE_ALifeHumanAbstract* owner) const;
    bool item_is_personal(CSE_ALifeInventoryItem* item, CSE_ALifeHumanAbstract* owner) const;
    bool item_is_keepable(CSE_ALifeInventoryItem* item, CSE_ALifeHumanAbstract* owner) const;
    // N.5: ammo boxes of the best weapon that must be kept (bullets reserve)
    int  ammo_keep_count(CSE_ALifeInventoryItem* item, CSE_ALifeHumanAbstract* owner) const;

    // N.2: trade tuning (defaults; N.5 will load them from a config section)
    float m_food_keep_days;     // game days of food to keep
    float m_ammo_keep_factor;   // extra ammo boxes to keep for the primary weapon
};

#include "alife_human_object_handler_inline.h"
