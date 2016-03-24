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
// bg_slidemove.c -- part of bg_pmove functionality

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundplane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
pmslidemode

Returns qtrue if the velocity was clipped in some way
==================
*/
#define MAX_CLIP_PLANES 5
qboolean
pmslidemode(qboolean gravity)
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity;
	vec3_t clipVelocity;
	int i, j, k;
	trace_t trace;
	vec3_t end;
	float time_left;
	float into;
	vec3_t endVelocity;
	vec3_t endClipVelocity;

	numbumps = 4;

	veccpy(pm->ps->velocity, primal_velocity);

	if(gravity){
		veccpy(pm->ps->velocity, endVelocity);
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = (pm->ps->velocity[2] + endVelocity[2]) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if(pml.groundplane)
			// slide along the ground plane
			pmclipvel(pm->ps->velocity, pml.groundtrace.plane.normal,
					pm->ps->velocity, OVERCLIP);
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if(pml.groundplane){
		numplanes = 1;
		veccpy(pml.groundtrace.plane.normal, planes[0]);
	}else
		numplanes = 0;

	// never turn against original velocity
	VectorNormalize2(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for(bumpcount = 0; bumpcount < numbumps; bumpcount++){
		// calculate position we are trying to move to
		vecmad(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if(trace.allsolid){
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if(trace.fraction > 0)
			// actually covered some distance
			veccpy(trace.endpos, pm->ps->origin);

		if(trace.fraction == 1)
			break;	// moved the entire distance

		// save entity for contact
		pmaddtouchent(trace.entityNum);

		time_left -= time_left * trace.fraction;

		if(numplanes >= MAX_CLIP_PLANES){
			// this shouldn't really happen
			vecclear(pm->ps->velocity);
			return qtrue;
		}

		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		for(i = 0; i < numplanes; i++)
			if(vecdot(trace.plane.normal, planes[i]) > 0.99){
				vecadd(trace.plane.normal, pm->ps->velocity, pm->ps->velocity);
				break;
			}
		if(i < numplanes)
			continue;
		veccpy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify velocity so it parallels all of the clip planes

		// find a plane that it enters
		for(i = 0; i < numplanes; i++){
			into = vecdot(pm->ps->velocity, planes[i]);
			if(into >= 0.1)
				continue;	// move doesn't interact with the plane

			// see how hard we are hitting things
			if(-into > pml.impactspeed)
				pml.impactspeed = -into;

			// slide along the plane
			pmclipvel(pm->ps->velocity, planes[i], clipVelocity, OVERCLIP);

			// slide along the plane
			pmclipvel(endVelocity, planes[i], endClipVelocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for(j = 0; j < numplanes; j++){
				if(j == i)
					continue;
				if(vecdot(clipVelocity, planes[j]) >= 0.1)
					continue;	// move doesn't interact with the plane

				// try clipping the move to the plane
				pmclipvel(clipVelocity, planes[j], clipVelocity, OVERCLIP);
				pmclipvel(endClipVelocity, planes[j], endClipVelocity, OVERCLIP);

				// see if it goes back into the first clip plane
				if(vecdot(clipVelocity, planes[i]) >= 0)
					continue;

				// slide the original velocity along the crease
				veccross(planes[i], planes[j], dir);
				vecnorm(dir);
				d = vecdot(dir, pm->ps->velocity);
				vecmul(dir, d, clipVelocity);

				veccross(planes[i], planes[j], dir);
				vecnorm(dir);
				d = vecdot(dir, endVelocity);
				vecmul(dir, d, endClipVelocity);

				// see if there is a third plane the the new move enters
				for(k = 0; k < numplanes; k++){
					if(k == i || k == j)
						continue;
					if(vecdot(clipVelocity, planes[k]) >= 0.1)
						continue;	// move doesn't interact with the plane

					// stop dead at a tripple plane interaction
					vecclear(pm->ps->velocity);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			veccpy(clipVelocity, pm->ps->velocity);
			veccpy(endClipVelocity, endVelocity);
			break;
		}
	}

	if(gravity)
		veccpy(endVelocity, pm->ps->velocity);

	// don't change velocity if in a timer (FIXME: is this correct?)
	if(pm->ps->pm_time)
		veccpy(primal_velocity, pm->ps->velocity);

	return bumpcount != 0;
}

/*
==================
pmstepslidemove

==================
*/
void
pmstepslidemove(qboolean gravity)
{
	vec3_t start_o, start_v;
//	vec3_t		down_o, down_v;
	trace_t trace;
//	float		down_dist, up_dist;
//	vec3_t		delta, delta2;
	vec3_t up, down;
	float stepSize;

	veccpy(pm->ps->origin, start_o);
	veccpy(pm->ps->velocity, start_v);

	if(pmslidemode(gravity) == 0)
		return;	// we got exactly where we wanted to go first try

	veccpy(start_o, down);
	down[2] -= STEPSIZE;
	pm->trace(&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	vecset(up, 0, 0, 1);
	// never step up when you still have up velocity
	if(pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
				       vecdot(trace.plane.normal, up) < 0.7))
		return;

	//veccpy (pm->ps->origin, down_o);
	//veccpy (pm->ps->velocity, down_v);

	veccpy(start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	pm->trace(&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if(trace.allsolid){
		if(pm->debuglevel)
			Com_Printf("%i:bend can't step\n", c_pmove);
		return;	// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	veccpy(trace.endpos, pm->ps->origin);
	veccpy(start_v, pm->ps->velocity);

	pmslidemode(gravity);

	// push down the final amount
	veccpy(pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if(!trace.allsolid)
		veccpy(trace.endpos, pm->ps->origin);
	if(trace.fraction < 1.0)
		pmclipvel(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);

#if 0
	// if the down trace can trace back to the original position directly, don't step
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, start_o, pm->ps->clientNum, pm->tracemask);
	if(trace.fraction == 1.0){
		// use the original move
		veccpy(down_o, pm->ps->origin);
		veccpy(down_v, pm->ps->velocity);
		if(pm->debuglevel)
			Com_Printf("%i:bend\n", c_pmove);
	}else
#endif
	{
		// use the step move
		float delta;

		delta = pm->ps->origin[2] - start_o[2];
		if(pm->debuglevel)
			Com_Printf("%i:stepped\n", c_pmove);
	}
}
