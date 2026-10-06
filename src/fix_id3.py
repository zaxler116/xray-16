p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

old = "ai().alife().objects().object(ai->dcast_GameObject()->ID())"
new = "ai().alife().objects().object(ai->cast_game_object()->ID())"
assert old in t, "PATTERN NOT FOUND"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
