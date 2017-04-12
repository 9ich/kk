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
#include "g_local.h"

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
Called on game shutdown
*/
void
writesessdata(gclient_t *client)
{
	const char *s;
	const char *var;

	s = va("%i %i %i %i %i %i %i",
	       client->sess.team,
	       client->sess.specnum,
	       client->sess.specstate,
	       client->sess.specclient,
	       client->sess.wins,
	       client->sess.losses,
	       client->sess.teamleader
	       );

	var = va("session%i", (int)(client - level.clients));

	trap_Cvar_Set(var, s);
}

/*
Called on a reconnect
*/
void
sessread(gclient_t *client)
{
	char s[MAX_STRING_CHARS];
	const char *var;
	int teamleader;
	int specstate;
	int team;

	var = va("session%i", (int)(client - level.clients));
	trap_Cvar_VariableStringBuffer(var, s, sizeof(s));

	sscanf(s, "%i %i %i %i %i %i %i",
	       &team,
	       &client->sess.specnum,
	       &specstate,
	       &client->sess.specclient,
	       &client->sess.wins,
	       &client->sess.losses,
	       &teamleader
	       );

	client->sess.team = (team_t)team;
	client->sess.specstate = (spectatorState_t)specstate;
	client->sess.teamleader = (qboolean)teamleader;
}

/*
Called on a first-time connect
*/
void
sessinit(gclient_t *client, char *userinfo)
{
	clientSession_t *sess;
	const char *value;

	sess = &client->sess;

	// initial team determination
	if(g_gametype.integer >= GT_TEAM){
		if(g_teamAutoJoin.integer && !(g_entities[client - level.clients].r.svFlags & SVF_BOT)){
			sess->team = pickteam(-1);
			broadcastteamchange(client, -1);
		}else
			// always spawn as spectator in team games
			sess->team = TEAM_SPECTATOR;
	}else{
		value = Info_ValueForKey(userinfo, "team");
		if(value[0] == 's')
			// a willing spectator, not a waiting-in-line
			sess->team = TEAM_SPECTATOR;
		else{
			switch(g_gametype.integer){
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if(g_maxGameClients.integer > 0 &&
				   level.nnonspecclients >= g_maxGameClients.integer)
					sess->team = TEAM_SPECTATOR;
				else
					sess->team = TEAM_FREE;
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if(level.nnonspecclients >= 2)
					sess->team = TEAM_SPECTATOR;
				else
					sess->team = TEAM_FREE;
				break;
			}
		}
	}

	sess->specstate = SPECTATOR_FREE;
	addduelqueue(client);

	writesessdata(client);
}

void
worldsessinit(void)
{
	char s[MAX_STRING_CHARS];
	int gt;

	trap_Cvar_VariableStringBuffer("session", s, sizeof(s));
	gt = atoi(s);

	// if the gametype changed since the last session, don't use any
	// client sessions
	if(g_gametype.integer != gt){
		level.newsess = qtrue;
		gprintf("Gametype changed, clearing session data.\n");
	}
}

void
sesswrite(void)
{
	int i;

	trap_Cvar_Set("session", va("%i", g_gametype.integer));

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			writesessdata(&level.clients[i]);
}
