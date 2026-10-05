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
        eTradePhaseGiveItems,
        eTradePhaseReceiveItems,
        eTradePhaseMirrorBack,
        eTradePhaseDone
    };

    const CEntity* m_trader_target;
    CSE_ALifeHumanAbstract* m_alife_human;
    CSE_ALifeTrader* m_alife_trader;
    u32 m_trade_time;
    float m_approach_distance_sqr;

    // V1.1 - animation state
    ETradePhase m_trade_phase;
    u32 m_animation_start_time;
    CGameObject* m_current_item_go;
    shared_str m_current_item_section;
    int m_animation_item_index;
    int m_max_animation_items;

    // V1.1 - config
    LPCSTR m_hand_over_animation;
    u32 m_animation_duration_ms;

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
