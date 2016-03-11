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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void
CG_MachineGunEjectBrass(centity_t *cent)
{
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if(cg_brassTime.integer <= 0)
		return;

	le = alloclocalent();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis(cent->lerpangles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	vecadd(cent->lerporigin, xoffset, re->origin);

	veccpy(re->origin, le->pos.trBase);

	if(pointcontents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	vecmul(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bouncefactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void
CG_ShotgunEjectBrass(centity_t *cent)
{
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	vec3_t v[3];
	int i;

	if(cg_brassTime.integer <= 0)
		return;

	for(i = 0; i < 2; i++){
		float waterScale = 1.0f;

		le = alloclocalent();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if(i == 0)
			velocity[1] = 40 + 10 * crandom();
		else
			velocity[1] = -40 + 10 * crandom();
		velocity[2] = 100 + 50 * crandom();

		le->type = LE_FRAGMENT;
		le->starttime = cg.time;
		le->endtime = le->starttime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis(cent->lerpangles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		vecadd(cent->lerporigin, xoffset, re->origin);
		veccpy(re->origin, le->pos.trBase);
		if(pointcontents(re->origin, -1) & CONTENTS_WATER)
			waterScale = 0.10f;

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		vecmul(xvelocity, waterScale, le->pos.trDelta);

		AxisCopy(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bouncefactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->flags = LEF_TUMBLE;
		le->bouncesoundtype = LEBS_BRASS;
		le->marktype = LEMT_NONE;
	}
}

#ifdef MISSIONPACK
/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void
CG_NailgunEjectBrass(centity_t *cent)
{
	localEntity_t *smoke;
	vec3_t origin;
	vec3_t v[3];
	vec3_t offset;
	vec3_t xoffset;
	vec3_t up;

	AnglesToAxis(cent->lerpangles, v);

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	vecadd(cent->lerporigin, xoffset, origin);

	vecset(up, 0, 0, 64);

	smoke = smokepuff(origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader);
	// use the optimized local entity add
	smoke->type = LE_SCALE_FADE;
}

#endif

/*
==========================
dorailtrail
==========================
*/
void
dorailtrail(clientInfo_t *ci, vec3_t start, vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp;
	float len;
	int i, j, skip;

	localEntity_t *le;
	refEntity_t *re;

#define RADIUS		4
#define ROTATION	1
#define SPACING		5

	start[2] -= 4;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FADE_RGB;
	le->starttime = cg.time;
	le->endtime = cg.time + cg_railTrailTime.value;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	veccpy(start, re->origin);
	veccpy(end, re->oldorigin);

	re->shaderRGBA[0] = ci->color1[0] * 255;
	re->shaderRGBA[1] = ci->color1[1] * 255;
	re->shaderRGBA[2] = ci->color1[2] * 255;
	re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear(re->axis);

	if(cg_oldRail.integer){
		// nudge down a bit so it isn't exactly in center
		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		return;
	}

	veccpy(start, move);
	vecsub(end, start, vec);
	len = vecnorm(vec);
	vecperp(temp, vec);
	for(i = 0; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);	//banshee 2.4 was 10

	vecmad(move, 20, vec, move);
	vecmul(vec, SPACING, vec);

	skip = -1;

	j = 18;
	for(i = 0; i < len; i += SPACING){
		if(i != skip){
			skip = i + SPACING;
			le = alloclocalent();
			re = &le->refEntity;
			le->flags = LEF_PUFF_DONT_SCALE;
			le->type = LE_MOVE_SCALE_FADE;
			le->starttime = cg.time;
			le->endtime = cg.time + (i>>1) + 600;
			le->liferate = 1.0 / (le->endtime - le->starttime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_SPRITE;
			re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;

			re->shaderRGBA[0] = ci->color2[0] * 255;
			re->shaderRGBA[1] = ci->color2[1] * 255;
			re->shaderRGBA[2] = ci->color2[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color2[0] * 0.75;
			le->color[1] = ci->color2[1] * 0.75;
			le->color[2] = ci->color2[2] * 0.75;
			le->color[3] = 1.0f;

			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;

			veccpy(move, move2);
			vecmad(move2, RADIUS, axis[j], move2);
			veccpy(move2, le->pos.trBase);

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		vecadd(move, vec, move);

		j = (j + ROTATION) % 36;
	}
}

/*
==========================
CG_RocketTrail
==========================
*/
static void
CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int step;
	vec3_t origin, lastPos;
	int t;
	int starttime, contents;
	int lastContents;
	entityState_t *es;
	vec3_t up;
	localEntity_t *smoke;

	if(cg_noProjectileTrail.integer)
		return;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currstate;
	starttime = ent->trailtime;
	t = step * ((starttime + step) / step);

	evaltrajectory(&es->pos, cg.time, origin);
	contents = pointcontents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if(es->pos.trType == TR_STATIONARY){
		ent->trailtime = cg.time;
		return;
	}

	evaltrajectory(&es->pos, ent->trailtime, lastPos);
	lastContents = pointcontents(lastPos, -1);

	ent->trailtime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		if(contents & lastContents & CONTENTS_WATER)
			bubbletrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailtime; t += step){
		evaltrajectory(&es->pos, t, lastPos);

		smoke = smokepuff(lastPos, up,
				     wi->trailradius,
				     1, 1, 1, 0.33f,
				     wi->trailtime,
				     t,
				     0,
				     0,
				     cgs.media.smokePuffShader);
		// use the optimized local entity add
		smoke->type = LE_SCALE_FADE;
	}
}

#ifdef MISSIONPACK
/*
==========================
CG_NailTrail
==========================
*/
static void
CG_NailTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int step;
	vec3_t origin, lastPos;
	int t;
	int starttime, contents;
	int lastContents;
	entityState_t *es;
	vec3_t up;
	localEntity_t *smoke;

	if(cg_noProjectileTrail.integer)
		return;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currstate;
	starttime = ent->trailtime;
	t = step * ((starttime + step) / step);

	evaltrajectory(&es->pos, cg.time, origin);
	contents = pointcontents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if(es->pos.trType == TR_STATIONARY){
		ent->trailtime = cg.time;
		return;
	}

	evaltrajectory(&es->pos, ent->trailtime, lastPos);
	lastContents = pointcontents(lastPos, -1);

	ent->trailtime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		if(contents & lastContents & CONTENTS_WATER)
			bubbletrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailtime; t += step){
		evaltrajectory(&es->pos, t, lastPos);

		smoke = smokepuff(lastPos, up,
				     wi->trailradius,
				     1, 1, 1, 0.33f,
				     wi->trailtime,
				     t,
				     0,
				     0,
				     cgs.media.nailPuffShader);
		// use the optimized local entity add
		smoke->type = LE_SCALE_FADE;
	}
}

#endif

/*
==========================
CG_PlasmaTrail
==========================
*/
static void
CG_PlasmaTrail(centity_t *cent, const weaponInfo_t *wi)
{
	localEntity_t *le;
	refEntity_t *re;
	entityState_t *es;
	vec3_t velocity, xvelocity, origin;
	vec3_t offset, xoffset;
	vec3_t v[3];

	float waterScale = 1.0f;

	if(cg_noProjectileTrail.integer || cg_oldPlasma.integer)
		return;

	es = &cent->currstate;

	evaltrajectory(&es->pos, cg.time, origin);

	le = alloclocalent();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->type = LE_MOVE_SCALE_FADE;
	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_NONE;
	le->marktype = LEMT_NONE;

	le->starttime = cg.time;
	le->endtime = le->starttime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis(cent->lerpangles, v);

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	vecadd(origin, xoffset, re->origin);
	veccpy(re->origin, le->pos.trBase);

	if(pointcontents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	vecmul(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bouncefactor = 0.3f;

	re->shaderRGBA[0] = wi->flashcolor[0] * 63;
	re->shaderRGBA[1] = wi->flashcolor[1] * 63;
	re->shaderRGBA[2] = wi->flashcolor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashcolor[0] * 0.2;
	le->color[1] = wi->flashcolor[1] * 0.2;
	le->color[2] = wi->flashcolor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;
}

/*
==========================
grappletrail
==========================
*/
void
grappletrail(centity_t *ent, const weaponInfo_t *wi)
{
	vec3_t origin;
	entityState_t *es;
	vec3_t forward, up;
	refEntity_t beam;

	es = &ent->currstate;

	evaltrajectory(&es->pos, cg.time, origin);
	ent->trailtime = cg.time;

	memset(&beam, 0, sizeof(beam));
	veccpy(cg_entities[ent->currstate.otherEntityNum].lerporigin, beam.origin);
	anglevecs(cg_entities[ent->currstate.otherEntityNum].lerpangles, forward, nil, up);
	vecmad(beam.origin, -15, up, beam.origin);
	veccpy(origin, beam.oldorigin);

	if(vecdist(beam.origin, beam.oldorigin) < 64)
		return;		// Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void
CG_GrenadeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	CG_RocketTrail(ent, wi);
}

/*
=================
registerweap

The server says this item is used on this level
=================
*/
void
registerweap(int weaponNum)
{
	weaponInfo_t *weapinfo;
	gitem_t *item, *ammo;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weapinfo = &cg_weapons[weaponNum];

	if(weaponNum == 0)
		return;

	if(weapinfo->registered)
		return;

	memset(weapinfo, 0, sizeof(*weapinfo));
	weapinfo->registered = qtrue;

	for(item = bg_itemlist + 1; item->classname; item++)
		if(item->type == IT_WEAPON && item->tag == weaponNum){
			weapinfo->item = item;
			break;
		}
	if(!item->classname)
		cgerrorf("Couldn't find weapon %i", weaponNum);
	registeritemgfx(item - bg_itemlist);

	// load cmodel before model so filecache works
	weapinfo->model = trap_R_RegisterModel(item->model[0]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weapinfo->model, mins, maxs);
	for(i = 0; i < 3; i++)
		weapinfo->midpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);

	weapinfo->icon = trap_R_RegisterShader(item->icon);
	weapinfo->ammoicon = trap_R_RegisterShader(item->icon);

	for(ammo = bg_itemlist + 1; ammo->classname; ammo++)
		if(ammo->type == IT_AMMO && ammo->tag == weaponNum)
			break;
	if(ammo->classname && ammo->model[0])
		weapinfo->ammomodel = trap_R_RegisterModel(ammo->model[0]);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_flash.md3");
	weapinfo->flashmodel = trap_R_RegisterModel(path);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_barrel.md3");
	weapinfo->barrelmodel = trap_R_RegisterModel(path);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_hand.md3");
	weapinfo->handsmodel = trap_R_RegisterModel(path);

	if(!weapinfo->handsmodel)
		weapinfo->handsmodel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");

	switch(weaponNum){
	case WP_GAUNTLET:
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->firingsound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/melee/fstatck.wav", qfalse);
		break;

	case WP_LIGHTNING:
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->rdysound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weapinfo->firingsound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", qfalse);

		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->missilemodel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weapinfo->missileTrailFunc = grappletrail;
		weapinfo->missilelight = 200;
		MAKERGB(weapinfo->missilelightcolor, 1, 0.75f, 0);
		weapinfo->rdysound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weapinfo->firingsound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		break;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		weapinfo->firingsound = trap_S_RegisterSound("sound/weapons/vulcan/wvulfire.wav", qfalse);
		MAKERGB(weapinfo->flashcolor, 1, 1, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf1b.wav", qfalse);
		weapinfo->flashsnd[1] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf2b.wav", qfalse);
		weapinfo->flashsnd[2] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf3b.wav", qfalse);
		weapinfo->flashsnd[3] = trap_S_RegisterSound("sound/weapons/vulcan/vulcanf4b.wav", qfalse);
		weapinfo->ejectbrass = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
		break;
#endif

	case WP_MACHINEGUN:
		MAKERGB(weapinfo->flashcolor, 1, 1, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weapinfo->flashsnd[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", qfalse);
		weapinfo->flashsnd[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", qfalse);
		weapinfo->flashsnd[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", qfalse);
		weapinfo->ejectbrass = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
		break;

	case WP_SHOTGUN:
		MAKERGB(weapinfo->flashcolor, 1, 1, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
		weapinfo->ejectbrass = CG_ShotgunEjectBrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weapinfo->missileTrailFunc = CG_RocketTrail;
		weapinfo->missilelight = 200;
		weapinfo->trailtime = 2000;
		weapinfo->trailradius = 64;

		MAKERGB(weapinfo->missilelightcolor, 1, 0.75f, 0);
		MAKERGB(weapinfo->flashcolor, 1, 0.75f, 0);

		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		break;

#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/weaphits/proxmine.md3");
		weapinfo->missileTrailFunc = CG_GrenadeTrail;
		weapinfo->trailtime = 700;
		weapinfo->trailradius = 32;
		MAKERGB(weapinfo->flashcolor, 1, 0.70f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/proxmine/wstbfire.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;
#endif

	case WP_GRENADE_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/ammo/grenade1.md3");
		weapinfo->missileTrailFunc = CG_GrenadeTrail;
		weapinfo->trailtime = 700;
		weapinfo->trailradius = 32;
		MAKERGB(weapinfo->flashcolor, 1, 0.70f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;

#ifdef MISSIONPACK
	case WP_NAILGUN:
		weapinfo->ejectbrass = CG_NailgunEjectBrass;
		weapinfo->missileTrailFunc = CG_NailTrail;
//		weapinfo->missilesound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weapinfo->trailradius = 16;
		weapinfo->trailtime = 250;
		weapinfo->missilemodel = trap_R_RegisterModel("models/weaphits/nail.md3");
		MAKERGB(weapinfo->flashcolor, 1, 0.75f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/nailgun/wnalfire.wav", qfalse);
		break;
#endif

	case WP_PLASMAGUN:
//		weapinfo->missilemodel = cgs.media.invulnerabilityPowerupModel;
		weapinfo->missileTrailFunc = CG_PlasmaTrail;
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
		break;

	case WP_RAILGUN:
		weapinfo->rdysound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", qfalse);
		MAKERGB(weapinfo->flashcolor, 1, 0.5f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);
		cgs.media.railExplosionShader = trap_R_RegisterShader("railExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
		cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
		break;

	case WP_BFG:
		weapinfo->rdysound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", qfalse);
		MAKERGB(weapinfo->flashcolor, 1, 0.7f, 1);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", qfalse);
		cgs.media.bfgExplosionShader = trap_R_RegisterShader("bfgExplosion");
		weapinfo->missilemodel = trap_R_RegisterModel("models/weaphits/bfg.md3");
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		break;

	default:
		MAKERGB(weapinfo->flashcolor, 1, 1, 1);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		break;
	}
}

/*
=================
registeritemgfx

The server says this item is used on this level
=================
*/
void
registeritemgfx(int itemNum)
{
	itemInfo_t *itemInfo;
	gitem_t *item;

	if(itemNum < 0 || itemNum >= bg_nitems)
		cgerrorf("registeritemgfx: itemNum %d out of range [0-%d]", itemNum, bg_nitems-1);

	itemInfo = &cg_items[itemNum];
	if(itemInfo->registered)
		return;

	item = &bg_itemlist[itemNum];

	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel(item->model[0]);

	itemInfo->icon = trap_R_RegisterShader(item->icon);

	if(item->type == IT_WEAPON)
		registerweap(item->tag);

	//
	// powerups have an accompanying ring or sphere
	//
	if(item->type == IT_POWERUP || item->type == IT_HEALTH ||
	   item->type == IT_ARMOR || item->type == IT_HOLDABLE)
		if(item->model[1])
			itemInfo->models[1] = trap_R_RegisterModel(item->model[1]);
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int
CG_MapTorsoToWeaponFrame(clientInfo_t *ci, int frame)
{
	// change weapon
	if(frame >= ci->animations[TORSO_DROP].firstframe
	   && frame < ci->animations[TORSO_DROP].firstframe + 9)
		return frame - ci->animations[TORSO_DROP].firstframe + 6;

	// stand attack
	if(frame >= ci->animations[TORSO_ATTACK].firstframe
	   && frame < ci->animations[TORSO_ATTACK].firstframe + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK].firstframe;

	// stand attack 2
	if(frame >= ci->animations[TORSO_ATTACK2].firstframe
	   && frame < ci->animations[TORSO_ATTACK2].firstframe + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstframe;

	return 0;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void
CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float scale;
	int delta;
	float fracsin;

	veccpy(cg.refdef.vieworg, origin);
	veccpy(cg.refdefviewangles, angles);

	// on odd legs, invert some angles
	if(cg.bobcycle & 1)
		scale = -cg.xyspeed;
	else
		scale = cg.xyspeed;

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landtime;
	if(delta < LAND_DEFLECT_TIME)
		origin[2] += cg.landchange*0.25 * delta / LAND_DEFLECT_TIME;
	else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
		origin[2] += cg.landchange*0.25 *
			     (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.steptime;
	if(delta < STEP_TIME/2)
		origin[2] -= cg.stepchange*0.25 * delta / (STEP_TIME/2);
	else if(delta < STEP_TIME)
		origin[2] -= cg.stepchange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);

#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void
CG_LightningBolt(centity_t *cent, vec3_t origin)
{
	trace_t trace;
	refEntity_t beam;
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;
	int anim;

	if(cent->currstate.weapon != WP_LIGHTNING)
		return;

	memset(&beam, 0, sizeof(beam));

	// CPMA  "true" lightning
	if((cent->currstate.number == cg.pps.clientNum) && (cg_trueLightning.value != 0)){
		vec3_t angle;
		int i;

		for(i = 0; i < 3; i++){
			float a = cent->lerpangles[i] - cg.refdefviewangles[i];
			if(a > 180)
				a -= 360;
			if(a < -180)
				a += 360;

			angle[i] = cg.refdefviewangles[i] + a * (1.0 - cg_trueLightning.value);
			if(angle[i] < 0)
				angle[i] += 360;
			if(angle[i] > 360)
				angle[i] -= 360;
		}

		anglevecs(angle, forward, nil, nil);
		veccpy(cent->lerporigin, muzzlePoint);
//		veccpy(cg.refdef.vieworg, muzzlePoint );
	}else{
		// !CPMA
		anglevecs(cent->lerpangles, forward, nil, nil);
		veccpy(cent->lerporigin, muzzlePoint);
	}

	anim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	else
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	vecmad(muzzlePoint, 14, forward, muzzlePoint);

	// project forward by the lightning range
	vecmad(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);

	// see if it hit a wall
	cgtrace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		 cent->currstate.number, MASK_SHOT);

	// this is the endpoint
	veccpy(trace.endpos, beam.oldorigin);

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	veccpy(origin, beam.origin);

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene(&beam);

	// add the impact flare if it hit something
	if(trace.fraction < 1.0){
		vec3_t angles;
		vec3_t dir;

		vecsub(beam.oldorigin, beam.origin, dir);
		vecnorm(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;

		vecmad(trace.endpos, -16, dir, beam.origin);

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}

/*

static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
        trace_t		trace;
        refEntity_t		beam;
        vec3_t			forward;
        vec3_t			muzzlePoint, endPoint;

        if ( cent->currstate.weapon != WP_LIGHTNING ) {
                return;
        }

        memset( &beam, 0, sizeof( beam ) );

        // find muzzle point for this frame
        veccpy( cent->lerporigin, muzzlePoint );
        anglevecs( cent->lerpangles, forward, nil, nil );

        // FIXME: crouch
        muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

        vecmad( muzzlePoint, 14, forward, muzzlePoint );

        // project forward by the lightning range
        vecmad( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

        // see if it hit a wall
        cgtrace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
                cent->currstate.number, MASK_SHOT );

        // this is the endpoint
        veccpy( trace.endpos, beam.oldorigin );

        // use the provided origin, even though it may be slightly
        // different than the muzzle origin
        veccpy( origin, beam.origin );

        beam.reType = RT_LIGHTNING;
        beam.customShader = cgs.media.lightningShader;
        trap_R_AddRefEntityToScene( &beam );

        // add the impact flare if it hit something
        if ( trace.fraction < 1.0 ) {
                vec3_t	angles;
                vec3_t	dir;

                vecsub( beam.oldorigin, beam.origin, dir );
                vecnorm( dir );

                memset( &beam, 0, sizeof( beam ) );
                beam.hModel = cgs.media.lightningExplosionModel;

                vecmad( trace.endpos, -16, dir, beam.origin );

                // make a random orientation
                angles[0] = rand() % 360;
                angles[1] = rand() % 360;
                angles[2] = rand() % 360;
                AnglesToAxis( angles, beam.axis );
                trap_R_AddRefEntityToScene( &beam );
        }
}
*/

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define         SPIN_SPEED	0.9
#define         COAST_TIME	1000
static float
CG_MachinegunSpinAngle(centity_t *cent)
{
	int delta;
	float angle;
	float speed;

	delta = cg.time - cent->pe.barreltime;
	if(cent->pe.barrelspin)
		angle = cent->pe.barrelangle + delta * SPIN_SPEED;
	else{
		if(delta > COAST_TIME)
			delta = COAST_TIME;

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelangle + delta * speed;
	}

	if(cent->pe.barrelspin == !(cent->currstate.eFlags & EF_FIRING)){
		cent->pe.barreltime = cg.time;
		cent->pe.barrelangle = AngleMod(angle);
		cent->pe.barrelspin = !!(cent->currstate.eFlags & EF_FIRING);
#ifdef MISSIONPACK
		if(cent->currstate.weapon == WP_CHAINGUN && !cent->pe.barrelspin)
			trap_S_StartSound(nil, cent->currstate.number, CHAN_WEAPON, trap_S_RegisterSound("sound/weapons/vulcan/wvulwind.wav", qfalse));

#endif
	}

	return angle;
}

/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void
CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups)
{
	// add powerup effects
	if(powerups & (1 << PW_INVIS)){
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(gun);
	}else{
		trap_R_AddRefEntityToScene(gun);

		if(powerups & (1 << PW_BATTLESUIT)){
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
		if(powerups & (1 << PW_QUAD)){
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
	}
}

/*
=============
addplayerweap

Used for both the view weapon (ps is valid) and the world modelother character models (ps is nil)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void
addplayerweap(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team)
{
	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;
	orientation_t lerped;

	weaponNum = cent->currstate.weapon;

	registerweap(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	veccpy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if(weaponNum == WP_RAILGUN){
		clientInfo_t *ci = &cgs.clientinfo[cent->currstate.clientNum];
		if(cent->pe.railfiretime + 1500 > cg.time){
			int scale = 255 * (cg.time - cent->pe.railfiretime) / 1500;
			gun.shaderRGBA[0] = (ci->c1rgba[0] * scale) >> 8;
			gun.shaderRGBA[1] = (ci->c1rgba[1] * scale) >> 8;
			gun.shaderRGBA[2] = (ci->c1rgba[2] * scale) >> 8;
			gun.shaderRGBA[3] = 255;
		}else
			Byte4Copy(ci->c1rgba, gun.shaderRGBA);
	}

	gun.hModel = weapon->model;
	if(!gun.hModel)
		return;

	if(!ps){
		// add weapon ready sound
		cent->pe.lightningfiring = qfalse;
		if((cent->currstate.eFlags & EF_FIRING) && weapon->firingsound){
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->firingsound);
			cent->pe.lightningfiring = qtrue;
		}else if(weapon->rdysound)
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->rdysound);
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, "tag_weapon");
	veccpy(parent->origin, gun.origin);

	vecmad(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		vecmad(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
		vecmad(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	vecmad(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups(&gun, cent->currstate.powerups);

	// add the spinning barrel
	if(weapon->barrelmodel){
		memset(&barrel, 0, sizeof(barrel));
		veccpy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelmodel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle(cent);
		AnglesToAxis(angles, barrel.axis);

		rotentontag(&barrel, &gun, weapon->model, "tag_barrel");

		CG_AddWeaponWithPowerups(&barrel, cent->currstate.powerups);
	}

	// make sure we aren't looking at cg.pplayerent for LG
	nonPredictedCent = &cg_entities[cent->currstate.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if((nonPredictedCent - cg_entities) != cent->currstate.clientNum)
		nonPredictedCent = cent;

	// add the flash
	if((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK)
	   && (nonPredictedCent->currstate.eFlags & EF_FIRING)){
		// continuous flash
	}else
	// impulse flash
	if(cg.time - cent->muzzleflashtime > MUZZLE_FLASH_TIME)
		return;


	memset(&flash, 0, sizeof(flash));
	veccpy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashmodel;
	if(!flash.hModel)
		return;
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	// colorize the railgun blast
	if(weaponNum == WP_RAILGUN){
		clientInfo_t *ci;

		ci = &cgs.clientinfo[cent->currstate.clientNum];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	rotentontag(&flash, &gun, weapon->model, "tag_flash");
	trap_R_AddRefEntityToScene(&flash);

	if(ps || cg.thirdperson ||
	   cent->currstate.number != cg.pps.clientNum){
		// add lightning bolt
		CG_LightningBolt(nonPredictedCent, flash.origin);

		if(weapon->flashcolor[0] || weapon->flashcolor[1] || weapon->flashcolor[2])
			trap_R_AddLightToScene(flash.origin, 300 + (rand()&31), weapon->flashcolor[0],
					       weapon->flashcolor[1], weapon->flashcolor[2]);
	}
}

/*
==============
addviewweap

Add the weapon, and flash for the player's view
==============
*/
void
addviewweap(playerState_t *ps)
{
	refEntity_t hand;
	centity_t *cent;
	clientInfo_t *ci;
	float fovOffset;
	vec3_t angles;
	weaponInfo_t *weapon;

	if(ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(ps->pm_type == PM_INTERMISSION)
		return;

	// no gun if in third person view or a camera is active
	//if ( cg.thirdperson || cg.cameramode) {
	if(cg.thirdperson)
		return;


	// allow the gun to be completely removed
	if(!cg_drawGun.integer){
		vec3_t origin;

		if(cg.pps.eFlags & EF_FIRING){
			// special hack for lightning gun...
			veccpy(cg.refdef.vieworg, origin);
			vecmad(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// don't draw if testing a gun model
	if(cg.testgun)
		return;

	// drop gun lower at higher fov
	if(cg_fov.integer > 90)
		fovOffset = -0.2 * (cg_fov.integer - 90);
	else
		fovOffset = 0;

	cent = &cg.pplayerent;	// &cg_entities[cg.snap->ps.clientNum];
	registerweap(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	vecmad(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	vecmad(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	vecmad(hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// map torso animations to weapon animations
	if(cg_gun_frame.integer){
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}else{
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currstate.clientNum];
		hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
		hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldframe);
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsmodel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	addplayerweap(&hand, ps, &cg.pplayerent, ps->persistant[PERS_TEAM]);
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
drawweapsel
===================
*/
void
drawweapsel(void)
{
	int i;
	int bits;
	int count;
	int x, y, w;
	char *name;
	float *color;

	// don't display if dead
	if(cg.pps.stats[STAT_HEALTH] <= 0)
		return;

	color = fadecolor(cg.weapseltime, WEAPON_SELECT_TIME);
	if(!color)
		return;
	trap_R_SetColor(color);

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itempkuptime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[STAT_WEAPONS];
	count = 0;
	for(i = 1; i < MAX_WEAPONS; i++)
		if(bits & (1 << i))
			count++;

	x = 320 - count * 20;
	y = 380;

	for(i = 1; i < MAX_WEAPONS; i++){
		if(!(bits & (1 << i)))
			continue;

		registerweap(i);

		// draw weapon icon
		drawpic(x, y, 32, 32, cg_weapons[i].icon);

		// draw selection marker
		if(i == cg.weapsel)
			drawpic(x-4, y-4, 40, 40, cgs.media.selectShader);

		// no ammo cross on top
		if(!cg.snap->ps.ammo[i])
			drawpic(x, y, 32, 32, cgs.media.noammoShader);

		x += 40;
	}

	// draw the selected name
	if(cg_weapons[cg.weapsel].item){
		name = cg_weapons[cg.weapsel].item->pickupname;
		if(name){
			w = drawstrlen(name) * BIGCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			drawbigstrcolor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor(nil);
}

/*
===============
weapselectable
===============
*/
static qboolean
weapselectable(int i)
{
	if(!cg.snap->ps.ammo[i])
		return qfalse;
	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)))
		return qfalse;

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void
CG_NextWeapon_f(void)
{
	int i;
	int original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weapseltime = cg.time;
	original = cg.weapsel;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel++;
		if(cg.weapsel == MAX_WEAPONS)
			cg.weapsel = 0;
		if(cg.weapsel == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(cg.weapsel))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel = original;
}

/*
===============
CG_PrevWeapon_f
===============
*/
void
CG_PrevWeapon_f(void)
{
	int i;
	int original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weapseltime = cg.time;
	original = cg.weapsel;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel--;
		if(cg.weapsel == -1)
			cg.weapsel = MAX_WEAPONS - 1;
		if(cg.weapsel == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(cg.weapsel))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel = original;
}

/*
===============
CG_Weapon_f
===============
*/
void
CG_Weapon_f(void)
{
	int num;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	num = atoi(cgargv(1));

	if(num < 1 || num > MAX_WEAPONS-1)
		return;

	cg.weapseltime = cg.time;

	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)))
		return;	// don't have the weapon

	cg.weapsel = num;
}

/*
===================
outofammochange

The current weapon has just run out of ammo
===================
*/
void
outofammochange(void)
{
	int i;

	cg.weapseltime = cg.time;

	for(i = MAX_WEAPONS-1; i > 0; i--)
		if(weapselectable(i)){
			cg.weapsel = i;
			break;
		}
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
fireweap

Caused by an EV_FIRE_WEAPON event
================
*/
void
fireweap(centity_t *cent)
{
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;

	ent = &cent->currstate;
	if(ent->weapon == WP_NONE)
		return;
	if(ent->weapon >= WP_NUM_WEAPONS){
		cgerrorf("fireweap: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleflashtime = cg.time;

	// lightning gun only does this this on initial press
	if(ent->weapon == WP_LIGHTNING)
		if(cent->pe.lightningfiring)
			return;

	if(ent->weapon == WP_RAILGUN)
		cent->pe.railfiretime = cg.time;

	// play quad sound if needed
	if(cent->currstate.powerups & (1 << PW_QUAD))
		trap_S_StartSound(nil, cent->currstate.number, CHAN_ITEM, cgs.media.quadSound);

	// play a sound
	for(c = 0; c < 4; c++)
		if(!weap->flashsnd[c])
			break;
	if(c > 0){
		c = rand() % c;
		if(weap->flashsnd[c])
			trap_S_StartSound(nil, ent->number, CHAN_WEAPON, weap->flashsnd[c]);
	}

	// do brass ejection
	if(weap->ejectbrass && cg_brassTime.integer > 0)
		weap->ejectbrass(cent);
}

/*
=================
missilehitwall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void
missilehitwall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType)
{
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightcolor;
	localEntity_t *le;
	int r;
	qboolean alphafade;
	qboolean isSprite;
	int duration;
	vec3_t sprOrg;
	vec3_t sprVel;

	mod = 0;
	shader = 0;
	light = 0;
	lightcolor[0] = 1;
	lightcolor[1] = 1;
	lightcolor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch(weapon){
	default:
#ifdef MISSIONPACK
	case WP_NAILGUN:
		if(soundType == IMPACTSOUND_FLESH)
			sfx = cgs.media.sfx_nghitflesh;
		else if(soundType == IMPACTSOUND_METAL)
			sfx = cgs.media.sfx_nghitmetal;
		else
			sfx = cgs.media.sfx_nghit;
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#endif
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if(r < 2)
			sfx = cgs.media.sfx_lghit2;
		else if(r == 2)
			sfx = cgs.media.sfx_lghit1;
		else
			sfx = cgs.media.sfx_lghit3;
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
#endif
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightcolor[0] = 1;
		lightcolor[1] = 0.75;
		lightcolor[2] = 0.0;
		if(cg_oldRocket.integer == 0){
			// explosion sprite animation
			vecmad(origin, 24, dir, sprOrg);
			vecmul(dir, 64, sprVel);

			CG_ParticleExplosion("explode1", sprOrg, sprVel, 1400, 20, 30);
		}
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		//sfx = cgs.media.sfx_railg;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if(soundType == IMPACTSOUND_FLESH)
			sfx = cgs.media.sfx_chghitflesh;
		else if(soundType == IMPACTSOUND_METAL)
			sfx = cgs.media.sfx_chghitmetal;
		else
			sfx = cgs.media.sfx_chghit;
		mark = cgs.media.bulletMarkShader;

		radius = 8;
		break;
#endif

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if(r == 0)
			sfx = cgs.media.sfx_ric1;
		else if(r == 1)
			sfx = cgs.media.sfx_ric2;
		else
			sfx = cgs.media.sfx_ric3;

		radius = 8;
		break;
	}

	if(sfx)
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);

	//
	// create the explosion
	//
	if(mod){
		le = explosion(origin, dir,
				      mod, shader,
				      duration, isSprite);
		le->light = light;
		veccpy(lightcolor, le->lightcolor);
		if(weapon == WP_RAILGUN){
			// colorize with client color
			veccpy(cgs.clientinfo[clientNum].color1, le->color);
			le->refEntity.shaderRGBA[0] = le->color[0] * 0xff;
			le->refEntity.shaderRGBA[1] = le->color[1] * 0xff;
			le->refEntity.shaderRGBA[2] = le->color[2] * 0xff;
			le->refEntity.shaderRGBA[3] = 0xff;
		}
	}

	//
	// impact mark
	//
	alphafade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if(weapon == WP_RAILGUN){
		float *color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		impactmark(mark, origin, dir, random()*360, color[0], color[1], color[2], 1, alphafade, radius, qfalse);
	}else
		impactmark(mark, origin, dir, random()*360, 1, 1, 1, 1, alphafade, radius, qfalse);
}

/*
=================
missilehitplayer
=================
*/
void
missilehitplayer(int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch(weapon){
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_PLASMAGUN:
	case WP_BFG:
#ifdef MISSIONPACK
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:
#endif
		missilehitwall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
		break;
	default:
		break;
	}
}

/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void
CG_ShotgunPellet(vec3_t start, vec3_t end, int skipNum)
{
	trace_t tr;
	int sourceContentType, destContentType;

	cgtrace(&tr, start, nil, nil, end, skipNum, MASK_SHOT);

	sourceContentType = pointcontents(start, 0);
	destContentType = pointcontents(tr.endpos, 0);

	// FIXME: should probably move this cruft into bubbletrail
	if(sourceContentType == destContentType){
		if(sourceContentType & CONTENTS_WATER)
			bubbletrail(start, tr.endpos, 32);
	}else if(sourceContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, nil, nil, 0, CONTENTS_WATER);
		bubbletrail(start, trace.endpos, 32);
	}else if(destContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, nil, nil, 0, CONTENTS_WATER);
		bubbletrail(tr.endpos, trace.endpos, 32);
	}

	if(tr.surfaceFlags & SURF_NOIMPACT)
		return;

	if(cg_entities[tr.entityNum].currstate.eType == ET_PLAYER)
		missilehitplayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	else{
		if(tr.surfaceFlags & SURF_NOIMPACT)
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		if(tr.surfaceFlags & SURF_METALSTEPS)
			missilehitwall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		else
			missilehitwall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void
CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum)
{
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

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

		CG_ShotgunPellet(origin, end, otherEntNum);
	}
}

/*
==============
shotgunfire
==============
*/
void
shotgunfire(entityState_t *es)
{
	vec3_t v;
	int contents;

	vecsub(es->origin2, es->pos.trBase, v);
	vecnorm(v);
	vecmul(v, 32, v);
	vecadd(es->pos.trBase, v, v);
	if(cgs.glconfig.hardwareType != GLHW_RAGEPRO){
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t up;

		contents = pointcontents(es->pos.trBase, 0);
		if(!(contents & CONTENTS_WATER)){
			vecset(up, 0, 0, 8);
			smokepuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
		}
	}
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum);
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
CG_Tracer
===============
*/
void
CG_Tracer(vec3_t source, vec3_t dest)
{
	vec3_t forward, right;
	polyVert_t verts[4];
	vec3_t line;
	float len, begin, end;
	vec3_t start, finish;
	vec3_t midpoint;

	// tracer
	vecsub(dest, source, forward);
	len = vecnorm(forward);

	// start at least a little ways from the muzzle
	if(len < 100)
		return;
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if(end > len)
		end = len;
	vecmad(source, begin, forward, start);
	vecmad(source, end, forward, finish);

	line[0] = vecdot(forward, cg.refdef.viewaxis[1]);
	line[1] = vecdot(forward, cg.refdef.viewaxis[2]);

	vecmul(cg.refdef.viewaxis[1], line[1], right);
	vecmad(right, -line[0], cg.refdef.viewaxis[2], right);
	vecnorm(right);

	vecmad(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	vecmad(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	vecmad(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	vecmad(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	// add the tracer sound
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean
CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle)
{
	vec3_t forward;
	centity_t *cent;
	int anim;

	if(entityNum == cg.snap->ps.clientNum){
		veccpy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		anglevecs(cg.snap->ps.viewangles, forward, nil, nil);
		vecmad(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if(!cent->currvalid)
		return qfalse;

	veccpy(cent->currstate.pos.trBase, muzzle);

	anglevecs(cent->currstate.apos.trBase, forward, nil, nil);
	anim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzle[2] += CROUCH_VIEWHEIGHT;
	else
		muzzle[2] += DEFAULT_VIEWHEIGHT;

	vecmad(muzzle, 14, forward, muzzle);

	return qtrue;
}

/*
======================
dobullet

Renders bullet effects.
======================
*/
void
dobullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum)
{
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if(sourceEntityNum >= 0 && cg_tracerChance.value > 0)
		if(CG_CalcMuzzlePoint(sourceEntityNum, start)){
			sourceContentType = pointcontents(start, 0);
			destContentType = pointcontents(end, 0);

			// do a complete bubble trail if necessary
			if((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER))
				bubbletrail(start, end, 32);
			// bubble trail from water into air
			else if((sourceContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, end, start, nil, nil, 0, CONTENTS_WATER);
				bubbletrail(start, trace.endpos, 32);
			}
			// bubble trail from air into water
			else if((destContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, start, end, nil, nil, 0, CONTENTS_WATER);
				bubbletrail(trace.endpos, end, 32);
			}

			// draw a tracer
			if(random() < cg_tracerChance.value)
				CG_Tracer(start, end);
		}

	// impact splash and mark
	if(flesh)
		bleed(end, fleshEntityNum);
	else
		missilehitwall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);

}
