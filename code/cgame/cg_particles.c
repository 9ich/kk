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
// Rafael particles
// cg_particles.c

#include "cg_local.h"

//#define WOLF_PARTICLES

#define BLOODRED	2
#define EMISIVEFADE	3
#define GREY75		4

typedef struct particle_s
{
	struct particle_s	*next;

	float			time;
	float			endtime;

	vec3_t			org;
	vec3_t			vel;
	vec3_t			accel;
	int			color;
	float			colorvel;
	float			alpha;
	float			alphavel;
	int			type;
	qhandle_t		pshader;

	float			height;
	float			width;

	float			endheight;
	float			endwidth;

	float			start;
	float			end;

	float			startfade;
	qboolean		rotate;
	int			snum;

	qboolean		link;

	// Ridah
	int			shaderAnim;
	int			roll;

	int			accumroll;
} cparticle_t;

typedef enum
{
	P_NONE,
	P_WEATHER,
	P_FLAT,
	P_SMOKE,
	P_ROTATE,
	P_WEATHER_TURBULENT,
	P_ANIM,	// Ridah
	P_BAT,
	P_BLEED,
	P_FLAT_SCALEUP,
	P_FLAT_SCALEUP_FADE,
	P_WEATHER_FLURRY,
	P_SMOKE_IMPACT,
	P_BUBBLE,
	P_BUBBLE_TURBULENT,
	P_SPRITE
} particle_type_t;

#define MAX_SHADER_ANIMS	32
#define MAX_SHADER_ANIM_FRAMES	64

#ifndef WOLF_PARTICLES
static char *shaderAnimNames[MAX_SHADER_ANIMS] = {
	"explode1",
	nil
};
static qhandle_t shaderAnims[MAX_SHADER_ANIMS][MAX_SHADER_ANIM_FRAMES];
static int shaderAnimCounts[MAX_SHADER_ANIMS] = {
	43
};
static float shaderAnimSTRatio[MAX_SHADER_ANIMS] = {
	1.0f
};
static int numShaderAnims;
// done.
#else
static char *shaderAnimNames[MAX_SHADER_ANIMS] = {
	"explode1",
	"blacksmokeanim",
	nil
};
static qhandle_t shaderAnims[MAX_SHADER_ANIMS][MAX_SHADER_ANIM_FRAMES];
static int shaderAnimCounts[MAX_SHADER_ANIMS] = {
	23,
	25
};
static float shaderAnimSTRatio[MAX_SHADER_ANIMS] = {
	//1.405f,
	1.0f,
	1.0f
};
#endif

#define         PARTICLE_GRAVITY	1

#ifdef WOLF_PARTICLES
#define         MAX_PARTICLES		1024 * 8
#else
#define         MAX_PARTICLES		1024
#endif

cparticle_t *active_particles, *free_particles;
cparticle_t particles[MAX_PARTICLES];
int cl_numparticles = MAX_PARTICLES;

qboolean initparticles = qfalse;
vec3_t vforward, vright, vup;
vec3_t rforward, rright, rup;

float oldtime;

void
clearparticles(void)
{
	int i;

	memset(particles, 0, sizeof(particles));

	free_particles = &particles[0];
	active_particles = nil;

	for(i = 0; i<cl_numparticles; i++){
		particles[i].next = &particles[i+1];
		particles[i].type = 0;
	}
	particles[cl_numparticles-1].next = nil;

	oldtime = cg.time;

	// Ridah, init the shaderAnims
	for(i = 0; shaderAnimNames[i]; i++){
		int j;

		for(j = 0; j<shaderAnimCounts[i]; j++)
			shaderAnims[i][j] = trap_R_RegisterShader(va("%s%i", shaderAnimNames[i], j+1));
	}
	numShaderAnims = i;
	// done.

	initparticles = qtrue;
}

void
CG_AddParticleToScene(cparticle_t *p, vec3_t org, float alpha)
{
	vec3_t point;
	polyVert_t verts[4];
	float width;
	float height;
	float time, time2;
	float ratio;
	float invratio;
	vec3_t color;
	polyVert_t TRIverts[3];
	vec3_t rright2, rup2;

	if(p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY
	   || p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){	// create a front facing polygon
		if(p->type != P_WEATHER_FLURRY){
			if(p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){
				if(org[2] > p->end){
					p->time = cg.time;
					veccpy(org, p->org);	// Ridah, fixes rare snow flakes that flicker on the ground

					p->org[2] = (p->start + crandom() * 4);

					if(p->type == P_BUBBLE_TURBULENT){
						p->vel[0] = crandom() * 4;
						p->vel[1] = crandom() * 4;
					}
				}
			}else    if(org[2] < p->end){
				p->time = cg.time;
				veccpy(org, p->org);		// Ridah, fixes rare snow flakes that flicker on the ground

				while(p->org[2] < p->end)
					p->org[2] += (p->start - p->end);


				if(p->type == P_WEATHER_TURBULENT){
					p->vel[0] = crandom() * 16;
					p->vel[1] = crandom() * 16;
				}
			}

			// Rafael snow pvs check
			if(!p->link)
				return;

			p->alpha = 1;
		}

		if(p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT){
			vecmad(org, -p->height, vup, point);
			vecmad(point, -p->width, vright, point);
			veccpy(point, verts[0].xyz);
			verts[0].st[0] = 0;
			verts[0].st[1] = 0;
			verts[0].modulate[0] = 255;
			verts[0].modulate[1] = 255;
			verts[0].modulate[2] = 255;
			verts[0].modulate[3] = 255 * p->alpha;

			vecmad(org, -p->height, vup, point);
			vecmad(point, p->width, vright, point);
			veccpy(point, verts[1].xyz);
			verts[1].st[0] = 0;
			verts[1].st[1] = 1;
			verts[1].modulate[0] = 255;
			verts[1].modulate[1] = 255;
			verts[1].modulate[2] = 255;
			verts[1].modulate[3] = 255 * p->alpha;

			vecmad(org, p->height, vup, point);
			vecmad(point, p->width, vright, point);
			veccpy(point, verts[2].xyz);
			verts[2].st[0] = 1;
			verts[2].st[1] = 1;
			verts[2].modulate[0] = 255;
			verts[2].modulate[1] = 255;
			verts[2].modulate[2] = 255;
			verts[2].modulate[3] = 255 * p->alpha;

			vecmad(org, p->height, vup, point);
			vecmad(point, -p->width, vright, point);
			veccpy(point, verts[3].xyz);
			verts[3].st[0] = 1;
			verts[3].st[1] = 0;
			verts[3].modulate[0] = 255;
			verts[3].modulate[1] = 255;
			verts[3].modulate[2] = 255;
			verts[3].modulate[3] = 255 * p->alpha;
		}else{
			vecmad(org, -p->height, vup, point);
			vecmad(point, -p->width, vright, point);
			veccpy(point, TRIverts[0].xyz);
			TRIverts[0].st[0] = 1;
			TRIverts[0].st[1] = 0;
			TRIverts[0].modulate[0] = 255;
			TRIverts[0].modulate[1] = 255;
			TRIverts[0].modulate[2] = 255;
			TRIverts[0].modulate[3] = 255 * p->alpha;

			vecmad(org, p->height, vup, point);
			vecmad(point, -p->width, vright, point);
			veccpy(point, TRIverts[1].xyz);
			TRIverts[1].st[0] = 0;
			TRIverts[1].st[1] = 0;
			TRIverts[1].modulate[0] = 255;
			TRIverts[1].modulate[1] = 255;
			TRIverts[1].modulate[2] = 255;
			TRIverts[1].modulate[3] = 255 * p->alpha;

			vecmad(org, p->height, vup, point);
			vecmad(point, p->width, vright, point);
			veccpy(point, TRIverts[2].xyz);
			TRIverts[2].st[0] = 0;
			TRIverts[2].st[1] = 1;
			TRIverts[2].modulate[0] = 255;
			TRIverts[2].modulate[1] = 255;
			TRIverts[2].modulate[2] = 255;
			TRIverts[2].modulate[3] = 255 * p->alpha;
		}
	}else if(p->type == P_SPRITE){
		vec3_t rr, ru;
		vec3_t rotate_ang;

#ifdef WOLF_PARTICLES
		vecset(color, 1.0, 1.0, 1.0);
#else
		vecset(color, 1.0, 1.0, 0.5);
#endif
		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		width = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			anglevecs(rotate_ang, nil, rr, ru);
		}

		if(p->roll){
			vecmad(org, -height, ru, point);
			vecmad(point, -width, rr, point);
		}else{
			vecmad(org, -height, vup, point);
			vecmad(point, -width, vright, point);
		}
		veccpy(point, verts[0].xyz);
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		if(p->roll)
			vecmad(point, 2*height, ru, point);
		else
			vecmad(point, 2*height, vup, point);
		veccpy(point, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		if(p->roll)
			vecmad(point, 2*width, rr, point);
		else
			vecmad(point, 2*width, vright, point);
		veccpy(point, verts[2].xyz);
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		if(p->roll)
			vecmad(point, -2*height, ru, point);
		else
			vecmad(point, -2*height, vup, point);
		veccpy(point, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}else if(p->type == P_SMOKE || p->type == P_SMOKE_IMPACT){	// create a front rotating facing polygon
		if(p->type == P_SMOKE_IMPACT && vecdist(cg.snap->ps.origin, org) > 1024)
			return;

		if(p->color == BLOODRED)
			vecset(color, 0.22f, 0.0f, 0.0f);
		else if(p->color == GREY75){
			float len;
			float greyit;
			float val;
			len = vecdist(cg.snap->ps.origin, org);
			if(!len)
				len = 1;

			val = 4096/len;
			greyit = 0.25 * val;
			if(greyit > 0.5)
				greyit = 0.5;

			vecset(color, greyit, greyit, greyit);
		}else
			vecset(color, 1.0, 1.0, 1.0);

		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		if(cg.time > p->startfade){
			invratio = 1 - ((cg.time - p->startfade) / (p->endtime - p->startfade));

			if(p->color == EMISIVEFADE){
				float fval;
				fval = (invratio * invratio);
				if(fval < 0)
					fval = 0;
				vecset(color, fval, fval, fval);
			}
			invratio *= p->alpha;
		}else
			invratio = 1 * p->alpha;

		if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
			invratio = 1;

		if(invratio > 1)
			invratio = 1;

		width = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		if(p->type != P_SMOKE_IMPACT){
			vec3_t temp;

			vectoangles(rforward, temp);
			p->accumroll += p->roll;
			temp[ROLL] += p->accumroll * 0.1;
			anglevecs(temp, nil, rright2, rup2);
		}else{
			veccpy(rright, rright2);
			veccpy(rup, rup2);
		}

		if(p->rotate){
			vecmad(org, -height, rup2, point);
			vecmad(point, -width, rright2, point);
		}else{
			vecmad(org, -p->height, vup, point);
			vecmad(point, -p->width, vright, point);
		}
		veccpy(point, verts[0].xyz);
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255 * invratio;

		if(p->rotate){
			vecmad(org, -height, rup2, point);
			vecmad(point, width, rright2, point);
		}else{
			vecmad(org, -p->height, vup, point);
			vecmad(point, p->width, vright, point);
		}
		veccpy(point, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255 * invratio;

		if(p->rotate){
			vecmad(org, height, rup2, point);
			vecmad(point, width, rright2, point);
		}else{
			vecmad(org, p->height, vup, point);
			vecmad(point, p->width, vright, point);
		}
		veccpy(point, verts[2].xyz);
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255 * invratio;

		if(p->rotate){
			vecmad(org, height, rup2, point);
			vecmad(point, -width, rright2, point);
		}else{
			vecmad(org, p->height, vup, point);
			vecmad(point, -p->width, vright, point);
		}
		veccpy(point, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255  * invratio;
	}else if(p->type == P_BLEED){
		vec3_t rr, ru;
		vec3_t rotate_ang;
		float alpha;

		alpha = p->alpha;

		if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
			alpha = 1;

		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			anglevecs(rotate_ang, nil, rr, ru);
		}else{
			veccpy(vup, ru);
			veccpy(vright, rr);
		}

		vecmad(org, -p->height, ru, point);
		vecmad(point, -p->width, rr, point);
		veccpy(point, verts[0].xyz);
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 111;
		verts[0].modulate[1] = 19;
		verts[0].modulate[2] = 9;
		verts[0].modulate[3] = 255 * alpha;

		vecmad(org, -p->height, ru, point);
		vecmad(point, p->width, rr, point);
		veccpy(point, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 111;
		verts[1].modulate[1] = 19;
		verts[1].modulate[2] = 9;
		verts[1].modulate[3] = 255 * alpha;

		vecmad(org, p->height, ru, point);
		vecmad(point, p->width, rr, point);
		veccpy(point, verts[2].xyz);
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 111;
		verts[2].modulate[1] = 19;
		verts[2].modulate[2] = 9;
		verts[2].modulate[3] = 255 * alpha;

		vecmad(org, p->height, ru, point);
		vecmad(point, -p->width, rr, point);
		veccpy(point, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 111;
		verts[3].modulate[1] = 19;
		verts[3].modulate[2] = 9;
		verts[3].modulate[3] = 255 * alpha;
	}else if(p->type == P_FLAT_SCALEUP){
		float sinR, cosR;

		if(p->color == BLOODRED)
			vecset(color, 1, 1, 1);
		else
			vecset(color, 0.5, 0.5, 0.5);

		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		width = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		if(width > p->endwidth)
			width = p->endwidth;

		if(height > p->endheight)
			height = p->endheight;

		sinR = height * sin(DEG2RAD(p->roll)) * sqrt(2);
		cosR = width * cos(DEG2RAD(p->roll)) * sqrt(2);

		veccpy(org, verts[0].xyz);
		verts[0].xyz[0] -= sinR;
		verts[0].xyz[1] -= cosR;
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255;

		veccpy(org, verts[1].xyz);
		verts[1].xyz[0] -= cosR;
		verts[1].xyz[1] += sinR;
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255;

		veccpy(org, verts[2].xyz);
		verts[2].xyz[0] += sinR;
		verts[2].xyz[1] += cosR;
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255;

		veccpy(org, verts[3].xyz);
		verts[3].xyz[0] += cosR;
		verts[3].xyz[1] -= sinR;
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255;
	}else if(p->type == P_FLAT){
		veccpy(org, verts[0].xyz);
		verts[0].xyz[0] -= p->height;
		verts[0].xyz[1] -= p->width;
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		veccpy(org, verts[1].xyz);
		verts[1].xyz[0] -= p->height;
		verts[1].xyz[1] += p->width;
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		veccpy(org, verts[2].xyz);
		verts[2].xyz[0] += p->height;
		verts[2].xyz[1] += p->width;
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		veccpy(org, verts[3].xyz);
		verts[3].xyz[0] += p->height;
		verts[3].xyz[1] -= p->width;
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	// Ridah
	else if(p->type == P_ANIM){
		vec3_t rr, ru;
		vec3_t rotate_ang;
		int i, j;

		time = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;
		if(ratio >= 1.0f)
			ratio = 0.9999f;

		width = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		i = p->shaderAnim;
		j = (int)floor(ratio * shaderAnimCounts[p->shaderAnim]);
		p->pshader = shaderAnims[i][j];

		if(p->roll){
			vectoangles(cg.refdef.viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			anglevecs(rotate_ang, nil, rr, ru);
		}

		if(p->roll){
			vecmad(org, -height, ru, point);
			vecmad(point, -width, rr, point);
		}else{
			vecmad(org, -height, vup, point);
			vecmad(point, -width, vright, point);
		}
		veccpy(point, verts[0].xyz);
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		if(p->roll)
			vecmad(point, 2*height, ru, point);
		else
			vecmad(point, 2*height, vup, point);
		veccpy(point, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		if(p->roll)
			vecmad(point, 2*width, rr, point);
		else
			vecmad(point, 2*width, vright, point);
		veccpy(point, verts[2].xyz);
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		if(p->roll)
			vecmad(point, -2*height, ru, point);
		else
			vecmad(point, -2*height, vup, point);
		veccpy(point, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	// done.

	if(!p->pshader)
// (SA) temp commented out for DM
//		cgprintf ("CG_AddParticleToScene type %d p->pshader == ZERO\n", p->type);
		return;

	if(p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY)
		trap_R_AddPolyToScene(p->pshader, 3, TRIverts);
	else
		trap_R_AddPolyToScene(p->pshader, 4, verts);
}

// Ridah, made this static so it doesn't interfere with other files
static float roll = 0.0;

void
addparticles(void)
{
	cparticle_t *p, *next;
	float alpha;
	float time, time2;
	vec3_t org;
	cparticle_t *active, *tail;
	vec3_t rotate_ang;

	if(!initparticles)
		clearparticles();

	veccpy(cg.refdef.viewaxis[0], vforward);
	veccpy(cg.refdef.viewaxis[1], vright);
	veccpy(cg.refdef.viewaxis[2], vup);

	vectoangles(cg.refdef.viewaxis[0], rotate_ang);
	roll += ((cg.time - oldtime) * 0.1);
	rotate_ang[ROLL] += (roll*0.9);
	anglevecs(rotate_ang, rforward, rright, rup);

	oldtime = cg.time;

	active = nil;
	tail = nil;

	for(p = active_particles; p; p = next){
		next = p->next;

		time = (cg.time - p->time)*0.001;

		alpha = p->alpha + time*p->alphavel;
		if(alpha <= 0){	// faded out
			p->next = free_particles;
			free_particles = p;
			p->type = 0;
			p->color = 0;
			p->alpha = 0;
			continue;
		}

		if(p->type == P_SMOKE || p->type == P_ANIM || p->type == P_BLEED || p->type == P_SMOKE_IMPACT)
			if(cg.time > p->endtime){
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;

				continue;
			}


		if(p->type == P_WEATHER_FLURRY)
			if(cg.time > p->endtime){
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;

				continue;
			}


		if(p->type == P_FLAT_SCALEUP_FADE)
			if(cg.time > p->endtime){
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
				continue;
			}


		if((p->type == P_BAT || p->type == P_SPRITE) && p->endtime < 0){
			// temporary sprite
			CG_AddParticleToScene(p, p->org, alpha);
			p->next = free_particles;
			free_particles = p;
			p->type = 0;
			p->color = 0;
			p->alpha = 0;
			continue;
		}

		p->next = nil;
		if(!tail)
			active = tail = p;
		else{
			tail->next = p;
			tail = p;
		}

		if(alpha > 1.0)
			alpha = 1;

		time2 = time*time;

		org[0] = p->org[0] + p->vel[0]*time + p->accel[0]*time2;
		org[1] = p->org[1] + p->vel[1]*time + p->accel[1]*time2;
		org[2] = p->org[2] + p->vel[2]*time + p->accel[2]*time2;

		CG_AddParticleToScene(p, org, alpha);
	}

	active_particles = active;
}

void
particlesnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t *p;

	if(!pshader)
		cgprintf("CG_ParticleSnow pshader == ZERO!\n");

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = 0;
	p->alpha = 0.40f;
	p->alphavel = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	p->height = 1;
	p->width = 1;

	p->vel[2] = -50;

	if(turb){
		p->type = P_WEATHER_TURBULENT;
		p->vel[2] = -50 * 1.3;
	}else
		p->type = P_WEATHER;

	veccpy(origin, p->org);

	p->org[0] = p->org[0] + (crandom() * range);
	p->org[1] = p->org[1] + (crandom() * range);
	p->org[2] = p->org[2] + (crandom() * (p->start - p->end));

	p->vel[0] = p->vel[1] = 0;

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if(turb){
		p->vel[0] = crandom() * 16;
		p->vel[1] = crandom() * 16;
	}

	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

void
particlebubble(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t *p;
	float randsize;

	if(!pshader)
		cgprintf("CG_ParticleSnow pshader == ZERO!\n");

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->color = 0;
	p->alpha = 0.40f;
	p->alphavel = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;

	randsize = 1 + (crandom() * 0.5);

	p->height = randsize;
	p->width = randsize;

	p->vel[2] = 50 + (crandom() * 10);

	if(turb){
		p->type = P_BUBBLE_TURBULENT;
		p->vel[2] = 50 * 1.3;
	}else
		p->type = P_BUBBLE;

	veccpy(origin, p->org);

	p->org[0] = p->org[0] + (crandom() * range);
	p->org[1] = p->org[1] + (crandom() * range);
	p->org[2] = p->org[2] + (crandom() * (p->start - p->end));

	p->vel[0] = p->vel[1] = 0;

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if(turb){
		p->vel[0] = crandom() * 4;
		p->vel[1] = crandom() * 4;
	}

	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

void
particlesmoke(qhandle_t pshader, centity_t *cent)
{
	// using cent->density = enttime
	//		 cent->frame = startfade
	cparticle_t *p;

	if(!pshader)
		cgprintf("CG_ParticleSmoke == ZERO!\n");

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;

	p->endtime = cg.time + cent->currstate.time;
	p->startfade = cg.time + cent->currstate.time2;

	p->color = 0;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->start = cent->currstate.origin[2];
	p->end = cent->currstate.origin2[2];
	p->pshader = pshader;
	p->rotate = qfalse;
	p->height = 8;
	p->width = 8;
	p->endheight = 32;
	p->endwidth = 32;
	p->type = P_SMOKE;

	veccpy(cent->currstate.origin, p->org);

	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[2] = 5;

	if(cent->currstate.frame == 1)	// reverse gravity
		p->vel[2] *= -1;

	p->roll = 8 + (crandom() * 4);
}

void
particlebulletdebris(vec3_t org, vec3_t vel, int duration)
{
	cparticle_t *p;

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;

	p->endtime = cg.time + duration;
	p->startfade = cg.time + duration/2;

	p->color = EMISIVEFADE;
	p->alpha = 1.0;
	p->alphavel = 0;

	p->height = 0.5;
	p->width = 0.5;
	p->endheight = 0.5;
	p->endwidth = 0.5;

	p->pshader = cgs.media.tracerShader;

	p->type = P_SMOKE;

	veccpy(org, p->org);

	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->accel[2] = -60;
	p->vel[2] += -20;
}


void
particleexplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd)
{
	cparticle_t *p;
	int anim;

	if(animStr < (char*)10)
		cgerrorf("CG_ParticleExplosion: animStr is probably an index rather than a string");

	// find the animation string
	for(anim = 0; shaderAnimNames[anim]; anim++)
		if(!Q_stricmp(animStr, shaderAnimNames[anim]))
			break;
	if(!shaderAnimNames[anim]){
		cgerrorf("CG_ParticleExplosion: unknown animation string: %s", animStr);
		return;
	}

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
#ifdef WOLF_PARTICLES
	p->alpha = 1.0;
#else
	p->alpha = 0.5;
#endif
	p->alphavel = 0;

	if(duration < 0){
		duration *= -1;
		p->roll = 0;
	}else
		p->roll = crandom()*179;

	p->shaderAnim = anim;

	p->width = sizeStart;
	p->height = sizeStart*shaderAnimSTRatio[anim];	// for sprites that are stretch in either direction

	p->endheight = sizeEnd;
	p->endwidth = sizeEnd*shaderAnimSTRatio[anim];

	p->endtime = cg.time + duration;

	p->type = P_ANIM;

	veccpy(origin, p->org);
	veccpy(vel, p->vel);
	vecclear(p->accel);
}

// done.

int
newparticlearea(int num)
{
	// const char *str;
	char *str;
	char *token;
	int type;
	vec3_t origin, origin2;
	int i;
	float range = 0;
	int turb;
	int numparticles;
	int snum;

	str = (char*)getconfigstr(num);
	if(!str[0])
		return 0;

	// returns type 128 64 or 32
	token = COM_Parse(&str);
	type = atoi(token);

	if(type == 1)
		range = 128;
	else if(type == 2)
		range = 64;
	else if(type == 3)
		range = 32;
	else if(type == 0)
		range = 256;
	else if(type == 4)
		range = 8;
	else if(type == 5)
		range = 16;
	else if(type == 6)
		range = 32;
	else if(type == 7)
		range = 64;

	for(i = 0; i<3; i++){
		token = COM_Parse(&str);
		origin[i] = atof(token);
	}

	for(i = 0; i<3; i++){
		token = COM_Parse(&str);
		origin2[i] = atof(token);
	}

	token = COM_Parse(&str);
	numparticles = atoi(token);

	token = COM_Parse(&str);
	turb = atoi(token);

	token = COM_Parse(&str);
	snum = atoi(token);

	for(i = 0; i<numparticles; i++){
		if(type >= 4)
			particlebubble(cgs.media.waterBubbleShader, origin, origin2, turb, range, snum);
		else
			particlesnow(cgs.media.waterBubbleShader, origin, origin2, turb, range, snum);
	}

	return 1;
}

void
particleimpactsmokepuff(qhandle_t pshader, vec3_t origin)
{
	cparticle_t *p;

	if(!pshader)
		cgprintf("CG_ParticleImpactSmokePuff pshader == ZERO!\n");

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 0.25;
	p->alphavel = 0;
	p->roll = crandom()*179;

	p->pshader = pshader;

	p->endtime = cg.time + 1000;
	p->startfade = cg.time + 100;

	p->width = rand()%4 + 8;
	p->height = rand()%4 + 8;

	p->endheight = p->height *2;
	p->endwidth = p->width * 2;

	p->endtime = cg.time + 500;

	p->type = P_SMOKE_IMPACT;

	veccpy(origin, p->org);
	vecset(p->vel, 0, 0, 20);
	vecset(p->accel, 0, 0, 20);

	p->rotate = qtrue;
}

void
particlebleed(qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration)
{
	cparticle_t *p;

	if(!pshader)
		cgprintf("CG_Particle_Bleed pshader == ZERO!\n");

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = 0;

	p->pshader = pshader;

	p->endtime = cg.time + duration;

	if(fleshEntityNum)
		p->startfade = cg.time;
	else
		p->startfade = cg.time + 100;

	p->width = 4;
	p->height = 4;

	p->endheight = 4+rand()%3;
	p->endwidth = p->endheight;

	p->type = P_SMOKE;

	veccpy(start, p->org);
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = -20;
	vecclear(p->accel);

	p->rotate = qfalse;

	p->roll = rand()%179;

	p->color = BLOODRED;
	p->alpha = 0.75;
}

#define NORMALSIZE	16
#define LARGESIZE	32

void
particlesparks(vec3_t org, vec3_t vel, int duration, float x, float speed)
{
	cparticle_t *p;
	vec3_t dir;

	if(!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0f;
	p->alphavel = 0;

	if(duration < 0){
		duration *= -1;
		p->roll = 0;
	}else
		p->roll = crandom()*179;

	p->pshader = cgs.media.tracerShader;

	p->width = 1.8f;
	p->height = 1.8f;	// for sprites that are stretch in either direction

	p->endheight = 0.001f;
	p->endwidth = 0.001f;

	p->endtime = cg.time + duration;

	p->type = P_SMOKE;

	veccpy(org, p->org);
	p->org[0] += (crandom() * x);
	p->org[1] += (crandom() * x);
	p->org[2] += (crandom() * x);

	veccpy(vel, p->vel);
	vecset(dir, crandom()*.6f, crandom()*.6f, crandom()*.6f);
	vecadd(p->vel, dir, p->vel);
	vecnorm(p->vel);

	vecmul(p->vel, speed, p->vel);
	vecclear(p->accel);
}

void
particlethrustplume(centity_t *cent, vec3_t origin, vec3_t dir)
{
	float length;
	float dist;
	float crittersize;
	vec3_t angles, forward;
	vec3_t point;
	cparticle_t *p;
	int i;

	dist = 0;

	VectorNegate(dir, dir);
	length = veclen(dir);
	vectoangles(dir, angles);
	anglevecs(angles, forward, nil, nil);

	crittersize = 6;

	if(length)
		dist = length / crittersize;

	if(dist < 1)
		dist = 1;

	veccpy(origin, point);

	for(i = 0; i<dist; i++){
		vecmad(point, crittersize, forward, point);

		if(!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cg.time;
		p->alpha = 2.0;
		p->alphavel = 0;
		p->roll = 0;

		p->pshader = cgs.media.smokePuffShader;

		// RF, stay around for long enough to expand and dissipate naturally
		p->endtime = cg.time + 1500 + (crandom() * 500);

		p->startfade = cg.time;

		p->width = NORMALSIZE;
		p->height = NORMALSIZE;

		// RF, expand while floating
		p->endheight = NORMALSIZE*4.0;
		p->endwidth = NORMALSIZE*4.0;

		p->type = P_SMOKE;

		veccpy(point, p->org);

		VectorNegate(dir, dir);
		vecmul(dir, 16, p->vel);
		p->vel[0] += crandom()*3;
		p->vel[1] += crandom()*3;
		p->vel[2] += crandom()*3;

		// RF, add some randomness
		p->accel[0] = crandom()*3;
		p->accel[1] = crandom()*3;
		p->accel[2] = crandom()*3;

		p->rotate = qtrue;

		p->roll = Com_Sign(crandom());
		p->accumroll = crandom()*180;

		p->alpha = 0.2f;
	}
}

void
particlemisc(qhandle_t pshader, vec3_t origin, int size, int duration, float alpha)
{
	cparticle_t *p;

	if(!pshader)
		cgprintf("CG_ParticleImpactSmokePuff pshader == ZERO!\n");

	if(!free_particles)
		return;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cg.time;
	p->alpha = 1.0;
	p->alphavel = 0;
	p->roll = rand()%179;

	p->pshader = pshader;

	if(duration > 0)
		p->endtime = cg.time + duration;
	else
		p->endtime = duration;

	p->startfade = cg.time;

	p->width = size;
	p->height = size;

	p->endheight = size;
	p->endwidth = size;

	p->type = P_SPRITE;

	veccpy(origin, p->org);

	p->rotate = qfalse;
}
