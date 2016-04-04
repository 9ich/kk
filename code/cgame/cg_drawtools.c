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
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

/*
================
drawsides

Coords are virtual 640x480
================
*/
void
drawsides(float x, float y, float w, float h, float size)
{
	adjustcoords(&x, &y, &w, &h);
	size *= cgs.scrnxscale;
	trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
}

void
drawtopbottom(float x, float y, float w, float h, float size)
{
	adjustcoords(&x, &y, &w, &h);
	size *= cgs.scrnyscale;
	trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

void
drawbigstr(int x, int y, const char *s, float alpha)
{
	float color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	drawstr2(x, y, s, color, BIGCHAR_WIDTH, BIGCHAR_HEIGHT);
}

void
drawbigstrcolor(int x, int y, const char *s, vec4_t color)
{
	drawstr2(x, y, s, color, BIGCHAR_WIDTH, BIGCHAR_HEIGHT);
}

void
drawsmallstr(int x, int y, const char *s, float alpha)
{
	float color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	drawstr2(x, y, s, color, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT);
}

void
drawsmallstrcolor(int x, int y, const char *s, vec4_t color)
{
	drawstr2(x, y, s, color, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT);
}

/*
=================
drawstrlen

Returns character count, skiping color escape codes
=================
*/
int
drawstrlen(const char *str)
{
	const char *s = str;
	int count = 0;

	while(*s){
		if(Q_IsColorString(s))
			s += 2;
		else{
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void
CG_TileClearBox(int x, int y, int w, int h, qhandle_t hShader)
{
	float s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
}

/*
==============
tileclear

Clear around a sized down screen
==============
*/
void
tileclear(void)
{
	int top, bottom, left, right;
	int w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if(cg.refdef.x == 0 && cg.refdef.y == 0 &&
	   cg.refdef.width == w && cg.refdef.height == h)
		return;	// full screen rendering

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox(0, 0, w, top, cgs.media.backTileShader);

	// clear below view screen
	CG_TileClearBox(0, bottom, w, h - bottom, cgs.media.backTileShader);

	// clear left of view screen
	CG_TileClearBox(0, top, left, bottom - top + 1, cgs.media.backTileShader);

	// clear right of view screen
	CG_TileClearBox(right, top, w - right, bottom - top + 1, cgs.media.backTileShader);
}

/*
================
fadecolor
================
*/
float *
fadecolor(int startMsec, int totalMsec)
{
	static vec4_t color;
	int t;

	if(startMsec == 0)
		return nil;

	t = cg.time - startMsec;

	if(t >= totalMsec)
		return nil;

	// fade out
	if(totalMsec - t < FADE_TIME)
		color[3] = (totalMsec - t) * 1.0/FADE_TIME;
	else
		color[3] = 1.0;
	color[0] = color[1] = color[2] = 1;

	return color;
}

/*
================
teamcolor
================
*/
float *
teamcolor(int team)
{
	static vec4_t red = {1, 0.2f, 0.2f, 1};
	static vec4_t blue = {0.2f, 0.2f, 1, 1};
	static vec4_t other = {1, 1, 1, 1};
	static vec4_t spectator = {0.7f, 0.7f, 0.7f, 1};

	switch(team){
	case TEAM_RED:
		return red;
	case TEAM_BLUE:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	default:
		return other;
	}
}

/*
=================
getcolorforhealth
=================
*/
void
getcolorforhealth(int health, int armor, vec4_t hcolor)
{
	int count;
	int max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if(health <= 0){
		vecclear(hcolor);	// black
		hcolor[3] = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / (1.0 - ARMOR_PROTECTION);
	if(max < count)
		count = max;
	health += count;

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if(health >= 100)
		hcolor[2] = 1.0;
	else if(health < 66)
		hcolor[2] = 0;
	else
		hcolor[2] = (health - 66) / 33.0;

	if(health > 60)
		hcolor[1] = 1.0;
	else if(health < 30)
		hcolor[1] = 0;
	else
		hcolor[1] = (health - 30) / 30.0;
}

/*
=================
colorforhealth
=================
*/
void
colorforhealth(vec4_t hcolor)
{
	getcolorforhealth(cg.snap->ps.stats[STAT_HEALTH],
			     cg.snap->ps.stats[STAT_ARMOR], hcolor);
}

