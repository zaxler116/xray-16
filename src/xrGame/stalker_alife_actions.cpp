////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_actions.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker alife action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_alife_actions.h"
#include "stalker_animation_manager.h"
#include "attachable_item.h"
#include "ai/stalker/ai_stalker.h"
#include "inventory_item.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "Inventory.h"
#include "WeaponMagazined.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "memory_manager.h"
#include "item_manager.h"
#include "sight_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "stalker_movement_manager_smart_cover.h"
#include "patrol_path_manager.h"
#include "sound_player.h"
#include "ai/stalker/ai_stalker_space.h"
#include "restricted_object.h"
#include "stalker_property_evaluators.h"
#include "PhraseDialog.h"
#include "PhraseDialogManager.h"
#include "AI_PhraseDialogManager.h"
#include "relation_registry.h"
#include "InventoryOwner.h"
#include "alife_object_registry.h"
#include "alife_communication_manager.h"
#include "alife_group_registry.h"
#include "alife_switch_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_object_registry.h"
#include "alife_communication_manager.h"
#include "alife_group_registry.h"
#include "alife_switch_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"

using namespace StalkerSpace;

#ifdef _DEBUG
//#	define STALKER_DEBUG_MODE
#endif

#ifdef STALKER_DEBUG_MODE
#include "attachable_item.h"
#endif

//////////////////////////////////////////////////////////////////////////
// CStalkerActionSquadGreeting
//////////////////////////////////////////////////////////////////////////

CStalkerActionSquadGreeting::CStalkerActionSquadGreeting(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name),
      m_greeting_target(0), m_greeting_start_time(0), m_dialog_said(false), m_approach_distance_sqr(4.f * 4.f)
{
}

void CStalkerActionSquadGreeting::initialize()
{
    inherited::initialize();

    m_greeting_start_time = Device.dwTimeGlobal;
    m_dialog_said = false;

    // find the nearest greeting target (other squad, friendly/neutral)
    m_greeting_target = 0;
    float best_dist_sqr = 100.f * 100.f;
    const CVisualMemoryManager::RAW_VISIBLES& visibles = object().memory().visual().raw_objects();
    for (CVisualMemoryManager::RAW_VISIBLES::const_iterator i = visibles.begin(); i != visibles.end(); ++i)
    {
        const CEntity* e = smart_cast<const CEntity*>(*i);
        if (!e || !e->g_Alive())
            continue;
        if (e->ID() == object().ID())
            continue;
        if (e->g_Squad() == object().g_Squad())
            continue;
        if (!smart_cast<const CAI_Stalker*>(e))
            continue;
        float dist_sqr = object().Position().distance_to_sqr(e->Position());
        if (dist_sqr > best_dist_sqr)
            continue;
        const CInventoryOwner* io = smart_cast<const CInventoryOwner*>(e);
        if (!io)
            continue;
        if (RELATION_REGISTRY().GetCommunityRelation(object().Community(), io->Community()) < 0)
            continue;
        m_greeting_target = e;
        best_dist_sqr = dist_sqr;
    }

    if (!m_greeting_target)
    {
        // no target - just stand
        object().movement().set_desired_position(0);
        object().movement().set_desired_direction(0);
        object().movement().set_body_state(eBodyStateStand);
        object().movement().set_movement_type(eMovementTypeStand);
        object().movement().set_mental_state(eMentalStateFree);
        object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
        return;
    }

    // face and stand toward the target
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_greeting_target), true));

    // holster / idle weapon
    if (!object().inventory().ActiveItem())
        object().CObjectHandler::set_goal(eObjectActionIdle);
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().inventory().ActiveItem());
}

void CStalkerActionSquadGreeting::finalize()
{
    inherited::finalize();

    object().movement().set_desired_position(0);
    object().sight().setup(SightManager::eSightTypePathDirection);

    if (!object().g_Alive())
        return;

    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
}

void CStalkerActionSquadGreeting::execute()
{
    inherited::execute();

    if (!object().g_Alive())
        return;

    if (!m_greeting_target)
        return;

    // stop if the target died or is gone
    if (!m_greeting_target->g_Alive())
    {
        m_greeting_target = 0;
        return;
    }

    u32 now = Device.dwTimeGlobal;
    u32 duration = 3000;

    // approach if far: walk toward target until within 4 m
    float dist_sqr = object().Position().distance_to_sqr(m_greeting_target->Position());
    if (dist_sqr > m_approach_distance_sqr)
    {
        object().movement().set_movement_type(eMovementTypeWalk);
        object().movement().set_body_state(eBodyStateStand);
        object().movement().set_path_type(MovementManager::ePathTypeGamePath);
        object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
        Fvector target_pos = m_greeting_target->Position();
        object().movement().set_desired_position(&target_pos);
        object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_greeting_target), true));
        // while approaching, don't count the greeting time
        m_greeting_start_time = now;
        return;
    }

    // close enough: stand, look, and greet
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_greeting_target), true));

    // say the greeting once, like NPC greets the actor
    if (!m_dialog_said)
    {
        m_dialog_said = true;
        CAI_PhraseDialogManager* other = smart_cast<CAI_PhraseDialogManager*>(const_cast<CEntity*>(m_greeting_target));
        if (other)
        {
            DIALOG_SHARED_PTR dlg(xr_new<CPhraseDialog>());
            dlg->Load("hello_dialog");
            object().InitDialog(other, dlg);
            object().SayPhrase(dlg, "0");
        }
    }

    if (now - m_greeting_start_time >= duration)
        m_greeting_target = 0; // done, planner will stop us next tick
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionTradeWithTrader
//////////////////////////////////////////////////////////////////////////

CStalkerActionTradeWithTrader::CStalkerActionTradeWithTrader(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name),
      m_trader_target(0), m_alife_human(0), m_alife_trader(0),
      m_trade_time(0), m_approach_distance_sqr(2.5f * 2.5f),
      m_trade_phase(eTradePhaseApproach), m_phase_start_time(0),
      m_current_item_go(0), m_animation_item_index(0), m_max_animation_items(5),
      m_hand_over_animation("zat_b14_give_artefact_act"), m_animation_duration_ms(3000),
      m_phase_timeout_ms(60000)
{
}

void CStalkerActionTradeWithTrader::initialize()
{
    inherited::initialize();

    m_trade_time = 0;
    m_trade_phase = eTradePhaseApproach;
    m_phase_start_time = 0;
    m_current_item_go = 0;
    m_current_item_section = "";
    m_animation_item_index = 0;
    m_give_items_p1.clear();
    m_receive_items_p1.clear();
    m_give_items_p2.clear();
    m_receive_items_p2.clear();

    // find the nearest ALife trader (same logic as the evaluator)
    m_trader_target = 0;
    m_alife_trader = 0;
    float best_dist_sqr = 30.f * 30.f;
    const CVisualMemoryManager::RAW_VISIBLES& visibles = object().memory().visual().raw_objects();
    for (CVisualMemoryManager::RAW_VISIBLES::const_iterator i = visibles.begin(); i != visibles.end(); ++i)
    {
        const CEntity* e = smart_cast<const CEntity*>(*i);
        if (!e || !e->g_Alive())
            continue;
        if (e->ID() == object().ID())
            continue;
        CSE_ALifeTrader* trader =
            smart_cast<CSE_ALifeTrader*>(ai().alife().objects().object(e->ID()));
        if (!trader)
            continue;
        float dist_sqr = object().Position().distance_to_sqr(e->Position());
        if (dist_sqr > best_dist_sqr)
            continue;
        m_trader_target = e;
        m_alife_trader = trader;
        best_dist_sqr = dist_sqr;
    }

    // the ALife server object of this stalker; it stays in the registry
    // while online, with empty children (the real inventory is client-side)
    m_alife_human =
        smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(object().ID()));

    if (!m_trader_target || !m_alife_human)
    {
        object().movement().set_desired_position(0);
        object().movement().set_desired_direction(0);
        object().movement().set_body_state(eBodyStateStand);
        object().movement().set_movement_type(eMovementTypeStand);
        object().movement().set_mental_state(eMentalStateFree);
        object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
        return;
    }

    // face the trader and stand
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_trader_target), true));

    if (!object().inventory().ActiveItem())
        object().CObjectHandler::set_goal(eObjectActionIdle);
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().inventory().ActiveItem());
}

void CStalkerActionTradeWithTrader::finalize()
{
    inherited::finalize();

    object().movement().set_desired_position(0);
    object().sight().setup(SightManager::eSightTypePathDirection);

    if (!object().g_Alive())
        return;

    // V1.1 - stop animation if in progress (NPC's phases play visible animations)
    if (m_trade_phase == eTradePhaseGiveItemsP1 || m_trade_phase == eTradePhaseReceiveItemsP1)
    {
        object().animation().clear_script_animations();
        if (m_current_item_go)
        {
            CInventoryItem* inv_item = smart_cast<CInventoryItem*>(m_current_item_go);
            if (inv_item)
            {
                CAttachableItem* attachable = inv_item->cast_attachable_item();
                if (attachable)
                    attachable->enable(false);
            }
        }
    }

    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
}

void CStalkerActionTradeWithTrader::start_hand_over_animation(CGameObject* item, LPCSTR section_id)
{
    m_current_item_go = item;
    m_current_item_section = section_id;
    m_phase_start_time = Device.dwTimeGlobal;

    // V1.1 - set weapon to idle
    if (object().inventory().ActiveItem())
        object().CObjectHandler::set_goal(eObjectActionIdle, object().inventory().ActiveItem());
    else
        object().CObjectHandler::set_goal(eObjectActionIdle);

    // V1.1 - start hand-over animation
    object().animation().add_script_animation(m_hand_over_animation, false, false);

    // V1.1 - enable attachable item (item appears in NPC's hand)
    if (item)
    {
        CInventoryItem* inv_item = smart_cast<CInventoryItem*>(item);
        if (inv_item)
        {
            CAttachableItem* attachable = inv_item->cast_attachable_item();
            if (attachable)
                attachable->enable(true);
        }
    }
}

void CStalkerActionTradeWithTrader::finish_hand_over_animation()
{
    // V1.1 - disable attachable item (item disappears from hand)
    if (m_current_item_go)
    {
        CInventoryItem* inv_item = smart_cast<CInventoryItem*>(m_current_item_go);
        if (inv_item)
        {
            CAttachableItem* attachable = inv_item->cast_attachable_item();
            if (attachable)
                attachable->enable(false);
        }
    }

    // V1.1 - stop animation
    object().animation().clear_script_animations();

    m_current_item_go = 0;
    m_current_item_section = "";
}

void CStalkerActionTradeWithTrader::mirror_client_to_alife()
{
    // move every client item to the ALife human (client inventory -> ALife)
    CInventoryItem* item = 0;
    while ((item = object().inventory().tpfGetObjectByIndex(0)) != 0)
    {
        CGameObject* go = smart_cast<CGameObject*>(item);
        if (!go)
            continue;
        CSE_ALifeInventoryItem* alife_item =
            smart_cast<CSE_ALifeInventoryItem*>(ai().alife().objects().object(go->ID()));
        if (!alife_item)
        {
            // no ALife twin - drop the item
            object().inventory().DropItem(go, true, true);
            continue;
        }
        // detach from the client inventory and attach to the ALife human
        object().inventory().DropItem(go, true, true);
        m_alife_human->attach(alife_item, true);
    }
    m_alife_human->m_dwMoney = object().get_money();
}

void CStalkerActionTradeWithTrader::mirror_alife_to_client()
{
    // mirror the result back: ALife children -> client inventory
    u32 money = m_alife_human->m_dwMoney;
    ALife::OBJECT_VECTOR taken;
    ALife::OBJECT_IT I = m_alife_human->children.begin();
    ALife::OBJECT_IT E = m_alife_human->children.end();
    for (; I != E; ++I)
    {
        CSE_ALifeInventoryItem* alife_item =
            smart_cast<CSE_ALifeInventoryItem*>(ai().alife().objects().object(*I));
        if (!alife_item)
            continue;
        CGameObject* go = smart_cast<CGameObject*>(ai().alife().objects().object(*I));
        if (!go)
            continue;
        // take the item into the client inventory, then detach it from the ALife human
        object().inventory().Take(go, true, false);
        taken.push_back(*I);
    }
    for (ALife::OBJECT_VECTOR::iterator J = taken.begin(); J != taken.end(); ++J)
    {
        CSE_ALifeInventoryItem* alife_item =
            smart_cast<CSE_ALifeInventoryItem*>(ai().alife().objects().object(*J));
        m_alife_human->detach(alife_item, 0, true, false);
    }
    object().set_money(money, true);
}

void CStalkerActionTradeWithTrader::compute_trade_plan()
{
    // snapshot the item types the NPC will give: the ALife children that are
    // client-owned (their CGameObject is in the registry, not a trader's stock)
    {
        ALife::OBJECT_IT I = m_alife_human->children.begin();
        ALife::OBJECT_IT E = m_alife_human->children.end();
        for (; I != E; ++I)
        {
            CSE_ALifeInventoryItem* item =
                smart_cast<CSE_ALifeInventoryItem*>(ai().alife().objects().object(*I));
            if (!item)
                continue;
            CGameObject* go = smart_cast<CGameObject*>(ai().alife().objects().object(*I));
            if (!go)
                continue;
            // the item's type is the client-side inventory item's section id
            CInventoryItem* inv = smart_cast<CInventoryItem*>(go);
            if (!inv)
                continue;
            // dedup by section (same item type = one hand-over animation)
            bool found = false;
            for (TRADE_ANIM_ITEMS::iterator K = m_give_items_p1.begin(); K != m_give_items_p1.end(); ++K)
            {
                if (K->m_section == inv->m_section_id)
                {
                    found = true;
                    break;
                }
            }
            if (found)
                continue;
            STradeAnimItem entry;
            entry.m_go = go;
            entry.m_section = inv->m_section_id;
            m_give_items_p1.push_back(entry);
        }
    }
    // cap: max N distinct item types per participant
    while (int(m_give_items_p1.size()) > m_max_animation_items)
        m_give_items_p1.resize(m_max_animation_items);

    // V2.1 - the trader's side (what it gives / receives) is filled in execute()
    // after communicate_with_customer, from the diff of the human's children.
}

void CStalkerActionTradeWithTrader::apply_trade_item(int index, bool giving)
{
    // V1.2 - no-op: the trade itself is monolithic (communicate_with_customer);
    // the per-item calls are only the presentational hand-over animations
    (void)index;
    (void)giving;
}

void CStalkerActionTradeWithTrader::execute()
{
    inherited::execute();

    if (!object().g_Alive())
        return;

    if (!m_trader_target || !m_alife_human || !m_alife_trader)
        return;

    if (!m_trader_target->g_Alive())
    {
        m_trader_target = 0;
        return;
    }

    // approach the trader until close enough
    float dist_sqr = object().Position().distance_to_sqr(m_trader_target->Position());
    if (dist_sqr > m_approach_distance_sqr)
    {
        object().movement().set_movement_type(eMovementTypeWalk);
        object().movement().set_body_state(eBodyStateStand);
        object().movement().set_path_type(MovementManager::ePathTypeGamePath);
        object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
        Fvector target_pos = m_trader_target->Position();
        object().movement().set_desired_position(&target_pos);
        object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_trader_target), true));
        return;
    }

    // close enough: stand and face
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeObject, smart_cast<const CGameObject*>(m_trader_target), true));

    // V2.1 - trade state machine:
    //  Approach -> Compute (mirror in + plan + communicate) ->
    //  GiveItemsP1 (NPC hands over what it sells, with animations) ->
    //  ReceiveItemsP1 (NPC receives what it buys, with animations) ->
    //  GiveItemsP2 (trader hands over, no animation - timing only) ->
    //  ReceiveItemsP2 (trader receives, no animation - timing only) ->
    //  MirrorBack -> Done
    if (!m_trade_time)
        m_trade_phase = eTradePhaseCompute;

    switch (m_trade_phase)
    {
    case eTradePhaseCompute:
    {
        m_trade_time = Device.dwTimeGlobal;
        m_phase_start_time = Device.dwTimeGlobal;

        // 1. mirror the client inventory onto the ALife human (so the trade has real data)
        mirror_client_to_alife();

        // 2. snapshot what the NPC will give (client-owned children, by type)
        compute_trade_plan();

        // 3. V2.2: pure computation of the trade plan (no inventory changes yet),
        //    then build the per-participant animation lists from the plan.
        STradePlan trade_plan;
        const_cast<CALifeSimulator&>(ai().alife()).compute_trade(m_alife_human, m_alife_trader, trade_plan);

        // what the NPC receives (and the trader gives): dedup by item type
        for (ALife::ITEM_P_VECTOR::const_iterator R = trade_plan.customer_receives.begin();
             R != trade_plan.customer_receives.end(); ++R)
        {
            CGameObject* go = smart_cast<CGameObject*>((*R)->base());
            if (!go)
                continue;
            CInventoryItem* inv = smart_cast<CInventoryItem*>(go);
            if (!inv)
                continue;
            bool found = false;
            for (TRADE_ANIM_ITEMS::iterator K = m_receive_items_p1.begin(); K != m_receive_items_p1.end(); ++K)
            {
                if (K->m_section == inv->m_section_id)
                {
                    found = true;
                    break;
                }
            }
            if (found)
                continue;
            STradeAnimItem entry;
            entry.m_go = go;
            entry.m_section = inv->m_section_id;
            m_receive_items_p1.push_back(entry);
            m_give_items_p2.push_back(entry);
        }
        while (int(m_receive_items_p1.size()) > m_max_animation_items)
            m_receive_items_p1.resize(m_max_animation_items);
        while (int(m_give_items_p2.size()) > m_max_animation_items)
            m_give_items_p2.resize(m_max_animation_items);

        // 4. what the trader receives = what the NPC gave (same item types)
        for (TRADE_ANIM_ITEMS::iterator K = m_give_items_p1.begin(); K != m_give_items_p1.end(); ++K)
        {
            STradeAnimItem entry;
            entry.m_go = K->m_go;
            entry.m_section = K->m_section;
            m_receive_items_p2.push_back(entry);
        }
        while (int(m_receive_items_p2.size()) > m_max_animation_items)
            m_receive_items_p2.resize(m_max_animation_items);

        // 5. apply the plan to the ALife inventories
        const_cast<CALifeSimulator&>(ai().alife()).apply_trade(m_alife_human, m_alife_trader, trade_plan);

        m_animation_item_index = 0;
        if (int(m_give_items_p1.size()) > 0)
            m_trade_phase = eTradePhaseGiveItemsP1;
        else if (int(m_receive_items_p1.size()) > 0)
            m_trade_phase = eTradePhaseReceiveItemsP1;
        else if (int(m_give_items_p2.size()) > 0)
            m_trade_phase = eTradePhaseGiveItemsP2;
        else
            m_trade_phase = eTradePhaseMirrorBack;
        return;
    }

    case eTradePhaseGiveItemsP1:
    case eTradePhaseReceiveItemsP1:
    {
        // NPC's phases: play the hand-over animation, timed by m_animation_duration_ms
        // hard cap: timeout per phase
        if (m_trade_time && Device.dwTimeGlobal - m_phase_start_time > m_phase_timeout_ms)
        {
            finish_hand_over_animation();
            m_animation_item_index = 0;
            if (m_trade_phase == eTradePhaseGiveItemsP1)
                m_trade_phase = eTradePhaseReceiveItemsP1;
            else
                m_trade_phase = eTradePhaseGiveItemsP2;
            m_phase_start_time = Device.dwTimeGlobal;
            return;
        }

        const TRADE_ANIM_ITEMS& list = current_list();

        if (m_current_item_go == 0)
        {
            // start the next hand-over animation
            if (m_animation_item_index < int(list.size()))
            {
                const STradeAnimItem& entry = list[m_animation_item_index];
                start_hand_over_animation(entry.m_go, entry.m_section.c_str());
            }
            else
            {
                // all animations of this phase done
                on_animation_phase_complete();
                return;
            }
        }
        else if (Device.dwTimeGlobal - m_phase_start_time >= m_animation_duration_ms)
        {
            finish_hand_over_animation();
            ++m_animation_item_index;
            if (m_animation_item_index >= int(list.size()))
                on_animation_phase_complete();
        }
        return;
    }

    case eTradePhaseGiveItemsP2:
    case eTradePhaseReceiveItemsP2:
    {
        // Trader's phases: no animation (the trader doesn't play the hand-over),
        // just a short stand so the sequence reads naturally.
        if (m_trade_time && Device.dwTimeGlobal - m_phase_start_time > m_phase_timeout_ms)
        {
            m_trade_phase = (m_trade_phase == eTradePhaseGiveItemsP2) ? eTradePhaseReceiveItemsP2
                                                                      : eTradePhaseMirrorBack;
            m_phase_start_time = Device.dwTimeGlobal;
            return;
        }

        if (Device.dwTimeGlobal - m_phase_start_time >= m_animation_duration_ms)
        {
            m_trade_phase = (m_trade_phase == eTradePhaseGiveItemsP2) ? eTradePhaseReceiveItemsP2
                                                                      : eTradePhaseMirrorBack;
            m_phase_start_time = Device.dwTimeGlobal;
            m_animation_item_index = 0;
        }
        return;
    }

    case eTradePhaseMirrorBack:
        // mirror the result back: ALife children -> client inventory
        mirror_alife_to_client();
        m_trade_phase = eTradePhaseDone;
        return;

    case eTradePhaseDone:
        // nothing to do; the action stays until the planner finalizes it
        return;
    }
}

bool CStalkerActionTradeWithTrader::in_animation_phase() const
{
    return m_trade_phase == eTradePhaseGiveItemsP1 || m_trade_phase == eTradePhaseReceiveItemsP1;
}

const CStalkerActionTradeWithTrader::TRADE_ANIM_ITEMS& CStalkerActionTradeWithTrader::current_list() const
{
    switch (m_trade_phase)
    {
    case eTradePhaseGiveItemsP1:
        return m_give_items_p1;
    case eTradePhaseReceiveItemsP1:
        return m_receive_items_p1;
    case eTradePhaseGiveItemsP2:
        return m_give_items_p2;
    case eTradePhaseReceiveItemsP2:
        return m_receive_items_p2;
    }
    return m_give_items_p1;
}

int CStalkerActionTradeWithTrader::current_list_size() const
{
    return int(current_list().size());
}

void CStalkerActionTradeWithTrader::on_animation_phase_complete()
{
    // next phase in the V2.1 sequence
    if (m_trade_phase == eTradePhaseGiveItemsP1)
        m_trade_phase = (int(m_receive_items_p1.size()) > 0) ? eTradePhaseReceiveItemsP1
                                                            : (int(m_give_items_p2.size()) > 0 ? eTradePhaseGiveItemsP2
                                                                                               : eTradePhaseMirrorBack);
    else // eTradePhaseReceiveItemsP1
        m_trade_phase = (int(m_give_items_p2.size()) > 0) ? eTradePhaseGiveItemsP2
                                                         : eTradePhaseMirrorBack;
    m_phase_start_time = Device.dwTimeGlobal;
    m_animation_item_index = 0;
}
CStalkerActionNoALife::CStalkerActionNoALife(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name)
{
}

void CStalkerActionNoALife::initialize()
{
    inherited::initialize();
#ifndef STALKER_DEBUG_MODE
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeGamePath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeCover, false, true));

    m_stop_weapon_handling_time = Device.dwTimeGlobal;
    if (object().inventory().ActiveItem() && object().best_weapon() &&
        (object().inventory().ActiveItem()->object().ID() == object().best_weapon()->object().ID()))
        m_stop_weapon_handling_time += ::Random32.random(30000) + 30000;

#else
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
    object().CObjectHandler::set_goal(
        eObjectActionFire1, object().inventory().ItemFromSlot(INV_SLOT_2), 0, 1, 2500, 3000);
//	object().movement().patrol().set_path		("way_0000",ePatrolStartTypeNearest);
#endif
}

void CStalkerActionNoALife::finalize()
{
    inherited::finalize();

    object().movement().set_desired_position(0);

    if (!object().g_Alive())
        return;

    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
}

void CStalkerActionNoALife::execute()
{
    inherited::execute();
#ifndef STALKER_DEBUG_MODE
    object().sound().play(eStalkerSoundHumming, 60000, 10000);
    if (Device.dwTimeGlobal >= m_stop_weapon_handling_time)
        if (!object().best_weapon())
            object().CObjectHandler::set_goal(eObjectActionIdle);
        else
            object().CObjectHandler::set_goal(eObjectActionStrapped, object().best_weapon());
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().best_weapon());
#else
//	object().movement().set_movement_type		(eMovementTypeRun);
#endif
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGatherItems
//////////////////////////////////////////////////////////////////////////

CStalkerActionGatherItems::CStalkerActionGatherItems(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionGatherItems::initialize()
{
    inherited::initialize();

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
    if (!object().inventory().ActiveItem())
        object().CObjectHandler::set_goal(eObjectActionIdle);
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().inventory().ActiveItem());

    IGameObject const* const selected = object().memory().item().selected();

    typedef CAI_Stalker::ignored_touched_objects_type ignored_touched_objects_type;
    ignored_touched_objects_type& ignored_touched_objects = m_object->ignored_touched_objects();
    ignored_touched_objects_type::iterator i =
        std::find(ignored_touched_objects.begin(), ignored_touched_objects.end(), selected);
    if (i == ignored_touched_objects.end())
        return;

    ignored_touched_objects.erase(i);

    m_object->generate_take_event(selected);
}

void CStalkerActionGatherItems::finalize()
{
    inherited::finalize();

    object().sight().setup(SightManager::eSightTypePathDirection);

    object().movement().set_desired_position(0);

    if (!object().g_Alive())
        return;

    object().sound().set_sound_mask(0);
}

void CStalkerActionGatherItems::execute()
{
    inherited::execute();

    if (!object().memory().item().selected())
        return;

    u32 level_vertex_id = object().memory().item().selected()->ai_location().level_vertex_id();
    //	if (object().movement().restrictions().accessible(level_vertex_id)) {
    object().movement().set_level_dest_vertex(level_vertex_id);
    object().movement().set_desired_position(&object().memory().item().selected()->Position());
    //	}
    //	else {
    //		object().movement().set_nearest_accessible_position	(
    //			object().memory().item().selected()->Position(),
    //			level_vertex_id
    //		);
    //	}

    object().sight().setup(SightManager::eSightTypePosition, &object().memory().item().selected()->Position());
}
