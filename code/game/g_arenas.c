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
// g_arenas.c

#include "g_local.h"

gentity_t *podium1;
gentity_t *podium2;
gentity_t *podium3;

/*
==================
updatetourney
==================
*/
void
updateduel(void)
{
	int i;
	gentity_t *player;
	int playerClientNum;
	int n, accuracy, perfect, msglen;
#ifdef MISSIONPACK
	int score1, score2;
	qboolean won;
#endif
	char buf[32];
	char msg[MAX_STRING_CHARS];

	// find the real player
	player = nil;
	for(i = 0; i < level.maxclients; i++){
		player = &g_entities[i];
		if(!player->inuse)
			continue;
		if(!(player->r.svFlags & SVF_BOT))
			break;
	}
	// this should never happen!
	if(!player || i == level.maxclients)
		return;
	playerClientNum = i;

	calcranks();

	if(level.clients[playerClientNum].sess.team == TEAM_SPECTATOR){
#ifdef MISSIONPACK
		Com_sprintf(msg, sizeof(msg), "postgame %i %i 0 0 0 0 0 0 0 0 0 0 0", level.nnonspecclients, playerClientNum);
#else
		Com_sprintf(msg, sizeof(msg), "postgame %i %i 0 0 0 0 0 0", level.nnonspecclients, playerClientNum);
#endif
	}else{
		if(player->client->accuracyshots)
			accuracy = player->client->accuracyhits * 100 / player->client->accuracyshots;
		else
			accuracy = 0;

#ifdef MISSIONPACK
		won = qfalse;
		if(g_gametype.integer >= GT_CTF){
			score1 = level.teamscores[TEAM_RED];
			score2 = level.teamscores[TEAM_BLUE];
			if(level.clients[playerClientNum].sess.team     == TEAM_RED)
				won = (level.teamscores[TEAM_RED] > level.teamscores[TEAM_BLUE]);
			else
				won = (level.teamscores[TEAM_BLUE] > level.teamscores[TEAM_RED]);
		}else{
			if(&level.clients[playerClientNum] == &level.clients[level.sortedclients[0]]){
				won = qtrue;
				score1 = level.clients[level.sortedclients[0]].ps.persistant[PERS_SCORE];
				score2 = level.clients[level.sortedclients[1]].ps.persistant[PERS_SCORE];
			}else{
				score2 = level.clients[level.sortedclients[0]].ps.persistant[PERS_SCORE];
				score1 = level.clients[level.sortedclients[1]].ps.persistant[PERS_SCORE];
			}
		}
		if(won && player->client->ps.persistant[PERS_KILLED] == 0)
			perfect = 1;
		else
			perfect = 0;
		Com_sprintf(msg, sizeof(msg), "postgame %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.nnonspecclients, playerClientNum, accuracy,
			    player->client->ps.awards[AWARD_IMPRESSIVE], 0 /* excellent */, player->client->ps.awards[AWARD_DEFEND],
			    player->client->ps.awards[AWARD_ASSIST], player->client->ps.awards[AWARD_GAUNTLET], player->client->ps.persistant[PERS_SCORE],
			    perfect, score1, score2, level.time, player->client->ps.awards[AWARD_CAPTURE]);

#else
		perfect = (level.clients[playerClientNum].ps.persistant[PERS_RANK] == 0 && player->client->ps.persistant[PERS_KILLED] == 0) ? 1 : 0;
		Com_sprintf(msg, sizeof(msg), "postgame %i %i %i %i %i %i %i %i", level.nnonspecclients, playerClientNum, accuracy,
			    player->client->ps.awards[AWARD_IMPRESSIVE], 0 /* excellent */,
			    player->client->ps.awards[AWARD_GAUNTLET], player->client->ps.persistant[PERS_SCORE],
			    perfect);
#endif
	}

	msglen = strlen(msg);
	for(i = 0; i < level.nnonspecclients; i++){
		n = level.sortedclients[i];
		Com_sprintf(buf, sizeof(buf), " %i %i %i", n, level.clients[n].ps.persistant[PERS_RANK], level.clients[n].ps.persistant[PERS_SCORE]);
		msglen += strlen(buf);
		if(msglen >= sizeof(msg))
			break;
		strcat(msg, buf);
	}
	trap_SendConsoleCommand(EXEC_APPEND, msg);
}

static gentity_t *
SpawnModelOnVictoryPad(gentity_t *pad, vec3_t offset, gentity_t *ent, int place)
{
	gentity_t *body;
	vec3_t vec;
	vec3_t f, r, u;

	body = entspawn();
	if(!body){
		gprintf(S_COLOR_RED "ERROR: out of entities\n");
		return nil;
	}

	body->classname = ent->client->pers.netname;
	body->client = ent->client;
	body->s = ent->s;
	body->s.eType = ET_PLAYER;	// could be ET_INVISIBLE
	body->s.eFlags = 0;		// clear EF_TALK, etc
	body->s.powerups = 0;		// clear powerups
	body->s.loopSound = 0;		// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physobj = qtrue;
	body->physbounce = 0;	// don't bounce
	body->s.event = 0;
	body->s.pos.trType = TR_STATIONARY;
	body->s.groundEntityNum = ENTITYNUM_WORLD;
	body->s.legsAnim = LEGS_IDLE;
	body->s.torsoAnim = TORSO_STAND;
	if(body->s.weapon == WP_NONE)
		body->s.weapon = WP_MACHINEGUN;
	if(body->s.weapon == WP_GAUNTLET)
		body->s.torsoAnim = TORSO_STAND2;
	body->s.event = 0;
	body->r.svFlags = ent->r.svFlags;
	veccpy(ent->r.mins, body->r.mins);
	veccpy(ent->r.maxs, body->r.maxs);
	veccpy(ent->r.absmin, body->r.absmin);
	veccpy(ent->r.absmax, body->r.absmax);
	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_BODY;
	body->r.ownerNum = ent->r.ownerNum;
	body->takedmg = qfalse;

	vecsub(level.intermissionpos, pad->r.currentOrigin, vec);
	vectoangles(vec, body->s.apos.trBase);
	body->s.apos.trBase[PITCH] = 0;
	body->s.apos.trBase[ROLL] = 0;

	anglevecs(body->s.apos.trBase, f, r, u);
	vecmad(pad->r.currentOrigin, offset[0], f, vec);
	vecmad(vec, offset[1], r, vec);
	vecmad(vec, offset[2], u, vec);

	setorigin(body, vec);

	trap_LinkEntity(body);

	body->count = place;

	return body;
}

static void
CelebrateStop(gentity_t *player)
{
	int anim;

	if(player->s.weapon == WP_GAUNTLET)
		anim = TORSO_STAND2;
	else
		anim = TORSO_STAND;
	player->s.torsoAnim = ((player->s.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

#define TIMER_GESTURE (34*66+50)
static void
CelebrateStart(gentity_t *player)
{
	player->s.torsoAnim = ((player->s.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | TORSO_GESTURE;
	player->nextthink = level.time + TIMER_GESTURE;
	player->think = CelebrateStop;

	/*
	player->client->ps.events[player->client->ps.eventSequence & (MAX_PS_EVENTS-1)] = EV_TAUNT;
	player->client->ps.eventParms[player->client->ps.eventSequence & (MAX_PS_EVENTS-1)] = 0;
	player->client->ps.eventSequence++;
	*/
	addevent(player, EV_TAUNT, 0);
}

static vec3_t offsetFirst = {0, 0, 74};
static vec3_t offsetSecond = {-10, 60, 54};
static vec3_t offsetThird = {-19, -60, 45};

static void
PodiumPlacementThink(gentity_t *podium)
{
	vec3_t vec;
	vec3_t origin;
	vec3_t f, r, u;

	podium->nextthink = level.time + 100;

	anglevecs(level.intermissionangle, vec, nil, nil);
	vecmad(level.intermissionpos, trap_Cvar_VariableIntegerValue("g_podiumDist"), vec, origin);
	origin[2] -= trap_Cvar_VariableIntegerValue("g_podiumDrop");
	setorigin(podium, origin);

	if(podium1){
		vecsub(level.intermissionpos, podium->r.currentOrigin, vec);
		vectoangles(vec, podium1->s.apos.trBase);
		podium1->s.apos.trBase[PITCH] = 0;
		podium1->s.apos.trBase[ROLL] = 0;

		anglevecs(podium1->s.apos.trBase, f, r, u);
		vecmad(podium->r.currentOrigin, offsetFirst[0], f, vec);
		vecmad(vec, offsetFirst[1], r, vec);
		vecmad(vec, offsetFirst[2], u, vec);

		setorigin(podium1, vec);
	}

	if(podium2){
		vecsub(level.intermissionpos, podium->r.currentOrigin, vec);
		vectoangles(vec, podium2->s.apos.trBase);
		podium2->s.apos.trBase[PITCH] = 0;
		podium2->s.apos.trBase[ROLL] = 0;

		anglevecs(podium2->s.apos.trBase, f, r, u);
		vecmad(podium->r.currentOrigin, offsetSecond[0], f, vec);
		vecmad(vec, offsetSecond[1], r, vec);
		vecmad(vec, offsetSecond[2], u, vec);

		setorigin(podium2, vec);
	}

	if(podium3){
		vecsub(level.intermissionpos, podium->r.currentOrigin, vec);
		vectoangles(vec, podium3->s.apos.trBase);
		podium3->s.apos.trBase[PITCH] = 0;
		podium3->s.apos.trBase[ROLL] = 0;

		anglevecs(podium3->s.apos.trBase, f, r, u);
		vecmad(podium->r.currentOrigin, offsetThird[0], f, vec);
		vecmad(vec, offsetThird[1], r, vec);
		vecmad(vec, offsetThird[2], u, vec);

		setorigin(podium3, vec);
	}
}

static gentity_t *
SpawnPodium(void)
{
	gentity_t *podium;
	vec3_t vec;
	vec3_t origin;

	podium = entspawn();
	if(!podium)
		return nil;

	podium->classname = "podium";
	podium->s.eType = ET_GENERAL;
	podium->s.number = podium - g_entities;
	podium->clipmask = CONTENTS_SOLID;
	podium->r.contents = CONTENTS_SOLID;
	podium->s.modelindex = getmodelindex(SP_PODIUM_MODEL);

	anglevecs(level.intermissionangle, vec, nil, nil);
	vecmad(level.intermissionpos, trap_Cvar_VariableIntegerValue("g_podiumDist"), vec, origin);
	origin[2] -= trap_Cvar_VariableIntegerValue("g_podiumDrop");
	setorigin(podium, origin);

	vecsub(level.intermissionpos, podium->r.currentOrigin, vec);
	podium->s.apos.trBase[YAW] = vectoyaw(vec);
	trap_LinkEntity(podium);

	podium->think = PodiumPlacementThink;
	podium->nextthink = level.time + 100;
	return podium;
}

/*
==================
spawnonvictorypads
==================
*/
void
spawnonvictorypads(void)
{
	gentity_t *player;
	gentity_t *podium;

	podium1 = nil;
	podium2 = nil;
	podium3 = nil;

	podium = SpawnPodium();

	player = SpawnModelOnVictoryPad(podium, offsetFirst, &g_entities[level.sortedclients[0]],
					level.clients[level.sortedclients[0]].ps.persistant[PERS_RANK] &~RANK_TIED_FLAG);
	if(player){
		player->nextthink = level.time + 2000;
		player->think = CelebrateStart;
		podium1 = player;
	}

	player = SpawnModelOnVictoryPad(podium, offsetSecond, &g_entities[level.sortedclients[1]],
					level.clients[level.sortedclients[1]].ps.persistant[PERS_RANK] &~RANK_TIED_FLAG);
	if(player)
		podium2 = player;

	if(level.nnonspecclients > 2){
		player = SpawnModelOnVictoryPad(podium, offsetThird, &g_entities[level.sortedclients[2]],
						level.clients[level.sortedclients[2]].ps.persistant[PERS_RANK] &~RANK_TIED_FLAG);
		if(player)
			podium3 = player;
	}
}

/*
===============
Svcmd_AbortPodium_f
===============
*/
void
Svcmd_AbortPodium_f(void)
{
	if(g_gametype.integer != GT_SINGLE_PLAYER)
		return;

	if(podium1){
		podium1->nextthink = level.time;
		podium1->think = CelebrateStop;
	}
}
