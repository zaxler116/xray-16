p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

# 1. Remove the m_target_killed guard from tick_kill_leave (added in fix_done_guard)
old_guard = """  // goal already completed (target killed + reached level changer)
  if (m_target_killed)
    return;
"""
assert old_guard in t, "guard not found"
t = t.replace(old_guard, "")

# 2. In tick_kill_leave: only call leave_zone when m_target_killed
#    (already the case: the code sets m_target_killed=true then calls leave_zone)
#    But we need: if m_target_killed is already true, just call leave_zone again
#    (NPC is walking to the changer). The target lookup will find target dead,
#    killer check passes, m_target_killed stays true, leave_zone called. OK.

# 3. In leave_zone: on arrival, set m_type = eGoalCount (goal done, no more ticking)
old_arrive = """    m_target_killed = true;
    // NOTE: full NPC teleport is not implemented (no server-side
    // M_CHANGE_LEVEL for NPCs). The NPC stays on the level and
    // simply stops pursuing this goal.
    Msg("NPC [%s] goal: reached level changer, goal complete (teleport not implemented)",
        stalker->cName().c_str());"""
new_arrive = """    m_target_killed = true;
    m_type = eGoalCount;
    // NOTE: full NPC teleport is not implemented (no server-side
    // M_CHANGE_LEVEL for NPCs). The NPC stays on the level and
    // simply stops pursuing this goal.
    Msg("NPC [%s] goal: reached level changer, goal complete (teleport not implemented)",
        stalker->cName().c_str());"""
assert old_arrive in t, "arrival block not found"
t = t.replace(old_arrive, new_arrive)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
