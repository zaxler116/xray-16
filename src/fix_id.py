import sys

path = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"

with open(path, "r", encoding="utf-8") as f:
    t = f.read()

# Use object().ID() instead of ai->ID() (object() returns CGameObject& which has ID())
t = t.replace(
    "ai().alife().objects().object(ai->ID())",
    "ai().alife().objects().object(ai->object().ID())",
)

with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(t)

print("OK")
