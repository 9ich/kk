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
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

/*
==================
bubbletrail

Bullets shot underwater
==================
*/
void
bubbletrail(vec3_t start, vec3_t end, float spacing)
{
	vec3_t move;
	vec3_t vec;
	float len;
	int i;

	if(cg_noProjectileTrail.integer)
		return;

	veccpy(start, move);
	vecsub(end, start, vec);
	len = vecnorm(vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	vecmad(move, i, vec, move);

	vecmul(vec, spacing, vec);

	for(; i < len; i += spacing){
		localEntity_t *le;
		refEntity_t *re;

		le = alloclocalent();
		le->flags = LEF_PUFF_DONT_SCALE;
		le->type = LE_MOVE_SCALE_FADE;
		le->starttime = cg.time;
		le->endtime = cg.time + 1000 + random() * 250;
		le->liferate = 1.0 / (le->endtime - le->starttime);

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		veccpy(move, le->pos.trBase);
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		vecadd(move, vec, move);
	}
}

/*
=====================
smokepuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *
smokepuff(const vec3_t p, const vec3_t vel,
	     float radius,
	     float r, float g, float b, float a,
	     float duration,
	     int starttime,
	     int fadeintime,
	     int flags,
	     qhandle_t hShader)
{
	static int seed = 0x92;
	localEntity_t *le;
	refEntity_t *re;
//	int fadeintime = starttime + duration / 2;

	le = alloclocalent();
	le->flags = flags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random(&seed) * 360;
	re->radius = radius;
	re->shaderTime = starttime / 1000.0f;

	le->type = LE_MOVE_SCALE_FADE;
	le->starttime = starttime;
	le->fadeintime = fadeintime;
	le->endtime = starttime + duration;
	if(fadeintime > starttime)
		le->liferate = 1.0 / (le->endtime - le->fadeintime);
	else
		le->liferate = 1.0 / (le->endtime - le->starttime);
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = starttime;
	veccpy(vel, le->pos.trDelta);
	veccpy(p, le->pos.trBase);

	veccpy(p, re->origin);
	re->customShader = hShader;

	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

localEntity_t *
shockwave(vec3_t pt, float radius)
{
	localEntity_t *le;
	refEntity_t *re;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_SHOCKWAVE;
	le->starttime = cg.time;
	le->endtime = cg.time + 100;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	vecclear(le->angles.trBase);

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.shockwaveModel;

	veccpy(pt, re->origin);

	return le;
}

/*
==================
spawneffect

Player teleporting in or out
==================
*/
void
spawneffect(vec3_t org)
{
	localEntity_t *le;
	refEntity_t *re;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_FADE_RGB;
	le->starttime = cg.time;
	le->endtime = cg.time + 500;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

#ifndef MISSIONPACK
	re->customShader = cgs.media.teleportEffectShader;
#endif
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear(re->axis);

	veccpy(org, re->origin);
#ifdef MISSIONPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif
}

#ifdef MISSIONPACK
/*
===============
CG_LightningBoltBeam
===============
*/
void
CG_LightningBoltBeam(vec3_t start, vec3_t end)
{
	localEntity_t *le;
	refEntity_t *beam;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_SHOWREFENTITY;
	le->starttime = cg.time;
	le->endtime = cg.time + 50;

	beam = &le->refEntity;

	veccpy(start, beam->origin);
	// this is the end point
	veccpy(end, beam->oldorigin);

	beam->reType = RT_LIGHTNING;
	beam->customShader = cgs.media.lightningShader;
}

/*
==================
CG_KamikazeEffect
==================
*/
void
CG_KamikazeEffect(vec3_t org)
{
	localEntity_t *le;
	refEntity_t *re;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_KAMIKAZE;
	le->starttime = cg.time;
	le->endtime = cg.time + 3000;	//2250;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	vecclear(le->angles.trBase);

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.kamikazeEffectModel;

	veccpy(org, re->origin);
}

/*
==================
CG_ObeliskExplode
==================
*/
void
CG_ObeliskExplode(vec3_t org, int entityNum)
{
	localEntity_t *le;
	vec3_t origin;

	// create an explosion
	veccpy(org, origin);
	origin[2] += 64;
	le = explosion(origin, vec3_origin,
			      cgs.media.dishFlashModel,
			      cgs.media.rocketExplosionShader,
			      600, qtrue);
	le->light = 300;
	le->lightcolor[0] = 1;
	le->lightcolor[1] = 0.75;
	le->lightcolor[2] = 0.0;
}

/*
==================
CG_ObeliskPain
==================
*/
void
CG_ObeliskPain(vec3_t org)
{
	float r;
	sfxHandle_t sfx;

	// hit sound
	r = rand() & 3;
	if(r < 2)
		sfx = cgs.media.obeliskHitSound1;
	else if(r == 2)
		sfx = cgs.media.obeliskHitSound2;
	else
		sfx = cgs.media.obeliskHitSound3;
	trap_S_StartSound(org, ENTITYNUM_NONE, CHAN_BODY, sfx);
}

/*
==================
CG_InvulnerabilityImpact
==================
*/
void
CG_InvulnerabilityImpact(vec3_t org, vec3_t angles)
{
	localEntity_t *le;
	refEntity_t *re;
	int r;
	sfxHandle_t sfx;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_INVULIMPACT;
	le->starttime = cg.time;
	le->endtime = cg.time + 1000;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityImpactModel;

	veccpy(org, re->origin);
	AnglesToAxis(angles, re->axis);

	r = rand() & 3;
	if(r < 2)
		sfx = cgs.media.invulnerabilityImpactSound1;
	else if(r == 2)
		sfx = cgs.media.invulnerabilityImpactSound2;
	else
		sfx = cgs.media.invulnerabilityImpactSound3;
	trap_S_StartSound(org, ENTITYNUM_NONE, CHAN_BODY, sfx);
}

/*
==================
CG_InvulnerabilityJuiced
==================
*/
void
CG_InvulnerabilityJuiced(vec3_t org)
{
	localEntity_t *le;
	refEntity_t *re;
	vec3_t angles;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_INVULJUICED;
	le->starttime = cg.time;
	le->endtime = cg.time + 10000;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityJuicedModel;

	veccpy(org, re->origin);
	vecclear(angles);
	AnglesToAxis(angles, re->axis);

	trap_S_StartSound(org, ENTITYNUM_NONE, CHAN_BODY, cgs.media.invulnerabilityJuicedSound);
}

#endif

/*
==================
scoreplum
==================
*/
void
scoreplum(int client, vec3_t org, int score)
{
	localEntity_t *le;
	refEntity_t *re;
	vec3_t angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if(client != cg.pps.clientNum || cg_scorePlum.integer == 0)
		return;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_SCOREPLUM;
	le->starttime = cg.time;
	le->endtime = cg.time + 4000;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	veccpy(org, le->pos.trBase);
	if(org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20)
		le->pos.trBase[2] -= 20;

	//cgprintf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)vecdist(org, lastPos));
	veccpy(org, lastPos);

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	vecclear(angles);
	AnglesToAxis(angles, re->axis);
}

/*
====================
explosion
====================
*/
localEntity_t *
explosion(vec3_t origin, vec3_t dir,
		 qhandle_t hModel, qhandle_t shader,
		 int msec, qboolean isSprite)
{
	float ang;
	localEntity_t *ex;
	int offset;
	vec3_t tmpVec, newOrigin;

	if(msec <= 0)
		cgerrorf("explosion: msec = %i", msec);

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = alloclocalent();
	if(isSprite){
		ex->type = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		vecmul(dir, 16, tmpVec);
		vecadd(tmpVec, origin, newOrigin);
	}else{
		ex->type = LE_EXPLOSION;
		veccpy(origin, newOrigin);

		// set axis with random rotate
		if(!dir)
			AxisClear(ex->refEntity.axis);
		else{
			ang = rand() % 360;
			veccpy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->starttime = cg.time - offset;
	ex->endtime = ex->starttime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->starttime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	veccpy(newOrigin, ex->refEntity.origin);
	veccpy(newOrigin, ex->refEntity.oldorigin);

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}

/*
=================
bleed

This is the spurt of blood when a character gets hit
=================
*/
void
bleed(vec3_t origin, int entityNum)
{
	localEntity_t *ex;

	if(!cg_blood.integer)
		return;

	ex = alloclocalent();
	ex->type = LE_EXPLOSION;

	ex->starttime = cg.time;
	ex->endtime = ex->starttime + 500;

	veccpy(origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = rand() % 360;
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if(entityNum == cg.snap->ps.clientNum)
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
}

/*
==================
CG_LaunchGib
==================
*/
void
CG_LaunchGib(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localEntity_t *le;
	refEntity_t *re;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + 5000 + random() * 3000;

	veccpy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	veccpy(origin, le->pos.trBase);
	veccpy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bouncefactor = 0.6f;

	le->bouncesoundtype = LEBS_BLOOD;
	le->marktype = LEMT_BLOOD;
}

/*
===================
gibplayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define GIB_VELOCITY	250
#define GIB_JUMP	250
void
gibplayer(vec3_t playerOrigin)
{
	vec3_t origin, velocity;

	if(!cg_blood.integer)
		return;

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	if(rand() & 1)
		CG_LaunchGib(origin, velocity, cgs.media.gibSkull);
	else
		CG_LaunchGib(origin, velocity, cgs.media.gibBrain);

	// allow gibs to be turned off for speed
	if(!cg_gibs.integer)
		return;

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibAbdomen);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibArm);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibChest);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFist);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFoot);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibForearm);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibIntestine);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibLeg);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibLeg);
}

/*
==================
CG_LaunchExplode
==================
*/
void
CG_LaunchExplode(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localEntity_t *le;
	refEntity_t *re;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + 10000 + random() * 6000;

	veccpy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	veccpy(origin, le->pos.trBase);
	veccpy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bouncefactor = 0.9f;

	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

#define EXP_VELOCITY	100
#define EXP_JUMP	150
/*
===================
CG_BigExplode

Generated a bunch of gibs launching out from the bodies location
===================
*/
void
CG_BigExplode(vec3_t playerOrigin)
{
	vec3_t origin, velocity;

	if(!cg_blood.integer)
		return;

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*1.5;
	velocity[1] = crandom()*EXP_VELOCITY*1.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*2.0;
	velocity[1] = crandom()*EXP_VELOCITY*2.0;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*2.5;
	velocity[1] = crandom()*EXP_VELOCITY*2.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);
}
