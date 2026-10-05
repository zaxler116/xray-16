////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_communication_manager.h
//	Created 	: 03.09.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife communication manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_simulator_base.h"

class CSE_ALifeSchedulable;
class CSE_ALifeHumanAbstract;
class CSE_ALifeTrader;
class CSE_ALifeTraderAbstract;
class CSE_ALifeItemPDA;

// V2.2 - pure trade plan: what each side gives/receives and the money delta.
// Computed by compute_trade() without touching inventories; applied by
// apply_trade() at the moment the hand-over animation plays.
struct STradePlan {
  ALife::ITEM_P_VECTOR
      customer_gives; // items the customer hands to the trader (sold)
  ALife::ITEM_P_VECTOR
      customer_receives; // items the trader hands to the customer (bought back)
  u32 money_customer_end;
  u32 money_customer_start;
  CSE_ALifeItemPDA *original_pda;
  void clear() {
    customer_gives.clear();
    customer_receives.clear();
    money_customer_end = 0;
    money_customer_start = 0;
    original_pda = 0;
  }
};

// #pragma todo("Dima to Dima : Be attentive with this speed optimization - it
// doesn't suit to the OOP paradigm!")
#define FAST_OWNERSHIP

class CALifeCommunicationManager : public virtual CALifeSimulatorBase {
  /**
  protected:
      typedef CALifeSimulatorBase inherited;

  private:
      enum {
          MAX_STACK_DEPTH					= u32(128),
          SUM_COUNT_THRESHOLD				= u32(30),
      };

      // temporary buffers for trading
      ALife::ITEM_P_VECTOR				m_tpItems1;
      ALife::ITEM_P_VECTOR				m_tpItems2;
      ALife::OBJECT_VECTOR				m_tpBlockedItems1;
      ALife::OBJECT_VECTOR				m_tpBlockedItems2;

      ALife::ITEM_P_VECTOR				m_tpTrader1;
      ALife::ITEM_P_VECTOR				m_tpTrader2;
      ALife::INT_VECTOR					m_tpSums1;
      ALife::INT_VECTOR					m_tpSums2;

      ALife::SSumStackCell
  m_tpStack1[MAX_STACK_DEPTH]; ALife::SSumStackCell
  m_tpStack2[MAX_STACK_DEPTH];
  **/

  // Stage 4.2: live trading buffers (fields of the commented block above)
  static const u32 MAX_STACK_DEPTH = u32(128);
  static const u32 SUM_COUNT_THRESHOLD = u32(30);

  ALife::ITEM_P_VECTOR m_tpItems1;
  ALife::ITEM_P_VECTOR m_tpItems2;
  ALife::OBJECT_VECTOR m_tpBlockedItems1;
  ALife::OBJECT_VECTOR m_tpBlockedItems2;
  ALife::ITEM_P_VECTOR m_tpTrader1;
  ALife::ITEM_P_VECTOR m_tpTrader2;
  ALife::INT_VECTOR m_tpSums1;
  ALife::INT_VECTOR m_tpSums2;
  ALife::SSumStackCell m_tpStack1[MAX_STACK_DEPTH];
  ALife::SSumStackCell m_tpStack2[MAX_STACK_DEPTH];

  /**
  protected:
              u32			dwfComputeItemCost
  (ALife::ITEM_P_VECTOR		&tpItemVector); void
  vfRunFunctionByIndex			(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract, ALife::OBJECT_VECTOR			&tpBlockedItems,
  ALife::ITEM_P_VECTOR	&tpItems, int i,
  int						&j); void
  vfAssignItemParents				(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract, int
  iItemCount); void		vfAppendBlockedItems
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract,
  ALife::OBJECT_VECTOR			&tpObjectVector1,
  ALife::OBJECT_VECTOR	&tpObjectVector2, int
  l_iItemCount1); void		vfAttachOwnerItems
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract,
  ALife::ITEM_P_VECTOR			&tpItemVector,
  ALife::ITEM_P_VECTOR	&tpOwnItems); void		vfRestoreItems
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract,
  ALife::ITEM_P_VECTOR			&tpItemVector);
              int			ifComputeBalance
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract,
  ALife::ITEM_P_VECTOR			&tpItemVector);
              void		vfFillTraderVector
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract, int
  iItemCount,						ALife::ITEM_P_VECTOR
  &tpItemVector); void		vfGenerateSums
  (ALife::ITEM_P_VECTOR		&tpTrader, ALife::INT_VECTOR
  &tpSums); bool		bfGetItemIndexes
  (ALife::ITEM_P_VECTOR		&tpTrader, int
  iSum1, ALife::INT_VECTOR		&tpIndexes, ALife::SSumStackCell
  *tpStack,				int
  iStartI,					int iStackPointer); bool
  bfCheckForInventoryCapacity		(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract1, ALife::ITEM_P_VECTOR			&tpTrader1,
  ALife::INT_VECTOR		&tpIndexes1, CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract2,	ALife::ITEM_P_VECTOR
  &tpTrader2, ALife::INT_VECTOR		&tpIndexes2); bool
  bfCheckForInventoryCapacity		(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract1, ALife::ITEM_P_VECTOR			&tpTrader1,
  int				iSum1, int
  iMoney1,				CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract2, ALife::ITEM_P_VECTOR	&tpTrader2,		int
  iSum2,		int iMoney2, int iBalance); bool
  bfCheckForTrade					(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract1, ALife::ITEM_P_VECTOR			&tpTrader1,
  ALife::INT_VECTOR		&tpSums1, int
  iMoney1,				CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract2, ALife::ITEM_P_VECTOR	&tpTrader2,
  ALife::INT_VECTOR	&tpSums2,	int iMoney2, int iBalance); bool
  bfCheckIfCanNullTradersBalance	(CSE_ALifeHumanAbstract
  *tpALifeHumanAbstract1, CSE_ALifeHumanAbstract	*tpALifeHumanAbstract2,
  int				iItemCount1,				int
  iItemCount2,			int
  iBalance); void		vfPerformTrading
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract1,
  CSE_ALifeHumanAbstract	*tpALifeHumanAbstract2);
              void		vfPerformCommunication			();

  #ifdef FAST_OWNERSHIP
              void		vfAttachGatheredItems
  (CSE_ALifeTraderAbstract	*tpALifeTraderAbstract1, CSE_ALifeTraderAbstract
  *tpALifeTraderAbstract2,		ALife::OBJECT_VECTOR
  &tpObjectVector); #else void		vfAttachGatheredItems
  (CSE_ALifeTraderAbstract	*tpALifeTraderAbstract1, ALife::OBJECT_VECTOR
  &tpObjectVector); #endif #ifdef DEBUG void		vfPrintItems
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract,
  ALife::ITEM_P_VECTOR			&tpItemVector);
              void		vfPrintItems
  (CSE_ALifeHumanAbstract		*tpALifeHumanAbstract); #endif
  **/
public:
  CALifeCommunicationManager(IPureServer *server, LPCSTR section);
  // Stage 4.1: live method (2003 logic, restored in 4.7)
  void vfPerformCommunication();
  // Stage 4.7: live (2003 logic, adapted to the 2005 API)
  void vfPerformTrading(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
                        CSE_ALifeHumanAbstract *tpALifeHumanAbstract2);
  // N.4: only surplus items (CALifeHumanObjectHandler::item_is_keepable)
  // of a human are offered into the trade pool
  void append_keepable_items(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                             ALife::ITEM_P_VECTOR &tpItemList);
  void vfAppendBlockedItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                            ALife::OBJECT_VECTOR &tpObjectVector1,
                            ALife::OBJECT_VECTOR &tpObjectVector2,
                            int l_iItemCount1);
  // Stage 4.3: live trading helpers (2003 logic, adapted to the 2005 API)
  u32 dwfComputeItemCost(ALife::ITEM_P_VECTOR &tpItemVector);
  void vfRunFunctionByIndex(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                            ALife::OBJECT_VECTOR &tpBlockedItems,
                            ALife::ITEM_P_VECTOR &tpItems, int i, int &j);
  void vfAssignItemParents(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                           int iItemCount);
  void vfAttachOwnerItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                          ALife::ITEM_P_VECTOR &tpItemVector,
                          ALife::ITEM_P_VECTOR &tpOwnItems);
  int ifComputeBalance(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                       ALife::ITEM_P_VECTOR &tpItemVector);
  void vfRestoreItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                      ALife::ITEM_P_VECTOR &tpItemVector);
  // Stage 4.5: live (2003 logic)
  void vfFillTraderVector(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                          int iItemCount, ALife::ITEM_P_VECTOR &tpItemVector);
  void vfGenerateSums(ALife::ITEM_P_VECTOR &tpTrader,
                      ALife::INT_VECTOR &tpSums);
  bool bfGetItemIndexes(ALife::ITEM_P_VECTOR &tpTrader, int iSum1,
                        ALife::INT_VECTOR &tpIndexes,
                        ALife::SSumStackCell *tpStack, int iStartI,
                        int iStackPointer);
  // Stage 4.6: live trade checks (2003 logic)
  bool bfCheckForInventoryCapacity(
      CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
      ALife::ITEM_P_VECTOR &tpTrader1, ALife::INT_VECTOR &tpIndexes1,
      CSE_ALifeHumanAbstract *tpALifeHumanAbstract2,
      ALife::ITEM_P_VECTOR &tpTrader2, ALife::INT_VECTOR &tpIndexes2);
  bool bfCheckForInventoryCapacity(
      CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
      ALife::ITEM_P_VECTOR &tpTrader1, int iSum1, int iMoney1,
      CSE_ALifeHumanAbstract *tpALifeHumanAbstract2,
      ALife::ITEM_P_VECTOR &tpTrader2, int iSum2, int iMoney2, int iBalance);
  bool bfCheckForTrade(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
                       ALife::ITEM_P_VECTOR &tpTrader1,
                       ALife::INT_VECTOR &tpSums1, int iMoney1,
                       CSE_ALifeHumanAbstract *tpALifeHumanAbstract2,
                       ALife::ITEM_P_VECTOR &tpTrader2,
                       ALife::INT_VECTOR &tpSums2, int iMoney2, int iBalance);
  bool
  bfCheckIfCanNullTradersBalance(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
                                 CSE_ALifeHumanAbstract *tpALifeHumanAbstract2,
                                 int iItemCount1, int iItemCount2,
                                 int iBalance);
#if defined(DEBUG) || defined(DEBUG_ALIFE)
  void vfPrintItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                    ALife::ITEM_P_VECTOR &tpItemVector);
  void vfPrintItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract);
#endif
  // Stage 4.4: live (2003 logic, m_tPreviousParentID -> base()->ID_Parent)
#ifdef FAST_OWNERSHIP
  void vfAttachGatheredItems(CSE_ALifeTraderAbstract *tpALifeTraderAbstract1,
                             CSE_ALifeTraderAbstract *tpALifeTraderAbstract2,
                             ALife::OBJECT_VECTOR &tpObjectVector);
#else
  void vfAttachGatheredItems(CSE_ALifeTraderAbstract *tpALifeTraderAbstract1,
                             ALife::OBJECT_VECTOR &tpObjectVector);
#endif
  // Stage 4.8: live (2003 logic, adapted to the 2005 API)
  void communicate_with_customer(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                                 CSE_ALifeTrader *tpALifeTrader);
  // V2.2: pure trade plan computation and application (split of
  // communicate_with_customer)
  void compute_trade(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                     CSE_ALifeTrader *tpALifeTrader, STradePlan &plan);
  void apply_trade(CSE_ALifeHumanAbstract *tpALifeHumanAbstract,
                   CSE_ALifeTrader *tpALifeTrader, const STradePlan &plan);
};
