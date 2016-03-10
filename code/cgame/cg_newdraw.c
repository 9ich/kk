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

#ifndef MISSIONPACK
#error This file not be used for classic Q3A.
#endif

#include "cg_local.h"
#include "../ui/ui_shared.h"

extern displayContextDef_t cgDC;

// set in CG_ParseTeamInfo

//static int sortedteamplayers[TEAM_MAXOVERLAY];
//static int nsortedteamplayers;
int drawTeamOverlayModificationCount = -1;

//static char syschat[256];
//static char teamchat1[256];
//static char teamchat2[256];

void
CG_InitTeamChat(void)
{
	memset(teamchat1, 0, sizeof(teamchat1));
	memset(teamchat2, 0, sizeof(teamchat2));
	memset(syschat, 0, sizeof(syschat));
}

void
CG_SetPrintString(int type, const char *p)
{
	if(type == SYSTEM_PRINT)
		strcpy(syschat, p);
	else{
		strcpy(teamchat2, teamchat1);
		strcpy(teamchat1, p);
	}
}

void
CG_CheckOrderPending(void)
{
	if(cgs.gametype < GT_CTF)
		return;
	if(cgs.orderPending){
		//clientInfo_t *ci = cgs.clientinfo + sortedteamplayers[cg_currentSelectedPlayer.integer];
		const char *p1, *p2, *b;
		p1 = p2 = b = nil;
		switch(cgs.currentOrder){
		case TEAMTASK_OFFENSE:
			p1 = VOICECHAT_ONOFFENSE;
			p2 = VOICECHAT_OFFENSE;
			b = "+button7; wait; -button7";
			break;
		case TEAMTASK_DEFENSE:
			p1 = VOICECHAT_ONDEFENSE;
			p2 = VOICECHAT_DEFEND;
			b = "+button8; wait; -button8";
			break;
		case TEAMTASK_PATROL:
			p1 = VOICECHAT_ONPATROL;
			p2 = VOICECHAT_PATROL;
			b = "+button9; wait; -button9";
			break;
		case TEAMTASK_FOLLOW:
			p1 = VOICECHAT_ONFOLLOW;
			p2 = VOICECHAT_FOLLOWME;
			b = "+button10; wait; -button10";
			break;
		case TEAMTASK_CAMP:
			p1 = VOICECHAT_ONCAMPING;
			p2 = VOICECHAT_CAMP;
			break;
		case TEAMTASK_RETRIEVE:
			p1 = VOICECHAT_ONGETFLAG;
			p2 = VOICECHAT_RETURNFLAG;
			break;
		case TEAMTASK_ESCORT:
			p1 = VOICECHAT_ONFOLLOWCARRIER;
			p2 = VOICECHAT_FOLLOWFLAGCARRIER;
			break;
		}

		if(cg_currentSelectedPlayer.integer == nsortedteamplayers)
			// to everyone
			trap_SendConsoleCommand(va("cmd vsay_team %s\n", p2));
		else{
			// for the player self
			if(sortedteamplayers[cg_currentSelectedPlayer.integer] == cg.snap->ps.clientNum && p1){
				trap_SendConsoleCommand(va("teamtask %i\n", cgs.currentOrder));
				//trap_SendConsoleCommand(va("cmd say_team %s\n", p2));
				trap_SendConsoleCommand(va("cmd vsay_team %s\n", p1));
			}else if(p2)
				//trap_SendConsoleCommand(va("cmd say_team %s, %s\n", ci->name,p));
				trap_SendConsoleCommand(va("cmd vtell %d %s\n", sortedteamplayers[cg_currentSelectedPlayer.integer], p2));
		}
		if(b)
			trap_SendConsoleCommand(b);
		cgs.orderPending = qfalse;
	}
}

static void
CG_SetSelectedPlayerName(void)
{
	if(cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < nsortedteamplayers){
		clientInfo_t *ci = cgs.clientinfo + sortedteamplayers[cg_currentSelectedPlayer.integer];
		if(ci){
			trap_Cvar_Set("cg_selectedPlayerName", ci->name);
			trap_Cvar_Set("cg_selectedPlayer", va("%d", sortedteamplayers[cg_currentSelectedPlayer.integer]));
			cgs.currentOrder = ci->teamtask;
		}
	}else
		trap_Cvar_Set("cg_selectedPlayerName", "Everyone");
}

int
CG_GetSelectedPlayer(void)
{
	if(cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= nsortedteamplayers)
		cg_currentSelectedPlayer.integer = 0;
	return cg_currentSelectedPlayer.integer;
}

void
CG_SelectNextPlayer(void)
{
	CG_CheckOrderPending();
	if(cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < nsortedteamplayers)
		cg_currentSelectedPlayer.integer++;
	else
		cg_currentSelectedPlayer.integer = 0;
	CG_SetSelectedPlayerName();
}

void
CG_SelectPrevPlayer(void)
{
	CG_CheckOrderPending();
	if(cg_currentSelectedPlayer.integer > 0 && cg_currentSelectedPlayer.integer < nsortedteamplayers)
		cg_currentSelectedPlayer.integer--;
	else
		cg_currentSelectedPlayer.integer = nsortedteamplayers;
	CG_SetSelectedPlayerName();
}

static void
CG_DrawPlayerArmorIcon(rectDef_t *rect, qboolean draw2D)
{
	vec3_t angles;
	vec3_t origin;

	if(cg_drawStatus.integer == 0)
		return;

	if(draw2D || (!cg_draw3dIcons.integer && cg_drawIcons.integer))
		drawpic(rect->x, rect->y + rect->h/2 + 1, rect->w, rect->h, cgs.media.armorIcon);
	else if(cg_draw3dIcons.integer){
		vecclear(angles);
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0f;
		drawmodel(rect->x, rect->y, rect->w, rect->h, cgs.media.armorModel, 0, origin, angles);
	}
}

static void
CG_DrawPlayerArmorValue(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	char num[16];
	int value;
	playerState_t *ps;

	ps = &cg.snap->ps;

	value = ps->stats[STAT_ARMOR];

	if(shader){
		trap_R_SetColor(color);
		drawpic(rect->x, rect->y, rect->w, rect->h, shader);
		trap_R_SetColor(nil);
	}else{
		Com_sprintf(num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}

#ifndef MISSIONPACK
static float healthColors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
	{1.0f, 0.69f, 0.0f, 1.0f},	// normal
	{1.0f, 0.2f, 0.2f, 1.0f},	// low health
	{0.5f, 0.5f, 0.5f, 1.0f},	// weapon firing
	{1.0f, 1.0f, 1.0f, 1.0f}
};					// health > 100
#endif

static void
CG_DrawPlayerAmmoIcon(rectDef_t *rect, qboolean draw2D)
{
	centity_t *cent;
	vec3_t angles;
	vec3_t origin;

	cent = &cg_entities[cg.snap->ps.clientNum];

	if(draw2D || (!cg_draw3dIcons.integer && cg_drawIcons.integer)){
		qhandle_t icon;
		icon = cg_weapons[cg.pps.weapon].ammoicon;
		if(icon)
			drawpic(rect->x, rect->y, rect->w, rect->h, icon);
	}else if(cg_draw3dIcons.integer)
		if(cent->currstate.weapon && cg_weapons[cent->currstate.weapon].ammomodel){
			vecclear(angles);
			origin[0] = 70;
			origin[1] = 0;
			origin[2] = 0;
			angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
			drawmodel(rect->x, rect->y, rect->w, rect->h, cg_weapons[cent->currstate.weapon].ammomodel, 0, origin, angles);
		}
}

static void
CG_DrawPlayerAmmoValue(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	char num[16];
	int value;
	centity_t *cent;
	playerState_t *ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	if(cent->currstate.weapon){
		value = ps->ammo[cent->currstate.weapon];
		if(value > -1){
			if(shader){
				trap_R_SetColor(color);
				drawpic(rect->x, rect->y, rect->w, rect->h, shader);
				trap_R_SetColor(nil);
			}else{
				Com_sprintf(num, sizeof(num), "%i", value);
				value = CG_Text_Width(num, scale, 0);
				CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
			}
		}
	}
}

static void
CG_DrawPlayerHead(rectDef_t *rect, qboolean draw2D)
{
	vec3_t angles;
	float size, stretch;
	float frac;
	float x = rect->x;

	vecclear(angles);

	if(cg.dmgtime && cg.time - cg.dmgtime < DAMAGE_TIME){
		frac = (float)(cg.time - cg.dmgtime) / DAMAGE_TIME;
		size = rect->w * 1.25 * (1.5 - frac * 0.5);

		stretch = size - rect->w * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.dmgx * stretch * 0.5;

		cg.headStartYaw = 180 + cg.dmgx * 45;

		cg.headEndYaw = 180 + 20 * cos(crandom()*M_PI);
		cg.headEndPitch = 5 * cos(crandom()*M_PI);

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	}else  if(cg.time >= cg.headEndTime){
		// select a new head angle
		cg.headStartYaw = cg.headEndYaw;
		cg.headStartPitch = cg.headEndPitch;
		cg.headStartTime = cg.headEndTime;
		cg.headEndTime = cg.time + 100 + random() * 2000;

		cg.headEndYaw = 180 + 20 * cos(crandom()*M_PI);
		cg.headEndPitch = 5 * cos(crandom()*M_PI);
	}

	// if the server was frozen for a while we may have a bad head start time
	if(cg.headStartTime > cg.time)
		cg.headStartTime = cg.time;

	frac = (cg.time - cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
	frac = frac * frac * (3 - 2 * frac);
	angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
	angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

	drawhead(x, rect->y, rect->w, rect->h, cg.snap->ps.clientNum, angles);
}

static void
CG_DrawSelectedPlayerHealth(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	clientInfo_t *ci;
	int value;
	char num[16];

	ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci){
		if(shader){
			trap_R_SetColor(color);
			drawpic(rect->x, rect->y, rect->w, rect->h, shader);
			trap_R_SetColor(nil);
		}else{
			Com_sprintf(num, sizeof(num), "%i", ci->health);
			value = CG_Text_Width(num, scale, 0);
			CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
		}
	}
}

static void
CG_DrawSelectedPlayerArmor(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	clientInfo_t *ci;
	int value;
	char num[16];
	ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci)
		if(ci->armor > 0){
			if(shader){
				trap_R_SetColor(color);
				drawpic(rect->x, rect->y, rect->w, rect->h, shader);
				trap_R_SetColor(nil);
			}else{
				Com_sprintf(num, sizeof(num), "%i", ci->armor);
				value = CG_Text_Width(num, scale, 0);
				CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
			}
		}
}

qhandle_t
CG_StatusHandle(int task)
{
	qhandle_t h;
	switch(task){
	case TEAMTASK_OFFENSE:
		h = cgs.media.assaultShader;
		break;
	case TEAMTASK_DEFENSE:
		h = cgs.media.defendShader;
		break;
	case TEAMTASK_PATROL:
		h = cgs.media.patrolShader;
		break;
	case TEAMTASK_FOLLOW:
		h = cgs.media.followShader;
		break;
	case TEAMTASK_CAMP:
		h = cgs.media.campShader;
		break;
	case TEAMTASK_RETRIEVE:
		h = cgs.media.retrieveShader;
		break;
	case TEAMTASK_ESCORT:
		h = cgs.media.escortShader;
		break;
	default:
		h = cgs.media.assaultShader;
		break;
	}
	return h;
}

static void
CG_DrawSelectedPlayerStatus(rectDef_t *rect)
{
	clientInfo_t *ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci){
		qhandle_t h;
		if(cgs.orderPending){
			// blink the icon
			if(cg.time > cgs.orderTime - 2500 && (cg.time >> 9) & 1)
				return;
			h = CG_StatusHandle(cgs.currentOrder);
		}else
			h = CG_StatusHandle(ci->teamtask);
		drawpic(rect->x, rect->y, rect->w, rect->h, h);
	}
}

static void
CG_DrawPlayerStatus(rectDef_t *rect)
{
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if(ci){
		qhandle_t h = CG_StatusHandle(ci->teamtask);
		drawpic(rect->x, rect->y, rect->w, rect->h, h);
	}
}

static void
CG_DrawSelectedPlayerName(rectDef_t *rect, float scale, vec4_t color, qboolean voice, int textStyle)
{
	clientInfo_t *ci;
	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedteamplayers[CG_GetSelectedPlayer()]);
	if(ci)
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, ci->name, 0, 0, textStyle);
}

static void
CG_DrawSelectedPlayerLocation(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	clientInfo_t *ci;
	ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci){
		const char *p = getconfigstr(CS_LOCATIONS + ci->location);
		if(!p || !*p)
			p = "unknown";
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle);
	}
}

static void
CG_DrawPlayerLocation(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if(ci){
		const char *p = getconfigstr(CS_LOCATIONS + ci->location);
		if(!p || !*p)
			p = "unknown";
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle);
	}
}

static void
CG_DrawSelectedPlayerWeapon(rectDef_t *rect)
{
	clientInfo_t *ci;

	ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci){
		if(cg_weapons[ci->currweap].icon)
			drawpic(rect->x, rect->y, rect->w, rect->h, cg_weapons[ci->currweap].icon);
		else
			drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader);
	}
}

static void
CG_DrawPlayerScore(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	char num[16];
	int value = cg.snap->ps.persistant[PERS_SCORE];

	if(shader){
		trap_R_SetColor(color);
		drawpic(rect->x, rect->y, rect->w, rect->h, shader);
		trap_R_SetColor(nil);
	}else{
		Com_sprintf(num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}

static void
CG_DrawPlayerItem(rectDef_t *rect, float scale, qboolean draw2D)
{
	int value;
	vec3_t origin, angles;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value){
		registeritemgfx(value);

		if(qtrue){
			registeritemgfx(value);
			drawpic(rect->x, rect->y, rect->w, rect->h, cg_items[value].icon);
		}else{
			vecclear(angles);
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
			drawmodel(rect->x, rect->y, rect->w, rect->h, cg_items[value].models[0], 0, origin, angles);
		}
	}
}

static void
CG_DrawSelectedPlayerPowerup(rectDef_t *rect, qboolean draw2D)
{
	clientInfo_t *ci;
	int j;
	float x, y;

	ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
	if(ci){
		x = rect->x;
		y = rect->y;

		for(j = 0; j < PW_NUM_POWERUPS; j++)
			if(ci->powerups & (1 << j)){
				gitem_t *item;
				item = finditemforpowerup(j);
				if(item){
					drawpic(x, y, rect->w, rect->h, trap_R_RegisterShader(item->icon));
					return;
				}
			}

	}
}

static void
CG_DrawSelectedPlayerHead(rectDef_t *rect, qboolean draw2D, qboolean voice)
{
	clipHandle_t cm;
	clientInfo_t *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs, angles;

	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedteamplayers[CG_GetSelectedPlayer()]);

	if(ci){
		if(cg_draw3dIcons.integer){
			cm = ci->headmodel;
			if(!cm)
				return;

			// offset the origin y and z to center the head
			trap_R_ModelBounds(cm, mins, maxs);

			origin[2] = -0.5 * (mins[2] + maxs[2]);
			origin[1] = 0.5 * (mins[1] + maxs[1]);

			// calculate distance so the head nearly fills the box
			// assume heads are taller than wide
			len = 0.7 * (maxs[2] - mins[2]);
			origin[0] = len / 0.268;	// len / tan( fov/2 )

			// allow per-model tweaking
			vecadd(origin, ci->headoffset, origin);

			angles[PITCH] = 0;
			angles[YAW] = 180;
			angles[ROLL] = 0;

			drawmodel(rect->x, rect->y, rect->w, rect->h, ci->headmodel, ci->headskin, origin, angles);
		}else if(cg_drawIcons.integer)
			drawpic(rect->x, rect->y, rect->w, rect->h, ci->modelicon);

		// if they are deferred, draw a cross out
		if(ci->deferred)
			drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader);
	}
}

static void
CG_DrawPlayerHealth(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	playerState_t *ps;
	int value;
	char num[16];

	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];

	if(shader){
		trap_R_SetColor(color);
		drawpic(rect->x, rect->y, rect->w, rect->h, shader);
		trap_R_SetColor(nil);
	}else{
		Com_sprintf(num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}

static void
CG_DrawRedScore(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	int value;
	char num[16];
	if(cgs.scores1 == SCORE_NOT_PRESENT)
		Com_sprintf(num, sizeof(num), "-");
	else
		Com_sprintf(num, sizeof(num), "%i", cgs.scores1);
	value = CG_Text_Width(num, scale, 0);
	CG_Text_Paint(rect->x + rect->w - value, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
}

static void
CG_DrawBlueScore(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	int value;
	char num[16];

	if(cgs.scores2 == SCORE_NOT_PRESENT)
		Com_sprintf(num, sizeof(num), "-");
	else
		Com_sprintf(num, sizeof(num), "%i", cgs.scores2);
	value = CG_Text_Width(num, scale, 0);
	CG_Text_Paint(rect->x + rect->w - value, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
}

// FIXME: team name support
static void
CG_DrawRedName(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_redTeamName.string, 0, 0, textStyle);
}

static void
CG_DrawBlueName(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_blueTeamName.string, 0, 0, textStyle);
}

static void
CG_DrawBlueFlagName(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	int i;
	for(i = 0; i < cgs.maxclients; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & (1<< PW_BLUEFLAG)){
			CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle);
			return;
		}
}

static void
CG_DrawBlueFlagStatus(rectDef_t *rect, qhandle_t shader)
{
	if(cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF){
		if(cgs.gametype == GT_HARVESTER){
			vec4_t color = {0, 0, 1, 1};
			trap_R_SetColor(color);
			drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.blueCubeIcon);
			trap_R_SetColor(nil);
		}
		return;
	}
	if(shader)
		drawpic(rect->x, rect->y, rect->w, rect->h, shader);
	else{
		gitem_t *item = finditemforpowerup(PW_BLUEFLAG);
		if(item){
			vec4_t color = {0, 0, 1, 1};
			trap_R_SetColor(color);
			if(cgs.blueflag >= 0 && cgs.blueflag <= 2)
				drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.blueflag]);
			else
				drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0]);
			trap_R_SetColor(nil);
		}
	}
}

static void
CG_DrawBlueFlagHead(rectDef_t *rect)
{
	int i;
	for(i = 0; i < cgs.maxclients; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & (1<< PW_BLUEFLAG)){
			vec3_t angles;
			vecclear(angles);
			angles[YAW] = 180 + 20 * sin(cg.time / 650.0);;
			drawhead(rect->x, rect->y, rect->w, rect->h, 0, angles);
			return;
		}
}

static void
CG_DrawRedFlagName(rectDef_t *rect, float scale, vec4_t color, int textStyle)
{
	int i;
	for(i = 0; i < cgs.maxclients; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & (1<< PW_REDFLAG)){
			CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle);
			return;
		}
}

static void
CG_DrawRedFlagStatus(rectDef_t *rect, qhandle_t shader)
{
	if(cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF){
		if(cgs.gametype == GT_HARVESTER){
			vec4_t color = {1, 0, 0, 1};
			trap_R_SetColor(color);
			drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.redCubeIcon);
			trap_R_SetColor(nil);
		}
		return;
	}
	if(shader)
		drawpic(rect->x, rect->y, rect->w, rect->h, shader);
	else{
		gitem_t *item = finditemforpowerup(PW_REDFLAG);
		if(item){
			vec4_t color = {1, 0, 0, 1};
			trap_R_SetColor(color);
			if(cgs.redflag >= 0 && cgs.redflag <= 2)
				drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.redflag]);
			else
				drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0]);
			trap_R_SetColor(nil);
		}
	}
}

static void
CG_DrawRedFlagHead(rectDef_t *rect)
{
	int i;
	for(i = 0; i < cgs.maxclients; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & (1<< PW_REDFLAG)){
			vec3_t angles;
			vecclear(angles);
			angles[YAW] = 180 + 20 * sin(cg.time / 650.0);;
			drawhead(rect->x, rect->y, rect->w, rect->h, 0, angles);
			return;
		}
}

static void
CG_HarvesterSkulls(rectDef_t *rect, float scale, vec4_t color, qboolean force2D, int textStyle)
{
	char num[16];
	vec3_t origin, angles;
	qhandle_t handle;
	int value = cg.snap->ps.generic1;

	if(cgs.gametype != GT_HARVESTER)
		return;

	if(value > 99)
		value = 99;

	Com_sprintf(num, sizeof(num), "%i", value);
	value = CG_Text_Width(num, scale, 0);
	CG_Text_Paint(rect->x + (rect->w - value), rect->y + rect->h, scale, color, num, 0, 0, textStyle);

	if(cg_drawIcons.integer){
		if(!force2D && cg_draw3dIcons.integer){
			vecclear(angles);
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				handle = cgs.media.redCubeModel;
			else
				handle = cgs.media.blueCubeModel;
			drawmodel(rect->x, rect->y, 35, 35, handle, 0, origin, angles);
		}else{
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				handle = cgs.media.redCubeIcon;
			else
				handle = cgs.media.blueCubeIcon;
			drawpic(rect->x + 3, rect->y + 16, 20, 20, handle);
		}
	}
}

static void
CG_OneFlagStatus(rectDef_t *rect)
{
	if(cgs.gametype != GT_1FCTF)
		return;
	else{
		gitem_t *item = finditemforpowerup(PW_NEUTRALFLAG);
		if(item)
			if(cgs.flagStatus >= 0 && cgs.flagStatus <= 4){
				vec4_t color = {1, 1, 1, 1};
				int index = 0;
				if(cgs.flagStatus == FLAG_TAKEN_RED){
					color[1] = color[2] = 0;
					index = 1;
				}else if(cgs.flagStatus == FLAG_TAKEN_BLUE){
					color[0] = color[1] = 0;
					index = 1;
				}else if(cgs.flagStatus == FLAG_DROPPED)
					index = 2;
				trap_R_SetColor(color);
				drawpic(rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[index]);
			}
	}
}

static void
CG_DrawCTFPowerUp(rectDef_t *rect)
{
	int value;

	if(cgs.gametype < GT_CTF)
		return;
	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if(value){
		registeritemgfx(value);
		drawpic(rect->x, rect->y, rect->w, rect->h, cg_items[value].icon);
	}
}

static void
CG_DrawTeamColor(rectDef_t *rect, vec4_t color)
{
	drawteambg(rect->x, rect->y, rect->w, rect->h, color[3], cg.snap->ps.persistant[PERS_TEAM]);
}

static void
CG_DrawAreaPowerUp(rectDef_t *rect, int align, float special, float scale, vec4_t color)
{
	char num[16];
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t *ps;
	int t;
	gitem_t *item;
	float f;
	rectDef_t r2;
	float *inc;
	r2.x = rect->x;
	r2.y = rect->y;
	r2.w = rect->w;
	r2.h = rect->h;

	inc = (align == HUD_VERTICAL) ? &r2.y : &r2.x;

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0)
		return;

	// sort the list by time remaining
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++){
		if(!ps->powerups[i])
			continue;
		t = ps->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if(t <= 0 || t >= 999000)
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
	for(i = 0; i < active; i++){
		item = finditemforpowerup(sorted[i]);

		if(item){
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

			drawpic(r2.x, r2.y, r2.w * .75, r2.h, trap_R_RegisterShader(item->icon));

			Com_sprintf(num, sizeof(num), "%i", sortedTime[i] / 1000);
			CG_Text_Paint(r2.x + (r2.w * .75) + 3, r2.y + r2.h, scale, color, num, 0, 0, 0);
			*inc += r2.w + special;
		}
	}
	trap_R_SetColor(nil);
}

float
CG_GetValue(int ownerDraw)
{
	centity_t *cent;
	clientInfo_t *ci;
	playerState_t *ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	switch(ownerDraw){
	case CG_SELECTEDPLAYER_ARMOR:
		ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
		return ci->armor;
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		ci = cgs.clientinfo + sortedteamplayers[CG_GetSelectedPlayer()];
		return ci->health;
		break;
	case CG_PLAYER_ARMOR_VALUE:
		return ps->stats[STAT_ARMOR];
		break;
	case CG_PLAYER_AMMO_VALUE:
		if(cent->currstate.weapon)
			return ps->ammo[cent->currstate.weapon];
		break;
	case CG_PLAYER_SCORE:
		return cg.snap->ps.persistant[PERS_SCORE];
		break;
	case CG_PLAYER_HEALTH:
		return ps->stats[STAT_HEALTH];
		break;
	case CG_RED_SCORE:
		return cgs.scores1;
		break;
	case CG_BLUE_SCORE:
		return cgs.scores2;
		break;
	default:
		break;
	}
	return -1;
}

qboolean
CG_OtherTeamHasFlag(void)
{
	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF){
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if(cgs.gametype == GT_1FCTF){
			if(team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_BLUE)
				return qtrue;
			else if(team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_RED)
				return qtrue;
			else
				return qfalse;
		}else{
			if(team == TEAM_RED && cgs.redflag == FLAG_TAKEN)
				return qtrue;
			else if(team == TEAM_BLUE && cgs.blueflag == FLAG_TAKEN)
				return qtrue;
			else
				return qfalse;
		}
	}
	return qfalse;
}

qboolean
CG_YourTeamHasFlag(void)
{
	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF){
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if(cgs.gametype == GT_1FCTF){
			if(team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_RED)
				return qtrue;
			else if(team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_BLUE)
				return qtrue;
			else
				return qfalse;
		}else{
			if(team == TEAM_RED && cgs.blueflag == FLAG_TAKEN)
				return qtrue;
			else if(team == TEAM_BLUE && cgs.redflag == FLAG_TAKEN)
				return qtrue;
			else
				return qfalse;
		}
	}
	return qfalse;
}

// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean
ownerdrawvisible(int flags)
{
	if(flags & CG_SHOW_TEAMINFO)
		return cg_currentSelectedPlayer.integer == nsortedteamplayers;

	if(flags & CG_SHOW_NOTEAMINFO)
		return !(cg_currentSelectedPlayer.integer == nsortedteamplayers);

	if(flags & CG_SHOW_OTHERTEAMHASFLAG)
		return CG_OtherTeamHasFlag();

	if(flags & CG_SHOW_YOURTEAMHASENEMYFLAG)
		return CG_YourTeamHasFlag();

	if(flags & (CG_SHOW_BLUE_TEAM_HAS_REDFLAG | CG_SHOW_RED_TEAM_HAS_BLUEFLAG)){
		if(flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG && (cgs.redflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_RED))
			return qtrue;
		else if(flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG && (cgs.blueflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_BLUE))
			return qtrue;
		return qfalse;
	}

	if(flags & CG_SHOW_ANYTEAMGAME)
		if(cgs.gametype >= GT_TEAM)
			return qtrue;

	if(flags & CG_SHOW_ANYNONTEAMGAME)
		if(cgs.gametype < GT_TEAM)
			return qtrue;

	if(flags & CG_SHOW_HARVESTER){
		if(cgs.gametype == GT_HARVESTER)
			return qtrue;
		else
			return qfalse;
	}

	if(flags & CG_SHOW_ONEFLAG){
		if(cgs.gametype == GT_1FCTF)
			return qtrue;
		else
			return qfalse;
	}

	if(flags & CG_SHOW_CTF)
		if(cgs.gametype == GT_CTF)
			return qtrue;

	if(flags & CG_SHOW_OBELISK){
		if(cgs.gametype == GT_OBELISK)
			return qtrue;
		else
			return qfalse;
	}

	if(flags & CG_SHOW_HEALTHCRITICAL)
		if(cg.snap->ps.stats[STAT_HEALTH] < 25)
			return qtrue;

	if(flags & CG_SHOW_HEALTHOK)
		if(cg.snap->ps.stats[STAT_HEALTH] >= 25)
			return qtrue;

	if(flags & CG_SHOW_SINGLEPLAYER)
		if(cgs.gametype == GT_SINGLE_PLAYER)
			return qtrue;

	if(flags & CG_SHOW_TOURNAMENT)
		if(cgs.gametype == GT_TOURNAMENT)
			return qtrue;

	if(flags & CG_SHOW_DURINGINCOMINGVOICE){
	}

	if(flags & CG_SHOW_IF_PLAYER_HAS_FLAG)
		if(cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG])
			return qtrue;
	return qfalse;
}

static void
CG_DrawPlayerHasFlag(rectDef_t *rect, qboolean force2D)
{
	int adj = (force2D) ? 0 : 2;
	if(cg.pps.powerups[PW_REDFLAG])
		drawflag(rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_RED, force2D);
	else if(cg.pps.powerups[PW_BLUEFLAG])
		drawflag(rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_BLUE, force2D);
	else if(cg.pps.powerups[PW_NEUTRALFLAG])
		drawflag(rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_FREE, force2D);
}

static void
CG_DrawAreaSystemChat(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, syschat, 0, 0, 0);
}

static void
CG_DrawAreaTeamChat(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, teamchat1, 0, 0, 0);
}

static void
CG_DrawAreaChat(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, teamchat2, 0, 0, 0);
}

const char *
CG_GetKillerText(void)
{
	const char *s = "";
	if(cg.killername[0])
		s = va("Fragged by %s", cg.killername);
	return s;
}

static void
CG_DrawKiller(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	// fragged by ... line
	if(cg.killername[0]){
		int x = rect->x + rect->w / 2;
		CG_Text_Paint(x - CG_Text_Width(CG_GetKillerText(), scale, 0) / 2, rect->y + rect->h, scale, color, CG_GetKillerText(), 0, 0, textStyle);
	}
}

static void
CG_DrawCapFragLimit(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	int limit = (cgs.gametype >= GT_CTF) ? cgs.capturelimit : cgs.fraglimit;
	CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", limit), 0, 0, textStyle);
}

static void
CG_Draw1stPlace(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	if(cgs.scores1 != SCORE_NOT_PRESENT)
		CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", cgs.scores1), 0, 0, textStyle);
}

static void
CG_Draw2ndPlace(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	if(cgs.scores2 != SCORE_NOT_PRESENT)
		CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", cgs.scores2), 0, 0, textStyle);
}

const char *
CG_GetGameStatusText(void)
{
	const char *s = "";
	if(cgs.gametype < GT_TEAM){
		if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
			s = va("%s place with %i", placestr(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
	}else{
		if(cg.teamscores[0] == cg.teamscores[1])
			s = va("Teams are tied at %i", cg.teamscores[0]);
		else if(cg.teamscores[0] >= cg.teamscores[1])
			s = va("Red leads Blue, %i to %i", cg.teamscores[0], cg.teamscores[1]);
		else
			s = va("Blue leads Red, %i to %i", cg.teamscores[1], cg.teamscores[0]);
	}
	return s;
}

static void
CG_DrawGameStatus(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, CG_GetGameStatusText(), 0, 0, textStyle);
}

const char *
CG_GameTypeString(void)
{
	if(cgs.gametype == GT_FFA)
		return "Free For All";
	else if(cgs.gametype == GT_TEAM)
		return "Team Deathmatch";
	else if(cgs.gametype == GT_CTF)
		return "Capture the Flag";
	else if(cgs.gametype == GT_1FCTF)
		return "One Flag CTF";
	else if(cgs.gametype == GT_OBELISK)
		return "Overload";
	else if(cgs.gametype == GT_HARVESTER)
		return "Harvester";
	return "";
}

static void
CG_DrawGameType(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, CG_GameTypeString(), 0, 0, textStyle);
}

static void
CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit)
{
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	if(text){
		const char *s = text;
		float max = *maxX;
		float useScale;
		fontInfo_t *font = &cgDC.Assets.textFont;
		if(scale <= cg_smallFont.value)
			font = &cgDC.Assets.smallFont;
		else if(scale > cg_bigFont.value)
			font = &cgDC.Assets.bigFont;
		useScale = scale * font->glyphScale;
		trap_R_SetColor(color);
		len = strlen(text);
		if(limit > 0 && len > limit)
			len = limit;
		count = 0;
		while(s && *s && count < len){
			glyph = &font->glyphs[*s & 255];
			if(Q_IsColorString(s)){
				memcpy(newColor, g_color_table[ColorIndex(*(s+1))], sizeof(newColor));
				newColor[3] = color[3];
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			}else{
				float yadj = useScale * glyph->top;
				if(CG_Text_Width(s, scale, 1) + x > max){
					*maxX = 0;
					break;
				}
				CG_Text_PaintChar(x, y - yadj,
						  glyph->imageWidth,
						  glyph->imageHeight,
						  useScale,
						  glyph->s,
						  glyph->t,
						  glyph->s2,
						  glyph->t2,
						  glyph->glyph);
				x += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap_R_SetColor(nil);
	}
}

#define PIC_WIDTH 12

void
CG_DrawNewTeamInfo(rectDef_t *rect, float text_x, float text_y, float scale, vec4_t color, qhandle_t shader)
{
	int xx;
	float y;
	int i, j, len, count;
	const char *p;
	vec4_t hcolor;
	float pwidth, lwidth, maxx, leftOver;
	clientInfo_t *ci;
	gitem_t *item;
	qhandle_t h;

	// max player name width
	pwidth = 0;
	count = (nsortedteamplayers > 8) ? 8 : nsortedteamplayers;
	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedteamplayers[i];
		if(ci->infovalid && ci->team == cg.snap->ps.persistant[PERS_TEAM]){
			len = CG_Text_Width(ci->name, scale, 0);
			if(len > pwidth)
				pwidth = len;
		}
	}

	// max location name width
	lwidth = 0;
	for(i = 1; i < MAX_LOCATIONS; i++){
		p = getconfigstr(CS_LOCATIONS + i);
		if(p && *p){
			len = CG_Text_Width(p, scale, 0);
			if(len > lwidth)
				lwidth = len;
		}
	}

	y = rect->y;

	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedteamplayers[i];
		if(ci->infovalid && ci->team == cg.snap->ps.persistant[PERS_TEAM]){
			xx = rect->x + 1;
			for(j = 0; j <= PW_NUM_POWERUPS; j++)
				if(ci->powerups & (1 << j)){
					item = finditemforpowerup(j);

					if(item){
						drawpic(xx, y, PIC_WIDTH, PIC_WIDTH, trap_R_RegisterShader(item->icon));
						xx += PIC_WIDTH;
					}
				}

			// FIXME: max of 3 powerups shown properly
			xx = rect->x + (PIC_WIDTH * 3) + 2;

			getcolorforhealth(ci->health, ci->armor, hcolor);
			trap_R_SetColor(hcolor);
			drawpic(xx, y + 1, PIC_WIDTH - 2, PIC_WIDTH - 2, cgs.media.heartShader);

			//Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
			//CG_Text_Paint(xx, y + text_y, scale, hcolor, st, 0, 0);

			// draw weapon icon
			xx += PIC_WIDTH + 1;

// weapon used is not that useful, use the space for task
#if 0
			if(cg_weapons[ci->currweap].icon)
				drawpic(xx, y, PIC_WIDTH, PIC_WIDTH, cg_weapons[ci->currweap].icon);
			else
				drawpic(xx, y, PIC_WIDTH, PIC_WIDTH, cgs.media.deferShader);

#endif

			trap_R_SetColor(nil);
			if(cgs.orderPending){
				// blink the icon
				if(cg.time > cgs.orderTime - 2500 && (cg.time >> 9) & 1)
					h = 0;
				else
					h = CG_StatusHandle(cgs.currentOrder);
			}else
				h = CG_StatusHandle(ci->teamtask);

			if(h)
				drawpic(xx, y, PIC_WIDTH, PIC_WIDTH, h);

			xx += PIC_WIDTH + 1;

			leftOver = rect->w - xx;
			maxx = xx + leftOver / 3;

			CG_Text_Paint_Limit(&maxx, xx, y + text_y, scale, color, ci->name, 0, 0);

			p = getconfigstr(CS_LOCATIONS + ci->location);
			if(!p || !*p)
				p = "unknown";

			xx += leftOver / 3 + 2;
			maxx = rect->w - 4;

			CG_Text_Paint_Limit(&maxx, xx, y + text_y, scale, color, p, 0, 0);
			y += text_y + 2;
			if(y + text_y + 2 > rect->y + rect->h)
				break;

		}
	}
}

void
CG_DrawTeamSpectators(rectDef_t *rect, float scale, vec4_t color, qhandle_t shader)
{
	if(cg.speclen){
		float maxX;

		if(cg.specwidth == -1){
			cg.specwidth = 0;
			cg.specpaintx = rect->x + 1;
			cg.specpaintx2 = -1;
		}

		if(cg.specoffset > cg.speclen){
			cg.specoffset = 0;
			cg.specpaintx = rect->x + 1;
			cg.specpaintx2 = -1;
		}

		if(cg.time > cg.spectime){
			cg.spectime = cg.time + 10;
			if(cg.specpaintx <= rect->x + 2){
				if(cg.specoffset < cg.speclen){
					cg.specpaintx += CG_Text_Width(&cg.speclist[cg.specoffset], scale, 1) - 1;
					cg.specoffset++;
				}else{
					cg.specoffset = 0;
					if(cg.specpaintx2 >= 0)
						cg.specpaintx = cg.specpaintx2;
					else
						cg.specpaintx = rect->x + rect->w - 2;
					cg.specpaintx2 = -1;
				}
			}else{
				cg.specpaintx--;
				if(cg.specpaintx2 >= 0)
					cg.specpaintx2--;
			}
		}

		maxX = rect->x + rect->w - 2;
		CG_Text_Paint_Limit(&maxX, cg.specpaintx, rect->y + rect->h - 3, scale, color, &cg.speclist[cg.specoffset], 0, 0);
		if(cg.specpaintx2 >= 0){
			float maxX2 = rect->x + rect->w - 2;
			CG_Text_Paint_Limit(&maxX2, cg.specpaintx2, rect->y + rect->h - 3, scale, color, cg.speclist, 0, cg.specoffset);
		}
		if(cg.specoffset && maxX > 0){
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if(cg.specpaintx2 == -1)
				cg.specpaintx2 = rect->x + rect->w - 2;
		}else
			cg.specpaintx2 = -1;

	}
}

void
CG_DrawMedal(int ownerDraw, rectDef_t *rect, float scale, vec4_t color, qhandle_t shader)
{
	score_t *score = &cg.scores[cg.selscore];
	float value = 0;
	char *text = nil;
	color[3] = 0.25;

	switch(ownerDraw){
	case CG_ACCURACY:
		value = score->accuracy;
		break;
	case CG_ASSISTS:
		value = score->nassist;
		break;
	case CG_DEFEND:
		value = score->ndefend;
		break;
	case CG_EXCELLENT:
		value = score->nexcellent;
		break;
	case CG_IMPRESSIVE:
		value = score->nimpressive;
		break;
	case CG_PERFECT:
		value = score->perfect;
		break;
	case CG_GAUNTLET:
		value = score->gauntletcount;
		break;
	case CG_CAPTURES:
		value = score->captures;
		break;
	}

	if(value > 0){
		if(ownerDraw != CG_PERFECT){
			if(ownerDraw == CG_ACCURACY){
				text = va("%i%%", (int)value);
				if(value > 50)
					color[3] = 1.0;
			}else{
				text = va("%i", (int)value);
				color[3] = 1.0;
			}
		}else{
			if(value)
				color[3] = 1.0;
			text = "Wow";
		}
	}

	trap_R_SetColor(color);
	drawpic(rect->x, rect->y, rect->w, rect->h, shader);

	if(text){
		color[3] = 1.0;
		value = CG_Text_Width(text, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h + 10, scale, color, text, 0, 0, 0);
	}
	trap_R_SetColor(nil);
}

//
void
ownerdraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle)
{
	rectDef_t rect;

	if(cg_drawStatus.integer == 0)
		return;

	//if (ownerDrawFlags != 0 && !ownerdrawvisible(ownerDrawFlags)) {
	//	return;
	//}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch(ownerDraw){
	case CG_PLAYER_ARMOR_ICON:
		CG_DrawPlayerArmorIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_ARMOR_ICON2D:
		CG_DrawPlayerArmorIcon(&rect, qtrue);
		break;
	case CG_PLAYER_ARMOR_VALUE:
		CG_DrawPlayerArmorValue(&rect, scale, color, shader, textStyle);
		break;
	case CG_PLAYER_AMMO_ICON:
		CG_DrawPlayerAmmoIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_AMMO_ICON2D:
		CG_DrawPlayerAmmoIcon(&rect, qtrue);
		break;
	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_HEAD:
		CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse);
		break;
	case CG_VOICE_HEAD:
		CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue);
		break;
	case CG_VOICE_NAME:
		CG_DrawSelectedPlayerName(&rect, scale, color, qtrue, textStyle);
		break;
	case CG_SELECTEDPLAYER_STATUS:
		CG_DrawSelectedPlayerStatus(&rect);
		break;
	case CG_SELECTEDPLAYER_ARMOR:
		CG_DrawSelectedPlayerArmor(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		CG_DrawSelectedPlayerHealth(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_NAME:
		CG_DrawSelectedPlayerName(&rect, scale, color, qfalse, textStyle);
		break;
	case CG_SELECTEDPLAYER_LOCATION:
		CG_DrawSelectedPlayerLocation(&rect, scale, color, textStyle);
		break;
	case CG_SELECTEDPLAYER_WEAPON:
		CG_DrawSelectedPlayerWeapon(&rect);
		break;
	case CG_SELECTEDPLAYER_POWERUP:
		CG_DrawSelectedPlayerPowerup(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_HEAD:
		CG_DrawPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_ITEM:
		CG_DrawPlayerItem(&rect, scale, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_SCORE:
		CG_DrawPlayerScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth(&rect, scale, color, shader, textStyle);
		break;
	case CG_RED_SCORE:
		CG_DrawRedScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_BLUE_SCORE:
		CG_DrawBlueScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_RED_NAME:
		CG_DrawRedName(&rect, scale, color, textStyle);
		break;
	case CG_BLUE_NAME:
		CG_DrawBlueName(&rect, scale, color, textStyle);
		break;
	case CG_BLUE_FLAGHEAD:
		CG_DrawBlueFlagHead(&rect);
		break;
	case CG_BLUE_FLAGSTATUS:
		CG_DrawBlueFlagStatus(&rect, shader);
		break;
	case CG_BLUE_FLAGNAME:
		CG_DrawBlueFlagName(&rect, scale, color, textStyle);
		break;
	case CG_RED_FLAGHEAD:
		CG_DrawRedFlagHead(&rect);
		break;
	case CG_RED_FLAGSTATUS:
		CG_DrawRedFlagStatus(&rect, shader);
		break;
	case CG_RED_FLAGNAME:
		CG_DrawRedFlagName(&rect, scale, color, textStyle);
		break;
	case CG_HARVESTER_SKULLS:
		CG_HarvesterSkulls(&rect, scale, color, qfalse, textStyle);
		break;
	case CG_HARVESTER_SKULLS2D:
		CG_HarvesterSkulls(&rect, scale, color, qtrue, textStyle);
		break;
	case CG_ONEFLAG_STATUS:
		CG_OneFlagStatus(&rect);
		break;
	case CG_PLAYER_LOCATION:
		CG_DrawPlayerLocation(&rect, scale, color, textStyle);
		break;
	case CG_TEAM_COLOR:
		CG_DrawTeamColor(&rect, color);
		break;
	case CG_CTF_POWERUP:
		CG_DrawCTFPowerUp(&rect);
		break;
	case CG_AREA_POWERUP:
		CG_DrawAreaPowerUp(&rect, align, special, scale, color);
		break;
	case CG_PLAYER_STATUS:
		CG_DrawPlayerStatus(&rect);
		break;
	case CG_PLAYER_HASFLAG:
		CG_DrawPlayerHasFlag(&rect, qfalse);
		break;
	case CG_PLAYER_HASFLAG2D:
		CG_DrawPlayerHasFlag(&rect, qtrue);
		break;
	case CG_AREA_SYSTEMCHAT:
		CG_DrawAreaSystemChat(&rect, scale, color, shader);
		break;
	case CG_AREA_TEAMCHAT:
		CG_DrawAreaTeamChat(&rect, scale, color, shader);
		break;
	case CG_AREA_CHAT:
		CG_DrawAreaChat(&rect, scale, color, shader);
		break;
	case CG_GAME_TYPE:
		CG_DrawGameType(&rect, scale, color, shader, textStyle);
		break;
	case CG_GAME_STATUS:
		CG_DrawGameStatus(&rect, scale, color, shader, textStyle);
		break;
	case CG_KILLER:
		CG_DrawKiller(&rect, scale, color, shader, textStyle);
		break;
	case CG_ACCURACY:
	case CG_ASSISTS:
	case CG_DEFEND:
	case CG_EXCELLENT:
	case CG_IMPRESSIVE:
	case CG_PERFECT:
	case CG_GAUNTLET:
	case CG_CAPTURES:
		CG_DrawMedal(ownerDraw, &rect, scale, color, shader);
		break;
	case CG_SPECTATORS:
		CG_DrawTeamSpectators(&rect, scale, color, shader);
		break;
	case CG_TEAMINFO:
		if(cg_currentSelectedPlayer.integer == nsortedteamplayers)
			CG_DrawNewTeamInfo(&rect, text_x, text_y, scale, color, shader);
		break;
	case CG_CAPFRAGLIMIT:
		CG_DrawCapFragLimit(&rect, scale, color, shader, textStyle);
		break;
	case CG_1STPLACE:
		CG_Draw1stPlace(&rect, scale, color, shader, textStyle);
		break;
	case CG_2NDPLACE:
		CG_Draw2ndPlace(&rect, scale, color, shader, textStyle);
		break;
	default:
		break;
	}
}

void
mouseevent(int x, int y)
{
	int n;

	if((cg.pps.pm_type == PM_NORMAL || cg.pps.pm_type == PM_SPECTATOR) && cg.showscores == qfalse){
		trap_Key_SetCatcher(0);
		return;
	}

	cgs.cursorx += x;
	if(cgs.cursorx < 0)
		cgs.cursorx = 0;
	else if(cgs.cursorx > 640)
		cgs.cursorx = 640;

	cgs.cursory += y;
	if(cgs.cursory < 0)
		cgs.cursory = 0;
	else if(cgs.cursory > 480)
		cgs.cursory = 480;

	n = Display_CursorType(cgs.cursorx, cgs.cursory);
	cgs.activecursor = 0;
	if(n == CURSOR_ARROW)
		cgs.activecursor = cgs.media.selectCursor;
	else if(n == CURSOR_SIZER)
		cgs.activecursor = cgs.media.sizeCursor;

	if(cgs.captureditem)
		Display_MouseMove(cgs.captureditem, x, y);
	else
		Display_MouseMove(nil, cgs.cursorx, cgs.cursory);

}

/*
==================
CG_HideTeamMenus
==================

*/
void
CG_HideTeamMenu(void)
{
	Menus_CloseByName("teamMenu");
	Menus_CloseByName("getMenu");
}

/*
==================
CG_ShowTeamMenus
==================

*/
void
CG_ShowTeamMenu(void)
{
	Menus_OpenByName("teamMenu");
}

/*
==================
eventhandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void
eventhandling(int type)
{
	cgs.evhandling = type;
	if(type == CGAME_EVENT_NONE)
		CG_HideTeamMenu();
	else if(type == CGAME_EVENT_TEAMMENU){
		//CG_ShowTeamMenu();
	}else if(type == CGAME_EVENT_SCOREBOARD){
	}
}

void
keyevent(int key, qboolean down)
{
	if(!down)
		return;

	if(cg.pps.pm_type == PM_NORMAL || (cg.pps.pm_type == PM_SPECTATOR && cg.showscores == qfalse)){
		eventhandling(CGAME_EVENT_NONE);
		trap_Key_SetCatcher(0);
		return;
	}

	//if (key == trap_Key_GetKey("teamMenu") || !Display_CaptureItem(cgs.cursorx, cgs.cursory)) {
	// if we see this then we should always be visible
	//  eventhandling(CGAME_EVENT_NONE);
	//  trap_Key_SetCatcher(0);
	//}

	Display_HandleKey(key, down, cgs.cursorx, cgs.cursory);

	if(cgs.captureditem)
		cgs.captureditem = nil;
	else if(key == K_MOUSE2 && down)
		cgs.captureditem = Display_CaptureItem(cgs.cursorx, cgs.cursory);

}

int
CG_ClientNumFromName(const char *p)
{
	int i;
	for(i = 0; i < cgs.maxclients; i++)
		if(cgs.clientinfo[i].infovalid && Q_stricmp(cgs.clientinfo[i].name, p) == 0)
			return i;
	return -1;
}

void
CG_ShowResponseHead(void)
{
	Menus_OpenByName("voiceMenu");
	trap_Cvar_Set("cl_conXOffset", "72");
	cg.voicetime = cg.time;
}

void
CG_RunMenuScript(char **args)
{
}

void
CG_GetTeamColor(vec4_t *color)
{
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
		(*color)[0] = 1.0f;
		(*color)[3] = 0.25f;
		(*color)[1] = (*color)[2] = 0.0f;
	}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
		(*color)[0] = (*color)[1] = 0.0f;
		(*color)[2] = 1.0f;
		(*color)[3] = 0.25f;
	}else{
		(*color)[0] = (*color)[2] = 0.0f;
		(*color)[1] = 0.17f;
		(*color)[3] = 0.25f;
	}
}
