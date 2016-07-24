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
// g_misc.c

#include "g_local.h"

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/

/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void
SP_info_camp(gentity_t *self)
{
	setorigin(self, self->s.origin);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void
SP_info_null(gentity_t *self)
{
	entfree(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void
SP_info_notnull(gentity_t *self)
{
	setorigin(self, self->s.origin);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear
Non-displayed light.
"light" overrides the default 300 intensity.
Linear checbox gives linear falloff instead of inverse square
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
*/
void
SP_light(gentity_t *self)
{
	self->s.eType = ET_POINTLIGHT;
	setorigin(self, self->s.origin);
	self->r.svFlags &= ~SVF_NOCLIENT;
	spawnvec("color", "1 1 1", self->s.lightcolor);
	spawnfloat("intensity", "200", &self->s.lightintensity);
	trap_LinkEntity(self);
}

/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void
teleportentity(gentity_t *player, vec3_t origin, vec3_t angles)
{
	gentity_t *tent;
	qboolean noAngles;

	noAngles = (angles[0] > 999999.0);
	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if(player->client->sess.team != TEAM_SPECTATOR){
		tent = enttemp(player->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = player->s.clientNum;

		tent = enttemp(origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with killbox
	trap_UnlinkEntity(player);

	veccpy(origin, player->client->ps.origin);
	player->client->ps.origin[2] += 1;
	if(!noAngles){
		// spit the player out
		anglevecs(angles, player->client->ps.velocity, nil, nil);
		vecmul(player->client->ps.velocity, 400, player->client->ps.velocity);
		player->client->ps.pm_time = 160;	// hold time
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		// set angles
		setviewangles(player, angles);
	}
	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	// kill anything at the destination
	if(player->client->sess.team != TEAM_SPECTATOR)
		killbox(player);

	// save results of pmove
	playerstate2entstate(&player->client->ps, &player->s, qtrue);

	// use the precise origin for linking
	veccpy(player->client->ps.origin, player->r.currentOrigin);

	if(player->client->sess.team != TEAM_SPECTATOR)
		trap_LinkEntity(player);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void
SP_misc_teleporter_dest(gentity_t *ent)
{
}

//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
*/
void
SP_misc_model(gentity_t *ent)
{
#if 0
	ent->s.modelindex = getmodelindex(ent->model);
	vecset(ent->mins, -16, -16, -16);
	vecset(ent->maxs, 16, 16, 16);
	trap_LinkEntity(ent);

	setorigin(ent, ent->s.origin);
	veccpy(ent->s.angles, ent->s.apos.trBase);
#else
	entfree(ent);
#endif
}

/*QUAKED misc_mesh (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
*/
void
SP_misc_mesh(gentity_t *ent)
{
	ent->s.modelindex = getmodelindex(ent->model);
	vecset(ent->r.mins, -16, -16, -16);
	vecset(ent->r.maxs, 16, 16, 16);
	trap_LinkEntity(ent);

	setorigin(ent, ent->s.origin);
	veccpy(ent->s.angles, ent->s.apos.trBase);
}

//===========================================================

void
locateCamera(gentity_t *ent)
{
	vec3_t dir;
	gentity_t *target;
	gentity_t *owner;

	owner = picktarget(ent->target);
	if(!owner){
		gprintf("Couldn't find target for misc_partal_surface\n");
		entfree(ent);
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if(owner->spawnflags & 1)
		ent->s.frame = 25;
	else if(owner->spawnflags & 2)
		ent->s.frame = 75;

	// swing camera ?
	if(owner->spawnflags & 4)
		// set to 0 for no rotation at all
		ent->s.powerups = 0;
	else
		ent->s.powerups = 1;

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	veccpy(owner->s.origin, ent->s.origin2);

	// see if the portal_camera has a target
	target = picktarget(owner->target);
	if(target){
		vecsub(target->s.origin, owner->s.origin, dir);
		vecnorm(dir);
	}else
		setmovedir(owner->s.angles, dir);

	ent->s.eventParm = DirToByte(dir);
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void
SP_misc_portal_surface(gentity_t *ent)
{
	vecclear(ent->r.mins);
	vecclear(ent->r.maxs);
	trap_LinkEntity(ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if(!ent->target)
		veccpy(ent->s.origin, ent->s.origin2);
	else{
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate noswing
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void
SP_misc_portal_camera(gentity_t *ent)
{
	float roll;

	vecclear(ent->r.mins);
	vecclear(ent->r.maxs);
	trap_LinkEntity(ent);

	spawnfloat("roll", "0", &roll);

	ent->s.clientNum = roll/360.0 * 256;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void
Use_Shooter(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	vec3_t dir;
	float deg;
	vec3_t up, right;

	// see if we have a target
	if(ent->enemy){
		vecsub(ent->enemy->r.currentOrigin, ent->s.origin, dir);
		vecnorm(dir);
	}else
		veccpy(ent->movedir, dir);

	// randomize a bit
	vecperp(up, dir);
	veccross(up, dir, right);

	deg = crandom() * ent->random;
	vecmad(dir, deg, up, dir);

	deg = crandom() * ent->random;
	vecmad(dir, deg, right, dir);

	vecnorm(dir);

	switch(ent->s.weapon[0]){
	case WP_GRENADE_LAUNCHER:
		fire_grenade(ent, ent->s.origin, dir);
		break;
	case WP_ROCKET_LAUNCHER:
		fire_rocket(ent, ent->s.origin, dir);
		break;
	case WP_PLASMAGUN:
		fire_plasma(ent, ent->s.origin, dir);
		break;
	}

	addevent(ent, EV_FIRE_WEAPON, 0);
}

static void
InitShooter_Finish(gentity_t *ent)
{
	ent->enemy = picktarget(ent->target);
	ent->think = 0;
	ent->nextthink = 0;
}

void
InitShooter(gentity_t *ent, int weapon)
{
	ent->use = Use_Shooter;
	ent->s.weapon[0] = weapon;

	registeritem(finditemforweapon(weapon));

	setmovedir(ent->s.angles, ent->movedir);

	if(!ent->random)
		ent->random = 1.0;
	ent->random = sin(M_PI * ent->random / 180);
	// target might be a moving object, so we can't set movedir for it
	if(ent->target){
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity(ent);
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_rocket(gentity_t *ent)
{
	InitShooter(ent, WP_ROCKET_LAUNCHER);
}

/*QUAKED shooter_plasma (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_plasma(gentity_t *ent)
{
	InitShooter(ent, WP_PLASMAGUN);
}

/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_grenade(gentity_t *ent)
{
	InitShooter(ent, WP_GRENADE_LAUNCHER);
}

#ifdef MISSIONPACK
static void
PortalDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	entfree(self);
	//FIXME do something more interesting
}

void
dropportaldest(gentity_t *player)
{
	gentity_t *ent;
	vec3_t snapped;

	// create the portal destination
	ent = entspawn();
	ent->s.modelindex = getmodelindex("models/powerups/teleporter/tele_exit.md3");

	veccpy(player->s.pos.trBase, snapped);
	setorigin(ent, snapped);
	veccpy(player->r.mins, ent->r.mins);
	veccpy(player->r.maxs, ent->r.maxs);

	ent->classname = "hi_portal destination";
	ent->s.pos.trType = TR_STATIONARY;

	ent->r.contents = CONTENTS_CORPSE;
	ent->takedmg = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	veccpy(player->s.apos.trBase, ent->s.angles);

	ent->think = entfree;
	ent->nextthink = level.time + 2 * 60 * 1000;

	trap_LinkEntity(ent);

	player->client->portalID = ++level.portalSequence;
	ent->count = player->client->portalID;

	// give the item back so they can drop the source now
	player->client->ps.stats[STAT_HOLDABLE_ITEM] = finditem("Portal") - bg_itemlist;
}

static void
PortalTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t *destination;

	// see if we will even let other try to use it
	if(other->health <= 0)
		return;
	if(!other->client)
		return;
	//	if( other->client->ps.persistant[PERS_TEAM] != self->spawnflags ) {
//		return;
//	}

	if(other->client->ps.powerups[PW_NEUTRALFLAG]){	// only happens in One Flag CTF
		itemdrop(other, finditemforpowerup(PW_NEUTRALFLAG), 0);
		other->client->ps.powerups[PW_NEUTRALFLAG] = 0;
	}else if(other->client->ps.powerups[PW_REDFLAG]){	// only happens in standard CTF
		itemdrop(other, finditemforpowerup(PW_REDFLAG), 0);
		other->client->ps.powerups[PW_REDFLAG] = 0;
	}else if(other->client->ps.powerups[PW_BLUEFLAG]){	// only happens in standard CTF
		itemdrop(other, finditemforpowerup(PW_BLUEFLAG), 0);
		other->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	// find the destination
	destination = nil;
	while((destination = findent(destination, FOFS(classname), "hi_portal destination")) != nil)
		if(destination->count == self->count)
			break;

	// if there is not one, die!
	if(!destination){
		if(self->pos1[0] || self->pos1[1] || self->pos1[2])
			teleportentity(other, self->pos1, self->s.angles);
		entdamage(other, other, other, nil, nil, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
		return;
	}

	teleportentity(other, destination->s.pos.trBase, destination->s.angles);
}

static void
PortalEnable(gentity_t *self)
{
	self->touch = PortalTouch;
	self->think = entfree;
	self->nextthink = level.time + 2 * 60 * 1000;
}

void
dropportalsrc(gentity_t *player)
{
	gentity_t *ent;
	gentity_t *destination;
	vec3_t snapped;

	// create the portal source
	ent = entspawn();
	ent->s.modelindex = getmodelindex("models/powerups/teleporter/tele_enter.md3");

	veccpy(player->s.pos.trBase, snapped);
	setorigin(ent, snapped);
	veccpy(player->r.mins, ent->r.mins);
	veccpy(player->r.maxs, ent->r.maxs);

	ent->classname = "hi_portal source";
	ent->s.pos.trType = TR_STATIONARY;

	ent->r.contents = CONTENTS_CORPSE | CONTENTS_TRIGGER;
	ent->takedmg = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	trap_LinkEntity(ent);

	ent->count = player->client->portalID;
	player->client->portalID = 0;

//	ent->spawnflags = player->client->ps.persistant[PERS_TEAM];

	ent->nextthink = level.time + 1000;
	ent->think = PortalEnable;

	// find the destination
	destination = nil;
	while((destination = findent(destination, FOFS(classname), "hi_portal destination")) != nil)
		if(destination->count == ent->count){
			veccpy(destination->s.pos.trBase, ent->pos1);
			break;
		}

}

#endif
