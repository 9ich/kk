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
// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

/*
==================
CG_ResetEntity
==================
*/
static void
CG_ResetEntity(centity_t *cent)
{
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if(cent->snapshottime < cg.time - EVENT_VALID_MSEC)
		cent->prevevent = 0;

	cent->trailtime = cg.snap->serverTime;

	veccpy(cent->currstate.origin, cent->lerporigin);
	veccpy(cent->currstate.angles, cent->lerpangles);
	if(cent->currstate.eType == ET_PLAYER)
		resetplayerent(cent);
}

/*
===============
CG_TransitionEntity

cent->nextstate is moved to cent->currstate and events are fired
===============
*/
static void
CG_TransitionEntity(centity_t *cent)
{
	cent->currstate = cent->nextstate;
	cent->currvalid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if(!cent->interpolate)
		CG_ResetEntity(cent);

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	chkevents(cent);
}

/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
void
CG_SetInitialSnapshot(snapshot_t *snap)
{
	int i;
	centity_t *cent;
	entityState_t *state;

	cg.snap = snap;

	playerstate2entstate(&snap->ps, &cg_entities[snap->ps.clientNum].currstate, qfalse);

	// sort out solid entities
	mksolidlist();

	execnewsrvcmds(snap->serverCommandSequence);

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	respawn();

	for(i = 0; i < cg.snap->numEntities; i++){
		state = &cg.snap->entities[i];
		cent = &cg_entities[state->number];

		memcpy(&cent->currstate, state, sizeof(entityState_t));
		//cent->currstate = *state;
		cent->interpolate = qfalse;
		cent->currvalid = qtrue;

		CG_ResetEntity(cent);

		// check for events
		chkevents(cent);
	}
}

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextsnap has passed
===================
*/
static void
CG_TransitionSnapshot(void)
{
	centity_t *cent;
	snapshot_t *oldframe;
	int i;

	if(!cg.snap)
		cgerrorf("CG_TransitionSnapshot: nil cg.snap");
	if(!cg.nextsnap)
		cgerrorf("CG_TransitionSnapshot: nil cg.nextsnap");

	// execute any server string commands before transitioning entities
	execnewsrvcmds(cg.nextsnap->serverCommandSequence);

	// if we had a map_restart, set everthing with initial
	if(!cg.snap)
		return;

	// clear the currvalid flag for all entities in the existing snapshot
	for(i = 0; i < cg.snap->numEntities; i++){
		cent = &cg_entities[cg.snap->entities[i].number];
		cent->currvalid = qfalse;
	}

	// move nextsnap to snap and do the transitions
	oldframe = cg.snap;
	cg.snap = cg.nextsnap;

	playerstate2entstate(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum].currstate, qfalse);
	cg_entities[cg.snap->ps.clientNum].interpolate = qfalse;

	for(i = 0; i < cg.snap->numEntities; i++){
		cent = &cg_entities[cg.snap->entities[i].number];
		CG_TransitionEntity(cent);

		// remember time of snapshot this entity was last updated in
		cent->snapshottime = cg.snap->serverTime;
	}

	cg.nextsnap = nil;

	// check for playerstate transition events
	if(oldframe){
		playerState_t *ops, *ps;

		ops = &oldframe->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if((ps->eFlags ^ ops->eFlags) & EF_TELEPORT_BIT)
			cg.teleportthisframe = qtrue;	// will be cleared by prediction code

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if(cg.demoplayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
		   || cg_nopredict.integer || cg_synchronousClients.integer)
			pstransition(ps, ops);
	}
}

/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void
CG_SetNextSnap(snapshot_t *snap)
{
	int num;
	entityState_t *es;
	centity_t *cent;

	cg.nextsnap = snap;

	playerstate2entstate(&snap->ps, &cg_entities[snap->ps.clientNum].nextstate, qfalse);
	cg_entities[cg.snap->ps.clientNum].interpolate = qtrue;

	// check for extrapolation errors
	for(num = 0; num < snap->numEntities; num++){
		es = &snap->entities[num];
		cent = &cg_entities[es->number];

		memcpy(&cent->nextstate, es, sizeof(entityState_t));
		//cent->nextstate = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if(!cent->currvalid || ((cent->currstate.eFlags ^ es->eFlags) & EF_TELEPORT_BIT))
			cent->interpolate = qfalse;
		else
			cent->interpolate = qtrue;
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if(cg.snap && ((snap->ps.eFlags ^ cg.snap->ps.eFlags) & EF_TELEPORT_BIT))
		cg.teleportnextframe = qtrue;
	else
		cg.teleportnextframe = qfalse;

	// if changing follow mode, don't interpolate
	if(cg.nextsnap->ps.clientNum != cg.snap->ps.clientNum)
		cg.teleportnextframe = qtrue;

	// if changing server restarts, don't interpolate
	if((cg.nextsnap->snapFlags ^ cg.snap->snapFlags) & SNAPFLAG_SERVERCOUNT)
		cg.teleportnextframe = qtrue;

	// sort out solid entities
	mksolidlist();
}

/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.nprocessedsnaps multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t *
CG_ReadNextSnapshot(void)
{
	qboolean r;
	snapshot_t *dest;

	if(cg.latestsnapnum > cgs.nprocessedsnaps + 1000)
		cgprintf("WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
			  cg.latestsnapnum, cgs.nprocessedsnaps);

	while(cgs.nprocessedsnaps < cg.latestsnapnum){
		// decide which of the two slots to load it into
		if(cg.snap == &cg.activesnaps[0])
			dest = &cg.activesnaps[1];
		else
			dest = &cg.activesnaps[0];

		// try to read the snapshot from the client system
		cgs.nprocessedsnaps++;
		r = trap_GetSnapshot(cgs.nprocessedsnaps, dest);

		// FIXME: why would trap_GetSnapshot return a snapshot with the same server time
		if(cg.snap && r && dest->serverTime == cg.snap->serverTime){
			//continue;
		}

		// if it succeeded, return
		if(r){
			lagometersnapinfo(dest);
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		lagometersnapinfo(nil);

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return nil;
}

/*
============
processsnaps

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextsnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void
processsnaps(void)
{
	snapshot_t *snap;
	int n;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber(&n, &cg.latestsnapttime);
	if(n != cg.latestsnapnum){
		if(n < cg.latestsnapnum)
			// this should never happen
			cgerrorf("processsnaps: n < cg.latestsnapnum");
		cg.latestsnapnum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while(!cg.snap){
		snap = CG_ReadNextSnapshot();
		if(!snap)
			// we can't continue until we get a snapshot
			return;

		// set our weapon selection to what
		// the playerstate is currently using
		if(!(snap->snapFlags & SNAPFLAG_NOT_ACTIVE))
			CG_SetInitialSnapshot(snap);
	}

	// loop until we either have a valid nextsnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if(!cg.nextsnap){
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if(!snap)
				break;

			CG_SetNextSnap(snap);

			// if time went backwards, we have a level restart
			if(cg.nextsnap->serverTime < cg.snap->serverTime)
				cgerrorf("processsnaps: Server time went backwards");
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if(cg.time >= cg.snap->serverTime && cg.time < cg.nextsnap->serverTime)
			break;

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while(1);

	// assert our valid conditions upon exiting
	if(cg.snap == nil)
		cgerrorf("processsnaps: cg.snap == nil");
	if(cg.time < cg.snap->serverTime)
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	if(cg.nextsnap != nil && cg.nextsnap->serverTime <= cg.time)
		cgerrorf("processsnaps: cg.nextsnap->serverTime <= cg.time");

}
