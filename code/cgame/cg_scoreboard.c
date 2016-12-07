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

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	16	// line height
#define SB_INTER_HEIGHT		16	// interleaved height

#define SB_HEADER		86		// header y
#define SB_TOP			(SB_HEADER+SB_NORMAL_HEIGHT)	// score list y

#define SB_MAXCLIENTS_NORMAL	((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER	((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

static float SB_SCORE_X;	// set in drawoldscoreboard to compensate for widescreen
#define SB_PING_X		(SB_SCORE_X+48)
#define SB_TIME_X		(SB_PING_X+64)
#define SB_NAME_X		(SB_TIME_X+48)

static float SB_HIGHLIGHTMARGIN;	// set in drawoldscoreboard to compensate for widescreen

static qboolean localclient;	// true if local client has been displayed

static void
drawclientscore(int y, score_t *score, float *color, float fade)
{
	clientInfo_t *ci;
	int font = FONT2;
	float sz = 12;
	vec4_t clr;
	qboolean connecting;

	if(score->client < 0 || score->client >= cgs.maxclients){
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

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
		fillrect(SB_HIGHLIGHTMARGIN, y, screenwidth() - 2*SB_HIGHLIGHTMARGIN,
		   stringheight(ci->name, font, sz), hcolor);
	}

	coloralpha(clr, CWhite, fade);

	connecting = (score->ping == -1);

	// 1st column
	if(connecting)
		drawstring(SB_SCORE_X, y, "connecting", font, sz, clr);
	else if(ci->team == TEAM_SPECTATOR)
		drawstring(SB_SCORE_X, y, "spec", font, sz, clr);
	else
		drawstring(SB_SCORE_X, y, va("%i", score->score), font, sz, clr);
	// 2nd column
	if(!connecting)
		drawstring(SB_PING_X, y, va("%i ms", score->ping), font, sz, clr);
	// 3rd
	if(!connecting)
		drawstring(SB_TIME_X, y, va("%i", score->time), font, sz, clr);
	// 4th
	drawstring(SB_NAME_X, y, ci->name, font, sz, clr);

	// add the "ready" marker for intermission exiting
	if(cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client))
		drawstring(SB_SCORE_X - 64, y, "ready", font, sz, CLightGreen);
}

/*
=================
CG_TeamScoreboard
=================
*/
static int
teamscoreboard(int y, team_t team, float fade, int maxClients, int lineHeight)
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

		drawclientscore(y + lineHeight * count, score, color, fade);

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
	int y, i, n1, n2;
	float fade;
	float *fadeColor;
	char *s;
	int maxClients;
	int lineHeight;
	int font = FONT2;
	float sz = 16;
	vec4_t clr;

	SB_SCORE_X = centerleft() + 210;
	SB_HIGHLIGHTMARGIN = centerleft() + 100;

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

	coloralpha(clr, CWhite, fade);

	pushalign("center");
	// fragged by ... line
	if(cg.killername[0]){
		s = va("Fragged by %s", cg.killername);
		y = 40;
		drawbigstr(screenwidth()/2, y, s, fade);
	}

	// current rank
	if(cgs.gametype < GT_TEAM){
		if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR){
			s = va("%s place with %i",
			       placestr(cg.snap->ps.persistant[PERS_RANK] + 1),
			       cg.snap->ps.persistant[PERS_SCORE]);
			y = 60;
			drawstring(screenwidth()/2, y, s, font, sz, clr);
		}
	}else{
		if(cg.teamscores[0] == cg.teamscores[1])
			s = va("Teams are tied at %i", cg.teamscores[0]);
		else if(cg.teamscores[0] >= cg.teamscores[1])
			s = va("Red leads %i to %i", cg.teamscores[0], cg.teamscores[1]);
		else
			s = va("Blue leads %i to %i", cg.teamscores[1], cg.teamscores[0]);

		y = 60;
		drawstring(screenwidth()/2, y, s, font, sz, clr);
	}
	popalign(1);

	// scoreboard
	y = SB_HEADER;


	// header background
	coloralpha(clr, CWhite, 0.4f);
	fillrect(SB_HIGHLIGHTMARGIN, y, screenwidth() - 2*SB_HIGHLIGHTMARGIN,
	   stringheight("A", FONT2, 12), clr);
	// header text
	drawstring(SB_SCORE_X , y, "score", FONT2, 12, CBlack);
	drawstring(SB_PING_X, y, "ping", FONT2, 12, CBlack);
	drawstring(SB_TIME_X, y, "time", FONT2, 12, CBlack);
	drawstring(SB_NAME_X, y, "name", FONT2, 12, CBlack);

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if(cg.nscores > SB_MAXCLIENTS_NORMAL){
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
	}else{
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
	}

	localclient = qfalse;

	if(cgs.gametype >= GT_TEAM){
		// teamplay scoreboard

		if(cg.teamscores[0] >= cg.teamscores[1]){
			n1 = teamscoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			drawteambg(0, y, 640, n1 * lineHeight, 0.1f, TEAM_RED);
			y += (n1 * lineHeight);
			maxClients -= n1;
			n2 = teamscoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			drawteambg(0, y, 640, n2 * lineHeight, 0.1f, TEAM_BLUE);
			y += (n2 * lineHeight);
			maxClients -= n2;
		}else{
			n1 = teamscoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			drawteambg(0, y, 640, n1 * lineHeight, 0.1f, TEAM_BLUE);
			y += (n1 * lineHeight);
			maxClients -= n1;
			n2 = teamscoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			drawteambg(0, y, 640, n2 * lineHeight, 0.1f, TEAM_RED);
			y += (n2 * lineHeight);
			maxClients -= n2;
		}
		n1 = teamscoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight);
		y += (n1 * lineHeight);
	}else{
		// free for all scoreboard
		n1 = teamscoreboard(y, TEAM_FREE, fade, maxClients, lineHeight);
		y += (n1 * lineHeight);
		n2 = teamscoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight);
		y += (n2 * lineHeight);
	}

	if(!localclient){
		// draw local client at the bottom
		for(i = 0; i < cg.nscores; i++)
			if(cg.scores[i].client == cg.snap->ps.clientNum){
				drawclientscore(y, &cg.scores[i], fadeColor, fade);
				break;
			}
	}

	// load any models that have been deferred
	if(++cg.deferredplayerloading > 10)
		loaddeferred();

	return qtrue;
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void
drawtourneyscoreboard(void)
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
	setalign("center");
	drawstring(screenwidth()/2, 8, s, FONT1, 32, CText);
	setalign("");

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones);

	setalign("center");
	drawstring(screenwidth()/2, 64, s, FONT1, 32, CText);
	setalign("");

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
