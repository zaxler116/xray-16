////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_actions.h
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker alife action classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_base_action.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGatherItems
//////////////////////////////////////////////////////////////////////////

class CStalkerActionGatherItems : public CStalkerActionBase
{
protected:
    typedef CStalkerActionBase inherited;

public:
    CStalkerActionGatherItems(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionSquadGreeting
//////////////////////////////////////////////////////////////////////////

class CStalkerActionSquadGreeting : public CStalkerActionBase
{
protected:
    typedef CStalkerActionBase inherited;

    const CEntity* m_greeting_target;
    u32 m_greeting_start_time;
    bool m_dialog_said;
    float m_approach_distance_sqr;

public:
    CStalkerActionSquadGreeting(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionTradeWithTrader
//////////////////////////////////////////////////////////////////////////

class CStalkerActionTradeWithTrader : public CStalkerActionBase
{
protected:
    typedef CStalkerActionBase inherited;

    enum ETradePhase
    {
        eTradePhaseApproach,
        eTradePhaseCompute,
        // V2.1 - participant 1 (the NPC): hand over the items it sells
        eTradePhaseGiveItemsP1,
        // V2.1 - participant 1 (the NPC): receive the items it buys
        eTradePhaseReceiveItemsP1,
        // V2.1 - participant 2 (the trader): hand over the items it sells
        eTradePhaseGiveItemsP2,
        // V2.1 - participant 2 (the trader): receive the items it buys
        eTradePhaseReceiveItemsP2,
        eTradePhaseMirrorBack,
        eTradePhaseDone
    };

    const CEntity* m_trader_target;
    CSE_ALifeHumanAbstract* m_alife_human;
    CSE_ALifeTrader* m_alife_trader;
    u32 m_trade_time;
    float m_approach_distance_sqr;
    bool m_computed; // V4.1 - true after compute_trade/apply_trade, used for rollback in finalize()

    // V1.1 - animation state
    ETradePhase m_trade_phase;
    u32 m_phase_start_time;
    CGameObject* m_current_item_go;
    shared_str m_current_item_section;
    int m_animation_item_index;
    int m_max_animation_items;

    // V1.1 - config
    LPCSTR m_hand_over_animation;
    u32 m_animation_duration_ms;
    u32 m_phase_timeout_ms;

    // V1.2/V2.1 - trade plan: what each participant gives/receives, by distinct item type
    struct STradeAnimItem
    {
        CGameObject* m_go;
        shared_str m_section;
    };
    typedef xr_vector<STradeAnimItem> TRADE_ANIM_ITEMS;
    // participant 1 = the NPC (client side, visible animations)
    TRADE_ANIM_ITEMS m_give_items_p1;
    TRADE_ANIM_ITEMS m_receive_items_p1;
    // participant 2 = the trader (server side, no animations, timing only)
    TRADE_ANIM_ITEMS m_give_items_p2;
    TRADE_ANIM_ITEMS m_receive_items_p2;

public:
    CStalkerActionTradeWithTrader(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();

private:
    void start_hand_over_animation(CGameObject* item, LPCSTR section_id);
    void finish_hand_over_animation();
    void compute_trade_plan();
    void apply_trade_item(int index, bool giving);
    void mirror_client_to_alife();
    void mirror_alife_to_client();
    // V2.1 - animation phases
    bool in_animation_phase() const;
    const TRADE_ANIM_ITEMS& current_list() const;
    int current_list_size() const;
    void on_animation_phase_complete();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionTradeWithSquad (V2.4)
//////////////////////////////////////////////////////////////////////////

class CStalkerActionTradeWithSquad : public CStalkerActionBase
{
protected:
    typedef CStalkerActionBase inherited;

    enum ETradePhase
    {
        eTradePhaseApproach,
        eTradePhaseCompute,
        // V2.4 - participant 1 (the NPC): hand over the items it gives, with animations
        eTradePhaseGiveItemsP1,
        // V2.4 - participant 1 (the NPC): receive the items it gets, with animations
        eTradePhaseReceiveItemsP1,
        // V2.4 - participant 2 (the other squad member): no animations, timing only
        eTradePhaseGiveItemsP2,
        eTradePhaseReceiveItemsP2,
        eTradePhaseMirrorBack,
        eTradePhaseDone
    };

    const CEntity* m_partner_target;
    CSE_ALifeHumanAbstract* m_alife_human;
    CSE_ALifeHumanAbstract* m_alife_partner;
    u32 m_trade_time;
    float m_approach_distance_sqr;
    bool m_computed; // V4.1 - true after vfPerformTrading, used for rollback in finalize()

    ETradePhase m_trade_phase;
    u32 m_phase_start_time;
    CGameObject* m_current_item_go;
    shared_str m_current_item_section;
    int m_animation_item_index;
    int m_max_animation_items;

    LPCSTR m_hand_over_animation;
    u32 m_animation_duration_ms;
    u32 m_phase_timeout_ms;

    struct STradeAnimItem
    {
        CGameObject* m_go;
        shared_str m_section;
    };
    typedef xr_vector<STradeAnimItem> TRADE_ANIM_ITEMS;
    TRADE_ANIM_ITEMS m_give_items_p1;
    TRADE_ANIM_ITEMS m_receive_items_p1;
    TRADE_ANIM_ITEMS m_give_items_p2;
    TRADE_ANIM_ITEMS m_receive_items_p2;

public:
    CStalkerActionTradeWithSquad(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();

private:
    void start_hand_over_animation(CGameObject* item, LPCSTR section_id);
    void finish_hand_over_animation();
    void mirror_client_to_alife(CAI_Stalker* npc, CSE_ALifeHumanAbstract* alife_human);
    void mirror_alife_to_client(CAI_Stalker* npc, CSE_ALifeHumanAbstract* alife_human);
    bool in_animation_phase() const;
    const TRADE_ANIM_ITEMS& current_list() const;
    void on_animation_phase_complete();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionNoALife
//////////////////////////////////////////////////////////////////////////

class CStalkerActionNoALife : public CStalkerActionBase
{
protected:
    typedef CStalkerActionBase inherited;

protected:
    u32 m_stop_weapon_handling_time;

public:
    CStalkerActionNoALife(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};
