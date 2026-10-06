import sys

p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

old1 = """  if (!target) {
    // target is gone (unregistered) -> reassign a new target
    Msg("NPC [%s] goal: target [%s] is gone, reassigning",
        self->name(), m_target_npc);"""
new1 = """  if (!target) {
    // target is gone (unregistered) -> reassign a new target
    Msg("NPC [%s] goal: target is gone, reassigning", self->name());"""
assert old1 in t, "PATTERN1 NOT FOUND"
t = t.replace(old1, new1)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
