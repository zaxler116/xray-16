p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

# Rename the parameter 'ai' to 'stalker' in tick_kill_leave, tick, and leave_zone
# to avoid shadowing the global ai() function

# tick_kill_leave signature
old = "void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *ai, CSE_ALifeHumanAbstract *self) {"
new = "void CNpcLifeGoal::tick_kill_leave(CAI_Stalker *stalker, CSE_ALifeHumanAbstract *self) {"
assert old in t, "tick_kill_leave sig not found"
t = t.replace(old, new)

# In tick_kill_leave body: replace ai-> with stalker->
# (only within tick_kill_leave function, which is between its sig and the next function)
start = t.find(new)
end = t.find("leave_zone", start)
body = t[start:end]
body = body.replace("ai->", "stalker->").replace("(void)ai;", "(void)stalker;")
t = t[:start] + body + t[end:]

# tick signature
old = "void CNpcLifeGoal::tick(CAI_Stalker *self, CSE_ALifeHumanAbstract *human) {"
new = "void CNpcLifeGoal::tick(CAI_Stalker *self, CSE_ALifeHumanAbstract *human) {"
# tick already uses 'self' not 'ai', no change needed

# leave_zone
old = "void CNpcLifeGoal::leave_zone(CAI_Stalker *ai) {\n  // TODO 3.1.3: find nearest CLevelChanger, go_to, mark done\n  (void)ai;\n}"
new = "void CNpcLifeGoal::leave_zone(CAI_Stalker *stalker) {\n  // TODO 3.1.3: find nearest CLevelChanger, go_to, mark done\n  (void)stalker;\n}"
if old in t:
    t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
