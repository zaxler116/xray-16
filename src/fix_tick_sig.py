import re

# --- patch header: add self param to tick/tick_kill_leave ---
h = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.h"
t = open(h, "r", encoding="utf-8").read()

old_tick = "  void tick(CAI_Stalker *self);"
new_tick = "  void tick(CAI_Stalker *self, CSE_ALifeHumanAbstract *human);"
assert old_tick in t, "tick decl not found"
t = t.replace(old_tick, new_tick)

old_tl = "  void tick_kill_leave(CAI_Stalker *ai);"
new_tl = "  void tick_kill_leave(CAI_Stalker *ai, CSE_ALifeHumanAbstract *self);"
assert old_tl in t, "tick_kill_leave decl not found"
t = t.replace(old_tl, new_tl)

open(h, "w", encoding="utf-8", newline="\n").write(t)
print("header OK")

# --- patch cpp ---
c = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(c, "r", encoding="utf-8").read()

# tick signature + dispatch
old_tick = """void CNpcLifeGoal::tick(CAI_Stalker *self) {
  if (is_none())
    return;

  switch (type()) {
  case eGoalKillNpcAndLeave:
    tick_kill_leave(self);
    break;"""
new_tick = """void CNpcLifeGoal::tick(CAI_Stalker *self, CSE_ALifeHumanAbstract *human) {
  if (is_none())
    return;

  switch (type()) {
  case eGoalKillNpcAndLeave:
    tick_kill_leave(self, human);
    break;"""
assert old_tick in t, "tick body not found"
t = t.replace(old_tick, new_tick)

# tick_kill_leave signature + remove self lookup
old_tl = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *ai) {
  // self entity
  CSE_ALifeDynamicObject *self_dyn = ai().alife().objects().object(ai->cast_game_object()->ID());
  CSE_ALifeHumanAbstract *self =
      smart_cast<CSE_ALifeHumanAbstract *>(self_dyn);
  if (!self)
    return;

  // get target"""
new_tl = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *ai, CSE_ALifeHumanAbstract *self) {
  if (!self)
    return;

  // get target"""
assert old_tl in t, "tick_kill_leave body not found"
t = t.replace(old_tl, new_tl)

open(c, "w", encoding="utf-8", newline="\n").write(t)
print("cpp OK")
