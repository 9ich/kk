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
#include "g_local.h"

#define MISSILE_PRESTEP_TIME 0

/*
================
G_BounceMissile

================
*/
void
G_BounceMissile(gentity_t *ent, trace_t *trace)
{
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.prevtime + (level.time - level.prevtime) * trace->fraction;
	evaltrajectorydelta(&ent->s.pos, hitTime, velocity);
	dot = vecdot(velocity, trace->plane.normal);
	vecmad(velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta);

	if(ent->s.eFlags & EF_BOUNCE_HALF){
		vecmul(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
		// check for stop
		if(trace->plane.normal[2] > 0.2 && veclen(ent->s.pos.trDelta) < 40){
			setorigin(ent, trace->endpos);
			ent->s.time = level.time / 4;
			return;
		}
	}

	vecadd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	veccpy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void
G_ExplodeMissile(gentity_t *ent)
{
	vec3_t dir;
	vec3_t origin;

	evaltrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	setorigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	addevent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeafterevent = qtrue;

	// splash damage
	if(ent->splashdmg)
		if(radiusdamage(ent->r.currentOrigin, ent->parent, ent->splashdmg, ent->splashradius, ent
				  , ent->splashmeansofdeath))
			g_entities[ent->r.ownerNum].client->accuracyhits++;

	trap_LinkEntity(ent);
}

#ifdef MISSIONPACK
/*
================
ProximityMine_Explode
================
*/
static void
ProximityMine_Explode(gentity_t *mine)
{
	G_ExplodeMissile(mine);
	// if the prox mine has a trigger free it
	if(mine->activator){
		entfree(mine->activator);
		mine->activator = nil;
	}
}

/*
================
ProximityMine_Die
================
*/
static void
ProximityMine_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void
ProximityMine_Trigger(gentity_t *trigger, gentity_t *other, trace_t *trace)
{
	vec3_t v;
	gentity_t *mine;

	if(!other->client)
		return;

	// trigger is a cube, do a distance test now to act as if it's a sphere
	vecsub(trigger->s.pos.trBase, other->s.pos.trBase, v);
	if(veclen(v) > trigger->parent->splashradius)
		return;


	if(g_gametype.integer >= GT_TEAM)
		// don't trigger same team mines
		if(trigger->parent->s.generic1 == other->client->sess.team)
			return;

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if(!candamage(other, trigger->s.pos.trBase))
		return;

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	addevent(mine, EV_PROXIMITY_MINE_TRIGGER, 0);
	mine->nextthink = level.time + 500;

	entfree(trigger);
}

/*
================
ProximityMine_Activate
================
*/
static void
ProximityMine_Activate(gentity_t *ent)
{
	gentity_t *trigger;
	float r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedmg = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = getsoundindex("sound/weapons/proxmine/wstbtick.wav");

	// build the proximity trigger
	trigger = entspawn();

	trigger->classname = "proxmine_trigger";

	r = ent->splashradius;
	vecset(trigger->r.mins, -r, -r, -r);
	vecset(trigger->r.maxs, r, r, r);

	setorigin(trigger, ent->s.pos.trBase);

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity(trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void
ProximityMine_ExplodeOnPlayer(gentity_t *mine)
{
	gentity_t *player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if(player->client->invulnerabilityTime > level.time){
		entdamage(player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED);
		player->client->invulnerabilityTime = 0;
		enttemp(player->client->ps.origin, EV_JUICED);
	}else{
		setorigin(mine, player->s.pos.trBase);
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashmeansofdeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile(mine);
	}
}

/*
================
ProximityMine_Player
================
*/
static void
ProximityMine_Player(gentity_t *mine, gentity_t *player)
{
	if(mine->s.eFlags & EF_NODRAW)
		return;

	addevent(mine, EV_PROXIMITY_MINE_STICK, 0);

	if(player->s.eFlags & EF_TICKING){
		player->activator->splashdmg += mine->splashdmg;
		player->activator->splashradius *= 1.50;
		mine->think = entfree;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	vecclear(mine->s.pos.trDelta);

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if(player->client->invulnerabilityTime > level.time)
		mine->nextthink = level.time + 2 * 1000;
	else
		mine->nextthink = level.time + 10 * 1000;
}

#endif

/*
================
G_MissileImpact
================
*/
void
G_MissileImpact(gentity_t *ent, trace_t *trace)
{
	gentity_t *other;
	qboolean hitClient = qfalse;
#ifdef MISSIONPACK
	vec3_t forward, impactpoint, bouncedir;
	int eFlags;
#endif
	other = &g_entities[trace->entityNum];

	// check for bounce
	if(!other->takedmg &&
	   (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF))){
		G_BounceMissile(ent, trace);
		if(ent->s.weapon == WP_GRENADE_LAUNCHER)
			addevent(ent, EV_GRENADE_BOUNCE, 0);
		return;
	}

#ifdef MISSIONPACK
	if(other->takedmg){
		if(ent->s.weapon != WP_PROX_LAUNCHER)
			if(other->client && other->client->invulnerabilityTime > level.time){
				veccpy(ent->s.pos.trDelta, forward);
				vecnorm(forward);
				if(G_InvulnerabilityEffect(other, forward, ent->s.pos.trBase, impactpoint, bouncedir)){
					veccpy(bouncedir, trace->plane.normal);
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile(ent, trace);
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
	}
#endif
	// impact damage
	if(other->takedmg)
		// FIXME: wrong damage direction?
		if(ent->damage){
			vec3_t velocity;

			if(logaccuracyhit(other, &g_entities[ent->r.ownerNum])){
				g_entities[ent->r.ownerNum].client->accuracyhits++;
				hitClient = qtrue;
			}
			evaltrajectorydelta(&ent->s.pos, level.time, velocity);
			if(veclen(velocity) == 0)
				velocity[2] = 1;	// stepped on a grenade
			entdamage(other, ent, &g_entities[ent->r.ownerNum], velocity,
				 ent->s.origin, ent->damage,
				 0, ent->meansofdeath);
		}

#ifdef MISSIONPACK
	if(ent->s.weapon == WP_PROX_LAUNCHER){
		if(ent->s.pos.trType != TR_GRAVITY)
			return;

		// if it's a player, stick it on to them (flag them and remove this entity)
		if(other->s.eType == ET_PLAYER && other->health > 0){
			ProximityMine_Player(ent, other);
			return;
		}

		snapvectortowards(trace->endpos, ent->s.pos.trBase);
		setorigin(ent, trace->endpos);
		ent->s.pos.trType = TR_STATIONARY;
		vecclear(ent->s.pos.trDelta);

		addevent(ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags);

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles(trace->plane.normal, ent->s.angles);
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		veccpy(trace->plane.normal, ent->movedir);
		vecset(ent->r.mins, -4, -4, -4);
		vecset(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

	if(!strcmp(ent->classname, "hook")){
		gentity_t *nent;
		vec3_t v;

		nent = entspawn();
		if(other->takedmg && other->client){
			addevent(nent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			snapvectortowards(v, ent->s.pos.trBase);	// save net bandwidth
		}else{
			veccpy(trace->endpos, v);
			addevent(nent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
			ent->enemy = nil;
		}

		snapvectortowards(v, ent->s.pos.trBase);	// save net bandwidth

		nent->freeafterevent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		setorigin(ent, v);
		setorigin(nent, v);

		ent->think = weapon_hook_think;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		veccpy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity(ent);
		trap_LinkEntity(nent);

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if(other->takedmg && other->client){
		addevent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}else if(trace->surfaceFlags & SURF_METALSTEPS)
		addevent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	else
		addevent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));

	ent->freeafterevent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	snapvectortowards(trace->endpos, ent->s.pos.trBase);	// save net bandwidth

	setorigin(ent, trace->endpos);

	// splash damage (doesn't apply to person directly hit)
	if(ent->splashdmg)
		if(radiusdamage(trace->endpos, ent->parent, ent->splashdmg, ent->splashradius,
				  other, ent->splashmeansofdeath))
			if(!hitClient)
				g_entities[ent->r.ownerNum].client->accuracyhits++;

	trap_LinkEntity(ent);
}

/*
================
runmissile
================
*/
void
runmissile(gentity_t *ent)
{
	vec3_t origin;
	trace_t tr;
	int passent;

	// get current position
	evaltrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if(ent->target_ent)
		passent = ent->target_ent->s.number;

#ifdef MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if(ent->s.weapon == WP_PROX_LAUNCHER && ent->count)
		passent = ENTITYNUM_NONE;

#endif
	else
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	// trace a line from the previous position to the current position
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

	if(tr.startsolid || tr.allsolid){
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
		tr.fraction = 0;
	}else
		veccpy(tr.endpos, ent->r.currentOrigin);

	trap_LinkEntity(ent);

	if(tr.fraction != 1){
		// never explode or bounce on sky
		if(tr.surfaceFlags & SURF_NOIMPACT){
			// If grapple, reset owner
			if(ent->parent && ent->parent->client && ent->parent->client->hook == ent)
				ent->parent->client->hook = nil;
			entfree(ent);
			return;
		}
		G_MissileImpact(ent, &tr);
		if(ent->s.eType != ET_MISSILE)
			return;	// exploded
	}
#ifdef MISSIONPACK
	// if the prox mine wasn't yet outside the player body
	if(ent->s.weapon == WP_PROX_LAUNCHER && !ent->count){
		// check if the prox mine is outside the owner bbox
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask);
		if(!tr.startsolid || tr.entityNum != ent->r.ownerNum)
			ent->count = 1;
	}
#endif
	// check think function after bouncing
	runthink(ent);
}

//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *
fire_plasma(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashdmg = 15;
	bolt->splashradius = 20;
	bolt->meansofdeath = MOD_PLASMA;
	bolt->splashmeansofdeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_grenade
=================
*/
gentity_t *
fire_grenade(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 3500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 150;
	bolt->meansofdeath = MOD_GRENADE;
	bolt->splashmeansofdeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_bfg
=================
*/
gentity_t *
fire_bfg(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 120;
	bolt->meansofdeath = MOD_BFG;
	bolt->splashmeansofdeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_rocket
=================
*/
gentity_t *
fire_rocket(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 120;
	bolt->meansofdeath = MOD_ROCKET;
	bolt->splashmeansofdeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 900, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

void
homingrocket_think(gentity_t *ent)
{
	vec3_t olddir, dir;
	gentity_t *targ;
	float dot;

	targ = &g_entities[ent->homingtarget];

	evaltrajectory(&ent->s.pos, level.time, ent->r.currentOrigin);
	veccpy(ent->s.pos.trDelta, olddir);
	vecnorm(olddir);

	vecsub(targ->r.currentOrigin, ent->r.currentOrigin, dir);
	vecnorm(dir);
	dot = vecdot(olddir, dir);
	if(dot >= 0.707f){
		float t;

		t = 0.66f;
		veccpy(ent->r.currentOrigin, ent->s.pos.trBase);
		dir[0] = (1-t)*olddir[0] + t*dir[0];
		dir[1] = (1-t)*olddir[1] + t*dir[1];
		dir[2] = (1-t)*olddir[2] + t*dir[2];
		vecmul(dir, 200, ent->s.pos.trDelta);
		ent->s.pos.trType = TR_LINEAR;
		ent->s.pos.trTime = level.time;
	}else{
		// lost track
		ent->nextthink = 0;
		ent->think = nil;
		veccpy(ent->r.currentOrigin, ent->s.pos.trBase);
		vecmul(olddir, 200, ent->s.pos.trDelta);
		ent->s.pos.trTime = level.time;
	}

	ent->think = homingrocket_think;
	ent->nextthink = level.time + 100;
}

gentity_t *
fire_homingrocket(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "homingrocket";
	bolt->think = homingrocket_think;
	bolt->nextthink = level.time + 100;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_HOMING_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 120;
	bolt->meansofdeath = MOD_ROCKET;
	bolt->splashmeansofdeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->homingtarget = self->client->ps.lockontarget;

	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 100, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *
fire_grapple(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *hook;

	vecnorm(dir);

	hook = entspawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = weapon_hook_free;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->meansofdeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = nil;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number;// use to match beam in client
	veccpy(start, hook->s.pos.trBase);
	vecmul(dir, 800, hook->s.pos.trDelta);
	SnapVector(hook->s.pos.trDelta);	// save net bandwidth
	veccpy(start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}

#ifdef MISSIONPACK
/*
=================
fire_nail
=================
*/
gentity_t *
fire_nail(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;
	vec3_t end;

	bolt = entspawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 5000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->s.eFlags = EF_BOUNCE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->meansofdeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 1600, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_prox
=================
*/
gentity_t *
fire_prox(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashdmg = 100;
	bolt->splashradius = 150;
	bolt->meansofdeath = MOD_PROXIMITY_MINE;
	bolt->splashmeansofdeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.team;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccpy(start, bolt->s.pos.trBase);
	vecmul(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	veccpy(start, bolt->r.currentOrigin);

	return bolt;
}

#endif
