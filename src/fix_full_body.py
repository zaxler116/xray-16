p = r"C:\DEV\code\personal\xray-16\src\xrGame\ai\stalker\ai_stalker_goal.cpp"
t = open(p, "r", encoding="utf-8").read()

old = """  (void)target;
}"""
new = """  if (!target) {
    // target is gone (unregistered) -> reassign a new target
    Msg("NPC [%s] goal: target is gone, reassigning", self->name());
    m_target_killed = false;
    pick_kill_leave(self);
    return;
  }

  if (target->g_Alive()) {
    // target is alive, navigate to it
    Fvector target_pos = target->o_Position;
    ALife::_OBJECT_ID target_game_vertex =
        ALife::_OBJECT_ID(target->m_tGraphID);
    go_to(stalker, target_game_vertex, ALife::_OBJECT_ID(0), target_pos);
    return;
  }

  // target is dead - check who killed it
  ALife::_OBJECT_ID killer = target->get_killer_id();
  if (killer != ALife::_OBJECT_ID(0xffff) && killer != self->ID) {
    // someone else killed the target -> reassign
    Msg("NPC [%s] goal: target [%s] was killed by another, reassigning",
        self->name(), target->name());
    m_target_killed = false;
    pick_kill_leave(self);
    return;
  }

  // target was killed by us (or killer_id not set) -> leave the Zone
  m_target_killed = true;
  Msg("NPC [%s] goal: target [%s] is dead, leaving the Zone",
      self->name(), target->name());
  leave_zone(stalker);
}"""
assert old in t, "pattern not found"
t = t.replace(old, new)

open(p, "w", encoding="utf-8", newline="\n").write(t)
print("OK")
