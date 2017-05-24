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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"


int forceModelModificationCount = -1;

void	cginit(int serverMessageNum, int serverCommandSequence, int clientNum);
void	cgshutdown(void);

/*
This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
*/
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch(command){
	case CG_INIT:
		cginit(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		cgshutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return consolecmd();
	case CG_DRAW_ACTIVE_FRAME:
		drawframe(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return xhairplayer();
	case CG_LAST_ATTACKER:
		return getlastattacker();
	case CG_KEY_EVENT:
		keyevent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		mouseevent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		eventhandling(arg0);
		return 0;
	default:
		cgerrorf("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t cg_items[MAX_ITEMS];
awardlist_t cg_awardlist[] = {
	{AWARD_IMPRESSIVE,	"sound/awards/impressive.wav",	"gfx/2d/impressive",		"Accuracy"},
	{AWARD_DEFEND,		"sound/awards/defense.wav",		"gfx/2d/defense",		"Defense"},
	{AWARD_ASSIST,		"sound/awards/assist.wav",		"gfx/2d/assist",		"Assist"},
	{AWARD_DENIED,		"sound/awards/denied.wav",		0,		"Denied"},
	{AWARD_GAUNTLET,	"sound/awards/humiliation.wav",	0,		"Humiliation"},
	{AWARD_HUMILIATED,	"sound/awards/humiliation.wav",	0,		"Humiliated"},
	{AWARD_KILLINGSPREE,	"sound/awards/killingspree.wav",	0,		"Killing Spree"},
	{AWARD_DOMINATING,	"sound/awards/dominating.wav",	0,		"Dominating"},
	{AWARD_RAMPAGE,		"sound/awards/rampage.wav",		0,		"Rampage"},
	{AWARD_UNSTOPPABLE,	"sound/awards/unstoppable.wav",	0,		"Unstoppable"},
	{AWARD_GODLIKE,		"sound/awards/godlike.wav",		0,		"Godlike"},
	{AWARD_WICKEDSICK,	"sound/awards/wickedsick.wav",	0,		"Wicked Sick!"},
	{AWARD_DOUBLEKILL,	"sound/awards/doublekill.wav",	0,		"Double Kill"},
	{AWARD_MULTIKILL,	"sound/awards/multikill.wav",	0,		"Multi Kill"},
	{AWARD_MEGAKILL,	"sound/awards/megakill.wav",	0,		"Mega Kill"},
	{AWARD_ULTRAKILL,	"sound/awards/ultrakill.wav",	0,		"Ultra Kill!"},
	{AWARD_MONSTERKILL,	"sound/awards/monsterkill.wav",	0,		"Monster Kill!"},
	{AWARD_LUDICROUSKILL,	"sound/awards/ludicrouskill.wav",	0,		"Ludicrous Kill!"},
	{AWARD_HOLYSHIT,	"sound/awards/holyshit.wav",	0,		"HOLY SHIT"},
	{AWARD_FIRSTBLOOD,	"sound/awards/firstblood.wav",	0,		"First Blood"},
	{AWARD_SADDAY,		"sound/awards/sadday.wav",		0,		"Sad Day!"}
};
int cg_nawardlist = ARRAY_LEN(cg_awardlist);

vmCvar_t cg_railTrailTime;
vmCvar_t cg_centertime;
vmCvar_t cg_shadows;
vmCvar_t cg_gibs;
vmCvar_t cg_drawDamageDir;
vmCvar_t cg_drawTimer;
vmCvar_t cg_drawObituaries;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSpeedometer;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_draw3dIcons;
vmCvar_t cg_drawIcons;
vmCvar_t cg_drawAmmoWarning;
vmCvar_t cg_crosshair;
vmCvar_t cg_crosshairNames;
vmCvar_t cg_drawRewards;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_crosshairColor;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_addMarks;
vmCvar_t cg_brassTime;
vmCvar_t cg_viewsize;
vmCvar_t cg_drawGun;
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_gun2_x;
vmCvar_t cg_gun2_y;
vmCvar_t cg_gun2_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_autoswitch;
vmCvar_t cg_ignore;
vmCvar_t cg_simpleItems;
vmCvar_t cg_fov;
vmCvar_t cg_zoomFov;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_lagometer;
vmCvar_t cg_synchronousClients;
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_forceModel;
vmCvar_t cg_brightskins;
vmCvar_t cg_enemyColor;
vmCvar_t cg_teamColor;
vmCvar_t cg_paused;
vmCvar_t cg_blood;
vmCvar_t cg_predictItems;
vmCvar_t cg_deferPlayers;
vmCvar_t cg_drawTeamOverlay;
vmCvar_t cg_teamOverlayUserinfo;
vmCvar_t cg_drawFriend;
vmCvar_t cg_drawBBox;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_hudFiles;
vmCvar_t cg_scorePlum;
vmCvar_t cg_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t cg_testmodelscale;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t cg_pmove_msec;
vmCvar_t cg_cameraMode;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_noTaunt;
vmCvar_t cg_noProjectileTrail;
vmCvar_t cg_oldRail;
vmCvar_t cg_oldRocket;
vmCvar_t cg_oldPlasma;
vmCvar_t cg_enemyThrustSounds;
vmCvar_t cg_ownThrustSounds;
vmCvar_t cg_thrustSmoke;
vmCvar_t cg_thrustLight;
vmCvar_t cg_rocketExpSmoke;
vmCvar_t cg_rocketExpShockwave;
vmCvar_t cg_rocketExpSparks;
vmCvar_t cg_rocketSmoke;
vmCvar_t cg_rocketFlame;
vmCvar_t cg_pickupFlash;
#ifdef MISSIONPACK
vmCvar_t cg_redTeamName;
vmCvar_t cg_blueTeamName;
vmCvar_t cg_currentSelectedPlayer;
vmCvar_t cg_currentSelectedPlayerName;
vmCvar_t cg_singlePlayer;
vmCvar_t cg_enableDust;
vmCvar_t cg_enableBreath;
vmCvar_t cg_singlePlayerActive;
vmCvar_t cg_recordSPDemo;
vmCvar_t cg_recordSPDemoName;
vmCvar_t cg_obeliskRespawnDelay;
#endif

typedef struct
{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{&cg_ignore, "cg_ignore", "0", 0},	// used for debugging
	{&cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE},
	{&cg_fov, "cg_fov", "120", CVAR_ARCHIVE},
	{&cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE},
	{&cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE},
	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawDamageDir, "cg_drawDamageDir", "1", CVAR_ARCHIVE},
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{&cg_drawObituaries, "cg_drawObituaries", "1", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{&cg_drawSpeedometer, "cg_drawSpeedometer", "2", CVAR_ARCHIVE},
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE},
	{&cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE},
	{&cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE},
	{&cg_crosshair, "cg_crosshair", "4", CVAR_ARCHIVE},
	{&cg_crosshairNames, "cg_crosshairNames", "1", CVAR_ARCHIVE},
	{&cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE},
	{&cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE},
	{&cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE},
	{&cg_crosshairColor, "cg_crosshairColor", "FFFFFFFF", CVAR_ARCHIVE},
	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_brassTime, "cg_brassTime", "10000", CVAR_ARCHIVE},
	{&cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE},
	{&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
	{&cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE},
	{&cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE},
	{&cg_gun_x, "cg_gunX", "0.0", CVAR_CHEAT},
	{&cg_gun_y, "cg_gunY", "10.0", CVAR_CHEAT},
	{&cg_gun_z, "cg_gunZ", "-6.0", CVAR_CHEAT},
	{&cg_gun2_x, "cg_gun2X", "0.0", CVAR_CHEAT},
	{&cg_gun2_y, "cg_gun2Y", "-10.0", CVAR_CHEAT},
	{&cg_gun2_z, "cg_gun2Z", "-6.0", CVAR_CHEAT},
	{&cg_centertime, "cg_centertime", "3", CVAR_CHEAT},
	{&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
	{&cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},
	{&cg_nopredict, "cg_nopredict", "0", 0},
	{&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
	{&cg_showmiss, "cg_showmiss", "0", 0},
	{&cg_tracerChance, "cg_tracerchance", "1", CVAR_ARCHIVE},
	{&cg_tracerWidth, "cg_tracerwidth", "4", CVAR_ARCHIVE},
	{&cg_tracerLength, "cg_tracerlength", "1000", CVAR_ARCHIVE},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPerson, "cg_thirdPerson", "0", 0},
	{&cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE},
	{&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
	{&cg_brightskins, "cg_brightskins", "1", CVAR_ARCHIVE},
	{&cg_enemyColor, "cg_enemyColor", "00FF00FF", CVAR_ARCHIVE},
	{&cg_teamColor, "cg_teamColor", "AAAAAAFF", CVAR_ARCHIVE},
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
#ifdef MISSIONPACK
	{&cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE},
#else
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
#endif
	{&cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE},
	{&cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO},
	{&cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT},
	{&cg_stats, "cg_stats", "0", 0},
	{&cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE},
	{&cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE},
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{&cg_buildScript, "com_buildScript", "0", 0},	// force loading of all possible data amd error on failures
	{&cg_paused, "cl_paused", "0", CVAR_ROM},
	{&cg_blood, "com_blood", "1", CVAR_ARCHIVE},
	{&cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO},
#ifdef MISSIONPACK
	{&cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{&cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{&cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{&cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{&cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{&cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},
	{&cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO},
	{&cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
#endif
	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&cg_testmodelscale, "cg_testmodelscale", "1.0", 0},

	{&pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{&pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE},
	{&cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE},
	{&cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE},
	{&cg_enemyThrustSounds, "cg_enemyThrustSounds", "1", CVAR_ARCHIVE},
	{&cg_ownThrustSounds, "cg_ownThrustSounds", "1", CVAR_ARCHIVE},
	{&cg_thrustSmoke, "cg_thrustSmoke", "1", CVAR_ARCHIVE},
	{&cg_thrustLight, "cg_thrustLight", "1", CVAR_ARCHIVE},
	{&cg_rocketExpShockwave, "cg_rocketExpShockwave", "1", CVAR_ARCHIVE},
	{&cg_rocketExpSmoke, "cg_rocketExpSmoke", "1", CVAR_ARCHIVE},
	{&cg_rocketExpSparks, "cg_rocketExpSparks", "60", CVAR_ARCHIVE},
	{&cg_rocketFlame, "cg_rocketFlame", "1", CVAR_ARCHIVE},
	{&cg_rocketSmoke, "cg_rocketSmoke", "1", CVAR_ARCHIVE},
	{&cg_pickupFlash, "cg_pickupFlash", "1", CVAR_ARCHIVE}
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
};

static int cvarTableSize = ARRAY_LEN(cvarTable);

void
registercvars(void)
{
	int i;
	cvarTable_t *cv;
	char var[MAX_TOKEN_CHARS];

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
				   cv->defaultString, cv->cvarFlags);

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.srvislocal = atoi(var);

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(nil, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
}

static void
forcemodelchange(void)
{
	int i;

	for(i = 0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		clientInfo = getconfigstr(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		newclientinfo(i);
	}
}

void
updatecvars(void)
{
	int i;
	cvarTable_t *cv;
	vec4_t clr;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Update(cv->vmCvar);

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if(drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount){
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if(cg_drawTeamOverlay.integer > 0)
			trap_Cvar_Set("teamoverlay", "1");
		else
			trap_Cvar_Set("teamoverlay", "0");
	}

	// if force model changed
	if(forceModelModificationCount != cg_forceModel.modificationCount){
		forceModelModificationCount = cg_forceModel.modificationCount;
		forcemodelchange();
	}

	// make brightskin rgba arrays
	Com_HexStrToColor(cg_enemyColor.string, clr);
	MAKERGBA(cg.enemyRGBA, clr[0]*0xFF, clr[1]*0xFF, clr[2]*0xFF, clr[3]*0xFF);
	Com_HexStrToColor(cg_teamColor.string, clr);
	MAKERGBA(cg.teamRGBA, clr[0]*0xFF, clr[1]*0xFF, clr[2]*0xFF, clr[3]*0xFF);
}

int
xhairplayer(void)
{
	if(cg.time > (cg.xhairclienttime + 1000))
		return -1;
	return cg.xhairclientnum;
}

int
getlastattacker(void)
{
	if(!cg.attackertime)
		return -1;
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL
cgprintf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

void QDECL
cgerrorf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

const char *
cgargv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

//========================================================================

/*
The server says this item is used on this level
*/
static void
regitemsounds(int itemNum)
{
	gitem_t *item;
	char data[MAX_QPATH];
	char *s, *start;
	int len;

	item = &bg_itemlist[itemNum];

	if(item->pickupsound)
		trap_S_RegisterSound(item->pickupsound, qfalse);

	// parse the space seperated precache string for other media
	s = item->sounds;
	if(!s || !s[0])
		return;

	while(*s){
		start = s;
		while(*s && *s != ' ')
			s++;

		len = s-start;
		if(len >= MAX_QPATH || len < 5){
			cgerrorf("PrecacheItem: %s has bad precache string",
				 item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if(*s)
			s++;

		if(!strcmp(data+len-3, "wav"))
			trap_S_RegisterSound(data, qfalse);
	}
}

/*
called during a precache command
*/
static void
regsounds(void)
{
	int i;
	char items[MAX_ITEMS+1];
	char name[MAX_QPATH];
	const char *soundName;

	cgs.media.killSound = trap_S_RegisterSound("sound/feedback/kill.wav", qtrue);
	cgs.media.oneMinuteSound = trap_S_RegisterSound("sound/feedback/1_minute.wav", qtrue);
	cgs.media.fiveMinuteSound = trap_S_RegisterSound("sound/feedback/5_minute.wav", qtrue);
	cgs.media.suddenDeathSound = trap_S_RegisterSound("sound/feedback/sudden_death.wav", qtrue);
	cgs.media.oneFragSound = trap_S_RegisterSound("sound/feedback/1_frag.wav", qtrue);
	cgs.media.twoFragSound = trap_S_RegisterSound("sound/feedback/2_frags.wav", qtrue);
	cgs.media.threeFragSound = trap_S_RegisterSound("sound/feedback/3_frags.wav", qtrue);
	cgs.media.count4Sound = trap_S_RegisterSound("sound/feedback/roundbegins.wav", qtrue);
	cgs.media.count3Sound = trap_S_RegisterSound("sound/feedback/three.wav", qtrue);
	cgs.media.count2Sound = trap_S_RegisterSound("sound/feedback/two.wav", qtrue);
	cgs.media.count1Sound = trap_S_RegisterSound("sound/feedback/one.wav", qtrue);
	cgs.media.countFightSound = trap_S_RegisterSound("sound/feedback/fight.wav", qtrue);
	cgs.media.countPrepareSound = trap_S_RegisterSound("sound/feedback/prepare.wav", qtrue);
	cgs.media.lockingOnSound = trap_S_RegisterSound("sound/feedback/lockingon.wav", qtrue);
	cgs.media.lockedOnSound = trap_S_RegisterSound("sound/feedback/lockedon.wav", qtrue);
	cgs.media.thrustSound = trap_S_RegisterSound("sound/player/thrust.wav", qfalse);
	cgs.media.thrustBackSound = trap_S_RegisterSound("sound/player/thrustback.wav", qfalse);
	cgs.media.thrustIdleSound = trap_S_RegisterSound("sound/player/idle.wav", qfalse);
	cgs.media.thrustOtherSound = trap_S_RegisterSound("sound/player/thrustother.wav", qfalse);
	cgs.media.thrustOtherBackSound = trap_S_RegisterSound("sound/player/thrustbackother.wav", qfalse);
	cgs.media.thrustOtherIdleSound = trap_S_RegisterSound("sound/player/idleother.wav", qfalse);
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound("sound/feedback/prepare_team.wav", qtrue);
#endif

	if(cgs.gametype >= GT_TEAM || cg_buildScript.integer){
		cgs.media.captureAwardSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.redLeadsSound = trap_S_RegisterSound("sound/feedback/redleads.wav", qtrue);
		cgs.media.blueLeadsSound = trap_S_RegisterSound("sound/feedback/blueleads.wav", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound("sound/feedback/teamstied.wav", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound("sound/feedback/hit_teammate.wav", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound("sound/teamplay/voc_red_scores.wav", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound("sound/teamplay/voc_blue_scores.wav", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.captureOpponentSound = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.wav", qtrue);

		cgs.media.returnYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagreturn_yourteam.wav", qtrue);
		cgs.media.returnOpponentSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);

		cgs.media.takenYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagtaken_yourteam.wav", qtrue);
		cgs.media.takenOpponentSound = trap_S_RegisterSound("sound/teamplay/flagtaken_opponent.wav", qtrue);

		if(cgs.gametype == GT_CTF || cg_buildScript.integer){
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_red_returned.wav", qtrue);
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_blue_returned.wav", qtrue);
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound("sound/teamplay/voc_enemy_flag.wav", qtrue);
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound("sound/teamplay/voc_team_flag.wav", qtrue);
		}

#ifdef MISSIONPACK
		if(cgs.gametype == GT_1FCTF || cg_buildScript.integer){
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound("sound/teamplay/voc_team_1flag.wav", qtrue);
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound("sound/teamplay/voc_enemy_1flag.wav", qtrue);
		}

		if(cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cg_buildScript.integer){
			cgs.media.youHaveFlagSound = trap_S_RegisterSound("sound/teamplay/voc_you_flag.wav", qtrue);
			cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		}

		if(cgs.gametype == GT_OBELISK || cg_buildScript.integer)
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound("sound/teamplay/voc_base_attack.wav", qtrue);

#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound("sound/teamplay/voc_you_flag.wav", qtrue);
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound("sound/weapons/machinegun/buletby1.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/change.wav", qfalse);
	cgs.media.wearOffSound = trap_S_RegisterSound("sound/items/wearoff.wav", qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound("sound/items/use_nothing.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gibsplt1.wav", qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound("sound/player/gibimp1.wav", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound("sound/player/gibimp2.wav", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound("sound/player/gibimp3.wav", qfalse);

#ifdef MISSIONPACK
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound("sound/items/invul_activate.wav", qfalse);
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound("sound/items/invul_impact_01.wav", qfalse);
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound("sound/items/invul_impact_02.wav", qfalse);
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound("sound/items/invul_impact_03.wav", qfalse);
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound("sound/items/invul_juiced.wav", qfalse);
	cgs.media.obeliskHitSound1 = trap_S_RegisterSound("sound/items/obelisk_hit_01.wav", qfalse);
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound("sound/items/obelisk_hit_02.wav", qfalse);
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound("sound/items/obelisk_hit_03.wav", qfalse);
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound("sound/items/obelisk_respawn.wav", qfalse);

	cgs.media.ammoregenSound = trap_S_RegisterSound("sound/items/cl_ammoregen.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("sound/items/cl_scout.wav", qfalse);
#endif

	cgs.media.teleInSound = trap_S_RegisterSound("sound/world/telein.wav", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sound/world/teleout.wav", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sound/items/respawn1.wav", qfalse);

	cgs.media.noAmmoSound = trap_S_RegisterSound("sound/weapons/noammo.wav", qfalse);

	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sound/player/land1.wav", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hit.wav", qfalse);
	cgs.media.hitTeamSound = trap_S_RegisterSound("sound/feedback/hitteam.wav", qfalse);

	cgs.media.takenLeadSound = trap_S_RegisterSound("sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound("sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound("sound/feedback/lostlead.wav", qtrue);

#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound("sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound("sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound("sound/feedback/vote_failed.wav", qtrue);
#endif

	cgs.media.jumpPadSound = trap_S_RegisterSound("sound/world/jumppad.wav", qfalse);

	// only register the items that the server says we need
	Q_strncpyz(items, getconfigstr(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_nitems; i++)
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
		regitemsounds(i);
//		}

	for(i = 1; i < MAX_SOUNDS; i++){
		soundName = getconfigstr(CS_SOUNDS+i);
		if(!soundName[0])
			break;
		if(soundName[0] == '*')
			continue;	// custom sound
		cgs.gamesounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound("sound/items/flight.wav", qfalse);
	cgs.media.medkitSound = trap_S_RegisterSound("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/quad.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound("sound/weapons/machinegun/ric3.wav", qfalse);
	//cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_rockexp[1] = trap_S_RegisterSound("sound/weapons/rocket/rocklx2a.wav", qfalse);
	cgs.media.sfx_plasmaexp[0] = trap_S_RegisterSound("sound/weapons/plasma/plasmx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp[1] = trap_S_RegisterSound("sound/weapons/plasma/plasmx2a.wav", qfalse);
#ifdef MISSIONPACK
	cgs.media.sfx_proxexp = trap_S_RegisterSound("sound/weapons/proxmine/wstbexpl.wav", qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound("sound/weapons/nailgun/wnalimpd.wav", qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound("sound/weapons/nailgun/wnalimpm.wav", qfalse);
	cgs.media.sfx_chghit = trap_S_RegisterSound("sound/weapons/vulcan/wvulimpd.wav", qfalse);
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound("sound/weapons/vulcan/wvulimpm.wav", qfalse);
	cgs.media.weaponHoverSound = trap_S_RegisterSound("sound/weapons/weapon_hover.wav", qfalse);
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound("sound/items/kam_explode.wav", qfalse);
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound("sound/items/kam_implode.wav", qfalse);
	cgs.media.kamikazeFarSound = trap_S_RegisterSound("sound/items/kam_explode_far.wav", qfalse);
	cgs.media.winnerSound = trap_S_RegisterSound("sound/feedback/voc_youwin.wav", qfalse);
	cgs.media.loserSound = trap_S_RegisterSound("sound/feedback/voc_youlose.wav", qfalse);

	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);
#endif

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse);
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);

}

//===================================================================================

/*
This function may execute for a couple of minutes with a slow disk.
*/
static void
reggraphics(void)
{
	int i;
	char items[MAX_ITEMS+1];
	static char *sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	loadingstr(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	loadingstr("game media");

	for(i = 0; i<11; i++)
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);

	cgs.media.deferShader = trap_R_RegisterShaderNoMip("gfx/2d/defer.tga");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
#ifdef MISSIONPACK
	cgs.media.nailPuffShader = trap_R_RegisterShader("nailtrail");
	cgs.media.blueProxMine = trap_R_RegisterModel("models/weaphits/proxmineb.md3");
#endif
	cgs.media.plasmaBallShader = trap_R_RegisterShader("sprites/plasma1");
	cgs.media.bloodTrailShader = trap_R_RegisterShader("bloodTrail");
	cgs.media.connectionShader = trap_R_RegisterShader("disconnected");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.selectShader = trap_R_RegisterShader("gfx/2d/select");

	for(i = 0; i < NUM_CROSSHAIRS; i++)
		cgs.media.crosshairShader[i] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%d", i+1));

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");
	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");
	cgs.media.hurtLeftShader = trap_R_RegisterShader("gfx/2d/hurtleft");
	cgs.media.hurtForwardShader = trap_R_RegisterShader("gfx/2d/hurtfwd");
	cgs.media.hurtUpShader = trap_R_RegisterShader("gfx/2d/hurtup");

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad");
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon");
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit");
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon");
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility");
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen");
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

#ifdef MISSIONPACK
	if(cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
		cgs.media.redCubeModel = trap_R_RegisterModel("models/powerups/orb/r_orb.md3");
		cgs.media.blueCubeModel = trap_R_RegisterModel("models/powerups/orb/b_orb.md3");
		cgs.media.redCubeIcon = trap_R_RegisterShader("icons/skull_red");
		cgs.media.blueCubeIcon = trap_R_RegisterShader("icons/skull_blue");
	}

	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
#else
	if(cgs.gametype == GT_CTF || cg_buildScript.integer){
#endif
		cgs.media.redFlagModel = trap_R_RegisterModel("models/flags/r_flag.md3");
		cgs.media.blueFlagModel = trap_R_RegisterModel("models/flags/b_flag.md3");
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_red1");
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_red2");
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_red3");
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_blu1");
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_blu2");
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_blu3");
#ifdef MISSIONPACK

		cgs.media.redFlagBaseModel = trap_R_RegisterModel("models/mapobjects/flagbase/red_base.md3");
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel("models/mapobjects/flagbase/blue_base.md3");
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel("models/mapobjects/flagbase/ntrl_base.md3");
#endif
	}

#ifdef MISSIONPACK
	if(cgs.gametype == GT_1FCTF || cg_buildScript.integer){
		cgs.media.neutralFlagModel = trap_R_RegisterModel("models/flags/n_flag.md3");
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_neutral1");
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_red2");
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_blu2");
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip("icons/iconf_neutral3");
	}

	if(cgs.gametype == GT_OBELISK || cg_buildScript.integer){
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel("models/powerups/overload_base.md3");
		cgs.media.overloadTargetModel = trap_R_RegisterModel("models/powerups/overload_target.md3");
		cgs.media.overloadLightsModel = trap_R_RegisterModel("models/powerups/overload_lights.md3");
		cgs.media.overloadEnergyModel = trap_R_RegisterModel("models/powerups/overload_energy.md3");
	}

	if(cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
		cgs.media.harvesterModel = trap_R_RegisterModel("models/powerups/harvester/harvester.md3");
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin("models/powerups/harvester/red.skin");
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin("models/powerups/harvester/blue.skin");
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel("models/powerups/obelisk/obelisk.md3");
	}

	cgs.media.redKamikazeShader = trap_R_RegisterShader("models/weaphits/kamikred");
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff");
#endif

	if(cgs.gametype >= GT_TEAM || cg_buildScript.integer){
		cgs.media.friendShader = trap_R_RegisterShader("sprites/foe");
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag");
		cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");
#ifdef MISSIONPACK
		cgs.media.blueKamikazeShader = trap_R_RegisterShader("models/weaphits/kamikblu");
#endif
	}

	trap_R_RegisterShader("icons/worlddeath");	// precache

	cgs.media.thrustFlameModel = trap_R_RegisterModel("models/players/thrust.md3");

	cgs.media.shieldGreenIcon = trap_R_RegisterShaderNoMip("icons/shield_green");
	cgs.media.shieldYellowIcon = trap_R_RegisterShaderNoMip("icons/shield_yellow");
	cgs.media.shieldRedIcon = trap_R_RegisterShaderNoMip("icons/shield_red");

	cgs.media.machinegunBrassModel = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.shotgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.gibSkull = trap_R_RegisterModel("models/gibs/gib.md3");

	cgs.media.smoke2 = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	cgs.media.lockingOnShader = trap_R_RegisterShaderNoMip("gfx/2d/lockingon");
	cgs.media.lockedOnShader = trap_R_RegisterShaderNoMip("gfx/2d/lockedon");

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");

	cgs.media.shockwaveModel = trap_R_RegisterModel("models/weaphits/shockwave.md3");


	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/misc/telep.md3");
	cgs.media.teleportEffectShader = trap_R_RegisterShader("teleportEffect");

#if 0 // MISSIONPACK
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel("models/weaphits/kamboom2.md3");
	cgs.media.kamikazeShockWave = trap_R_RegisterModel("models/weaphits/kamwave.md3");
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel("models/powerups/kamikazi.md3");
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel("models/powerups/trailtest.md3");
	cgs.media.guardPowerupModel = trap_R_RegisterModel("models/powerups/guard_player.md3");
	cgs.media.scoutPowerupModel = trap_R_RegisterModel("models/powerups/scout_player.md3");
	cgs.media.doublerPowerupModel = trap_R_RegisterModel("models/powerups/doubler_player.md3");
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel("models/powerups/ammo_player.md3");
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel("models/powerups/shield/impact.md3");
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel("models/powerups/shield/juicer.md3");
	cgs.media.medkitUsageModel = trap_R_RegisterModel("models/powerups/regen.md3");
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel("models/powerups/shield/shield.md3");
#endif


	// register award assets
	for(i = 0; i < ARRAY_LEN(cg_awardlist); i++){
		if(cg_awardlist[i].sfx != nil)
			trap_S_RegisterSound(cg_awardlist[i].sfx, qtrue);
		if(cg_awardlist[i].shader != nil)
			trap_R_RegisterShaderNoMip(cg_awardlist[i].shader);
	}

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// only register the items that the server says we need
	Q_strncpyz(items, getconfigstr(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_nitems; i++)
		if(items[i] == '1' || cg_buildScript.integer){
			loadingitem(i);
			registeritemgfx(i);
		}

	// precache offhand hook
	cgs.media.grappleTrailShader = trap_R_RegisterShader("grappletrail");

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.holeMarkShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrk");
	cgs.media.energyMarkShader = trap_R_RegisterShader("gfx/damage/plasma_mrk");
	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader = trap_R_RegisterShader("wake");
	cgs.media.bloodMarkShader = trap_R_RegisterShader("bloodMark");

	// register the inline models
	cgs.ninlinemodels = trap_CM_NumInlineModels();
	for(i = 1; i < cgs.ninlinemodels; i++){
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlinedrawmodel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlinedrawmodel[i], mins, maxs);
		for(j = 0; j < 3; j++)
			cgs.inlinemodelmidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
	}

	// register all the server specified models
	for(i = 1; i<MAX_MODELS; i++){
		const char *modelname;

		modelname = getconfigstr(CS_MODELS+i);
		if(!modelname[0])
			break;
		cgs.gamemodels[i] = trap_R_RegisterModel(modelname);
	}

	clearparticles();
/*
        for (i=1; i<MAX_PARTICLES_AREAS; i++)
        {
                {
                        int rval;

                        rval = CG_NewParticleArea ( CS_PARTICLES + i);
                        if (!rval)
                                break;
                }
        }
*/
}

void
mkspecstr(void)
{
	int i;
	cg.speclist[0] = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_SPECTATOR)
			Q_strcat(cg.speclist, sizeof(cg.speclist), va("%s     ", cgs.clientinfo[i].name));
	i = strlen(cg.speclist);
	if(i != cg.speclen){
		cg.speclen = i;
		cg.specwidth = -1;
	}
}

static void
regclients(void)
{
	int i;

	loadingclient(cg.clientNum);
	newclientinfo(cg.clientNum);

	for(i = 0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		if(cg.clientNum == i)
			continue;

		clientInfo = getconfigstr(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		loadingclient(i);
		newclientinfo(i);
	}
	mkspecstr();
}

//===========================================================================

const char *
getconfigstr(int index)
{
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		cgerrorf("getconfigstr: bad index: %i", index);
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

void
startmusic(void)
{
	char *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char*)getconfigstr(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}



/*
Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
*/
void
cginit(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	const char *s;

	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cg.clientNum = clientNum;

	cgs.nprocessedsnaps = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	drawlibinit();

	// load a few needed things before we do any screen updates
	cgs.media.whiteShader = trap_R_RegisterShader("white");

	registercvars();

	initconsolecmds();

	cg.weapsel[0] = WP_MACHINEGUN;
	cg.weapsel[1] = WP_MACHINEGUN;
	cg.weapsel[2] = WP_GRAPPLING_HOOK;

	cgs.redflag = cgs.blueflag = -1;// For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.scrnxscale = cgs.glconfig.vidWidth / 640.0;
	cgs.scrnyscale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = getconfigstr(CS_GAME_VERSION);
	if(strcmp(s, GAME_VERSION))
		cgerrorf("Client/Server game mismatch: %s/%s", GAME_VERSION, s);

	s = getconfigstr(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	parsesrvinfo();

	// load the new map
	loadingstr("collision map");

	trap_CM_LoadMap(cgs.mapname);


	cg.loading = qtrue;	// force players to load instead of defer

	loadingstr("sounds");

	regsounds();

	loadingstr("graphics");

	reggraphics();

	loadingstr("clients");

	regclients();	// if low on memory, some clients will be deferred


	cg.loading = qfalse;	// future players will be deferred

	initlocalents();

	initmarkpolys();

	// remove the last loading update
	cg.infoscreentext[0] = 0;

	// Make sure we have update values (scores)
	setconfigvals();

	startmusic();

	loadingstr("");


	shaderstatechanged();

	trap_S_ClearLoopingSounds(qtrue);
}

/*
Called before every level change or subsystem restart
*/
void
cgshutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void
eventhandling(int type)
{
}

void
keyevent(int key, qboolean down)
{
}

void
mouseevent(int x, int y)
{
}

