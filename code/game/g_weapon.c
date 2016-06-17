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
// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

static float s_quadFactor;
static vec3_t forward, right, up;
static vec3_t muzzle;

static void
inheritvel(gentity_t *ent, gentity_t *m)
{
	float speed, d;
	vec3_t dir;
	
	veccpy(ent->client->ps.velocity, dir);
	vecnorm(dir);
	d = vecdot(forward, dir);
	if(d > 0.0f){
		speed = veclen(ent->client->ps.velocity);
		vecmad(m->s.pos.trDelta, d*speed, forward, m->s.pos.trDelta);
	}
}

/*
================
G_BounceProjectile
================
*/
void
G_BounceProjectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout)
{
	vec3_t v, newv;
	float dot;

	vecsub(impact, start, v);
	dot = vecdot(v, dir);
	vecmad(v, -2*dot, dir, newv);

	vecnorm(newv);
	vecmad(impact, 8192, newv, endout);
}

/*
======================================================================

GAUNTLET

======================================================================
*/

void
Weapon_Gauntlet(gentity_t *ent)
{
}

/*
===============
chkgauntletattack
===============
*/
qboolean
chkgauntletattack(gentity_t *ent)
{
	trace_t tr;
	vec3_t end;
	gentity_t *tent;
	gentity_t *traceEnt;
	int damage;

	// set aiming directions
	anglevecs(ent->client->ps.viewangles, forward, right, up);

	calcmuzzlepoint(ent, forward, right, up, muzzle);

	vecmad(muzzle, 32, forward, end);

	trap_Trace(&tr, muzzle, nil, nil, end, ent->s.number, MASK_SHOT);
	if(tr.surfaceFlags & SURF_NOIMPACT)
		return qfalse;

	if(ent->client->noclip)
		return qfalse;

	traceEnt = &g_entities[tr.entityNum];

	// send blood impact
	if(traceEnt->takedmg && traceEnt->client){
		tent = enttemp(tr.endpos, EV_MISSILE_HIT);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	}

	if(!traceEnt->takedmg)
		return qfalse;

	if(ent->client->ps.powerups[PW_QUAD]){
		addevent(ent, EV_POWERUP_QUAD, 0);
		s_quadFactor = g_quadfactor.value;
	}else
		s_quadFactor = 1;

#ifdef MISSIONPACK
	if(ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->tag == PW_DOUBLER)
		s_quadFactor *= 2;

#endif

	damage = 50 * s_quadFactor;
	entdamage(traceEnt, ent, ent, forward, tr.endpos,
		 damage, 0, MOD_GAUNTLET);

	return qtrue;
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
snapvectortowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void
snapvectortowards(vec3_t v, vec3_t to)
{
	int i;

	for(i = 0; i < 3; i++){
		if(to[i] <= v[i])
			v[i] = floor(v[i]);
		else
			v[i] = ceil(v[i]);
	}
}

#ifdef MISSIONPACK
#define CHAINGUN_SPREAD		600
#define CHAINGUN_DAMAGE		7
#endif
#define MACHINEGUN_SPREAD	200
#define MACHINEGUN_DAMAGE	7
#define MACHINEGUN_TEAM_DAMAGE	5	// wimpier MG in teamplay

void
Bullet_Fire(gentity_t *ent, float spread, int damage, int mod)
{
	trace_t tr;
	vec3_t end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	float r;
	float u;
	gentity_t *tent;
	gentity_t *traceEnt;
	int i, passent;

	damage *= s_quadFactor;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
	vecmad(muzzle, 8192*16, forward, end);
	vecmad(end, r, right, end);
	vecmad(end, u, up, end);

	passent = ent->s.number;
	for(i = 0; i < 10; i++){
		trap_Trace(&tr, muzzle, nil, nil, end, passent, MASK_SHOT);
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return;

		traceEnt = &g_entities[tr.entityNum];

		// snap the endpos to integers, but nudged towards the line
		snapvectortowards(tr.endpos, muzzle);

		// send bullet impact
		if(traceEnt->takedmg && traceEnt->client){
			tent = enttemp(tr.endpos, EV_BULLET_HIT_FLESH);
			tent->s.eventParm = traceEnt->s.number;
			if(logaccuracyhit(traceEnt, ent))
				ent->client->accuracyhits++;
		}else{
			tent = enttemp(tr.endpos, EV_BULLET_HIT_WALL);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}
		tent->s.otherEntityNum = ent->s.number;

		if(traceEnt->takedmg){
#ifdef MISSIONPACK
			if(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time){
				if(invulneffect(traceEnt, forward, tr.endpos, impactpoint, bouncedir)){
					G_BounceProjectile(muzzle, impactpoint, bouncedir, end);
					veccpy(impactpoint, muzzle);
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}else{
					veccpy(tr.endpos, muzzle);
					passent = traceEnt->s.number;
				}
				continue;
			}else{
#endif
			entdamage(traceEnt, ent, ent, forward, tr.endpos,
				 damage, 0, mod);
#ifdef MISSIONPACK
		}
#endif
		}
		break;
	}
}

/*
======================================================================

BFG

======================================================================
*/

void
BFG_Fire(gentity_t *ent)
{
	gentity_t       *m;

	m = fire_bfg(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

	inheritvel(ent, m);
}

/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define DEFAULT_SHOTGUN_DAMAGE 10

qboolean
ShotgunPellet(vec3_t start, vec3_t end, gentity_t *ent)
{
	trace_t tr;
	int damage, i, passent;
	gentity_t       *traceEnt;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	vec3_t tr_start, tr_end;

	passent = ent->s.number;
	veccpy(start, tr_start);
	veccpy(end, tr_end);
	for(i = 0; i < 10; i++){
		trap_Trace(&tr, tr_start, nil, nil, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[tr.entityNum];

		// send bullet impact
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return qfalse;

		if(traceEnt->takedmg){
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
#ifdef MISSIONPACK
			if(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time){
				if(invulneffect(traceEnt, forward, tr.endpos, impactpoint, bouncedir)){
					G_BounceProjectile(tr_start, impactpoint, bouncedir, tr_end);
					veccpy(impactpoint, tr_start);
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}else{
					veccpy(tr.endpos, tr_start);
					passent = traceEnt->s.number;
				}
				continue;
			}else{
				entdamage(traceEnt, ent, ent, forward, tr.endpos,
					 damage, 0, MOD_SHOTGUN);
				if(logaccuracyhit(traceEnt, ent))
					return qtrue;
			}
#else
			entdamage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN);
			if(logaccuracyhit(traceEnt, ent))
				return qtrue;

#endif
		}
		return qfalse;
	}
	return qfalse;
}

// this should match CG_ShotgunPattern
void
ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, gentity_t *ent)
{
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;
	qboolean hitClient = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	vecperp(right, forward);
	veccross(forward, right, up);

	// generate the "random" spread pattern
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		vecmad(origin, 8192 * 16, forward, end);
		vecmad(end, r, right, end);
		vecmad(end, u, up, end);
		if(ShotgunPellet(origin, end, ent) && !hitClient){
			hitClient = qtrue;
			ent->client->accuracyhits++;
		}
	}
}

void
weapon_supershotgun_fire(gentity_t *ent)
{
	gentity_t               *tent;

	// send shotgun blast
	tent = enttemp(muzzle, EV_SHOTGUN);
	vecmul(forward, 4096, tent->s.origin2);
	SnapVector(tent->s.origin2);
	tent->s.eventParm = rand() & 255;	// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern(tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent);
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void
weapon_grenadelauncher_fire(gentity_t *ent)
{
	gentity_t       *m;

	vecnorm(forward);

	m = fire_grenade(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

	inheritvel(ent, m);
}

/*
======================================================================

ROCKET

======================================================================
*/

void
Weapon_RocketLauncher_Fire(gentity_t *ent)
{
	gentity_t       *m;

	m = fire_rocket(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

	inheritvel(ent, m);
}

/*
======================================================================

HOMINGLAUNCHER

======================================================================
*/

void
Weapon_HomingLauncher_Fire(gentity_t *ent)
{
	gentity_t       *m;

	m = fire_homingrocket(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;
	ent->client->ps.lockontarget = ENTITYNUM_NONE;

	inheritvel(ent, m);
}
/*
======================================================================

PLASMA GUN

======================================================================
*/

void
Weapon_Plasmagun_Fire(gentity_t *ent)
{
	gentity_t       *m;

	m = fire_plasma(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

	inheritvel(ent, m);
}

/*
======================================================================

RAILGUN

======================================================================
*/

/*
=================
weapon_railgun_fire
=================
*/
#define MAX_RAIL_HITS 4
void
weapon_railgun_fire(gentity_t *ent)
{
	vec3_t end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	trace_t trace;
	gentity_t       *tent;
	gentity_t       *traceEnt;
	int damage;
	int i;
	int hits;
	int unlinked;
	int passent;
	gentity_t       *unlinkedEntities[MAX_RAIL_HITS];

	damage = 100 * s_quadFactor;

	vecmad(muzzle, 8192, forward, end);

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace(&trace, muzzle, nil, nil, end, passent, MASK_SHOT);
		if(trace.entityNum >= ENTITYNUM_MAX_NORMAL)
			break;
		traceEnt = &g_entities[trace.entityNum];
		if(traceEnt->takedmg){
#ifdef MISSIONPACK
			if(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time){
				if(invulneffect(traceEnt, forward, trace.endpos, impactpoint, bouncedir)){
					G_BounceProjectile(muzzle, impactpoint, bouncedir, end);
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					snapvectortowards(trace.endpos, muzzle);
					// send railgun beam effect
					tent = enttemp(trace.endpos, EV_RAILTRAIL);
					// set player number for custom colors on the railtrail
					tent->s.clientNum = ent->s.clientNum;
					veccpy(muzzle, tent->s.origin2);
					// move origin a bit to come closer to the drawn gun muzzle
					vecmad(tent->s.origin2, 4, right, tent->s.origin2);
					vecmad(tent->s.origin2, -1, up, tent->s.origin2);
					tent->s.eventParm = 255;	// don't make the explosion at the end
					veccpy(impactpoint, muzzle);
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			}else{
				if(logaccuracyhit(traceEnt, ent))
					hits++;
				entdamage(traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
			}
#else
			if(logaccuracyhit(traceEnt, ent))
				hits++;
			entdamage(traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
#endif
		}
		if(trace.contents & CONTENTS_SOLID)
			break;	// we hit something solid enough to stop the beam
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while(unlinked < MAX_RAIL_HITS);

	// link back in any entities we unlinked
	for(i = 0; i < unlinked; i++)
		trap_LinkEntity(unlinkedEntities[i]);

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	snapvectortowards(trace.endpos, muzzle);

	// send railgun beam effect
	tent = enttemp(trace.endpos, EV_RAILTRAIL);

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	veccpy(muzzle, tent->s.origin2);
	// move origin a bit to come closer to the drawn gun muzzle
	vecmad(tent->s.origin2, 4, right, tent->s.origin2);
	vecmad(tent->s.origin2, -1, up, tent->s.origin2);

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if(trace.surfaceFlags & SURF_NOIMPACT)
		tent->s.eventParm = 255;	// don't make the explosion at the end
	else
		tent->s.eventParm = DirToByte(trace.plane.normal);
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if(hits == 0)
		// complete miss
		ent->client->accuratecount = 0;
	else{
		// check for "impressive" reward sound
		ent->client->accuratecount += hits;
		if(ent->client->accuratecount >= 2){
			ent->client->accuratecount -= 2;
			giveaward(ent->client, AWARD_IMPRESSIVE);
		}
		ent->client->accuracyhits++;
	}
}

/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void
Weapon_GrapplingHook_Fire(gentity_t *ent)
{
	if(!ent->client->fireheld && !ent->client->hook)
		fire_grapple(ent, muzzle, forward);

	ent->client->fireheld = qtrue;
}

void
weapon_hook_free(gentity_t *ent)
{
	ent->parent->client->hook = nil;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	entfree(ent);
}

void
weapon_hook_think(gentity_t *ent)
{
	if(ent->enemy){
		vec3_t v, oldorigin;

		veccpy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		snapvectortowards(v, oldorigin);	// save net bandwidth

		setorigin(ent, v);
	}

	veccpy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void
Weapon_LightningFire(gentity_t *ent)
{
	trace_t tr;
	vec3_t end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	gentity_t       *traceEnt, *tent;
	int damage, i, passent;

	damage = 8 * s_quadFactor;

	passent = ent->s.number;
	for(i = 0; i < 10; i++){
		vecmad(muzzle, LIGHTNING_RANGE, forward, end);

		trap_Trace(&tr, muzzle, nil, nil, end, passent, MASK_SHOT);

#ifdef MISSIONPACK
		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if(i){
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			tent = enttemp(muzzle, EV_LIGHTNINGBOLT);
			veccpy(tr.endpos, end);
			SnapVector(end);
			veccpy(end, tent->s.origin2);
		}
#endif
		if(tr.entityNum == ENTITYNUM_NONE)
			return;

		traceEnt = &g_entities[tr.entityNum];

		if(traceEnt->takedmg){
#ifdef MISSIONPACK
			if(traceEnt->client && traceEnt->client->invulnerabilityTime > level.time){
				if(invulneffect(traceEnt, forward, tr.endpos, impactpoint, bouncedir)){
					G_BounceProjectile(muzzle, impactpoint, bouncedir, end);
					veccpy(impactpoint, muzzle);
					vecsub(end, impactpoint, forward);
					vecnorm(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}else{
					veccpy(tr.endpos, muzzle);
					passent = traceEnt->s.number;
				}
				continue;
			}else
				entdamage(traceEnt, ent, ent, forward, tr.endpos,
					 damage, 0, MOD_LIGHTNING);

#else
			entdamage(traceEnt, ent, ent, forward, tr.endpos,
				 damage, 0, MOD_LIGHTNING);
#endif
		}

		if(traceEnt->takedmg && traceEnt->client){
			tent = enttemp(tr.endpos, EV_MISSILE_HIT);
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weapon = ent->s.weapon;
			if(logaccuracyhit(traceEnt, ent))
				ent->client->accuracyhits++;
		}else if(!(tr.surfaceFlags & SURF_NOIMPACT)){
			tent = enttemp(tr.endpos, EV_MISSILE_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}

		break;
	}
}

#ifdef MISSIONPACK
/*
======================================================================

NAILGUN

======================================================================
*/

void
Weapon_Nailgun_Fire(gentity_t *ent)
{
	gentity_t       *m;

	m = fire_nail(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;
	inheritvel(ent, m);

}

/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void
weapon_proxlauncher_fire(gentity_t *ent)
{
	gentity_t       *m;

	// extra vertical velocity
	forward[2] += 0.2f;
	vecnorm(forward);

	m = fire_prox(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

	inheritvel(ent, m);
}

#endif

//======================================================================

/*
===============
logaccuracyhit
===============
*/
qboolean
logaccuracyhit(gentity_t *target, gentity_t *attacker)
{
	if(!target->takedmg)
		return qfalse;

	if(target == attacker)
		return qfalse;

	if(!target->client)
		return qfalse;

	if(!attacker->client)
		return qfalse;

	if(target->client->ps.stats[STAT_HEALTH] <= 0)
		return qfalse;

	if(onsameteam(target, attacker))
		return qfalse;

	return qtrue;
}

/*
===============
calcmuzzlepoint

set muzzle location relative to pivoting eye
===============
*/
void
calcmuzzlepoint(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	veccpy(ent->client->ps.origin, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/*
===============
calcmuzzlepointorigin

set muzzle location relative to pivoting eye
===============
*/
void
calcmuzzlepointorigin(gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	veccpy(ent->client->ps.origin, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/*
===============
fireweapon
===============
*/
void
fireweapon(gentity_t *ent)
{
	if(ent->client->ps.powerups[PW_QUAD])
		s_quadFactor = g_quadfactor.value;
	else
		s_quadFactor = 1;

#ifdef MISSIONPACK
	if(ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->tag == PW_DOUBLER)
		s_quadFactor *= 2;

#endif

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if(ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET){
		ent->client->accuracyshots++;
	}

	// set aiming directions
	anglevecs(ent->client->ps.viewangles, forward, right, up);

	calcmuzzlepointorigin(ent, ent->client->oldorigin, forward, right, up, muzzle);

	// fire the specific weapon
	switch(ent->s.weapon){
	case WP_GAUNTLET:
		Weapon_Gauntlet(ent);
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire(ent);
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire(ent);
		break;
	case WP_MACHINEGUN:
		if(g_gametype.integer != GT_TEAM)
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_MACHINEGUN);
		else
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE, MOD_MACHINEGUN);
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire(ent);
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire(ent);
		break;
	case WP_HOMING_LAUNCHER:
		Weapon_HomingLauncher_Fire(ent);
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire(ent);
		break;
	case WP_RAILGUN:
		weapon_railgun_fire(ent);
		break;
	case WP_BFG:
		BFG_Fire(ent);
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire(ent);
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		Weapon_Nailgun_Fire(ent);
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire(ent);
		break;
	case WP_CHAINGUN:
		Bullet_Fire(ent, CHAINGUN_SPREAD, CHAINGUN_DAMAGE, MOD_CHAINGUN);
		break;
#endif
	default:
// FIXME		errorf( "Bad ent->s.weapon" );
		break;
	}
}

#ifdef MISSIONPACK

/*
===============
KamikazeRadiusDamage
===============
*/
static void
KamikazeRadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius)
{
	float dist;
	gentity_t       *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++){
		ent = &g_entities[entityList[e]];

		if(!ent->takedmg)
			continue;

		// dont hit things we have already hit
		if(ent->kamikazeTime > level.time)
			continue;

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++){
			if(origin[i] < ent->r.absmin[i])
				v[i] = ent->r.absmin[i] - origin[i];
			else if(origin[i] > ent->r.absmax[i])
				v[i] = origin[i] - ent->r.absmax[i];
			else
				v[i] = 0;
		}

		dist = veclen(v);
		if(dist >= radius)
			continue;

//		if( candamage (ent, origin) ) {
		vecsub(ent->r.currentOrigin, origin, dir);
		// push the center of mass higher than the origin so players
		// get knocked into the air more
		dir[2] += 24;
		entdamage(ent, nil, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE);
		ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void
KamikazeShockWave(vec3_t origin, gentity_t *attacker, float damage, float push, float radius)
{
	float dist;
	gentity_t       *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++){
		ent = &g_entities[entityList[e]];

		// dont hit things we have already hit
		if(ent->kamikazeShockTime > level.time)
			continue;

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++){
			if(origin[i] < ent->r.absmin[i])
				v[i] = ent->r.absmin[i] - origin[i];
			else if(origin[i] > ent->r.absmax[i])
				v[i] = origin[i] - ent->r.absmax[i];
			else
				v[i] = 0;
		}

		dist = veclen(v);
		if(dist >= radius)
			continue;

//		if( candamage (ent, origin) ) {
		vecsub(ent->r.currentOrigin, origin, dir);
		dir[2] += 24;
		entdamage(ent, nil, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE);
		dir[2] = 0;
		vecnorm(dir);
		if(ent->client){
			ent->client->ps.velocity[0] = dir[0] * push;
			ent->client->ps.velocity[1] = dir[1] * push;
			ent->client->ps.velocity[2] = 100;
		}
		ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void
KamikazeDamage(gentity_t *self)
{
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if(self->count >= KAMI_SHOCKWAVE_STARTTIME){
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400, (int)(float)t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME));
	}
	if(self->count >= KAMI_EXPLODE_STARTTIME){
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage(self->s.pos.trBase, self->activator, 400, (int)(float)t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME));
	}

	// either cycle or kill self
	if(self->count >= KAMI_SHOCKWAVE_ENDTIME){
		entfree(self);
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];
		if(!ent->inuse)
			continue;
		if(!ent->client)
			continue;

		if(ent->client->ps.groundEntityNum != ENTITYNUM_NONE){
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	veccpy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void
G_StartKamikaze(gentity_t *ent)
{
	gentity_t       *explosion;
	gentity_t       *te;
	vec3_t snapped;

	// start up the explosion logic
	explosion = entspawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventtime = level.time;

	if(ent->client)
		veccpy(ent->s.pos.trBase, snapped);
	else
		veccpy(ent->activator->s.pos.trBase, snapped);
	SnapVector(snapped);	// save network bandwidth
	setorigin(explosion, snapped);

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	vecclear(explosion->movedir);

	trap_LinkEntity(explosion);

	if(ent->client){
		explosion->activator = ent;
		ent->s.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		entdamage(ent, ent, ent, nil, nil, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE);
	}else{
		if(!strcmp(ent->activator->classname, "bodyque"))
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		else
			explosion->activator = ent->activator;
	}

	// play global sound at all clients
	te = enttemp(snapped, EV_GLOBAL_TEAM_SOUND);
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}

#endif

void
homing_scan(gentity_t *ent)
{
	vec3_t mins, maxs;
	trace_t tr;
	playerState_t *ps;
	float bestdist;
	int i, best;

	if(ent->client == nil)
		return;

	ps = &ent->client->ps;

	if(ps->weapon != WP_HOMING_LAUNCHER){
		ps->lockontarget = ENTITYNUM_NONE;
		ps->lockontime = 0;
		ps->lockonstarttime = 0;
		return;
	}

	vecset(mins, -1, -1, -1);
	vecset(maxs, 1, 1, 1);
	anglevecs(ps->viewangles, forward, nil, nil);

	best = ENTITYNUM_NONE;
	bestdist = 99999;

	for(i = 0; i < level.maxclients; i++){
		vec3_t entpos, dir;
		float dist;

		if(!g_entities[i].inuse)
			continue;
		veccpy(g_entities[i].r.currentOrigin, entpos);
		vecsub(entpos, ps->origin, dir);
		vecnorm(dir);
		if(vecdot(forward, dir) < 0.95f)
			continue;
		dist = vecdist(ps->origin, entpos);
		if(dist > HOMING_SCANRANGE)
			continue;
		if(dist > bestdist)
			continue;
		trap_Trace(&tr, ps->origin, mins, maxs, entpos,
		   ent->s.number, MASK_SHOT);
		if(tr.entityNum != i)
			continue;
		best = i;
		bestdist = dist;
	}

	if(best == ENTITYNUM_NONE || onsameteam(ent, &g_entities[best]) ||
	   !g_entities[best].inuse){
	   	// nothing to lock on to
		ps->lockontarget = ENTITYNUM_NONE;
		ps->lockontime = 0;
		ps->lockonstarttime = 0;
		return;
	}

	if(ps->lockontarget != best){
		ps->lockontarget = best;
		ps->lockontime = level.time;
		ps->lockonstarttime = level.time;
		return;
	}
	// continue locking on
	ps->lockontime = level.time;
}
