/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t bg_itemlist[] =
{
	{
		nil,
		nil,
		{nil,
		 nil,
		 nil, nil},
/* icon */ nil,
/* pickup */ nil,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	// leave index 0 alone

	// SHIELD

/*QUAKED item_shield_tiny (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_shield_tiny",
		"sound/items/shieldshard.wav",
		{"models/powerups/shieldblob/shieldblob.md3",
		 "models/powerups/shieldblob/shieldsphere.md3",
		 nil, nil},
/* icon */ "icons/shield_tiny",
/* pickup */ "5 Shield",
		5,
		IT_SHIELD,
		SHIELD_GREEN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_shield_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_shield_small",
		"sound/items/shieldgreen.wav",
		{"models/powerups/greenshieldgen/greenshieldgen.md3",
		 "models/powerups/shieldblob/shieldsphere.md3",
		 "models/powerups/shieldblob/shieldblob.md3",
		 nil},
/* icon */ "icons/shield_green",
/* pickup */ "50 Shield",
		50,
		IT_SHIELD,
		SHIELD_GREEN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_shield_med (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_shield_med",
		"sound/items/shieldyellow.wav",
		{"models/powerups/yellowshieldgen/yellowshieldgen.md3",
		 "models/powerups/shieldblob/shieldsphere.md3",
		 "models/powerups/shieldblob/shieldblob.md3",
		 nil},
/* icon */ "icons/shield_yellow",
/* pickup */ "100 Shield",
		100,
		IT_SHIELD,
		SHIELD_YELLOW,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_shield_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_shield_large",
		"sound/items/shieldred.wav",
		{"models/powerups/redshieldgen/redshieldgen.md3",
		 "models/powerups/shieldblob/shieldsphere.md3",
		 "models/powerups/redshieldgen/redshieldgen2.md3",
		 "models/powerups/shieldblob/shieldblob.md3"},
/* icon */ "icons/shield_red",
/* pickup */ "150 Shield",
		150,
		IT_SHIELD,
		SHIELD_RED,
/* precache */ "",
/* sounds */ ""
	},

	// health
/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_small",
		"sound/items/health_tiny.wav",
		{"models/powerups/5hp/5hp.md3",
		 "models/powerups/shieldblob/shieldblob.md3",
		 nil, nil},
/* icon */ "icons/health_green",
/* pickup */ "5 Health",
		5,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health",
		"sound/items/health.wav",
		{"models/powerups/25hp/25hp.md3",
		 "models/powerups/shieldblob/shieldblob.md3",
		 nil, nil},
/* icon */ "icons/health_yellow",
/* pickup */ "25 Health",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_large",
		"sound/items/health_large.wav",
		{"models/powerups/50hp/50hp.md3",
		 "models/powerups/shieldblob/shieldblob.md3",
		 nil, nil},
/* icon */ "icons/health_red",
/* pickup */ "50 Health",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_mega",
		"sound/items/health_mega.wav",
		{"models/powerups/refit/refit.md3",
		 nil,
		 nil, nil},
/* icon */ "icons/health_mega",
/* pickup */ "Mega Health",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

	// WEAPONS

/*QUAKED weapon_melee (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_melee",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/melee/melee.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_melee",
/* pickup */ "Melee",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shotgun",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/shotgun/shotgun.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_shotgun",
/* pickup */ "Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_minigun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_minigun",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/minigun/minigun.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_minigun",
/* pickup */ "Minigun",
		40,
		IT_WEAPON,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/podgrenade/podgrenade.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_grenade",
/* pickup */ "Grenade Launcher",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons2/grenade/hgrenb1a.wav sound/weapons2/grenade/hgrenb2a.wav",
/* slot */	1
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/podrocket/podrocket.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_rocket",
/* pickup */ "Rocket Launcher",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ "",
/* slot */	1
	},

/*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_lightning",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/lightning/lightning.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_lightning",
/* pickup */ "Lightning Gun",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_railgun",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/railgun/railgun.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_railgun",
/* pickup */ "Railgun",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_plasmagun",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/plasma/plasma.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_plasma",
/* pickup */ "Plasma Gun",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ "",
/* slot */	0
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_bfg",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/bfg/bfg.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_bfg",
/* pickup */ "BFG10K",
		20,
		IT_WEAPON,
		WP_BFG,
/* precache */ "",
/* sounds */ "",
/* slot */	1
	},

/*QUAKED weapon_hominglauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/	{
		"weapon_hominglauncher",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/podhoming/podhoming.md3",
		 nil, nil, nil},
		"icons/weap_homing",
		"Homing launcher",
		10,
		IT_WEAPON,
		WP_HOMING_LAUNCHER,
		"",						// precache
		"",						// sounds
		1						// slot
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/grapple/grapple.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_grapple",
/* pickup */ "Grappling Hook",
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
/* precache */ "",
/* sounds */ "",
/* slot */	2
	},

	// AMMO ITEMS

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_shells",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/shotgunam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_shotgun",
/* pickup */ "Shells",
		10,
		IT_AMMO,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bullets",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/machinegunam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_machinegun",
/* pickup */ "Bullets",
		100,
		IT_AMMO,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/grenadeam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_grenade",
/* pickup */ "Grenades",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_cells",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/plasmaam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_plasma",
/* pickup */ "Cells",
		30,
		IT_AMMO,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_lightning",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/lightningam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_lightning",
/* pickup */ "Lightning",
		60,
		IT_AMMO,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/rocketam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_rocket",
/* pickup */ "Rockets",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_homingrockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_homingrockets",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/homingam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_homingrockets",
/* pickup */ "Homing Rockets",
		5,
		IT_AMMO,
		WP_HOMING_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_slugs",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/railgunam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_railgun",
/* pickup */ "Slugs",
		10,
		IT_AMMO,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bfg",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/bfgam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_bfg",
/* pickup */ "Bfg Ammo",
		15,
		IT_AMMO,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

	// HOLDABLE ITEMS
/*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_teleporter",
		"sound/items/holdable.wav",
		{"models/powerups/holdable/teleporter.md3",
		 nil, nil, nil},
/* icon */ "icons/teleporter",
/* pickup */ "Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_medkit",
		"sound/items/holdable.wav",
		{
			"models/powerups/holdable/medkit.md3",
			"models/powerups/holdable/medkit_sphere.md3",
			nil, nil
		},
/* icon */ "icons/medkit",
/* pickup */ "Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* precache */ "",
/* sounds */ "sound/items/use_medkit.wav"
	},

	// POWERUP ITEMS
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_quad",
		"sound/items/quaddamage.wav",
		{"models/powerups/quad/quad.md3",
		 nil,
		 nil, nil},
/* icon */ "icons/quad",
/* pickup */ "Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
/* precache */ "",
/* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_enviro",
		"sound/items/protect.wav",
		{"models/powerups/instant/enviro.md3",
		 "models/powerups/instant/enviro_ring.md3",
		 nil, nil},
/* icon */ "icons/envirosuit",
/* pickup */ "Battle Suit",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* precache */ "",
/* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
	},

/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_haste",
		"sound/items/haste.wav",
		{"models/powerups/instant/haste.md3",
		 "models/powerups/instant/haste_ring.md3",
		 nil, nil},
/* icon */ "icons/haste",
/* pickup */ "Speed",
		30,
		IT_POWERUP,
		PW_HASTE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_invis",
		"sound/items/invisibility.wav",
		{"models/powerups/instant/invis.md3",
		 "models/powerups/instant/invis_ring.md3",
		 nil, nil},
/* icon */ "icons/invis",
/* pickup */ "Invisibility",
		30,
		IT_POWERUP,
		PW_INVIS,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_regen",
		"sound/items/regeneration.wav",
		{"models/powerups/instant/regen.md3",
		 "models/powerups/instant/regen_ring.md3",
		 nil, nil},
/* icon */ "icons/regen",
/* pickup */ "Regeneration",
		30,
		IT_POWERUP,
		PW_REGEN,
/* precache */ "",
/* sounds */ "sound/items/regen.wav"
	},

/*QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_flight",
		"sound/items/flight.wav",
		{"models/powerups/instant/flight.md3",
		 "models/powerups/instant/flight_ring.md3",
		 nil, nil},
/* icon */ "icons/flight",
/* pickup */ "Flight",
		60,
		IT_POWERUP,
		PW_FLIGHT,
/* precache */ "",
/* sounds */ "sound/items/flight.wav"
	},

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_redflag",
		nil,
		{"models/flags/r_flag.md3",
		 nil, nil, nil},
/* icon */ "icons/flag_red1",
/* pickup */ "Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_blueflag",
		nil,
		{"models/flags/b_flag.md3",
		 nil, nil, nil},
/* icon */ "icons/flag_blu1",
/* pickup */ "Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ ""
	},

#ifdef MISSIONPACK
/*QUAKED holdable_kamikaze (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_kamikaze",
		"sound/items/holdable.wav",
		{"models/powerups/kamikazi.md3",
		 nil, nil, nil},
/* icon */ "icons/kamikaze",
/* pickup */ "Kamikaze",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* precache */ "",
/* sounds */ "sound/items/kamikazerespawn.wav"
	},

/*QUAKED holdable_portal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_portal",
		"sound/items/holdable.wav",
		{"models/powerups/holdable/porter.md3",
		 nil, nil, nil},
/* icon */ "icons/portal",
/* pickup */ "Portal",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_invulnerability",
		"sound/items/holdable.wav",
		{"models/powerups/holdable/invulnerability.md3",
		 nil, nil, nil},
/* icon */ "icons/invulnerability",
/* pickup */ "Invulnerability",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_nails (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_nails",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/nanoidam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_nailgun",
/* pickup */ "Nails",
		50,
		IT_AMMO,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_mines",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/proxmineam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_prox",
/* pickup */ "Proximity Mines",
		10,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_belt",
		"sound/misc/am_pkup.wav",
		{"models/powerups/ammo/heminigunam.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_chaingun",
/* pickup */ "GAU-9 Belt",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ ""
	},

	// PERSISTANT POWERUP ITEMS
/*QUAKED item_scout (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redteam blueteam
*/
	{
		"item_scout",
		"sound/items/scout.wav",
		{"models/powerups/scout.md3",
		 nil, nil, nil},
/* icon */ "icons/scout",
/* pickup */ "Scout",
		30,
		IT_PERSISTANT_POWERUP,
		PW_SCOUT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_guard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redteam blueteam
*/
	{
		"item_guard",
		"sound/items/guard.wav",
		{"models/powerups/guard.md3",
		 nil, nil, nil},
/* icon */ "icons/guard",
/* pickup */ "Guard",
		30,
		IT_PERSISTANT_POWERUP,
		PW_GUARD,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redteam blueteam
*/
	{
		"item_doubler",
		"sound/items/doubler.wav",
		{"models/powerups/doubler.md3",
		 nil, nil, nil},
/* icon */ "icons/doubler",
/* pickup */ "Doubler",
		30,
		IT_PERSISTANT_POWERUP,
		PW_DOUBLER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redteam blueteam
*/
	{
		"item_ammoregen",
		"sound/items/ammoregen.wav",
		{"models/powerups/ammo.md3",
		 nil, nil, nil},
/* icon */ "icons/ammo_regen",
/* pickup */ "Ammo Regen",
		30,
		IT_PERSISTANT_POWERUP,
		PW_AMMOREGEN,
/* precache */ "",
/* sounds */ ""
	},

	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
	Only in One Flag CTF games
	*/
	{
		"team_CTF_neutralflag",
		nil,
		{"models/flags/n_flag.md3",
		 nil, nil, nil},
/* icon */ "icons/flag_neutral1",
/* pickup */ "Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_redcube",
		"sound/misc/am_pkup.wav",
		{"models/powerups/orb/r_orb.md3",
		 nil, nil, nil},
/* icon */ "icons/health_rorb",
/* pickup */ "Red Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_bluecube",
		"sound/misc/am_pkup.wav",
		{"models/powerups/orb/b_orb.md3",
		 nil, nil, nil},
/* icon */ "icons/health_borb",
/* pickup */ "Blue Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED weapon_nanoid (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_nanoid",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/nanoid/nanoid.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_nanoid",
/* pickup */ "Nailgun",
		40,
		IT_WEAPON,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_prox_launcher",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/prox/prox.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_prox",
/* pickup */ "Prox Launcher",
		5,
		IT_WEAPON,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons2/proxmine/wstbtick.wav "
		"sound/weapons2/proxmine/wstbactv.wav "
		"sound/weapons2/proxmine/wstbimpl.wav "
		"sound/weapons2/proxmine/wstbimpm.wav "
		"sound/weapons2/proxmine/wstbimpd.wav "
		"sound/weapons2/proxmine/wstbactv.wav",
		1
	},

/*QUAKED weapon_heminigun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_heminigun",
		"sound/misc/w_pkup.wav",
		{"models/weapons2/heminigun/heminigun.md3",
		 nil, nil, nil},
/* icon */ "icons/weap_heminigun",
/* pickup */ "GAU-9",
		80,
		IT_WEAPON,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ "sound/weapons2/heminigun/flash.wav"
	},
#endif

	// end of list marker
	{nil}
};

int bg_nitems = ARRAY_LEN(bg_itemlist) - 1;

gitem_t *
finditemforpowerup(powerup_t pw)
{
	int i;

	for(i = 0; i < bg_nitems; i++)
		if((bg_itemlist[i].type == IT_POWERUP ||
		    bg_itemlist[i].type == IT_TEAM ||
		    bg_itemlist[i].type == IT_PERSISTANT_POWERUP) &&
		   bg_itemlist[i].tag == pw)
			return &bg_itemlist[i];

	return nil;
}

gitem_t *
finditemforholdable(holdable_t pw)
{
	int i;

	for(i = 0; i < bg_nitems; i++)
		if(bg_itemlist[i].type == IT_HOLDABLE && bg_itemlist[i].tag == pw)
			return &bg_itemlist[i];

	Com_Error(ERR_DROP, "HoldableItem not found");

	return nil;
}

gitem_t *
finditemforweapon(weapon_t weapon)
{
	gitem_t *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if(it->type == IT_WEAPON && it->tag == weapon)
			return it;

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return nil;
}

gitem_t *
finditem(const char *pickupName)
{
	gitem_t *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if(!Q_stricmp(it->pickupname, pickupName))
			return it;

	return nil;
}

/*
Items can be picked up without actually touching their physical bounds to make
grabbing them easier
*/
qboolean
playertouchingitem(playerState_t *ps, entityState_t *item, int atTime)
{
	vec3_t origin;

	evaltrajectory(&item->pos, atTime, origin);

	if(ps->origin[0] - origin[0] > ITEM_RADIUS
	   || ps->origin[0] - origin[0] < -ITEM_RADIUS
	   || ps->origin[1] - origin[1] > ITEM_RADIUS
	   || ps->origin[1] - origin[1] < -ITEM_RADIUS
	   || ps->origin[2] - origin[2] > ITEM_RADIUS
	   || ps->origin[2] - origin[2] < -ITEM_RADIUS)
		return qfalse;

	return qtrue;
}

/*
Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
*/
qboolean
cangrabitem(int gametype, const entityState_t *ent, const playerState_t *ps)
{
	gitem_t *item;
#ifdef MISSIONPACK
	int upperBound;
#endif

	if(ent->modelindex < 1 || ent->modelindex >= bg_nitems)
		Com_Error(ERR_DROP, "cangrabitem: index out of range");

	item = &bg_itemlist[ent->modelindex];

	switch(item->type){
	case IT_WEAPON:
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		if(ps->ammo[item->tag] >= 200)
			return qfalse;	// can't hold any more
		return qtrue;

	case IT_SHIELD:
#ifdef MISSIONPACK
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].tag == PW_SCOUT)
			return qfalse;

		// we also clamp shield to the maxhealth for handicapping
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].tag == PW_GUARD)
			upperBound = ps->stats[STAT_MAX_HEALTH];
		else
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;

		if(ps->stats[STAT_SHIELD] >= upperBound)
			return qfalse;

#else
		if(ps->stats[STAT_SHIELD] >= ps->stats[STAT_MAX_HEALTH] * 2)
			return qfalse;

#endif
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
#ifdef MISSIONPACK
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].tag == PW_GUARD){
		}else
#endif
		if(item->quantity == 5 || item->quantity == 100){
			if(ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2)
				return qfalse;
			return qtrue;
		}

		if(ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
			return qfalse;
		return qtrue;

	case IT_POWERUP:
		return qtrue;	// powerups are always picked up

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if(ps->stats[STAT_PERSISTANT_POWERUP])
			return qfalse;

		// check team only
		if((ent->generic1 & 2) && (ps->persistant[PERS_TEAM] != TEAM_RED))
			return qfalse;
		if((ent->generic1 & 4) && (ps->persistant[PERS_TEAM] != TEAM_BLUE))
			return qfalse;

		return qtrue;
#endif

	case IT_TEAM:	// team items, such as flags
#ifdef MISSIONPACK
		if(gametype == GT_1FCTF){
			// neutral flag can always be picked up
			if(item->tag == PW_NEUTRALFLAG)
				return qtrue;
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->tag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->tag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
		}
#endif
		if(gametype == GT_CTF){
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->tag == PW_BLUEFLAG ||
				   (item->tag == PW_REDFLAG && ent->modelindex2) ||
				   (item->tag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]))
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->tag == PW_REDFLAG ||
				   (item->tag == PW_BLUEFLAG && ent->modelindex2) ||
				   (item->tag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]))
					return qtrue;
		}

#ifdef MISSIONPACK
		if(gametype == GT_HARVESTER)
			return qtrue;

#endif
		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if(ps->stats[STAT_HOLDABLE_ITEM])
			return qfalse;
		return qtrue;

	case IT_BAD:
		Com_Error(ERR_DROP, "cangrabitem: IT_BAD");
	default:
#ifndef Q3_VM
#ifndef NDEBUG
		Com_Printf("cangrabitem: unknown enum %d\n", item->type);
#endif
#endif
		break;
	}

	return qfalse;
}

//======================================================================

void
evaltrajectory(const trajectory_t *tr, int atTime, vec3_t result)
{
	float deltaTime;
	float phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		veccpy(tr->trBase, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		vecmad(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = sin(deltaTime * M_PI * 2);
		vecmad(tr->trBase, phase, tr->trDelta, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration)
			atTime = tr->trTime + tr->trDuration;
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		if(deltaTime < 0)
			deltaTime = 0;
		vecmad(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;			// milliseconds to seconds
		vecmad(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;	// FIXME: local gravity...
		break;
	default:
		Com_Error(ERR_DROP, "evaltrajectory: unknown trType: %i", tr->trType);
		break;
	}
}

/*
For determining velocity at a given time
*/
void
evaltrajectorydelta(const trajectory_t *tr, int atTime, vec3_t result)
{
	float deltaTime;
	float phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		vecclear(result);
		break;
	case TR_LINEAR:
		veccpy(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = cos(deltaTime * M_PI * 2);	// derivative of sin = cos
		phase *= 0.5;
		vecmul(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration){
			vecclear(result);
			return;
		}
		veccpy(tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		veccpy(tr->trDelta, result);
		result[2] -= DEFAULT_GRAVITY * deltaTime;	// FIXME: local gravity...
		break;
	default:
		Com_Error(ERR_DROP, "evaltrajectorydelta: unknown trType: %i", tr->trType);
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_JUMP_PAD",	// boing sound at origin, jump sound on player

	"EV_WATER_TOUCH",	// origin touches
	"EV_WATER_LEAVE",	// origin leaves
	"EV_WATER_UNDER",	// origin touches
	"EV_WATER_CLEAR",	// origin leaves

	"EV_ITEM_PICKUP",		// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",		// slot 0
	"EV_NOAMMO2",		// slot 1
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",		// slot 0
	"EV_FIRE_WEAPON2",	// slot 1
	"EV_FIRE_WEAPON3",	// slot 2

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",	// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",	// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",	// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",	// gib a previously living player
	"EV_SCOREPLUM",	// score plum

//#ifdef MISSIONPACK
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",		// kamikaze explodes
	"EV_OBELISKEXPLODE",	// obelisk explodes
	"EV_OBELISKPAIN",		// obelisk is in pain
	"EV_INVUL_IMPACT",	// invulnerability sphere impact
	"EV_JUICED",		// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",	// lightning bolt bounced of invulnerability sphere
//#endif

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
};

/*
Handles the sequence numbers
*/

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

void
bgaddpredictableevent(int newEvent, int eventParm, playerState_t *ps)
{
#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if(atof(buf) != 0){
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

void
touchjumppad(playerState_t *ps, entityState_t *jumppad)
{
	vec3_t angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if(ps->pm_type != PM_NORMAL)
		return;

	// flying characters don't hit bounce pads
	if(ps->powerups[PW_FLIGHT])
		return;

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if(ps->jumppad_ent != jumppad->number){
		vectoangles(jumppad->origin2, angles);
		p = fabs(AngleNormalize180(angles[PITCH]));
		if(p < 45)
			effectNum = 0;
		else
			effectNum = 1;
		bgaddpredictableevent(EV_JUMP_PAD, effectNum, ps);
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	veccpy(jumppad->origin2, ps->velocity);
}

void
touchtriggergravity(playerState_t *ps, entityState_t *zone, float framedur)
{
	vec3_t grav;

	if(ps->pm_type != PM_NORMAL)
		return;
	vecmul(zone->origin2, 0.001f*framedur, grav);
	vecadd(ps->velocity, grav, ps->velocity);
}

/*
playerState_t to entityState_t

This is done after each set of usercmd_t on the server,
and after local prediction on the client
*/
void
ps2es(playerState_t *ps, entityState_t *s, qboolean snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= 0)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	veccpy(ps->origin, s->pos.trBase);
	// set the trDelta for flag direction
	veccpy(ps->velocity, s->pos.trDelta);

	s->apos.trType = TR_INTERPOLATE;
	veccpy(ps->viewangles, s->apos.trBase);

	s->forwardmove = ps->forwardmove;
	s->rightmove = ps->rightmove;
	s->upmove = ps->upmove;

	s->weapAnim[0] = ps->weapAnim[0];
	s->weapAnim[1] = ps->weapAnim[1];

	s->shipanim = ps->shipanim;
	s->clientNum = ps->clientNum;	// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->awardflags = ps->awardflags;	// FIXME: redundant awardflags
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon[0] = ps->weapon[0];
	s->weapon[1] = ps->weapon[1];
	s->weapon[2] = ps->weapon[2];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[i])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	s->lockontarget = ps->lockontarget;
	s->lockonstarttime = ps->lockonstarttime;
	s->lockontime = ps->lockontime;
}

/*
playerState_t to entityState_t, extrapolated

This is done after each set of usercmd_t on the server,
and after local prediction on the client
*/
void
ps2es_xerp(playerState_t *ps, entityState_t *s, int time, qboolean snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= 0)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	veccpy(ps->origin, s->pos.trBase);
	// set the trDelta for flag direction and linear prediction
	veccpy(ps->velocity, s->pos.trDelta);
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 100;	// 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	veccpy(ps->viewangles, s->apos.trBase);

	s->forwardmove = ps->forwardmove;
	s->rightmove = ps->rightmove;
	s->upmove = ps->upmove;

	s->weapAnim[0] = ps->weapAnim[0];
	s->weapAnim[1] = ps->weapAnim[1];

	s->shipanim = ps->shipanim;
	s->clientNum = ps->clientNum;	// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->awardflags = ps->awardflags;
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon[0] = ps->weapon[0];
	s->weapon[1] = ps->weapon[1];
	s->weapon[2] = ps->weapon[2];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[i])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	s->lockontarget = ps->lockontarget;
	s->lockonstarttime = ps->lockonstarttime;
	s->lockontime = ps->lockontime;
}
