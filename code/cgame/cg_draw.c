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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

int drawTeamOverlayModificationCount = -1;

int sortedteamplayers[TEAM_MAXOVERLAY];
int nsortedteamplayers;

char syschat[256];
char teamchat1[256];
char teamchat2[256];

/*
================
drawmodel

================
*/
void
drawmodel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles)
{
	refdef_t refdef;
	refEntity_t ent;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer)
		return;

	aligncoords(&x, &y, &w, &h);
	scalecoords(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	veccpy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;	// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
Draws 2D damage indicators depending on the attacker's
position relative to our viewangles.
*/
void
drawdmgindicator(void)
{
	const float thresh = 0.5;	// 60 deg
	const float xofs = 140;
	const float yofs = 140;
	const float w = 140;
	const float h = 140;
	vec3_t dir;
	int attacker, t;
	float d, x, y;

	if(!cg_drawDamageDir.integer)
		return;

	if(!cg.dmgval)
		return;

	t = cg.time - cg.dmgtime;
	if(t <= 0 || t >= DAMAGE_TIME)
		return;

	attacker = getlastattacker();
	if(attacker == -1)
		return;

	vecsub(cg.snap->ps.origin, cg_entities[attacker].lerporigin, dir);
	vecnorm(dir);

	x = screenwidth()/2;
	y = screenheight()/2;

	setalign("midcenter");
	d = vecdot(cg.refdef.viewaxis[0], dir);
	if(d < -thresh)	// in front of us
		drawpic(x, y - yofs, w, h, cgs.media.hurtForwardShader);
	else if(d > thresh)	// behind us
		drawpic(x, y + yofs, w, -h, cgs.media.hurtForwardShader);

	d = vecdot(cg.refdef.viewaxis[1], dir);
	if(d < -thresh)	// on our left
		drawpic(x - xofs, y, w, h, cgs.media.hurtLeftShader);
	else if(d > thresh)	// on our right
		drawpic(x + xofs, y, -w, h, cgs.media.hurtLeftShader);

	d = vecdot(cg.refdef.viewaxis[2], dir);
	if(d < -thresh)	// below us
		drawpic(x, y - yofs, w, h, cgs.media.hurtUpShader);
	else if(d > thresh)	// above us
		drawpic(x, y + yofs, w, -h, cgs.media.hurtUpShader);

	setalign("");
}

/*
================
drawflag

Used for both the status bar and the scoreboard
================
*/
void
drawflag(float x, float y, float w, float h, int team, qboolean force2D)
{
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	qhandle_t handle;

	if(!force2D && cg_draw3dIcons.integer){
		vecclear(angles);

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2] = -0.5 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);;

		if(team == TEAM_RED)
			handle = cgs.media.redFlagModel;
		else if(team == TEAM_BLUE)
			handle = cgs.media.blueFlagModel;
		else if(team == TEAM_FREE)
			handle = cgs.media.neutralFlagModel;
		else
			return;
		drawmodel(x, y, w, h, handle, 0, origin, angles);
	}else if(cg_drawIcons.integer){
		gitem_t *item;

		if(team == TEAM_RED)
			item = finditemforpowerup(PW_REDFLAG);
		else if(team == TEAM_BLUE)
			item = finditemforpowerup(PW_BLUEFLAG);
		else if(team == TEAM_FREE)
			item = finditemforpowerup(PW_NEUTRALFLAG);
		else
			return;
		if(item)
			drawpic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
	}
}

/*
================
CG_DrawStatusBarFlag

================
*/
static void
CG_DrawStatusBarFlag(float x, int team)
{
	drawflag(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse);
}


/*
================
drawteambg

================
*/
void
drawteambg(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t hcolor;

	hcolor[3] = alpha;
	if(team == TEAM_RED){
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	}else if(team == TEAM_BLUE){
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}else
		return;
	trap_R_SetColor(hcolor);
	drawpic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(nil);
}

// this should go to common
static float
sawtoothwave(int time, float hz, float phase, float amp)
{
	time *= hz;
	return amp * (((time + (int)(phase*time)) % 1000) / 1000.0f);
}

void
drawweapontime(void)
{
	vec4_t clr;

	Vector4Copy(CRed, clr);
	clr[3] = sawtoothwave(cg.pps.weaponTime, 8, 0, 1);
	fillrect(0.5f*screenwidth() + 79, 314, 6, 6, clr);
}

/*
================
drawstatusbar

================
*/
static void
drawstatusbar(void)
{
	int color;
	centity_t *cent;
	playerState_t *ps;
	int value;
	vec4_t hcolor;
	vec3_t angles;
	vec3_t origin;

	static float colors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{1.0f, 0.69f, 0.0f, 1.0f},	// normal
		{1.0f, 0.2f, 0.2f, 1.0f},	// low health
		{0.5f, 0.5f, 0.5f, 1.0f},	// weapon firing
		{1.0f, 1.0f, 1.0f, 1.0f}
	};					// health > 100

	if(cg_drawStatus.integer == 0)
		return;

	// draw the team background
	drawteambg(0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM]);

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	vecclear(angles);

	// draw any 3D icons first, so the changes back to 2D are minimized
	if(cent->currstate.weapon && cg_weapons[cent->currstate.weapon].ammomodel){
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
		drawmodel(CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
			       cg_weapons[cent->currstate.weapon].ammomodel, 0, origin, angles);
	}

	if(cg.pps.powerups[PW_REDFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
	else if(cg.pps.powerups[PW_BLUEFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
	else if(cg.pps.powerups[PW_NEUTRALFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);

	if(ps->stats[STAT_ARMOR]){
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
		drawmodel(370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
			       cgs.media.armorModel, 0, origin, angles);
	}

	setalign("left");

	// ammo
	if(cent->currstate.weapon){
		value = ps->ammo[cent->currstate.weapon];
		if(value > -1){
			if(cg.pps.weaponTime > 100){
				// draw as dark grey when reloading
				color = 2;	// dark grey
				drawweapontime();
			}else{
				if(value >= 0)
					color = 0;	// green
				else
					color = 1;	// red
			}
			trap_R_SetColor(colors[color]);

			//drawfield(0, 432, 3, value);
			drawhudfield(0.5f*screenwidth() + 75, 320, va("%d", value), CWhite);
			trap_R_SetColor(nil);

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer){
				qhandle_t icon;

				icon = cg_weapons[cg.pps.weapon].ammoicon;
				if(icon)
					drawpic(CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon);
			}
		}
	}

	// health
	value = ps->stats[STAT_HEALTH];
	if(value > 100)
		trap_R_SetColor(colors[3]);	// white
	else if(value > 25)
		trap_R_SetColor(colors[0]);	// green
	else if(value > 0){
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor(colors[color]);
	}else
		trap_R_SetColor(colors[1]);	// red

	setalign("right");

	// stretch the health up when taking damage
	drawhudfield(0.5f*screenwidth() - 75, 320, va("%d", value), CWhite);
	colorforhealth(hcolor);
	trap_R_SetColor(hcolor);

	// armor
	value = ps->stats[STAT_ARMOR];
	if(value > 0){
		trap_R_SetColor(colors[0]);
		drawhudfield(0.5f*screenwidth() - 75, 358, va("%d", value), CWhite);
		trap_R_SetColor(nil);
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
			drawpic(370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon);

	}
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float
CG_DrawAttacker(float y)
{
	const char *info;
	const char *name;
	int clientNum;

	if(cg.pps.stats[STAT_HEALTH] <= 0)
		return y;

	if(!cg.attackertime)
		return y;

	clientNum = cg.pps.persistant[PERS_ATTACKER];
	if(clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum)
		return y;

	if(!cgs.clientinfo[clientNum].infovalid){
		cg.attackertime = 0;
		return y;
	}

	info = getconfigstr(CS_PLAYERS + clientNum);
	name = Info_ValueForKey(info, "n");
	pushalign("right");
	drawbigstr(screenwidth(), y, name, 0.5);
	popalign(1);

	return y + BIGCHAR_HEIGHT + 2;
}

static float
drawobituaries(float y)
{
	float x, yy, sz, pad;
	float *clr;
	int font, i;

	pad = 2;
	sz = 16;
	font = FONT2;

	// draw obituaries newest to oldest, bottom to top
	y += (ARRAY_LEN(cg.obit) - 1) * (sz + pad);
	yy = y;
	pushalign("left");
	for(i = 0; i < ARRAY_LEN(cg.obit); i++){
		clr = fadecolor(cg.obit[i].time, 4000);
		if(clr[3] < 0.01f)
			continue;

		// truncate names that take the piss
		truncstringtowidth(cg.obit[i].killer, font, sz, 120);
		truncstringtowidth(cg.obit[i].victim, font, sz, 120);

		x = screenwidth() - 2;

		// killer
		x -= stringwidth(cg.obit[i].victim, font, sz, 0, -1) + pad;
		drawstring(x, y, cg.obit[i].victim, font, sz, clr);
		trap_R_SetColor(clr);
		// means of death
		x -= sz + pad + 2;
		drawpic(x, y, sz, sz, cg.obit[i].icon);
		// victim
		x -= stringwidth(cg.obit[i].killer, font, sz, 0, -1) + pad;
		drawstring(x, y, cg.obit[i].killer, font, sz, clr);

		y -= sz + pad;
	}
	popalign(1);

	return yy;
}

/*
==================
drawsnap
==================
*/
static float
drawsnap(float y)
{
	char *s;
	int w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
	       cg.latestsnapnum, cgs.serverCommandSequence);
	w = drawstrlen(s) * BIGCHAR_WIDTH;

	drawbigstr(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
drawfps
==================
*/
#define FPS_FRAMES 4
static float
drawfps(float y)
{
	char *s;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frametime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frametime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frametime;
	index++;
	if(index > FPS_FRAMES){
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++)
			total += previousTimes[i];
		if(!total)
			total = 1;
		fps = 1000 * FPS_FRAMES / total;

		s = va("%i", fps);

		setalign("topright");
		drawfixedstr(screenwidth() - 2, y + 2, s, 1.0F);
		setalign("");
	}

	return y + 16;
}

#define SPEEDOMETER_FRAMES 20

static void
drawspeedometer(void)
{
	static float speeds[SPEEDOMETER_FRAMES];
	static int index;
	const float limit = 1500.0f;
	const float width = 100.0f, height = 16.0f;
	const float bgalpha = 40/255.0f, alpha = 0.7f;
	float x, y, speed, avgspeed, frac, avgfrac;
	vec4_t clr;
	vec3_t dir;
	char *s;
	int i;

	speed = veclen(cg.pps.velocity);
	speeds[index % SPEEDOMETER_FRAMES] = speed;
	index++;

	setalign("center");

	// draw instantaneous speed
	s = va("%iu/s", (int)speed);
	drawfixedstr(0.5f*screenwidth(), 330, s, 1.0f);

	setalign("left");

	x = 0.5f*screenwidth();
	y = 345;

	x -= 0.5f*width;	// centre

	Vector4Copy(CWhite, clr);
	clr[3] = bgalpha;
	fillrect(x, y, width, height, clr);
	if(0)
		drawrect(x-1, y-1, width+2, height+2, CWhite);	// outline

	// draw angle between forward and velocity
	veccpy(cg.pps.velocity, dir);
	vecnorm(dir);
	Vector4Copy(CGold, clr);
	clr[3] = alpha;
	fillrect(x, y, width*MAX(0, vecdot(dir, cg.refdef.viewaxis[0])), height/4, clr);

	// draw instantaneous speed bar
	speed = MIN(limit, speed);
	frac = speed / limit;
	Vector4Copy(CWhite, clr);
	clr[3] = alpha;
	fillrect(x, y+(height/4), width*frac, height/4, clr);

	if(index > SPEEDOMETER_FRAMES){
		// take root mean square of speeds over a window
		avgspeed = 0;
		for(i = 0; i < SPEEDOMETER_FRAMES; i++)
			avgspeed += Square(speeds[i]);
		avgspeed = sqrt(avgspeed / SPEEDOMETER_FRAMES);
		// draw RMS speed bar
		avgspeed = MIN(limit, avgspeed);
		avgfrac = avgspeed / limit;
		Vector4Copy(CMediumSlateBlue, clr);
		clr[3] = alpha;
		fillrect(x, y+2*(height/4), width*avgfrac, height/2, clr);
	}

	// draw axis ticks
	fillrect(x, y+height-2, 1, 2, CBlack);
	fillrect(x - 0.5f + width/2, y+height-2, 1, 2, CBlack);
	fillrect(x - 1 + width, y+height-2, 1, 2, CBlack);

	setalign("");
}

/*
=================
drawtimer
=================
*/
static float
drawtimer(float y)
{
	char *s;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);

	pushalign("right");
	drawfixedstr(screenwidth() - 2, y + 2, s, 1.0F);
	popalign(1);

	return y + 16;
}

/*
=================
CG_DrawTeamOverlay
=================
*/

static float
CG_DrawTeamOverlay(float y, qboolean right, qboolean upper)
{
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t *item;
	int ret_y, count;

	if(!cg_drawTeamOverlay.integer)
		return y;

	if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
		return y;	// Not on any team

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (nsortedteamplayers > 8) ? 8 : nsortedteamplayers;
	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedteamplayers[i];
		if(ci->infovalid && ci->team == cg.snap->ps.persistant[PERS_TEAM]){
			plyrs++;
			len = drawstrlen(ci->name);
			if(len > pwidth)
				pwidth = len;
		}
	}

	if(!plyrs)
		return y;

	if(pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for(i = 1; i < MAX_LOCATIONS; i++){
		p = getconfigstr(CS_LOCATIONS + i);
		if(p && *p){
			len = drawstrlen(p);
			if(len > lwidth)
				lwidth = len;
		}
	}

	if(lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if(right)
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if(upper)
		ret_y = y + h;
	else{
		y -= h;
		ret_y = y;
	}

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	}else{	// if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor(hcolor);
	drawpic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(nil);

	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedteamplayers[i];
		if(ci->infovalid && ci->team == cg.snap->ps.persistant[PERS_TEAM]){
			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			drawstring(xx, y, ci->name, FONT2, 12, hcolor);

			if(lwidth){
				p = getconfigstr(CS_LOCATIONS + ci->location);
				if(!p || !*p)
					p = "unknown";
//				len = drawstrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				drawstring(xx, y, p, FONT2, 12, hcolor);
			}

			getcolorforhealth(ci->health, ci->armor, hcolor);

			Com_sprintf(st, sizeof(st), "%3i %3i", ci->health, ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
			     TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			drawstring(xx, y, st, FONT2, 12, hcolor);

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if(cg_weapons[ci->currweap].icon)
				drawpic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					   cg_weapons[ci->currweap].icon);
			else
				drawpic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					   cgs.media.deferShader);

			// Draw powerup icons
			if(right)
				xx = x;
			else
				xx = x + w - TINYCHAR_WIDTH;
			for(j = 0; j <= PW_NUM_POWERUPS; j++)
				if(ci->powerups & (1 << j)){
					item = finditemforpowerup(j);

					if(item){
						drawpic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
							   trap_R_RegisterShader(item->icon));
						if(right)
							xx -= TINYCHAR_WIDTH;
						else
							xx += TINYCHAR_WIDTH;
					}
				}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}

/*
=====================
drawupperright

=====================
*/
static void
drawupperright(stereoFrame_t stereoFrame)
{
	float y;

	y = 0;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1)
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	if(cg_drawSnapshot.integer)
		y = drawsnap(y);
	if(cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT))
		y = drawfps(y);
	if(cg_drawTimer.integer)
		y = drawtimer(y);
	if(cg_drawObituaries.integer)
		y = drawobituaries(y);
	if(cg_drawAttacker.integer)
		y = CG_DrawAttacker(y);
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
================
drawpowerups
================
*/
static float
drawpowerups(float y)
{
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t *ps;
	int t;
	gitem_t *item;
	int x;
	int color;
	float size;
	float f;
	static float colors[2][4] = {
		{0.2f, 1.0f, 0.2f, 1.0f},
		{1.0f, 0.2f, 0.2f, 1.0f}
	};

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0)
		return y;

	// sort the list by time remaining
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++){
		if(!ps->powerups[i])
			continue;
		t = ps->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if(t < 0 || t > 999000)
			continue;

		// insert into the list
		for(j = 0; j < active; j++)
			if(sortedTime[j] >= t){
				for(k = active - 1; k >= j; k--){
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for(i = 0; i < active; i++){
		item = finditemforpowerup(sorted[i]);

		if(item){
			color = 1;

			y -= ICON_SIZE;

			drawhudfield(x, y, va("%i", sortedTime[i] / 1000), colors[color]);

			t = ps->powerups[sorted[i]];
			if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
				trap_R_SetColor(nil);
			else{
				vec4_t modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if(cg.powerupactive == sorted[i] &&
			   cg.time - cg.poweruptime < PULSE_TIME){
				f = 1.0 - (((float)cg.time - cg.poweruptime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			}else
				size = ICON_SIZE;

			drawpic(640 - size, y + ICON_SIZE / 2 - size / 2,
				   size, size, trap_R_RegisterShader(item->icon));
		}
	}
	trap_R_SetColor(nil);

	return y;
}


/*
=====================
drawlowerright

=====================
*/
static void
drawlowerright(void)
{
	float y;

	y = 480 - ICON_SIZE;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2)
		y = CG_DrawTeamOverlay(y, qtrue, qfalse);
	drawpowerups(y);
}


/*
===================
drawpickup
===================
*/
static int
drawpickup(int y)
{
	int value;
	float *fadeColor;

	if(cg.snap->ps.stats[STAT_HEALTH] <= 0)
		return y;

	y -= ICON_SIZE;

	value = cg.itempkup;
	if(value){
		fadeColor = fadecolor(cg.itempkuptime, 3000);
		if(fadeColor){
			registeritemgfx(value);
			trap_R_SetColor(fadeColor);
			drawpic(8, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
			drawbigstr(ICON_SIZE + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[value].pickupname, fadeColor[0]);
			trap_R_SetColor(nil);
		}
	}

	return y;
}


/*
=====================
drawlowerleft

=====================
*/
static void
drawlowerleft(void)
{
	float y;

	y = 480 - ICON_SIZE;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3)
		y = CG_DrawTeamOverlay(y, qfalse, qfalse);


	drawpickup(y);
}


//===========================================================================================

/*
=================
drawteaminfo
=================
*/
static void
drawteaminfo(void)
{
	int h;
	int i;
	int font;
	float size;
	vec4_t hcolor;
	int chatHeight;

#define CHATLOC_Y	420	// bottom end
#define CHATLOC_X	0

	font = FONT2;
	size = 14;
	i = 0;

	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if(chatHeight <= 0)
		return;		// disabled

	if(cgs.teamlastchatpos != cgs.teamchatpos){
		if(cg.time - cgs.teamchatmsgtimes[cgs.teamlastchatpos % chatHeight] > cg_teamChatTime.integer)
			cgs.teamlastchatpos++;

		h = (cgs.teamchatpos - cgs.teamlastchatpos) *
		   stringheight(cgs.teamchatmsgs[i % chatHeight], font, size);	// FIXME: what was this?

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		}else{
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor(hcolor);
		drawpic(CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar);
		trap_R_SetColor(nil);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for(i = cgs.teamchatpos - 1; i >= cgs.teamlastchatpos; i--)
			drawstring(CHATLOC_X + TINYCHAR_WIDTH, CHATLOC_Y - (cgs.teamchatpos - i)*TINYCHAR_HEIGHT, cgs.teamchatmsgs[i % chatHeight], font, size, hcolor);
	}
}


/*
===================
drawholdable
===================
*/
static void
drawholdable(void)
{
	int value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value){
		registeritemgfx(value);
		drawpic(640-ICON_SIZE, (screenheight()-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
	}
}



/*
===================
drawreward
===================
*/
static void
drawreward(void)
{
	float *color;
	int i, count;
	float x, y;
	char buf[32];

	if(!cg_drawRewards.integer)
		return;

	color = fadecolor(cg.rewardtime, REWARD_TIME);
	if(!color){
		if(cg.rewardstack > 0){
			for(i = 0; i < cg.rewardstack; i++){
				cg.rewardsounds[i] = cg.rewardsounds[i+1];
				cg.rewardshaders[i] = cg.rewardshaders[i+1];
				Q_strncpyz(cg.rewardmsgs[i], cg.rewardmsgs[i+1],
				   sizeof cg.rewardmsgs[i]);
				cg.nrewards[i] = cg.nrewards[i+1];
			}
			cg.rewardtime = cg.time;
			cg.rewardstack--;
			color = fadecolor(cg.rewardtime, REWARD_TIME);
			if(cg.rewardsounds[0])
				addbufferedsound(cg.rewardsounds[0]);
		}else
			return;
	}

	trap_R_SetColor(color);

	/*
	count = cg.nrewards[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			drawpic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardshaders[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.nrewards[0] - count*10;		// number of small rewards to draw
	*/

	if(cg.nrewards[0] >= 10 || !cg.rewardshaders[0]){
		y = 56;
		setalign("center");
		if(cg.rewardshaders[0])
			drawpic(0.5f*screenwidth(), y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardshaders[0]);
		if(cg.nrewards[0] > 1){
			Com_sprintf(buf, sizeof(buf), "%d x", cg.nrewards[0]);
			drawstring(0.5f*screenwidth(), y+ICON_SIZE, buf, FONT1, 18, CText);
		}
		setalign("");
	}else{
		count = cg.nrewards[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for(i = 0; i < count; i++){
			drawpic(x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardshaders[0]);
			x += ICON_SIZE;
		}
	}
	centerprint(cg.rewardmsgs[cg.rewardstack], screenheight()*0.30f, BIGCHAR_WIDTH);
	trap_R_SetColor(nil);
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES 128

typedef struct
{
	int	frameSamples[LAG_SAMPLES];
	int	frameCount;
	int	snapshotFlags[LAG_SAMPLES];
	int	snapshotSamples[LAG_SAMPLES];
	int	snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
lagometerframeinfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void
lagometerframeinfo(void)
{
	int offset;

	offset = cg.time - cg.latestsnapttime;
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
	lagometer.frameCount++;
}

/*
==============
lagometersnapinfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass nil for a dropped packet.
==============
*/
void
lagometersnapinfo(snapshot_t *snap)
{
	// dropped packet
	if(!snap){
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
drawdisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void
drawdisconnect(void)
{
	int cmdNum;
	usercmd_t cmd;
	const char *s;
	int w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if(cmd.serverTime <= cg.snap->ps.commandTime
	   || cmd.serverTime > cg.time)		// special check for map_restart
		return;

	// also add text in center of screen
	s = "connection interrupted";
	w = drawstrlen(s) * BIGCHAR_WIDTH;
	drawbigstr(320 - w/2, 100, s, 1.0F);

	// blink the icon
	if((cg.time >> 9) & 1)
		return;

	setalign("bottomright");
	drawnamedpic(screenwidth(), screenheight(), 48, 48, "gfx/2d/net.tga");
	setalign("");
}

#define MAX_LAGOMETER_PING	900
#define MAX_LAGOMETER_RANGE	300

/*
==============
drawlagometer
==============
*/
static void
drawlagometer(void)
{
	int a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if(!cg_lagometer.integer){
		drawdisconnect();
		return;
	}

	// draw the graph
	setalign("bottomright");

	x = screenwidth();
	y = screenheight();

	trap_R_SetColor(nil);
	drawpic(x, y, 48, 48, cgs.media.lagometerShader);

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	aligncoords(&ax, &ay, &aw, &ah);
	scalecoords(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for(a = 0; a < aw; a++){
		i = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if(v > 0){
			if(color != 1){
				color = 1;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
			}
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 2){
				color = 2;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
			}
			v = -v;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for(a = 0; a < aw; a++){
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if(v > 0){
			if(lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED){
				if(color != 5){
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
				}
			}else  if(color != 3){
				color = 3;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
			}
			v = v * vscale;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 4){
				color = 4;	// RED for dropped snapshots
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(nil);

	if(cg_nopredict.integer || cg_synchronousClients.integer)
		drawbigstr(x, y, "snc", 1.0);

	setalign("");

	drawdisconnect();
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
centerprint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void
centerprint(const char *str, int y, int charWidth)
{
	char *s;

	Q_strncpyz(cg.centerprint, str, sizeof(cg.centerprint));

	cg.centerprinttime = cg.time;
	cg.centerprinty = y;
	cg.centerprintcharwidth = charWidth;

	// count the number of lines for centering
	cg.centerprintlines = 1;
	s = cg.centerprint;
	while(*s){
		if(*s == '\n')
			cg.centerprintlines++;
		s++;
	}
}

/*
===================
drawcenterstr
===================
*/
static void
drawcenterstr(void)
{
	char *start;
	int l;
	int y;
	float *color;

	if(!cg.centerprinttime)
		return;

	color = fadecolor(cg.centerprinttime, 1000 * cg_centertime.value);
	if(!color)
		return;

	trap_R_SetColor(color);

	start = cg.centerprint;

	y = cg.centerprinty - cg.centerprintlines * BIGCHAR_HEIGHT / 2;

	while(1){
		char linebuffer[1024];

		for(l = 0; l < 50; l++){
			if(!start[l] || start[l] == '\n')
				break;
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		setalign("center");
		drawstring(0.5f*screenwidth(), y, linebuffer, FONT1, 24, color);
		setalign("");

		y += cg.centerprintcharwidth * 1.5;
		while(*start && (*start != '\n'))
			start++;
		if(!*start)
			break;
		start++;
	}

	trap_R_SetColor(nil);
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
drawxhair
=================
*/
static void
drawxhair(void)
{
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int ca;
	vec4_t clr;

	if(!cg_drawCrosshair.integer)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(cg.thirdperson)
		return;

	if(cg_crosshairHealth.integer)
		colorforhealth(clr);	// set color based on health
	else
		Com_HexStrToColor(cg_crosshairColor.string, clr);
	trap_R_SetColor(clr);

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itempkupblendtime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
		h *= (1 + f);
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	//w = round(w);
	//h = round(h);

	ca = cg_drawCrosshair.integer - 1;
	if(ca < 0)
		ca = 0;
	if(ca >= NUM_CROSSHAIRS)
		ca = NUM_CROSSHAIRS - 1;
	hShader = cgs.media.crosshairShader[ca];

	setalign("mid center");
	drawpic(screenwidth()/2.0 + x, screenheight()/2.0 + y, w, h, hShader);
	setalign("");

	trap_R_SetColor(nil);
}

/*
=================
drawxhair3d
=================
*/
static void
drawxhair3d(void)
{
	float w;
	qhandle_t hShader;
	float f;
	int ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if(!cg_drawCrosshair.integer)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(cg.thirdperson)
		return;

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itempkupblendtime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
	}

	ca = cg_drawCrosshair.integer;
	if(ca < 0)
		ca = 0;
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);

	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	vecmad(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	cgtrace(&trace, cg.refdef.vieworg, nil, nil, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

	veccpy(trace.endpos, ent.origin);

	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

/*
=================
xhairscan
=================
*/
static void
xhairscan(void)
{
	trace_t trace;
	vec3_t start, end;
	int content;

	veccpy(cg.refdef.vieworg, start);
	vecmad(start, 131072, cg.refdef.viewaxis[0], end);

	cgtrace(&trace, start, vec3_origin, vec3_origin, end,
		 cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);
	if(trace.entityNum >= MAX_CLIENTS)
		return;

	// if the player is in fog, don't show it
	content = pointcontents(trace.endpos, 0);
	if(content & CONTENTS_FOG)
		return;

	// if the player is invisible, don't show it
	if(cg_entities[trace.entityNum].currstate.powerups & (1 << PW_INVIS))
		return;

	// update the fade timer
	cg.xhairclientnum = trace.entityNum;
	cg.xhairclienttime = cg.time;
}

/*
=====================
drawxhairnames
=====================
*/
static void
drawxhairnames(void)
{
	float *color;
	char *name;
	float w;

	if(!cg_drawCrosshair.integer)
		return;
	if(!cg_drawCrosshairNames.integer)
		return;
	if(cg.thirdperson)
		return;

	// scan the known entities to see if the crosshair is sighted on one
	xhairscan();

	// draw the name of the player being looked at
	color = fadecolor(cg.xhairclienttime, 1000);
	if(!color){
		trap_R_SetColor(nil);
		return;
	}

	name = cgs.clientinfo[cg.xhairclientnum].name;
	w = drawstrlen(name) * BIGCHAR_WIDTH;
	drawbigstr(320 - w / 2, 170, name, color[3] * 0.5f);
	trap_R_SetColor(nil);
}

//==============================================================================

/*
=================
drawspec
=================
*/
static void
drawspec(void)
{
	drawbigstr(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if(cgs.gametype == GT_TOURNAMENT)
		drawbigstr(320 - 15 * 8, 460, "waiting to play", 1.0F);
	else if(cgs.gametype >= GT_TEAM)
		drawbigstr(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
}

static void
drawlockonwarning(void)
{
	int i;
	entityState_t *es;

	for(i = 0; i < cgs.maxclients; i++){
		es = &cg_entities[i].currstate;
		if(!cgs.clientinfo[i].infovalid ||
		   es->lockontarget != cg.snap->ps.clientNum ||
		   es->lockonstarttime == 0)
			continue;

		if(es->lockontime - es->lockonstarttime > HOMING_SCANWAIT)
			drawbigstr(screenwidth()/2, screenheight()/2 - 100, "ENEMY LOCK", 1.0f);
		else
			drawbigstr(screenwidth()/2, screenheight()/2 - 100, "WARNING", 1.0f);
		break;
	}
}

/*
=================
drawvote
=================
*/
static void
drawvote(void)
{
	char *s;
	int sec;

	if(!cgs.votetime)
		return;

	// play a talk beep whenever it is modified
	if(cgs.votemodified){
		cgs.votemodified = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.votetime)) / 1000;
	if(sec < 0)
		sec = 0;

	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.votestr, cgs.voteyes, cgs.voteno);
	drawsmallstr(0, 58, s, 1.0F);
}

/*
=================
drawteamvote
=================
*/
static void
drawteamvote(void)
{
	char *s;
	int sec, cs_offset;

	if(cgs.clientinfo[cg.clientNum].team == TEAM_RED)
		cs_offset = 0;
	else if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!cgs.teamvotetime[cs_offset])
		return;

	// play a talk beep whenever it is modified
	if(cgs.teamVoteModified[cs_offset]){
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamvotetime[cs_offset])) / 1000;
	if(sec < 0)
		sec = 0;
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamvotestr[cs_offset],
	       cgs.teamvoteyes[cs_offset], cgs.teamvoteno[cs_offset]);
	drawsmallstr(0, 90, s, 1.0F);
}

static qboolean
drawscoreboard(void)
{
	return drawoldscoreboard();
}

/*
=================
drawintermission
=================
*/
static void
drawintermission(void)
{
//	int key;
	if(cgs.gametype == GT_SINGLE_PLAYER){
		drawcenterstr();
		return;
	}
	cg.scorefadetime = cg.time;
	cg.scoreboardshown = drawscoreboard();
}

/*
=================
drawfollow
=================
*/
static qboolean
drawfollow(void)
{
	float x;
	vec4_t color;
	const char *name;

	if(!(cg.snap->ps.pm_flags & PMF_FOLLOW))
		return qfalse;
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	drawbigstr(320 - 9 * 8, 24, "following", 1.0F);

	name = cgs.clientinfo[cg.snap->ps.clientNum].name;

	x = 0.5 * (640 - GIANT_WIDTH * drawstrlen(name));

	drawstring(x, 40, name, FONT2, 12, color);

	return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void
CG_DrawAmmoWarning(void)
{
	const char *s;

	if(cg_drawAmmoWarning.integer == 0)
		return;

	if(!cg.lowAmmoWarning)
		return;

	if(cg.lowAmmoWarning == 2)
		s = "OUT OF AMMO";
	else
		s = "LOW AMMO";
	pushalign("midcenter");
	drawbigstr(0.5f*screenwidth(), screenheight()-100, s, 0.8f);
	popalign(1);
}

#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void
CG_DrawProxWarning(void)
{
	char s [32];
	int w;
	static int proxTime;
	int proxTick;

	if(!(cg.snap->ps.eFlags & EF_TICKING)){
		proxTime = 0;
		return;
	}

	if(proxTime == 0)
		proxTime = cg.time;

	proxTick = 10 - ((cg.time - proxTime) / 1000);

	if(proxTick > 0 && proxTick <= 5)
		Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
	else
		Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");

	w = drawstrlen(s) * BIGCHAR_WIDTH;
	drawbigstrcolor(320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)]);
}

#endif

/*
=================
drawwarmup
=================
*/
static void
drawwarmup(void)
{
	int sec;
	int i;
	clientInfo_t *ci1, *ci2;
	const char *s;
	char buf[MAX_INFO_STRING];

	sec = cg.warmup;
	if(!sec)
		return;

	if(sec == WARMUP_NEEDPLAYERS){
		s = "Waiting for players";
		pushalign("center");
		drawbigstr(0.5f*screenwidth(), 24, s, 1.0F);
		popalign(1);
		cg.warmupcount = 0;
		return;
	}else if(sec == WARMUP_READYUP){
		trap_Cvar_VariableStringBuffer("cl_ready", buf, sizeof buf);
		if(!atoi(buf))
			s = "Press F3 to ready up";
		else
			s = "Waiting for players to ready up";
		pushalign("center");
		drawbigstr(0.5f*screenwidth(), 24, s, 1.0F);
		popalign(1);
		cg.warmupcount = 0;
		return;
	}

	if(cgs.gametype == GT_TOURNAMENT){
		// find the two active players
		ci1 = nil;
		ci2 = nil;
		for(i = 0; i < cgs.maxclients; i++)
			if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_FREE){
				if(!ci1)
					ci1 = &cgs.clientinfo[i];
				else
					ci2 = &cgs.clientinfo[i];
			}

		if(ci1 && ci2){
			s = va("%s vs %s", ci1->name, ci2->name);
			pushalign("center");
			drawstring(0.5f*screenwidth(), 20, s, FONT2, 23,
				   colorWhite);
			popalign(1);
		}
	}else{
		if(cgs.gametype == GT_FFA)
			s = "Free For All";
		else if(cgs.gametype == GT_LMS)
			s = "Last Man Standing";
		else if(cgs.gametype == GT_TEAM)
			s = "Team Deathmatch";
		else if(cgs.gametype == GT_CTF)
			s = "Capture the Flag";
		else if(cgs.gametype == GT_LTS)
			s = "Last Team Standing";
		else if(cgs.gametype == GT_CA)
			s = "Clan Arena";
		else if(cgs.gametype == GT_1FCTF)
			s = "One Flag CTF";
		else if(cgs.gametype == GT_OBELISK)
			s = "Overload";
		else if(cgs.gametype == GT_CP)
			s = "Control Point";
		else if(cgs.gametype == GT_HARVESTER)
			s = "Harvester";
		else
			s = "";

		pushalign("center");
		drawstring(0.5f*screenwidth(), 25, s, FONT2, 23, colorWhite);
		popalign(1);
	}

	sec = (sec - cg.time) / 1000;
	if(sec < 0){
		cg.warmup = 0;
		sec = 0;
	}
	s = va("Starts in: %i", sec + 1);
	if(sec != cg.warmupcount){
		cg.warmupcount = sec;
		switch(sec){
		case 0:
			trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
			break;
		case 1:
			trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
			break;
		case 2:
			trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
			break;
		default:
			break;
		}
	}

	pushalign("center");
	drawstring(0.5f*screenwidth(), 70, s, FONT1, 24, CText);
	popalign(1);
}

static void
drawroundwarmup(void)
{
	int sec;
	int i;
	const char *s;

	sec = cg.roundwarmup;
	if(!sec)
		return;

	if(sec < 0){
		cg.roundwarmupcount = 0;
		return;
	}

	sec = (sec - cg.time) / 1000;
	if(sec < 0){
		cg.roundwarmup = 0;
		sec = 0;
	}
	s = va("Round begins in: %i", sec + 1);
	if(sec != cg.roundwarmupcount){
		cg.roundwarmupcount = sec;
		switch(sec){
		case 0:
			trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
			break;
		case 1:
			trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
			break;
		case 2:
			trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
			break;
		case 3:
			trap_S_StartLocalSound(cgs.media.count4Sound, CHAN_ANNOUNCER);
		default:
			break;
		}
	}

	pushalign("center");
	drawstring(0.5f*screenwidth(), 70, s, FONT1, 24, CText);
	popalign(1);
}

static void
drawreadyup(void)
{
	static char lastcs[MAX_INFO_STRING];
	const char *cs, *info, *name, *msg;
	int i;

	cs = getconfigstr(CS_LAST_READY);
	if(*cs == '\0')
		return;
	if(strcmp(cs, lastcs) == 0)
		return;
	Q_strncpyz(lastcs, cs, sizeof lastcs);

	if(Q_strncmp(cs, "ready\\", strlen("ready\\")) == 0){
		cs += strlen("ready\\");
		msg = "is ready";
	}else if(Q_strncmp(cs, "notready\\", strlen("notready\\")) == 0){
		cs += strlen("notready\\");
		msg = "is not ready";
	}

	i = atoi(cs);
	if(i < 0 || i > MAX_CLIENTS)
		return;	// bogus data from server

	info = getconfigstr(CS_PLAYERS + cg_entities[i].currstate.number);
	name = Info_ValueForKey(info, "n");
	centerprint(va("%s %s", name, msg), 16, BIGCHAR_WIDTH);
}

/*
 * Draws GT_CP control point statuses.
 */
static void
drawcp(void)
{
	centity_t *cp;
	int font;
	float sz, x, y;
	int i;
	char *s, *status, *team;

	pushalign("topleft");

	font = FONT4;
	sz = 12;

	x = 0;
	y = screenheight()/2;
	if(cgs.gametype != GT_CP)
		return;
	for(i = 0; i < cgs.numcp; i++){
		cp = cgs.cp[i];
		switch(cp->cp.status){
		case CP_IDLE:
			status = "idle";
			break;
		case CP_INACTIVE:
			status = "disabled";
			break;
		case CP_LOCKED:
			status = "locked";
			break;
		case CP_CONTESTED:
			status = "contested";
			break;
		case CP_DECAYING:
			status = "decaying";
			break;
		case CP_DEADLOCK:
			status = "deadlocked";
			break;
		default:
			status = "what";
		}
		switch(cp->cp.owner){
		case TEAM_RED:
			team = "red";
			break;
		case TEAM_BLUE:
			team = "blue";
			break;
		default:
			team = "nobody";
		}
		s = va("Control point %d is %s, owned by %s, %d%% captured", i, status,
		   team, (int)(cp->cp.progress*100));
		drawstring(x, y, s, font, sz, CPaleVioletRed);
		y += sz;
	}

	popalign(1);
}

//==================================================================================
/*
=================
draw2d
=================
*/
static void
draw2d(stereoFrame_t stereoFrame)
{
	// if we are taking a levelshot for the menu, don't draw anything
	if(cg.levelshot)
		return;

	if(cg_draw2D.integer == 0)
		return;

	if(cg.snap->ps.pm_type == PM_INTERMISSION){
		drawintermission();
		return;
	}

/*
	if (cg.cameramode) {
		return;
	}
*/

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR){
		drawspec();

		if(stereoFrame == STEREO_CENTER)
			drawxhair();

		drawxhairnames();
	}else{
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if(!cg.showscores && cg.snap->ps.stats[STAT_HEALTH] > 0){
			drawstatusbar();

			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif
			if(stereoFrame == STEREO_CENTER)
				drawxhair();
			drawxhairnames();
			drawweapsel();

			drawholdable();
		}
		if(!cg.showscores)
			drawreward();

		if(cgs.gametype >= GT_TEAM)
			drawteaminfo();
	}

	drawdmgindicator();

	drawlockonwarning();

	drawvote();
	drawteamvote();

	drawlagometer();
	if(cg_drawSpeedometer.integer)
		drawspeedometer();

	drawupperright(stereoFrame);

	drawlowerright();
	drawlowerleft();

	if(!drawfollow())
		drawwarmup();
	if(!drawfollow())
		drawroundwarmup();

	drawreadyup();

	// don't draw center string if scoreboard is up
	cg.scoreboardshown = drawscoreboard();
	if(!cg.scoreboardshown)
		drawcenterstr();

	drawcp();
}

static void
CG_DrawTourneyScoreboard(void)
{
	CG_DrawOldTourneyScoreboard();
}

/*
=====================
drawactive

Perform all drawing needed to completely fill the screen
=====================
*/
void
drawactive(stereoFrame_t stereoview)
{
	drawlibbeginframe(cg.time, cg.refdef.width, cg.refdef.height);

	// optionally draw the info screen instead
	if(!cg.snap){
		drawinfo();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
	   (cg.snap->ps.pm_flags & PMF_SCOREBOARD)){
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	tileclear();

	if(stereoview != STEREO_CENTER)
		drawxhair3d();

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	// draw status bar and other floating elements
	draw2d(stereoview);
}
