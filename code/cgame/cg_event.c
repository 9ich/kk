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
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

// for the voice chats
//==========================================================================

/*
Also called by scoreboard drawing
*/
const char      *
placestr(int rank)
{
	static char str[64];
	char *s, *t;

	if(rank & RANK_TIED_FLAG){
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	}else
		t = "";

	if(rank == 1)
		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;	// draw in blue
	else if(rank == 2)
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;	// draw in red
	else if(rank == 3)
		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;	// draw in yellow
	else if(rank == 11)
		s = "11th";
	else if(rank == 12)
		s = "12th";
	else if(rank == 13)
		s = "13th";
	else if(rank % 10 == 1)
		s = va("%ist", rank);
	else if(rank % 10 == 2)
		s = va("%ind", rank);
	else if(rank % 10 == 3)
		s = va("%ird", rank);
	else
		s = va("%ith", rank);

	Com_sprintf(str, sizeof(str), "%s%s", t, s);
	return str;
}

static void
queueobit(const char *killer, const char *icon, const char *victim)
{
	int i;

	// shift older lines up
	for(i = ARRAY_LEN(cg.obit)-1; i > 0; i--)
		cg.obit[i] = cg.obit[i-1];
	cg.obit[0].time = cg.time;
	Q_strncpyz(cg.obit[0].killer, killer, sizeof cg.obit[0].killer);
	cg.obit[0].icon = trap_R_RegisterShaderNoMip(icon);
	Q_strncpyz(cg.obit[0].victim, victim, sizeof cg.obit[0].victim);
}

static void
CG_Obituary(entityState_t *ent)
{
	int mod;
	int target, attacker;
	const char *targetInfo;
	const char *attackerInfo;
	char targetName[MAX_NAME_LENGTH];
	char attackerName[MAX_NAME_LENGTH];
	char *icon;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if(target < 0 || target >= MAX_CLIENTS)
		cgerrorf("CG_Obituary: target out of range");

	if(attacker < 0 || attacker >= MAX_CLIENTS){
		attacker = ENTITYNUM_WORLD;
		attackerInfo = nil;
	}else
		attackerInfo = getconfigstr(CS_PLAYERS + attacker);

	targetInfo = getconfigstr(CS_PLAYERS + target);
	if(!targetInfo)
		return;
	Q_strncpyz(targetName, Info_ValueForKey(targetInfo, "n"), sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	// check for single client obituary

	switch(mod){
	case MOD_SUICIDE:
	case MOD_FALLING:
	case MOD_CRUSH:
	case MOD_WATER:
	case MOD_SLIME:
	case MOD_LAVA:
	case MOD_TARGET_LASER:
	case MOD_TRIGGER_HURT:
		icon = "icons/worlddeath";
		break;
	default:
		icon = nil;
		break;
	}

	if(attacker == target)
		icon = "icons/worlddeath";

	if(icon != nil){
		queueobit("", icon, targetName);
		return;
	}

	// check for kill messages from the current clientNum
	if(attacker == cg.snap->ps.clientNum){
		char *s;

		if(cgs.gametype < GT_TEAM)
			s = va("You fragged %s\n%s place with %i", targetName,
			       placestr(cg.snap->ps.persistant[PERS_RANK] + 1),
			       cg.snap->ps.persistant[PERS_SCORE]);
		else
			s = va("You fragged %s", targetName);

		centerprint(s, screenheight() * 0.30, 16);
		trap_S_StartLocalSound(cgs.media.killSound, CHAN_ANNOUNCER);
	}

	// check for double client messages
	if(!attackerInfo){
		attacker = ENTITYNUM_WORLD;
		strcpy(attackerName, "noname");
	}else{
		Q_strncpyz(attackerName, Info_ValueForKey(attackerInfo, "n"), sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
		// check for kill messages about the current clientNum
		if(target == cg.snap->ps.clientNum)
			Q_strncpyz(cg.killername, attackerName, sizeof(cg.killername));
	}

	if(attacker == ENTITYNUM_WORLD){
		// we don't know what it was
		queueobit("", "icons/worlddeath", targetName);
		return;
	}
	switch(mod){
	case MOD_GRAPPLE:
		icon = finditemforweapon(WP_GRAPPLING_HOOK)->icon;
		break;
	case MOD_GAUNTLET:
		icon = finditemforweapon(WP_GAUNTLET)->icon;
		break;
	case MOD_MACHINEGUN:
		icon = finditemforweapon(WP_MACHINEGUN)->icon;
		break;
	case MOD_CHAINGUN:
		icon = finditemforweapon(WP_CHAINGUN)->icon;
		break;
	case MOD_SHOTGUN:
		icon = finditemforweapon(WP_SHOTGUN)->icon;
		break;
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
		icon = finditemforweapon(WP_GRAPPLING_HOOK)->icon;
		break;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
		icon = finditemforweapon(WP_ROCKET_LAUNCHER)->icon;
		break;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
		icon = finditemforweapon(WP_PLASMAGUN)->icon;
		break;
	case MOD_RAILGUN:
		icon = finditemforweapon(WP_RAILGUN)->icon;
		break;
	case MOD_LIGHTNING:
		icon = finditemforweapon(WP_LIGHTNING)->icon;
		break;
	case MOD_BFG:
	case MOD_BFG_SPLASH:
		icon = "";
		break;
	case MOD_NAIL:
		icon = finditemforweapon(WP_NAILGUN)->icon;
		break;
	case MOD_PROXIMITY_MINE:
		icon = finditemforweapon(WP_PROX_LAUNCHER)->icon;
		break;
	case MOD_KAMIKAZE:
		icon = "icons/kamikaze";
		break;
	case MOD_JUICED:
		icon = "";
		break;
	case MOD_TELEFRAG:
	default:
		icon = "icons/worlddeath";
		break;
	}

	queueobit(attackerName, icon, targetName);
}

//==========================================================================

static void
CG_UseItem(centity_t *cent)
{
	clientInfo_t *ci;
	int itemNum, clientNum;
	gitem_t *item;
	entityState_t *es;

	es = &cent->currstate;

	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if(itemNum < 0 || itemNum > HI_NUM_HOLDABLE)
		itemNum = 0;

	// print a message if the local player
	if(es->number == cg.snap->ps.clientNum){
		if(!itemNum)
			centerprint("No item to use", screenheight() * 0.30, BIGCHAR_WIDTH);
		else{
			item = finditemforholdable(itemNum);
			centerprint(va("Use %s", item->pickupname), screenheight() * 0.30, BIGCHAR_WIDTH);
		}
	}

	switch(itemNum){
	default:
	case HI_NONE:
		trap_S_StartSound(nil, es->number, CHAN_BODY, cgs.media.useNothingSound);
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		clientNum = cent->currstate.clientNum;
		if(clientNum >= 0 && clientNum < MAX_CLIENTS){
			ci = &cgs.clientinfo[clientNum];
			ci->medkitUsageTime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_BODY, cgs.media.medkitSound);
		break;

#ifdef MISSIONPACK
	case HI_KAMIKAZE:
		break;

	case HI_PORTAL:
		break;
	case HI_INVULNERABILITY:
		trap_S_StartSound(nil, es->number, CHAN_BODY, cgs.media.useInvulnerabilitySound);
		break;
#endif
	}
}

/*
A new item was picked up this frame
*/
static void
CG_ItemPickup(int itemNum)
{
	cg.itempkup = itemNum;
	cg.itempkuptime = cg.time;
	cg.itempkupblendtime = cg.time;
	// see if it should be the grabbed weapon
	if(bg_itemlist[itemNum].type == IT_WEAPON)
		// select it immediately
		if(cg_autoswitch.integer && bg_itemlist[itemNum].tag != WP_MACHINEGUN){
			int slot;

			slot = bg_itemlist[itemNum].weapslot;
			cg.weapseltime[slot] = cg.time;
			cg.weapsel[slot] = bg_itemlist[itemNum].tag;
		}
}

/*
Returns waterlevel for entity origin
*/
int
CG_WaterLevel(centity_t *cent)
{
	vec3_t point;
	int contents, sample1, sample2, anim, waterlevel;
	int viewheight;

	anim = 0;

	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		viewheight = CROUCH_VIEWHEIGHT;
	else
		viewheight = DEFAULT_VIEWHEIGHT;

	// get waterlevel, accounting for ducking
	waterlevel = 0;

	point[0] = cent->lerporigin[0];
	point[1] = cent->lerporigin[1];
	point[2] = cent->lerporigin[2] + MINS_Z + 1;
	contents = pointcontents(point, -1);

	if(contents & MASK_WATER){
		sample2 = viewheight - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerporigin[2] + MINS_Z + sample1;
		contents = pointcontents(point, -1);

		if(contents & MASK_WATER){
			waterlevel = 2;
			point[2] = cent->lerporigin[2] + MINS_Z + sample2;
			contents = pointcontents(point, -1);

			if(contents & MASK_WATER)
				waterlevel = 3;
		}
	}

	return waterlevel;
}

/*
Also called by playerstate transition
*/
void
painevent(centity_t *cent, int health)
{
	char *snd;

	// don't do more than two pain sounds a second
	if(cg.time - cent->pe.paintime < 500)
		return;

	if(health < 25)
		snd = "sound/player/pain25_1.wav";
	else if(health < 50)
		snd = "sound/player/pain50_1.wav";
	else if(health < 75)
		snd = "sound/player/pain75_1.wav";
	else
		snd = "sound/player/pain100_1.wav";
	// play a gurp sound instead of a normal pain sound
	if(CG_WaterLevel(cent) == 3){
		if(rand()&1)
			trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, "sound/player/gurp1.wav"));
		else
			trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, "sound/player/gurp2.wav"));
	}else
		trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, snd));
	// save pain time for programitic twitch animation
	cent->pe.paintime = cg.time;
	cent->pe.paindir ^= 1;
}

/*
An entity has an event value
also called by CG_CheckPlayerstateEvents
*/
#define DEBUGNAME(x) if(cg_debugEvents.integer){cgprintf(x "\n"); }
void
entevent(centity_t *cent, vec3_t position)
{
	entityState_t *es;
	int event;
	vec3_t dir;
	const char *s;
	int clientNum;
	clientInfo_t *ci;

	es = &cent->currstate;
	event = es->event & ~EV_EVENT_BITS;

	if(cg_debugEvents.integer)
		cgprintf("ent:%3i  event:%3i ", es->number, event);

	if(!event){
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS)
		clientNum = 0;
	ci = &cgs.clientinfo[clientNum];

	switch(event){
	// movement generated events
	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.landSound);
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -8;
			cg.landtime = cg.time;
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*pain100_1.wav"));
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -16;
			cg.landtime = cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, customsound(es->number, "*fall1.wav"));
		cent->pe.paintime = cg.time;	// don't play a pain sound right after this
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -24;
			cg.landtime = cg.time;
		}
		break;

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
//		cgprintf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		{
			vec3_t up = {0, 0, 1};

			smokepuff(cent->lerporigin, up,
				     32,
				     1, 1, 1, 0.33f,
				     1000,
				     cg.time, 0,
				     LEF_PUFF_DONT_SCALE,
				     cgs.media.smokePuffShader);
		}

		// boing sound at origin, jump sound on player
		trap_S_StartSound(cent->lerporigin, -1, CHAN_VOICE, cgs.media.jumpPadSound);
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*jump1.wav"));
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*jump1.wav"));
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*taunt.wav"));
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrInSound);
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrUnSound);
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, customsound(es->number, "*gasp.wav"));
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t *item;
			int index;

			index = es->eventParm;	// player predicted

			if(index < 1 || index >= bg_nitems)
				break;
			item = &bg_itemlist[index];

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if(item->type == IT_POWERUP || item->type == IT_TEAM)
				trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.n_healthSound);
			else if(item->type == IT_PERSISTANT_POWERUP){
#ifdef MISSIONPACK
				switch(item->tag){
				case PW_SCOUT:
					trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.scoutSound);
					break;
				case PW_GUARD:
					trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.guardSound);
					break;
				case PW_DOUBLER:
					trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.doublerSound);
					break;
				case PW_AMMOREGEN:
					trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.ammoregenSound);
					break;
				}
#endif
			}else
				trap_S_StartSound(nil, es->number, CHAN_AUTO, trap_S_RegisterSound(item->pickupsound, qfalse));

			// show icon and name on status bar
			if(es->number == cg.snap->ps.clientNum)
				CG_ItemPickup(index);
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			gitem_t *item;
			int index;

			index = es->eventParm;	// player predicted

			if(index < 1 || index >= bg_nitems)
				break;
			item = &bg_itemlist[index];
			// powerup pickups are global
			if(item->pickupsound)
				trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound(item->pickupsound, qfalse));

			// show icon and name on status bar
			if(es->number == cg.snap->ps.clientNum)
				CG_ItemPickup(index);
		}
		break;

	// weapon events
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
//		trap_S_StartSound (nil, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if(es->number == cg.snap->ps.clientNum)
			outofammochange(0);
		break;
	case EV_NOAMMO2:
		DEBUGNAME("EV_NOAMMO");
//		trap_S_StartSound (nil, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if(es->number == cg.snap->ps.clientNum)
			outofammochange(1);
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		memset(&cent->weaplerpframe[0], 0, sizeof cent->weaplerpframe[0]);
		memset(&cent->weaplerpframe[1], 0, sizeof cent->weaplerpframe[1]);
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.selectSound);
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		fireweap(cent, 0);
		break;
	case EV_FIRE_WEAPON2:
		DEBUGNAME("EV_FIRE_WEAPON2");
		fireweap(cent, 1);
		break;
	case EV_FIRE_WEAPON3:
		DEBUGNAME("EV_FIRE_WEAPON3");
		fireweap(cent, 2);
		break;

	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM15:
		DEBUGNAME("EV_USE_ITEM15");
		CG_UseItem(cent);
		break;

	//=================================================================

	// other events
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.teleInSound);
		spawneffect(position);
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.teleOutSound);
		spawneffect(position);
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->misctime = cg.time;	// scale up from this
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		if(rand() & 1)
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound);
		else
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound);
		break;

#ifdef MISSIONPACK
	case EV_PROXIMITY_MINE_STICK:
		DEBUGNAME("EV_PROXIMITY_MINE_STICK");
		if(es->eventParm & SURF_FLESH)
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.wstbimplSound);
		else if(es->eventParm & SURF_METALSTEPS)
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.wstbimpmSound);
		else
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.wstbimpdSound);
		break;

	case EV_PROXIMITY_MINE_TRIGGER:
		DEBUGNAME("EV_PROXIMITY_MINE_TRIGGER");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.wstbactvSound);
		break;
	case EV_KAMIKAZE:
		DEBUGNAME("EV_KAMIKAZE");
		kamikazeeffect(cent->lerporigin);
		break;
	case EV_OBELISKEXPLODE:
		DEBUGNAME("EV_OBELISKEXPLODE");
		obeliskexplode(cent->lerporigin, es->eventParm);
		break;
	case EV_OBELISKPAIN:
		DEBUGNAME("EV_OBELISKPAIN");
		obeliskpain(cent->lerporigin);
		break;
	case EV_INVUL_IMPACT:
		DEBUGNAME("EV_INVUL_IMPACT");
		invulnimpact(cent->lerporigin, cent->currstate.angles);
		break;
	case EV_JUICED:
		DEBUGNAME("EV_JUICED");
		invulnjuiced(cent->lerporigin);
		break;
	case EV_LIGHTNINGBOLT:
		DEBUGNAME("EV_LIGHTNINGBOLT");
		lightningboltbeam(es->origin2, es->pos.trBase);
		break;
#endif
	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		scoreplum(cent->currstate.otherEntityNum, cent->lerporigin, cent->currstate.time);
		break;

	// missile impacts
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir(es->eventParm, dir);
		missilehitplayer(es->weapon[0], position, dir, es->otherEntityNum);
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir(es->eventParm, dir);
		missilehitwall(es->weapon[0], 0, position, dir, IMPACTSOUND_DEFAULT);
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir(es->eventParm, dir);
		missilehitwall(es->weapon[0], 0, position, dir, IMPACTSOUND_METAL);
		break;

	case EV_RAILTRAIL:
		DEBUGNAME("EV_RAILTRAIL");
		cent->currstate.weapon[0] = WP_RAILGUN;

		if(es->clientNum == cg.snap->ps.clientNum && !cg.thirdperson){
			if(cg_drawGun.integer == 2)
				vecmad(es->origin2, 8, cg.refdef.viewaxis[1], es->origin2);
			else if(cg_drawGun.integer == 3)
				vecmad(es->origin2, 4, cg.refdef.viewaxis[1], es->origin2);
		}

		dorailtrail(ci, es->origin2, es->pos.trBase);

		// if the end was on a nomark surface, don't make an explosion
		if(es->eventParm != 255){
			ByteToDir(es->eventParm, dir);
			missilehitwall(es->weapon[0], es->clientNum, position, dir, IMPACTSOUND_DEFAULT);
		}
		break;

	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		ByteToDir(es->eventParm, dir);
		dobullet(es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD);
		break;

	case EV_BULLET_HIT_FLESH:
		DEBUGNAME("EV_BULLET_HIT_FLESH");
		dobullet(es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm);
		break;

	case EV_SHOTGUN:
		DEBUGNAME("EV_SHOTGUN");
		shotgunfire(es);
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if(cgs.gamesounds[es->eventParm])
			trap_S_StartSound(nil, es->number, CHAN_VOICE, cgs.gamesounds[es->eventParm]);
		else{
			s = getconfigstr(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, s));
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if(cgs.gamesounds[es->eventParm])
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gamesounds[es->eventParm]);
		else{
			s = getconfigstr(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, customsound(es->number, s));
		}
		break;

	case EV_GLOBAL_TEAM_SOUND:	// play from the player's head so it never diminishes
	{
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		switch(es->eventParm){
		case GTS_RED_CAPTURE:			// CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
				addbufferedsound(cgs.media.captureYourTeamSound);
			else
				addbufferedsound(cgs.media.captureOpponentSound);
			break;
		case GTS_BLUE_CAPTURE:			// CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				addbufferedsound(cgs.media.captureYourTeamSound);
			else
				addbufferedsound(cgs.media.captureOpponentSound);
			break;
		case GTS_RED_RETURN:			// CTF: blue flag returned, 1FCTF: never used
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
				addbufferedsound(cgs.media.returnYourTeamSound);
			else
				addbufferedsound(cgs.media.returnOpponentSound);
			addbufferedsound(cgs.media.blueFlagReturnedSound);
			break;
		case GTS_BLUE_RETURN:			// CTF red flag returned, 1FCTF: neutral flag returned
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				addbufferedsound(cgs.media.returnYourTeamSound);
			else
				addbufferedsound(cgs.media.returnOpponentSound);
			addbufferedsound(cgs.media.redFlagReturnedSound);
			break;

		case GTS_RED_TAKEN:			// CTF: red team took blue flag, 1FCTF: blue team took the neutral flag
			// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
			if(cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
					addbufferedsound(cgs.media.enemyTookYourFlagSound);
				}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
					addbufferedsound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
			break;
		case GTS_BLUE_TAKEN:			// CTF: blue team took the red flag, 1FCTF red team took the neutral flag
			// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
			if(cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
					addbufferedsound(cgs.media.enemyTookYourFlagSound);
				}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
					addbufferedsound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
			break;

		case GTS_REDTEAM_SCORED:
			addbufferedsound(cgs.media.redScoredSound);
			break;
		case GTS_BLUETEAM_SCORED:
			addbufferedsound(cgs.media.blueScoredSound);
			break;
		case GTS_REDTEAM_TOOK_LEAD:
			addbufferedsound(cgs.media.redLeadsSound);
			break;
		case GTS_BLUETEAM_TOOK_LEAD:
			addbufferedsound(cgs.media.blueLeadsSound);
			break;
		case GTS_TEAMS_ARE_TIED:
			addbufferedsound(cgs.media.teamsTiedSound);
			break;
		default:
			break;
		}
		break;
	}

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if(cent->currstate.number != cg.snap->ps.clientNum)
			painevent(cent, es->eventParm);
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");

		if(CG_WaterLevel(cent) == 3)
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*drown.wav"));
		else
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, va("sound/player/death.wav", event - EV_DEATH1 + 1)));

		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary(es);
		break;

	// powerup events
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_QUAD;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.quadSound);
		break;
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_BATTLESUIT;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.protectSound);
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_REGEN;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.regenSound);
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		// don't play gib sound when using the kamikaze because it interferes
		// with the kamikaze sound, downside is that the gib sound will also
		// not be played when someone is gibbed while just carrying the kamikaze
		if(!(es->eFlags & EF_KAMIKAZE))
			trap_S_StartSound(nil, es->number, CHAN_BODY, cgs.media.gibSound);
		gibplayer(cent->lerporigin);
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		trap_S_StopLoopingSound(es->number);
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		drawbeam(cent);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		cgerrorf("Unknown event: %i", event);
		break;
	}
}

void
chkevents(centity_t *cent)
{
	// check for event-only entities
	if(cent->currstate.eType > ET_EVENTS){
		if(cent->prevevent)
			return;	// already fired
		// if this is a player event set the entity number of the client entity number
		if(cent->currstate.eFlags & EF_PLAYER_EVENT)
			cent->currstate.number = cent->currstate.otherEntityNum;

		cent->prevevent = 1;

		cent->currstate.event = cent->currstate.eType - ET_EVENTS;
	}else{
		// check for events riding with another entity
		if(cent->currstate.event == cent->prevevent)
			return;
		cent->prevevent = cent->currstate.event;
		if((cent->currstate.event & ~EV_EVENT_BITS) == 0)
			return;
	}

	// calculate the position at exactly the frame time
	evaltrajectory(&cent->currstate.pos, cg.snap->serverTime, cent->lerporigin);
	setentsoundpos(cent);

	entevent(cent, cent->lerporigin);
}
