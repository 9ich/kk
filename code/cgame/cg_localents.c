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

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define MAX_LOCAL_ENTITIES 512
localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t cg_activeLocalEntities;	// double linked list
localEntity_t *cg_freeLocalEntities;	// single linked list

/*
This is called at startup and for tournement restarts
*/
void
initlocalents(void)
{
	int i;

	memset(cg_localEntities, 0, sizeof(cg_localEntities));
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for(i = 0; i < MAX_LOCAL_ENTITIES - 1; i++)
		cg_localEntities[i].next = &cg_localEntities[i+1];
}

void
CG_FreeLocalEntity(localEntity_t *le)
{
	if(!le->prev)
		cgerrorf("CG_FreeLocalEntity: not active");

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
Will always succeed, even if it requires freeing an old active entity
*/
localEntity_t   *
alloclocalent(void)
{
	localEntity_t *le;

	if(!cg_freeLocalEntities)
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity(cg_activeLocalEntities.prev);

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset(le, 0, sizeof(*le));

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}

/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
Leave expanding blood puffs behind gibs
*/
void
CG_BloodTrail(localEntity_t *le)
{
	int t;
	int t2;
	int step;
	vec3_t newOrigin;
	localEntity_t *blood;

	step = 150;
	t = step * ((cg.time - cg.frametime + step) / step);
	t2 = step * (cg.time / step);

	for(; t <= t2; t += step){
		evaltrajectory(&le->pos, t, newOrigin);

		blood = smokepuff(newOrigin, vec3_origin,
				     20,		// radius
				     1, 1, 1, 1,	// color
				     2000,		// trailtime
				     t,			// starttime
				     0,			// fadeintime
				     0,			// flags
				     cgs.media.bloodTrailShader);
		// use the optimized version
		blood->type = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
	}
}

void
CG_FragmentBounceMark(localEntity_t *le, trace_t *trace)
{
	int radius;

	if(le->marktype == LEMT_BLOOD){
		radius = 16 + (rand()&31);
		impactmark(cgs.media.bloodMarkShader, trace->endpos, trace->plane.normal, random()*360,
			      1, 1, 1, 1, qtrue, radius, qfalse);
	}else if(le->marktype == LEMT_BURN){
		radius = 8 + (rand()&15);
		impactmark(cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360,
			      1, 1, 1, 1, qtrue, radius, qfalse);
	}

	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->marktype = LEMT_NONE;
}

void
CG_FragmentBounceSound(localEntity_t *le, trace_t *trace)
{
	if(le->bouncesoundtype == LEBS_BLOOD){
		// half the gibs will make splat sounds
		if(rand() & 1){
			int r = rand()&3;
			sfxHandle_t s;

			if(r == 0)
				s = cgs.media.gibBounce1Sound;
			else if(r == 1)
				s = cgs.media.gibBounce2Sound;
			else
				s = cgs.media.gibBounce3Sound;
			trap_S_StartSound(trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s);
		}
	}else if(le->bouncesoundtype == LEBS_BRASS){
	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->bouncesoundtype = LEBS_NONE;
}

void
CG_ReflectVelocity(localEntity_t *le, trace_t *trace)
{
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	evaltrajectorydelta(&le->pos, hitTime, velocity);
	dot = vecdot(velocity, trace->plane.normal);
	vecmad(velocity, -2*dot, trace->plane.normal, le->pos.trDelta);

	vecmul(le->pos.trDelta, le->bouncefactor, le->pos.trDelta);

	veccpy(trace->endpos, le->pos.trBase);
	le->pos.trTime = cg.time;
}

void
CG_AddFragment(localEntity_t *le)
{
	vec3_t newOrigin;
	trace_t trace;

	if(le->pos.trType == TR_STATIONARY){
		// sink into the ground if near the removal time
		int t;
		float oldZ;

		t = le->endtime - cg.time;
		if(t < SINK_TIME){
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			veccpy(le->refEntity.origin, le->refEntity.lightingOrigin);
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * (1.0 - (float)t / SINK_TIME);
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.origin[2] = oldZ;
		}else
			trap_R_AddRefEntityToScene(&le->refEntity);

		return;
	}

	// calculate new position
	evaltrajectory(&le->pos, cg.time, newOrigin);

	// trace a line from previous position to new position
	cgtrace(&trace, le->refEntity.origin, nil, nil, newOrigin, -1, CONTENTS_SOLID);
	if(trace.fraction == 1.0){
		// still in free fall
		veccpy(newOrigin, le->refEntity.origin);

		if(le->flags & LEF_TUMBLE){
			vec3_t angles;

			evaltrajectory(&le->angles, cg.time, angles);
			AnglesToAxis(angles, le->refEntity.axis);
		}

		trap_R_AddRefEntityToScene(&le->refEntity);

		// add a blood trail
		if(le->bouncesoundtype == LEBS_BLOOD)
			CG_BloodTrail(le);

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if(pointcontents(trace.endpos, 0) & CONTENTS_NODROP){
		CG_FreeLocalEntity(le);
		return;
	}

	// leave a mark
	CG_FragmentBounceMark(le, &trace);

	// do a bouncy sound
	CG_FragmentBounceSound(le, &trace);

	// reflect the velocity on the trace plane
	CG_ReflectVelocity(le, &trace);

	trap_R_AddRefEntityToScene(&le->refEntity);
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

void
CG_AddFadeRGB(localEntity_t *le)
{
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = (le->endtime - cg.time) * le->liferate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene(re);
}

static void
CG_AddMoveScaleFade(localEntity_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	if(le->fadeintime > le->starttime && cg.time < le->fadeintime)
		// fade / grow time
		c = 1.0 - (float)(le->fadeintime - cg.time) / (le->fadeintime - le->starttime);
	else
		// fade / grow time
		c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if(!(le->flags & LEF_PUFF_DONT_SCALE))
		re->radius = le->radius * (1.0 - c) + 8;

	evaltrajectory(&le->pos, cg.time, re->origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/*
For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
*/
static void
CG_AddScaleFade(localEntity_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade / grow time
	c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * (1.0 - c) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/*
This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
*/
static void
CG_AddFallScaleFade(localEntity_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade time
	c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - (1.0 - c) * le->pos.trDelta[2];

	re->radius = le->radius * (1.0 - c) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

static void
CG_AddExplosion(localEntity_t *ex)
{
	refEntity_t *ent;

	ent = &ex->refEntity;

	// add the entity
	trap_R_AddRefEntityToScene(ent);

	// add the dlight
	if(ex->light){
		float light;

		light = (float)(cg.time - ex->starttime) / (ex->endtime - ex->starttime);
		if(light < 0.5)
			light = 1.0;
		else
			light = 1.0 - (light - 0.5) * 2;
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);
	}
}


static void
CG_AddSpriteExplosion(localEntity_t *le)
{
	refEntity_t re;
	float c;

	re = le->refEntity;

	c = (le->endtime - cg.time) / (float)(le->endtime - le->starttime);
	if(c > 1)
		c = 1.0;	// can happen during connection problems

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * (1.0 - c) + 30;

	trap_R_AddRefEntityToScene(&re);

	// add the dlight
	if(le->light){
		float light;

		light = (float)(cg.time - le->starttime) / (le->endtime - le->starttime);
		if(light < 0.5)
			light = 1.0;
		else
			light = 1.0 - (light - 0.5) * 2;
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightcolor[0], le->lightcolor[1], le->lightcolor[2]);
	}
}

#define SHOCKWAVE_TIME		100.0f
// model radius
#define SHOCKWAVE_RADIUS	27.0f
#define SHOCKWAVE_ENDRADIUS	700.0f

void
CG_AddShockwave(localEntity_t *le)
{
	refEntity_t *re;
	refEntity_t shockwave;
	float c, cc, scale;
	vec3_t angles, axis[3];
	int t;

	re = &le->refEntity;

	t = cg.time - le->starttime;
	vecclear(angles);
	AnglesToAxis(angles, axis);

	if(t < SHOCKWAVE_TIME){
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.shockwaveModel;
		shockwave.reType = RT_MODEL;
		shockwave.shaderTime = re->shaderTime;
		veccpy(re->origin, shockwave.origin);

		c = t / SHOCKWAVE_TIME;
		cc = 0.25 + c*0.75f;
		scale = SHOCKWAVE_ENDRADIUS / SHOCKWAVE_RADIUS;
		vecmul(axis[0], cc * scale, shockwave.axis[0]);
		vecmul(axis[1], cc * scale, shockwave.axis[1]);
		vecmul(axis[2], cc * scale, shockwave.axis[2]);
		shockwave.nonNormalizedAxes = qtrue;

		shockwave.shaderRGBA[0] = 0xf0;
		shockwave.shaderRGBA[1] = 0xf0;
		shockwave.shaderRGBA[2] = 0xf0;
		shockwave.shaderRGBA[3] = 0x10 - c*0x10;

		trap_R_AddRefEntityToScene(&shockwave);
	}
}


#define NUMBER_SIZE 8

void
CG_AddScorePlum(localEntity_t *le)
{
	refEntity_t *re;
	vec3_t origin, delta, dir, vec, up = {0, 0, 1};
	float c, len;
	int i, score, digits[10], numdigits, negative;

	re = &le->refEntity;

	c = (le->endtime - cg.time) * le->liferate;

	score = le->radius;
	if(score < 0){
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}else{
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if(score >= 50)
			re->shaderRGBA[1] = 0;
		else if(score >= 20)
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		else if(score >= 10)
			re->shaderRGBA[2] = 0;
		else if(score >= 2)
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;

	}
	if(c < 0.25)
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	veccpy(le->pos.trBase, origin);
	origin[2] += 110 - c * 100;

	vecsub(cg.refdef.vieworg, origin, dir);
	veccross(dir, up, vec);
	vecnorm(vec);

	vecmad(origin, -10 + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < 20){
		CG_FreeLocalEntity(le);
		return;
	}

	negative = qfalse;
	if(score < 0){
		negative = qtrue;
		score = -score;
	}

	for(numdigits = 0; !(numdigits && !score); numdigits++){
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if(negative){
		digits[numdigits] = 10;
		numdigits++;
	}

	for(i = 0; i < numdigits; i++){
		vecmad(origin, (float)(((float)numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene(re);
	}
}

#ifdef MISSIONPACK

void
CG_AddKamikaze(localEntity_t *le)
{
	refEntity_t *re;
	refEntity_t shockwave;
	float c;
	vec3_t test, axis[3];
	int t;

	re = &le->refEntity;

	t = cg.time - le->starttime;
	vecclear(test);
	AnglesToAxis(test, axis);

	if(t > KAMI_SHOCKWAVE_STARTTIME && t < KAMI_SHOCKWAVE_ENDTIME){
		if(!(le->flags & LEF_SOUND1)){
//			trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeExplodeSound );
			trap_S_StartLocalSound(cgs.media.kamikazeExplodeSound, CHAN_AUTO);
			le->flags |= LEF_SOUND1;
		}
		// 1st kamikaze shockwave
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.shaderTime = re->shaderTime;
		veccpy(re->origin, shockwave.origin);

		c = (float)(t - KAMI_SHOCKWAVE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME);
		vecmul(axis[0], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0]);
		vecmul(axis[1], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1]);
		vecmul(axis[2], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2]);
		shockwave.nonNormalizedAxes = qtrue;

		if(t > KAMI_SHOCKWAVEFADE_STARTTIME)
			c = (float)(t - KAMI_SHOCKWAVEFADE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVEFADE_STARTTIME);
		else
			c = 0;
		c *= 0xff;
		shockwave.shaderRGBA[0] = 0xff - c;
		shockwave.shaderRGBA[1] = 0xff - c;
		shockwave.shaderRGBA[2] = 0xff - c;
		shockwave.shaderRGBA[3] = 0xff - c;

		trap_R_AddRefEntityToScene(&shockwave);
	}

	if(t > KAMI_EXPLODE_STARTTIME && t < KAMI_IMPLODE_ENDTIME){
		// explosion and implosion
		c = (le->endtime - cg.time) * le->liferate;
		c *= 0xff;
		re->shaderRGBA[0] = le->color[0] * c;
		re->shaderRGBA[1] = le->color[1] * c;
		re->shaderRGBA[2] = le->color[2] * c;
		re->shaderRGBA[3] = le->color[3] * c;

		if(t < KAMI_IMPLODE_STARTTIME)
			c = (float)(t - KAMI_EXPLODE_STARTTIME) / (float)(KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME);
		else{
			if(!(le->flags & LEF_SOUND2)){
//				trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeImplodeSound );
				trap_S_StartLocalSound(cgs.media.kamikazeImplodeSound, CHAN_AUTO);
				le->flags |= LEF_SOUND2;
			}
			c = (float)(KAMI_IMPLODE_ENDTIME - t) / (float)(KAMI_IMPLODE_ENDTIME - KAMI_IMPLODE_STARTTIME);
		}
		vecmul(axis[0], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[0]);
		vecmul(axis[1], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[1]);
		vecmul(axis[2], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[2]);
		re->nonNormalizedAxes = qtrue;

		trap_R_AddRefEntityToScene(re);
		// add the dlight
		trap_R_AddLightToScene(re->origin, c * 1000.0, 1.0, 1.0, c);
	}

	if(t > KAMI_SHOCKWAVE2_STARTTIME && t < KAMI_SHOCKWAVE2_ENDTIME){
		// 2nd kamikaze shockwave
		if(le->angles.trBase[0] == 0 &&
		   le->angles.trBase[1] == 0 &&
		   le->angles.trBase[2] == 0){
			le->angles.trBase[0] = random() * 360;
			le->angles.trBase[1] = random() * 360;
			le->angles.trBase[2] = random() * 360;
		}
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.shaderTime = re->shaderTime;
		veccpy(re->origin, shockwave.origin);

		test[0] = le->angles.trBase[0];
		test[1] = le->angles.trBase[1];
		test[2] = le->angles.trBase[2];
		AnglesToAxis(test, axis);

		c = (float)(t - KAMI_SHOCKWAVE2_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2_STARTTIME);
		vecmul(axis[0], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0]);
		vecmul(axis[1], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1]);
		vecmul(axis[2], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2]);
		shockwave.nonNormalizedAxes = qtrue;

		if(t > KAMI_SHOCKWAVE2FADE_STARTTIME)
			c = (float)(t - KAMI_SHOCKWAVE2FADE_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2FADE_STARTTIME);
		else
			c = 0;
		c *= 0xff;
		shockwave.shaderRGBA[0] = 0xff - c;
		shockwave.shaderRGBA[1] = 0xff - c;
		shockwave.shaderRGBA[2] = 0xff - c;
		shockwave.shaderRGBA[3] = 0xff - c;

		trap_R_AddRefEntityToScene(&shockwave);
	}
}

/*
CG_AddInvulnerabilityImpact
*/
void
CG_AddInvulnerabilityImpact(localEntity_t *le)
{
	trap_R_AddRefEntityToScene(&le->refEntity);
}

/*
CG_AddInvulnerabilityJuiced
*/
void
CG_AddInvulnerabilityJuiced(localEntity_t *le)
{
	int t;

	t = cg.time - le->starttime;
	if(t > 3000){
		le->refEntity.axis[0][0] = (float)1.0 + 0.3 * (t - 3000) / 2000;
		le->refEntity.axis[1][1] = (float)1.0 + 0.3 * (t - 3000) / 2000;
		le->refEntity.axis[2][2] = (float)0.7 + 0.3 * (2000 - (t - 3000)) / 2000;
	}
	if(t > 5000){
		le->endtime = 0;
		gibplayer(le->refEntity.origin);
	}else
		trap_R_AddRefEntityToScene(&le->refEntity);
}

void
CG_AddRefEntity(localEntity_t *le)
{
	if(le->endtime < cg.time){
		CG_FreeLocalEntity(le);
		return;
	}
	trap_R_AddRefEntityToScene(&le->refEntity);
}

#endif

//==============================================================================

void
addlocalents(void)
{
	localEntity_t *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for(; le != &cg_activeLocalEntities; le = next){
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if(cg.time >= le->endtime){
			CG_FreeLocalEntity(le);
			continue;
		}
		switch(le->type){
		default:
			cgerrorf("Bad type: %i", le->type);
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion(le);
			break;

		case LE_EXPLOSION:
			CG_AddExplosion(le);
			break;

		case LE_SHOCKWAVE:
			CG_AddShockwave(le);
			break;

		case LE_FRAGMENT:	// gibs and brass
			CG_AddFragment(le);
			break;

		case LE_MOVE_SCALE_FADE:	// water bubbles
			CG_AddMoveScaleFade(le);
			break;

		case LE_FADE_RGB:	// teleporters, railtrails
			CG_AddFadeRGB(le);
			break;

		case LE_FALL_SCALE_FADE:// gib blood trails
			CG_AddFallScaleFade(le);
			break;

		case LE_SCALE_FADE:	// rocket trails
			CG_AddScaleFade(le);
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum(le);
			break;

#ifdef MISSIONPACK
		case LE_KAMIKAZE:
			CG_AddKamikaze(le);
			break;
		case LE_INVULIMPACT:
			CG_AddInvulnerabilityImpact(le);
			break;
		case LE_INVULJUICED:
			CG_AddInvulnerabilityJuiced(le);
			break;
		case LE_SHOWREFENTITY:
			CG_AddRefEntity(le);
			break;
#endif
		}
	}
}
