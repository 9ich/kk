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
// events and effects dealing with weapons
#include "cg_local.h"

static void
machinegunejectbrass(centity_t *cent)
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

	velocity[0] = -5 + 10 * crandom();
	velocity[1] = -400 + 10 * crandom();
	velocity[2] = -5 + 10 * crandom();

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	angles2axis(cent->lerpangles, v);

	offset[0] = -4;
	offset[1] = -12;
	offset[2] = 0;

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
	vecadd(le->pos.trDelta, cent->currstate.pos.trDelta, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bouncefactor = 0.9 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = cent->lerpangles[0] + (rand()&31);
	le->angles.trBase[1] = cent->lerpangles[1] + (-89 + (rand()&31));
	le->angles.trBase[2] = cent->lerpangles[2] + (rand()&31);
	le->angles.trDelta[0] = -15 + 80*random();
	le->angles.trDelta[1] = -15 + 80*random();
	le->angles.trDelta[2] = -15 + 80*random();

	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

static void
shotgunejectbrass(centity_t *cent)
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

		angles2axis(cent->lerpangles, v);

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
		le->bouncefactor = 0.9f;

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

static void
nailgunejectbrass(centity_t *cent)
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

	velocity[0] = -5 + 10 * crandom();
	velocity[1] = -400 + 10 * crandom();
	velocity[2] = -5 + 10 * crandom();

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	angles2axis(cent->lerpangles, v);

	offset[0] = -4;
	offset[1] = -12;
	offset[2] = 0;

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

	le->bouncefactor = 0.9 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = cent->lerpangles[0] + (rand()&31);
	le->angles.trBase[1] = cent->lerpangles[1] + (-89 + (rand()&31));
	le->angles.trBase[2] = cent->lerpangles[2] + (rand()&31);
	le->angles.trDelta[0] = -15 + 30*random();
	le->angles.trDelta[1] = -15 + 30*random();
	le->angles.trDelta[2] = -15 + 30*random();

	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

#endif

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

static void
rockettrail(centity_t *ent, const weaponInfo_t *wi)
{
	int step;
	vec3_t origin, lastPos;
	int t;
	int starttime, contents;
	int lastContents;
	entityState_t *es;
	localEntity_t *smoke;

	if(cg_noProjectileTrail.integer)
		return;

	es = &ent->currstate;
	starttime = ent->trailtime;

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

	if(cg_rocketFlame.integer){
		// flame
		step = 4;	// msec interval
		t = step * ((starttime + step) / step);
		for(; t <= ent->trailtime + 0.5f*step; t += step){
			vec3_t ofs;

			evaltrajectory(&es->pos, t, lastPos);
			vecset(ofs, crandom(), crandom(), crandom());
			vecnorm(ofs);
			vecmul(ofs, 0.5f, ofs);
			vecadd(lastPos, ofs, lastPos);
			particleexplosion("explode1", lastPos, ofs, 80, 2, 14);
		}
	}

	if(cg_rocketSmoke.integer){
		// smoke plume
		step = 16;	// msec interval
		t = step * ((starttime + step) / step);
		for(; t <= ent->trailtime; t += step){
			evaltrajectory(&es->pos, t, lastPos);

			smoke = smokepuff(lastPos, vec3_origin, wi->trailradius,
			   1, 1, 1, 0.40f, wi->trailtime, t, 0, 0,
			   cgs.media.smokePuffShader);
			// use the optimized local entity add
			smoke->type = LE_SCALE_FADE;
		}
	}
}

void
grappletrail(centity_t *ent, const weaponInfo_t *wi)
{
	vec3_t origin, angles;
	vec3_t forward, up;
	refEntity_t beam;
	centity_t *player;

	evaltrajectory(&ent->currstate.pos, cg.time, origin);
	ent->trailtime = cg.time;
	player = &cg_entities[ent->currstate.otherEntityNum];

	memset(&beam, 0, sizeof(beam));

	// if we're drawing our own trail, use our own playerstate
	// to avoid any prediction errors
	playerpos(player, beam.origin);
	playerangles(player, angles);
	anglevecs(angles, forward, nil, up);
	vecmad(beam.origin, -12, up, beam.origin);
	veccpy(origin, beam.oldorigin);

	if(vecdist(beam.origin, beam.oldorigin) < 20)
		return;		// Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.grappleTrailShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0;
	beam.shaderRGBA[1] = 0x32;
	beam.shaderRGBA[2] = 0xd0;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

static void
grenadetrail(centity_t *ent, const weaponInfo_t *wi)
{
}

/*
 * Loads model and animation.cfg.
 */
void
loadmodelbundle(const char *filename, qhandle_t *model, animation_t *anims)
{
	char stripped[MAX_QPATH], path[MAX_QPATH];
	char *p;

	*model = trap_R_RegisterModel(filename);
	if(!model)
		return;

	p = strrchr(filename, '/');
	if(p == nil)
		return;
	COM_StripFilename(filename, stripped, sizeof stripped);
	Com_sprintf(path, sizeof path, "%s/animation.cfg", stripped);
	parseanimfile(path, anims);
}

/*
The server says this item is used on this level.
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
	loadmodelbundle(item->model[0], &weapinfo->model.h,
	   cgs.media.itemanims[item - bg_itemlist]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weapinfo->model.h, mins, maxs);
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
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBolt");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/rocket.md3");
		weapinfo->missileTrailFunc = grappletrail;
		weapinfo->missilelight = 200;
		MAKERGB(weapinfo->missilelightcolor, 0.9f*0.2f, 0.4f*0.2f, 0.0f);
		weapinfo->rdysound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weapinfo->firingsound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBolt");
		break;

	case WP_CHAINGUN:
		MAKERGB(weapinfo->flashcolor, 0.9f, 0.4f, 0.0f);
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/tracer.md3");
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("explode2");
		break;

	case WP_MACHINEGUN:
		MAKERGB(weapinfo->flashcolor, 0.9f, 0.4f, 0.0f);
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/tracer.md3");
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weapinfo->ejectbrass = machinegunejectbrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("explode2");
		break;

	case WP_SHOTGUN:
		MAKERGB(weapinfo->flashcolor, 0.9f, 0.4f, 0.0f);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
		weapinfo->ejectbrass = shotgunejectbrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/rocket.md3");
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weapinfo->missileTrailFunc = rockettrail;
		weapinfo->missilelight = 400;
		weapinfo->trailtime = 2000;
		weapinfo->trailradius = 64;

		MAKERGB(weapinfo->missilelightcolor, 0.9f*0.4f, 0.45f*0.4f, 0.0f);
		MAKERGB(weapinfo->flashcolor, 0.9f, 0.4f, 0.0f);

		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		weapinfo->flashsnd[1] = trap_S_RegisterSound("sound/weapons/rocket/rocklf2a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("explode2");
		break;

	case WP_HOMING_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/rockethoming.md3");
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weapinfo->missileTrailFunc = rockettrail;
		weapinfo->missilelight = 200;
		weapinfo->trailtime = 2000;
		weapinfo->trailradius = 64;

		MAKERGB(weapinfo->missilelightcolor, 0.9f*0.4f, 0.45f*0.4f, 0.0f);
		MAKERGB(weapinfo->flashcolor, 0.9f, 0.4f, 0.0f);

		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		weapinfo->flashsnd[1] = trap_S_RegisterSound("sound/weapons/rocket/rocklf2a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("explode2");
		break;

#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/proxmine.md3");
		weapinfo->missileTrailFunc = grenadetrail;
		weapinfo->trailtime = 700;
		weapinfo->trailradius = 32;
		MAKERGB(weapinfo->flashcolor, 1, 0.70f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/proxmine/wstbfire.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;
#endif

	case WP_GRENADE_LAUNCHER:
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/grenade.md3");
		weapinfo->missileTrailFunc = grenadetrail;
		weapinfo->trailtime = 700;
		weapinfo->trailradius = 32;
		MAKERGB(weapinfo->flashcolor, 1, 0.70f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;

#ifdef MISSIONPACK
	case WP_NAILGUN:
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weapinfo->ejectbrass = nailgunejectbrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("explode2");
//		weapinfo->missilesound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/rocket.md3");
		MAKERGB(weapinfo->flashcolor, 1, 0.75f, 0);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/nailgun/wnalfire.wav", qfalse);
		break;
#endif

	case WP_PLASMAGUN:
		weapinfo->missilemodel = trap_R_RegisterModel("models/missiles/plasma.md3");
		weapinfo->missilesound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
		weapinfo->missilelight = 160;
		MAKERGB(weapinfo->flashcolor, 0.6f, 0.6f, 1.0f);
		MAKERGB(weapinfo->missilelightcolor, 0.6f, 0.6f, 1.0f);
		weapinfo->flashsnd[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
		weapinfo->flashsnd[1] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf2a.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("models/missiles/plasmaball");
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
The server says this item is used on this level
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

	// powerups have an accompanying ring or sphere
	if(item->type == IT_POWERUP || item->type == IT_HEALTH ||
	   item->type == IT_SHIELD || item->type == IT_HOLDABLE){
		int i;

		for(i = 0; i < ARRAY_LEN(item->model); i++)
			if(item->model[i] != nil)
				itemInfo->models[i] = trap_R_RegisterModel(item->model[i]);
	}
}

static int
maptorsotoweapframe(clientInfo_t *ci, int frame)
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
Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
*/
static void
lightningbolt(centity_t *cent, vec3_t origin, int slot)
{
	trace_t trace;
	refEntity_t beam;
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;

	if(cent->currstate.weapon[slot] != WP_LIGHTNING)
		return;

	memset(&beam, 0, sizeof(beam));

	if(cent->currstate.number == cg.pps.clientNum){
		anglevecs(cg.refdefviewangles, forward, nil, nil);
//		veccpy(cent->lerporigin, muzzlePoint);
		veccpy(cg.refdef.vieworg, muzzlePoint );
	}else{
		anglevecs(cent->lerpangles, forward, nil, nil);
		veccpy(cent->lerporigin, muzzlePoint);
	}

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
		angles2axis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}

#define         SPIN_SPEED	0.9
#define         COAST_TIME	1000
static float
machinegunspinangle(centity_t *cent)
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
		if(cent->currstate.weapon[0] == WP_CHAINGUN && !cent->pe.barrelspin)
			trap_S_StartSound(nil, cent->currstate.number, CHAN_WEAPON, trap_S_RegisterSound("sound/weapons/vulcan/wvulwind.wav", qfalse));

#endif
	}

	return angle;
}

static void
addweapwithpowerups(refEntity_t *gun, int powerups)
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

static void
weapanim(centity_t *cent, weaponInfo_t *weapinfo, int slot, int *oldframe, int *frame, float *backlerp)
{
	float speedScale;

	if(cent->currstate.powerups & (1 << PW_HASTE))
		speedScale = 1.5f;
	else
		speedScale = 1.0f;

	runlerpframe(cgs.media.itemanims[finditemforweapon(cent->currstate.weapon[slot]) - bg_itemlist],
	   &cent->weaplerpframe[slot], cent->currstate.weapAnim[slot], speedScale);
	*oldframe = cent->weaplerpframe[slot].oldframe;
	*frame = cent->weaplerpframe[slot].frame;
	*backlerp = cent->weaplerpframe[slot].backlerp;
}

/*
Used for both the view weapon (ps is valid) and the world modelother character models (ps is nil)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
*/
void
addplayerweap(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, int slot)
{
	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;
	orientation_t lerped;
	int firingmask;

	weaponNum = cent->currstate.weapon[slot];

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

	gun.hModel = weapon->model.h;
	if(!gun.hModel)
		return;

	weapanim(cent, weapon, slot, &gun.oldframe, &gun.frame, &gun.backlerp);

	firingmask = EF_FIRING;
	if(slot == 1)
		firingmask = EF_FIRING2;
	else if(slot == 2)
		firingmask = EF_FIRING3;

	if(!ps){
		// add weapon ready sound
		if((cent->currstate.eFlags & firingmask) && weapon->firingsound){
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currstate.number, 2, cent->lerporigin, vec3_origin, weapon->firingsound);
		}else if(weapon->rdysound)
			trap_S_AddLoopingSound(cent->currstate.number, 2, cent->lerporigin, vec3_origin, weapon->rdysound);
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
	//gun.backlerp = parent->backlerp;

	addweapwithpowerups(&gun, cent->currstate.powerups);

	// add the spinning barrel
	if(weapon->barrelmodel){
		memset(&barrel, 0, sizeof(barrel));
		veccpy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelmodel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = machinegunspinangle(cent);
		angles2axis(angles, barrel.axis);

		rotentontag(&barrel, &gun, weapon->model.h, "tag_barrel");

		addweapwithpowerups(&barrel, cent->currstate.powerups);
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
	   && (nonPredictedCent->currstate.eFlags & firingmask)){
		// continuous flash
	}else
		// impulse flash
		if(cg.time - cent->muzzleflashtime[slot] > MUZZLE_FLASH_TIME)
			return;


	memset(&flash, 0, sizeof(flash));
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 100;
	angles2axis(angles, flash.axis);
	rotentontag(&flash, &gun, weapon->model.h, "tag_flash");

	// add lightning bolt
	if(ps || cg.thirdperson ||
	   cent->currstate.number != cg.pps.clientNum){
		lightningbolt(nonPredictedCent, flash.origin, slot);

		if(weapon->flashcolor[0] || weapon->flashcolor[1] || weapon->flashcolor[2])
			trap_R_AddLightToScene(flash.origin, 300 + (rand()&31), weapon->flashcolor[0],
					       weapon->flashcolor[1], weapon->flashcolor[2]);
	}

	veccpy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashmodel;
	if(!flash.hModel)
		return;

	// colorize the railgun blast
	if(weaponNum == WP_RAILGUN){
		clientInfo_t *ci;

		ci = &cgs.clientinfo[cent->currstate.clientNum];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	trap_R_AddRefEntityToScene(&flash);
}

/*
Add the weapon and flash for the player's view
*/
void
addviewweap(playerState_t *ps)
{
	refEntity_t hand;
	centity_t *cent;
	clientInfo_t *ci;
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
	// but still draw the lg
	if(!cg_drawGun.integer){
		vec3_t origin;

		if(cg.pps.eFlags & EF_FIRING){
			// special hack for lightning gun...
			veccpy(cg.refdef.vieworg, origin);
			vecmad(origin, -8, cg.refdef.viewaxis[2], origin);
			//lightningbolt(&cg_entities[ps->clientNum], origin, 0);
			//lightningbolt(&cg_entities[ps->clientNum], origin, 1);
			//lightningbolt(&cg_entities[ps->clientNum], origin, 2);
		}
		return;
	}

	// don't draw if testing a gun model
	if(cg.testgun)
		return;

	cent = &cg.pplayerent;	// &cg_entities[cg.snap->ps.clientNum];

	//
	// slot 0
	//
	registerweap(ps->weapon[0]);
	weapon = &cg_weapons[ps->weapon[0]];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	veccpy(cg.refdef.vieworg, hand.origin);
	veccpy(cg.refdefviewangles, angles);

	vecmad(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	vecmad(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	vecmad(hand.origin, cg_gun_z.value, cg.refdef.viewaxis[2], hand.origin);

	angles2axis(angles, hand.axis);

	// map torso animations to weapon animations
	if(cg_gun_frame.integer){
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}else{
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currstate.clientNum];
		hand.frame = maptorsotoweapframe(ci, cent->pe.torso.frame);
		hand.oldframe = maptorsotoweapframe(ci, cent->pe.torso.oldframe);
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsmodel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	addplayerweap(&hand, ps, &cg.pplayerent, ps->persistant[PERS_TEAM], 0);

	//
	// slot 1
	//
	registerweap(ps->weapon[1]);
	weapon = &cg_weapons[ps->weapon[1]];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	veccpy(cg.refdef.vieworg, hand.origin);
	veccpy(cg.refdefviewangles, angles);

	vecmad(hand.origin, cg_gun2_x.value, cg.refdef.viewaxis[0], hand.origin);
	vecmad(hand.origin, cg_gun2_y.value, cg.refdef.viewaxis[1], hand.origin);
	vecmad(hand.origin, cg_gun2_z.value, cg.refdef.viewaxis[2], hand.origin);

	angles2axis(angles, hand.axis);

	// map torso animations to weapon animations
	if(cg_gun_frame.integer){
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}else{
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currstate.clientNum];
		hand.frame = maptorsotoweapframe(ci, cent->pe.torso.frame);
		hand.oldframe = maptorsotoweapframe(ci, cent->pe.torso.oldframe);
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsmodel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	addplayerweap(&hand, ps, &cg.pplayerent, ps->persistant[PERS_TEAM], 1);
}

/*
Weapon selection.
*/

void
drawweapsel(void)
{
	const float sz = 18, h = 29, pad = 4;
	const int font = FONT1, fontsz = 8;
	float strh;
	float rectw, recth;
	int i, bits, count;
	float x, y, iconx, icony, selx, sely, labelx, labely;
	float *fade, *fade2;
	vec4_t priclr, secclr, bgclr;
	float starty;

	if(cg.pps.stats[STAT_HEALTH] <= 0)
		return;

	// display only if we're selecting either primary or secondary weapon
	fade = fadecolor(cg.weapseltime[0], WEAPON_SELECT_TIME);
	fade2 = fadecolor(cg.weapseltime[1], WEAPON_SELECT_TIME);
	if(!fade && !fade2)
		return;

	if(fade2[3] > fade[3])
		fade = fade2;

	trap_R_SetColor(fade);

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itempkuptime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[STAT_WEAPONS];
	bits &= ~(1 << WP_GRAPPLING_HOOK);		// do not display the grappling hook
	count = 0;
	for(i = 1; i < MAX_WEAPONS; i++)
		if(bits & (1 << i))
			count++;

	Vector4Copy(CLightBlue, priclr);
	Vector4Copy(CLightSalmon, secclr);
	priclr[3] = fade[3];
	secclr[3] = fade[3];

	VectorCopy4(fade, bgclr);
	bgclr[3] *= 0.07f;

	x = 2;
	starty = screenheight() - 160;

	y = starty;
	
	for(i = MAX_WEAPONS; i >= 1; i--)
		if(bits & (1 << i))
			registerweap(i);

	for(i = MAX_WEAPONS; i >= 1; i--){
		if(!(bits & (1 << i)))
			continue;
		if(cg_weapons[i].item->weapslot != 0)
			continue;

		selx = x + 8;
		sely = y;
		iconx = selx + pad;
		icony = sely + pad;
		labelx = selx + sz + 2*pad + 2;
		labely = y + 7;

		recth = h;
		rectw = labelx - x + sz + 2*pad + 4;

		fillrect(x, y, rectw - 1, recth - 1, bgclr);

		// draw weapon icon
		drawpic(iconx, icony, sz, sz, cg_weapons[i].icon);
		// ammo count
		if(cg.pps.ammo[i] != -1)
			drawstring(labelx, labely, va("%d", cg.pps.ammo[i]), font, 12, fade);

		// draw selection marker
		if(i == cg.weapsel[0]){
			// pri/sec
			drawstring(x + 2, y, "1", font, 7, priclr);
			// selector icon
			trap_R_SetColor(priclr);
			drawpic(selx, sely, sz + 2*pad, sz + 2*pad, cgs.media.selectShader);
			trap_R_SetColor(fade);
		}

		y -= h + 1;
	}

	x += 68;
	y = starty;

	for(i = MAX_WEAPONS; i >= 1; i--){
		if(!(bits & (1 << i)))
			continue;
		if(cg_weapons[i].item->weapslot != 1)
			continue;

		selx = x + 8;
		sely = y;
		iconx = selx + pad;
		icony = sely + pad;
		labelx = selx + sz + 2*pad + 2 + 4;
		labely = y + 7;

		recth = h;
		rectw = labelx - x + sz + 2*pad;

		if(i == cg.weapsel[1])
			registerweap(i);

		fillrect(x, y, rectw - 1, recth - 1, bgclr);

		// draw weapon icon
		drawpic(iconx, icony, sz, sz, cg_weapons[i].icon);
		// ammo count
		if(cg.pps.ammo[i] != -1)
			drawstring(labelx, labely, va("%d", cg.pps.ammo[i]), font, 12, fade);

		if(i == cg.weapsel[1]){
			// pri/sec
			drawstring(x + 2, y, "2", font, 7, secclr);
			// selector icon
			trap_R_SetColor(secclr);
			drawpic(selx, sely, sz + 2*pad, sz + 2*pad, cgs.media.selectShader);
			trap_R_SetColor(fade);
		}

		y -= h + 1;
	}
	trap_R_SetColor(nil);
}

static qboolean
weapselectable(int slot, int i)
{
	if(i <= WP_NONE || i >= WP_NUM_WEAPONS)
		return qfalse;
	if(slot != finditemforweapon(i)->weapslot)
		return qfalse;
	if(slot != 2 && i == WP_GRAPPLING_HOOK)
		return qfalse;
	if(!cg.snap->ps.ammo[i])
		return qfalse;
	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)))
		return qfalse;

	return qtrue;
}

void
CG_NextWeapon_f(void)
{
	int i, original, slot;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	slot = 0;
	if(cg.weapmod)
		slot = 1;

	cg.weapseltime[slot] = cg.time;
	original = cg.weapsel[slot];

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel[slot]++;
		if(cg.weapsel[slot] == MAX_WEAPONS)
			cg.weapsel[slot] = 0;
		if(cg.weapsel[slot] == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(slot, cg.weapsel[slot]))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel[slot] = original;
}

void
CG_PrevWeapon_f(void)
{
	int i, original, slot;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	slot = 0;
	if(cg.weapmod)
		slot = 1;

	cg.weapseltime[slot] = cg.time;
	original = cg.weapsel[slot];

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel[slot]--;
		if(cg.weapsel[slot] == -1)
			cg.weapsel[slot] = MAX_WEAPONS - 1;
		if(cg.weapsel[slot] == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(slot, cg.weapsel[slot]))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel[slot] = original;
}

void
CG_WeapModDown_f(void)
{
	cg.weapmod = qtrue;
}

void
CG_WeapModUp_f(void)
{
	cg.weapmod = qfalse;
}

void
CG_Weapon_f(void)
{
	int num, slot;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	num = atoi(cgargv(1));

	if(num < 1 || num >= WP_NUM_WEAPONS)
		return;

	slot = finditemforweapon(num)->weapslot;

	cg.weapseltime[slot] = cg.time;

	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)))
		return;	// don't have the weapon

	cg.weapsel[slot] = num;
}

/*
The current weapon has just run out of ammo
*/
void
outofammochange(int slot)
{
	int i;

	cg.weapseltime[slot] = cg.time;

	for(i = WP_NUM_WEAPONS-1; i > 0; i--)
		if(weapselectable(slot, i)){
			cg.weapsel[slot] = i;
			break;
		}
}

/*
Caused by an EV_FIRE_WEAPON event
*/
void
fireweap(centity_t *cent, int slot)
{
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;
	int firingmask;

	ent = &cent->currstate;
	if(ent->weapon[slot] == WP_NONE)
		return;
	if(ent->weapon[slot] >= WP_NUM_WEAPONS){
		cgerrorf("fireweap: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	weap = &cg_weapons[ent->weapon[slot]];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleflashtime[slot] = cg.time;

	firingmask = EF_FIRING;
	if(slot == 1)
		firingmask = EF_FIRING2;
	else if(slot == 2)
		firingmask = EF_FIRING3;

	// lightning gun only does this this on initial press
	if(ent->weapon[slot] == WP_LIGHTNING)
		if(cent->currstate.eFlags & firingmask)
			return;

	if(ent->weapon[slot] == WP_RAILGUN)
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

	setlerpframeanim(cgs.media.itemanims[finditemforweapon(cent->currstate.weapon[slot]) - bg_itemlist],
	   &cent->weaplerpframe[slot], ANIM_FLASH);
}

/*
Caused by an EV_MISSILE_MISS event, or directly by hitscan bullet tracing
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
	int i;

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
	case WP_PROX_LAUNCHER:
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_HOMING_LAUNCHER:
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = PICKRANDOM(cgs.media.sfx_rockexp);
		mark = cgs.media.burnMarkShader;
		radius = 100;
		light = 300;
		isSprite = qtrue;
		duration = 16*16.666666f;
		lightcolor[0] = 0.9f;
		lightcolor[1] = 0.45f;
		lightcolor[2] = 0.0f;

		for(i = 0; i < cg_rocketExpSparks.integer; i++)
			particlesparks(origin, dir, 500+crandom()*400, 60, 600 + crandom()*140);

		if(cg_rocketExpShockwave.integer)
			shockwave(origin, 400);

		// flame
		for(i = 0; i < 10; i++){
			vec3_t pt;

			vecset(pt, crandom(), crandom(), crandom());
			vecmul(pt, 45, pt);
			vecadd(pt, origin, pt);
			le = explosion(pt, dir, mod, shader, duration-6, isSprite);
		}

		// upper flame
		for(i = 0; i < 14; i++){
			vec3_t pt;

			vecset(pt, crandom(), crandom(), crandom());
			vecmad(dir, 0.7f, pt, pt);
			vecmul(pt, 60, pt);
			vecadd(origin, pt, pt);
			le = explosion(pt, dir, mod, shader, duration, isSprite);
		}

		// smoke
		if(cg_rocketExpSmoke.integer){
			for(i = 0; i < 24; i++){
				vec3_t pt;


			vecset(pt, crandom(), crandom(), crandom());
			vecmad(dir, 0.7f, pt, pt);
			vecmul(pt, 70, pt);
			vecadd(origin, pt, pt);
				smokepuff(pt, dir, 140, .6, .6, .6, 1, 4*duration,
				   cg.time-300, 0, 0, cgs.media.shotgunSmokePuffShader);
			}
		}

		mod = 0;	// don't draw the usual sprite
		break;
	case WP_CHAINGUN:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bulletExplosionShader;
		sfx = PICKRANDOM(cgs.media.sfx_rockexp);
		mark = cgs.media.burnMarkShader;
		radius = 20;
		//light = 100;
		isSprite = qtrue;
		duration = 16*16.666666f;
		lightcolor[0] = 0.9f;
		lightcolor[1] = 0.45f;
		lightcolor[2] = 0.0f;

		for(i = 0; i < cg_rocketExpSparks.integer / 20; i++)
			particlesparks(origin, dir, 500+crandom()*400, 60, 600 + crandom()*140);

		if(cg_rocketExpShockwave.integer)
			shockwave(origin, 50);

		// flame
		for(i = 0; i < 4; i++){
			vec3_t pt;

			vecset(pt, crandom(), crandom(), crandom());
			vecmul(pt, 20, pt);
			vecadd(pt, origin, pt);
			le = explosion(pt, dir, mod, shader, duration-6, isSprite);
		}

		mod = 0;	// don't draw the usual sprite
		break;
	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = cgs.media.sfx_ric1;
		radius = 5;
		duration = 100;
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		//sfx = cgs.media.sfx_railg;
		sfx = PICKRANDOM(cgs.media.sfx_plasmaexp);
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = PICKRANDOM(cgs.media.sfx_plasmaexp);
		mark = cgs.media.energyMarkShader;
		isSprite = qtrue;
		radius = 8;
		duration = 6*16.666666f;

		// flame
		for(i = 0; i < 5; i++){
			vec3_t pt;

			vecset(pt, crandom(), crandom(), crandom());
			vecmul(pt, 7, pt);
			vecadd(pt, origin, pt);
			le = explosion(pt, dir, mod, shader, duration, isSprite);
		}

		mod = 0;	// don't draw the usual sprite
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = cgs.media.sfx_ric1;
		radius = 5;
		duration = 100;
		break;
	}

	if(sfx)
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);

	// create the explosion
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

	// impact mark
	alphafade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if(weapon == WP_RAILGUN){
		float *color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		impactmark(mark, origin, dir, random()*360, color[0], color[1], color[2], 1, alphafade, radius, qfalse);
	}else
		impactmark(mark, origin, dir, random()*360, 1, 1, 1, 1, alphafade, radius, qfalse);
}

void
missilehitplayer(int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch(weapon){
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_HOMING_LAUNCHER:
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

static void
shotgunpellet(vec3_t start, vec3_t end, int skipNum)
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
Perform the same traces the server did to locate the
hit splashes.

This must match code/game/g_weapon.c:/shotgunpattern/
*/
static void
shotgunpattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum)
{
	int i;
	float r, u, spread, angle;
	vec3_t end, forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	vecnorm2(origin2, forward);
	vecperp(right, forward );
	veccross(forward, right, up);

	angle = 360.0f / DEFAULT_SHOTGUN_COUNT;

	// generate the spread pattern
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = sin(DEG2RAD(angle + (45 * i)));
		u = cos(DEG2RAD(angle + (45 * i)));
		spread = i < 8? 0.5f*DEFAULT_SHOTGUN_SPREAD : DEFAULT_SHOTGUN_SPREAD;
		r *= spread * 16;
		u *= spread * 16;
		vecmad(origin, 8192 * 16, forward, end);
		vecmad(end, r, right, end);
		vecmad(end, u, up, end);
		shotgunpellet(origin, end, otherEntNum);
	}
}

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
	shotgunpattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum);
}

static qboolean
calcmuzzlepoint(int entityNum, vec3_t muzzle)
{
	vec3_t forward, up;
	centity_t *cent;

	cent = &cg_entities[entityNum];

	if(entityNum == cg.snap->ps.clientNum){
		playerpos(cent, muzzle);
		anglevecs(cg.pps.viewangles, forward, nil, up);
		vecmad(muzzle, -14, up, muzzle);
		vecmad(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	if(!cent->currvalid)
		return qfalse;

	veccpy(cent->lerporigin, muzzle);

	anglevecs(cent->lerpangles, forward, nil, nil);

	vecmad(muzzle, 14, forward, muzzle);

	return qtrue;
}

/*
Renders bullet effects.
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
		if(calcmuzzlepoint(sourceEntityNum, start)){
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
		}

	// impact splash and mark
	if(flesh)
		bleed(end, fleshEntityNum);
	else
		missilehitwall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);

}
