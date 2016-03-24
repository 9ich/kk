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

void
InitTrigger(gentity_t *self)
{
	if(!veccmp(self->s.angles, vec3_origin))
		setmovedir(self->s.angles, self->movedir);

	trap_SetBrushModel(self, self->model);
	self->r.contents = CONTENTS_TRIGGER;	// replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
void
multi_wait(gentity_t *ent)
{
	ent->nextthink = 0;
}

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void
multi_trigger(gentity_t *ent, gentity_t *activator)
{
	ent->activator = activator;
	if(ent->nextthink)
		return;	// can't retrigger until the wait is over

	if(activator->client){
		if((ent->spawnflags & 1) &&
		   activator->client->sess.team != TEAM_RED)
			return;
		if((ent->spawnflags & 2) &&
		   activator->client->sess.team != TEAM_BLUE)
			return;
	}

	usetargets(ent, ent->activator);

	if(ent->wait > 0){
		ent->think = multi_wait;
		ent->nextthink = level.time + (ent->wait + ent->random * crandom()) * 1000;
	}else{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = entfree;
	}
}

void
Use_Multi(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	multi_trigger(ent, activator);
}

void
Touch_Multi(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(!other->client)
		return;
	multi_trigger(self, other);
}

/*QUAKED trigger_multiple (.5 .5 .5) ?
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void
SP_trigger_multiple(gentity_t *ent)
{
	spawnfloat("wait", "0.5", &ent->wait);
	spawnfloat("random", "0", &ent->random);

	if(ent->random >= ent->wait && ent->wait >= 0){
		ent->random = ent->wait - FRAMETIME;
		gprintf("trigger_multiple has random >= wait\n");
	}

	ent->touch = Touch_Multi;
	ent->use = Use_Multi;

	InitTrigger(ent);
	trap_LinkEntity(ent);
}

/*
==============================================================================

trigger_always

==============================================================================
*/

void
trigger_always_think(gentity_t *ent)
{
	usetargets(ent, ent);
	entfree(ent);
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void
SP_trigger_always(gentity_t *ent)
{
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = trigger_always_think;
}

/*
==============================================================================

trigger_push

==============================================================================
*/

void
trigger_push_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(!other->client)
		return;

	touchjumppad(&other->client->ps, &self->s);
}

static void
trigger_gravity_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(other->client == nil)
		return;
	BG_TouchTriggerGravity(&other->client->ps, &self->s, level.time - level.prevtime);
}

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void
AimAtTarget(gentity_t *self)
{
	gentity_t *ent;
	vec3_t origin;
	float height, gravity, time, forward;
	float dist;

	vecadd(self->r.absmin, self->r.absmax, origin);
	vecmul(origin, 0.5, origin);

	ent = picktarget(self->target);
	if(!ent){
		entfree(self);
		return;
	}

	height = ent->s.origin[2] - origin[2];
	gravity = g_gravity.value;
	time = sqrt(height / (.5 * gravity));
	if(!time){
		entfree(self);
		return;
	}

	// set s.origin2 to the push velocity
	vecsub(ent->s.origin, origin, self->s.origin2);
	self->s.origin2[2] = 0;
	dist = vecnorm(self->s.origin2);

	forward = dist / time;
	vecmul(self->s.origin2, forward, self->s.origin2);

	self->s.origin2[2] = time * gravity;
}

/*
Copy target's "angles" key to origin2, then scale by "gravity" key.
*/
static void
aimgravity(gentity_t *self)
{
	gentity_t *targ;
	vec3_t dir;

	if(self->target == nil){
		gprintf("trigger_gravity with null target\n");
		return;
	}
	targ = nil;
	targ = findent(targ, FOFS(targetname), self->target);
	if(self->target == nil){
		gprintf("trigger_gravity with unfound target\n");
		return;
	}
	anglevecs(targ->s.angles, dir, nil, nil);
	vecmul(dir, self->speed, self->s.origin2);
}

/*QUAKED trigger_push (.5 .5 .5) ?
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
*/
void
SP_trigger_push(gentity_t *self)
{
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	getsoundindex("sound/world/jumppad.wav");

	self->s.eType = ET_PUSH_TRIGGER;
	self->touch = trigger_push_touch;
	self->think = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity(self);
}

/* QUAKED trigger_gravity (.5 .9 .5) ?
When entered, this brush additively increases the player's velocity in
the direction given by the "angles" key of the target target_position.
This effect is a trigger_ so that it can be client-side predicted.

speed		strength of the gravity field (default 120).
*/
void
SP_trigger_gravity(gentity_t *self)
{
	if(!self->speed)
		self->speed = 120;

	InitTrigger(self);

	// send to client for prediction
	self->r.svFlags &= ~SVF_NOCLIENT;
	self->s.eType = ET_TRIGGER_GRAVITY;
	self->touch = trigger_gravity_touch;
	self->think = aimgravity;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity(self);
}

void
Use_target_push(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if(!activator->client)
		return;

	if(activator->client->ps.pm_type != PM_NORMAL)
		return;
	if(activator->client->ps.powerups[PW_FLIGHT])
		return;

	veccpy(self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if(activator->flysounddebouncetime < level.time){
		activator->flysounddebouncetime = level.time + 1500;
		mksound(activator, CHAN_AUTO, self->noiseindex);
	}
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void
SP_target_push(gentity_t *self)
{
	if(!self->speed)
		self->speed = 1000;
	setmovedir(self->s.angles, self->s.origin2);
	vecmul(self->s.origin2, self->speed, self->s.origin2);

	if(self->spawnflags & 1)
		self->noiseindex = getsoundindex("sound/world/jumppad.wav");
	else
		self->noiseindex = getsoundindex("sound/misc/windfly.wav");
	if(self->target){
		veccpy(self->s.origin, self->r.absmin);
		veccpy(self->s.origin, self->r.absmax);
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void
trigger_teleporter_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t *dest;

	if(!other->client)
		return;
	if(other->client->ps.pm_type == PM_DEAD)
		return;
	// Spectators only?
	if((self->spawnflags & 1) &&
	   other->client->sess.team != TEAM_SPECTATOR)
		return;


	dest = picktarget(self->target);
	if(!dest){
		gprintf("Couldn't find teleporter destination\n");
		return;
	}

	teleportentity(other, dest->s.origin, dest->s.angles);
}

/*QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

If spectator is set, only spectators can use this teleport
Spectator teleporters are not normally placed in the editor, but are created
automatically near doors to allow spectators to move through them
*/
void
SP_trigger_teleport(gentity_t *self)
{
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if(self->spawnflags & 1)
		self->r.svFlags |= SVF_NOCLIENT;
	else
		self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	getsoundindex("sound/world/jumppad.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = trigger_teleporter_touch;

	trap_LinkEntity(self);
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF - SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)

*/
void
hurt_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if(self->r.linked)
		trap_UnlinkEntity(self);
	else
		trap_LinkEntity(self);
}

void
hurt_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int dflags;

	if(!other->takedmg)
		return;

	if(self->timestamp > level.time)
		return;

	if(self->spawnflags & 16)
		self->timestamp = level.time + 1000;
	else
		self->timestamp = level.time + FRAMETIME;

	// play sound
	if(!(self->spawnflags & 4))
		mksound(other, CHAN_AUTO, self->noiseindex);

	if(self->spawnflags & 8)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	entdamage(other, self, self, nil, nil, self->damage, dflags, MOD_TRIGGER_HURT);
}

void
SP_trigger_hurt(gentity_t *self)
{
	InitTrigger(self);

	self->noiseindex = getsoundindex("sound/world/electro.wav");
	self->touch = hurt_touch;

	if(!self->damage)
		self->damage = 5;

	self->use = hurt_use;

	// link in to the world if starting active
	if(self->spawnflags & 1)
		trap_UnlinkEntity(self);
	else
		trap_LinkEntity(self);
}

/*
==============================================================================

timer

==============================================================================
*/

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void
func_timer_think(gentity_t *self)
{
	usetargets(self, self->activator);
	// set time before next firing
	self->nextthink = level.time + 1000 * (self->wait + crandom() * self->random);
}

void
func_timer_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;

	// if on, turn it off
	if(self->nextthink){
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think(self);
}

void
SP_func_timer(gentity_t *self)
{
	spawnfloat("random", "1", &self->random);
	spawnfloat("wait", "1", &self->wait);

	self->use = func_timer_use;
	self->think = func_timer_think;

	if(self->random >= self->wait){
		self->random = self->wait - FRAMETIME;
		gprintf("func_timer at %s has random >= wait\n", vtos(self->s.origin));
	}

	if(self->spawnflags & 1){
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}
