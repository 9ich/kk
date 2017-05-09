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

typedef struct teamgame_s
{
	float		last_flag_capture;
	int		last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int		redTakenTime;
	int		blueTakenTime;
	int		redObeliskAttackedTime;
	int		blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

gentity_t *neutralObelisk;

void setflagstatus(int team, flagStatus_t status);

void
teamgameinit(void)
{
	memset(&teamgame, 0, sizeof teamgame);

	switch(g_gametype.integer){
	case GT_CTF:
		teamgame.redStatus = -1;// Invalid to force update
		setflagstatus(TEAM_RED, FLAG_ATBASE);
		teamgame.blueStatus = -1;	// Invalid to force update
		setflagstatus(TEAM_BLUE, FLAG_ATBASE);
		break;
	case GT_1FCTF:
		teamgame.flagStatus = -1;	// Invalid to force update
		setflagstatus(TEAM_FREE, FLAG_ATBASE);
		break;
	default:
		break;
	}
}

int
getotherteam(int team)
{
	if(team==TEAM_RED)
		return TEAM_BLUE;
	else if(team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *
teamname(int team)
{
	if(team==TEAM_RED)
		return "RED";
	else if(team==TEAM_BLUE)
		return "BLUE";
	else if(team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *
teamcolorstr(int team)
{
	if(team==TEAM_RED)
		return S_COLOR_RED;
	else if(team==TEAM_BLUE)
		return S_COLOR_BLUE;
	else if(team==TEAM_SPECTATOR)
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

int
numaliveonteam(int team)
{
	int i, n;

	n = 0;
	for(i = 0; i < g_maxclients.integer; i++){
		if(level.clients[i].ps.persistant[PERS_TEAM] != team ||
		   level.clients[i].pers.connected != CON_CONNECTED ||
		   level.clients[i].ps.stats[STAT_HEALTH] <= 0)
			continue;
		n++;
	}
	return n;
}


int
numonteam(int team)
{
	int i, n;

	n = 0;
	for(i = 0; i < g_maxclients.integer; i++){
		if(level.clients[i].ps.persistant[PERS_TEAM] != team ||
		   level.clients[i].pers.connected != CON_CONNECTED)
			continue;
		n++;
	}
	return n;
}

/*
 * Finds the number of clients occupying a control point.
 * Returns the total.
 */
int
numclientsoncp(const gentity_t *cp, int *red, int *blue)
{
	gentity_t *clients[MAX_CLIENTS];
	int i, n;

	*red = *blue = 0;
	n = clientsoncp(cp, clients, ARRAY_LEN(clients));
	for(i = 0; i < n; i++){
		if(clients[i]->client->sess.team == TEAM_RED)
			(*red)++;
		else if(clients[i]->client->sess.team == TEAM_BLUE)
			(*blue)++;
	}
	return *red + *blue;
}

/*
 * Finds the clients occupying a control point.
 * Returns the total.
 */
int
clientsoncp(const gentity_t *cp, gentity_t **clients, int nclients)
{
	int i, n;

	n = 0;
	for(i = 0; i < g_maxclients.integer; i++){
		if(n >= nclients)
			break;
		if(level.clients[i].pers.connected != CON_CONNECTED)
			continue;
		if(level.clients[i].ps.stats[STAT_HEALTH] <= 0)
			continue;
		if(vecdist(cp->s.pos.trBase, level.clients[i].ps.origin) > 64)
			continue;
		clients[n++] = g_entities + i;
	}
	return n;
}

// nil for everyone
static __attribute__ ((format(printf, 2, 3))) void QDECL
PrintMsg(gentity_t *ent, const char *fmt, ...)
{
	char msg[1024];
	va_list argptr;
	char *p;

	va_start(argptr, fmt);
	if(Q_vsnprintf(msg, sizeof(msg), fmt, argptr) >= sizeof(msg))
		errorf("PrintMsg overrun");
	va_end(argptr);

	// double quotes are bad
	while((p = strchr(msg, '"')) != nil)
		*p = '\'';

	trap_SendServerCommand(((ent == nil) ? -1 : ent-g_entities), va("print \"%s\"", msg));
}

/*
 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamscores is updated in addscore in g_combat.c
*/
void
addteamscore(vec3_t origin, int team, int score)
{
	gentity_t *te;

	te = enttemp(origin, EV_GLOBAL_TEAM_SOUND);
	te->r.svFlags |= SVF_BROADCAST;

	if(team == TEAM_RED){
		if(level.teamscores[TEAM_RED] + score == level.teamscores[TEAM_BLUE])
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		else if(level.teamscores[TEAM_RED] <= level.teamscores[TEAM_BLUE] &&
			level.teamscores[TEAM_RED] + score > level.teamscores[TEAM_BLUE])
			// red took the lead sound
			te->s.eventParm = GTS_REDTEAM_TOOK_LEAD;
		else
			// red scored sound
			te->s.eventParm = GTS_REDTEAM_SCORED;
	}else{
		if(level.teamscores[TEAM_BLUE] + score == level.teamscores[TEAM_RED])
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		else if(level.teamscores[TEAM_BLUE] <= level.teamscores[TEAM_RED] &&
			level.teamscores[TEAM_BLUE] + score > level.teamscores[TEAM_RED])
			// blue took the lead sound
			te->s.eventParm = GTS_BLUETEAM_TOOK_LEAD;
		else
			// blue scored sound
			te->s.eventParm = GTS_BLUETEAM_SCORED;
	}
	level.teamscores[team] += score;
}

qboolean
onsameteam(gentity_t *ent1, gentity_t *ent2)
{
	if(!ent1->client || !ent2->client)
		return qfalse;

	if(g_gametype.integer < GT_TEAM)
		return qfalse;

	if(ent1->client->sess.team == ent2->client->sess.team)
		return qtrue;

	return qfalse;
}

static char ctfFlagStatusRemap[] = {'0', '1', '*', '*', '2'};
static char oneFlagStatusRemap[] = {'0', '1', '2', '3', '4'};

void
setflagstatus(int team, flagStatus_t status)
{
	qboolean modified = qfalse;

	switch(team){
	case TEAM_RED:	// CTF
		if(teamgame.redStatus != status){
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_BLUE:	// CTF
		if(teamgame.blueStatus != status){
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_FREE:	// One Flag CTF
		if(teamgame.flagStatus != status){
			teamgame.flagStatus = status;
			modified = qtrue;
		}
		break;
	}

	if(modified){
		char st[4];

		if(g_gametype.integer == GT_CTF){
			st[0] = ctfFlagStatusRemap[teamgame.redStatus];
			st[1] = ctfFlagStatusRemap[teamgame.blueStatus];
			st[2] = 0;
		}else{	// GT_1FCTF
			st[0] = oneFlagStatusRemap[teamgame.flagStatus];
			st[1] = 0;
		}

		trap_SetConfigstring(CS_FLAGSTATUS, st);
	}
}

void
ckhdroppedteamitem(gentity_t *dropped)
{
	if(dropped->item->tag == PW_REDFLAG)
		setflagstatus(TEAM_RED, FLAG_DROPPED);
	else if(dropped->item->tag == PW_BLUEFLAG)
		setflagstatus(TEAM_BLUE, FLAG_DROPPED);
	else if(dropped->item->tag == PW_NEUTRALFLAG)
		setflagstatus(TEAM_FREE, FLAG_DROPPED);
}

void
forcegesture(int team)
{
	int i;
	gentity_t *ent;

	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];
		if(!ent->inuse)
			continue;
		if(!ent->client)
			continue;
		if(ent->client->sess.team != team)
			continue;
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
*/
void
fragbonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker)
{
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	int tokens;
	gentity_t *flag, *carrier = nil;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself or team mates
	if(!targ->client || !attacker->client || targ == attacker || onsameteam(targ, attacker))
		return;

	team = targ->client->sess.team;
	otherteam = getotherteam(targ->client->sess.team);
	if(otherteam < 0)
		return;		// whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if(team == TEAM_RED){
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	}else{
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

#ifdef MISSIONPACK
	if(g_gametype.integer == GT_1FCTF)
		enemy_flag_pw = PW_NEUTRALFLAG;

#endif

	// did the attacker frag the flag carrier?
	tokens = 0;
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_HARVESTER)
		tokens = targ->client->ps.generic1;

#endif
	if(targ->client->ps.powerups[enemy_flag_pw]){
		attacker->client->pers.teamstate.lastfraggedcarrier = level.time;
		addscore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamstate.fragcarrier++;
		PrintMsg(nil, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			 attacker->client->pers.netname, teamname(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;
			if(ent->inuse && ent->client->sess.team == otherteam)
				ent->client->pers.teamstate.lasthurtcarrier = 0;
		}
		return;
	}

	// did the attacker frag a head carrier? other->client->ps.generic1
	if(tokens){
		attacker->client->pers.teamstate.lastfraggedcarrier = level.time;
		addscore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * tokens * tokens);
		attacker->client->pers.teamstate.fragcarrier++;
		PrintMsg(nil, "%s" S_COLOR_WHITE " fragged %s's skull carrier!\n",
			 attacker->client->pers.netname, teamname(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;
			if(ent->inuse && ent->client->sess.team == otherteam)
				ent->client->pers.teamstate.lasthurtcarrier = 0;
		}
		return;
	}

	if(targ->client->pers.teamstate.lasthurtcarrier &&
	   level.time - targ->client->pers.teamstate.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
	   !attacker->client->ps.powerups[flag_pw]){
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamstate.carrierdefense++;
		targ->client->pers.teamstate.lasthurtcarrier = 0;

		giveaward(attacker->client, AWARD_DEFEND);

		return;
	}

	if(targ->client->pers.teamstate.lasthurtcarrier &&
	   level.time - targ->client->pers.teamstate.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT){
		// attacker is on the same team as the skull carrier and
		addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamstate.carrierdefense++;
		targ->client->pers.teamstate.lasthurtcarrier = 0;

		giveaward(attacker->client, AWARD_DEFEND);

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

#ifdef MISSIONPACK
	if(g_gametype.integer == GT_OBELISK){
		// find the team obelisk
		switch(attacker->client->sess.team){
		case TEAM_RED:
			c = "team_redobelisk";
			break;
		case TEAM_BLUE:
			c = "team_blueobelisk";
			break;
		default:
			return;
		}
	}else if(g_gametype.integer == GT_HARVESTER)
		// find the center obelisk
		c = "team_neutralobelisk";
	else{
#endif
	// find the flag
	switch(attacker->client->sess.team){
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}
	// find attacker's team's flag carrier
	for(i = 0; i < g_maxclients.integer; i++){
		carrier = g_entities + i;
		if(carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = nil;
	}
#ifdef MISSIONPACK
}

#endif
	flag = nil;
	while((flag = findent(flag, FOFS(classname), c)) != nil)
		if(!(flag->flags & FL_DROPPED_ITEM))
			break;

	if(!flag)
		return;		// can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	vecsub(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	vecsub(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if(((veclen(v1) < CTF_TARGET_PROTECT_RADIUS &&
	     trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin)) ||
	    (veclen(v2) < CTF_TARGET_PROTECT_RADIUS &&
	     trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin))) &&
	   attacker->client->sess.team != targ->client->sess.team){
		// we defended the base flag
		addscore(attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamstate.basedefense++;

		giveaward(attacker->client, AWARD_DEFEND);

		return;
	}

	if(carrier && carrier != attacker){
		vecsub(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		vecsub(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if(((veclen(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
		     trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin)) ||
		    (veclen(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
		     trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin))) &&
		   attacker->client->sess.team != targ->client->sess.team){
			addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamstate.carrierdefense++;

			giveaward(attacker->client, AWARD_DEFEND);

			return;
		}
	}
}

/*
Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
*/
void
chkhurtcarrier(gentity_t *targ, gentity_t *attacker)
{
	int flag_pw;

	if(!targ->client || !attacker->client)
		return;

	if(targ->client->sess.team == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	// flags
	if(targ->client->ps.powerups[flag_pw] &&
	   targ->client->sess.team != attacker->client->sess.team)
		attacker->client->pers.teamstate.lasthurtcarrier = level.time;

	// skulls
	if(targ->client->ps.generic1 &&
	   targ->client->sess.team != attacker->client->sess.team)
		attacker->client->pers.teamstate.lasthurtcarrier = level.time;
}

gentity_t *
flagreset(int team)
{
	char *c;
	gentity_t *ent, *rent = nil;

	switch(team){
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	case TEAM_FREE:
		c = "team_CTF_neutralflag";
		break;
	default:
		return nil;
	}

	ent = nil;
	while((ent = findent(ent, FOFS(classname), c)) != nil){
		if(ent->flags & FL_DROPPED_ITEM)
			entfree(ent);
		else{
			rent = ent;
			itemrespawn(ent);
		}
	}

	setflagstatus(team, FLAG_ATBASE);

	return rent;
}

void
flagresetall(void)
{
	if(g_gametype.integer == GT_CTF){
		flagreset(TEAM_RED);
		flagreset(TEAM_BLUE);
	}
#ifdef MISSIONPACK
	else if(g_gametype.integer == GT_1FCTF)
		flagreset(TEAM_FREE);

#endif
}

void
flagreturnevent(gentity_t *ent, int team)
{
	gentity_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_ReturnFlagSound\n");
		return;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_RED_RETURN;
	else
		te->s.eventParm = GTS_BLUE_RETURN;
	te->r.svFlags |= SVF_BROADCAST;
}

void
flagtakeevent(gentity_t *ent, int team)
{
	gentity_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_TakeFlagSound\n");
		return;
	}

	// only play sound when the flag was at the base
	// or not picked up the last 10 seconds
	switch(team){
	case TEAM_RED:
		if(teamgame.blueStatus != FLAG_ATBASE)
			if(teamgame.blueTakenTime > level.time - 10000)
				return;
		teamgame.blueTakenTime = level.time;
		break;

	case TEAM_BLUE:		// CTF
		if(teamgame.redStatus != FLAG_ATBASE)
			if(teamgame.redTakenTime > level.time - 10000)
				return;
		teamgame.redTakenTime = level.time;
		break;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_RED_TAKEN;
	else
		te->s.eventParm = GTS_BLUE_TAKEN;
	te->r.svFlags |= SVF_BROADCAST;
}

void
flagcaptureevent(gentity_t *ent, int team)
{
	gentity_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_CaptureFlagSound\n");
		return;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_BLUE_CAPTURE;
	else
		te->s.eventParm = GTS_RED_CAPTURE;
	te->r.svFlags |= SVF_BROADCAST;
}

void
flagreturn(int team)
{
	flagreturnevent(flagreset(team), team);
	if(team == TEAM_FREE)
		PrintMsg(nil, "The flag has returned!\n");
	else
		PrintMsg(nil, "The %s flag has returned!\n", teamname(team));
}

void
teamfreeent(gentity_t *ent)
{
	if(ent->item->tag == PW_REDFLAG)
		flagreturn(TEAM_RED);
	else if(ent->item->tag == PW_BLUEFLAG)
		flagreturn(TEAM_BLUE);
	else if(ent->item->tag == PW_NEUTRALFLAG)
		flagreturn(TEAM_FREE);
}

/*
Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
*/
void
droppedflag_think(gentity_t *ent)
{
	int team = TEAM_FREE;

	if(ent->item->tag == PW_REDFLAG)
		team = TEAM_RED;
	else if(ent->item->tag == PW_BLUEFLAG)
		team = TEAM_BLUE;
	else if(ent->item->tag == PW_NEUTRALFLAG)
		team = TEAM_FREE;

	flagreturnevent(flagreset(team), team);
	// Reset Flag will delete this entity
}

int
touchenemyflag(gentity_t *ent, gentity_t *other, int team)
{
	int i;
	gentity_t *player;
	gclient_t *cl = other->client;
	int enemy_flag;

#ifdef MISSIONPACK
	if(g_gametype.integer == GT_1FCTF)
		enemy_flag = PW_NEUTRALFLAG;
	else{
#endif
	if(cl->sess.team == TEAM_RED)
		enemy_flag = PW_BLUEFLAG;
	else
		enemy_flag = PW_REDFLAG;
	}

	if(ent->flags & FL_DROPPED_ITEM){
		// hey, it's not home.  return it by teleporting it back
		PrintMsg(nil, "%s" S_COLOR_WHITE " returned the %s flag!\n",
			 cl->pers.netname, teamname(team));
		addscore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
		other->client->pers.teamstate.flagrecovery++;
		other->client->pers.teamstate.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		flagreturnevent(flagreset(team), team);
		return 0;
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if(!cl->ps.powerups[enemy_flag])
		return 0;	// We don't have the flag
	PrintMsg(nil, "%s" S_COLOR_WHITE " captured the %s flag!\n", cl->pers.netname, teamname(getotherteam(team)));

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	addteamscore(ent->s.pos.trBase, other->client->sess.team, 1);
	forcegesture(other->client->sess.team);

	other->client->pers.teamstate.captures++;

	giveaward(other->client, AWARD_CAPTURE);

	// other gets another 10 frag bonus
	addscore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);

	flagcaptureevent(ent, team);

	// Ok, let's do the player loop, hand out the bonuses
	for(i = 0; i < g_maxclients.integer; i++){
		player = &g_entities[i];

		// also make sure we don't award assist bonuses to the flag carrier himself.
		if(!player->inuse || player == other)
			continue;

		if(player->client->sess.team !=
		   cl->sess.team)
			player->client->pers.teamstate.lasthurtcarrier = -5;
		else if(player->client->sess.team ==
			cl->sess.team){
#ifdef MISSIONPACK
			addscore(player, ent->r.currentOrigin, CTF_TEAM_BONUS);
#endif
			// award extra points for capture assists
			if(player->client->pers.teamstate.lastreturnedflag +
			   CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time){
				addscore(player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamstate.assists++;

				giveaward(player->client, AWARD_ASSIST);
			}
			if(player->client->pers.teamstate.lastfraggedcarrier +
			   CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time){
				addscore(player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamstate.assists++;

				giveaward(player->client, AWARD_ASSIST);
			}
		}
	}
	flagresetall();

	calcranks();

	return 0;	// Do not respawn this automatically
}

int
touchourflag(gentity_t *ent, gentity_t *other, int team)
{
	gclient_t *cl = other->client;

#ifdef MISSIONPACK
	if(g_gametype.integer == GT_1FCTF){
		PrintMsg(nil, "%s" S_COLOR_WHITE " got the flag!\n", other->client->pers.netname);

		cl->ps.powerups[PW_NEUTRALFLAG] = INT_MAX;	// flags never expire

		if(team == TEAM_RED)
			setflagstatus(TEAM_FREE, FLAG_TAKEN_RED);
		else
			setflagstatus(TEAM_FREE, FLAG_TAKEN_BLUE);
	}else{
#endif
	PrintMsg(nil, "%s" S_COLOR_WHITE " got the %s flag!\n",
		 other->client->pers.netname, teamname(team));

	if(team == TEAM_RED)
		cl->ps.powerups[PW_REDFLAG] = INT_MAX;		// flags never expire
	else
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX;		// flags never expire

	setflagstatus(team, FLAG_TAKEN);
#ifdef MISSIONPACK
}

addscore(other, ent->r.currentOrigin, CTF_FLAG_BONUS);
#endif
	cl->pers.teamstate.flagsince = level.time;
	flagtakeevent(ent, team);

	return -1;	// Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int
pickupteam(gentity_t *ent, gentity_t *other)
{
	int team;
	gclient_t *cl = other->client;

#ifdef MISSIONPACK
	if(g_gametype.integer == GT_OBELISK){
		// there are no team items that can be picked up in obelisk
		entfree(ent);
		return 0;
	}

	if(g_gametype.integer == GT_HARVESTER){
		// the only team items that can be picked up in harvester are the cubes
		if(ent->spawnflags != cl->sess.team)
			cl->ps.generic1 += 1;
		entfree(ent);
		return 0;
	}
#endif
	// figure out what team this flag is
	if(strcmp(ent->classname, "team_CTF_redflag") == 0)
		team = TEAM_RED;
	else if(strcmp(ent->classname, "team_CTF_blueflag") == 0)
		team = TEAM_BLUE;

#ifdef MISSIONPACK
	else if(strcmp(ent->classname, "team_CTF_neutralflag") == 0)
		team = TEAM_FREE;

#endif
	else{
		PrintMsg(other, "Don't know what team the flag is on.\n");
		return 0;
	}
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_1FCTF){
		if(team == TEAM_FREE)
			return touchourflag(ent, other, cl->sess.team);
		if(team != cl->sess.team)
			return touchenemyflag(ent, other, cl->sess.team);
		return 0;
	}
#endif
	// GT_CTF
	if(team == cl->sess.team)
		return touchenemyflag(ent, other, team);
	return touchourflag(ent, other, team);
}

/*
Report a location for the player. Uses placed nearby target_location entities
*/
gentity_t *
teamgetlocation(gentity_t *ent)
{
	gentity_t *eloc, *best;
	float bestlen, len;
	vec3_t origin;

	best = nil;
	bestlen = 3*8192.0*8192.0;

	veccpy(ent->r.currentOrigin, origin);

	for(eloc = level.lochead; eloc; eloc = eloc->nexttrain){
		len = (origin[0] - eloc->r.currentOrigin[0]) * (origin[0] - eloc->r.currentOrigin[0])
		      + (origin[1] - eloc->r.currentOrigin[1]) * (origin[1] - eloc->r.currentOrigin[1])
		      + (origin[2] - eloc->r.currentOrigin[2]) * (origin[2] - eloc->r.currentOrigin[2]);

		if(len > bestlen)
			continue;

		if(!trap_InPVS(origin, eloc->r.currentOrigin))
			continue;

		bestlen = len;
		best = eloc;
	}

	return best;
}

/*
Report a location for the player. Uses placed nearby target_location entities
*/
qboolean
teamgetlocationmsg(gentity_t *ent, char *loc, int loclen)
{
	gentity_t *best;

	best = teamgetlocation(ent);

	if(!best)
		return qfalse;

	if(best->count){
		if(best->count < 0)
			best->count = 0;
		if(best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message);
	}else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}

/*---------------------------------------------------------------------------*/

/*
go to a random point that doesn't telefrag
*/
#define MAX_TEAM_SPAWN_POINTS 32
gentity_t *
findteamspawnpoint(int teamstate, team_t team)
{
	gentity_t *spot;
	int count;
	int selection;
	gentity_t *spots[MAX_TEAM_SPAWN_POINTS];
	char *classname;

	if(teamstate == TEAM_BEGIN){
		if(team == TEAM_RED)
			classname = "team_ctf_redplayer";
		else if(team == TEAM_BLUE)
			classname = "team_ctf_blueplayer";
		else
			return nil;
	}else{
		if(team == TEAM_RED)
			classname = "team_ctf_redspawn";
		else if(team == TEAM_BLUE)
			classname = "team_ctf_bluespawn";
		else
			return nil;
	}
	count = 0;

	spot = nil;

	while((spot = findent(spot, FOFS(classname), classname)) != nil){
		if(maytelefrag(spot))
			continue;
		spots[count] = spot;
		if(++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if(!count)	// no spots that won't telefrag
		return findent(nil, FOFS(classname), classname);

	selection = rand() % count;
	return spots[selection];
}

gentity_t *
findctfspawnpoint(team_t team, int teamstate, vec3_t origin, vec3_t angles, qboolean isbot)
{
	gentity_t *spot;

	spot = findteamspawnpoint(teamstate, team);

	if(!spot)
		return selectspawnpoint(vec3_origin, origin, angles, isbot);

	veccpy(spot->s.origin, origin);
	veccpy(spot->s.angles, angles);

	return spot;
}

gentity_t*
findcpspawnpoint(team_t team, int teamstate, vec3_t origin, vec3_t angles, qboolean isbot)
{
	gentity_t *spot;
	int count;
	int selection;
	gentity_t *spots[MAX_TEAM_SPAWN_POINTS];
	char *classname;

	if(teamstate == TEAM_BEGIN){
		if(team == TEAM_RED)
			classname = "team_ctf_redplayer";
		else if(team == TEAM_BLUE)
			classname = "team_ctf_blueplayer";
		else
			return nil;
	}else{
		if(team == TEAM_RED)
			classname = "team_ctf_redspawn";
		else if(team == TEAM_BLUE)
			classname = "team_ctf_bluespawn";
		else
			return nil;
	}

	count = 0;
	spot = nil;

	while((spot = findent(spot, FOFS(classname), classname)) != nil){
		if(spot->cpround != level.round)
			continue;
		if(maytelefrag(spot))
			continue;
		spots[count] = spot;
		if(++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if(count == 0)
		Com_Error(ERR_DROP, "map has no %s spawnpoints assigned to this round",
		   teamname(team));

	selection = rand() % count;
	veccpy(spots[selection]->s.origin, origin);
	veccpy(spots[selection]->s.angles, angles);
	return spots[selection];
}

/*---------------------------------------------------------------------------*/

static int QDECL
clientcmp(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

/*
Format:
	clientNum location health shield weapon powerups
*/
void
teamplayinfomsg(gentity_t *ent)
{
	char entry[1024];
	char string[8192];
	int stringlength;
	int i, j;
	gentity_t *player;
	int cnt;
	int h, a;
	int clients[TEAM_MAXOVERLAY];
	int team;

	if(!ent->client->pers.teaminfo)
		return;

	// send team info to spectator for team of followed client
	if(ent->client->sess.team == TEAM_SPECTATOR){
		if(ent->client->sess.specstate != SPECTATOR_FOLLOW
		   || ent->client->sess.specclient < 0)
			return;
		team = g_entities[ent->client->sess.specclient].client->sess.team;
	}else
		team = ent->client->sess.team;

	if(team != TEAM_RED && team != TEAM_BLUE)
		return;

	// figure out what client should be on the display
	// we are limited to 8, but we want to use the top eight players
	// but in client order (so they don't keep changing position on the overlay)
	for(i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++){
		player = g_entities + level.sortedclients[i];
		if(player->inuse && player->client->sess.team == team)
			clients[cnt++] = level.sortedclients[i];
	}

	// We have the top eight players, sort them by clientNum
	qsort(clients, cnt, sizeof(clients[0]), clientcmp);

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for(i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++){
		player = g_entities + i;
		if(player->inuse && player->client->sess.team == team){
			h = player->client->ps.stats[STAT_HEALTH];
			a = player->client->ps.stats[STAT_SHIELD];
			if(h < 0) h = 0;
			if(a < 0) a = 0;

			Com_sprintf(entry, sizeof(entry),
				    " %i %i %i %i %i %i",
//				level.sortedclients[i], player->client->pers.teamstate.location, h, a,
				    i, player->client->pers.teamstate.location, h, a,
				    player->client->ps.weapon, player->s.powerups);
			j = strlen(entry);
			if(stringlength + j >= sizeof(string))
				break;
			strcpy(string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand(ent-g_entities, va("tinfo %i %s", cnt, string));
}

void
chkteamstatus(void)
{
	int i;
	gentity_t *loc, *ent;

	if(level.time - level.lastteamlocationtime > TEAM_LOCATION_UPDATE_TIME){
		level.lastteamlocationtime = level.time;

		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;

			if(ent->client->pers.connected != CON_CONNECTED)
				continue;

			if(ent->inuse && (ent->client->sess.team == TEAM_RED || ent->client->sess.team == TEAM_BLUE)){
				loc = teamgetlocation(ent);
				if(loc)
					ent->client->pers.teamstate.location = loc->health;
				else
					ent->client->pers.teamstate.location = 0;
			}
		}

		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;

			if(ent->client->pers.connected != CON_CONNECTED)
				continue;

			if(ent->inuse)
				teamplayinfomsg(ent);
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void
SP_team_CTF_redplayer(gentity_t *ent)
{
	spawnint("round", "1", &ent->cpround);
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void
SP_team_CTF_blueplayer(gentity_t *ent)
{
	spawnint("round", "1", &ent->cpround);
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void
SP_team_CTF_redspawn(gentity_t *ent)
{
	spawnint("round", "1", &ent->cpround);
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void
SP_team_CTF_bluespawn(gentity_t *ent)
{
	spawnint("round", "1", &ent->cpround);
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

#ifdef MISSIONPACK


static void
ObeliskRegen(gentity_t *self)
{
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	if(self->health >= g_obeliskHealth.integer)
		return;

	addevent(self, EV_POWERUP_REGEN, 0);
	self->health += g_obeliskRegenAmount.integer;
	if(self->health > g_obeliskHealth.integer)
		self->health = g_obeliskHealth.integer;

	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	self->activator->s.frame = 0;
}

static void
ObeliskRespawn(gentity_t *self)
{
	self->takedmg = qtrue;
	self->health = g_obeliskHealth.integer;

	self->think = ObeliskRegen;
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;

	self->activator->s.frame = 0;
}

static void
ObeliskDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	int otherTeam;

	otherTeam = getotherteam(self->spawnflags);
	addteamscore(self->s.pos.trBase, otherTeam, 1);
	forcegesture(otherTeam);

	calcranks();

	self->takedmg = qfalse;
	self->think = ObeliskRespawn;
	self->nextthink = level.time + g_obeliskRespawnDelay.integer * 1000;

	self->activator->s.modelindex2 = 0xff;
	self->activator->s.frame = 2;

	addevent(self->activator, EV_OBELISKEXPLODE, 0);

	addscore(attacker, self->r.currentOrigin, CTF_CAPTURE_BONUS);

	giveaward(attacker->client, AWARD_CAPTURE);

	teamgame.redObeliskAttackedTime = 0;
	teamgame.blueObeliskAttackedTime = 0;
}

static void
ObeliskTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int tokens;

	if(!other->client)
		return;

	if(getotherteam(other->client->sess.team) != self->spawnflags)
		return;

	tokens = other->client->ps.generic1;
	if(tokens <= 0)
		return;

	PrintMsg(nil, "%s" S_COLOR_WHITE " brought in %i skull%s.\n",
		 other->client->pers.netname, tokens, tokens ? "s" : "");

	addteamscore(self->s.pos.trBase, other->client->sess.team, tokens);
	forcegesture(other->client->sess.team);

	addscore(other, self->r.currentOrigin, CTF_CAPTURE_BONUS*tokens);

	giveaward(other->client, AWARD_CAPTURE);	// FIXME: captures += tokens

	other->client->ps.generic1 = 0;
	calcranks();

	flagcaptureevent(self, self->spawnflags);
}

static void
ObeliskPain(gentity_t *self, gentity_t *attacker, int damage)
{
	int actualDamage = damage / 10;
	if(actualDamage <= 0)
		actualDamage = 1;
	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	if(!self->activator->s.frame)
		addevent(self, EV_OBELISKPAIN, 0);
	self->activator->s.frame = 1;
	addscore(attacker, self->r.currentOrigin, actualDamage);
}

gentity_t *
spawnobelisk(vec3_t origin, int team, int spawnflags)
{
	trace_t tr;
	vec3_t dest;
	gentity_t *ent;

	ent = entspawn();

	veccpy(origin, ent->s.origin);
	veccpy(origin, ent->s.pos.trBase);
	veccpy(origin, ent->r.currentOrigin);

	vecset(ent->r.mins, -15, -15, 0);
	vecset(ent->r.maxs, 15, 15, 87);

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;

	if(g_gametype.integer == GT_OBELISK){
		ent->r.contents = CONTENTS_SOLID;
		ent->takedmg = qtrue;
		ent->health = g_obeliskHealth.integer;
		ent->die = ObeliskDie;
		ent->pain = ObeliskPain;
		ent->think = ObeliskRegen;
		ent->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	}
	if(g_gametype.integer == GT_HARVESTER){
		ent->r.contents = CONTENTS_TRIGGER;
		ent->touch = ObeliskTouch;
	}

	// suspended
	setorigin(ent, ent->s.origin);

	ent->spawnflags = team;

	trap_LinkEntity(ent);

	return ent;
}

/*QUAKED team_redobelisk (1 0 0) (-16 -16 0) (16 16 8)
*/
void
SP_team_redobelisk(gentity_t *ent)
{
	gentity_t *obelisk;

	if(g_gametype.integer <= GT_TEAM){
		entfree(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if(g_gametype.integer == GT_OBELISK){
		obelisk = spawnobelisk(ent->s.origin, TEAM_RED, ent->spawnflags);
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if(g_gametype.integer == GT_HARVESTER){
		obelisk = spawnobelisk(ent->s.origin, TEAM_RED, ent->spawnflags);
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_RED;
	trap_LinkEntity(ent);
}

/*QUAKED team_blueobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void
SP_team_blueobelisk(gentity_t *ent)
{
	gentity_t *obelisk;

	if(g_gametype.integer <= GT_TEAM){
		entfree(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if(g_gametype.integer == GT_OBELISK){
		obelisk = spawnobelisk(ent->s.origin, TEAM_BLUE, ent->spawnflags);
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if(g_gametype.integer == GT_HARVESTER){
		obelisk = spawnobelisk(ent->s.origin, TEAM_BLUE, ent->spawnflags);
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_BLUE;
	trap_LinkEntity(ent);
}

/*QUAKED team_neutralobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void
SP_team_neutralobelisk(gentity_t *ent)
{
	if(g_gametype.integer != GT_1FCTF && g_gametype.integer != GT_HARVESTER){
		entfree(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if(g_gametype.integer == GT_HARVESTER){
		neutralObelisk = spawnobelisk(ent->s.origin, TEAM_FREE, ent->spawnflags);
		neutralObelisk->spawnflags = TEAM_FREE;
	}
	ent->s.modelindex = TEAM_FREE;
	trap_LinkEntity(ent);
}

qboolean
chkobeliskattacked(gentity_t *obelisk, gentity_t *attacker)
{
	gentity_t *te;

	// if this really is an obelisk
	if(obelisk->die != ObeliskDie)
		return qfalse;

	// if the attacker is a client
	if(!attacker->client)
		return qfalse;

	// if the obelisk is on the same team as the attacker then don't hurt it
	if(obelisk->spawnflags == attacker->client->sess.team)
		return qtrue;

	// obelisk may be hurt

	// if not played any sounds recently
	if((obelisk->spawnflags == TEAM_RED &&
	    teamgame.redObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME) ||
	   (obelisk->spawnflags == TEAM_BLUE &&
	    teamgame.blueObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME)){
		// tell which obelisk is under attack
		te = enttemp(obelisk->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
		if(obelisk->spawnflags == TEAM_RED){
			te->s.eventParm = GTS_REDOBELISK_ATTACKED;
			teamgame.redObeliskAttackedTime = level.time;
		}else{
			te->s.eventParm = GTS_BLUEOBELISK_ATTACKED;
			teamgame.blueObeliskAttackedTime = level.time;
		}
		te->r.svFlags |= SVF_BROADCAST;
	}

	return qfalse;
}

#endif

/*QUAKED team_cp_config (0 0 1) (-8 -8 -8) (8 8 8)
 * 
 * Controls CP game parameters.
 * 
 * numrounds            number of CP rounds in this map (1 default)
 */
void
SP_team_cp_config(gentity_t *ent)
{
	if(g_gametype.integer != GT_CP){
		entfree(ent);
		return;
	}
	ent->r.svFlags |= SVF_NOCLIENT;
	spawnint("numrounds", "1", &level.numcprounds);
	trap_LinkEntity(ent);
}

/*QUAKED team_cp_round_timer (0 0 1) (-8 -8 -8) (8 8 8)
 *
 * Configures timing for one stage of a CP game.
 * Also activates targets when the setup period ends, which is used to open
 * spawnroom gates.
 * 
 * round		round to which this timer is relevant (1 default)
 * timelimit		initial time limit for this round, in seconds (180 default)
 * maxtimelimit		maximum the time limit can be bumped for overtime etc. (240 default)
 * setuptime		setup time before gates open, in seconds (20 default)
 * attackingteam	attacking team this round ("blue" default)
 */
void
SP_team_cp_round_timer(gentity_t *ent)
{
	if(g_gametype.integer != GT_CP){
		entfree(ent);
		return;
	}
	ent->r.svFlags |= SVF_NOCLIENT;
	spawnint("round", "1", &ent->cpround);
	spawnint("timelimit", "180", &ent->cptimelimit);
	spawnint("setuptime", "20", &ent->cpsetuptime);
	spawnint("maxtimelimit", "240", &ent->cpmaxtimelimit);
	spawnstr("attackingteam", "blue", &ent->cpattackingteam);
	trap_LinkEntity(ent);
}

/*QUAKED team_cp_round_end (0 0 1) (-8 -8 -8) (8 8 8)
 * 
 * Triggers its targets when a GT_CP round ends.
 * 
 * onredwin		triggered if red won (1 default)
 * onbluewin		triggered if blue won (1 default)
 * onstalemate		triggered if there was a stalemate (1 default)
 * onround		triggered only on round n (-1 default)
 */
void
SP_team_cp_round_end(gentity_t *ent)
{
	if(g_gametype.integer != GT_CP){
		entfree(ent);
		return;
	}
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

/*QUAKED team_cp_controlpoint (0 0 1) (-8 -8 -8) (8 8 8)
 * 
 * capturerate		capture time in seconds (12 default)
 * captureorder		capture order of this control point relative to others (0 default (= master))
 * round		round in which this control point is active (0 default)
 */
void
SP_team_cp_controlpoint(gentity_t *ent)
{
	int i;

	if(g_gametype.integer != GT_CP){
		entfree(ent);
		return;
	}
	spawnfloat("capturerate", "6", &ent->cpcaprate);
	spawnint("captureorder", "0", &ent->cporder);
	spawnint("round", "0", &ent->cpround);
	// insert ourself into control point array
	for(i = 0; i < ARRAY_LEN(level.cp); i++){
		if(level.cp[i] == nil){
			level.cp[i] = ent;
			break;
		}
	}
	// tell clients to precache all the models
	ent->s.modelindex = getmodelindex("models/powerups/controlpoint_free.md3");
	getmodelindex("models/powerups/controlpoint_red.md3");
	getmodelindex("models/powerups/controlpoint_blue.md3");
	trap_LinkEntity(ent);
}
