////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler.h
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "xrServer_Objects_ALife.h"

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
    // W.2: combat context for situation-aware weapon selection (W.3).
    // Filled by CALifeCombatManager before best_weapon() is re-evaluated.
    CSE_ALifeMonsterAbstract* m_combat_target_enemy;
    Fvector m_combat_target_pos;
    xr_vector<Fvector> m_combat_enemy_positions;
    bool m_bHasCombatTarget;

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
    // W.2: combat context accessors (set by the combat manager)
    void set_combat_target(const Fvector& target_pos, CSE_ALifeMonsterAbstract* enemy, const Fvector* enemy_positions, int enemy_count);
    void reset_combat_target();
    bool has_combat_target() const;
    CSE_ALifeMonsterAbstract* combat_target_enemy() const;
    const Fvector& combat_target_pos() const;
    const Fvector* combat_enemy_positions(int& count) const;

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

    // V2.1: can the owner fight a single enemy of this class (best weapon +
    // ammo vs. the enemy's hp)? V3 will use it with world-knowledge enemy
    // counts to decide whether to go to a target at all.
    bool combat_power_estimate(CSE_ALifeHumanAbstract* owner, CSE_ALifeMonsterAbstract* enemy) const;

    // N.2: trade tuning (defaults; N.5 will load them from a config section)
    float m_food_keep_days;     // game days of food to keep
    float m_ammo_keep_factor;   // extra ammo boxes to keep for the primary weapon
};

#include "alife_human_object_handler_inline.h"
