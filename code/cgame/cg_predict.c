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
// cg_predict.c -- this file generates cg.pps by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static pmove_t cg_pmove;

static int cg_numSolidEntities;
static centity_t *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static int cg_numTriggerEntities;
static centity_t *cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
mksolidlist

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void
mksolidlist(void)
{
	int i;
	centity_t *cent;
	snapshot_t *snap;
	entityState_t *ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if(cg.nextsnap && !cg.teleportnextframe && !cg.teleportthisframe)
		snap = cg.nextsnap;
	else
		snap = cg.snap;

	for(i = 0; i < snap->numEntities; i++){
		cent = &cg_entities[snap->entities[i].number];
		ent = &cent->currstate;

		if(ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER || ET_TRIGGER_GRAVITY){
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if(cent->nextstate.solid){
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void
CG_ClipMoveToEntities(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
		      int skipNumber, int mask, trace_t *tr)
{
	int i, x, zd, zu;
	trace_t trace;
	entityState_t *ent;
	clipHandle_t cmodel;
	vec3_t bmins, bmaxs;
	vec3_t origin, angles;
	centity_t *cent;

	for(i = 0; i < cg_numSolidEntities; i++){
		cent = cg_solidEntities[i];
		ent = &cent->currstate;

		if(ent->number == skipNumber)
			continue;

		if(ent->solid == SOLID_BMODEL){
			// special value for bmodel
			cmodel = trap_CM_InlineModel(ent->modelindex);
			veccpy(cent->lerpangles, angles);
			evaltrajectory(&cent->currstate.pos, cg.phystime, origin);
		}else{
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel(bmins, bmaxs);
			veccpy(vec3_origin, angles);
			veccpy(cent->lerporigin, origin);
		}

		trap_CM_TransformedBoxTrace(&trace, start, end,
					    mins, maxs, cmodel, mask, origin, angles);

		if(trace.allsolid || trace.fraction < tr->fraction){
			trace.entityNum = ent->number;
			*tr = trace;
		}else if(trace.startsolid)
			tr->startsolid = qtrue;
		if(tr->allsolid)
			return;
	}
}

/*
================
cgtrace
================
*/
void
cgtrace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	 int skipNumber, int mask)
{
	trace_t t;

	trap_CM_BoxTrace(&t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
pointcontents
================
*/
int
pointcontents(const vec3_t point, int passEntityNum)
{
	int i;
	entityState_t *ent;
	centity_t *cent;
	clipHandle_t cmodel;
	int contents;

	contents = trap_CM_PointContents(point, 0);

	for(i = 0; i < cg_numSolidEntities; i++){
		cent = cg_solidEntities[i];

		ent = &cent->currstate;

		if(ent->number == passEntityNum)
			continue;

		if(ent->solid != SOLID_BMODEL)		// special value for bmodel
			continue;

		cmodel = trap_CM_InlineModel(ent->modelindex);
		if(!cmodel)
			continue;

		contents |= trap_CM_TransformedPointContents(point, cmodel, cent->lerporigin, cent->lerpangles);
	}

	return contents;
}

/*
========================
CG_InterpolatePlayerState

Generates cg.pps by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void
CG_InterpolatePlayerState(qboolean grabAngles)
{
	float f;
	int i;
	playerState_t *out;
	snapshot_t *prev, *next;

	out = &cg.pps;
	prev = cg.snap;
	next = cg.nextsnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if(grabAngles){
		usercmd_t cmd;
		int cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd(cmdNum, &cmd);

		PM_UpdateViewAngles(out, &cmd);
	}

	// if the next frame is a teleport, we can't lerp to it
	if(cg.teleportnextframe)
		return;

	if(!next || next->serverTime <= prev->serverTime)
		return;

	f = (float)(cg.time - prev->serverTime) / (next->serverTime - prev->serverTime);

	i = next->ps.bobCycle;
	if(i < prev->ps.bobCycle)
		i += 256;	// handle wraparound
	out->bobCycle = prev->ps.bobCycle + f * (i - prev->ps.bobCycle);

	for(i = 0; i < 3; i++){
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i]);
		if(!grabAngles)
			out->viewangles[i] = LerpAngle(
				prev->ps.viewangles[i], next->ps.viewangles[i], f);
		out->velocity[i] = prev->ps.velocity[i] +
				   f * (next->ps.velocity[i] - prev->ps.velocity[i]);
	}
}

/*
===================
CG_TouchItem
===================
*/
static void
CG_TouchItem(centity_t *cent)
{
	gitem_t *item;

	if(!cg_predictItems.integer)
		return;
	if(!playertouchingitem(&cg.pps, &cent->currstate, cg.time))
		return;

	// never pick an item up twice in a prediction
	if(cent->misctime == cg.time)
		return;

	if(!cangrabitem(cgs.gametype, &cent->currstate, &cg.pps))
		return;	// can't hold it

	item = &bg_itemlist[cent->currstate.modelindex];

	// Special case for flags.
	// We don't predict touching our own flag
#ifdef MISSIONPACK
	if(cgs.gametype == GT_1FCTF)
		if(item->type == IT_TEAM && item->tag != PW_NEUTRALFLAG)
			return;

#endif
	if(cgs.gametype == GT_CTF){
		if(cg.pps.persistant[PERS_TEAM] == TEAM_RED &&
		   item->type == IT_TEAM && item->tag == PW_REDFLAG)
			return;
		if(cg.pps.persistant[PERS_TEAM] == TEAM_BLUE &&
		   item->type == IT_TEAM && item->tag == PW_BLUEFLAG)
			return;
	}

	// grab it
	bgaddpredictableevent(EV_ITEM_PICKUP, cent->currstate.modelindex, &cg.pps);

	// remove it from the frame so it won't be drawn
	cent->currstate.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->misctime = cg.time;

	// if it's a weapon, give them some predicted ammo so the autoswitch will work
	if(item->type == IT_WEAPON){
		cg.pps.stats[STAT_WEAPONS] |= 1 << item->tag;
		if(!cg.pps.ammo[item->tag])
			cg.pps.ammo[item->tag] = 1;
	}
}

/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void
CG_TouchTriggerPrediction(void)
{
	int i;
	trace_t trace;
	entityState_t *ent;
	clipHandle_t cmodel;
	centity_t *cent;
	qboolean spectator;

	// dead clients don't activate triggers
	if(cg.pps.stats[STAT_HEALTH] <= 0)
		return;

	spectator = (cg.pps.pm_type == PM_SPECTATOR);

	if(cg.pps.pm_type != PM_NORMAL && !spectator)
		return;

	for(i = 0; i < cg_numTriggerEntities; i++){
		cent = cg_triggerEntities[i];
		ent = &cent->currstate;

		if(ent->eType == ET_ITEM && !spectator){
			CG_TouchItem(cent);
			continue;
		}

		if(ent->solid != SOLID_BMODEL)
			continue;

		cmodel = trap_CM_InlineModel(ent->modelindex);
		if(!cmodel)
			continue;

		trap_CM_BoxTrace(&trace, cg.pps.origin, cg.pps.origin,
				 cg_pmove.mins, cg_pmove.maxs, cmodel, -1);

		if(!trace.startsolid)
			continue;

		if(ent->eType == ET_TELEPORT_TRIGGER)
			cg.hyperspace = qtrue;
		else if(ent->eType == ET_PUSH_TRIGGER)
			touchjumppad(&cg.pps, ent);
		else if(ent->eType == ET_TRIGGER_GRAVITY)
			BG_TouchTriggerGravity(&cg.pps, ent, cg.frametime);
	}

	// if we didn't touch a jump pad this pmove frame
	if(cg.pps.jumppad_frame != cg.pps.pmove_framecount){
		cg.pps.jumppad_frame = 0;
		cg.pps.jumppad_ent = 0;
	}
}

/*
=================
predictplayerstate

Generates cg.pps for the current cg.time
cg.pps is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void
predictplayerstate(void)
{
	int cmdNum, current;
	playerState_t oldPlayerState;
	qboolean moved;
	usercmd_t oldestCmd;
	usercmd_t latestCmd;

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// pps is valid even if there is some
	// other error condition
	if(!cg.validpps){
		cg.validpps = qtrue;
		cg.pps = cg.snap->ps;
	}

	// demo playback just copies the moves
	if(cg.demoplayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)){
		CG_InterpolatePlayerState(qfalse);
		return;
	}

	// non-predicting local movement will grab the latest angles
	if(cg_nopredict.integer || cg_synchronousClients.integer){
		CG_InterpolatePlayerState(qtrue);
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.pps;
	cg_pmove.trace = cgtrace;
	cg_pmove.pointcontents = pointcontents;
	if(cg_pmove.ps->pm_type == PM_DEAD)
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	else
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	cg_pmove.nofootsteps = (cgs.dmflags & DF_NO_FOOTSTEPS) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.pps;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &oldestCmd);
	if(oldestCmd.serverTime > cg.snap->ps.commandTime
	   && oldestCmd.serverTime < cg.time){	// special check for map_restart
		if(cg_showmiss.integer)
			cgprintf("exceeded PACKET_BACKUP on commands\n");
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd(current, &latestCmd);

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	if(cg.nextsnap && !cg.teleportnextframe && !cg.teleportthisframe){
		cg.pps = cg.nextsnap->ps;
		cg.phystime = cg.nextsnap->serverTime;
	}else{
		cg.pps = cg.snap->ps;
		cg.phystime = cg.snap->serverTime;
	}

	if(pmove_msec.integer < 8)
		trap_Cvar_Set("pmove_msec", "8");
	else if(pmove_msec.integer > 33)
		trap_Cvar_Set("pmove_msec", "33");

	cg_pmove.pmove_fixed = pmove_fixed.integer;	// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	// run cmds
	moved = qfalse;
	for(cmdNum = current - CMD_BACKUP + 1; cmdNum <= current; cmdNum++){
		// get the command
		trap_GetUserCmd(cmdNum, &cg_pmove.cmd);

		if(cg_pmove.pmove_fixed)
			PM_UpdateViewAngles(cg_pmove.ps, &cg_pmove.cmd);

		// don't do anything if the time is before the snapshot player time
		if(cg_pmove.cmd.serverTime <= cg.pps.commandTime)
			continue;

		// don't do anything if the command was from a previous map_restart
		if(cg_pmove.cmd.serverTime > latestCmd.serverTime)
			continue;

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if(cg.pps.commandTime == oldPlayerState.commandTime){
			vec3_t delta;
			float len;

			if(cg.teleportthisframe){
				// a teleport will not cause an error decay
				vecclear(cg.predictederr);
				if(cg_showmiss.integer)
					cgprintf("PredictionTeleport\n");
				cg.teleportthisframe = qfalse;
			}else{
				vec3_t adjusted, new_angles;
				adjustposformover(cg.pps.origin,
							  cg.pps.groundEntityNum, cg.phystime, cg.oldtime, adjusted, cg.pps.viewangles, new_angles);

				if(cg_showmiss.integer)
					if(!veccmp(oldPlayerState.origin, adjusted))
						cgprintf("prediction error\n");
				vecsub(oldPlayerState.origin, adjusted, delta);
				len = veclen(delta);
				if(len > 0.1){
					if(cg_showmiss.integer)
						cgprintf("Prediction miss: %f\n", len);
					if(cg_errorDecay.integer){
						int t;
						float f;

						t = cg.time - cg.predictederrtime;
						f = (cg_errorDecay.value - t) / cg_errorDecay.value;
						if(f < 0)
							f = 0;
						if(f > 0 && cg_showmiss.integer)
							cgprintf("Double prediction decay: %f\n", f);
						vecmul(cg.predictederr, f, cg.predictederr);
					}else
						vecclear(cg.predictederr);
					vecadd(delta, cg.predictederr, cg.predictederr);
					cg.predictederrtime = cg.oldtime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntlethit = qfalse;

		if(cg_pmove.pmove_fixed)
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;

		Pmove(&cg_pmove);

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//chkpredictableevents(&cg.pps);
	}

	if(cg_showmiss.integer > 1)
		cgprintf("[%i : %i] ", cg_pmove.cmd.serverTime, cg.time);

	if(!moved){
		if(cg_showmiss.integer)
			cgprintf("not moved\n");
		return;
	}

	// adjust for the movement of the groundentity
	adjustposformover(cg.pps.origin,
				  cg.pps.groundEntityNum,
				  cg.phystime, cg.time, cg.pps.origin, cg.pps.viewangles, cg.pps.viewangles);

	if(cg_showmiss.integer)
		if(cg.pps.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS)
			cgprintf("WARNING: dropped event\n");

	// fire events and other transition triggered things
	pstransition(&cg.pps, &oldPlayerState);

	if(cg_showmiss.integer)
		if(cg.eventSequence > cg.pps.eventSequence){
			cgprintf("WARNING: double event\n");
			cg.eventSequence = cg.pps.eventSequence;
		}
}
