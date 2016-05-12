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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

/*
=================
CG_ParseScores

=================
*/
static void
CG_ParseScores(void)
{
	int i, powerups;

	cg.nscores = atoi(cgargv(1));
	if(cg.nscores > MAX_CLIENTS)
		cg.nscores = MAX_CLIENTS;

	cg.teamscores[0] = atoi(cgargv(2));
	cg.teamscores[1] = atoi(cgargv(3));

	memset(cg.scores, 0, sizeof(cg.scores));
	for(i = 0; i < cg.nscores; i++){
		cg.scores[i].client = atoi(cgargv(i * 14 + 4));
		cg.scores[i].score = atoi(cgargv(i * 14 + 5));
		cg.scores[i].ping = atoi(cgargv(i * 14 + 6));
		cg.scores[i].time = atoi(cgargv(i * 14 + 7));
		cg.scores[i].scoreflags = atoi(cgargv(i * 14 + 8));
		powerups = atoi(cgargv(i * 14 + 9));
		cg.scores[i].accuracy = atoi(cgargv(i * 14 + 10));
		cg.scores[i].nimpressive = atoi(cgargv(i * 14 + 11));
		cg.scores[i].nexcellent = atoi(cgargv(i * 14 + 12));
		cg.scores[i].gauntletcount = atoi(cgargv(i * 14 + 13));
		cg.scores[i].ndefend = atoi(cgargv(i * 14 + 14));
		cg.scores[i].nassist = atoi(cgargv(i * 14 + 15));
		cg.scores[i].perfect = atoi(cgargv(i * 14 + 16));
		cg.scores[i].captures = atoi(cgargv(i * 14 + 17));

		if(cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS)
			cg.scores[i].client = 0;
		cgs.clientinfo[cg.scores[i].client].score = cg.scores[i].score;
		cgs.clientinfo[cg.scores[i].client].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
	}
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void
CG_ParseTeamInfo(void)
{
	int i;
	int client;

	nsortedteamplayers = atoi(cgargv(1));
	if(nsortedteamplayers < 0 || nsortedteamplayers > TEAM_MAXOVERLAY){
		cgerrorf("CG_ParseTeamInfo: nsortedteamplayers out of range (%d)",
			 nsortedteamplayers);
		return;
	}

	for(i = 0; i < nsortedteamplayers; i++){
		client = atoi(cgargv(i * 6 + 2));
		if(client < 0 || client >= MAX_CLIENTS){
			cgerrorf("CG_ParseTeamInfo: bad client number: %d", client);
			return;
		}

		sortedteamplayers[i] = client;

		cgs.clientinfo[client].location = atoi(cgargv(i * 6 + 3));
		cgs.clientinfo[client].health = atoi(cgargv(i * 6 + 4));
		cgs.clientinfo[client].armor = atoi(cgargv(i * 6 + 5));
		cgs.clientinfo[client].currweap = atoi(cgargv(i * 6 + 6));
		cgs.clientinfo[client].powerups = atoi(cgargv(i * 6 + 7));
	}
}

/*
================
parsesrvinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void
parsesrvinfo(void)
{
	const char *info;
	char *mapname;

	info = getconfigstr(CS_SERVERINFO);
	cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
	cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
	cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));
	cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
	cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
	Q_strncpyz(cgs.redteam, Info_ValueForKey(info, "g_redTeam"), sizeof(cgs.redteam));
	trap_Cvar_Set("g_redTeam", cgs.redteam);
	Q_strncpyz(cgs.blueteam, Info_ValueForKey(info, "g_blueTeam"), sizeof(cgs.blueteam));
	trap_Cvar_Set("g_blueTeam", cgs.blueteam);
}

/*
==================
CG_ParseWarmup
==================
*/
static void
CG_ParseWarmup(void)
{
	const char *info;
	int warmup;

	info = getconfigstr(CS_WARMUP);

	warmup = atoi(info);
	cg.warmupcount = -1;

	if(warmup == 0 && cg.warmup){
	}else if(warmup > 0 && cg.warmup <= 0){
		if(cgs.gametype >= GT_CTF && cgs.gametype <= GT_HARVESTER)
			trap_S_StartLocalSound(cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER);
		else
			trap_S_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
	}

	cg.warmup = warmup;
}

static void
parseroundwarmup(void)
{
	const char *info;
	int warmup;

	info = getconfigstr(CS_ROUNDWARMUP);

	warmup = atoi(info);
	cg.roundwarmupcount = -1;
	cg.roundwarmup = warmup;
}

/*
================
setconfigvals

Called on load to set the initial values from configure strings
================
*/
void
setconfigvals(void)
{
	const char *s;

	cgs.scores1 = atoi(getconfigstr(CS_SCORES1));
	cgs.scores2 = atoi(getconfigstr(CS_SCORES2));
	cgs.levelStartTime = atoi(getconfigstr(CS_LEVEL_START_TIME));
	if(cgs.gametype == GT_CTF){
		s = getconfigstr(CS_FLAGSTATUS);
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
#ifdef MISSIONPACK
	else if(cgs.gametype == GT_1FCTF){
		s = getconfigstr(CS_FLAGSTATUS);
		cgs.flagStatus = s[0] - '0';
	}
#endif
	cg.warmup = atoi(getconfigstr(CS_WARMUP));
	cg.roundwarmup = atoi(getconfigstr(CS_ROUNDWARMUP));
}

/*
=====================
shaderstatechanged
=====================
*/
void
shaderstatechanged(void)
{
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n, *t;

	o = getconfigstr(CS_SHADERSTATE);
	while(o && *o){
		n = strstr(o, "=");
		if(n && *n){
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if(t && *t){
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			}else
				break;
			t++;
			o = strstr(t, "@");
			if(o){
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				trap_R_RemapShader(originalShader, newShader, timeOffset);
			}
		}else
			break;
	}
}

/*
================
CG_ConfigStringModified

================
*/
static void
CG_ConfigStringModified(void)
{
	const char *str;
	int num;

	num = atoi(cgargv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = getconfigstr(num);

	// do something with it if necessary
	if(num == CS_MUSIC)
		startmusic();
	else if(num == CS_SERVERINFO)
		parsesrvinfo();
	else if(num == CS_WARMUP)
		CG_ParseWarmup();
	else if(num == CS_ROUNDWARMUP)
		parseroundwarmup();
	else if(num == CS_SCORES1)
		cgs.scores1 = atoi(str);
	else if(num == CS_SCORES2)
		cgs.scores2 = atoi(str);
	else if(num == CS_LEVEL_START_TIME)
		cgs.levelStartTime = atoi(str);
	else if(num == CS_VOTE_TIME){
		cgs.votetime = atoi(str);
		cgs.votemodified = qtrue;
	}else if(num == CS_VOTE_YES){
		cgs.voteyes = atoi(str);
		cgs.votemodified = qtrue;
	}else if(num == CS_VOTE_NO){
		cgs.voteno = atoi(str);
		cgs.votemodified = qtrue;
	}else if(num == CS_VOTE_STRING){
		Q_strncpyz(cgs.votestr, str, sizeof(cgs.votestr));
#ifdef MISSIONPACK
		trap_S_StartLocalSound(cgs.media.voteNow, CHAN_ANNOUNCER);
#endif	//MISSIONPACK
	}else if(num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1){
		cgs.teamvotetime[num-CS_TEAMVOTE_TIME] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_TIME] = qtrue;
	}else if(num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1){
		cgs.teamvoteyes[num-CS_TEAMVOTE_YES] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_YES] = qtrue;
	}else if(num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1){
		cgs.teamvoteno[num-CS_TEAMVOTE_NO] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_NO] = qtrue;
	}else if(num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1){
		Q_strncpyz(cgs.teamvotestr[num-CS_TEAMVOTE_STRING], str, sizeof(cgs.teamvotestr[0]));
#ifdef MISSIONPACK
		trap_S_StartLocalSound(cgs.media.voteNow, CHAN_ANNOUNCER);
#endif
	}else if(num == CS_INTERMISSION)
		cg.intermissionstarted = atoi(str);
	else if(num >= CS_MODELS && num < CS_MODELS+MAX_MODELS)
		cgs.gamemodels[num-CS_MODELS] = trap_R_RegisterModel(str);
	else if(num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS){
		if(str[0] != '*')	// player specific sounds don't register here
			cgs.gamesounds[num-CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
	}else if(num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS){
		newclientinfo(num - CS_PLAYERS);
		mkspecstr();
	}else if(num == CS_FLAGSTATUS){
		if(cgs.gametype == GT_CTF){
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
#ifdef MISSIONPACK
		else if(cgs.gametype == GT_1FCTF)
			cgs.flagStatus = str[0] - '0';

#endif
	}else if(num == CS_SHADERSTATE)
		shaderstatechanged();

}

/*
=======================
CG_AddToTeamChat

=======================
*/
static void
CG_AddToTeamChat(const char *str)
{
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;

	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;

	if(chatHeight <= 0 || cg_teamChatTime.integer <= 0){
		// team chat disabled, dump into normal chat
		cgs.teamchatpos = cgs.teamlastchatpos = 0;
		return;
	}

	len = 0;

	p = cgs.teamchatmsgs[cgs.teamchatpos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = nil;
	while(*str){
		if(len > TEAMCHAT_WIDTH - 1){
			if(ls){
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamchatmsgtimes[cgs.teamchatpos % chatHeight] = cg.time;

			cgs.teamchatpos++;
			p = cgs.teamchatmsgs[cgs.teamchatpos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = nil;
		}

		if(Q_IsColorString(str)){
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if(*str == ' ')
			ls = p;
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamchatmsgtimes[cgs.teamchatpos % chatHeight] = cg.time;
	cgs.teamchatpos++;

	if(cgs.teamchatpos - cgs.teamlastchatpos > chatHeight)
		cgs.teamlastchatpos = cgs.teamchatpos - chatHeight;
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void
CG_MapRestart(void)
{
	if(cg_showmiss.integer)
		cgprintf("CG_MapRestart\n");

	initlocalents();
	initmarkpolys();
	CG_ClearParticles();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitwarnings = 0;

	cg.timelimitwarnings = 0;
	cg.rewardtime = 0;
	cg.rewardstack = 0;
	cg.intermissionstarted = qfalse;
	cg.levelshot = qfalse;

	cgs.votetime = 0;

	cg.maprestart = qtrue;

	startmusic();

	trap_S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if(cg.warmup == 0 && cgs.gametype != GT_LMS && cgs.gametype != GT_CA &&
	   cgs.gametype != GT_LTS){
		trap_S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
		centerprint("FIGHT", 120, GIANTCHAR_WIDTH*2);
	}
	trap_Cvar_Set("cg_thirdPerson", "0");
}


/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void
CG_RemoveChatEscapeChar(char *text)
{
	int i, l;

	l = 0;
	for(i = 0; text[i]; i++){
		if(text[i] == '\x19')
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void
CG_ServerCommand(void)
{
	const char *cmd;
	char text[MAX_SAY_TEXT];

	cmd = cgargv(0);

	if(!cmd[0])
		// server claimed the command
		return;

	if(!strcmp(cmd, "cp")){
		centerprint(cgargv(1), screenheight() * 0.30, BIGCHAR_WIDTH);
		return;
	}

	if(!strcmp(cmd, "cs")){
		CG_ConfigStringModified();
		return;
	}

	if(!strcmp(cmd, "print")){
		cgprintf("%s", cgargv(1));
		return;
	}

	if(!strcmp(cmd, "chat")){
		if(!cg_teamChatsOnly.integer){
			trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
			Q_strncpyz(text, cgargv(1), MAX_SAY_TEXT);
			CG_RemoveChatEscapeChar(text);
			cgprintf("%s\n", text);
		}
		return;
	}

	if(!strcmp(cmd, "tchat")){
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, cgargv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_AddToTeamChat(text);
		cgprintf("%s\n", text);
		return;
	}


	if(!strcmp(cmd, "scores")){
		CG_ParseScores();
		return;
	}

	if(!strcmp(cmd, "tinfo")){
		CG_ParseTeamInfo();
		return;
	}

	if(!strcmp(cmd, "map_restart")){
		CG_MapRestart();
		return;
	}

	if(Q_stricmp(cmd, "remapShader") == 0){
		if(trap_Argc() == 4){
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];

			Q_strncpyz(shader1, cgargv(1), sizeof(shader1));
			Q_strncpyz(shader2, cgargv(2), sizeof(shader2));
			Q_strncpyz(shader3, cgargv(3), sizeof(shader3));

			trap_R_RemapShader(shader1, shader2, shader3);
		}

		return;
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if(!strcmp(cmd, "loaddefered")){	// FIXME: spelled wrong, but not changing for demo
		loaddeferred();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if(!strcmp(cmd, "clientLevelShot")){
		cg.levelshot = qtrue;
		return;
	}

	cgprintf("Unknown client game command: %s\n", cmd);
}

/*
====================
execnewsrvcmds

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void
execnewsrvcmds(int latestSequence)
{
	while(cgs.serverCommandSequence < latestSequence)
		if(trap_GetServerCommand(++cgs.serverCommandSequence))
			CG_ServerCommand();
}
