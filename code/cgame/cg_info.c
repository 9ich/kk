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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

static int loadingPlayerIconCount;
static int loadingItemIconCount;
static qhandle_t loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t loadingItemIcons[MAX_LOADING_ITEM_ICONS];

static void
CG_DrawLoadingIcons(void)
{
	int n;
	int x, y;

	for(n = 0; n < loadingPlayerIconCount; n++){
		x = 16 + n * 78;
		y = 324-40;
		drawpic(x, y, 64, 64, loadingPlayerIcons[n]);
	}

	for(n = 0; n < loadingItemIconCount; n++){
		y = 400-40;
		if(n >= 13)
			y += 40;
		x = 16 + n % 13 * 48;
		drawpic(x, y, 32, 32, loadingItemIcons[n]);
	}
}

void
loadingstr(const char *s)
{
	Q_strncpyz(cg.infoscreentext, s, sizeof(cg.infoscreentext));

	trap_UpdateScreen();
}

void
loadingitem(int itemNum)
{
	gitem_t *item;

	item = &bg_itemlist[itemNum];

	if(item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS)
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip(item->icon);

	loadingstr(item->pickupname);
}

void
loadingclient(int clientNum)
{
	const char *info;
	char *skin;
	char personality[MAX_QPATH];
	char model[MAX_QPATH];
	char iconName[MAX_QPATH];

	info = getconfigstr(CS_PLAYERS + clientNum);

	if(loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS){
		Q_strncpyz(model, Info_ValueForKey(info, "model"), sizeof(model));
		skin = strrchr(model, '/');
		if(skin)
			*skin++ = '\0';
		else
			skin = "default";

		Com_sprintf(iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin);

		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		if(!loadingPlayerIcons[loadingPlayerIconCount]){
			Com_sprintf(iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin);
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		}
		if(!loadingPlayerIcons[loadingPlayerIconCount]){
			Com_sprintf(iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default");
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		}
		if(loadingPlayerIcons[loadingPlayerIconCount])
			loadingPlayerIconCount++;
	}

	Q_strncpyz(personality, Info_ValueForKey(info, "n"), sizeof(personality));
	Q_CleanStr(personality);

	if(cgs.gametype == GT_SINGLE_PLAYER)
		trap_S_RegisterSound(va("sound/player/announce/%s.wav", personality), qtrue);

	loadingstr(personality);
}

/*
Draw all the status / pacifier stuff during level loading
*/
void
drawinfo(void)
{
	const char *s;
	const char *info;
	const char *sysInfo;
	float x, y, value, size;
	int font;
	qhandle_t levelshot;
	char buf[1024];

	info = getconfigstr(CS_SERVERINFO);
	sysInfo = getconfigstr(CS_SYSTEMINFO);

	s = Info_ValueForKey(info, "mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s.tga", s));
	if(!levelshot)
		levelshot = trap_R_RegisterShaderNoMip("menu/art/unknownmap");
	trap_R_SetColor(nil);
	drawpic(0, 0, screenwidth(), screenheight(), levelshot);

	pushalign("center");

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if(cg.infoscreentext[0])
		drawstring(screenwidth()/2, 128-32, va("Loading... %s", cg.infoscreentext), FONT2, 32, CWhite);
	else
		drawstring(screenwidth()/2, 128-32, "Awaiting snapshot...", FONT2, 32, CWhite);

	// draw info string information

	x = screenwidth()/2;
	y = 180-32;
	font = FONT2;
	size = 32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if(!atoi(buf)){
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 1024);
		Q_CleanStr(buf);
		drawstring(x, y, buf, font, size, CWhite);
		y += stringheight(buf, font, size);

		// pure server
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if(s[0] == '1'){
			drawstring(x, y, "Pure Server", font, size, CWhite);
			y += stringheight("Pure Server", font, size);
		}

		// server-specific message of the day
		s = getconfigstr(CS_MOTD);
		if(s[0]){
			drawstring(x, y, s, font, size, CWhite);
			y += stringheight(s, font, size);
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = getconfigstr(CS_MESSAGE);
	if(s[0]){
		drawstring(x, y, s, font, size, CWhite);
		y += stringheight(s, font, size);
	}

	// cheats warning
	s = Info_ValueForKey(sysInfo, "sv_cheats");
	if(s[0] == '1'){
		drawstring(x, y, "CHEATS ARE ENABLED", font, size, CWhite);
		y += stringheight("CHEATS ARE ENABLED", font, size);
	}

	// game type
	switch(cgs.gametype){
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_SINGLE_PLAYER:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
	default:
		s = "Unknown Gametype";
		break;
	}
	drawstring(x, y, s, font, size, CWhite);
	y += stringheight(s, font, size);

	value = atoi(Info_ValueForKey(info, "timelimit"));
	if(value){
		s = va("timelimit %f", value);
		drawstring(x, y, s, font, size, CWhite);
		y += stringheight(s, font, size);
	}

	if(cgs.gametype < GT_CTF){
		value = atoi(Info_ValueForKey(info, "fraglimit"));
		if(value){
			s = va("fraglimit %f", value);
			drawstring(x, y, s, font, size, CWhite);
			y += stringheight(s, font, size);
		}
	}

	if(cgs.gametype >= GT_CTF){
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if(value){
			s = va("capturelimit %f", value);
			drawstring(x, y, s, font, size, CWhite);
			y += stringheight(s, font, size);
		}
	}

	popalign(1);
}
