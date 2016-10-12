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
// g_combat.c

#include "g_local.h"

/*
============
ScorePlum
============
*/
void
scoreplum(gentity_t *ent, vec3_t origin, int score)
{
	gentity_t *plum;

	plum = enttemp(origin, EV_SCOREPLUM);
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

/*
============
addscore

Adds score to both the client and his team
============
*/
void
addscore(gentity_t *ent, vec3_t origin, int score)
{
	if(!ent->client)
		return;
	// no scoring during pre-match warmup
	if(level.warmuptime)
		return;
	// show score plum
	scoreplum(ent, origin, score);
	ent->client->ps.persistant[PERS_SCORE] += score;
	if(g_gametype.integer == GT_TEAM)
		level.teamscores[ent->client->ps.persistant[PERS_TEAM]] += score;
	calcranks();
}

/*
=================
tossclientitems

Toss the weapon and powerups for the killed player
=================
*/
void
tossclientitems(gentity_t *self)
{
	gitem_t *item;
	int weapon;
	float angle;
	int i;
	gentity_t *drop;

	// players drop nothing in CA
	if(g_gametype.integer == GT_CA)
		return;

	// drop the weapon if not a gauntlet or machinegun
	for(i = 0; i < 3; i++){
		weapon = self->s.weapon[i];

		// make a special check to see if they are changing to a new
		// weapon that isn't the mg or gauntlet.  Without this, a client
		// can pick up a weapon, be killed, and not drop the weapon because
		// their weapon change hasn't completed yet and they are still holding the MG.
		if(weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK){
			if(self->client->ps.weaponstate[i] == WEAPON_DROPPING)
				weapon = self->client->pers.cmd.weapon[i];
			if(!(self->client->ps.stats[STAT_WEAPONS] & (1 << weapon)))
				weapon = WP_NONE;
		}

		if(weapon > WP_MACHINEGUN && weapon != WP_GRAPPLING_HOOK &&
		   self->client->ps.ammo[weapon]){
			// find the item type for this weapon
			item = finditemforweapon(weapon);

			// spawn the item
			itemdrop(self, item, 0);
		}
	}

	// drop all the powerups if not in teamplay
	if(g_gametype.integer != GT_TEAM){
		angle = 45;
		for(i = 1; i < PW_NUM_POWERUPS; i++)
			if(self->client->ps.powerups[i] > level.time){
				item = finditemforpowerup(i);
				if(!item)
					continue;
				drop = itemdrop(self, item, angle);
				// decide how many seconds it has left
				drop->count = (self->client->ps.powerups[i] - level.time) / 1000;
				if(drop->count < 1)
					drop->count = 1;
				angle += 45;
			}
	}
}

#ifdef MISSIONPACK

/*
=================
TossClientCubes
=================
*/
extern gentity_t *neutralObelisk;

void
tossclientcubes(gentity_t *self)
{
	gitem_t *item;
	gentity_t *drop;
	vec3_t velocity;
	vec3_t angles;
	vec3_t origin;

	self->client->ps.generic1 = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if(!numentsfree())
		return;

	if(self->client->sess.team == TEAM_RED)
		item = finditem("Red Cube");
	else
		item = finditem("Blue Cube");

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	anglevecs(angles, velocity, nil, nil);
	vecmul(velocity, 150, velocity);
	velocity[2] += 200 + crandom() * 50;

	if(neutralObelisk){
		veccpy(neutralObelisk->s.pos.trBase, origin);
		origin[2] += 44;
	}else
		vecclear(origin);

	drop = itemlaunch(item, origin, velocity);

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = entfree;
	drop->spawnflags = self->client->sess.team;
}

/*
=================
TossClientPersistantPowerups
=================
*/
void
tossclientpowerups(gentity_t *ent)
{
	gentity_t *powerup;

	if(!ent->client)
		return;

	if(!ent->client->persistantPowerup)
		return;

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity(powerup);

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = nil;
}

#endif

/*
==================
LookAtKiller
==================
*/
void
lookatkiller(gentity_t *self, gentity_t *inflictor, gentity_t *attacker)
{
	vec3_t dir;

	if(attacker && attacker != self)
		vecsub(attacker->s.pos.trBase, self->s.pos.trBase, dir);
	else if(inflictor && inflictor != self)
		vecsub(inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	else{
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw(dir);
}

/*
==================
GibEntity
==================
*/
void
entgib(gentity_t *self, int killer)
{
	gentity_t *ent;
	int i;

	//if this entity still has kamikaze
	if(self->s.eFlags & EF_KAMIKAZE)
		// check if there is a kamikaze timer around for this owner
		for(i = 0; i < level.nentities; i++){
			ent = &g_entities[i];
			if(!ent->inuse)
				continue;
			if(ent->activator != self)
				continue;
			if(strcmp(ent->classname, "kamikaze timer"))
				continue;
			entfree(ent);
			break;
		}
	addevent(self, EV_GIB_PLAYER, killer);
	self->takedmg = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void
body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath)
{
	if(self->health > GIB_HEALTH)
		return;
	if(!g_blood.integer){
		self->health = GIB_HEALTH+1;
		return;
	}

	entgib(self, 0);
}

// these are just for logging, the client prints its own messages
char *modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_PLASMA",
	"MOD_PLASMA_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
#ifdef MISSIONPACK
	"MOD_NAIL",
	"MOD_CHAINGUN",
	"MOD_PROXIMITY_MINE",
	"MOD_KAMIKAZE",
	"MOD_JUICED",
#endif
	"MOD_GRAPPLE"
};

#ifdef MISSIONPACK
/*
==================
Kamikaze_DeathActivate
==================
*/
void
Kamikaze_DeathActivate(gentity_t *ent)
{
	G_StartKamikaze(ent);
	entfree(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void
Kamikaze_DeathTimer(gentity_t *self)
{
	gentity_t *ent;

	ent = entspawn();
	ent->classname = "kamikaze timer";
	veccpy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}

#endif

/*
==================
CheckAlmostCapture
==================
*/
void
chkalmostcaptured(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent;
	vec3_t dir;
	char *classname;

	// if this player was carrying a flag
	if(self->client->ps.powerups[PW_REDFLAG] ||
	   self->client->ps.powerups[PW_BLUEFLAG] ||
	   self->client->ps.powerups[PW_NEUTRALFLAG]){
		// get the goal flag this player should have been going for
		if(g_gametype.integer == GT_CTF){
			if(self->client->sess.team == TEAM_BLUE)
				classname = "team_CTF_blueflag";
			else
				classname = "team_CTF_redflag";
		}else{
			if(self->client->sess.team == TEAM_BLUE)
				classname = "team_CTF_redflag";
			else
				classname = "team_CTF_blueflag";
		}
		ent = nil;
		do
			ent = findent(ent, FOFS(classname), classname);
		while(ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if(ent && !(ent->r.svFlags & SVF_NOCLIENT)){
			// if the player was *very* close
			vecsub(self->client->ps.origin, ent->s.origin, dir);
			if(veclen(dir) < 200){
				giveaward(self->client, AWARD_DENIED);
				if(attacker->client)
					giveaward(attacker->client, AWARD_DENIED);
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void
chkalmostscored(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent;
	vec3_t dir;
	char *classname;

	// if the player was carrying cubes
	if(self->client->ps.generic1){
		if(self->client->sess.team == TEAM_BLUE)
			classname = "team_redobelisk";
		else
			classname = "team_blueobelisk";
		ent = findent(nil, FOFS(classname), classname);
		// if we found the destination obelisk
		if(ent){
			// if the player was *very* close
			vecsub(self->client->ps.origin, ent->s.origin, dir);
			if(veclen(dir) < 200){
				giveaward(self->client, AWARD_DENIED);
				if(attacker->client)
					giveaward(attacker->client, AWARD_DENIED);
			}
		}
	}
}

/*
attacker has just got at least a doublekill, so give awards as appropriate
*/
static void
chkmultikill(gentity_t *attacker)
{
	gclient_t *cl;

	cl = attacker->client;

	if(cl->lastmultikill == 0 ||
	   cl->lastmultikilltime != cl->lastkilltime){
	   	// first award
		giveaward(cl, AWARD_DOUBLEKILL);
		cl->lastmultikill = AWARD_DOUBLEKILL;
		cl->lastmultikilltime = level.time;
	} else if(cl->lastmultikill < AWARD_HOLYSHIT){
		// subsequent awards up to AWARD_HOLYSHIT
		giveaward(cl, ++cl->lastmultikill);
		cl->lastmultikilltime = level.time;
	}
}

/*
==================
player_die
==================
*/
void
player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath)
{
	gentity_t *ent;
	int anim;
	int contents;
	int killer;
	int i;
	char *killername, *obit;

	if(self->client->ps.pm_type == PM_DEAD)
		return;

	if(level.intermissiontime)
		return;

	// check for an almost capture
	chkalmostcaptured(self, attacker);
	// check for a player that almost brought in cubes
	chkalmostscored(self, attacker);

	if(self->client && self->client->hook)
		weapon_hook_free(self->client->hook);

#ifdef MISSIONPACK
	if((self->client->ps.eFlags & EF_TICKING) && self->activator){
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = entfree;
		self->activator->nextthink = level.time;
	}
#endif
	self->client->ps.pm_type = PM_DEAD;

	if(attacker){
		killer = attacker->s.number;
		if(attacker->client)
			killername = attacker->client->pers.netname;
		else
			killername = "<non-client>";
	}else{
		killer = ENTITYNUM_WORLD;
		killername = "<world>";
	}

	if(killer < 0 || killer >= MAX_CLIENTS){
		killer = ENTITYNUM_WORLD;
		killername = "<world>";
	}

	if(meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN(modNames))
		obit = "<bad obituary>";
	else
		obit = modNames[meansOfDeath];

	logprintf("Kill: %i %i %i: %s killed %s by %s\n",
		    killer, self->s.number, meansOfDeath, killername,
		    self->client->pers.netname, obit);

	// broadcast the death event to everyone
	ent = enttemp(self->r.currentOrigin, EV_OBITUARY);
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST;	// send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	// if victim has died three times in a row without killing anyone,
	// and is not afk, give them an AWARD_SADDAY
	if(self->client->killsthislife < 1 &&
	   level.time < self->client->afktime &&
	   level.roundwarmuptime == 0 && level.warmuptime == 0)
		self->client->ps.persistant[PERS_NO_KILLS]++;
	else
		self->client->ps.persistant[PERS_NO_KILLS] = 0;
	if(self->client->ps.persistant[PERS_NO_KILLS] >= 3)
		giveaward(self->client, AWARD_SADDAY);

	if(attacker && attacker->client){
		attacker->client->lastkilledclient = self->s.number;

		if(attacker == self || onsameteam(self, attacker)){
			addscore(attacker, self->r.currentOrigin, -1);
			trap_StatAdd(attacker->s.number, QSTAT_DEATHS, 1);
		}else{
			addscore(attacker, self->r.currentOrigin, 1);
			trap_StatAdd(attacker->s.number, QSTAT_KILLS, 1);
			attacker->client->killsthislife++;
			if(level.totalkills < 1)
				giveaward(attacker->client, AWARD_FIRSTBLOOD);
			level.totalkills++;

			// if attacker is a bad enough dude to have
			// many kills since respawning, give them an
			// award
			switch(attacker->client->killsthislife){
			case 5:
				giveaward(attacker->client, AWARD_KILLINGSPREE);
				break;
			case 10:
				giveaward(attacker->client, AWARD_DOMINATING);
				break;
			case 15:
				giveaward(attacker->client, AWARD_RAMPAGE);
				break;
			case 20:
				giveaward(attacker->client, AWARD_UNSTOPPABLE);
				break;
			case 25:
				giveaward(attacker->client, AWARD_GODLIKE);
				break;
			case 30:
				giveaward(attacker->client, AWARD_WICKEDSICK);
				break;
			}

			if(meansOfDeath == MOD_GAUNTLET){
				// play humiliation on player
				giveaward(attacker->client, AWARD_GAUNTLET);
				// also play humiliation on target
				giveaward(self->client, AWARD_HUMILIATED);
			}

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if(level.time - attacker->client->lastkilltime < MULTIKILL_TIME){
				chkmultikill(attacker);
			}
			attacker->client->lastkilltime = level.time;
		}
	}else{
		addscore(self, self->r.currentOrigin, -1);
		trap_StatAdd(self->s.number, QSTAT_DEATHS, 1);
	}

	// Add team bonuses
	fragbonuses(self, inflictor, attacker);

	// if I committed suicide, the flag does not fall, it returns.
	if(meansOfDeath == MOD_SUICIDE){
		if(self->client->ps.powerups[PW_NEUTRALFLAG]){	// only happens in One Flag CTF
			flagreturn(TEAM_FREE);
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}else if(self->client->ps.powerups[PW_REDFLAG]){	// only happens in standard CTF
			flagreturn(TEAM_RED);
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}else if(self->client->ps.powerups[PW_BLUEFLAG]){	// only happens in standard CTF
			flagreturn(TEAM_BLUE);
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	tossclientitems(self);
#ifdef MISSIONPACK
	tossclientpowerups(self);
	if(g_gametype.integer == GT_HARVESTER)
		tossclientcubes(self);

#endif

	Cmd_Score_f(self);	// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for(i = 0; i < level.maxclients; i++){
		gclient_t *client;

		client = &level.clients[i];
		if(client->pers.connected != CON_CONNECTED)
			continue;
		if(client->sess.team != TEAM_SPECTATOR)
			continue;
		if(client->sess.specclient == self->s.number)
			Cmd_Score_f(g_entities + i);
	}

	self->takedmg = qtrue;	// can still be gibbed

	self->s.weapon[0] = WP_NONE;
	self->s.weapon[1] = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	lookatkiller(self, inflictor, attacker);

	veccpy(self->s.angles, self->client->ps.viewangles);

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// respawn wait
	// in normal CA/LTS/LMS play, players can't respawn until next round
	// begins. during roundwarmup, however, players do respawn.
	if((g_gametype.integer == GT_CA || g_gametype.integer == GT_LMS ||
	   g_gametype.integer == GT_LTS) && numonteam(TEAM_RED) > 0 &&
	   numonteam(TEAM_BLUE) > 0 && (level.roundwarmuptime != 0 &&
	   level.time > level.roundwarmuptime))
		self->client->respawntime = -1;
	else
		self->client->respawntime = level.time + 1700;

	// remove powerups
	memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	// never gib in a nodrop
	contents = trap_PointContents(self->r.currentOrigin, -1);

	if((self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) && g_blood.integer) || meansOfDeath == MOD_SUICIDE)
		// gib death
		entgib(self, killer);
	else{
		// normal death
		static int i;

		switch(i){
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if(self->health <= GIB_HEALTH)
			self->health = GIB_HEALTH+1;

		self->client->ps.torsoAnim =
			((self->client->ps.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

		addevent(self, EV_DEATH1 + i, killer);

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = (i + 1) % 3;

#ifdef MISSIONPACK
		if(self->s.eFlags & EF_KAMIKAZE)
			Kamikaze_DeathTimer(self);

#endif
	}

	trap_LinkEntity(self);
}

/*
================
CheckArmor
================
*/
int
chkarmor(gentity_t *ent, int damage, int dflags)
{
	gclient_t *client;
	int save;
	int count;
	float protection;

	if(client->ps.stats[STAT_ARMOR] < 1)
		client->ps.stats[STAT_ARMORTYPE] = 0;

	if(!damage)
		return 0;

	client = ent->client;

	if(!client)
		return 0;

	if(dflags & DAMAGE_NO_ARMOR)
		return 0;

gprintf("%d\n", ent->client->ps.stats[STAT_ARMORTYPE]);
	switch(ent->client->ps.stats[STAT_ARMORTYPE]){
	case ARMOR_YELLOW:
		protection = 0.66f;
		break;
	case ARMOR_RED:
		protection = 0.75f;
		break;
	default:
		protection = 0;
		break;
	}

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil(damage * protection);
	if(save >= count)
		save = count;

	if(!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

/*
================
RaySphereIntersections
================
*/
int
RaySphereIntersections(vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2])
{
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	vecnorm(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
	    (point[1] - origin[1]) * (point[1] - origin[1]) +
	    (point[2] - origin[2]) * (point[2] - origin[2]) -
	    radius * radius;

	d = b * b - 4 * c;
	if(d > 0){
		t = (-b + sqrt(d)) / 2;
		vecmad(point, t, dir, intersections[0]);
		t = (-b - sqrt(d)) / 2;
		vecmad(point, t, dir, intersections[1]);
		return 2;
	}else if(d == 0){
		t = (-b) / 2;
		vecmad(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

#ifdef MISSIONPACK
/*
================
G_InvulnerabilityEffect
================
*/
int
invulneffect(gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir)
{
	gentity_t *impact;
	vec3_t intersections[2], vec;
	int n;

	if(!targ->client)
		return qfalse;
	veccpy(dir, vec);
	vecinv(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections(targ->client->ps.origin, 42, point, vec, intersections);
	if(n > 0){
		impact = enttemp(targ->client->ps.origin, EV_INVUL_IMPACT);
		vecsub(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if(impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if(impactpoint)
			veccpy(intersections[0], impactpoint);
		if(bouncedir){
			veccpy(vec, bouncedir);
			vecnorm(bouncedir);
		}
		return qtrue;
	}else
		return qfalse;
}

#endif
/*
============
entdamage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
        example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be nil for environmental effects

dflags		these flags are used to control how T_Damage works
        DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
        DAMAGE_NO_ARMOR			armor does not protect from this damage
        DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
        DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void
entdamage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
	 vec3_t dir, vec3_t point, int damage, int dflags, int mod)
{
	gclient_t *client;
	int take;
	int asave;
	int knockback;
	int max;
#ifdef MISSIONPACK
	vec3_t bouncedir, impactpoint;
#endif

	if(!targ->takedmg)
		return;

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if(level.intermissionqueued)
		return;

#ifdef MISSIONPACK
	if(targ->client && mod != MOD_JUICED)
		if(targ->client->invulnerabilityTime > level.time){
			if(dir && point)
				invulneffect(targ, dir, point, impactpoint, bouncedir);
			return;
		}

#endif
	if(!inflictor)
		inflictor = &g_entities[ENTITYNUM_WORLD];
	if(!attacker)
		attacker = &g_entities[ENTITYNUM_WORLD];

	// shootable doors / buttons don't actually have any health
	if(targ->s.eType == ET_MOVER){
		if(targ->use && targ->moverstate == MOVER_POS1)
			targ->use(targ, inflictor, attacker);
		return;
	}
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_OBELISK && chkobeliskattacked(targ, attacker))
		return;

#endif
	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if(attacker->client && attacker != targ){
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
#ifdef MISSIONPACK
		if(bg_itemlist[attacker->client->ps.stats[STAT_PERSISTANT_POWERUP]].tag == PW_GUARD)
			max /= 2;

#endif
		damage = damage * max / 100;
	}

	client = targ->client;

	if(client)
		if(client->noclip)
			return;

	if(!dir)
		dflags |= DAMAGE_NO_KNOCKBACK;
	else
		vecnorm(dir);

	knockback = damage;
	if(knockback > 200)
		knockback = 200;
	if(targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;
	if(dflags & DAMAGE_NO_KNOCKBACK)
		knockback = 0;

	// figure momentum add, even if the damage won't be taken
	if(knockback && targ->client){
		vec3_t kvel;
		float mass;

		mass = 200;

		vecmul(dir, g_knockback.value * (float)knockback / mass, kvel);
		vecadd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if(!targ->client->ps.pm_time){
			int t;

			t = knockback * 2;
			if(t < 50)
				t = 50;
			if(t > 200)
				t = 200;
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if(!(dflags & DAMAGE_NO_PROTECTION)){
		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
#ifdef MISSIONPACK
		if(mod != MOD_JUICED && targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && onsameteam(targ, attacker)){
#else
		if(targ != attacker && onsameteam(targ, attacker)){
#endif
			if(!g_friendlyFire.integer)
				return;
		}
#ifdef MISSIONPACK
		if(mod == MOD_PROXIMITY_MINE){
			if(inflictor && inflictor->parent && onsameteam(targ, inflictor->parent))
				return;
			if(targ == attacker)
				return;
		}
#endif

		// check for godmode
		if(targ->flags & FL_GODMODE)
			return;
	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if(client && client->ps.powerups[PW_BATTLESUIT]){
		addevent(targ, EV_POWERUP_BATTLESUIT, 0);
		if((dflags & DAMAGE_RADIUS) || (mod == MOD_FALLING))
			return;
		damage *= 0.5;
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if(attacker->client && client
	   && targ != attacker && targ->health > 0
	   && targ->s.eType != ET_MISSILE
	   && targ->s.eType != ET_GENERAL){
		if(onsameteam(targ, attacker))
			attacker->client->ps.persistant[PERS_HITS]--;
		else
			attacker->client->ps.persistant[PERS_HITS]++;
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(client->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if(targ == attacker)
		damage *= 0.5;

	if(damage < 1)
		damage = 1;
	take = damage;

	// save some from armor
	asave = chkarmor(targ, take, dflags);
	take -= asave;

	if(g_debugDamage.integer)
		gprintf("%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			 targ->health, take, asave);

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if(client){
		if(attacker)
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		else
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		client->dmgarmor += asave;
		client->dmgblood += take;
		client->dmgknockback += knockback;
		if(dir){
			veccpy(dir, client->dmgfrom);
			client->dmgfromworld = qfalse;
		}else{
			veccpy(targ->r.currentOrigin, client->dmgfrom);
			client->dmgfromworld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF){
#else
	if(g_gametype.integer == GT_CTF){
#endif
		chkhurtcarrier(targ, attacker);
	}

	if(targ->client){
		// set the last client who damaged the target
		targ->client->lasthurtclient = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if(take){
		targ->health = targ->health - take;
		if(targ->client)
			targ->client->ps.stats[STAT_HEALTH] = targ->health;

		if(targ->health <= 0){
			if(client)
				targ->flags |= FL_NO_KNOCKBACK;

			if(targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die(targ, inflictor, attacker, take, mod);
			return;
		}else if(targ->pain)
			targ->pain(targ, attacker, take);
	}
}

/*
============
candamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean
candamage(gentity_t *targ, vec3_t origin)
{
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	vec3_t offsetmins = {-15, -15, -15};
	vec3_t offsetmaxs = {15, 15, 15};

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	vecadd(targ->r.absmin, targ->r.absmax, midpoint);
	vecmul(midpoint, 0.5, midpoint);

	veccpy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection,
	// rather than in world coordinate
	veccpy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	veccpy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[2];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if(tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}

/*
============
radiusdamage
============
*/
qboolean
radiusdamage(vec3_t origin, gentity_t *attacker, float damage, float radius,
	       gentity_t *ignore, int mod)
{
	float points, dist;
	gentity_t *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++){
		ent = &g_entities[entityList[e]];

		if(ent == ignore)
			continue;
		if(!ent->takedmg)
			continue;

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++){
			if(origin[i] < ent->r.absmin[i])
				v[i] = ent->r.absmin[i] - origin[i];
			else if(origin[i] > ent->r.absmax[i])
				v[i] = origin[i] - ent->r.absmax[i];
			else
				v[i] = 0;
		}

		dist = veclen(v);
		if(dist >= radius)
			continue;

		points = damage * (1.0 - dist / radius);

		if(candamage(ent, origin)){
			if(logaccuracyhit(ent, attacker))
				hitClient = qtrue;
			vecsub(ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			entdamage(ent, nil, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return hitClient;
}
