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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

/*
Modifies the entities position and axis by the given
tag location
*/
void
entontag(refEntity_t *entity, const refEntity_t *parent,
		       qhandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
Modifies the entities position and axis by the given
tag location
*/
void
rotentontag(refEntity_t *entity, const refEntity_t *parent,
			      qhandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t*)parent)->axis, entity->axis);
}

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
Also called by event processing code
*/
void
setentsoundpos(centity_t *cent)
{
	if(cent->currstate.solid == SOLID_BMODEL){
		vec3_t origin;
		float *v;

		v = cgs.inlinemodelmidpoints[cent->currstate.modelindex];
		vecadd(cent->lerporigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currstate.number, origin);
	}else
		trap_S_UpdateEntityPosition(cent->currstate.number, cent->lerporigin);
}

/*
Add continuous entity effects, like local entity emission and lighting
*/
static void
entfx(centity_t *cent)
{
	// update sound origins
	setentsoundpos(cent);

	// add loop sound
	if(cent->currstate.loopSound){
		if(cent->currstate.eType != ET_SPEAKER)
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin,
					       cgs.gamesounds[cent->currstate.loopSound]);
		else
			trap_S_AddRealLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin,
						   cgs.gamesounds[cent->currstate.loopSound]);
	}

	// constant light glow
	if(cent->currstate.constantLight){
		int cl;
		float i, r, g, b;

		cl = cent->currstate.constantLight;
		r = (float)(cl & 0xFF) / 255.0;
		g = (float)((cl >> 8) & 0xFF) / 255.0;
		b = (float)((cl >> 16) & 0xFF) / 255.0;
		i = (float)((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerporigin, i, r, g, b);
	}
}

static void
dogeneral(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// if set to invisible, skip
	if(!s1->modelindex)
		return;

	memset(&ent, 0, sizeof(ent));

	// set frame
	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	ent.hModel = cgs.gamemodels[s1->modelindex];

	// player model
	if(s1->number == cg.snap->ps.clientNum)
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors

	// convert angles to axis
	angles2axis(cent->lerpangles, ent.axis);

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
Speaker entities can automatically play sounds
*/
static void
dospeaker(centity_t *cent)
{
	if(!cent->currstate.clientNum)	// FIXME: use something other than clientNum...
		return;	// not auto triggering

	if(cg.time < cent->misctime)
		return;

	trap_S_StartSound(nil, cent->currstate.number, CHAN_ITEM, cgs.gamesounds[cent->currstate.eventParm]);

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->misctime = cg.time + cent->currstate.frame * 100 + cent->currstate.clientNum * 100 * crandom();
}

static void
doitem(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *es;
	gitem_t *item;
	int msec;
	float frac;
	float scale;
	weaponInfo_t *wi;
	vec3_t tmp;

	es = &cent->currstate;
	if(es->modelindex >= bg_nitems)
		cgerrorf("Bad item index %i on entity", es->modelindex);

	// if set to invisible, skip
	if(!es->modelindex || (es->eFlags & EF_NODRAW))
		return;

	item = &bg_itemlist[es->modelindex];
	if(cg_simpleItems.integer && item->type != IT_TEAM){
		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		veccpy(cent->lerporigin, ent.origin);
		ent.radius = 14*cg_simpleItems.value;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	memset(&ent, 0, sizeof(ent));

	// item rotation

	AxisCopy(cg.refdef.viewaxis, ent.axis);
	// create an arbitrary ent.axis[1]
	veccpy(ent.axis[1], tmp);
	// rotate it around "up" axis, axis[2]
	RotatePointAroundVector(ent.axis[1], ent.axis[2], tmp, cg.time/4);
	veccross(ent.axis[2], ent.axis[1], ent.axis[0]);

	// items bob up and down on their "up" axis
	scale = 0.005 + cent->currstate.number * 0.00001;
	vecmad(cent->lerporigin, 4 + cos((cg.time + 1000) *  scale) * 4, ent.axis[2], cent->lerporigin);

	// the weapons have their origin where they attatch to player
	// models, so we need to place them at their midpoint or they
	// will rotate eccentricly

	wi = nil;
	if(item->type == IT_WEAPON){
		wi = &cg_weapons[item->tag];
		cent->lerporigin[0] -=
			wi->midpoint[0] * ent.axis[0][0] +
			wi->midpoint[1] * ent.axis[1][0] +
			wi->midpoint[2] * ent.axis[2][0];
		cent->lerporigin[1] -=
			wi->midpoint[0] * ent.axis[0][1] +
			wi->midpoint[1] * ent.axis[1][1] +
			wi->midpoint[2] * ent.axis[2][1];
		cent->lerporigin[2] -=
			wi->midpoint[0] * ent.axis[0][2] +
			wi->midpoint[1] * ent.axis[1][2] +
			wi->midpoint[2] * ent.axis[2][2];
	}

	if(item->type == IT_WEAPON && item->tag == WP_RAILGUN){
		clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
		Byte4Copy(ci->c1rgba, ent.shaderRGBA);
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->misctime;
	if(msec >= 0 && msec < ITEM_SCALEUP_TIME){
		frac = (float)msec / ITEM_SCALEUP_TIME;
		vecmul(ent.axis[0], frac, ent.axis[0]);
		vecmul(ent.axis[1], frac, ent.axis[1]);
		vecmul(ent.axis[2], frac, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}else
		frac = 1.0;

	ent.renderfx |= RF_MINLIGHT;

	// increase the size of the weapons when they are presented as items
	if(item->type == IT_WEAPON){
		vecmul(ent.axis[0], 1.5, ent.axis[0]);
		vecmul(ent.axis[1], 1.5, ent.axis[1]);
		vecmul(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}

#ifdef MISSIONPACK
	if(item->type == IT_HOLDABLE && item->tag == HI_KAMIKAZE){
		vecmul(ent.axis[0], 2, ent.axis[0]);
		vecmul(ent.axis[1], 2, ent.axis[1]);
		vecmul(ent.axis[2], 2, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}
#endif

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if(item->type == IT_WEAPON && wi && wi->barrelmodel){
		refEntity_t barrel;
		vec3_t angles;

		memset(&barrel, 0, sizeof(barrel));

		barrel.hModel = wi->barrelmodel;

		veccpy(ent.lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = 0;
		angles2axis(angles, barrel.axis);

		rotentontag(&barrel, &ent, wi->model.h, "tag_barrel");

		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene(&barrel);
	}

	// accompanying rings / spheres for powerups
	if(!cg_simpleItems.integer){
		if(item->type == IT_HEALTH || item->type == IT_POWERUP || item->type == IT_SHIELD)
			if((ent.hModel = cg_items[es->modelindex].models[1]) != 0){
				// scale up if respawning
				if(frac != 1.0){
					vecmul(ent.axis[0], frac, ent.axis[0]);
					vecmul(ent.axis[1], frac, ent.axis[1]);
					vecmul(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}

			if((ent.hModel = cg_items[es->modelindex].models[2]) != 0){
				AxisCopy(cg.refdef.viewaxis, ent.axis);
				// create an arbitrary ent.axis[1]
				veccpy(ent.axis[1], tmp);
				veccpy(ent.axis[1], tmp);
				RotatePointAroundVector(ent.axis[1], ent.axis[2], tmp, cg.time/6);
				veccross(ent.axis[2], ent.axis[1], ent.axis[0]);

				// scale up if respawning
				if(frac != 1.0){
					vecmul(ent.axis[0], frac, ent.axis[0]);
					vecmul(ent.axis[1], frac, ent.axis[1]);
					vecmul(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}

			if((ent.hModel = cg_items[es->modelindex].models[3]) != 0){
				AxisCopy(cg.refdef.viewaxis, ent.axis);
				// create an arbitrary ent.axis[1]
				veccpy(ent.axis[1], tmp);
				veccpy(ent.axis[1], tmp);
				RotatePointAroundVector(ent.axis[1], ent.axis[2], tmp, cg.time/8);
				veccross(ent.axis[2], ent.axis[1], ent.axis[0]);

				// scale up if respawning
				if(frac != 1.0){
					vecmul(ent.axis[0], frac, ent.axis[0]);
					vecmul(ent.axis[1], frac, ent.axis[1]);
					vecmul(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}
	}
}

//============================================================================

static void
domissile(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	const weaponInfo_t *weapon;
//	int	col;

	s1 = &cent->currstate;
	if(s1->weapon[0] >= WP_NUM_WEAPONS)
		s1->weapon[0] = 0;
	weapon = &cg_weapons[s1->weapon[0]];

	// calculate the axis
	veccpy(s1->angles, cent->lerpangles);

	// add trails
	if(weapon->missileTrailFunc)
		weapon->missileTrailFunc(cent, weapon);
	/*
	if ( cent->currstate.modelindex == TEAM_RED ) {
	        col = 1;
	}
	else if ( cent->currstate.modelindex == TEAM_BLUE ) {
	        col = 2;
	}
	else {
	        col = 0;
	}

	// add dynamic light
	if ( weapon->missilelight ) {
	        trap_R_AddLightToScene(cent->lerporigin, weapon->missilelight,
	                weapon->missilelightcolor[col][0], weapon->missilelightcolor[col][1], weapon->missilelightcolor[col][2] );
	}
	*/
	// add dynamic light
	if(weapon->missilelight)
		trap_R_AddLightToScene(cent->lerporigin, weapon->missilelight,
				       weapon->missilelightcolor[0], weapon->missilelightcolor[1], weapon->missilelightcolor[2]);

	// add missile sound
	if(weapon->missilesound){
		vec3_t velocity;

		evaltrajectorydelta(&cent->currstate.pos, cg.time, velocity);

		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, velocity, weapon->missilesound);
	}

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clframe & 1;
	if(!weapon->missilemodel)
		return;
	ent.hModel = weapon->missilemodel;
	ent.renderfx = weapon->missilerenderfx | RF_MINLIGHT;


	// convert direction of travel into axis
	if(VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	// spin as it moves
	if(s1->pos.trType != TR_STATIONARY)
		RotateAroundDirection(ent.axis, cg.time / 16);
	else{
		{
			RotateAroundDirection(ent.axis, s1->time);
		}
	}

	// add to refresh list, possibly with quad glow
	addrefentitywithpowerups(&ent, s1, TEAM_FREE);
}

/*
This is called when the grapple is sitting up against the wall
*/
static void
dograpple(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	const weaponInfo_t *weapon;

	s1 = &cent->currstate;
	if(s1->weapon[0] >= WP_NUM_WEAPONS)
		s1->weapon[0] = 0;
	weapon = &cg_weapons[s1->weapon[0]];

	// calculate the axis
	veccpy(s1->angles, cent->lerpangles);

#if 0	// FIXME add grapple pull sound here..?
	// add missile sound
	if(weapon->missilesound)
		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->missilesound);

#endif

	// Will draw cable if needed
	grappletrail(cent, weapon);

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clframe & 1;
	ent.hModel = weapon->missilemodel;
	ent.renderfx = weapon->missilerenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if(VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	trap_R_AddRefEntityToScene(&ent);
}

static void
domover(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);
	angles2axis(cent->lerpangles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if(s1->solid == SOLID_BMODEL)
		ent.hModel = cgs.inlinedrawmodel[s1->modelindex];
	else
		ent.hModel = cgs.gamemodels[s1->modelindex];

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if(s1->modelindex2){
		ent.skinNum = 0;
		ent.hModel = cgs.gamemodels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}
}

/*
Also called as an event
*/
void
drawbeam(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(s1->pos.trBase, ent.origin);
	veccpy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

static void
doportal(centity_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	vecperp(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	vecsub(vec3_origin, ent.axis[1], ent.axis[1]);

	veccross(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;				// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
Also called by client movement prediction code
*/
void
adjustposformover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out)
{
	centity_t *cent;
	vec3_t oldorigin, origin, deltaOrigin;
	vec3_t oldAngles, angles, deltaAngles;

	if(moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL){
		veccpy(in, out);
		veccpy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[moverNum];
	if(cent->currstate.eType != ET_MOVER){
		veccpy(in, out);
		veccpy(angles_in, angles_out);
		return;
	}

	evaltrajectory(&cent->currstate.pos, fromTime, oldorigin);
	evaltrajectory(&cent->currstate.apos, fromTime, oldAngles);

	evaltrajectory(&cent->currstate.pos, toTime, origin);
	evaltrajectory(&cent->currstate.apos, toTime, angles);

	vecsub(origin, oldorigin, deltaOrigin);
	vecsub(angles, oldAngles, deltaAngles);

	vecadd(in, deltaOrigin, out);
	vecadd(angles_in, deltaAngles, angles_out);
	// FIXME: origin change when on a rotating object
}

static void
lerpentpos(centity_t *cent)
{
	vec3_t current, next;
	float f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if(cg.nextsnap == nil)
		cgerrorf("CG_InterpoateEntityPosition: cg.nextsnap == nil");

	f = cg.frameinterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	evaltrajectory(&cent->currstate.pos, cg.snap->serverTime, current);
	evaltrajectory(&cent->nextstate.pos, cg.nextsnap->serverTime, next);

	cent->lerporigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerporigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerporigin[2] = current[2] + f * (next[2] - current[2]);

	evaltrajectory(&cent->currstate.apos, cg.snap->serverTime, current);
	evaltrajectory(&cent->nextstate.apos, cg.nextsnap->serverTime, next);

	cent->lerpangles[0] = LerpAngle(current[0], next[0], f);
	cent->lerpangles[1] = LerpAngle(current[1], next[1], f);
	cent->lerpangles[2] = LerpAngle(current[2], next[2], f);
}

static void
lerpentitypositions(centity_t *cent)
{
	// if this player does not want to see extrapolated players
	if(!cg_smoothClients.integer)
		// make sure the clients use TR_INTERPOLATE
		if(cent->currstate.number < MAX_CLIENTS){
			cent->currstate.pos.trType = TR_INTERPOLATE;
			cent->nextstate.pos.trType = TR_INTERPOLATE;
		}

	if(cent->interpolate && cent->currstate.pos.trType == TR_INTERPOLATE){
		lerpentpos(cent);
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if(cent->interpolate && cent->currstate.pos.trType == TR_LINEAR_STOP &&
	   cent->currstate.number < MAX_CLIENTS){
		lerpentpos(cent);
		return;
	}

	// just use the current frame and evaluate as best we can
	evaltrajectory(&cent->currstate.pos, cg.time, cent->lerporigin);
	evaltrajectory(&cent->currstate.apos, cg.time, cent->lerpangles);

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if(cent != &cg.pplayerent)
		adjustposformover(cent->lerporigin, cent->currstate.groundEntityNum,
					  cg.snap->serverTime, cg.time, cent->lerporigin, cent->lerpangles, cent->lerpangles);
}

static void
doteambase(centity_t *cent)
{
	refEntity_t model;
#ifdef MISSIONPACK
	vec3_t angles;
	int t, h;
	float c;

	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF){
#else
	if(cgs.gametype == GT_CTF){
#endif
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		veccpy(cent->lerporigin, model.lightingOrigin);
		veccpy(cent->lerporigin, model.origin);
		angles2axis(cent->currstate.angles, model.axis);
		if(cent->currstate.modelindex == TEAM_RED)
			model.hModel = cgs.media.redFlagBaseModel;
		else if(cent->currstate.modelindex == TEAM_BLUE)
			model.hModel = cgs.media.blueFlagBaseModel;
		else
			model.hModel = cgs.media.neutralFlagBaseModel;
		trap_R_AddRefEntityToScene(&model);
	}
#ifdef MISSIONPACK
	else if(cgs.gametype == GT_OBELISK){
		// show the obelisk
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		veccpy(cent->lerporigin, model.lightingOrigin);
		veccpy(cent->lerporigin, model.origin);
		angles2axis(cent->currstate.angles, model.axis);

		model.hModel = cgs.media.overloadBaseModel;
		trap_R_AddRefEntityToScene(&model);
		// if hit
		if(cent->currstate.frame == 1){
			// show hit model
			// modelindex2 is the health value of the obelisk
			c = cent->currstate.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			model.hModel = cgs.media.overloadEnergyModel;
			trap_R_AddRefEntityToScene(&model);
		}
		// if respawning
		if(cent->currstate.frame == 2){
			if(!cent->misctime)
				cent->misctime = cg.time;
			t = cg.time - cent->misctime;
			h = (cg_obeliskRespawnDelay.integer - 5) * 1000;
			if(t > h){
				c = (float)(t - h) / h;
				if(c > 1)
					c = 1;
			}else
				c = 0;
			// show the lights
			angles2axis(cent->currstate.angles, model.axis);
			model.shaderRGBA[0] = c * 0xff;
			model.shaderRGBA[1] = c * 0xff;
			model.shaderRGBA[2] = c * 0xff;
			model.shaderRGBA[3] = c * 0xff;

			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene(&model);
			// show the target
			if(t > h){
				if(!cent->muzzleflashtime[0]){
					trap_S_StartSound(cent->lerporigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.obeliskRespawnSound);
					cent->muzzleflashtime[0] = 1;
				}
				veccpy(cent->currstate.angles, angles);
				angles[YAW] += (float)16 * acos(1-c) * 180 / M_PI;
				angles2axis(angles, model.axis);

				vecmul(model.axis[0], c, model.axis[0]);
				vecmul(model.axis[1], c, model.axis[1]);
				vecmul(model.axis[2], c, model.axis[2]);

				model.shaderRGBA[0] = 0xff;
				model.shaderRGBA[1] = 0xff;
				model.shaderRGBA[2] = 0xff;
				model.shaderRGBA[3] = 0xff;
				model.origin[2] += 56;
				model.hModel = cgs.media.overloadTargetModel;
				trap_R_AddRefEntityToScene(&model);
			}else{
				//FIXME: show animated smoke
			}
		}else{
			cent->misctime = 0;
			cent->muzzleflashtime[0] = 0;
			cent->muzzleflashtime[1] = 0;
			cent->muzzleflashtime[2] = 0;
			// modelindex2 is the health value of the obelisk
			c = cent->currstate.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			// show the lights
			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene(&model);
			// show the target
			model.origin[2] += 56;
			model.hModel = cgs.media.overloadTargetModel;
			trap_R_AddRefEntityToScene(&model);
		}
	}else if(cgs.gametype == GT_HARVESTER){
		// show harvester model
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		veccpy(cent->lerporigin, model.lightingOrigin);
		veccpy(cent->lerporigin, model.origin);
		angles2axis(cent->currstate.angles, model.axis);

		if(cent->currstate.modelindex == TEAM_RED){
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterRedSkin;
		}else if(cent->currstate.modelindex == TEAM_BLUE){
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterBlueSkin;
		}else{
			model.hModel = cgs.media.harvesterNeutralModel;
			model.customSkin = 0;
		}
		trap_R_AddRefEntityToScene(&model);
	}
#endif
}

static void
addcentity(centity_t *cent)
{
	// event-only entities will have been dealt with already
	if(cent->currstate.eType >= ET_EVENTS)
		return;

	// calculate the current origin
	lerpentitypositions(cent);

	// add automatic effects
	entfx(cent);

	switch(cent->currstate.eType){
	default:
		cgerrorf("Bad entity type: %i", cent->currstate.eType);
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TRIGGER_GRAVITY:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		dogeneral(cent);
		break;
	case ET_PLAYER:
		doplayer(cent);
		break;
	case ET_ITEM:
		doitem(cent);
		break;
	case ET_MISSILE:
		domissile(cent);
		break;
	case ET_MOVER:
		domover(cent);
		break;
	case ET_BEAM:
		drawbeam(cent);
		break;
	case ET_PORTAL:
		doportal(cent);
		break;
	case ET_SPEAKER:
		dospeaker(cent);
		break;
	case ET_GRAPPLE:
		dograpple(cent);
		break;
	case ET_TEAM:
		doteambase(cent);
		break;
	case ET_POINTLIGHT:
		trap_R_AddLightToScene(cent->currstate.origin, cent->currstate.lightintensity,
		   cent->currstate.lightcolor[0], cent->currstate.lightcolor[1],
		   cent->currstate.lightcolor[2]);
		break;
	}
}

void
addpacketents(void)
{
	int num;
	centity_t *cent;
	playerState_t *ps;

	// set cg.frameinterpolation
	if(cg.nextsnap){
		int delta;

		delta = (cg.nextsnap->serverTime - cg.snap->serverTime);
		if(delta == 0)
			cg.frameinterpolation = 0;
		else
			cg.frameinterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
	}else
		cg.frameinterpolation = 0;	// actually, it should never be used, because
		// no entities should be marked as interpolating

	// generate and add the entity from the playerstate
	ps = &cg.pps;
	ps2es(ps, &cg.pplayerent.currstate, qfalse);
	addcentity(&cg.pplayerent);

	// lerp the non-predicted value for lightning gun origins
	lerpentitypositions(&cg_entities[cg.snap->ps.clientNum]);

	// add each entity sent over by the server
	for(num = 0; num < cg.snap->numEntities; num++){
		cent = &cg_entities[cg.snap->entities[num].number];
		addcentity(cent);
	}
}
