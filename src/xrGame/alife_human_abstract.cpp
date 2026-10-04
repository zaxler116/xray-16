////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_abstract.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human abstract class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_human_brain.h"
#include "alife_human_object_handler.h"
#include "alife_object_registry.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "relation_registry.h"

void CSE_ALifeHumanAbstract::update()
{
    if (!bfActive())
        return;

    brain().update();
}

// Stage 2.3: real 2003 attack logic (CSE_ALifeHumanAbstract::bfPerformAttack,
// alife_human_brain_save.h L3-46), adapted to 2005 (m_dwSlot -> get_slot(),
// shared_str -> .c_str()). Replaces the temporary brain().perform_attack()
// bridge from Stage 1.2 and closes that recursion: the brain's
// perform_attack() routes back to this object method, which now works on
// object state only.
bool CSE_ALifeHumanAbstract::bfPerformAttack()
{
    if (!m_tpCurrentBestWeapon)
        return (false);

    switch (m_tpCurrentBestWeapon->get_slot()) {
    case 0:
        return (true);
    case 3: {
        bool l_bOk = false;
        ALife::OBJECT_IT I = children.begin();
        ALife::OBJECT_IT E = children.end();
        for (; I != E; ++I) {
            if (*I != m_tpCurrentBestWeapon->ID)
                continue;
            l_bOk = true;
            CSE_ALifeItem* l_tpALifeItem =
                smart_cast<CSE_ALifeItem*>(ai().alife().objects().object(*I));
            const_cast<CALifeSimulator&>(ai().alife()).release(l_tpALifeItem, true);
            break;
        }
        R_ASSERT2(l_bOk, "Cannot find specified weapon in the inventory");
        return (false);
    }
    default: {
        R_ASSERT2(m_tpCurrentBestWeapon->m_dwAmmoAvailable, "No ammo for the selected weapon!");
        if (!m_trader_flags.test(eTraderFlagInfiniteAmmo))
            --(m_tpCurrentBestWeapon->m_dwAmmoAvailable);
        if (m_tpCurrentBestWeapon->m_dwAmmoAvailable)
            return (true);

        for (int i = 0, n = (int)children.size(); i < n; ++i) {
            CSE_ALifeItemAmmo* l_tpALifeItemAmmo =
                smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[i]));
            if (l_tpALifeItemAmmo &&
                strstr(m_tpCurrentBestWeapon->m_caAmmoSections, l_tpALifeItemAmmo->s_name.c_str()) &&
                l_tpALifeItemAmmo->a_elapsed) {
                const_cast<CALifeSimulator&>(ai().alife()).release(l_tpALifeItemAmmo, true);
                --i;
                --n;
            }
        }
        m_tpCurrentBestWeapon = 0;
        return (false);
    }
    }
}
ALife::EMeetActionType CSE_ALifeHumanAbstract::tfGetActionType(
    CSE_ALifeSchedulable* schedulable, int iGroupIndex, bool bMutualDetection)
{
    return (brain().action_type(schedulable, iGroupIndex, bMutualDetection));
}

void CSE_ALifeHumanAbstract::vfDetachAll(bool bFictitious) { brain().objects().detach_all(bFictitious); }
void CSE_ALifeHumanAbstract::vfUpdateWeaponAmmo() { brain().objects().update_weapon_ammo(); }
void CSE_ALifeHumanAbstract::vfProcessItems() { brain().objects().process_items(); }
void CSE_ALifeHumanAbstract::vfAttachItems(ALife::ETakeType tTakeType) { brain().objects().attach_items(); }
CSE_ALifeDynamicObject* CSE_ALifeHumanAbstract::tpfGetBestDetector() { return (brain().objects().best_detector()); }
CSE_ALifeItemWeapon* CSE_ALifeHumanAbstract::tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower)
{
    return (brain().objects().best_weapon());
}

void CSE_ALifeHumanAbstract::on_register()
{
    inherited2::on_register();
    // because we need to load profile to setup graph vertex masks
    specific_character();
}

void CSE_ALifeHumanAbstract::on_unregister() { inherited2::on_unregister(); }
void CSE_ALifeHumanAbstract::spawn_supplies()
{
    specific_character();
    inherited1::spawn_supplies();
    inherited2::spawn_supplies();
}

void CSE_ALifeHumanAbstract::add_online(const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_online(update_registries);
    brain().on_switch_online();
}

void CSE_ALifeHumanAbstract::add_offline(
    const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_offline(saved_children, update_registries);
    brain().on_switch_offline();
}
