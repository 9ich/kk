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
// bg_local.h -- local definitions for the bg (both games) files

#define STEPSIZE	18

#define TIMER_GESTURE	(34*66+50)

#define OVERCLIP	1.2f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t		forward, right, up;
	float		frametime;

	int		msec;

	qboolean	walking;
	qboolean	groundplane;
	trace_t		groundtrace;

	float		impactspeed;

	vec3_t		prevorigin;
	vec3_t		prevvelocity;
	int		prevwaterlevel;
} pml_t;

extern pmove_t *pm;
extern pml_t pml;

// movement parameters
extern float pm_stopspeed;
extern float pm_duckScale;
extern float pm_swimScale;

extern float pm_accelerate;
extern float pm_airaccelerate;
extern float pm_wateraccelerate;
extern float pm_flyaccelerate;

extern float pm_friction;
extern float pm_waterfriction;
extern float pm_flightfriction;

extern int c_pmove;

void		pmclipvel(vec3_t in, vec3_t normal, vec3_t out, float overbounce);
void		pmaddtouchent(int entityNum);
void		pmaddevent(int newEvent);

qboolean	pmslidemove(qboolean gravity);
void		pmstepslidemove(qboolean gravity);
