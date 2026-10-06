p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

# Add early-return in tick_kill_leave when target is already killed
old = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *stalker, CSE_ALifeHumanAbstract *self) {
  if (!self)
    return;"""
new = """void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *stalker, CSE_ALifeHumanAbstract *self) {
  if (!self)
    return;

  // goal already completed (target killed + reached level changer)
  if (m_target_killed)
    return;"""
assert old in t, "tick_kill_leave guard not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
