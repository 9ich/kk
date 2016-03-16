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
//
// g_utils.c -- misc utility functions for game module

#include "g_local.h"

typedef struct
{
	char	oldShader[MAX_QPATH];
	char	newShader[MAX_QPATH];
	float	timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int remapCount = 0;
shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

void
addshaderremap(const char *oldShader, const char *newShader, float timeOffset)
{
	int i;

	for(i = 0; i < remapCount; i++)
		if(Q_stricmp(oldShader, remappedShaders[i].oldShader) == 0){
			// found it, just update this one
			strcpy(remappedShaders[i].newShader, newShader);
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	if(remapCount < MAX_SHADER_REMAPS){
		strcpy(remappedShaders[remapCount].newShader, newShader);
		strcpy(remappedShaders[remapCount].oldShader, oldShader);
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
}

const char *
mkshaderstateconfigstr(void)
{
	static char buff[MAX_STRING_CHARS*4];
	char out[(MAX_QPATH * 2) + 5];
	int i;

	memset(buff, 0, MAX_STRING_CHARS);
	for(i = 0; i < remapCount; i++){
		Com_sprintf(out, (MAX_QPATH * 2) + 5, "%s=%s:%5.2f@", remappedShaders[i].oldShader, remappedShaders[i].newShader, remappedShaders[i].timeOffset);
		Q_strcat(buff, sizeof(buff), out);
	}
	return buff;
}

/*
=========================================================================

model / sound configstring indexes

=========================================================================
*/

/*
================
findconfigstrindex

================
*/
int
findconfigstrindex(char *name, int start, int max, qboolean create)
{
	int i;
	char s[MAX_STRING_CHARS];

	if(!name || !name[0])
		return 0;

	for(i = 1; i<max; i++){
		trap_GetConfigstring(start + i, s, sizeof(s));
		if(!s[0])
			break;
		if(!strcmp(s, name))
			return i;
	}

	if(!create)
		return 0;

	if(i == max)
		errorf("findconfigstrindex: overflow");

	trap_SetConfigstring(start + i, name);

	return i;
}

int
getmodelindex(char *name)
{
	return findconfigstrindex(name, CS_MODELS, MAX_MODELS, qtrue);
}

int
getsoundindex(char *name)
{
	return findconfigstrindex(name, CS_SOUNDS, MAX_SOUNDS, qtrue);
}

//=====================================================================

/*
================
teamcmd

Broadcasts a command to only a specific team
================
*/
void
teamcmd(team_t team, char *cmd)
{
	int i;

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			if(level.clients[i].sess.team == team)
				trap_SendServerCommand(i, va("%s", cmd));
}

/*
=============
findent

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if nil
nil will be returned if the end of the list is reached.

=============
*/
gentity_t *
findent(gentity_t *from, int fieldofs, const char *match)
{
	char *s;

	if(!from)
		from = g_entities;
	else
		from++;

	for(; from < &g_entities[level.nentities]; from++){
		if(!from->inuse)
			continue;
		s = *(char**)((byte*)from + fieldofs);
		if(!s)
			continue;
		if(!Q_stricmp(s, match))
			return from;
	}

	return nil;
}

/*
=============
picktarget

Selects a random entity from among the targets
=============
*/
#define MAXCHOICES 32

gentity_t *
picktarget(char *targetname)
{
	gentity_t *ent = nil;
	int num_choices = 0;
	gentity_t *choice[MAXCHOICES];

	if(!targetname){
		gprintf("picktarget called with nil targetname\n");
		return nil;
	}

	while(1){
		ent = findent(ent, FOFS(targetname), targetname);
		if(!ent)
			break;
		choice[num_choices++] = ent;
		if(num_choices == MAXCHOICES)
			break;
	}

	if(!num_choices){
		gprintf("picktarget: target %s not found\n", targetname);
		return nil;
	}

	return choice[rand() % num_choices];
}

/*
==============================
usetargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void
usetargets(gentity_t *ent, gentity_t *activator)
{
	gentity_t *t;

	if(!ent)
		return;

	if(ent->targetshadername && ent->newtargetshadername){
		float f = level.time * 0.001;
		addshaderremap(ent->targetshadername, ent->newtargetshadername, f);
		trap_SetConfigstring(CS_SHADERSTATE, mkshaderstateconfigstr());
	}

	if(!ent->target)
		return;

	t = nil;
	while((t = findent(t, FOFS(targetname), ent->target)) != nil){
		if(t == ent)
			gprintf("WARNING: Entity used itself.\n");
		else  if(t->use)
			t->use(t, ent, activator);

		if(!ent->inuse){
			gprintf("entity was removed while using targets\n");
			return;
		}
	}
}

/*
=============
tempvector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float   *
tv(float x, float y, float z)
{
	static int index;
	static vec3_t vecs[8];
	float *v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
===============
setmovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void
setmovedir(vec3_t angles, vec3_t movedir)
{
	static vec3_t VEC_UP = {0, -1, 0};
	static vec3_t MOVEDIR_UP = {0, 0, 1};
	static vec3_t VEC_DOWN = {0, -2, 0};
	static vec3_t MOVEDIR_DOWN = {0, 0, -1};

	if(veccmp(angles, VEC_UP))
		veccpy(MOVEDIR_UP, movedir);
	else if(veccmp(angles, VEC_DOWN))
		veccpy(MOVEDIR_DOWN, movedir);
	else
		anglevecs(angles, movedir, nil, nil);
	vecclear(angles);
}

float
vectoyaw(const vec3_t vec)
{
	float yaw;

	if(vec[YAW] == 0 && vec[PITCH] == 0)
		yaw = 0;
	else{
		if(vec[PITCH])
			yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		else if(vec[YAW] > 0)
			yaw = 90;
		else
			yaw = 270;
		if(yaw < 0)
			yaw += 360;
	}

	return yaw;
}

void
entinit(gentity_t *e)
{
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
}

/*
=================
entspawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *
entspawn(void)
{
	int i, force;
	gentity_t *e;

	e = nil;	// shut up warning
	i = 0;		// shut up warning
	for(force = 0; force < 2; force++){
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];
		for(i = MAX_CLIENTS; i<level.nentities; i++, e++){
			if(e->inuse)
				continue;

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if(!force && e->freetime > level.starttime + 2000 && level.time - e->freetime < 1000)
				continue;

			// reuse this slot
			entinit(e);
			return e;
		}
		if(i != MAX_GENTITIES)
			break;
	}
	if(i == ENTITYNUM_MAX_NORMAL){
		for(i = 0; i < MAX_GENTITIES; i++)
			gprintf("%4i: %s\n", i, g_entities[i].classname);
		errorf("entspawn: no free entities");
	}

	// open up a new slot
	level.nentities++;

	// let the server system know that there are more entities
	trap_LocateGameData(level.entities, level.nentities, sizeof(gentity_t),
			    &level.clients[0].ps, sizeof(level.clients[0]));

	entinit(e);
	return e;
}

/*
=================
nentsfree
=================
*/
qboolean
nentsfree(void)
{
	int i;
	gentity_t *e;

	e = &g_entities[MAX_CLIENTS];
	for(i = MAX_CLIENTS; i < level.nentities; i++, e++){
		if(e->inuse)
			continue;
		// slot available
		return qtrue;
	}
	return qfalse;
}

/*
=================
entfree

Marks the entity as free
=================
*/
void
entfree(gentity_t *ed)
{
	trap_UnlinkEntity(ed);	// unlink from world

	if(ed->neverfree)
		return;

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
}

/*
=================
enttemp

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *
enttemp(vec3_t origin, int event)
{
	gentity_t *e;
	vec3_t snapped;

	e = entspawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventtime = level.time;
	e->freeafterevent = qtrue;

	veccpy(origin, snapped);
	SnapVector(snapped);	// save network bandwidth
	setorigin(e, snapped);

	// find cluster for PVS
	trap_LinkEntity(e);

	return e;
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
killbox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
void
killbox(gentity_t *ent)
{
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t *hit;
	vec3_t mins, maxs;

	vecadd(ent->client->ps.origin, ent->r.mins, mins);
	vecadd(ent->client->ps.origin, ent->r.maxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for(i = 0; i<num; i++){
		hit = &g_entities[touch[i]];
		if(!hit->client)
			continue;

		// nail it
		entdamage(hit, ent, ent, nil, nil,
			 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}
}

//==============================================================================

/*
===============
addpredictable

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void
addpredictable(gentity_t *ent, int event, int eventParm)
{
	if(!ent->client)
		return;
	bgaddpredictableevent(event, eventParm, &ent->client->ps);
}

/*
===============
addevent

Adds an event+parm and twiddles the event counter
===============
*/
void
addevent(gentity_t *ent, int event, int eventParm)
{
	int bits;

	if(!event){
		gprintf("addevent: zero event added for entity %i\n", ent->s.number);
		return;
	}

	// clients need to add the event in playerState_t instead of entityState_t
	if(ent->client){
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
	}else{
		bits = ent->s.event & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}
	ent->eventtime = level.time;
}

/*
=============
mksound
=============
*/
void
mksound(gentity_t *ent, int channel, int soundIndex)
{
	gentity_t *te;

	te = enttemp(ent->r.currentOrigin, EV_GENERAL_SOUND);
	te->s.eventParm = soundIndex;
}

//==============================================================================

/*
================
setorigin

Sets the pos trajectory for a fixed position
================
*/
void
setorigin(gentity_t *ent, vec3_t origin)
{
	veccpy(origin, ent->s.pos.trBase);
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	vecclear(ent->s.pos.trDelta);

	veccpy(origin, ent->r.currentOrigin);
}

/*
================
DebugLine

  debug polygons only work when running a local game
  with r_debugSurface set to 2
================
*/
int
DebugLine(vec3_t start, vec3_t end, int color)
{
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	veccpy(start, points[0]);
	veccpy(start, points[1]);
	//points[1][2] -= 2;
	veccpy(end, points[2]);
	//points[2][2] -= 2;
	veccpy(end, points[3]);

	vecsub(end, start, dir);
	vecnorm(dir);
	dot = vecdot(dir, up);
	if(dot > 0.99 || dot < -0.99) vecset(cross, 1, 0, 0);
	else veccross(dir, up, cross);

	vecnorm(cross);

	vecmad(points[0], 2, cross, points[0]);
	vecmad(points[1], -2, cross, points[1]);
	vecmad(points[2], -2, cross, points[2]);
	vecmad(points[3], 2, cross, points[3]);

	return trap_DebugPolygonCreate(color, 4, points);
}
