p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

old = """    m_target_killed = true;
    // NOTE: full NPC teleport is not implemented (no server-side
    // M_CHANGE_LEVEL for NPCs). The NPC stays on the level and
    // simply stops pursuing this goal.
    Msg("NPC [%s] goal: reached level changer, goal complete (teleport not implemented)",
        stalker->cName().c_str());"""
new = """    m_target_killed = true;
    m_type = eGoalCount;
    // NOTE: full NPC teleport is not implemented (no server-side
    // M_CHANGE_LEVEL for NPCs). The NPC stays on the level and
    // simply stops pursuing this goal.
    Msg("NPC [%s] goal: reached level changer, goal complete (teleport not implemented)",
        stalker->cName().c_str());"""
assert old in t, "arrival block not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
