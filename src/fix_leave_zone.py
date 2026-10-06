p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

# add include + extern
old_inc = '#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"'
new_inc = '#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"\n#include "level_changer.h"\n\nextern xr_vector<CLevelChanger*> g_lchangers;'
assert old_inc in t, "include not found"
t = t.replace(old_inc, new_inc)

# replace leave_zone stub
old = """void CNpcLifeGoal::leave_zone(CAI_Stalker *stalker) {
  // TODO 3.1.3: find nearest CLevelChanger, go_to, mark done
  (void)stalker;
}"""
new = """void CNpcLifeGoal::leave_zone(CAI_Stalker *stalker) {
  // find nearest enabled level changer
  CLevelChanger *best = nullptr;
  float best_dist = 1e9f;
  for (xr_vector<CLevelChanger *>::iterator I = g_lchangers.begin();
       I != g_lchangers.end(); ++I) {
    CLevelChanger *lc = *I;
    if (!lc || !lc->IsLevelChangerEnabled())
      continue;
    float d = stalker->Position().distance_to_sqr(lc->Position());
    if (d < best_dist) {
      best_dist = d;
      best = lc;
    }
  }

  if (!best) {
    // no level changer on this level -> switch to wander
    Msg("NPC [%s] goal: no level changer found, switching to wander",
        stalker->cName().c_str());
    m_type = eGoalExploreZone;
    return;
  }

  // navigate to the level changer
  go_to(stalker, ALife::_OBJECT_ID(0), ALife::_OBJECT_ID(0),
        best->Position());

  // arrived? (within 2m)
  if (stalker->Position().distance_to(best->Position()) < 2.0f) {
    m_target_killed = true;
    // NOTE: full NPC teleport is not implemented (no server-side
    // M_CHANGE_LEVEL for NPCs). The NPC stays on the level and
    // simply stops pursuing this goal.
    Msg("NPC [%s] goal: reached level changer, goal complete (teleport not implemented)",
        stalker->cName().c_str());
  }
}"""
assert old in t, "leave_zone stub not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
