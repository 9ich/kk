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
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
If the ammo has gone low enough to generate the warning, play a sound
*/
void
chkammo(void)
{
	int i;
	int total;
	int previous;
	int weapons;

	// see about how many seconds of ammo we have remaining
	weapons = cg.snap->ps.stats[STAT_WEAPONS];
	total = 0;
	for(i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++){
		if(!(weapons & (1 << i)))
			continue;
		switch(i){
		case WP_ROCKET_LAUNCHER:
		case WP_HOMING_LAUNCHER:
		case WP_GRENADE_LAUNCHER:
		case WP_RAILGUN:
		case WP_SHOTGUN:
#ifdef MISSIONPACK
		case WP_PROX_LAUNCHER:
#endif
			total += cg.snap->ps.ammo[i] * 1000;
			break;
		default:
			total += cg.snap->ps.ammo[i] * 200;
			break;
		}
		if(total >= 5000){
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	previous = cg.lowAmmoWarning;

	if(total == 0)
		cg.lowAmmoWarning = 2;
	else
		cg.lowAmmoWarning = 1;

	// play a sound on transitions
	if(cg.lowAmmoWarning != previous)
		trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
}

void
dmgfeedback(int yawbyte, int pitchbyte, int damage)
{
	float kick;
	int health;
	float scale;

	// show the attacking player's head and name in corner
	cg.attackertime = cg.time;

	health = cg.snap->ps.stats[STAT_HEALTH];
	if(health < 40)
		scale = 1;
	else
		scale = 40.0 / health;
	kick = damage * scale;

	if(kick < 5)
		kick = 5;
	if(kick > 10)
		kick = 10;

	cg.dmgval = kick;
	cg.vdmgtime = cg.time + DAMAGE_TIME;
	cg.dmgtime = cg.snap->serverTime;
}

/*
A respawn happened this snapshot
*/
void
respawn(void)
{
	// no error decay on player movement
	cg.teleportthisframe = qtrue;

	// display weapons available
	cg.weapseltime[0] = cg.time;
	cg.weapseltime[1] = cg.time;

	// select the weapon the server says we are using
	cg.weapsel[0] = cg.snap->ps.weapon[0];
	cg.weapsel[1] = cg.snap->ps.weapon[1];
	cg.weapsel[2] = cg.snap->ps.weapon[2];
}

extern char *eventnames[];

void
chkplayerstateevents(playerState_t *ps, playerState_t *ops)
{
	int i;
	int event;
	centity_t *cent;

	if(ps->externalEvent && ps->externalEvent != ops->externalEvent){
		cent = &cg_entities[ps->clientNum];
		cent->currstate.event = ps->externalEvent;
		cent->currstate.eventParm = ps->externalEventParm;
		entevent(cent, cent->lerporigin);
	}

	cent = &cg.pplayerent;	// cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++)
		// if we have a new predictable event
		if(i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
		   || (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)])){
			event = ps->events[i & (MAX_PS_EVENTS-1)];
			cent->currstate.event = event;
			cent->currstate.eventParm = ps->eventParms[i & (MAX_PS_EVENTS-1)];
			entevent(cent, cent->lerporigin);

			cg.pevents[i & (MAX_PREDICTED_EVENTS-1)] = event;

			cg.eventSequence++;
		}
}

void
chkpredictableevents(playerState_t *ps)
{
	int i;
	int event;
	centity_t *cent;

	cent = &cg.pplayerent;
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++){
		if(i >= cg.eventSequence)
			continue;
		// if this event is not further back in than the maximum predictable events we remember
		if(i > cg.eventSequence - MAX_PREDICTED_EVENTS)
			// if the new playerstate event is different from a previously predicted one
			if(ps->events[i & (MAX_PS_EVENTS-1)] != cg.pevents[i & (MAX_PREDICTED_EVENTS-1)]){
				event = ps->events[i & (MAX_PS_EVENTS-1)];
				cent->currstate.event = event;
				cent->currstate.eventParm = ps->eventParms[i & (MAX_PS_EVENTS-1)];
				entevent(cent, cent->lerporigin);

				cg.pevents[i & (MAX_PREDICTED_EVENTS-1)] = event;

				if(cg_showmiss.integer)
					cgprintf("WARNING: changed predicted event\n");
			}
	}
}

void
pushreward(sfxHandle_t sfx, qhandle_t shader, const char *msg, int nrewards)
{
	if(cg.rewardstack < (MAX_REWARDSTACK-1)){
		cg.rewardstack++;
		cg.rewardsounds[cg.rewardstack] = sfx;
		cg.rewardshaders[cg.rewardstack] = shader;
		Q_strncpyz(cg.rewardmsgs[cg.rewardstack], msg,
		   sizeof cg.rewardmsgs[cg.rewardstack]);
		cg.nrewards[cg.rewardstack] = nrewards;
	}
}

void
chklocalsounds(playerState_t *ps, playerState_t *ops)
{
	int highScore, reward;
	int i;

	// don't play the sounds if the player just changed teams
	if(ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM])
		return;

	// hit changes
	if(ps->persistant[PERS_DMG] > ops->persistant[PERS_DMG])
		trap_S_StartLocalSound(cgs.media.hitSound, CHAN_LOCAL_SOUND);

	// health changes of more than -1 should make pain sounds
	if(ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1)
		if(ps->stats[STAT_HEALTH] > 0)
			painevent(&cg.pplayerent, ps->stats[STAT_HEALTH]);


	// if we are going into the intermission, don't start any voices
	if(cg.intermissionstarted)
		return;

	// play award sounds
	reward = qfalse;

	for(i = 0; i < cg_nawardlist; i++){
		sfxHandle_t sfx;
		qhandle_t shader;

		sfx = 0;
		shader = 0;

		if(ps->awards[cg_awardlist[i].award] != ops->awards[cg_awardlist[i].award]){
			if(cg_awardlist[i].sfx != nil && *cg_awardlist[i].sfx != 0)
				sfx = trap_S_RegisterSound(cg_awardlist[i].sfx, qtrue);
			if(cg_awardlist[i].shader != nil && *cg_awardlist[i].shader != 0)
				shader = trap_R_RegisterShader(cg_awardlist[i].shader);
			pushreward(sfx, shader, cg_awardlist[i].msg,
			   ps->awards[cg_awardlist[i].award]);
			reward = qtrue;
		}
	}

	// check for flag pickup
	if(cgs.gametype > GT_TEAM)
		if((ps->powerups[PW_REDFLAG] != ops->powerups[PW_REDFLAG] && ps->powerups[PW_REDFLAG]) ||
		   (ps->powerups[PW_BLUEFLAG] != ops->powerups[PW_BLUEFLAG] && ps->powerups[PW_BLUEFLAG]) ||
		   (ps->powerups[PW_NEUTRALFLAG] != ops->powerups[PW_NEUTRALFLAG] && ps->powerups[PW_NEUTRALFLAG]))
			trap_S_StartLocalSound(cgs.media.youHaveFlagSound, CHAN_ANNOUNCER);

	// lead changes
	if(!reward)
		if(!cg.warmup){
			// never play lead changes during warmup
			if(ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK])
				if(cgs.gametype < GT_TEAM){
					if(ps->persistant[PERS_RANK] == 0)
						addbufferedsound(cgs.media.takenLeadSound);
					else if(ps->persistant[PERS_RANK] == RANK_TIED_FLAG)
						addbufferedsound(cgs.media.tiedLeadSound);
					else if((ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG) == 0)
						addbufferedsound(cgs.media.lostLeadSound);
				}
		}

	// timelimit warnings
	if(cgs.timelimit > 0){
		int msec;

		msec = cg.time - cgs.levelStartTime;
		if(!(cg.timelimitwarnings & 4) && msec > (cgs.timelimit * 60 + 2) * 1000){
			cg.timelimitwarnings |= 1 | 2 | 4;
			trap_S_StartLocalSound(cgs.media.suddenDeathSound, CHAN_ANNOUNCER);
		}else if(!(cg.timelimitwarnings & 2) && msec > (cgs.timelimit - 1) * 60 * 1000){
			cg.timelimitwarnings |= 1 | 2;
			trap_S_StartLocalSound(cgs.media.oneMinuteSound, CHAN_ANNOUNCER);
		}else if(cgs.timelimit > 5 && !(cg.timelimitwarnings & 1) && msec > (cgs.timelimit - 5) * 60 * 1000){
			cg.timelimitwarnings |= 1;
			trap_S_StartLocalSound(cgs.media.fiveMinuteSound, CHAN_ANNOUNCER);
		}
	}

	// fraglimit warnings
	if(cgs.fraglimit > 0 && cgs.gametype < GT_CTF){
		highScore = cgs.scores1;

		if(cgs.gametype == GT_TEAM && cgs.scores2 > highScore)
			highScore = cgs.scores2;

		if(!(cg.fraglimitwarnings & 4) && highScore == (cgs.fraglimit - 1)){
			cg.fraglimitwarnings |= 1 | 2 | 4;
			addbufferedsound(cgs.media.oneFragSound);
		}else if(cgs.fraglimit > 2 && !(cg.fraglimitwarnings & 2) && highScore == (cgs.fraglimit - 2)){
			cg.fraglimitwarnings |= 1 | 2;
			addbufferedsound(cgs.media.twoFragSound);
		}else if(cgs.fraglimit > 3 && !(cg.fraglimitwarnings & 1) && highScore == (cgs.fraglimit - 3)){
			cg.fraglimitwarnings |= 1;
			addbufferedsound(cgs.media.threeFragSound);
		}
	}
}

void
pstransition(playerState_t *ps, playerState_t *ops)
{
	// check for changing follow mode
	if(ps->clientNum != ops->clientNum){
		cg.teleportthisframe = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if(ps->damageEvent != ops->damageEvent && ps->damageCount)
		dmgfeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);

	// respawning
	if(ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
		respawn();

	if(cg.maprestart){
		respawn();
		cg.maprestart = qfalse;
	}

	if(cg.snap->ps.pm_type != PM_INTERMISSION
	   && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
		chklocalsounds(ps, ops);

	// check for going low on ammo
	chkammo();

	// run events
	chkplayerstateevents(ps, ops);

	// smooth the ducking viewheight change
	if(ps->viewheight != ops->viewheight){
		cg.duckchange = ps->viewheight - ops->viewheight;
		cg.ducktime = cg.time;
	}
}
