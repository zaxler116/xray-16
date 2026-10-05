#pragma once

namespace AlifeWeaponScore {
IC EAlifeEnemyType classify_enemy(u32 ef_creature, float hp) {
  // ef_creature_type from creatures ltx:
  //   0 = human, 1 = stalker, 2 = mutant, 3 = psy-mutant (controller/burer)
  // hp = m_fMaxHealthValue

  if (ef_creature == 3)
    return eEnemyPsy;

  if (ef_creature == 2) {
    if (hp > 2000.f)
      return eEnemyHeavy;
    if (hp >= 500.f)
      return eEnemyMedium;
    return eEnemyLight;
  }

  return eEnemyMedium;
}

IC float class_factor(u32 w, EAlifeEnemyType e) {
  // rows: weapon ef_type, cols: enemy type
  //
  //            light  medium  heavy   psy   grenade_target
  // knife(1)    1.0    0.8    0.5    1.0    0.5
  // pistol(5)   1.3    1.2    1.0    1.2    1.0
  // smg(6-8)    1.2    1.2    1.0    1.2    1.2
  // shotgun(9)  2.0    2.0    1.5    1.3    1.5
  // mg(10)      1.0    1.8    2.5    1.2    2.0
  // sniper(11)  1.2    1.5    1.8    1.3    1.8
  // gl(12)      1.0    1.2    1.5    0.8    2.0

  switch (w) {
  case efW_Melee:
    switch (e) {
    case eEnemyLight:
      return 1.0f;
    case eEnemyMedium:
      return 0.8f;
    case eEnemyHeavy:
      return 0.5f;
    case eEnemyPsy:
      return 1.0f;
    case eEnemyGrenadeTarget:
      return 0.5f;
    }
    break;
  case efW_Pistol:
    switch (e) {
    case eEnemyLight:
      return 1.3f;
    case eEnemyMedium:
      return 1.2f;
    case eEnemyHeavy:
      return 1.0f;
    case eEnemyPsy:
      return 1.2f;
    case eEnemyGrenadeTarget:
      return 1.0f;
    }
    break;
  case efW_SMG:
  case efW_SMG2:
  case efW_Rifle:
    switch (e) {
    case eEnemyLight:
      return 1.2f;
    case eEnemyMedium:
      return 1.2f;
    case eEnemyHeavy:
      return 1.0f;
    case eEnemyPsy:
      return 1.2f;
    case eEnemyGrenadeTarget:
      return 1.2f;
    }
    break;
  case efW_Shotgun:
    switch (e) {
    case eEnemyLight:
      return 2.0f;
    case eEnemyMedium:
      return 2.0f;
    case eEnemyHeavy:
      return 1.5f;
    case eEnemyPsy:
      return 1.3f;
    case eEnemyGrenadeTarget:
      return 1.5f;
    }
    break;
  case efW_MachineGun:
    switch (e) {
    case eEnemyLight:
      return 1.0f;
    case eEnemyMedium:
      return 1.8f;
    case eEnemyHeavy:
      return 2.5f;
    case eEnemyPsy:
      return 1.2f;
    case eEnemyGrenadeTarget:
      return 2.0f;
    }
    break;
  case efW_Sniper:
    switch (e) {
    case eEnemyLight:
      return 1.2f;
    case eEnemyMedium:
      return 1.5f;
    case eEnemyHeavy:
      return 1.8f;
    case eEnemyPsy:
      return 1.3f;
    case eEnemyGrenadeTarget:
      return 1.8f;
    }
    break;
  case efW_GrenadeLauncher:
    switch (e) {
    case eEnemyLight:
      return 1.0f;
    case eEnemyMedium:
      return 1.2f;
    case eEnemyHeavy:
      return 1.5f;
    case eEnemyPsy:
      return 0.8f;
    case eEnemyGrenadeTarget:
      return 2.0f;
    }
    break;
  default:
    return 1.0f;
  }
  return 1.0f;
}

IC float grenade_boost(u32 w, bool has_gl, EAlifeEnemyType e) {
  bool is_gl = (w == efW_GrenadeLauncher) || has_gl;
  if (!is_gl)
    return 1.0f;
  if (e == eEnemyGrenadeTarget)
    return 2.0f;
  return 1.0f;
}

IC float cluster_factor(u32 w, bool has_gl, int n) {
  if (n <= 2)
    return 1.0f;
  bool is_gl = (w == efW_GrenadeLauncher) || has_gl;
  if (is_gl)
    return 2.0f;
  if (w == efW_MachineGun)
    return 2.0f;
  return 1.0f;
}

IC float accuracy_factor(float d) {
  const float range = 40.f;
  float r = d / range;
  return 1.f / (1.f + r * r);
}

IC float recoil_factor(float d, float min_range) {
  if (min_range > 0.f && d < min_range)
    return 0.f;
  return 1.f;
}

IC float shotgun_close_factor(float d, u32 w) {
  if (w == efW_Shotgun && d < 8.f)
    return 0.2f;
  return 1.f;
}

IC float ammo_factor(u32 avail, u16 limit) {
  if (avail == 0)
    return 0.f;
  if (limit > 0 && avail < (u32)limit)
    return 0.5f;
  return 1.0f;
}

IC float switch_factor(float t) {
  if (t <= 0.f)
    return 1.0f;
  return 1.f / (1.f + t / 2.f);
}

IC float compute(const SWeaponScore &s) {
  float a = accuracy_factor(s.enemy_dist);
  float r = recoil_factor(s.enemy_dist, s.min_range);
  float c = class_factor(s.weapon_ef_type, s.enemy);
  float g = grenade_boost(s.weapon_ef_type, s.has_gl_addon, s.enemy);
  float cl = cluster_factor(s.weapon_ef_type, s.has_gl_addon, s.cluster_count);
  float am = ammo_factor(s.ammo_available, s.ammo_limit);
  float sw = switch_factor(s.switch_time);
  float sg = shotgun_close_factor(s.enemy_dist, s.weapon_ef_type);

  float total = s.hit_power * a * r * c * g * cl * am * sw * sg;

  s.accuracy = a;
  s.recoil = r;
  s.cls = c;
  s.gl = g;
  s.cluster = cl;
  s.ammo = am;
  s.sw = sw;
  s.shotgun = sg;
  s.total = total;
  return total;
}
} // namespace AlifeWeaponScore
