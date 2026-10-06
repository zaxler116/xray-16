////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_planner.h
//	Created 	: 25.03.2004
//  Modified 	: 27.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker ALife planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action_script.h"
#include "stalker_property_evaluators.h"

class CAI_Stalker;

class CStalkerALifePlanner : public CActionPlannerActionScript<CAI_Stalker>
{
private:
    typedef CActionPlannerActionScript<CAI_Stalker> inherited;

    // V5 - pointers to the trade/greeting evaluators (to stamp cooldowns)
    CStalkerPropertyEvaluatorSquadGreeting* m_evaluator_squad_greeting;
    CStalkerPropertyEvaluatorTradeWithTrader* m_evaluator_trade_with_trader;
    CStalkerPropertyEvaluatorTradeWithSquad* m_evaluator_trade_with_squad;

public:
    CStalkerALifePlanner(CAI_Stalker* object = 0, LPCSTR action_name = "");
    virtual ~CStalkerALifePlanner();
    virtual void setup(CAI_Stalker* object, CPropertyStorage* storage);
    void add_evaluators();
    void add_actions();

    // V5 - getters for the trade/greeting evaluators (to stamp cooldowns)
    CStalkerPropertyEvaluatorSquadGreeting* evaluator_squad_greeting() const { return m_evaluator_squad_greeting; }
    CStalkerPropertyEvaluatorTradeWithTrader* evaluator_trade_with_trader() const { return m_evaluator_trade_with_trader; }
    CStalkerPropertyEvaluatorTradeWithSquad* evaluator_trade_with_squad() const { return m_evaluator_trade_with_squad; }
};
