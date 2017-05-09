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

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/

#define RESPAWN_SHIELD		25
#define RESPAWN_HEALTH		35
#define RESPAWN_AMMO		40
#define RESPAWN_HOLDABLE	60
#define RESPAWN_MEGAHEALTH	35	//120
#define RESPAWN_POWERUP		120

//======================================================================

int
Pickup_Powerup(gentity_t *ent, gentity_t *other)
{
	int quantity;
	int i;
	gclient_t *client;

	if(!other->client->ps.powerups[ent->item->tag])
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->tag] =
			level.time - (level.time % 1000);

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	other->client->ps.powerups[ent->item->tag] += quantity * 1000;

	// give any nearby players a "denied" anti-reward
	for(i = 0; i < level.maxclients; i++){
		vec3_t delta;
		float len;
		vec3_t forward;
		trace_t tr;

		client = &level.clients[i];
		if(client == other->client)
			continue;
		if(client->pers.connected == CON_DISCONNECTED)
			continue;
		if(client->ps.stats[STAT_HEALTH] <= 0)
			continue;

		// if same team in team game, no sound
		// cannot use onsameteam as it expects to g_entities, not clients
		if(g_gametype.integer >= GT_TEAM && other->client->sess.team == client->sess.team)
			continue;

		// if too far away, no sound
		vecsub(ent->s.pos.trBase, client->ps.origin, delta);
		len = vecnorm(delta);
		if(len > 192)
			continue;

		// if not facing, no sound
		anglevecs(client->ps.viewangles, forward, nil, nil);
		if(vecdot(delta, forward) < 0.4)
			continue;

		// if not line of sight, no sound
		trap_Trace(&tr, client->ps.origin, nil, nil, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID);
		if(tr.fraction != 1.0)
			continue;

		// anti-reward
		giveaward(client, AWARD_DENIED);
	}
	return RESPAWN_POWERUP;
}

//======================================================================

#ifdef MISSIONPACK
int
Pickup_PersistantPowerup(gentity_t *ent, gentity_t *other)
{
	int clientNum;
	char userinfo[MAX_INFO_STRING];
	float handicap;
	int max;

	other->client->ps.stats[STAT_PERSISTANT_POWERUP] = ent->item - bg_itemlist;
	other->client->persistantPowerup = ent;

	switch(ent->item->tag){
	case PW_GUARD:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		max = (int)(2 *  handicap);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_SHIELD] = max;
		other->client->pers.maxhealth = max;

		break;

	case PW_SCOUT:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxhealth = handicap;
		other->client->ps.stats[STAT_SHIELD] = 0;
		break;

	case PW_DOUBLER:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxhealth = handicap;
		break;
	case PW_AMMOREGEN:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxhealth = handicap;
		memset(other->client->ammoTimes, 0, sizeof(other->client->ammoTimes));
		break;
	default:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxhealth = handicap;
		break;
	}

	return -1;
}

//======================================================================
#endif

int
Pickup_Holdable(gentity_t *ent, gentity_t *other)
{
	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	if(ent->item->tag == HI_KAMIKAZE)
		other->client->ps.eFlags |= EF_KAMIKAZE;

	return RESPAWN_HOLDABLE;
}

//======================================================================

void
addammo(gentity_t *ent, int weapon, int count)
{
	ent->client->ps.ammo[weapon] += count;
	if(ent->client->ps.ammo[weapon] > 200)
		ent->client->ps.ammo[weapon] = 200;
}

int
Pickup_Ammo(gentity_t *ent, gentity_t *other)
{
	int quantity;

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	addammo(other, ent->item->tag, quantity);

	return RESPAWN_AMMO;
}

//======================================================================

int
Pickup_Weapon(gentity_t *ent, gentity_t *other)
{
	int quantity;

	if(ent->count < 0)
		quantity = 0;	// None for you, sir!
	else{
		if(ent->count)
			quantity = ent->count;
		else
			quantity = ent->item->quantity;

		// dropped items and teamplay weapons always have full ammo
		if(!(ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM){
			// respawning rules
			// drop the quantity if the already have over the minimum
			if(other->client->ps.ammo[ent->item->tag] < quantity)
				quantity = quantity - other->client->ps.ammo[ent->item->tag];
			else
				quantity = 1;	// only add a single shot
		}
	}

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= (1 << ent->item->tag);

	addammo(other, ent->item->tag, quantity);

	if(ent->item->tag == WP_GRAPPLING_HOOK)
		other->client->ps.ammo[ent->item->tag] = -1;	// unlimited ammo

	// team deathmatch has slow weapon respawns
	if(g_gametype.integer == GT_TEAM)
		return g_weaponTeamRespawn.integer;

	return g_weaponRespawn.integer;
}

//======================================================================

int
Pickup_Health(gentity_t *ent, gentity_t *other)
{
	int max;
	int quantity;

	// small and mega healths will go over the max
#ifdef MISSIONPACK
	if(bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].tag == PW_GUARD)
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	else
#endif
	if(ent->item->quantity != 5 && ent->item->quantity != 100)
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	else
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	other->health += quantity;

	if(other->health > max)
		other->health = max;
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if(ent->item->quantity == 100)	// mega health respawns slow
		return RESPAWN_MEGAHEALTH;

	return RESPAWN_HEALTH;
}

//======================================================================

int
Pickup_Armor(gentity_t *ent, gentity_t *other)
{
#ifdef MISSIONPACK
	int upperBound;

	other->client->ps.stats[STAT_SHIELD] += ent->item->quantity;

	if(Q_stricmp(ent->item->classname, "item_shield_tiny") == 0){
		if(other->client->ps.stats[STAT_SHIELDTYPE] == 0)
			other->client->ps.stats[STAT_SHIELDTYPE] = ent->item->tag;
	}else{
		other->client->ps.stats[STAT_SHIELDTYPE] = ent->item->tag;
	}

	if(other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].tag == PW_GUARD)
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH];
	else
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH] * 2;

	if(other->client->ps.stats[STAT_SHIELD] > upperBound)
		other->client->ps.stats[STAT_SHIELD] = upperBound;

#else
	other->client->ps.stats[STAT_SHIELD] += ent->item->quantity;
	if(other->client->ps.stats[STAT_SHIELD] > other->client->ps.stats[STAT_MAX_HEALTH] * 2)
		other->client->ps.stats[STAT_SHIELD] = other->client->ps.stats[STAT_MAX_HEALTH] * 2;

#endif

	return RESPAWN_SHIELD;
}

//======================================================================

void
itemrespawn(gentity_t *ent)
{
	if(!ent)
		return;

	// randomly select from teamed entities
	if(ent->team){
		gentity_t *master;
		int count;
		int choice;

		if(!ent->teammaster)
			errorf("itemrespawn: bad teammaster");
		master = ent->teammaster;

		for(count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = rand() % count;

		for(count = 0, ent = master; ent && count < choice; ent = ent->teamchain, count++)
			;
	}

	if(!ent)
		return;

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity(ent);

	if(ent->item->type == IT_POWERUP){
		// play powerup spawn sound to all clients
		gentity_t *te;

		// if the powerup respawn sound should Not be global
		if(ent->speed)
			te = enttemp(ent->s.pos.trBase, EV_GENERAL_SOUND);
		else
			te = enttemp(ent->s.pos.trBase, EV_GLOBAL_SOUND);
		te->s.eventParm = getsoundindex("sound/items/poweruprespawn.wav");
		te->r.svFlags |= SVF_BROADCAST;
	}

	if(ent->item->type == IT_HOLDABLE && ent->item->tag == HI_KAMIKAZE){
		// play powerup spawn sound to all clients
		gentity_t *te;

		// if the powerup respawn sound should Not be global
		if(ent->speed)
			te = enttemp(ent->s.pos.trBase, EV_GENERAL_SOUND);
		else
			te = enttemp(ent->s.pos.trBase, EV_GLOBAL_SOUND);
		te->s.eventParm = getsoundindex("sound/items/kamikazerespawn.wav");
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	addevent(ent, EV_ITEM_RESPAWN, 0);

	ent->nextthink = 0;
}

void
item_touch(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	int respawn;
	qboolean predict;

	if(!other->client)
		return;
	if(other->health < 1)
		return;		// dead people can't pickup

	// the same pickup rules are used for client side and server side
	if(!cangrabitem(g_gametype.integer, &ent->s, &other->client->ps))
		return;

	logprintf("Item: %i %s\n", other->s.number, ent->item->classname);

	predict = other->client->pers.predictitempickup;

	// call the item-specific pickup function
	switch(ent->item->type){
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
//		predict = qfalse;
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
//		predict = qfalse;
		break;
	case IT_SHIELD:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		respawn = Pickup_PersistantPowerup(ent, other);
		break;
#endif
	case IT_TEAM:
		respawn = pickupteam(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	default:
		return;
	}

	if(!respawn)
		return;

	// play the normal pickup sound
	if(predict)
		addpredictable(other, EV_ITEM_PICKUP, ent->s.modelindex);
	else
		addevent(other, EV_ITEM_PICKUP, ent->s.modelindex);

	// powerup pickups are global broadcasts
	if(ent->item->type == IT_POWERUP || ent->item->type == IT_TEAM){
		// if we want the global sound to play
		if(!ent->speed){
			gentity_t *te;

			te = enttemp(ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelindex;
			te->r.svFlags |= SVF_BROADCAST;
		}else{
			gentity_t *te;

			te = enttemp(ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelindex;
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	// fire item targets
	usetargets(ent, other);

	// wait of -1 will not respawn
	if(ent->wait == -1){
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if(ent->wait)
		respawn = ent->wait;

	// random can be used to vary the respawn time
	if(ent->random){
		respawn += crandom() * ent->random;
		if(respawn < 1)
			respawn = 1;
	}

	// dropped items will not respawn
	if(ent->flags & FL_DROPPED_ITEM)
		ent->freeafterevent = qtrue;

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if(respawn <= 0){
		ent->nextthink = 0;
		ent->think = 0;
	}else{
		ent->nextthink = level.time + respawn * 1000;
		ent->think = itemrespawn;
	}
	trap_LinkEntity(ent);
}

//======================================================================

/*
Spawns an item and tosses it forward
*/
gentity_t *
itemlaunch(gitem_t *item, vec3_t origin, vec3_t velocity)
{
	gentity_t *dropped;

	dropped = entspawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1;	// This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	vecset(dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	vecset(dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = item_touch;

	setorigin(dropped, origin);
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	veccpy(velocity, dropped->s.pos.trDelta);

	dropped->s.eFlags |= EF_BOUNCE_HALF;
#ifdef MISSIONPACK
	if((g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF)                    && item->type == IT_TEAM){	// Special case for CTF flags
#else
	if(g_gametype.integer == GT_CTF && item->type == IT_TEAM){								// Special case for CTF flags
#endif
		dropped->think = droppedflag_think;
		dropped->nextthink = level.time + 30000;
		ckhdroppedteamitem(dropped);
	}else{	// auto-remove after 30 seconds
		dropped->think = entfree;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity(dropped);

	return dropped;
}

/*
Spawns an item and tosses it forward
*/
gentity_t *
itemdrop(gentity_t *ent, gitem_t *item, float angle)
{
	vec3_t velocity;
	vec3_t angles;

	veccpy(ent->s.apos.trBase, angles);

	anglevecs(angles, velocity, nil, nil);
	vecmul(velocity, 150, velocity);
	if(ent->client != nil)
		vecadd(velocity, ent->client->ps.velocity, velocity);

	return itemlaunch(item, ent->s.pos.trBase, velocity);
}

/*
Respawn the item
*/
void
Use_Item(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	itemrespawn(ent);
}

//======================================================================

/*
Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
*/
void
itemspawnfinish(gentity_t *ent)
{
	trace_t tr;
	vec3_t dest;

	vecset(ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	vecset(ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;	// store item number in modelindex
	ent->s.modelindex2 = 0;	// zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = item_touch;
	// using an item causes it to respawn
	ent->use = Use_Item;

	// suspended
	setorigin(ent, ent->s.origin);

	// team slaves and targeted items aren't present at start
	if((ent->flags & FL_TEAMSLAVE) || ent->targetname){
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// powerups don't spawn in for a while
	if(ent->item->type == IT_POWERUP){
		float respawn;

		respawn = 45 + crandom() * 15;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = itemrespawn;
		return;
	}

	trap_LinkEntity(ent);
}

qboolean itemRegistered[MAX_ITEMS];

void
checkteamitems(void)
{
	// Set up team stuff
	teamgameinit();

	if(g_gametype.integer == GT_CTF){
		gitem_t *item;

		// check for the two flags
		item = finditem("Red Flag");
		if(!item || !itemRegistered[item - bg_itemlist])
			gprintf(S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n");
		item = finditem("Blue Flag");
		if(!item || !itemRegistered[item - bg_itemlist])
			gprintf(S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n");
	}
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_1FCTF){
		gitem_t *item;

		// check for all three flags
		item = finditem("Red Flag");
		if(!item || !itemRegistered[item - bg_itemlist])
			gprintf(S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n");
		item = finditem("Blue Flag");
		if(!item || !itemRegistered[item - bg_itemlist])
			gprintf(S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n");
		item = finditem("Neutral Flag");
		if(!item || !itemRegistered[item - bg_itemlist])
			gprintf(S_COLOR_YELLOW "WARNING: No team_CTF_neutralflag in map\n");
	}

	if(g_gametype.integer == GT_OBELISK){
		gentity_t *ent;

		// check for the two obelisks
		ent = nil;
		ent = findent(ent, FOFS(classname), "team_redobelisk");
		if(!ent)
			gprintf(S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n");

		ent = nil;
		ent = findent(ent, FOFS(classname), "team_blueobelisk");
		if(!ent)
			gprintf(S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n");
	}

	if(g_gametype.integer == GT_HARVESTER){
		gentity_t *ent;

		// check for all three obelisks
		ent = nil;
		ent = findent(ent, FOFS(classname), "team_redobelisk");
		if(!ent)
			gprintf(S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n");

		ent = nil;
		ent = findent(ent, FOFS(classname), "team_blueobelisk");
		if(!ent)
			gprintf(S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n");

		ent = nil;
		ent = findent(ent, FOFS(classname), "team_neutralobelisk");
		if(!ent)
			gprintf(S_COLOR_YELLOW "WARNING: No team_neutralobelisk in map\n");
	}
#endif
}

void
clearitems(void)
{
	memset(itemRegistered, 0, sizeof(itemRegistered));

	// players always start with the base weapon
	registeritem(finditemforweapon(WP_MACHINEGUN));
	registeritem(finditemforweapon(WP_GAUNTLET));
	registeritem(finditemforweapon(WP_GRAPPLING_HOOK));
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_HARVESTER){
		registeritem(finditem("Red Cube"));
		registeritem(finditem("Blue Cube"));
	}
#endif
}

/*
The item will be added to the precache list
*/
void
registeritem(gitem_t *item)
{
	if(!item)
		errorf("registeritem: nil");
	itemRegistered[item - bg_itemlist] = qtrue;
}

/*
Write the needed items to a config string
so the client will know which ones to precache
*/
void
mkitemsconfigstr(void)
{
	char string[MAX_ITEMS+1];
	int i;
	int count;

	count = 0;
	for(i = 0; i < bg_nitems; i++){
		if(itemRegistered[i]){
			count++;
			string[i] = '1';
		}else
			string[i] = '0';
	}
	string[bg_nitems] = 0;

	gprintf("%i items registered\n", count);
	trap_SetConfigstring(CS_ITEMS, string);
}

int
itemdisabled(gitem_t *item)
{
	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue(name);
}

/*
Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
*/
void
itemspawn(gentity_t *ent, gitem_t *item)
{
	spawnfloat("random", "0", &ent->random);
	spawnfloat("wait", "0", &ent->wait);

	registeritem(item);
	if(itemdisabled(item))
		return;

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = itemspawnfinish;

	ent->physbounce = 0.50;	// items are bouncy

	if(item->type == IT_POWERUP){
		getsoundindex("sound/items/poweruprespawn.wav");
		spawnfloat("noglobalsound", "0", &ent->speed);
	}

#ifdef MISSIONPACK
	if(item->type == IT_PERSISTANT_POWERUP)
		ent->s.generic1 = ent->spawnflags;

#endif
}

void
itembounce(gentity_t *ent, trace_t *trace)
{
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.prevtime + (level.time - level.prevtime) * trace->fraction;
	evaltrajectorydelta(&ent->s.pos, hitTime, velocity);
	dot = vecdot(velocity, trace->plane.normal);
	vecmad(velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta);

	// cut the velocity to keep from bouncing forever
	vecmul(ent->s.pos.trDelta, ent->physbounce, ent->s.pos.trDelta);

	// check for stop
	if(trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40){
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector(trace->endpos);
		setorigin(ent, trace->endpos);
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	vecadd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	veccpy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

void
runitem(gentity_t *ent)
{
	vec3_t origin;
	trace_t tr;
	int contents;
	int mask;

	// if its groundentity has been set to none, it may have been pushed off an edge
	if(ent->s.groundEntityNum == ENTITYNUM_NONE)
		if(ent->s.pos.trType != TR_GRAVITY){
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}

	if(ent->s.pos.trType == TR_STATIONARY){
		// check think function
		runthink(ent);
		return;
	}

	// get current position
	evaltrajectory(&ent->s.pos, level.time, origin);

	// trace a line from the previous position to the current position
	if(ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	//MASK_SOLID;
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		   ent->r.ownerNum, mask);

	veccpy(tr.endpos, ent->r.currentOrigin);

	if(tr.startsolid)
		tr.fraction = 0;

	trap_LinkEntity(ent);	// FIXME: avoid this for stationary?

	// check think function
	runthink(ent);

	if(tr.fraction == 1)
		return;

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents(ent->r.currentOrigin, -1);
	if(contents & CONTENTS_NODROP){
		if(ent->item && ent->item->type == IT_TEAM)
			teamfreeent(ent);
		else
			entfree(ent);
		return;
	}

	itembounce(ent, &tr);
}
