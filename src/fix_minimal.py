p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

old = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *ai, CSE_ALifeHumanAbstract *self) {
  if (!self)
    return;

  // get target
  CSE_ALifeHumanAbstract *target = nullptr;
  {
    ALife::D_OBJECT_P_MAP::const_iterator I =
        ai().alife().objects().objects().begin();
    ALife::D_OBJECT_P_MAP::const_iterator E =
        ai().alife().objects().objects().end();
    for (; I != E; ++I) {
      if (I->first == m_target_npc) {
        target = smart_cast<CSE_ALifeHumanAbstract *>(I->second);
        break;
      }
    }
  }"""
new = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *ai, CSE_ALifeHumanAbstract *self) {
  (void)ai; (void)self;
  // TODO: implement
  return;

  // get target
  CSE_ALifeHumanAbstract *target = nullptr;
  {
    ALife::D_OBJECT_P_MAP::const_iterator I =
        ai().alife().objects().objects().begin();
    ALife::D_OBJECT_P_MAP::const_iterator E =
        ai().alife().objects().objects().end();
    for (; I != E; ++I) {
      if (I->first == m_target_npc) {
        target = smart_cast<CSE_ALifeHumanAbstract *>(I->second);
        break;
      }
    }
  }"""
assert old in t, "pattern not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
