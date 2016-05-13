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

qboolean
spawnstr(const char *key, const char *defaultString, char **out)
{
	int i;

	if(!level.spawning)
		*out = (char*)defaultString;
//		errorf( "spawnstr() called while not spawning" );

	for(i = 0; i < level.nspawnvars; i++)
		if(!Q_stricmp(key, level.spawnvars[i][0])){
			*out = level.spawnvars[i][1];
			return qtrue;
		}

	*out = (char*)defaultString;
	return qfalse;
}

qboolean
spawnfloat(const char *key, const char *defaultString, float *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	*out = atof(s);
	return present;
}

qboolean
spawnint(const char *key, const char *defaultString, int *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	*out = atoi(s);
	return present;
}

qboolean
spawnvec(const char *key, const char *defaultString, float *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

// fields are needed for spawning from the entity string
typedef enum
{
	F_INT,
	F_FLOAT,
	F_STRING,
	F_VECTOR,
	F_ANGLEHACK
} fieldtype_t;

typedef struct
{
	char		*name;
	size_t		ofs;
	fieldtype_t	type;
} field_t;

field_t fields[] = {
	{"classname", FOFS(classname), F_STRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_STRING},
	{"model2", FOFS(model2), F_STRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_STRING},
	{"targetname", FOFS(targetname), F_STRING},
	{"message", FOFS(message), F_STRING},
	{"team", FOFS(team), F_STRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"yaw", FOFS(s.angles), F_ANGLEHACK},
	{"targetshadername", FOFS(targetshadername), F_STRING},
	{"newtargetshadername", FOFS(newtargetshadername), F_STRING},

	{nil}
};

typedef struct
{
	char *name;
	void (*spawn)(gentity_t *ent);
} spawn_t;

void	SP_info_player_start(gentity_t *ent);
void	SP_info_player_deathmatch(gentity_t *ent);
void	SP_info_player_intermission(gentity_t *ent);

void	SP_func_plat(gentity_t *ent);
void	SP_func_static(gentity_t *ent);
void	SP_func_rotating(gentity_t *ent);
void	SP_func_bobbing(gentity_t *ent);
void	SP_func_pendulum(gentity_t *ent);
void	SP_func_button(gentity_t *ent);
void	SP_func_door(gentity_t *ent);
void	SP_func_train(gentity_t *ent);
void	SP_func_timer(gentity_t *self);

void	SP_trigger_always(gentity_t *ent);
void	SP_trigger_multiple(gentity_t *ent);
void	SP_trigger_push(gentity_t *ent);
void	SP_trigger_gravity(gentity_t *ent);
void	SP_trigger_teleport(gentity_t *ent);
void	SP_trigger_hurt(gentity_t *ent);

void	SP_target_remove_powerups(gentity_t *ent);
void	SP_target_give(gentity_t *ent);
void	SP_target_delay(gentity_t *ent);
void	SP_target_speaker(gentity_t *ent);
void	SP_target_print(gentity_t *ent);
void	SP_target_laser(gentity_t *self);
void	SP_target_score(gentity_t *ent);
void	SP_target_teleporter(gentity_t *ent);
void	SP_target_relay(gentity_t *ent);
void	SP_target_kill(gentity_t *ent);
void	SP_target_position(gentity_t *ent);
void	SP_target_location(gentity_t *ent);
void	SP_target_push(gentity_t *ent);

void	SP_light(gentity_t *self);
void	SP_info_null(gentity_t *self);
void	SP_info_notnull(gentity_t *self);
void	SP_info_camp(gentity_t *self);
void	SP_path_corner(gentity_t *self);

void	SP_misc_teleporter_dest(gentity_t *self);
void	SP_misc_model(gentity_t *ent);
void	SP_misc_portal_camera(gentity_t *ent);
void	SP_misc_portal_surface(gentity_t *ent);

void	SP_shooter_rocket(gentity_t *ent);
void	SP_shooter_plasma(gentity_t *ent);
void	SP_shooter_grenade(gentity_t *ent);

void	SP_team_CTF_redplayer(gentity_t *ent);
void	SP_team_CTF_blueplayer(gentity_t *ent);

void	SP_team_CTF_redspawn(gentity_t *ent);
void	SP_team_CTF_bluespawn(gentity_t *ent);

#ifdef MISSIONPACK
void	SP_team_blueobelisk(gentity_t *ent);
void	SP_team_redobelisk(gentity_t *ent);
void	SP_team_neutralobelisk(gentity_t *ent);
#endif
void
SP_item_botroam(gentity_t *ent) { }

spawn_t spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},	// use target_position instead
	{"info_camp", SP_info_camp},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},	// rename trigger_timer?

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_gravity", SP_trigger_gravity},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},

	{"liteEntity", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

#ifdef MISSIONPACK
	{"team_redobelisk", SP_team_redobelisk},
	{"team_blueobelisk", SP_team_blueobelisk},
	{"team_neutralobelisk", SP_team_neutralobelisk},
#endif
	{"item_botroam", SP_item_botroam},

	{nil, 0}
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean
G_CallSpawn(gentity_t *ent)
{
	spawn_t *s;
	gitem_t *item;

	if(!ent->classname){
		gprintf("G_CallSpawn: nil classname\n");
		return qfalse;
	}

	// check item spawn functions
	if(g_gametype.integer != GT_CA)
		for(item = bg_itemlist+1; item->classname; item++)
			if(!strcmp(item->classname, ent->classname)){
				itemspawn(ent, item);
				return qtrue;
			}

	// check normal spawn functions
	for(s = spawns; s->name; s++)
		if(!strcmp(s->name, ent->classname)){
			// found it
			s->spawn(ent);
			return qtrue;
		}
	gprintf("%s doesn't have a spawn function\n", ent->classname);
	return qfalse;
}

/*
=============
newstr

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *
newstr(const char *string)
{
	char *newb, *new_p;
	int i, l;

	l = strlen(string) + 1;

	newb = alloc(l);

	new_p = newb;

	// turn \n into a real linefeed
	for(i = 0; i< l; i++){
		if(string[i] == '\\' && i < l-1){
			i++;
			if(string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}else
			*new_p++ = string[i];
	}

	return newb;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void
G_ParseField(const char *key, const char *value, gentity_t *ent)
{
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for(f = fields; f->name; f++)
		if(!Q_stricmp(f->name, key)){
			// found it
			b = (byte*)ent;

			switch(f->type){
			case F_STRING:
				*(char**)(b+f->ofs) = newstr(value);
				break;
			case F_VECTOR:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float*)(b+f->ofs))[0] = vec[0];
				((float*)(b+f->ofs))[1] = vec[1];
				((float*)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int*)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float*)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				// skip this key if the ent has obviously
				// already been given an "angles" key
				if(((float*)(b+f->ofs))[0] != 0 ||
				  ((float*)(b+f->ofs))[1] != 0 ||
				  ((float*)(b+f->ofs))[2] != 0)
					break;
				((float*)(b+f->ofs))[0] = 0;
				((float*)(b+f->ofs))[1] = v;
				((float*)(b+f->ofs))[2] = 0;
				break;
			}
			return;
		}
}

#define ADJUST_AREAPORTAL() \
	if(ent->s.eType == ET_MOVER) \
	{ \
		trap_LinkEntity(ent); \
		trap_AdjustAreaPortalState(ent, qtrue);	\
	}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnvars[], then call the class specfic spawn function
===================
*/
void
G_SpawnGEntityFromSpawnVars(void)
{
	int i;
	gentity_t *ent;
	char *s, *value, *gametypeName;
	static char *gametypeNames[] = {"ffa", "tournament", "single", "team", "ctf", "oneflag", "obelisk", "harvester"};

	// get the next free entity
	ent = entspawn();

	for(i = 0; i < level.nspawnvars; i++)
		G_ParseField(level.spawnvars[i][0], level.spawnvars[i][1], ent);

	// check for "notsingle" flag
	if(g_gametype.integer == GT_SINGLE_PLAYER){
		spawnint("notsingle", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if(g_gametype.integer >= GT_TEAM){
		spawnint("notteam", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}else{
		spawnint("notfree", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}

	spawnint("notq3a", "0", &i);
	if(i){
		ADJUST_AREAPORTAL();
		entfree(ent);
		return;
	}

	if(spawnstr("gametype", nil, &value))
		if(g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE){
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr(value, gametypeName);
			if(!s){
				ADJUST_AREAPORTAL();
				entfree(ent);
				return;
			}
		}

	// move editor origin to pos
	veccpy(ent->s.origin, ent->s.pos.trBase);
	veccpy(ent->s.origin, ent->r.currentOrigin);

	// if we didn't get a classname, don't bother spawning anything
	if(!G_CallSpawn(ent))
		entfree(ent);
}

/*
====================
G_AddSpawnVarToken
====================
*/
char *
G_AddSpawnVarToken(const char *string)
{
	int l;
	char *dest;

	l = strlen(string);
	if(level.nspawnvarchars + l + 1 > MAX_SPAWN_VARS_CHARS)
		errorf("G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS");

	dest = level.spawnvarchars + level.nspawnvarchars;
	memcpy(dest, string, l+1);

	level.nspawnvarchars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnvars[]

This does not actually spawn an entity.
====================
*/
qboolean
G_ParseSpawnVars(void)
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.nspawnvars = 0;
	level.nspawnvarchars = 0;

	// parse the opening brace
	if(!trap_GetEntityToken(com_token, sizeof(com_token)))
		// end of spawn string
		return qfalse;
	if(com_token[0] != '{')
		errorf("G_ParseSpawnVars: found %s when expecting {", com_token);

	// go through all the key / value pairs
	while(1){
		// parse key
		if(!trap_GetEntityToken(keyname, sizeof(keyname)))
			errorf("G_ParseSpawnVars: EOF without closing brace");

		if(keyname[0] == '}')
			break;

		// parse value
		if(!trap_GetEntityToken(com_token, sizeof(com_token)))
			errorf("G_ParseSpawnVars: EOF without closing brace");

		if(com_token[0] == '}')
			errorf("G_ParseSpawnVars: closing brace without data");
		if(level.nspawnvars == MAX_SPAWN_VARS)
			errorf("G_ParseSpawnVars: MAX_SPAWN_VARS");
		level.spawnvars[level.nspawnvars][0] = G_AddSpawnVarToken(keyname);
		level.spawnvars[level.nspawnvars][1] = G_AddSpawnVarToken(com_token);
		level.nspawnvars++;
	}

	return qtrue;
}

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void
SP_worldspawn(void)
{
	char *s;

	spawnstr("classname", "", &s);
	if(Q_stricmp(s, "worldspawn"))
		errorf("SP_worldspawn: The first entity isn't 'worldspawn'");

	// make some data visible to connecting client
	trap_SetConfigstring(CS_GAME_VERSION, GAME_VERSION);

	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.starttime));

	spawnstr("music", "", &s);
	trap_SetConfigstring(CS_MUSIC, s);

	spawnstr("message", "", &s);
	trap_SetConfigstring(CS_MESSAGE, s);		// map specific message

	trap_SetConfigstring(CS_MOTD, g_motd.string);	// message of the day

	spawnstr("gravity", "0", &s);
	trap_Cvar_Set("g_gravity", s);

	spawnstr("enableDust", "0", &s);
	trap_Cvar_Set("g_enableDust", s);

	spawnstr("enableBreath", "0", &s);
	trap_Cvar_Set("g_enableBreath", s);

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring(CS_WARMUP, "");
	trap_SetConfigstring(CS_ROUNDWARMUP, "");
	if(g_restarted.integer){
		trap_Cvar_Set("g_restarted", "0");
		level.warmuptime = 0;
		// clan arena roundwarmup
		if(g_gametype.integer == GT_LMS || g_gametype.integer == GT_CA ||
		   g_gametype.integer == GT_LTS){
			beginroundwarmup();
		}
	}else if(g_doWarmup.integer){	// Turn it on
		level.warmuptime = WARMUP_NEEDPLAYERS;
		trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
		logprintf("Warmup:\n");
	}
}

/*
==============
spawnall

Parses textual entity definitions out of an entstring and spawns entities.
==============
*/
void
spawnall(void)
{
	// allow calls to entspawn*()
	level.spawning = qtrue;
	level.nspawnvars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if(!G_ParseSpawnVars())
		errorf("SpawnEntities: no entities");
	SP_worldspawn();

	// parse ents
	while(G_ParseSpawnVars())
		G_SpawnGEntityFromSpawnVars();

	level.spawning = qfalse;	// any future calls to entspawn*() will be errors
}
