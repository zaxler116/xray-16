////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_brain.h
//	Created 	: 06.10.2005
//  Modified 	: 06.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human brain class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_monster_brain.h"

class CALifeHumanObjectHandler;
class CSE_ALifeHumanAbstract;

class CALifeHumanBrain : public CALifeMonsterBrain {
private:
  typedef CALifeMonsterBrain inherited;

public:
  typedef CSE_ALifeHumanAbstract object_type;
  typedef CALifeHumanObjectHandler object_handler_type;

private:
  object_type *m_object;
  object_handler_type *m_object_handler;

  // old not yet obsolete stuff
public:
  svector<char, 5> m_cpEquipmentPreferences;
  svector<char, 4> m_cpMainWeaponPreferences;

  // old, to be obsolete
public:
  u32 m_dwTotalMoney;

public:
  CALifeHumanBrain(object_type *object);
  virtual ~CALifeHumanBrain();

public:
  void on_state_write(NET_Packet &packet);
  void on_state_read(NET_Packet &packet);

  // Stage 1.2: route the human's attack through the object. NOTE: this is a
  // temporary bridge - CSE_ALifeHumanAbstract::bfPerformAttack() currently
  // delegates back to brain().perform_attack(), so this pair recurses until
  // Stage 2.3 replaces CSE_ALifeHumanAbstract::bfPerformAttack() with the real
  // 2003 logic (which works on object state and does not call back into the
  // brain). Until then the human never actually attacks, so this is dead code.
  virtual bool perform_attack();

  // Stage 1.3: human meet-action logic from the 2003 code
  // (CSE_ALifeHumanAbstract::tfGetActionType in alife_human_brain_save.h).
  // Unlike the monster brain, a human meeting a *friend* returns Interact
  // (the 2003 human/monster distinction); other combat types fall through to
  // Attack.
  virtual ALife::EMeetActionType
  action_type(CSE_ALifeSchedulable *tpALifeSchedulable, const int &iGroupIndex,
              const bool &bMutualDetection);

public:
  IC object_type &object() const;
  IC object_handler_type &objects() const;

private:
  DECLARE_SCRIPT_REGISTER_FUNCTION(CALifeMonsterBrain);
};

#include "alife_human_brain_inline.h"
