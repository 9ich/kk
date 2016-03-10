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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char *cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};

/*
================
customsound

================
*/
sfxHandle_t
customsound(int clientNum, const char *soundName)
{
	clientInfo_t *ci;
	int i;

	if(soundName[0] != '*')
		return trap_S_RegisterSound(soundName, qfalse);

	if(clientNum < 0 || clientNum >= MAX_CLIENTS)
		clientNum = 0;
	ci = &cgs.clientinfo[clientNum];

	for(i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i]; i++)
		if(!strcmp(soundName, cg_customSoundNames[i]))
			return ci->sounds[i];

	cgerrorf("Unknown custom sound: %s", soundName);
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean
CG_ParseAnimationFile(const char *filename, clientInfo_t *ci)
{
	char *text_p, *prev;
	int len;
	int i;
	char *token;
	float fps;
	int skip;
	char text[20000];
	fileHandle_t f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
		return qfalse;
	if(len >= sizeof(text) - 1){
		cgprintf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	vecclear(ci->headoffset);
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while(1){
		prev = text_p;	// so we can unget
		token = COM_Parse(&text_p);
		if(!token)
			break;
		if(!Q_stricmp(token, "footsteps")){
			token = COM_Parse(&text_p);
			if(!token)
				break;
			if(!Q_stricmp(token, "default") || !Q_stricmp(token, "normal"))
				ci->footsteps = FOOTSTEP_NORMAL;
			else if(!Q_stricmp(token, "boot"))
				ci->footsteps = FOOTSTEP_BOOT;
			else if(!Q_stricmp(token, "flesh"))
				ci->footsteps = FOOTSTEP_FLESH;
			else if(!Q_stricmp(token, "mech"))
				ci->footsteps = FOOTSTEP_MECH;
			else if(!Q_stricmp(token, "energy"))
				ci->footsteps = FOOTSTEP_ENERGY;
			else
				cgprintf("Bad footsteps parm in %s: %s\n", filename, token);
			continue;
		}else if(!Q_stricmp(token, "headoffset")){
			for(i = 0; i < 3; i++){
				token = COM_Parse(&text_p);
				if(!token)
					break;
				ci->headoffset[i] = atof(token);
			}
			continue;
		}else if(!Q_stricmp(token, "sex")){
			token = COM_Parse(&text_p);
			if(!token)
				break;
			if(token[0] == 'f' || token[0] == 'F')
				ci->gender = GENDER_FEMALE;
			else if(token[0] == 'n' || token[0] == 'N')
				ci->gender = GENDER_NEUTER;
			else
				ci->gender = GENDER_MALE;
			continue;
		}else if(!Q_stricmp(token, "fixedlegs")){
			ci->fixedlegs = qtrue;
			continue;
		}else if(!Q_stricmp(token, "fixedtorso")){
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if(token[0] >= '0' && token[0] <= '9'){
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf("unknown token '%s' in %s\n", token, filename);
	}

	// read information for each frame
	for(i = 0; i < MAX_ANIMATIONS; i++){
		token = COM_Parse(&text_p);
		if(!*token){
			if(i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE){
				animations[i].firstframe = animations[TORSO_GESTURE].firstframe;
				animations[i].framelerp = animations[TORSO_GESTURE].framelerp;
				animations[i].initiallerp = animations[TORSO_GESTURE].initiallerp;
				animations[i].loopframes = animations[TORSO_GESTURE].loopframes;
				animations[i].nframes = animations[TORSO_GESTURE].nframes;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstframe = atoi(token);
		// leg only frames are adjusted to not count the upper body only frames
		if(i == LEGS_WALKCR)
			skip = animations[LEGS_WALKCR].firstframe - animations[TORSO_GESTURE].firstframe;
		if(i >= LEGS_WALKCR && i<TORSO_GETFLAG)
			animations[i].firstframe -= skip;

		token = COM_Parse(&text_p);
		if(!*token)
			break;
		animations[i].nframes = atoi(token);

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if nframes is negative the animation is reversed
		if(animations[i].nframes < 0){
			animations[i].nframes = -animations[i].nframes;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse(&text_p);
		if(!*token)
			break;
		animations[i].loopframes = atoi(token);

		token = COM_Parse(&text_p);
		if(!*token)
			break;
		fps = atof(token);
		if(fps == 0)
			fps = 1;
		animations[i].framelerp = 1000 / fps;
		animations[i].initiallerp = 1000 / fps;
	}

	if(i != MAX_ANIMATIONS){
		cgprintf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstframe = 0;
	animations[FLAG_RUN].nframes = 16;
	animations[FLAG_RUN].loopframes = 16;
	animations[FLAG_RUN].framelerp = 1000 / 15;
	animations[FLAG_RUN].initiallerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstframe = 16;
	animations[FLAG_STAND].nframes = 5;
	animations[FLAG_STAND].loopframes = 0;
	animations[FLAG_STAND].framelerp = 1000 / 20;
	animations[FLAG_STAND].initiallerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstframe = 16;
	animations[FLAG_STAND2RUN].nframes = 5;
	animations[FLAG_STAND2RUN].loopframes = 1;
	animations[FLAG_STAND2RUN].framelerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initiallerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
static qboolean
CG_FileExists(const char *filename)
{
	int len;

	len = trap_FS_FOpenFile(filename, nil, FS_READ);
	if(len>0)
		return qtrue;
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean
CG_FindClientModelFile(char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelname, const char *skinname, const char *base, const char *ext)
{
	char *team, *charactersFolder;
	int i;

	if(cgs.gametype >= GT_TEAM){
		switch(ci->team){
		case TEAM_BLUE: {
			team = "blue";
			break;
		}
		default: {
			team = "red";
			break;
		}
		}
	}else
		team = "default";
	charactersFolder = "";
	while(1){
		for(i = 0; i < 2; i++){
			if(i == 0 && teamName && *teamName)
				//								"models/players/characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelname, teamName, base, skinname, team, ext);
			else
				//								"models/players/characters/james/lower_lily_red.skin"
				Com_sprintf(filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelname, base, skinname, team, ext);
			if(CG_FileExists(filename))
				return qtrue;
			if(cgs.gametype >= GT_TEAM){
				if(i == 0 && teamName && *teamName)
					//								"models/players/characters/james/stroggs/lower_red.skin"
					Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelname, teamName, base, team, ext);
				else
					//								"models/players/characters/james/lower_red.skin"
					Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelname, base, team, ext);
			}else{
				if(i == 0 && teamName && *teamName)
					//								"models/players/characters/james/stroggs/lower_lily.skin"
					Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelname, teamName, base, skinname, ext);
				else
					//								"models/players/characters/james/lower_lily.skin"
					Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelname, base, skinname, ext);
			}
			if(CG_FileExists(filename))
				return qtrue;
			if(!teamName || !*teamName)
				break;
		}
		// if tried the heads folder first
		if(charactersFolder[0])
			break;
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static qboolean
CG_FindClientHeadFile(char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headmodelname, const char *headskinname, const char *base, const char *ext)
{
	char *team, *headsFolder;
	int i;

	if(cgs.gametype >= GT_TEAM){
		switch(ci->team){
		case TEAM_BLUE: {
			team = "blue";
			break;
		}
		default: {
			team = "red";
			break;
		}
		}
	}else
		team = "default";

	if(headmodelname[0] == '*'){
		headsFolder = "heads/";
		headmodelname++;
	}else
		headsFolder = "";
	while(1){
		for(i = 0; i < 2; i++){
			if(i == 0 && teamName && *teamName)
				Com_sprintf(filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headmodelname, headskinname, teamName, base, team, ext);
			else
				Com_sprintf(filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headmodelname, headskinname, base, team, ext);
			if(CG_FileExists(filename))
				return qtrue;
			if(cgs.gametype >= GT_TEAM){
				if(i == 0 &&  teamName && *teamName)
					Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headmodelname, teamName, base, team, ext);
				else
					Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headmodelname, base, team, ext);
			}else{
				if(i == 0 && teamName && *teamName)
					Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headmodelname, teamName, base, headskinname, ext);
				else
					Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headmodelname, base, headskinname, ext);
			}
			if(CG_FileExists(filename))
				return qtrue;
			if(!teamName || !*teamName)
				break;
		}
		// if tried the heads folder first
		if(headsFolder[0])
			break;
		headsFolder = "heads/";
	}

	return qfalse;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean
CG_RegisterClientSkin(clientInfo_t *ci, const char *teamName, const char *modelname, const char *skinname, const char *headmodelname, const char *headskinname)
{
	char filename[MAX_QPATH];

	/*
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%slower_%s.skin", modelname, teamName, skinname );
	ci->legsskin = trap_R_RegisterSkin( filename );
	if (!ci->legsskin) {
	        Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%slower_%s.skin", modelname, teamName, skinname );
	        ci->legsskin = trap_R_RegisterSkin( filename );
	        if (!ci->legsskin) {
	                Com_Printf( "Leg skin load failure: %s\n", filename );
	        }
	}


	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%supper_%s.skin", modelname, teamName, skinname );
	ci->torsoskin = trap_R_RegisterSkin( filename );
	if (!ci->torsoskin) {
	        Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%supper_%s.skin", modelname, teamName, skinname );
	        ci->torsoskin = trap_R_RegisterSkin( filename );
	        if (!ci->torsoskin) {
	                Com_Printf( "Torso skin load failure: %s\n", filename );
	        }
	}
	*/
	if(CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelname, skinname, "lower", "skin"))
		ci->legsskin = trap_R_RegisterSkin(filename);
	if(!ci->legsskin)
		Com_Printf("Leg skin load failure: %s\n", filename);

	if(CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelname, skinname, "upper", "skin"))
		ci->torsoskin = trap_R_RegisterSkin(filename);
	if(!ci->torsoskin)
		Com_Printf("Torso skin load failure: %s\n", filename);

	if(CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headmodelname, headskinname, "head", "skin"))
		ci->headskin = trap_R_RegisterSkin(filename);
	if(!ci->headskin)
		Com_Printf("Head skin load failure: %s\n", filename);

	// if any skins failed to load
	if(!ci->legsskin || !ci->torsoskin || !ci->headskin)
		return qfalse;
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean
CG_RegisterClientModelname(clientInfo_t *ci, const char *modelname, const char *skinname, const char *headmodelname, const char *headskinname, const char *teamName)
{
	char filename[MAX_QPATH];
	const char *headName;
	char newTeamName[MAX_QPATH];

	if(headmodelname[0] == '\0')
		headName = modelname;
	else
		headName = headmodelname;
	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower.md3", modelname);
	ci->legsmodel = trap_R_RegisterModel(filename);
	if(!ci->legsmodel){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower.md3", modelname);
		ci->legsmodel = trap_R_RegisterModel(filename);
		if(!ci->legsmodel){
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper.md3", modelname);
	ci->torsomodel = trap_R_RegisterModel(filename);
	if(!ci->torsomodel){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper.md3", modelname);
		ci->torsomodel = trap_R_RegisterModel(filename);
		if(!ci->torsomodel){
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	if(headName[0] == '*')
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", &headmodelname[1], &headmodelname[1]);
	else
		Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headName);
	ci->headmodel = trap_R_RegisterModel(filename);
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if(!ci->headmodel && headName[0] != '*'){
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", headmodelname, headmodelname);
		ci->headmodel = trap_R_RegisterModel(filename);
	}
	if(!ci->headmodel){
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	// if any skins failed to load, return failure
	if(!CG_RegisterClientSkin(ci, teamName, modelname, skinname, headName, headskinname)){
		if(teamName && *teamName){
			Com_Printf("Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelname, skinname, headName, headskinname);
			if(ci->team == TEAM_BLUE)
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			else
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			if(!CG_RegisterClientSkin(ci, newTeamName, modelname, skinname, headName, headskinname)){
				Com_Printf("Failed to load skin file: %s : %s : %s, %s : %s\n", newTeamName, modelname, skinname, headName, headskinname);
				return qfalse;
			}
		}else{
			Com_Printf("Failed to load skin file: %s : %s, %s : %s\n", modelname, skinname, headName, headskinname);
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelname);
	if(!CG_ParseAnimationFile(filename, ci)){
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/animation.cfg", modelname);
		if(!CG_ParseAnimationFile(filename, ci)){
			Com_Printf("Failed to load animation file %s\n", filename);
			return qfalse;
		}
	}

	if(CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, headskinname, "icon", "skin"))
		ci->modelicon = trap_R_RegisterShaderNoMip(filename);
	else if(CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, headskinname, "icon", "tga"))
		ci->modelicon = trap_R_RegisterShaderNoMip(filename);

	if(!ci->modelicon)
		return qfalse;

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void
CG_ColorFromString(const char *v, vec3_t color)
{
	int val;

	vecclear(color);

	val = atoi(v);

	if(val < 1 || val > 7){
		vecset(color, 1, 1, 1);
		return;
	}

	if(val & 1)
		color[2] = 1.0f;
	if(val & 2)
		color[1] = 1.0f;
	if(val & 4)
		color[0] = 1.0f;
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void
CG_LoadClientInfo(int clientNum, clientInfo_t *ci)
{
	const char *dir, *fallback;
	int i, modelloaded;
	const char *s;
	char teamname[MAX_QPATH];

	teamname[0] = 0;
#ifdef MISSIONPACK
	if(cgs.gametype >= GT_TEAM){
		if(ci->team == TEAM_BLUE)
			Q_strncpyz(teamname, cg_blueTeamName.string, sizeof(teamname));
		else
			Q_strncpyz(teamname, cg_redTeamName.string, sizeof(teamname));
	}
	if(teamname[0])
		strcat(teamname, "/");

#endif
	modelloaded = qtrue;
	if(!CG_RegisterClientModelname(ci, ci->modelname, ci->skinname, ci->headmodelname, ci->headskinname, teamname)){
		if(cg_buildScript.integer)
			cgerrorf("CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelname, ci->skinname, ci->headmodelname, ci->headskinname, teamname);

		// fall back to default team name
		if(cgs.gametype >= GT_TEAM){
			// keep skin name
			if(ci->team == TEAM_BLUE)
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname));
			else
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname));
			if(!CG_RegisterClientModelname(ci, DEFAULT_TEAM_MODEL, ci->skinname, DEFAULT_TEAM_HEAD, ci->skinname, teamname))
				cgerrorf("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinname);
		}else  if(!CG_RegisterClientModelname(ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname))
			cgerrorf("DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL);

		modelloaded = qfalse;
	}

	ci->newanims = qfalse;
	if(ci->torsomodel){
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if(trap_R_LerpTag(&tag, ci->torsomodel, 0, 0, 1, "tag_flag"))
			ci->newanims = qtrue;
	}

	// sounds
	dir = ci->modelname;
	fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for(i = 0; i < MAX_CUSTOM_SOUNDS; i++){
		s = cg_customSoundNames[i];
		if(!s)
			break;
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if(modelloaded)
			ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
		if(!ci->sounds[i])
			ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for(i = 0; i < MAX_GENTITIES; i++)
		if(cg_entities[i].currstate.clientNum == clientNum
		   && cg_entities[i].currstate.eType == ET_PLAYER)
			resetplayerent(&cg_entities[i]);
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void
CG_CopyClientInfoModel(clientInfo_t *from, clientInfo_t *to)
{
	veccpy(from->headoffset, to->headoffset);
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->legsmodel = from->legsmodel;
	to->legsskin = from->legsskin;
	to->torsomodel = from->torsomodel;
	to->torsoskin = from->torsoskin;
	to->headmodel = from->headmodel;
	to->headskin = from->headskin;
	to->modelicon = from->modelicon;

	to->newanims = from->newanims;

	memcpy(to->animations, from->animations, sizeof(to->animations));
	memcpy(to->sounds, from->sounds, sizeof(to->sounds));
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean
CG_ScanForExistingClientInfo(clientInfo_t *ci)
{
	int i;
	clientInfo_t *match;

	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[i];
		if(!match->infovalid)
			continue;
		if(match->deferred)
			continue;
		if(!Q_stricmp(ci->modelname, match->modelname)
		   && !Q_stricmp(ci->skinname, match->skinname)
		   && !Q_stricmp(ci->headmodelname, match->headmodelname)
		   && !Q_stricmp(ci->headskinname, match->headskinname)
		   && !Q_stricmp(ci->blueteam, match->blueteam)
		   && !Q_stricmp(ci->redteam, match->redteam)
		   && (cgs.gametype < GT_TEAM || ci->team == match->team)){
			// this clientinfo is identical, so use its handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel(match, ci);

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void
CG_SetDeferredClientInfo(int clientNum, clientInfo_t *ci)
{
	int i;
	clientInfo_t *match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[i];
		if(!match->infovalid || match->deferred)
			continue;
		if(Q_stricmp(ci->skinname, match->skinname) ||
		   Q_stricmp(ci->modelname, match->modelname) ||
//			 Q_stricmp( ci->headmodelname, match->headmodelname ) ||
//			 Q_stricmp( ci->headskinname, match->headskinname ) ||
		   (cgs.gametype >= GT_TEAM && ci->team != match->team))
			continue;
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo(clientNum, ci);
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if(cgs.gametype >= GT_TEAM){
		for(i = 0; i < cgs.maxclients; i++){
			match = &cgs.clientinfo[i];
			if(!match->infovalid || match->deferred)
				continue;
			if(Q_stricmp(ci->skinname, match->skinname) ||
			   (cgs.gametype >= GT_TEAM && ci->team != match->team))
				continue;
			ci->deferred = qtrue;
			CG_CopyClientInfoModel(match, ci);
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo(clientNum, ci);
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[i];
		if(!match->infovalid)
			continue;

		ci->deferred = qtrue;
		CG_CopyClientInfoModel(match, ci);
		return;
	}

	// we should never get here...
	cgprintf("CG_SetDeferredClientInfo: no valid clients!\n");

	CG_LoadClientInfo(clientNum, ci);
}

/*
======================
newclientinfo
======================
*/
void
newclientinfo(int clientNum)
{
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char *configstring;
	const char *v;
	char *slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = getconfigstr(clientNum + CS_PLAYERS);
	if(!configstring[0]){
		memset(ci, 0, sizeof(*ci));
		return;	// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&newInfo, 0, sizeof(newInfo));

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name, v, sizeof(newInfo.name));

	// colors
	v = Info_ValueForKey(configstring, "c1");
	CG_ColorFromString(v, newInfo.color1);

	newInfo.c1rgba[0] = 255 * newInfo.color1[0];
	newInfo.c1rgba[1] = 255 * newInfo.color1[1];
	newInfo.c1rgba[2] = 255 * newInfo.color1[2];
	newInfo.c1rgba[3] = 255;

	v = Info_ValueForKey(configstring, "c2");
	CG_ColorFromString(v, newInfo.color2);

	newInfo.c2rgba[0] = 255 * newInfo.color2[0];
	newInfo.c2rgba[1] = 255 * newInfo.color2[1];
	newInfo.c2rgba[2] = 255 * newInfo.color2[2];
	newInfo.c2rgba[3] = 255;

	// bot skill
	v = Info_ValueForKey(configstring, "skill");
	newInfo.botskill = atoi(v);

	// handicap
	v = Info_ValueForKey(configstring, "hc");
	newInfo.handicap = atoi(v);

	// wins
	v = Info_ValueForKey(configstring, "w");
	newInfo.wins = atoi(v);

	// losses
	v = Info_ValueForKey(configstring, "l");
	newInfo.losses = atoi(v);

	// team
	v = Info_ValueForKey(configstring, "t");
	newInfo.team = atoi(v);

	// team task
	v = Info_ValueForKey(configstring, "tt");
	newInfo.teamtask = atoi(v);

	// team leader
	v = Info_ValueForKey(configstring, "tl");
	newInfo.teamleader = atoi(v);

	v = Info_ValueForKey(configstring, "g_redteam");
	Q_strncpyz(newInfo.redteam, v, MAX_TEAMNAME);

	v = Info_ValueForKey(configstring, "g_blueteam");
	Q_strncpyz(newInfo.blueteam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey(configstring, "model");
	if(cg_forceModel.integer){
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if(cgs.gametype >= GT_TEAM){
			Q_strncpyz(newInfo.modelname, DEFAULT_TEAM_MODEL, sizeof(newInfo.modelname));
			Q_strncpyz(newInfo.skinname, "default", sizeof(newInfo.skinname));
		}else{
			trap_Cvar_VariableStringBuffer("model", modelStr, sizeof(modelStr));
			if((skin = strchr(modelStr, '/')) == nil)
				skin = "default";
			else
				*skin++ = 0;

			Q_strncpyz(newInfo.skinname, skin, sizeof(newInfo.skinname));
			Q_strncpyz(newInfo.modelname, modelStr, sizeof(newInfo.modelname));
		}

		if(cgs.gametype >= GT_TEAM){
			// keep skin name
			slash = strchr(v, '/');
			if(slash)
				Q_strncpyz(newInfo.skinname, slash + 1, sizeof(newInfo.skinname));
		}
	}else{
		Q_strncpyz(newInfo.modelname, v, sizeof(newInfo.modelname));

		slash = strchr(newInfo.modelname, '/');
		if(!slash)
			// modelname didn not include a skin name
			Q_strncpyz(newInfo.skinname, "default", sizeof(newInfo.skinname));
		else{
			Q_strncpyz(newInfo.skinname, slash + 1, sizeof(newInfo.skinname));
			// truncate modelname
			*slash = 0;
		}
	}

	// head model
	v = Info_ValueForKey(configstring, "hmodel");
	if(cg_forceModel.integer){
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if(cgs.gametype >= GT_TEAM){
			Q_strncpyz(newInfo.headmodelname, DEFAULT_TEAM_HEAD, sizeof(newInfo.headmodelname));
			Q_strncpyz(newInfo.headskinname, "default", sizeof(newInfo.headskinname));
		}else{
			trap_Cvar_VariableStringBuffer("headmodel", modelStr, sizeof(modelStr));
			if((skin = strchr(modelStr, '/')) == nil)
				skin = "default";
			else
				*skin++ = 0;

			Q_strncpyz(newInfo.headskinname, skin, sizeof(newInfo.headskinname));
			Q_strncpyz(newInfo.headmodelname, modelStr, sizeof(newInfo.headmodelname));
		}

		if(cgs.gametype >= GT_TEAM){
			// keep skin name
			slash = strchr(v, '/');
			if(slash)
				Q_strncpyz(newInfo.headskinname, slash + 1, sizeof(newInfo.headskinname));
		}
	}else{
		Q_strncpyz(newInfo.headmodelname, v, sizeof(newInfo.headmodelname));

		slash = strchr(newInfo.headmodelname, '/');
		if(!slash)
			// modelname didn not include a skin name
			Q_strncpyz(newInfo.headskinname, "default", sizeof(newInfo.headskinname));
		else{
			Q_strncpyz(newInfo.headskinname, slash + 1, sizeof(newInfo.headskinname));
			// truncate modelname
			*slash = 0;
		}
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if(!CG_ScanForExistingClientInfo(&newInfo)){
		qboolean forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if(forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading)){
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo(clientNum, &newInfo);
			// if we are low on memory, leave them with this model
			if(forceDefer){
				cgprintf("Memory is low. Using deferred model.\n");
				newInfo.deferred = qfalse;
			}
		}else
			CG_LoadClientInfo(clientNum, &newInfo);
	}

	// replace whatever was there with the new one
	newInfo.infovalid = qtrue;
	*ci = newInfo;
}

/*
======================
loaddeferred

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void
loaddeferred(void)
{
	int i;
	clientInfo_t *ci;

	// scan for a deferred player to load
	for(i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++)
		if(ci->infovalid && ci->deferred){
			// if we are low on memory, leave it deferred
			if(trap_MemoryRemaining() < 4000000){
				cgprintf("Memory is low. Using deferred model.\n");
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo(i, ci);
//			break;
		}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void
CG_SetLerpFrameAnimation(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;

	lf->animnum = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if(newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS)
		cgerrorf("Bad animation number: %i", newAnimation);

	anim = &ci->animations[newAnimation];

	lf->animation = anim;
	lf->animtime = lf->frametime + anim->initiallerp;

	if(cg_debugAnim.integer)
		cgprintf("Anim: %i\n", newAnimation);
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldframe, and cg.backlerp
cg.time should be between oldframetime and frametime after exit
===============
*/
static void
CG_RunLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	int f, nframes;
	animation_t *anim;

	// debugging tool to get no animations
	if(cg_animSpeed.integer == 0){
		lf->oldframe = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if(newAnimation != lf->animnum || !lf->animation)
		CG_SetLerpFrameAnimation(ci, lf, newAnimation);

	// if we have passed the current frame, move it to
	// oldframe and calculate a new frame
	if(cg.time >= lf->frametime){
		lf->oldframe = lf->frame;
		lf->oldframetime = lf->frametime;

		// get the next frame based on the animation
		anim = lf->animation;
		if(!anim->framelerp)
			return;					// shouldn't happen
		if(cg.time < lf->animtime)
			lf->frametime = lf->animtime;	// initial lerp
		else
			lf->frametime = lf->oldframetime + anim->framelerp;
		f = (lf->frametime - lf->animtime) / anim->framelerp;
		f *= speedScale;	// adjust for haste, etc

		nframes = anim->nframes;
		if(anim->flipflop)
			nframes *= 2;
		if(f >= nframes){
			f -= nframes;
			if(anim->loopframes){
				f %= anim->loopframes;
				f += anim->nframes - anim->loopframes;
			}else{
				f = nframes - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frametime = cg.time;
			}
		}
		if(anim->reversed)
			lf->frame = anim->firstframe + anim->nframes - 1 - f;
		else if(anim->flipflop && f>=anim->nframes)
			lf->frame = anim->firstframe + anim->nframes - 1 - (f%anim->nframes);
		else
			lf->frame = anim->firstframe + f;
		if(cg.time > lf->frametime){
			lf->frametime = cg.time;
			if(cg_debugAnim.integer)
				cgprintf("Clamp lf->frametime\n");
		}
	}

	if(lf->frametime > cg.time + 200)
		lf->frametime = cg.time;

	if(lf->oldframetime > cg.time)
		lf->oldframetime = cg.time;
	// calculate current lerp value
	if(lf->frametime == lf->oldframetime)
		lf->backlerp = 0;
	else
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldframetime) / (lf->frametime - lf->oldframetime);
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void
CG_ClearLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animnum)
{
	lf->frametime = lf->oldframetime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animnum);
	lf->oldframe = lf->frame = lf->animation->firstframe;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void
CG_PlayerAnimation(centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
		   int *torsoOld, int *torso, float *torsoBackLerp)
{
	clientInfo_t *ci;
	int clientNum;
	float speedScale;

	clientNum = cent->currstate.clientNum;

	if(cg_noPlayerAnims.integer){
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if(cent->currstate.powerups & (1 << PW_HASTE))
		speedScale = 1.5;
	else
		speedScale = 1;

	ci = &cgs.clientinfo[clientNum];

	// do the shuffle turn frames locally
	if(cent->pe.legs.yawing && (cent->currstate.legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE)
		CG_RunLerpFrame(ci, &cent->pe.legs, LEGS_TURN, speedScale);
	else
		CG_RunLerpFrame(ci, &cent->pe.legs, cent->currstate.legsAnim, speedScale);

	*legsOld = cent->pe.legs.oldframe;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame(ci, &cent->pe.torso, cent->currstate.torsoAnim, speedScale);

	*torsoOld = cent->pe.torso.oldframe;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void
CG_SwingAngles(float destination, float swingTolerance, float clampTolerance,
	       float speed, float *angle, qboolean *swinging)
{
	float swing;
	float move;
	float scale;

	if(!*swinging){
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if(swing > swingTolerance || swing < -swingTolerance)
			*swinging = qtrue;
	}

	if(!*swinging)
		return;

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if(scale < swingTolerance * 0.5)
		scale = 0.5;
	else if(scale < swingTolerance)
		scale = 1.0;
	else
		scale = 2.0;

	// swing towards the destination angle
	if(swing >= 0){
		move = cg.frametime * scale * speed;
		if(move >= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}else if(swing < 0){
		move = cg.frametime * scale * -speed;
		if(move <= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if(swing > clampTolerance)
		*angle = AngleMod(destination - (clampTolerance - 1));
	else if(swing < -clampTolerance)
		*angle = AngleMod(destination + (clampTolerance - 1));
}

/*
=================
CG_AddPainTwitch
=================
*/
static void
CG_AddPainTwitch(centity_t *cent, vec3_t torsoAngles)
{
	int t;
	float f;

	t = cg.time - cent->pe.paintime;
	if(t >= PAIN_TWITCH_TIME)
		return;

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if(cent->pe.paindir)
		torsoAngles[ROLL] += 20 * f;
	else
		torsoAngles[ROLL] -= 20 * f;
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpangles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void
CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t legsAngles, torsoAngles, headAngles;
	float dest;
	static int movementOffsets[8] = {0, 22, 45, -22, 0, 22, -45, -22};
	vec3_t velocity;
	float speed;
	int dir, clientNum;
	clientInfo_t *ci;

	veccpy(cent->lerpangles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	vecclear(legsAngles);
	vecclear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if((cent->currstate.legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
	   || ((cent->currstate.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND
	       && (cent->currstate.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)){
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;		// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;		// always center
	}

	// adjust legs for movement dir
	if(cent->currstate.eFlags & EF_DEAD)
		// don't let dead bodies twitch
		dir = 0;
	else{
		dir = cent->currstate.angles2[YAW];
		if(dir < 0 || dir > 7)
			cgerrorf("Bad player movement angle");
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[dir];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[dir];

	// torso
	CG_SwingAngles(torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yaw, &cent->pe.torso.yawing);
	CG_SwingAngles(legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yaw, &cent->pe.legs.yawing);

	torsoAngles[YAW] = cent->pe.torso.yaw;
	legsAngles[YAW] = cent->pe.legs.yaw;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if(headAngles[PITCH] > 180)
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	else
		dest = headAngles[PITCH] * 0.75f;
	CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitch, &cent->pe.torso.pitching);
	torsoAngles[PITCH] = cent->pe.torso.pitch;

	//
	clientNum = cent->currstate.clientNum;
	if(clientNum >= 0 && clientNum < MAX_CLIENTS){
		ci = &cgs.clientinfo[clientNum];
		if(ci->fixedtorso)
			torsoAngles[PITCH] = 0.0f;
	}

	// --------- roll -------------

	// lean towards the direction of travel
	veccpy(cent->currstate.pos.trDelta, velocity);
	speed = vecnorm(velocity);
	if(speed){
		vec3_t axis[3];
		float side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side = speed * vecdot(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side = speed * vecdot(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currstate.clientNum;
	if(clientNum >= 0 && clientNum < MAX_CLIENTS){
		ci = &cgs.clientinfo[clientNum];
		if(ci->fixedlegs){
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}

//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void
CG_HasteTrail(centity_t *cent)
{
	localEntity_t *smoke;
	vec3_t origin;
	int anim;

	if(cent->trailtime > cg.time)
		return;
	anim = cent->pe.legs.animnum & ~ANIM_TOGGLEBIT;
	if(anim != LEGS_RUN && anim != LEGS_BACK)
		return;

	cent->trailtime += 100;
	if(cent->trailtime < cg.time)
		cent->trailtime = cg.time;

	veccpy(cent->lerporigin, origin);
	origin[2] -= 16;

	smoke = smokepuff(origin, vec3_origin,
			     8,
			     1, 1, 1, 1,
			     500,
			     cg.time,
			     0,
			     0,
			     cgs.media.hastePuffShader);

	// use the optimized local entity add
	smoke->type = LE_SCALE_FADE;
}

#ifdef MISSIONPACK
/*
===============
CG_BreathPuffs
===============
*/
static void
CG_BreathPuffs(centity_t *cent, refEntity_t *head)
{
	clientInfo_t *ci;
	vec3_t up, origin;
	int contents;

	ci = &cgs.clientinfo[cent->currstate.number];

	if(!cg_enableBreath.integer)
		return;
	if(cent->currstate.number == cg.snap->ps.clientNum && !cg.thirdperson)
		return;
	if(cent->currstate.eFlags & EF_DEAD)
		return;
	contents = pointcontents(head->origin, 0);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		return;
	if(ci->breathPuffTime > cg.time)
		return;

	vecset(up, 0, 0, 8);
	vecmad(head->origin, 8, head->axis[0], origin);
	vecmad(origin, -4, head->axis[2], origin);
	smokepuff(origin, up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
	ci->breathPuffTime = cg.time + 2000;
}

/*
===============
CG_DustTrail
===============
*/
static void
CG_DustTrail(centity_t *cent)
{
	int anim;
	vec3_t end, vel;
	trace_t tr;

	if(!cg_enableDust.integer)
		return;

	if(cent->dusttrailtime > cg.time)
		return;

	anim = cent->pe.legs.animnum & ~ANIM_TOGGLEBIT;
	if(anim != LEGS_LANDB && anim != LEGS_LAND)
		return;

	cent->dusttrailtime += 40;
	if(cent->dusttrailtime < cg.time)
		cent->dusttrailtime = cg.time;

	veccpy(cent->currstate.pos.trBase, end);
	end[2] -= 64;
	cgtrace(&tr, cent->currstate.pos.trBase, nil, nil, end, cent->currstate.number, MASK_PLAYERSOLID);

	if(!(tr.surfaceFlags & SURF_DUST))
		return;

	veccpy(cent->currstate.pos.trBase, end);
	end[2] -= 16;

	vecset(vel, 0, 0, -30);
	smokepuff(end, vel,
		     24,
		     .8f, .8f, 0.7f, 0.33f,
		     500,
		     cg.time,
		     0,
		     0,
		     cgs.media.dustPuffShader);
}

#endif

/*
===============
CG_TrailItem
===============
*/
static void
CG_TrailItem(centity_t *cent, qhandle_t hModel)
{
	refEntity_t ent;
	vec3_t angles;
	vec3_t axis[3];

	veccpy(cent->lerpangles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	vecmad(cent->lerporigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerFlag
===============
*/
static void
CG_PlayerFlag(centity_t *cent, qhandle_t hSkin, refEntity_t *torso)
{
	clientInfo_t *ci;
	refEntity_t pole;
	refEntity_t flag;
	vec3_t angles, dir;
	int legsAnim, flagAnim, updateangles;
	float angle, d;

	// show the flag pole model
	memset(&pole, 0, sizeof(pole));
	pole.hModel = cgs.media.flagPoleModel;
	veccpy(torso->lightingOrigin, pole.lightingOrigin);
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	entontag(&pole, torso, torso->hModel, "tag_flag");
	trap_R_AddRefEntityToScene(&pole);

	// show the flag model
	memset(&flag, 0, sizeof(flag));
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;
	veccpy(torso->lightingOrigin, flag.lightingOrigin);
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;

	vecclear(angles);

	updateangles = qfalse;
	legsAnim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;
	if(legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR)
		flagAnim = FLAG_STAND;
	else if(legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR){
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	}else{
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if(updateangles){
		veccpy(cent->currstate.pos.trDelta, dir);
		// add gravity
		dir[2] += 100;
		vecnorm(dir);
		d = vecdot(pole.axis[2], dir);
		// if there is enough movement orthogonal to the flag pole
		if(fabs(d) < 0.9){
			//
			d = vecdot(pole.axis[0], dir);
			if(d > 1.0f)
				d = 1.0f;
			else if(d < -1.0f)
				d = -1.0f;
			angle = acos(d);

			d = vecdot(pole.axis[1], dir);
			if(d < 0)
				angles[YAW] = 360 - angle * 180 / M_PI;
			else
				angles[YAW] = angle * 180 / M_PI;
			if(angles[YAW] < 0)
				angles[YAW] += 360;
			if(angles[YAW] > 360)
				angles[YAW] -= 360;

			//vectoangles( cent->currstate.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yaw;
			// change the yaw angle
			CG_SwingAngles(angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yaw, &cent->pe.flag.yawing);
		}

		/*
		d = vecdot(pole.axis[2], dir);
		angle = Q_acos(d);

		d = vecdot(pole.axis[1], dir);
		if (d < 0) {
		        angle = 360 - angle * 180 / M_PI;
		}
		else {
		        angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
		        flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
		        flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles[YAW] = cent->pe.flag.yaw;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[cent->currstate.clientNum];
	CG_RunLerpFrame(ci, &cent->pe.flag, flagAnim, 1);
	flag.oldframe = cent->pe.flag.oldframe;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis(angles, flag.axis);
	rotentontag(&flag, &pole, pole.hModel, "tag_flag");

	trap_R_AddRefEntityToScene(&flag);
}

#ifdef MISSIONPACK
/*
===============
CG_PlayerTokens
===============
*/
static void
CG_PlayerTokens(centity_t *cent, int renderfx)
{
	int tokens, i, j;
	float angle;
	refEntity_t ent;
	vec3_t dir, origin;
	skulltrail_t *trail;
	if(cent->currstate.number >= MAX_CLIENTS)
		return;
	trail = &cg.skulltrails[cent->currstate.number];
	tokens = cent->currstate.generic1;
	if(!tokens){
		trail->numpositions = 0;
		return;
	}

	if(tokens > MAX_SKULLTRAIL)
		tokens = MAX_SKULLTRAIL;

	// add skulls if there are more than last time
	for(i = 0; i < tokens - trail->numpositions; i++){
		for(j = trail->numpositions; j > 0; j--)
			veccpy(trail->positions[j-1], trail->positions[j]);
		veccpy(cent->lerporigin, trail->positions[0]);
	}
	trail->numpositions = tokens;

	// move all the skulls along the trail
	veccpy(cent->lerporigin, origin);
	for(i = 0; i < trail->numpositions; i++){
		vecsub(trail->positions[i], origin, dir);
		if(vecnorm(dir) > 30)
			vecmad(origin, 30, dir, trail->positions[i]);
		veccpy(trail->positions[i], origin);
	}

	memset(&ent, 0, sizeof(ent));
	if(cgs.clientinfo[cent->currstate.clientNum].team == TEAM_BLUE)
		ent.hModel = cgs.media.redCubeModel;
	else
		ent.hModel = cgs.media.blueCubeModel;
	ent.renderfx = renderfx;

	veccpy(cent->lerporigin, origin);
	for(i = 0; i < trail->numpositions; i++){
		vecsub(origin, trail->positions[i], ent.axis[0]);
		ent.axis[0][2] = 0;
		vecnorm(ent.axis[0]);
		vecset(ent.axis[2], 0, 0, 1);
		veccross(ent.axis[0], ent.axis[2], ent.axis[1]);

		veccpy(trail->positions[i], ent.origin);
		angle = (((cg.time + 500 * MAX_SKULLTRAIL - 500 * i) / 16) & 255) * (M_PI * 2) / 255;
		ent.origin[2] += sin(angle) * 10;
		trap_R_AddRefEntityToScene(&ent);
		veccpy(trail->positions[i], origin);
	}
}

#endif

/*
===============
CG_PlayerPowerups
===============
*/
static void
CG_PlayerPowerups(centity_t *cent, refEntity_t *torso)
{
	int powerups;
	clientInfo_t *ci;

	powerups = cent->currstate.powerups;
	if(!powerups)
		return;

	// quad gives a dlight
	if(powerups & (1 << PW_QUAD))
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 0.2f, 0.2f, 1);

	// flight plays a looped sound
	if(powerups & (1 << PW_FLIGHT))
		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, cgs.media.flightSound);

	ci = &cgs.clientinfo[cent->currstate.clientNum];
	// redflag
	if(powerups & (1 << PW_REDFLAG)){
		if(ci->newanims)
			CG_PlayerFlag(cent, cgs.media.redFlagFlapSkin, torso);
		else
			CG_TrailItem(cent, cgs.media.redFlagModel);
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f);
	}

	// blueflag
	if(powerups & (1 << PW_BLUEFLAG)){
		if(ci->newanims)
			CG_PlayerFlag(cent, cgs.media.blueFlagFlapSkin, torso);
		else
			CG_TrailItem(cent, cgs.media.blueFlagModel);
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0);
	}

	// neutralflag
	if(powerups & (1 << PW_NEUTRALFLAG)){
		if(ci->newanims)
			CG_PlayerFlag(cent, cgs.media.neutralFlagFlapSkin, torso);
		else
			CG_TrailItem(cent, cgs.media.neutralFlagModel);
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 1.0, 1.0, 1.0);
	}

	// haste leaves smoke trails
	if(powerups & (1 << PW_HASTE))
		CG_HasteTrail(cent);
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void
CG_PlayerFloatSprite(centity_t *cent, qhandle_t shader)
{
	int rf;
	refEntity_t ent;

	if(cent->currstate.number == cg.snap->ps.clientNum && !cg.thirdperson)
		rf = RF_THIRD_PERSON;	// only show in mirrors
	else
		rf = 0;

	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void
CG_PlayerSprites(centity_t *cent)
{
	int team;

	if(cent->currstate.eFlags & EF_CONNECTION){
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		return;
	}

	if(cent->currstate.eFlags & EF_TALK){
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_IMPRESSIVE){
		CG_PlayerFloatSprite(cent, cgs.media.medalImpressive);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_EXCELLENT){
		CG_PlayerFloatSprite(cent, cgs.media.medalExcellent);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_GAUNTLET){
		CG_PlayerFloatSprite(cent, cgs.media.medalGauntlet);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_DEFEND){
		CG_PlayerFloatSprite(cent, cgs.media.medalDefend);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_ASSIST){
		CG_PlayerFloatSprite(cent, cgs.media.medalAssist);
		return;
	}

	if(cent->currstate.eFlags & EF_AWARD_CAP){
		CG_PlayerFloatSprite(cent, cgs.media.medalCapture);
		return;
	}

	team = cgs.clientinfo[cent->currstate.clientNum].team;
	if(!(cent->currstate.eFlags & EF_DEAD) &&
	   cg.snap->ps.persistant[PERS_TEAM] == team &&
	   cgs.gametype >= GT_TEAM){
		if(cg_drawFriend.integer)
			CG_PlayerFloatSprite(cent, cgs.media.friendShader);
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE 128
static qboolean
CG_PlayerShadow(centity_t *cent, float *shadowPlane)
{
	vec3_t end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t trace;
	float alpha;

	*shadowPlane = 0;

	if(cg_shadows.integer == 0)
		return qfalse;

	// no shadows when invisible
	if(cent->currstate.powerups & (1 << PW_INVIS))
		return qfalse;

	// send a trace down from the player to the ground
	veccpy(cent->lerporigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerporigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

	// no shadow if too high
	if(trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
		return qfalse;

	*shadowPlane = trace.endpos[2] + 1;

	if(cg_shadows.integer != 1)	// no mark for stencil or projection shadows
		return qtrue;

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// hack / FPE - bogus planes?
	//assert( vecdot( trace.plane.normal, trace.plane.normal ) != 0.0f )

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	impactmark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		      cent->pe.legs.yaw, alpha, alpha, alpha, 1, qfalse, 24, qtrue);

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void
CG_PlayerSplash(centity_t *cent)
{
	vec3_t start, end;
	trace_t trace;
	int contents;
	polyVert_t verts[4];

	if(!cg_shadows.integer)
		return;

	veccpy(cent->lerporigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = pointcontents(end, 0);
	if(!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
		return;

	veccpy(cent->lerporigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = pointcontents(start, 0);
	if(contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		return;

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, nil, nil, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if(trace.fraction == 1.0)
		return;

	// create a mark polygon
	veccpy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	veccpy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	veccpy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	veccpy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

/*
===============
addrefentitywithpowerups

Adds a piece with modifications or duplications for powerups
Also called by domissile for quad rockets, but nobody can tell...
===============
*/
void
addrefentitywithpowerups(refEntity_t *ent, entityState_t *state, int team)
{
	if(state->powerups & (1 << PW_INVIS)){
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(ent);
	}else{
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
		        if (team == TEAM_BLUE)
		                ent->customShader = cgs.media.blueKamikazeShader;
		        else
		                ent->customShader = cgs.media.redKamikazeShader;
		        trap_R_AddRefEntityToScene( ent );
		}
		else {*/
		trap_R_AddRefEntityToScene(ent);
		//}

		if(state->powerups & (1 << PW_QUAD)){
			if(team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene(ent);
		}
		if(state->powerups & (1 << PW_REGEN))
			if(((cg.time / 100) % 10) == 1){
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene(ent);
			}
		if(state->powerups & (1 << PW_BATTLESUIT)){
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene(ent);
		}
	}
}

/*
=================
CG_LightVerts
=================
*/
int
CG_LightVerts(vec3_t normal, int numVerts, polyVert_t *verts)
{
	int i, j;
	float incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;

	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);

	for(i = 0; i < numVerts; i++){
		incoming = vecdot(normal, lightDir);
		if(incoming <= 0){
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = (ambientLight[0] + incoming * directedLight[0]);
		if(j > 255)
			j = 255;
		verts[i].modulate[0] = j;

		j = (ambientLight[1] + incoming * directedLight[1]);
		if(j > 255)
			j = 255;
		verts[i].modulate[1] = j;

		j = (ambientLight[2] + incoming * directedLight[2]);
		if(j > 255)
			j = 255;
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
doplayer
===============
*/
void
doplayer(centity_t *cent)
{
	clientInfo_t *ci;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t head;
	int clientNum;
	int renderfx;
	qboolean shadow;
	float shadowPlane;
#ifdef MISSIONPACK
	refEntity_t skull;
	refEntity_t powerup;
	int t;
	float c;
	float angle;
	vec3_t dir, angles;
#endif

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currstate.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS)
		cgerrorf("Bad clientNum on player entity");
	ci = &cgs.clientinfo[clientNum];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if(!ci->infovalid)
		return;

	// get the player model information
	renderfx = 0;
	if(cent->currstate.number == cg.snap->ps.clientNum){
		if(!cg.thirdperson)
			renderfx = RF_THIRD_PERSON;	// only draw in mirrors
		else  if(cg_cameraMode.integer)
			return;

	}

	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	// get the rotation information
	CG_PlayerAngles(cent, legs.axis, torso.axis, head.axis);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp,
			   &torso.oldframe, &torso.frame, &torso.backlerp);

	// add the talk baloon or disconnect icon
	CG_PlayerSprites(cent);

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if(cg_shadows.integer == 3 && shadow)
		renderfx |= RF_SHADOW_PLANE;
	renderfx |= RF_LIGHTING_ORIGIN;	// use the same origin for all
#ifdef MISSIONPACK
	if(cgs.gametype == GT_HARVESTER)
		CG_PlayerTokens(cent, renderfx);

#endif
	//
	// add the legs
	//
	legs.hModel = ci->legsmodel;
	legs.customSkin = ci->legsskin;

	veccpy(cent->lerporigin, legs.origin);

	veccpy(cent->lerporigin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	veccpy(legs.origin, legs.oldorigin);	// don't positionally lerp at all

	addrefentitywithpowerups(&legs, &cent->currstate, ci->team);

	// if the model failed, allow the default nullmodel to be displayed
	if(!legs.hModel)
		return;

	//
	// add the torso
	//
	torso.hModel = ci->torsomodel;
	if(!torso.hModel)
		return;

	torso.customSkin = ci->torsoskin;

	veccpy(cent->lerporigin, torso.lightingOrigin);

	rotentontag(&torso, &legs, ci->legsmodel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	addrefentitywithpowerups(&torso, &cent->currstate, ci->team);

#ifdef MISSIONPACK
	if(cent->currstate.eFlags & EF_KAMIKAZE){
		memset(&skull, 0, sizeof(skull));

		veccpy(cent->lerporigin, skull.lightingOrigin);
		skull.shadowPlane = shadowPlane;
		skull.renderfx = renderfx;

		if(cent->currstate.eFlags & EF_DEAD){
			// one skull bobbing above the dead body
			angle = ((cg.time / 7) & 255) * (M_PI * 2) / 255;
			if(angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[2] = 15 + sin(angle) * 8;
			vecadd(torso.origin, dir, skull.origin);

			dir[2] = 0;
			veccpy(dir, skull.axis[1]);
			vecnorm(skull.axis[1]);
			vecset(skull.axis[2], 0, 0, 1);
			veccross(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene(&skull);
		}else{
			// three skulls spinning around the player
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 20;
			vecadd(torso.origin, dir, skull.origin);

			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if(angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis(angles, skull.axis);

			/*
			dir[2] = 0;
			vecinv(dir);
			veccpy(dir, skull.axis[1]);
			vecnorm(skull.axis[1]);
			vecset(skull.axis[2], 0, 0, 1);
			veccross(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene(&skull);
			// flip the trail because this skull is spinning in the other direction
			vecinv(skull.axis[1]);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene(&skull);

			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255 + M_PI;
			if(angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = cos(angle) * 20;
			vecadd(torso.origin, dir, skull.origin);

			angles[0] = cos(angle - 0.5 * M_PI) * 30;
			angles[1] = 360 - (angle * 180 / M_PI);
			if(angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis(angles, skull.axis);

			/*
			dir[2] = 0;
			veccpy(dir, skull.axis[1]);
			vecnorm(skull.axis[1]);
			vecset(skull.axis[2], 0, 0, 1);
			veccross(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene(&skull);

			angle = ((cg.time / 3) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
			if(angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = 0;
			vecadd(torso.origin, dir, skull.origin);

			veccpy(dir, skull.axis[1]);
			vecnorm(skull.axis[1]);
			vecset(skull.axis[2], 0, 0, 1);
			veccross(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene(&skull);
		}
	}

	if(cent->currstate.powerups & (1 << PW_GUARD)){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene(&powerup);
	}
	if(cent->currstate.powerups & (1 << PW_SCOUT)){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.scoutPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene(&powerup);
	}
	if(cent->currstate.powerups & (1 << PW_DOUBLER)){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.doublerPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene(&powerup);
	}
	if(cent->currstate.powerups & (1 << PW_AMMOREGEN)){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.ammoRegenPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene(&powerup);
	}
	if(cent->currstate.powerups & (1 << PW_INVULNERABILITY)){
		if(!ci->invulnerabilityStartTime)
			ci->invulnerabilityStartTime = cg.time;
		ci->invulnerabilityStopTime = cg.time;
	}else
		ci->invulnerabilityStartTime = 0;
	if((cent->currstate.powerups & (1 << PW_INVULNERABILITY)) ||
	   cg.time - ci->invulnerabilityStopTime < 250){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.invulnerabilityPowerupModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		veccpy(cent->lerporigin, powerup.origin);

		if(cg.time - ci->invulnerabilityStartTime < 250)
			c = (float)(cg.time - ci->invulnerabilityStartTime) / 250;
		else if(cg.time - ci->invulnerabilityStopTime < 250)
			c = (float)(250 - (cg.time - ci->invulnerabilityStopTime)) / 250;
		else
			c = 1;
		vecset(powerup.axis[0], c, 0, 0);
		vecset(powerup.axis[1], 0, c, 0);
		vecset(powerup.axis[2], 0, 0, c);
		trap_R_AddRefEntityToScene(&powerup);
	}

	t = cg.time - ci->medkitUsageTime;
	if(ci->medkitUsageTime && t < 500){
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		vecclear(angles);
		AnglesToAxis(angles, powerup.axis);
		veccpy(cent->lerporigin, powerup.origin);
		powerup.origin[2] += -24 + (float)t * 80 / 500;
		if(t > 400){
			c = (float)(t - 1000) * 0xff / 100;
			powerup.shaderRGBA[0] = 0xff - c;
			powerup.shaderRGBA[1] = 0xff - c;
			powerup.shaderRGBA[2] = 0xff - c;
			powerup.shaderRGBA[3] = 0xff - c;
		}else{
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		trap_R_AddRefEntityToScene(&powerup);
	}
#endif	// MISSIONPACK

	//
	// add the head
	//
	head.hModel = ci->headmodel;
	if(!head.hModel)
		return;
	head.customSkin = ci->headskin;

	veccpy(cent->lerporigin, head.lightingOrigin);

	rotentontag(&head, &torso, ci->torsomodel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	addrefentitywithpowerups(&head, &cent->currstate, ci->team);

#ifdef MISSIONPACK
	CG_BreathPuffs(cent, &head);

	CG_DustTrail(cent);
#endif

	//
	// add the gun / barrel / flash
	//
	addplayerweap(&torso, nil, cent, ci->team);

	// add powerups floating behind the player
	CG_PlayerPowerups(cent, &torso);
}

//=====================================================================

/*
===============
resetplayerent

A player just came into view or teleported, so reset all animation info
===============
*/
void
resetplayerent(centity_t *cent)
{
	cent->errtime = -99999;	// guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame(&cgs.clientinfo[cent->currstate.clientNum], &cent->pe.legs, cent->currstate.legsAnim);
	CG_ClearLerpFrame(&cgs.clientinfo[cent->currstate.clientNum], &cent->pe.torso, cent->currstate.torsoAnim);

	evaltrajectory(&cent->currstate.pos, cg.time, cent->lerporigin);
	evaltrajectory(&cent->currstate.apos, cg.time, cent->lerpangles);

	veccpy(cent->lerporigin, cent->raworigin);
	veccpy(cent->lerpangles, cent->rawangles);

	memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
	cent->pe.legs.yaw = cent->rawangles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitch = 0;
	cent->pe.legs.pitching = qfalse;

	memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
	cent->pe.torso.yaw = cent->rawangles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitch = cent->rawangles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if(cg_debugPosition.integer)
		cgprintf("%i ResetPlayerEntity yaw=%f\n", cent->currstate.number, cent->pe.torso.yaw);
}
