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
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

#define SCOREBOARD_X		(0)

#define SB_HEADER		86
#define SB_TOP			(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16	// interleaved height

#define SB_MAXCLIENTS_NORMAL	((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER	((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved

#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X		(SCOREBOARD_X+64)

#define SB_SCORELINE_X		112

#define SB_RATING_WIDTH		(6 * BIGCHAR_WIDTH)	// width 6
#define SB_SCORE_X		(SB_SCORELINE_X + BIGCHAR_WIDTH)	// width 6
#define SB_RATING_X		(SB_SCORELINE_X + 6 * BIGCHAR_WIDTH)		// width 6
#define SB_PING_X		(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8)		// width 5
#define SB_TIME_X		(SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8)		// width 5
#define SB_NAME_X		(SB_SCORELINE_X + 22 * BIGCHAR_WIDTH)		// width 15

// The new and improved score board
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  wins/losses are drawn on bot icon now

static qboolean localclient;	// true if local client has been displayed

/*
=================
drawscoreboard
=================
*/
static void
CG_DrawClientScore(int y, score_t *score, float *color, float fade, qboolean largeFormat)
{
	char string[1024];
	clientInfo_t *ci;
	int iconx;

	if(score->client < 0 || score->client >= cgs.maxclients){
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if(ci->powerups & (1 << PW_NEUTRALFLAG)){
		if(largeFormat)
			drawflag(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_FREE, qfalse);
		else
			drawflag(iconx, y, 16, 16, TEAM_FREE, qfalse);
	}else if(ci->powerups & (1 << PW_REDFLAG)){
		if(largeFormat)
			drawflag(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_RED, qfalse);
		else
			drawflag(iconx, y, 16, 16, TEAM_RED, qfalse);
	}else if(ci->powerups & (1 << PW_BLUEFLAG)){
		if(largeFormat)
			drawflag(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_BLUE, qfalse);
		else
			drawflag(iconx, y, 16, 16, TEAM_BLUE, qfalse);
	}else{
		if(ci->botskill > 0 && ci->botskill <= 5){
			if(cg_drawIcons.integer){
				if(largeFormat)
					drawpic(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, cgs.media.botSkillShaders[ci->botskill - 1]);
				else
					drawpic(iconx, y, 16, 16, cgs.media.botSkillShaders[ci->botskill - 1]);
			}
		}else if(ci->handicap < 100){
			Com_sprintf(string, sizeof(string), "%i", ci->handicap);
			if(cgs.gametype == GT_TOURNAMENT)
				drawsmallstrcolor(iconx, y - SMALLCHAR_HEIGHT/2, string, color);
			else
				drawsmallstrcolor(iconx, y, string, color);
		}

		// draw the wins / losses
		if(cgs.gametype == GT_TOURNAMENT){
			Com_sprintf(string, sizeof(string), "%i/%i", ci->wins, ci->losses);
			if(ci->handicap < 100 && !ci->botskill)
				drawsmallstrcolor(iconx, y + SMALLCHAR_HEIGHT/2, string, color);
			else
				drawsmallstrcolor(iconx, y, string, color);
		}
	}

	// draw the score line
	if(score->ping == -1)
		Com_sprintf(string, sizeof(string),
			    " connecting    %s", ci->name);
	else if(ci->team == TEAM_SPECTATOR)
		Com_sprintf(string, sizeof(string),
			    " SPECT %3i %4i %s", score->ping, score->time, ci->name);
	else
		Com_sprintf(string, sizeof(string),
			    "%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);

	// highlight your position
	if(score->client == cg.snap->ps.clientNum){
		float hcolor[4];
		int rank;

		localclient = qtrue;

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
		   || cgs.gametype >= GT_TEAM)
			rank = -1;
		else
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		if(rank == 0){
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		}else if(rank == 1){
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		}else if(rank == 2){
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		}else{
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		fillrect(SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2), y,
			    640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT+1, hcolor);
	}

	drawbigstr(SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade);

	// add the "ready" marker for intermission exiting
	if(cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client))
		drawbigstrcolor(iconx, y, "READY", color);
}

/*
=================
CG_TeamScoreboard
=================
*/
static int
CG_TeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight)
{
	int i;
	score_t *score;
	float color[4];
	int count;
	clientInfo_t *ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for(i = 0; i < cg.nscores && count < maxClients; i++){
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];

		if(team != ci->team)
			continue;

		CG_DrawClientScore(y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT);

		count++;
	}

	return count;
}

/*
=================
drawscoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean
drawoldscoreboard(void)
{
	int x, y, w, i, n1, n2;
	float fade;
	float *fadeColor;
	char *s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if(cg_paused.integer){
		cg.deferredplayerloading = 0;
		return qfalse;
	}

	if(cgs.gametype == GT_SINGLE_PLAYER && cg.pps.pm_type == PM_INTERMISSION){
		cg.deferredplayerloading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if(cg.warmup && !cg.showscores)
		return qfalse;

	if(cg.showscores || cg.pps.pm_type == PM_DEAD ||
	   cg.pps.pm_type == PM_INTERMISSION){
		fade = 1.0;
		fadeColor = colorWhite;
	}else{
		fadeColor = fadecolor(cg.scorefadetime, FADE_TIME);

		if(!fadeColor){
			// next time scoreboard comes up, don't print killer
			cg.deferredplayerloading = 0;
			cg.killername[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	// fragged by ... line
	if(cg.killername[0]){
		s = va("Fragged by %s", cg.killername);
		w = drawstrlen(s) * BIGCHAR_WIDTH;
		x = (screenwidth() - w) / 2;
		y = 40;
		drawbigstr(x, y, s, fade);
	}

	// current rank
	if(cgs.gametype < GT_TEAM){
		if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR){
			s = va("%s place with %i",
			       placestr(cg.snap->ps.persistant[PERS_RANK] + 1),
			       cg.snap->ps.persistant[PERS_SCORE]);
			w = drawstrlen(s) * BIGCHAR_WIDTH;
			x = (screenwidth() - w) / 2;
			y = 60;
			drawbigstr(x, y, s, fade);
		}
	}else{
		if(cg.teamscores[0] == cg.teamscores[1])
			s = va("Teams are tied at %i", cg.teamscores[0]);
		else if(cg.teamscores[0] >= cg.teamscores[1])
			s = va("Red leads %i to %i", cg.teamscores[0], cg.teamscores[1]);
		else
			s = va("Blue leads %i to %i", cg.teamscores[1], cg.teamscores[0]);

		w = drawstrlen(s) * BIGCHAR_WIDTH;
		x = (screenwidth() - w) / 2;
		y = 60;
		drawbigstr(x, y, s, fade);
	}

	// scoreboard
	y = SB_HEADER;

	drawpic(SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore);
	drawpic(SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing);
	drawpic(SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime);
	drawpic(SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName);

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if(cg.nscores > SB_MAXCLIENTS_NORMAL){
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	}else{
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	localclient = qfalse;

	if(cgs.gametype >= GT_TEAM){
		// teamplay scoreboard
		y += lineHeight/2;

		if(cg.teamscores[0] >= cg.teamscores[1]){
			n1 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			drawteambg(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			drawteambg(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}else{
			n1 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			drawteambg(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			drawteambg(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
	}else{
		// free for all scoreboard
		n1 = CG_TeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight);
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if(!localclient){
		// draw local client at the bottom
		for(i = 0; i < cg.nscores; i++)
			if(cg.scores[i].client == cg.snap->ps.clientNum){
				CG_DrawClientScore(y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT);
				break;
			}
	}

	// load any models that have been deferred
	if(++cg.deferredplayerloading > 10)
		loaddeferred();

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void
CG_CenterGiantLine(float y, const char *string)
{
	setalign("center");
	drawstring(screenwidth()/2, y, string, FONT1, 32, CText);
	setalign("");
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void
CG_DrawOldTourneyScoreboard(void)
{
	const char *s;
	vec4_t color;
	int min, tens, ones;
	clientInfo_t *ci;
	int y;
	int i;

	// request more scores regularly
	if(cg.scoresreqtime + 2000 < cg.time){
		cg.scoresreqtime = cg.time;
		trap_SendClientCommand("score");
	}

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	fillrect(0, 0, screenwidth(), screenheight(), color);

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// print the mesage of the day
	s = getconfigstr(CS_MOTD);
	if(!s[0])
		s = "Scoreboard";

	// print optional title
	CG_CenterGiantLine(8, s);

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones);

	CG_CenterGiantLine(64, s);

	// print the two scores

	y = 160;
	if(cgs.gametype >= GT_TEAM){
		// teamplay scoreboard
		drawstring(8, y, "Red Team", FONT2, 12, color);
		s = va("%i", cg.teamscores[0]);
		drawstring(632 - GIANT_WIDTH * strlen(s), y, s, FONT2, 12,
		           color);

		y += 64;

		drawstring(8, y, "Blue Team", FONT2, 12, color);
		s = va("%i", cg.teamscores[1]);
		drawstring(632 - GIANT_WIDTH * strlen(s), y, s, FONT2, 12,
		           color);
	}else
		// free for all scoreboard
		for(i = 0; i < MAX_CLIENTS; i++){
			ci = &cgs.clientinfo[i];
			if(!ci->infovalid)
				continue;
			if(ci->team != TEAM_FREE)
				continue;

			drawstring(8, y, ci->name, FONT1, 16, CText);
			s = va("%i", ci->score);
			setalign("right");
			drawstring(632, y, s, FONT1, 16, CText);
			setalign("");
			y += 64;
		}

}
