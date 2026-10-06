p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker.cpp"
t = open(p, "r", encoding="utf-8").read()

old = """    CSE_ALifeDynamicObject *self_entity = ai().alife().objects().object(ID());
    CSE_ALifeHumanAbstract *human = smart_cast<CSE_ALifeHumanAbstract *>(self_entity);
    if (human && human->g_Alive())
      human->goal().tick(this);"""
new = """    CSE_ALifeDynamicObject *self_entity = ai().alife().objects().object(ID());
    CSE_ALifeHumanAbstract *human = smart_cast<CSE_ALifeHumanAbstract *>(self_entity);
    if (human && human->g_Alive())
      human->goal().tick(this, human);"""
assert old in t, "Think() tick call not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ai_stalker.cpp OK")
