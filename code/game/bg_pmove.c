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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t *pm;
pml_t pml;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;

float pm_accelerate = 10.0f;
float pm_airaccelerate = 6.0f;
float pm_grappleaccelerate = 1.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 0.5f;
float pm_flightcontrolfriction = 1.9f;
float pm_spectatorfriction = 5.0f;
float pm_brakefriction = 5.0f;

int c_pmove = 0;

void
pmaddevent(int newEvent)
{
	bgaddpredictableevent(newEvent, 0, pm->ps);
}

void
pmaddtouchent(int entityNum)
{
	int i;

	if(entityNum == ENTITYNUM_WORLD)
		return;
	if(pm->numtouch == MAXTOUCH)
		return;

	// see if it is already added
	for(i = 0; i < pm->numtouch; i++)
		if(pm->touchents[i] == entityNum)
			return;

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

static void
pmbeginanim(int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void
pmfinishanim(int anim)
{
	if((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim)
		return;
	if(pm->ps->torsoTimer > 0)
		return;	// a high priority animation is running
	pmbeginanim(anim);
}

/*
Slide off of the impacting surface
*/
void
pmclipvel(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff;
	float change;
	int i;

	backoff = vecdot(in, normal);

	if(backoff < 0)
		backoff *= overbounce;
	else
		backoff /= overbounce;

	for(i = 0; i<3; i++){
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}

static void
applyfriction(void)
{
	vec3_t vec;
	float *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	veccpy(vel, vec);

	speed = veclen(vec);
	if(speed < 0.01f){
		vecset(vel, 0, 0, 0);
		return;
	}

	drop = 0;

	// apply ground friction
	if(pm->waterlevel <= 1){
		if(pml.walking && !(pml.groundtrace.surfaceFlags & SURF_SLICK))
			// if getting knocked back, no friction
			if(!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)){
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
	}

	// apply water friction even if just wading
	if(pm->waterlevel)
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;

	// apply flying friction
	if(pm->cmd.forwardmove != 0 || pm->cmd.rightmove != 0 ||
	   pm->cmd.upmove != 0)
		drop += speed*pm->ps->airFriction*pml.frametime;
	else
		drop += speed*pm->ps->airIdleFriction*pml.frametime;

	if(pm->ps->pm_type == PM_SPECTATOR)
		drop += speed*pm_spectatorfriction*pml.frametime;

	if(pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		drop = speed*pm->ps->airIdleFriction*pml.frametime;

	// brake if below certain speed and holding no movement buttons
	if(speed < 100.0f && pm->cmd.forwardmove == 0 &&
	   pm->cmd.rightmove == 0 && pm->cmd.upmove == 0 &&
	   !(pm->ps->pm_flags & PMF_GRAPPLE_PULL)){
		drop = speed*pm_brakefriction*pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if(newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vecmul(vel, newspeed, vel);
}

/*
Handles user intended acceleration
*/
static void
pmaccelerate(vec3_t wishdir, float wishspeed, float accel)
{
#if 1
	// the konkrete way, passthrough
	int i;
	float accelspeed;

	accelspeed = accel*pml.frametime*wishspeed;
	vecmad(pm->ps->velocity, accelspeed, wishdir, pm->ps->velocity);

#elif 0
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = vecdot(pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if(addspeed <= 0)
		return;
	accelspeed = accel*pml.frametime*wishspeed;
	if(accelspeed > addspeed)
		accelspeed = addspeed;
	vecmad(pm->ps->velocity, accelspeed, wishdir, pm->ps->velocity);

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;
	float pushLen;
	float canPush;

	vecmul(wishdir, wishspeed, wishVelocity);
	vecsub(wishVelocity, pm->ps->velocity, pushDir);
	pushLen = vecnorm(pushDir);

	canPush = accel*pml.frametime*wishspeed;
	if(canPush > pushLen)
		canPush = pushLen;

	vecmad(pm->ps->velocity, canPush, pushDir, pm->ps->velocity);
#endif
}


static void
pmq2accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	//currentspeed = vecdot(pm->ps->velocity, wishdir);
	//addspeed = wishspeed - currentspeed;
	addspeed = wishspeed;
	if(addspeed <= 0)
		return;
	accelspeed = accel*pml.frametime*wishspeed;
	if(accelspeed > addspeed)
		accelspeed = addspeed;
	vecmad(pm->ps->velocity, accelspeed, wishdir, pm->ps->velocity);
}

/*
Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
*/
static float
cmdscale(usercmd_t *cmd)
{
#if 0	// 1 = allow the speed distortion, 0 = don't
	float scale;

	// max achievable scale with the old code
	scale = 127.0f / (127.0f*127.0f);
	return (float)pm->ps->speed * scale;
#else
	int max;
	float total;
	float scale;

	max = abs(cmd->forwardmove);
	if(abs(cmd->rightmove) > max)
		max = abs(cmd->rightmove);
	if(abs(cmd->upmove) > max)
		max = abs(cmd->upmove);
	if(!max)
		return 0;
	total = sqrt(Square(cmd->forwardmove) + Square(cmd->rightmove) + Square(cmd->upmove));
	scale = max / (127.0f * total);
	return (float)pm->ps->speed * scale;
#endif
}

/*
Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
*/
static float
q3cmdscale(usercmd_t *cmd)
{
	int max;
	float total;
	float scale;

	max = abs(cmd->forwardmove);
	if(abs(cmd->rightmove) > max)
		max = abs(cmd->rightmove);
	if(abs(cmd->upmove) > max)
		max = abs(cmd->upmove);
	if(!max)
		return 0;
	total = sqrt(Square(cmd->forwardmove) + Square(cmd->rightmove) + Square(cmd->upmove));
	scale = max / (127.0f * total);
	return (float)pm->ps->speed * scale;
}

static void
watermove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

#if 0
	// jump = head for surface
	if(pm->cmd.upmove >= 10)
		if(pm->ps->velocity[2] > -300){
			if(pm->watertype & CONTENTS_WATER)
				pm->ps->velocity[2] = 100;
			else if(pm->watertype & CONTENTS_SLIME)
				pm->ps->velocity[2] = 80;
			else
				pm->ps->velocity[2] = 50;
		}

#endif
	applyfriction();

	scale = cmdscale(&pm->cmd);
	// user intentions
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;	// sink towards bottom
	}else{
		for(i = 0; i<3; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);

	if(wishspeed > pm->ps->speed * pm_swimScale)
		wishspeed = pm->ps->speed * pm_swimScale;

	pmaccelerate(wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if(pml.groundplane && vecdot(pm->ps->velocity, pml.groundtrace.plane.normal) < 0){
		vel = veclen(pm->ps->velocity);
		// slide along the ground plane
		pmclipvel(pm->ps->velocity, pml.groundtrace.plane.normal,
				pm->ps->velocity, OVERCLIP);

		vecnorm(pm->ps->velocity);
		vecmul(pm->ps->velocity, vel, pm->ps->velocity);
	}

	pmslidemove(qfalse);
}

static void
specmove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	vec3_t fwd, right, up;
	float scale;

	// normal slowdown
	applyfriction();

	scale = cmdscale(&pm->cmd);
	// user intentions
	if(!scale){
		vecset(wishvel, 0, 0, 0);
	}else{
		vecmul(pml.forward, pm->cmd.forwardmove, fwd);
		vecmul(pml.right, pm->cmd.rightmove, right);
		vecmul(pml.up, pm->cmd.upmove, up);
		vecadd(fwd, right, wishvel);
		vecadd(wishvel, up, wishvel);
	}

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir) * scale;

	pmaccelerate(wishdir, wishspeed, pm_flyaccelerate);
	pmstepslidemove(qfalse);
}

static void
airmove(void)
{
	vec3_t wishvel, wishdir;
	vec3_t fwd, right, up;
	vec3_t dodge;
	float wishspeed;
	float fwdspeed, rightspeed, upspeed;
	const float tolerance = 250;		// dodge tolerance in u/s

	applyfriction();

	vecmul(pml.forward, pm->cmd.forwardmove, fwd);
	vecmul(pml.right, pm->cmd.rightmove, right);
	vecmul(pml.up, pm->cmd.upmove, up);

	//
	// calc regular movement
	//

	vecadd(fwd, right, wishvel);
	vecadd(wishvel, up, wishvel);

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir) * cmdscale(&pm->cmd);

	//
	// calc dodge
	//
	// if the player is not moving fast enough along one of their
	// fwd/right/up dirs, and is holding the key to go in that dir,
	// then give a boost along that dir
	//

	vecset(dodge, 0, 0, 0);

	// project current vel onto player's fwd/right/up vectors to get the
	// speed in those dirs
	fwdspeed = vecdot(pm->ps->velocity, pml.forward);
	rightspeed = vecdot(pm->ps->velocity, pml.right);
	upspeed = vecdot(pm->ps->velocity, pml.up);

	if((pm->cmd.forwardmove > 0 && fwdspeed < tolerance) ||
	   (pm->cmd.forwardmove < 0 && fwdspeed > -tolerance))
		vecmad(dodge, 5, fwd, dodge);
	if((pm->cmd.rightmove > 0 && rightspeed < tolerance) ||
	   (pm->cmd.rightmove < 0 && rightspeed > -tolerance))
		vecmad(dodge, 5, right, dodge);
	if((pm->cmd.upmove > 0 && upspeed < tolerance) ||
	   (pm->cmd.upmove < 0 && upspeed > -tolerance))
		vecmad(dodge, 5, up, dodge);

	// accelerate normally towards wishdir
	pmaccelerate(wishdir, wishspeed, pm->ps->airAccel);
	// add the dodge boost to velocity
	vecmad(pm->ps->velocity, pml.frametime * pm->ps->airAccel, dodge, pm->ps->velocity);
	
	//
	// calc air control
	//
	// straighten out towards wishdir if player is only holding +forward
	// or +back
	//

	if(pm->cmd.forwardmove != 0 && pm->cmd.rightmove == 0 &&
	   pm->cmd.upmove == 0 && wishspeed != 0){
		float currspeed, d, airctl;

		currspeed = veclen(pm->ps->velocity);
		d = vecdot(pm->ps->velocity, wishdir);
		if(d > 0){
			// add scaled influence from wishdir, conserving the
			// original total speed
			airctl = pml.frametime * 1.5f * d;
			vecmad(pm->ps->velocity, airctl, wishdir, pm->ps->velocity);
			vecnorm(pm->ps->velocity);
			vecmul(pm->ps->velocity, currspeed, pm->ps->velocity);
		}
	}

	// perform the move
	pmslidemove(qtrue);
}

/*
sab's hook
*/
static void
grapplemove(void)
{
	vec3_t v, vel;
	float grspd, vlen;
	const float hookpullspeed = 400.0f;
	const float swingstrength = 20.0f;
	float pullspeedcoef, oldlen;

	//
	// add airmove
	//

	vecnorm(pml.forward);
	vecnorm(pml.right);
	vecnorm(pml.up);

	grspd = hookpullspeed;

	vecmul(pml.forward, -16, v);
	vecadd(pm->ps->grapplePoint, v, v);
	vecsub(v, pm->ps->origin, vel);
	vlen = veclen(vel);
	if(pm->ps->grappleLen == 0)
		oldlen = vlen;
	else
		oldlen = pm->ps->grappleLen;
	if(pm->ps->grappleLen != 0 && vlen > oldlen){
		pullspeedcoef = vlen - oldlen;
		pullspeedcoef *= swingstrength;
		grspd *= pullspeedcoef;
	}
	if(grspd < hookpullspeed)
		grspd = hookpullspeed;
	
	vecnorm(vel);
	pmq2accelerate(vel, grspd, pm_grappleaccelerate);
	airmove();
	pm->ps->grappleLen = vlen;
}

static void
deadmove(void)
{
	float forward;

	if(!pml.walking)
		return;

	// extra friction

	forward = veclen(pm->ps->velocity);
	forward -= 20;
	if(forward <= 0)
		vecclear(pm->ps->velocity);
	else{
		vecnorm(pm->ps->velocity);
		vecmul(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

static void
noclipmove(void)
{
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel, wishdir;
	vec3_t fwd, right, up;
	float wishspeed;
	float scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = veclen(pm->ps->velocity);
	if(speed < 1)
		veccpy(vec3_origin, pm->ps->velocity);
	else{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if(newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		vecmul(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = cmdscale(&pm->cmd);

	vecmul(pml.forward, pm->cmd.forwardmove, fwd);
	vecmul(pml.right, pm->cmd.rightmove, right);
	vecmul(pml.up, pm->cmd.upmove, up);
	vecadd(fwd, right, wishvel);
	vecadd(wishvel, up, wishvel);

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir) * 2;
	wishspeed *= scale;

	pmaccelerate(wishdir, wishspeed, pm_accelerate);

	// move
	vecmad(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

/*
FIXME: avoid this twice?  certainly if not moving
*/
static void
setwaterlevel(void)
{
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	// get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum);

	if(cont & MASK_WATER){
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if(cont & MASK_WATER){
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if(cont & MASK_WATER)
				pm->waterlevel = 3;
		}
	}
}

static void
setplayerbounds(void)
{
	vecset(pm->mins, MINS_X, MINS_Y, MINS_Z);
	vecset(pm->maxs, MAXS_X, MAXS_Y, MAXS_Z);
	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
}

/*
Generate sound events for entering and leaving water
*/
static void
waterevents(void)	// FIXME?
{	//
	// if just entered a water volume, play a sound
	if(!pml.prevwaterlevel && pm->waterlevel)
		pmaddevent(EV_WATER_TOUCH);

	// if just completely exited a water volume, play a sound
	if(pml.prevwaterlevel && !pm->waterlevel)
		pmaddevent(EV_WATER_LEAVE);

	// check for head just going under water
	if(pml.prevwaterlevel != 3 && pm->waterlevel == 3)
		pmaddevent(EV_WATER_UNDER);

	// check for head just coming out of water
	if(pml.prevwaterlevel == 3 && pm->waterlevel != 3)
		pmaddevent(EV_WATER_CLEAR);
}

static void
pmbeginweapchange(int slot, int weapon)
{
	if(weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
		return;

	if(!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)))
		return;

	if(pm->ps->weaponstate[slot] == WEAPON_DROPPING)
		return;

	pmaddevent(EV_CHANGE_WEAPON);
	pm->ps->weaponstate[slot] = WEAPON_DROPPING;
	pm->ps->weaponTime[slot] += 50;
	pmbeginanim(TORSO_DROP);
}

static void
pmfinishweapchange(int slot)
{
	int weapon;

	weapon = pm->cmd.weapon[slot];
	if(weapon < WP_NONE || weapon >= WP_NUM_WEAPONS)
		weapon = WP_NONE;

	if(!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)))
		weapon = WP_NONE;

	pm->ps->weapon[slot] = weapon;
	pm->ps->weaponstate[slot] = WEAPON_RAISING;
	pm->ps->weaponTime[slot] += 50;
	pmbeginanim(TORSO_RAISE);
}

static void
pmtorsoanim(void)
{
	if(pm->ps->weaponstate[0] == WEAPON_READY){
		if(pm->ps->weapon[0] == WP_GAUNTLET)
			pmfinishanim(TORSO_STAND2);
		else
			pmfinishanim(TORSO_STAND);
		return;
	}
}

/*
Generates weapon events and modifes the weapon counters
*/
static void
pmweapevents(int slot)
{
	int addTime;
	int button;

	// don't allow attack until all buttons are up
	if(pm->ps->pm_flags & PMF_RESPAWNED)
		return;

	// ignore if spectator
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	// check for dead player
	if(pm->ps->stats[STAT_HEALTH] <= 0){
		pm->ps->weapon[slot] = WP_NONE;
		return;
	}

	// check for item using
	if(pm->cmd.buttons & BUTTON_USE_HOLDABLE){
		if(!(pm->ps->pm_flags & PMF_USE_ITEM_HELD)){
			if(bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].tag == HI_MEDKIT
			   && pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25)){
				// don't use medkit if at max health
			}else{
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				pmaddevent(EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].tag);
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	}else
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;

	// make weapon function
	if(pm->ps->weaponTime[slot] > 0)
		pm->ps->weaponTime[slot] -= pml.msec;

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if(pm->ps->weaponTime[slot] <= 0 || pm->ps->weaponstate[slot] != WEAPON_FIRING)
		if(pm->ps->weapon[slot] != pm->cmd.weapon[slot])
			pmbeginweapchange(slot, pm->cmd.weapon[slot]);

	if(pm->ps->weaponTime[slot] > 0)
		return;

	// change weapon if time
	if(pm->ps->weaponstate[slot] == WEAPON_DROPPING){
		pmfinishweapchange(slot);
		return;
	}

	if(pm->ps->weaponstate[slot] == WEAPON_RAISING){
		pm->ps->weaponstate[slot] = WEAPON_READY;
		if(pm->ps->weapon[slot] == WP_GAUNTLET)
			pmbeginanim(TORSO_STAND2);
		else
			pmbeginanim(TORSO_STAND);
		return;
	}

	// can't fire during warmup
	if(pm->ps->pm_flags & PMF_WARMUP)
		return;

	// check for fire
	switch(slot){
	case 1:
		button = BUTTON_ATTACK2;
		break;
	case 2:
		button = BUTTON_HOOK;
		break;
	default:	// 0
		button = BUTTON_ATTACK;
		break;
	}
	if(!(pm->cmd.buttons & button)){
		pm->ps->weaponTime[slot] = 0;
		pm->ps->weaponstate[slot] = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if(pm->ps->weapon[slot] == WP_GAUNTLET){
		// the guantlet only "fires" when it actually hits something
		if(!pm->gauntlethit){
			pm->ps->weaponTime[slot] = 0;
			pm->ps->weaponstate[slot] = WEAPON_READY;
			return;
		}
		pmbeginanim(TORSO_ATTACK2);
	}else
		pmbeginanim(TORSO_ATTACK);

	if(pm->ps->weapon[slot] == WP_HOMING_LAUNCHER &&
	   (pm->ps->lockontarget == ENTITYNUM_NONE ||
	   pm->ps->lockontime - pm->ps->lockonstarttime < HOMING_SCANWAIT))
		return;

	pm->ps->weaponstate[slot] = WEAPON_FIRING;

	// check for out of ammo
	if(!pm->ps->ammo[pm->ps->weapon[slot]]){
		pmaddevent(EV_NOAMMO);
		pm->ps->weaponTime[slot] += 200;
		return;
	}

	// take an ammo away if not infinite
	if(pm->ps->ammo[pm->ps->weapon[slot]] != -1)
		pm->ps->ammo[pm->ps->weapon[slot]]--;

	// fire weapon
	switch(slot){
	case 1:
		pmaddevent(EV_FIRE_WEAPON2);
		break;
	case 2:
		pmaddevent(EV_FIRE_WEAPON3);
		break;
	default:	// 0
		pmaddevent(EV_FIRE_WEAPON);
		break;
	}

	switch(pm->ps->weapon[slot]){
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_MACHINEGUN:
		addTime = 1000/20.0f;
		break;
	case WP_GRENADE_LAUNCHER:
		addTime = 800;
		break;
	case WP_ROCKET_LAUNCHER:
		addTime = 800;
		break;
	case WP_PLASMAGUN:
		addTime = 100;
		break;
	case WP_RAILGUN:
		addTime = 1500;
		break;
	case WP_BFG:
		addTime = 200;
		break;
	case WP_GRAPPLING_HOOK:
		addTime = 1;
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 64;
		break;
	case WP_PROX_LAUNCHER:
		addTime = 800;
		break;
	case WP_CHAINGUN:
		addTime = 30;
		break;
#endif
	case WP_HOMING_LAUNCHER:
		addTime = 500;
		break;
	}

#ifdef MISSIONPACK
	if(bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].tag == PW_SCOUT)
		addTime /= 1.5;
	else
	if(bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].tag == PW_AMMOREGEN)
		addTime /= 1.3;
	else
#endif
	if(pm->ps->powerups[PW_HASTE])
		addTime /= 1.3;

	pm->ps->weaponTime[slot] += addTime;
}

static void
pmanimate(void)
{
	if(pm->cmd.buttons & BUTTON_GESTURE){
		if(pm->ps->torsoTimer == 0){
			pmbeginanim(TORSO_GESTURE);
			pm->ps->torsoTimer = TIMER_GESTURE;
			pmaddevent(EV_TAUNT);
		}
	}
}

static void
advancetimers(void)
{
	// drop misc timing counter
	if(pm->ps->pm_time){
		if(pml.msec >= pm->ps->pm_time){
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}else
			pm->ps->pm_time -= pml.msec;
	}

	// drop animation counter
	if(pm->ps->legsTimer > 0){
		pm->ps->legsTimer -= pml.msec;
		if(pm->ps->legsTimer < 0)
			pm->ps->legsTimer = 0;
	}

	if(pm->ps->torsoTimer > 0){
		pm->ps->torsoTimer -= pml.msec;
		if(pm->ps->torsoTimer < 0)
			pm->ps->torsoTimer = 0;
	}
}

/*
This can be used as another entry point when only the viewangles
are being updated instead of a full move
*/
void
updateviewangles(playerState_t *ps, const usercmd_t *cmd)
{
	short temp;
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION)
		return;	// no view changes at all

	if(ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0)
		return;	// no view changes at all

	// circularly clamp the angles with deltas
	for(i = 0; i<3; i++){
		temp = cmd->angles[i]; //+ ps->delta_angles[i];
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

void
PmoveSingle(pmove_t *pmove)
{
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if(pm->ps->stats[STAT_HEALTH] <= 0)
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if(abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
		pm->cmd.buttons &= ~BUTTON_WALKING;

	// set the talk balloon flag
	if(pm->cmd.buttons & BUTTON_TALK)
		pm->ps->eFlags |= EF_TALK;
	else
		pm->ps->eFlags &= ~EF_TALK;

	// set the firing flag for continuous beam weapons
	if(!(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && pm->ps->pm_type != PM_NOCLIP){
		if(((pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->ammo[pm->ps->weapon[0]]))
			pm->ps->eFlags |= EF_FIRING;
		else
			pm->ps->eFlags &= ~EF_FIRING;
		if((pm->cmd.buttons & BUTTON_ATTACK2) && pm->ps->ammo[pm->ps->weapon[1]])
			pm->ps->eFlags |= EF_FIRING2;
		else
			pm->ps->eFlags &= ~EF_FIRING2;
		if((pm->cmd.buttons & BUTTON_HOOK) && pm->ps->ammo[pm->ps->weapon[2]])
			pm->ps->eFlags |= EF_FIRING3;
		else
			pm->ps->eFlags &= ~EF_FIRING3;
	}

	// clear the respawned flag if attack and use are cleared
	if(pm->ps->stats[STAT_HEALTH] > 0 &&
	   !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ATTACK2 | BUTTON_HOOK | BUTTON_USE_HOLDABLE)))
		pm->ps->pm_flags &= ~PMF_RESPAWNED;

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if(pmove->cmd.buttons & BUTTON_TALK){
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1)
		pml.msec = 1;
	else if(pml.msec > 200)
		pml.msec = 200;
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	veccpy(pm->ps->origin, pml.prevorigin);

	// save old velocity for crashlanding
	veccpy(pm->ps->velocity, pml.prevvelocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	updateviewangles(pm->ps, &pm->cmd);

	anglevecs(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if(pm->cmd.upmove < 10)
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;

	// decide if backpedaling animations should be used
	if(pm->cmd.forwardmove < 0)
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	else if(pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove))
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;

	if(pm->ps->pm_type >= PM_DEAD){
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if(pm->ps->pm_type == PM_SPECTATOR){
		setplayerbounds();
		specmove();
		advancetimers();
		return;
	}

	if(pm->ps->pm_type == PM_NOCLIP){
		noclipmove();
		advancetimers();
		return;
	}

	if(pm->ps->pm_type == PM_FREEZE)
		return;	// no movement at all

	if(pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION)
		return;	// no movement at all

	// set watertype, and waterlevel
	setwaterlevel();
	pml.prevwaterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	setplayerbounds();

	if(pm->ps->pm_type == PM_DEAD)
		deadmove();

	advancetimers();

	if(pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		grapplemove();
	else if(pm->waterlevel > 1)
		// swimming
		watermove();
	else
		// airborne
		airmove();

	if(!(pm->ps->pm_flags & PMF_GRAPPLE_PULL))
		pm->ps->grappleLen = 0;

	pmanimate();

	// watertype, and waterlevel
	setwaterlevel();

	// weapons
	pmweapevents(0);
	pmweapevents(1);
	pmweapevents(2);

	// torso animation
	pmtorsoanim();

	// entering / leaving water splashes
	waterevents();
}

/*
Can be called by either the server or the client
*/
void
Pmove(pmove_t *pmove)
{
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if(finalTime < pmove->ps->commandTime)
		return;	// should not happen

	if(finalTime > pmove->ps->commandTime + 1000)
		pmove->ps->commandTime = finalTime - 1000;

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while(pmove->ps->commandTime != finalTime){
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if(pmove->pmove_fixed){
			if(msec > pmove->pmove_msec)
				msec = pmove->pmove_msec;
		}else    if(msec > 66)
			msec = 66;

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);
	}

	pm->ps->forwardmove = pm->cmd.forwardmove;
	pm->ps->rightmove = pm->cmd.rightmove;
	pm->ps->upmove = pm->cmd.upmove;
}
