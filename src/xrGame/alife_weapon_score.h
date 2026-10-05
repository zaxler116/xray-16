////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_weapon_score.h
//	Created 	: 05.10.2026
//	Description : ALife weapon scoring — smart weapon selection
//
// W.1: data structures, enemy classification, factor tables.
// W.3 will use these in CALifeHumanObjectHandler::best_weapon().
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCommon/xr_vector.h"
#include "xrServer/xrServer_defs.h"
#include "xrServer_Objects_ALife.h"


namespace AlifeWeaponScore {
// --- enemy classification -------------------------------------------

enum EAlifeEnemyType {
  eEnemyLight = 0,     // dog, boar, runner
  eEnemyMedium,        // chimera, tushkano, zombie, blox
  eEnemyHeavy,         // giant, dux, blox (thick)
  eEnemyPsy,           // controller, burer
  eEnemyGrenadeTarget, // chimera, giant, burer, controller -> GL boost
  eEnemyLast,
};

// ef_weapon_type values (gamedata/configs/weapons/*.ltx)
enum {
  efW_Item = 0,
  efW_Melee = 1,
  efW_Mutant1 = 2,
  efW_Mutant2 = 3,
  efW_Grenade = 4,
  efW_Pistol = 5,
  efW_SMG = 6,
  efW_SMG2 = 7,
  efW_Rifle = 8,
  efW_Shotgun = 9,
  efW_MachineGun = 10,
  efW_Sniper = 11,
  efW_GrenadeLauncher = 12,
};

// --- classification from monster data -------------------------------

// ef_creature_type of the monster (from creatures ltx), hp from
// m_fMaxHealthValue. Returns the best-matching enemy type.
// eEnemyGrenadeTarget is set for chimera/giant/burer/controller.
IC EAlifeEnemyType classify_enemy(u32 ef_creature, float hp);

// --- per-class factors ----------------------------------------------
// factor(weapon_class, enemy_type) — multiplier on hit_power.
// 0 = "don't use this weapon against this enemy" (e.g. knife vs heavy).

IC float class_factor(u32 weapon_ef_type, EAlifeEnemyType enemy);

// GL boost: x2 for eEnemyGrenadeTarget (chimera, giant, burer, controller)
// when the weapon is a grenade launcher (ef 12) or has a GL addon.
IC float grenade_boost(u32 weapon_ef_type, bool has_gl_addon,
                       EAlifeEnemyType enemy);

// cluster bonus: >2 enemies within 5 m of the target point.
// GL/underbarrel get x2, machine gun x2 (fallback), others x1.
IC float cluster_factor(u32 weapon_ef_type, bool has_gl_addon,
                        int cluster_count);

// --- distance factors -----------------------------------------------

// accuracy: 1/(1+(d/range)^2), range=40 m default.
IC float accuracy_factor(float dist);

// recoil: 0 if dist < min_range (e.g. GL at <50 m), else 1.
IC float recoil_factor(float dist, float min_range);

// shotgun penalty at very close range (<8 m).
IC float shotgun_close_factor(float dist, u32 weapon_ef_type);

// --- ammo / switch factors ------------------------------------------

IC float ammo_factor(u32 ammo_available, u16 ammo_limit);
IC float switch_factor(float switch_time);

// --- full score ------------------------------------------------------

struct SWeaponScore {
  u32 weapon_ef_type;
  bool has_gl_addon;
  u32 ammo_available;
  u16 ammo_limit;
  float hit_power;
  float switch_time;
  float min_range;

  float enemy_dist;
  EAlifeEnemyType enemy;
  int cluster_count;

  float total;
  float accuracy;
  float recoil;
  float cls;
  float gl;
  float cluster;
  float ammo;
  float sw;
  float shotgun;

  SWeaponScore()
      : weapon_ef_type(0), has_gl_addon(false), ammo_available(0),
        ammo_limit(0), hit_power(0.f), switch_time(0.f), min_range(0.f),
        enemy_dist(0.f), enemy(eEnemyLast), cluster_count(0), total(0.f),
        accuracy(0.f), recoil(0.f), cls(0.f), gl(0.f), cluster(0.f), ammo(0.f),
        sw(0.f), shotgun(0.f) {}
};

// computes score.total from all fields
IC float compute(const SWeaponScore &s);
} // namespace AlifeWeaponScore

#include "alife_weapon_score_inline.h"
