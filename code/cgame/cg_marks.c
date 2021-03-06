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
// cg_marks.c -- wall marks

#include "cg_local.h"

/*
===================================================================

MARK POLYS

===================================================================
*/

markPoly_t cg_activeMarkPolys;	// double linked list
markPoly_t *cg_freeMarkPolys;	// single linked list
markPoly_t cg_markPolys[MAX_MARK_POLYS];
static int markTotal;

/*
This is called at startup and for tournement restarts
*/
void
initmarkpolys(void)
{
	int i;

	memset(cg_markPolys, 0, sizeof(cg_markPolys));

	cg_activeMarkPolys.next = &cg_activeMarkPolys;
	cg_activeMarkPolys.prev = &cg_activeMarkPolys;
	cg_freeMarkPolys = cg_markPolys;
	for(i = 0; i < MAX_MARK_POLYS - 1; i++)
		cg_markPolys[i].next = &cg_markPolys[i+1];
}

void
freemarkpoly(markPoly_t *le)
{
	if(!le->prev || !le->next)
		cgerrorf("CG_FreeLocalEntity: not active");

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeMarkPolys;
	cg_freeMarkPolys = le;
}

/*
Will allways succeed, even if it requires freeing an old active mark
*/
markPoly_t*
allocmark(void)
{
	markPoly_t *le;
	int time;

	if(!cg_freeMarkPolys){
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		time = cg_activeMarkPolys.prev->time;
		while(cg_activeMarkPolys.prev && time == cg_activeMarkPolys.prev->time)
			freemarkpoly(cg_activeMarkPolys.prev);
	}

	le = cg_freeMarkPolys;
	cg_freeMarkPolys = cg_freeMarkPolys->next;

	memset(le, 0, sizeof(*le));

	// link into the active list
	le->next = cg_activeMarkPolys.next;
	le->prev = &cg_activeMarkPolys;
	cg_activeMarkPolys.next->prev = le;
	cg_activeMarkPolys.next = le;
	return le;
}

/*
origin should be a point within a unit of the plane
dir should be the plane normal

temporary marks will not be stored or randomly oriented, but immediately
passed to the renderer.
*/
#define MAX_MARK_FRAGMENTS	128
#define MAX_MARK_POINTS		384

void
impactmark(qhandle_t shader, const vec3_t origin, const vec3_t dir,
	      float orientation, float red, float green, float blue, float alpha,
	      qboolean alphafade, float radius, qboolean temporary)
{
	vec3_t axis[3];
	float texCoordScale;
	vec3_t originalPoints[4];
	byte colors[4];
	int i, j;
	int numFragments;
	markFragment_t markFragments[MAX_MARK_FRAGMENTS], *mf;
	vec3_t markPoints[MAX_MARK_POINTS];
	vec3_t projection;

	if(!cg_addMarks.integer)
		return;

	if(radius <= 0)
		cgerrorf("impactmark called with <= 0 radius");

	//if ( markTotal >= MAX_MARK_POLYS ) {
	//	return;
	//}

	// create the texture axis
	VectorNormalize2(dir, axis[0]);
	vecperp(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], orientation);
	veccross(axis[0], axis[2], axis[1]);

	texCoordScale = 0.5 * 1.0 / radius;

	// create the full polygon
	for(i = 0; i < 3; i++){
		originalPoints[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	// get the fragments
	vecmul(dir, -20, projection);
	numFragments = trap_CM_MarkFragments(4, (void*)originalPoints,
					     projection, MAX_MARK_POINTS, markPoints[0],
					     MAX_MARK_FRAGMENTS, markFragments);

	colors[0] = red * 255;
	colors[1] = green * 255;
	colors[2] = blue * 255;
	colors[3] = alpha * 255;

	for(i = 0, mf = markFragments; i < numFragments; i++, mf++){
		polyVert_t *v;
		polyVert_t verts[MAX_VERTS_ON_POLY];
		markPoly_t *mark;

		// we have an upper limit on the complexity of polygons
		// that we store persistantly
		if(mf->numPoints > MAX_VERTS_ON_POLY)
			mf->numPoints = MAX_VERTS_ON_POLY;
		for(j = 0, v = verts; j < mf->numPoints; j++, v++){
			vec3_t delta;

			veccpy(markPoints[mf->firstPoint + j], v->xyz);

			vecsub(v->xyz, origin, delta);
			v->st[0] = 0.5 + vecdot(delta, axis[1]) * texCoordScale;
			v->st[1] = 0.5 + vecdot(delta, axis[2]) * texCoordScale;
			*(int*)v->modulate = *(int*)colors;
		}

		// if it is a temporary (shadow) mark, add it immediately and forget about it
		if(temporary){
			trap_R_AddPolyToScene(shader, mf->numPoints, verts);
			continue;
		}

		// otherwise save it persistantly
		mark = allocmark();
		mark->time = cg.time;
		mark->alphafade = alphafade;
		mark->shader = shader;
		mark->poly.numVerts = mf->numPoints;
		mark->color[0] = red;
		mark->color[1] = green;
		mark->color[2] = blue;
		mark->color[3] = alpha;
		memcpy(mark->verts, verts, mf->numPoints * sizeof(verts[0]));
		markTotal++;
	}
}

#define MARK_TOTAL_TIME 10000
#define MARK_FADE_TIME	1000

void
addmarks(void)
{
	int j;
	markPoly_t *mp, *next;
	int t;
	int fade;

	if(!cg_addMarks.integer)
		return;

	mp = cg_activeMarkPolys.next;
	for(; mp != &cg_activeMarkPolys; mp = next){
		// grab next now, so if the local entity is freed we
		// still have it
		next = mp->next;

		// see if it is time to completely remove it
		if(cg.time > mp->time + MARK_TOTAL_TIME){
			freemarkpoly(mp);
			continue;
		}

		// fade out the energy bursts
		if(mp->shader == cgs.media.energyMarkShader){
			fade = 450 - 450 * ((cg.time - mp->time) / 3000.0);
			if(fade < 255){
				if(fade < 0)
					fade = 0;
				if(mp->verts[0].modulate[0] != 0)
					for(j = 0; j < mp->poly.numVerts; j++){
						mp->verts[j].modulate[0] = mp->color[0] * fade;
						mp->verts[j].modulate[1] = mp->color[1] * fade;
						mp->verts[j].modulate[2] = mp->color[2] * fade;
					}
			}
		}

		// fade all marks out with time
		t = mp->time + MARK_TOTAL_TIME - cg.time;
		if(t < MARK_FADE_TIME){
			fade = 255 * t / MARK_FADE_TIME;
			if(mp->alphafade)
				for(j = 0; j < mp->poly.numVerts; j++)
					mp->verts[j].modulate[3] = fade;
			else
				for(j = 0; j < mp->poly.numVerts; j++){
					mp->verts[j].modulate[0] = mp->color[0] * fade;
					mp->verts[j].modulate[1] = mp->color[1] * fade;
					mp->verts[j].modulate[2] = mp->color[2] * fade;
				}
		}

		trap_R_AddPolyToScene(mp->shader, mp->poly.numVerts, mp->verts);
	}
}
