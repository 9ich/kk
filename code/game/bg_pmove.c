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
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 0.5f;
float pm_flightcontrolfriction = 1.9f;
float pm_spectatorfriction = 5.0f;

int c_pmove = 0;

/*
===============
pmaddevent

===============
*/
void
pmaddevent(int newEvent)
{
	bgaddpredictableevent(newEvent, 0, pm->ps);
}

/*
===============
pmaddtouchent
===============
*/
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

/*
===================
PM_StartTorsoAnim
===================
*/
static void
PM_StartTorsoAnim(int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
			    | anim;
}

static void
PM_ContinueTorsoAnim(int anim)
{
	if((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim)
		return;
	if(pm->ps->torsoTimer > 0)
		return;	// a high priority animation is running
	PM_StartTorsoAnim(anim);
}

/*
==================
pmclipvel

Slide off of the impacting surface
==================
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

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void
PM_Friction(void)
{
	vec3_t vec;
	float *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	veccpy(vel, vec);
	if(pml.walking)
		vec[2] = 0;	// ignore slope movement

	speed = veclen(vec);
	if(speed < 0.01f){
		vel[0] = 0;
		vel[1] = 0;
		vel[2] = 0;
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
	if(pm->cmd.forwardmove != 0 || pm->cmd.rightmove != 0 || pm->cmd.upmove != 0)
		drop += speed*pm->ps->airFriction*pml.frametime;
	else
		drop += speed*pm->ps->airIdleFriction*pml.frametime;

	if(pm->ps->pm_type == PM_SPECTATOR)
		drop += speed*pm_spectatorfriction*pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if(newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void
PM_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
#if 1
	int i;
	float accelspeed;

	accelspeed = accel*pml.frametime*wishspeed;
	for(i = 0; i<3; i++)
		pm->ps->velocity[i] += accelspeed*wishdir[i];

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

static float
PM_CmdScale(usercmd_t *cmd)
{
	float scale;

	// max achievable scale with the old code
	scale = 127.0f / (127.0f*127.0f);
	return (float)pm->ps->speed * scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void
PM_SetMovementDir(void)
{
	if(pm->cmd.forwardmove || pm->cmd.rightmove){
		if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 0;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 1;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 2;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 3;
		else if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 4;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 5;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 6;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 7;
	}else{
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if(pm->ps->movementDir == 2)
			pm->ps->movementDir = 1;
		else if(pm->ps->movementDir == 6)
			pm->ps->movementDir = 7;
	}
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean
PM_CheckWaterJump(void)
{
	vec3_t spot;
	int cont;
	vec3_t flatforward;

	if(pm->ps->pm_time)
		return qfalse;

	// check for water jump
	if(pm->waterlevel != 2)
		return qfalse;

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	vecnorm(flatforward);

	vecmad(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(!(cont & CONTENTS_SOLID))
		return qfalse;

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY))
		return qfalse;

	// jump out of water
	vecmul(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void
PM_WaterJumpMove(void)
{
	// waterjump has no control, but falls

	pmstepslidemove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if(pm->ps->velocity[2] < 0){
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void
PM_WaterMove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

	if(PM_CheckWaterJump()){
		PM_WaterJumpMove();
		return;
	}
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
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
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

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);

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

#ifdef MISSIONPACK
/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void
PM_InvulnerabilityMove(void)
{
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	vecclear(pm->ps->velocity);
}

#endif

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void
PM_FlyMove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	// user intentions
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}else{
		for(i = 0; i<3; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	pmstepslidemove(qfalse);
}

static void
_airmove(usercmd_t *cmd, float *wvel, float *wdir, float *wspeed)
{
	int i;
	float fm, sm, um, scale, wishspeed;
	vec3_t wishvel;

	fm = cmd->forwardmove;
	sm = cmd->rightmove;
	um = cmd->upmove;
	wishspeed = 0.0f;
	scale = PM_CmdScale(cmd);
	PM_SetMovementDir();
	vecnorm(pml.forward);
	vecnorm(pml.right);
	vecnorm(pml.up);
	for(i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i]*fm + pml.right[i]*sm + pml.up[i]*um;
	wishspeed = veclen(wishvel);
	if(wvel != nil)
		veccpy(wishvel, wvel);
	if(wdir != nil){
		veccpy(wishvel, wdir);
		vecnorm(wdir);
	}
	if(wspeed != nil)
		*wspeed = wishspeed * scale;
}

/*
===================
PM_AirMove

===================
*/
static void
PM_AirMove(void)
{
	vec3_t wishvel, wishdir;
	float wishspeed;

	PM_Friction();
	_airmove(&pm->cmd, wishvel, wishdir, &wishspeed);
	PM_Accelerate(wishdir, wishspeed, pm->ps->airAccel);
	pmslidemove(qtrue);
}

/*
===================
PM_GrappleMove

===================
*/
static void
PM_GrappleMove(void)
{
	vec3_t vel, v;
	float vlen;

	vecmul(pml.forward, -16, v);
	vecadd(pm->ps->grapplePoint, v, v);
	vecsub(v, pm->ps->origin, vel);
	vlen = veclen(vel);
	vecnorm(vel);

	if(vlen <= 100)
		vecmul(vel, 10 * vlen, vel);
	else
		vecmul(vel, 800, vel);

	veccpy(vel, pm->ps->velocity);

	pml.groundplane = qfalse;
}

/*
==============
PM_DeadMove
==============
*/
static void
PM_DeadMove(void)
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

/*
===============
PM_NoclipMove
===============
*/
static void
PM_NoclipMove(void)
{
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
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
	scale = PM_CmdScale(&pm->cmd);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for(i = 0; i<3; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	veccpy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	vecmad(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void
PM_SetWaterLevel(void)
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

//===================================================================

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void
PM_WaterEvents(void)	// FIXME?
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

/*
===============
PM_BeginWeaponChange
===============
*/
static void
PM_BeginWeaponChange(int slot, int weapon)
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
	PM_StartTorsoAnim(TORSO_DROP);
}

/*
===============
PM_FinishWeaponChange
===============
*/
static void
PM_FinishWeaponChange(int slot)
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
	PM_StartTorsoAnim(TORSO_RAISE);
}

/*
==============
PM_TorsoAnimation

==============
*/
static void
PM_TorsoAnimation(void)
{
	if(pm->ps->weaponstate[0] == WEAPON_READY){
		if(pm->ps->weapon[0] == WP_GAUNTLET)
			PM_ContinueTorsoAnim(TORSO_STAND2);
		else
			PM_ContinueTorsoAnim(TORSO_STAND);
		return;
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void
PM_Weapon(int slot)
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
			PM_BeginWeaponChange(slot, pm->cmd.weapon[slot]);

	if(pm->ps->weaponTime[slot] > 0)
		return;

	// change weapon if time
	if(pm->ps->weaponstate[slot] == WEAPON_DROPPING){
		PM_FinishWeaponChange(slot);
		return;
	}

	if(pm->ps->weaponstate[slot] == WEAPON_RAISING){
		pm->ps->weaponstate[slot] = WEAPON_READY;
		if(pm->ps->weapon[slot] == WP_GAUNTLET)
			PM_StartTorsoAnim(TORSO_STAND2);
		else
			PM_StartTorsoAnim(TORSO_STAND);
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
		PM_StartTorsoAnim(TORSO_ATTACK2);
	}else
		PM_StartTorsoAnim(TORSO_ATTACK);

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
		addTime = 1000/20.0;
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
		addTime = 400;
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 120;
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

/*
================
PM_Animate
================
*/

static void
PM_Animate(void)
{
	if(pm->cmd.buttons & BUTTON_GESTURE){
		if(pm->ps->torsoTimer == 0){
			PM_StartTorsoAnim(TORSO_GESTURE);
			pm->ps->torsoTimer = TIMER_GESTURE;
			pmaddevent(EV_TAUNT);
		}
	}
}

/*
================
PM_DropTimers
================
*/
static void
PM_DropTimers(void)
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
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void
PM_UpdateViewAngles(playerState_t *ps, const usercmd_t *cmd)
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

/*
================
PmoveSingle

================
*/
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
	PM_UpdateViewAngles(pm->ps, &pm->cmd);

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
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if(pm->ps->pm_type == PM_NOCLIP){
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if(pm->ps->pm_type == PM_FREEZE)
		return;	// no movement at all

	if(pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION)
		return;	// no movement at all

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.prevwaterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	setplayerbounds();

	if(pm->ps->pm_type == PM_DEAD)
		PM_DeadMove();

	PM_DropTimers();

#ifdef MISSIONPACK
	if(pm->ps->powerups[PW_INVULNERABILITY])
		PM_InvulnerabilityMove();
	else
#endif
	if(pm->ps->powerups[PW_FLIGHT])
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	else if(pm->ps->pm_flags & PMF_GRAPPLE_PULL){
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	}else if(pm->ps->pm_flags & PMF_TIME_WATERJUMP)
		PM_WaterJumpMove();
	else if(pm->waterlevel > 1)
		// swimming
		PM_WaterMove();
	else
		// airborne
		PM_AirMove();

	PM_Animate();

	// watertype, and waterlevel
	PM_SetWaterLevel();

	// weapons
	PM_Weapon(0);
	PM_Weapon(1);
	PM_Weapon(2);

	// torso animation
	PM_TorsoAnimation();

	// entering / leaving water splashes
	PM_WaterEvents();
}

/*
================
Pmove

Can be called by either the server or the client
================
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
	//PM_CheckStuck();
}
