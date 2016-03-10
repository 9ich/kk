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
//
// ui_players.c

#include "ui_local.h"

#define UI_TIMER_GESTURE	2300
#define UI_TIMER_JUMP		1000
#define UI_TIMER_LAND		130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK		500
#define UI_TIMER_MUZZLE_FLASH	20
#define UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT		56

#define SWINGSPEED		0.3f

#define SPIN_SPEED		0.9f
#define COAST_TIME		1000

static int dp_realtime;
static float jumpHeight;
sfxHandle_t weaponChangeSound;

/*
===============
UI_PlayerInfo_SetWeapon
===============
*/
static void
UI_PlayerInfo_SetWeapon(playerInfo_t *pi, weapon_t weaponNum)
{
	gitem_t * item;
	char path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
 tryagain:
	pi->realWeapon = weaponNum;
	pi->model = 0;
	pi->barrelmodel = 0;
	pi->flashmodel = 0;

	if(weaponNum == WP_NONE)
		return;

	for(item = bg_itemlist + 1; item->classname; item++){
		if(item->type != IT_WEAPON)
			continue;
		if(item->tag == weaponNum)
			break;
	}

	if(item->classname)
		pi->model = trap_R_RegisterModel(item->model[0]);

	if(pi->model == 0){
		if(weaponNum == WP_MACHINEGUN){
			weaponNum = WP_NONE;
			goto tryagain;
		}
		weaponNum = WP_MACHINEGUN;
		goto tryagain;
	}

	if(weaponNum == WP_MACHINEGUN || weaponNum == WP_GAUNTLET || weaponNum == WP_BFG){
		COM_StripExtension(item->model[0], path, sizeof(path));
		Q_strcat(path, sizeof(path), "_barrel.md3");
		pi->barrelmodel = trap_R_RegisterModel(path);
	}

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_flash.md3");
	pi->flashmodel = trap_R_RegisterModel(path);

	switch(weaponNum){
	case WP_GAUNTLET:
		MAKERGB(pi->flashcolor, 0.6f, 0.6f, 1);
		break;

	case WP_MACHINEGUN:
		MAKERGB(pi->flashcolor, 1, 1, 0);
		break;

	case WP_SHOTGUN:
		MAKERGB(pi->flashcolor, 1, 1, 0);
		break;

	case WP_GRENADE_LAUNCHER:
		MAKERGB(pi->flashcolor, 1, 0.7f, 0.5f);
		break;

	case WP_ROCKET_LAUNCHER:
		MAKERGB(pi->flashcolor, 1, 0.75f, 0);
		break;

	case WP_LIGHTNING:
		MAKERGB(pi->flashcolor, 0.6f, 0.6f, 1);
		break;

	case WP_RAILGUN:
		MAKERGB(pi->flashcolor, 1, 0.5f, 0);
		break;

	case WP_PLASMAGUN:
		MAKERGB(pi->flashcolor, 0.6f, 0.6f, 1);
		break;

	case WP_BFG:
		MAKERGB(pi->flashcolor, 1, 0.7f, 1);
		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(pi->flashcolor, 0.6f, 0.6f, 1);
		break;

	default:
		MAKERGB(pi->flashcolor, 1, 1, 1);
		break;
	}
}

/*
===============
UI_ForceLegsAnim
===============
*/
static void
UI_ForceLegsAnim(playerInfo_t *pi, int anim)
{
	pi->legsAnim = ((pi->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if(anim == LEGS_JUMP)
		pi->legsAnimationTimer = UI_TIMER_JUMP;
}

/*
===============
UI_SetLegsAnim
===============
*/
static void
UI_SetLegsAnim(playerInfo_t *pi, int anim)
{
	if(pi->pendingLegsAnim){
		anim = pi->pendingLegsAnim;
		pi->pendingLegsAnim = 0;
	}
	UI_ForceLegsAnim(pi, anim);
}

/*
===============
UI_ForceTorsoAnim
===============
*/
static void
UI_ForceTorsoAnim(playerInfo_t *pi, int anim)
{
	pi->torsoAnim = ((pi->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if(anim == TORSO_GESTURE)
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;

	if(anim == TORSO_ATTACK || anim == TORSO_ATTACK2)
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
}

/*
===============
UI_SetTorsoAnim
===============
*/
static void
UI_SetTorsoAnim(playerInfo_t *pi, int anim)
{
	if(pi->pendingTorsoAnim){
		anim = pi->pendingTorsoAnim;
		pi->pendingTorsoAnim = 0;
	}

	UI_ForceTorsoAnim(pi, anim);
}

/*
===============
UI_TorsoSequencing
===============
*/
static void
UI_TorsoSequencing(playerInfo_t *pi)
{
	int currentAnim;

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if(pi->weapon != pi->currentWeapon)
		if(currentAnim != TORSO_DROP){
			pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
			UI_ForceTorsoAnim(pi, TORSO_DROP);
		}

	if(pi->torsoAnimationTimer > 0)
		return;

	if(currentAnim == TORSO_GESTURE){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if(currentAnim == TORSO_ATTACK || currentAnim == TORSO_ATTACK2){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if(currentAnim == TORSO_DROP){
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
		UI_ForceTorsoAnim(pi, TORSO_RAISE);
		return;
	}

	if(currentAnim == TORSO_RAISE){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}
}

/*
===============
UI_LegsSequencing
===============
*/
static void
UI_LegsSequencing(playerInfo_t *pi)
{
	int currentAnim;

	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;

	if(pi->legsAnimationTimer > 0){
		if(currentAnim == LEGS_JUMP)
			jumpHeight = JUMP_HEIGHT * sin(M_PI * (UI_TIMER_JUMP - pi->legsAnimationTimer) / UI_TIMER_JUMP);
		return;
	}

	if(currentAnim == LEGS_JUMP){
		UI_ForceLegsAnim(pi, LEGS_LAND);
		pi->legsAnimationTimer = UI_TIMER_LAND;
		jumpHeight = 0;
		return;
	}

	if(currentAnim == LEGS_LAND){
		UI_SetLegsAnim(pi, LEGS_IDLE);
		return;
	}
}

/*
======================
UI_PositionEntityOnTag
======================
*/
static void
UI_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent,
		       clipHandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
			1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// cast away const because of compiler problems
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
======================
UI_PositionRotatedEntityOnTag
======================
*/
static void
UI_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent,
			      clipHandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

	// lerp the tag
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
			1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// cast away const because of compiler problems
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t*)parent)->axis, entity->axis);
}

/*
===============
UI_SetLerpFrameAnimation
===============
*/
static void
UI_SetLerpFrameAnimation(playerInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;

	lf->animnum = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if(newAnimation < 0 || newAnimation >= MAX_ANIMATIONS)
		trap_Error(va("Bad animation number: %i", newAnimation));

	anim = &ci->animations[newAnimation];

	lf->animation = anim;
	lf->animtime = lf->frametime + anim->initiallerp;
}

/*
===============
UI_RunLerpFrame
===============
*/
static void
UI_RunLerpFrame(playerInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	int f;
	animation_t *anim;

	// see if the animation sequence is switching
	if(newAnimation != lf->animnum || !lf->animation)
		UI_SetLerpFrameAnimation(ci, lf, newAnimation);

	// if we have passed the current frame, move it to
	// oldframe and calculate a new frame
	if(dp_realtime >= lf->frametime){
		lf->oldframe = lf->frame;
		lf->oldframetime = lf->frametime;

		// get the next frame based on the animation
		anim = lf->animation;
		if(dp_realtime < lf->animtime)
			lf->frametime = lf->animtime;	// initial lerp
		else
			lf->frametime = lf->oldframetime + anim->framelerp;
		f = (lf->frametime - lf->animtime) / anim->framelerp;
		if(f >= anim->nframes){
			f -= anim->nframes;
			if(anim->loopframes){
				f %= anim->loopframes;
				f += anim->nframes - anim->loopframes;
			}else{
				f = anim->nframes - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frametime = dp_realtime;
			}
		}
		lf->frame = anim->firstframe + f;
		if(dp_realtime > lf->frametime)
			lf->frametime = dp_realtime;
	}

	if(lf->frametime > dp_realtime + 200)
		lf->frametime = dp_realtime;

	if(lf->oldframetime > dp_realtime)
		lf->oldframetime = dp_realtime;
	// calculate current lerp value
	if(lf->frametime == lf->oldframetime)
		lf->backlerp = 0;
	else
		lf->backlerp = 1.0 - (float)(dp_realtime - lf->oldframetime) / (lf->frametime - lf->oldframetime);
}

/*
===============
UI_PlayerAnimation
===============
*/
static void
UI_PlayerAnimation(playerInfo_t *pi, int *legsOld, int *legs, float *legsBackLerp,
		   int *torsoOld, int *torso, float *torsoBackLerp)
{
	// legs animation
	pi->legsAnimationTimer -= uiInfo.uiDC.frametime;
	if(pi->legsAnimationTimer < 0)
		pi->legsAnimationTimer = 0;

	UI_LegsSequencing(pi);

	if(pi->legs.yawing && (pi->legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE)
		UI_RunLerpFrame(pi, &pi->legs, LEGS_TURN);
	else
		UI_RunLerpFrame(pi, &pi->legs, pi->legsAnim);
	*legsOld = pi->legs.oldframe;
	*legs = pi->legs.frame;
	*legsBackLerp = pi->legs.backlerp;

	// torso animation
	pi->torsoAnimationTimer -= uiInfo.uiDC.frametime;
	if(pi->torsoAnimationTimer < 0)
		pi->torsoAnimationTimer = 0;

	UI_TorsoSequencing(pi);

	UI_RunLerpFrame(pi, &pi->torso, pi->torsoAnim);
	*torsoOld = pi->torso.oldframe;
	*torso = pi->torso.frame;
	*torsoBackLerp = pi->torso.backlerp;
}

/*
==================
UI_SwingAngles
==================
*/
static void
UI_SwingAngles(float destination, float swingTolerance, float clampTolerance,
	       float speed, float *angle, qboolean *swinging)
{
	float swing;
	float move;
	float scale;

	if(!*swinging){
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if(swing > swingTolerance || swing < -swingTolerance)
			*swinging = qtrue;
	}

	if(!*swinging)
		return;

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if(scale < swingTolerance * 0.5)
		scale = 0.5;
	else if(scale < swingTolerance)
		scale = 1.0;
	else
		scale = 2.0;

	// swing towards the destination angle
	if(swing >= 0){
		move = uiInfo.uiDC.frametime * scale * speed;
		if(move >= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}else if(swing < 0){
		move = uiInfo.uiDC.frametime * scale * -speed;
		if(move <= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if(swing > clampTolerance)
		*angle = AngleMod(destination - (clampTolerance - 1));
	else if(swing < -clampTolerance)
		*angle = AngleMod(destination + (clampTolerance - 1));
}

/*
======================
UI_MovedirAdjustment
======================
*/
static float
UI_MovedirAdjustment(playerInfo_t *pi)
{
	vec3_t relativeAngles;
	vec3_t moveVector;

	vecsub(pi->viewAngles, pi->moveAngles, relativeAngles);
	anglevecs(relativeAngles, moveVector, nil, nil);
	if(Q_fabs(moveVector[0]) < 0.01)
		moveVector[0] = 0.0;
	if(Q_fabs(moveVector[1]) < 0.01)
		moveVector[1] = 0.0;

	if(moveVector[1] == 0 && moveVector[0] > 0)
		return 0;
	if(moveVector[1] < 0 && moveVector[0] > 0)
		return 22;
	if(moveVector[1] < 0 && moveVector[0] == 0)
		return 45;
	if(moveVector[1] < 0 && moveVector[0] < 0)
		return -22;
	if(moveVector[1] == 0 && moveVector[0] < 0)
		return 0;
	if(moveVector[1] > 0 && moveVector[0] < 0)
		return 22;
	if(moveVector[1] > 0 && moveVector[0] == 0)
		return -45;

	return -22;
}

/*
===============
UI_PlayerAngles
===============
*/
static void
UI_PlayerAngles(playerInfo_t *pi, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t legsAngles, torsoAngles, headAngles;
	float dest;
	float adjust;

	veccpy(pi->viewAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	vecclear(legsAngles);
	vecclear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if((pi->legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
	   || (pi->torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND){
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;	// always center
		pi->torso.pitching = qtrue;	// always center
		pi->legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	adjust = UI_MovedirAdjustment(pi);
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;

	// torso
	UI_SwingAngles(torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yaw, &pi->torso.yawing);
	UI_SwingAngles(legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yaw, &pi->legs.yawing);

	torsoAngles[YAW] = pi->torso.yaw;
	legsAngles[YAW] = pi->legs.yaw;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if(headAngles[PITCH] > 180)
		dest = (-360 + headAngles[PITCH]) * 0.75;
	else
		dest = headAngles[PITCH] * 0.75;
	UI_SwingAngles(dest, 15, 30, 0.1f, &pi->torso.pitch, &pi->torso.pitching);
	torsoAngles[PITCH] = pi->torso.pitch;

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}

/*
===============
UI_PlayerFloatSprite
===============
*/
static void
UI_PlayerFloatSprite(playerInfo_t *pi, vec3_t origin, qhandle_t shader)
{
	refEntity_t ent;

	memset(&ent, 0, sizeof(ent));
	veccpy(origin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene(&ent);
}

/*
======================
UI_MachinegunSpinAngle
======================
*/
float
UI_MachinegunSpinAngle(playerInfo_t *pi)
{
	int delta;
	float angle;
	float speed;
	int torsoAnim;

	delta = dp_realtime - pi->barreltime;
	if(pi->barrelspin)
		angle = pi->barrelangle + delta * SPIN_SPEED;
	else{
		if(delta > COAST_TIME)
			delta = COAST_TIME;

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = pi->barrelangle + delta * speed;
	}

	torsoAnim = pi->torsoAnim  & ~ANIM_TOGGLEBIT;
	if(torsoAnim == TORSO_ATTACK2)
		torsoAnim = TORSO_ATTACK;
	if(pi->barrelspin == !(torsoAnim == TORSO_ATTACK)){
		pi->barreltime = dp_realtime;
		pi->barrelangle = AngleMod(angle);
		pi->barrelspin = !!(torsoAnim == TORSO_ATTACK);
	}

	return angle;
}

/*
===============
UI_DrawPlayer
===============
*/
void
UI_DrawPlayer(float x, float y, float w, float h, playerInfo_t *pi, int time)
{
	refdef_t refdef;
	refEntity_t legs = {0};
	refEntity_t torso = {0};
	refEntity_t head = {0};
	refEntity_t gun = {0};
	refEntity_t barrel = {0};
	refEntity_t flash = {0};
	vec3_t origin;
	int renderfx;
	vec3_t mins = {-16, -16, -24};
	vec3_t maxs = {16, 16, 32};
	float len;
	float xx;

	if(!pi->legsmodel || !pi->torsomodel || !pi->headmodel || !pi->animations[0].nframes)
		return;

	// this allows the ui to cache the player model on the main menu
	if(w == 0 || h == 0)
		return;

	dp_realtime = time;

	if(pi->pendingWeapon != WP_NUM_WEAPONS && dp_realtime > pi->weaponTimer){
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = WP_NUM_WEAPONS;
		pi->weaponTimer = 0;
		if(pi->currentWeapon != pi->weapon)
			trap_S_StartLocalSound(weaponChangeSound, CHAN_LOCAL);
	}

	UI_AdjustFrom640(&x, &y, &w, &h);

	y -= jumpHeight;

	memset(&refdef, 0, sizeof(refdef));
	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / uiInfo.uiDC.xscale / 640.0f * 90.0f);
	xx = refdef.width / uiInfo.uiDC.xscale / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y = atan2(refdef.height / uiInfo.uiDC.yscale, xx);
	refdef.fov_y *= (360 / (float)M_PI);

	// calculate distance so the player nearly fills the box
	len = 0.7 * (maxs[2] - mins[2]);
	origin[0] = len / tan(DEG2RAD(refdef.fov_x) * 0.5);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
	UI_PlayerAngles(pi, legs.axis, torso.axis, head.axis);

	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation(pi, &legs.oldframe, &legs.frame, &legs.backlerp,
			   &torso.oldframe, &torso.frame, &torso.backlerp);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	//
	// add the legs
	//
	legs.hModel = pi->legsmodel;
	legs.customSkin = pi->legsskin;

	veccpy(origin, legs.origin);

	veccpy(origin, legs.lightingOrigin);
	legs.renderfx = renderfx;
	veccpy(legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene(&legs);

	if(!legs.hModel)
		return;

	//
	// add the torso
	//
	torso.hModel = pi->torsomodel;
	if(!torso.hModel)
		return;

	torso.customSkin = pi->torsoskin;

	veccpy(origin, torso.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&torso, &legs, pi->legsmodel, "tag_torso");

	torso.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&torso);

	//
	// add the head
	//
	head.hModel = pi->headmodel;
	if(!head.hModel)
		return;
	head.customSkin = pi->headskin;

	veccpy(origin, head.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&head, &torso, pi->torsomodel, "tag_head");

	head.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&head);

	//
	// add the gun
	//
	if(pi->currentWeapon != WP_NONE){
		memset(&gun, 0, sizeof(gun));
		gun.hModel = pi->model;
		veccpy(origin, gun.lightingOrigin);
		UI_PositionEntityOnTag(&gun, &torso, pi->torsomodel, "tag_weapon");
		gun.renderfx = renderfx;
		trap_R_AddRefEntityToScene(&gun);
	}

	//
	// add the spinning barrel
	//
	if(pi->realWeapon == WP_MACHINEGUN || pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG){
		vec3_t angles;

		memset(&barrel, 0, sizeof(barrel));
		veccpy(origin, barrel.lightingOrigin);
		barrel.renderfx = renderfx;

		barrel.hModel = pi->barrelmodel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = UI_MachinegunSpinAngle(pi);
		AnglesToAxis(angles, barrel.axis);

		UI_PositionRotatedEntityOnTag(&barrel, &gun, pi->model, "tag_barrel");

		trap_R_AddRefEntityToScene(&barrel);
	}

	//
	// add muzzle flash
	//
	if(dp_realtime <= pi->muzzleflashtime){
		if(pi->flashmodel){
			memset(&flash, 0, sizeof(flash));
			flash.hModel = pi->flashmodel;
			veccpy(origin, flash.lightingOrigin);
			UI_PositionEntityOnTag(&flash, &gun, pi->model, "tag_flash");
			flash.renderfx = renderfx;
			trap_R_AddRefEntityToScene(&flash);
		}

		// make a dlight for the flash
		if(pi->flashcolor[0] || pi->flashcolor[1] || pi->flashcolor[2])
			trap_R_AddLightToScene(flash.origin, 200 + (rand()&31), pi->flashcolor[0],
					       pi->flashcolor[1], pi->flashcolor[2]);
	}

	//
	// add the chat icon
	//
	if(pi->chat)
		UI_PlayerFloatSprite(pi, origin, trap_R_RegisterShaderNoMip("sprites/balloon3"));

	//
	// add an accent light
	//
	origin[0] -= 100;	// + = behind, - = in front
	origin[1] += 100;	// + = left, - = right
	origin[2] += 100;	// + = above, - = below
	trap_R_AddLightToScene(origin, 500, 1.0, 1.0, 1.0);

	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene(origin, 500, 1.0, 0.0, 0.0);

	trap_R_RenderScene(&refdef);
}

/*
==========================
UI_FileExists
==========================
*/
static qboolean
UI_FileExists(const char *filename)
{
	int len;

	len = trap_FS_FOpenFile(filename, nil, FS_READ);
	if(len>0)
		return qtrue;
	return qfalse;
}

/*
==========================
UI_FindClientHeadFile
==========================
*/
static qboolean
UI_FindClientHeadFile(char *filename, int length, const char *teamName, const char *headmodelname, const char *headskinname, const char *base, const char *ext)
{
	char *team, *headsFolder;
	int i;

	team = "default";

	if(headmodelname[0] == '*'){
		headsFolder = "heads/";
		headmodelname++;
	}else
		headsFolder = "";
	while(1){
		for(i = 0; i < 2; i++){
			if(i == 0 && teamName && *teamName)
				Com_sprintf(filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headmodelname, headskinname, teamName, base, team, ext);
			else
				Com_sprintf(filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headmodelname, headskinname, base, team, ext);
			if(UI_FileExists(filename))
				return qtrue;
			if(i == 0 && teamName && *teamName)
				Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headmodelname, teamName, base, headskinname, ext);
			else
				Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headmodelname, base, headskinname, ext);
			if(UI_FileExists(filename))
				return qtrue;
			if(!teamName || !*teamName)
				break;
		}
		// if tried the heads folder first
		if(headsFolder[0])
			break;
		headsFolder = "heads/";
	}

	return qfalse;
}

/*
==========================
UI_RegisterClientSkin
==========================
*/
static qboolean
UI_RegisterClientSkin(playerInfo_t *pi, const char *modelname, const char *skinname, const char *headmodelname, const char *headskinname, const char *teamName)
{
	char filename[MAX_QPATH];

	if(teamName && *teamName)
		Com_sprintf(filename, sizeof(filename), "models/players/%s/%s/lower_%s.skin", modelname, teamName, skinname);
	else
		Com_sprintf(filename, sizeof(filename), "models/players/%s/lower_%s.skin", modelname, skinname);
	pi->legsskin = trap_R_RegisterSkin(filename);
	if(!pi->legsskin){
		if(teamName && *teamName)
			Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/%s/lower_%s.skin", modelname, teamName, skinname);
		else
			Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower_%s.skin", modelname, skinname);
		pi->legsskin = trap_R_RegisterSkin(filename);
	}

	if(teamName && *teamName)
		Com_sprintf(filename, sizeof(filename), "models/players/%s/%s/upper_%s.skin", modelname, teamName, skinname);
	else
		Com_sprintf(filename, sizeof(filename), "models/players/%s/upper_%s.skin", modelname, skinname);
	pi->torsoskin = trap_R_RegisterSkin(filename);
	if(!pi->torsoskin){
		if(teamName && *teamName)
			Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/%s/upper_%s.skin", modelname, teamName, skinname);
		else
			Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper_%s.skin", modelname, skinname);
		pi->torsoskin = trap_R_RegisterSkin(filename);
	}

	if(UI_FindClientHeadFile(filename, sizeof(filename), teamName, headmodelname, headskinname, "head", "skin"))
		pi->headskin = trap_R_RegisterSkin(filename);

	if(!pi->legsskin || !pi->torsoskin || !pi->headskin)
		return qfalse;

	return qtrue;
}

/*
======================
UI_ParseAnimationFile
======================
*/
static qboolean
UI_ParseAnimationFile(const char *filename, animation_t *animations)
{
	char *text_p, *prev;
	int len;
	int i;
	char *token;
	float fps;
	int skip;
	char text[20000];
	fileHandle_t f;

	memset(animations, 0, sizeof(animation_t) * MAX_ANIMATIONS);

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
		return qfalse;
	if(len >= (sizeof(text) - 1)){
		Com_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	COM_Compress(text);

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	// read optional parameters
	while(1){
		prev = text_p;	// so we can unget
		token = COM_Parse(&text_p);
		if(!token)
			break;
		if(!Q_stricmp(token, "footsteps")){
			token = COM_Parse(&text_p);
			if(!token)
				break;
			continue;
		}else if(!Q_stricmp(token, "headoffset")){
			for(i = 0; i < 3; i++){
				token = COM_Parse(&text_p);
				if(!token)
					break;
			}
			continue;
		}else if(!Q_stricmp(token, "sex")){
			token = COM_Parse(&text_p);
			if(!token)
				break;
			continue;
		}

		// if it is a number, start parsing animations
		if(token[0] >= '0' && token[0] <= '9'){
			text_p = prev;	// unget the token
			break;
		}

		Com_Printf("unknown token '%s' in %s\n", token, filename);
	}

	// read information for each frame
	for(i = 0; i < MAX_ANIMATIONS; i++){
		token = COM_Parse(&text_p);
		if(!token)
			break;
		animations[i].firstframe = atoi(token);
		// leg only frames are adjusted to not count the upper body only frames
		if(i == LEGS_WALKCR)
			skip = animations[LEGS_WALKCR].firstframe - animations[TORSO_GESTURE].firstframe;
		if(i >= LEGS_WALKCR)
			animations[i].firstframe -= skip;

		token = COM_Parse(&text_p);
		if(!token)
			break;
		animations[i].nframes = atoi(token);

		token = COM_Parse(&text_p);
		if(!token)
			break;
		animations[i].loopframes = atoi(token);

		token = COM_Parse(&text_p);
		if(!token)
			break;
		fps = atof(token);
		if(fps == 0)
			fps = 1;
		animations[i].framelerp = 1000 / fps;
		animations[i].initiallerp = 1000 / fps;
	}

	if(i != MAX_ANIMATIONS){
		Com_Printf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}

/*
==========================
UI_RegisterClientModelname
==========================
*/
qboolean
UI_RegisterClientModelname(playerInfo_t *pi, const char *modelSkinName, const char *headModelSkinName, const char *teamName)
{
	char modelname[MAX_QPATH];
	char skinname[MAX_QPATH];
	char headmodelname[MAX_QPATH];
	char headskinname[MAX_QPATH];
	char filename[MAX_QPATH];
	char *slash;

	pi->torsomodel = 0;
	pi->headmodel = 0;

	if(!modelSkinName[0])
		return qfalse;

	Q_strncpyz(modelname, modelSkinName, sizeof(modelname));

	slash = strchr(modelname, '/');
	if(!slash)
		// modelname did not include a skin name
		Q_strncpyz(skinname, "default", sizeof(skinname));
	else{
		Q_strncpyz(skinname, slash + 1, sizeof(skinname));
		*slash = '\0';
	}

	Q_strncpyz(headmodelname, headModelSkinName, sizeof(headmodelname));
	slash = strchr(headmodelname, '/');
	if(!slash)
		// modelname did not include a skin name
		Q_strncpyz(headskinname, "default", sizeof(skinname));
	else{
		Q_strncpyz(headskinname, slash + 1, sizeof(skinname));
		*slash = '\0';
	}

	// load cmodels before models so filecache works

	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower.md3", modelname);
	pi->legsmodel = trap_R_RegisterModel(filename);
	if(!pi->legsmodel){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower.md3", modelname);
		pi->legsmodel = trap_R_RegisterModel(filename);
		if(!pi->legsmodel){
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper.md3", modelname);
	pi->torsomodel = trap_R_RegisterModel(filename);
	if(!pi->torsomodel){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper.md3", modelname);
		pi->torsomodel = trap_R_RegisterModel(filename);
		if(!pi->torsomodel){
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	if(headmodelname[0] == '*')
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", &headmodelname[1], &headmodelname[1]);
	else
		Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headmodelname);
	pi->headmodel = trap_R_RegisterModel(filename);
	if(!pi->headmodel && headmodelname[0] != '*'){
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", headmodelname, headmodelname);
		pi->headmodel = trap_R_RegisterModel(filename);
	}

	if(!pi->headmodel){
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	// if any skins failed to load, fall back to default
	if(!UI_RegisterClientSkin(pi, modelname, skinname, headmodelname, headskinname, teamName))
		if(!UI_RegisterClientSkin(pi, modelname, "default", headmodelname, "default", teamName)){
			Com_Printf("Failed to load skin file: %s : %s\n", modelname, skinname);
			return qfalse;
		}

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelname);
	if(!UI_ParseAnimationFile(filename, pi->animations)){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/animation.cfg", modelname);
		if(!UI_ParseAnimationFile(filename, pi->animations)){
			Com_Printf("Failed to load animation file %s\n", filename);
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
UI_PlayerInfo_SetModel
===============
*/
void
UI_PlayerInfo_SetModel(playerInfo_t *pi, const char *model, const char *headmodel, char *teamName)
{
	memset(pi, 0, sizeof(*pi));
	UI_RegisterClientModelname(pi, model, headmodel, teamName);
	pi->weapon = WP_MACHINEGUN;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = WP_NUM_WEAPONS;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon(pi, pi->weapon);
}

/*
===============
UI_PlayerInfo_SetInfo
===============
*/
void
UI_PlayerInfo_SetInfo(playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNumber, qboolean chat)
{
	int currentAnim;
	weapon_t weaponNum;

	pi->chat = chat;

	// view angles
	veccpy(viewAngles, pi->viewAngles);

	// move angles
	veccpy(moveAngles, pi->moveAngles);

	if(pi->newModel){
		pi->newModel = qfalse;

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
		pi->legs.yaw = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
		pi->torso.yaw = viewAngles[YAW];
		pi->torso.yawing = qfalse;

		if(weaponNumber != WP_NUM_WEAPONS){
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = WP_NUM_WEAPONS;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		}

		return;
	}

	// weapon
	if(weaponNumber == WP_NUM_WEAPONS){
		pi->pendingWeapon = WP_NUM_WEAPONS;
		pi->weaponTimer = 0;
	}else if(weaponNumber != WP_NONE){
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

	if(torsoAnim == BOTH_DEATH1 || legsAnim == BOTH_DEATH1){
		torsoAnim = legsAnim = BOTH_DEATH1;
		pi->weapon = pi->currentWeapon = WP_NONE;
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);

		return;
	}

	// leg animation
	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	if(legsAnim != LEGS_JUMP && (currentAnim == LEGS_JUMP || currentAnim == LEGS_LAND))
		pi->pendingLegsAnim = legsAnim;
	else if(legsAnim != currentAnim){
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
	}

	// torso animation
	if(torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2){
		if(weaponNum == WP_NONE || weaponNum == WP_GAUNTLET)
			torsoAnim = TORSO_STAND2;
		else
			torsoAnim = TORSO_STAND;
	}

	if(torsoAnim == TORSO_ATTACK || torsoAnim == TORSO_ATTACK2){
		if(weaponNum == WP_NONE || weaponNum == WP_GAUNTLET)
			torsoAnim = TORSO_ATTACK2;
		else
			torsoAnim = TORSO_ATTACK;
		pi->muzzleflashtime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		//FIXME play firing sound here
	}

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if(weaponNum != pi->currentWeapon || currentAnim == TORSO_RAISE || currentAnim == TORSO_DROP)
		pi->pendingTorsoAnim = torsoAnim;
	else if((currentAnim == TORSO_GESTURE || currentAnim == TORSO_ATTACK) && (torsoAnim != currentAnim))
		pi->pendingTorsoAnim = torsoAnim;
	else if(torsoAnim != currentAnim){
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
	}
}
