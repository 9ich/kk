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
 * A thruster file has entries like this, one per line:
 * //	tagname		movekeys	shader		color		dur	vel	startsize	endsize
 * 	tag_thrustXDL	( r !rd )	explode1	( 1 0 .9 .9 )	70	1000	4		8
 *
 * tagname gives the name of the MD3 tag that this line corresponds to. This
 * can be any text (except spaces) as long as the tag is present in the
 * accompanying MD3.
 * 
 * movekeys controls which combinations of movement keys fire the thruster.
 * The combinations are inclusive, so "fr" will make the thruster fire only if
 * both "f" and "r" are held.
 * Prefixing any combination of movement keys with "!"  instead makes the
 * thruster stop firing when that combination of keys is held.
 * 
 * dur controls the duration of the thruster's particle effect, and vel its
 * velocity.
 */
static void
parsethrustersfile(const char *filename, clientInfo_t *ci)
{
	char buf[20000], *tok, *bufp;
	fileHandle_t f;
	thrusterparms_t *tp;
	int i, j, len;

	ci->nthrusttab = 0;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
		return;
	if(len >= sizeof(buf) - 1){
		cgprintf("%s: file too long\n", filename);
		trap_FS_FCloseFile(f);
		return;
	}
	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	bufp = buf;

	for(i = 0; i < ARRAY_LEN(ci->thrusttab); i++){
		tp = &ci->thrusttab[i];
		memset(tp, 0, sizeof *tp);

		//
		// tagname
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0')
			break;
		Q_strncpyz(tp->tagname, tok, sizeof tp->tagname);

		//
		// movekeys
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		if(*tok != '('){
			cgprintf(S_COLOR_RED "%s: expected '(', got '%s'\n", filename, tok);
			break;
		}
		// leave a sentinel: tp->dirs[len-1] == '\0'
		for(j = 0; j < ARRAY_LEN(tp->dirs)-1; j++){
			tok = COM_Parse(&bufp);
			if(*tok == '\0'){
				cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
				break;
			}
			if(*tok == ')'){	// end of movekeys
				break;
			}
			Q_strncpyz(tp->dirs[j], tok, sizeof tp->dirs[j]);
		}

		//
		// shader
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		Q_strncpyz(tp->shader, tok, sizeof tp->shader);

		//
		// color
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		if(*tok != '('){
			cgprintf(S_COLOR_RED "%s: expected '(', got '%s'\n", filename, tok);
			break;
		}
		for(j = 0; j < 4; j++){
			tok = COM_Parse(&bufp);
			if(*tok == '\0'){
				cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
				break;
			}
			tp->color[j] = atof(tok);
		}
		tok = COM_Parse(&bufp);		// closing rparen
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		if(*tok != ')'){
			cgprintf(S_COLOR_RED "%s: expected ')', got '%s'\n", filename, tok);
			break;
		}

		//
		// dur
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		tp->dur = atoi(tok);

		//
		// vel
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		tp->vel = atof(tok);

		//
		// startsize
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		tp->startsize = atof(tok);

		//
		// endsize
		//
		tok = COM_Parse(&bufp);
		if(*tok == '\0'){
			cgprintf(S_COLOR_RED "%s: unexpected end of file\n", filename);
			break;
		}
		tp->endsize = atof(tok);

		ci->nthrusttab++;
	}
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
	char *team;
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

	if(cg_brightskins.integer){
		skinname = "bright";
		// "models/players/griever/hull_bright.skin"
		Com_sprintf(filename, length, "models/players/%s/%s_%s.%s", modelname, base, skinname, ext);
		if(CG_FileExists(filename))
			return qtrue;
	}

	for(i = 0; i < 2; i++){
		if(i == 0 && teamName && *teamName)
			// "models/players/james/stroggs/lower_lily_red.skin"
			Com_sprintf(filename, length, "models/players/%s/%s%s_%s_%s.%s", modelname, teamName, base, skinname, team, ext);
		else
			// "models/players/james/lower_lily_red.skin"
			Com_sprintf(filename, length, "models/players/%s/%s_%s_%s.%s", modelname, base, skinname, team, ext);
		if(CG_FileExists(filename))
			return qtrue;
		if(cgs.gametype >= GT_TEAM){
			if(i == 0 && teamName && *teamName)
				// "models/players/james/stroggs/lower_red.skin"
				Com_sprintf(filename, length, "models/players/%s/%s%s_%s.%s", modelname, teamName, base, team, ext);
			else
				//"models/players/james/lower_red.skin"
				Com_sprintf(filename, length, "models/players/%s/%s_%s.%s", modelname, base, team, ext);
		}else{
			if(i == 0 && teamName && *teamName)
				// "models/players/james/stroggs/lower_lily.skin"
				Com_sprintf(filename, length, "models/players/%s/%s%s_%s.%s", modelname, teamName, base, skinname, ext);
			else
				// "models/players/james/lower_lily.skin"
				Com_sprintf(filename, length, "models/players/%s/%s_%s.%s", modelname, base, skinname, ext);
		}
		if(CG_FileExists(filename))
			return qtrue;
		if(!teamName || !*teamName)
			break;
	}

	return qfalse;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean
CG_RegisterClientSkin(clientInfo_t *ci, const char *teamName, const char *modelname, const char *skinname)
{
	char filename[MAX_QPATH];

	if(CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelname, skinname, "hull", "skin"))
		ci->torsoskin = trap_R_RegisterSkin(filename);
	if(!ci->torsoskin)
		Com_Printf("Torso skin load failure: %s\n", filename);

	// if any skins failed to load
	if(!ci->torsoskin)
		return qfalse;
	return qtrue;
}

/*
=========================
CG_RegisterClientModelname
==========================
*/
static qboolean
CG_RegisterClientModelname(clientInfo_t *ci, const char *modelname, const char *skinname, const char *teamName)
{
	char filename[MAX_QPATH];
	char newTeamName[MAX_QPATH];

	Com_sprintf(filename, sizeof(filename), "models/players/%s/hull", modelname);
	ci->torsomodel = trap_R_RegisterModel(filename);
	if(!ci->torsomodel){
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	// if any skins failed to load, return failure
	if(!CG_RegisterClientSkin(ci, teamName, modelname, skinname)){
		if(teamName && *teamName){
			Com_Printf("Failed to load skin file: %s : %s : %s\n", teamName, modelname, skinname);
			if(ci->team == TEAM_BLUE)
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			else
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			if(!CG_RegisterClientSkin(ci, newTeamName, modelname, skinname)){
				Com_Printf("Failed to load skin file: %s : %s : %s\n", newTeamName, modelname, skinname);
				return qfalse;
			}
		}else{
			Com_Printf("Failed to load skin file: %s : %s\n", modelname, skinname);
			return qfalse;
		}
	}

	// load the thruster params
	Com_sprintf(filename, sizeof(filename), "models/players/%s/thrusters.cfg", modelname);
	parsethrustersfile(filename, ci);

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelname);
	if(!CG_ParseAnimationFile(filename, ci->animations)){
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

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
	if(!CG_RegisterClientModelname(ci, ci->modelname, ci->skinname, teamname)){
		if(cg_buildScript.integer)
			cgerrorf("CG_RegisterClientModelname( %s, %s, %s ) failed", ci->modelname, ci->skinname, teamname);

		// fall back to default team name
		if(cgs.gametype >= GT_TEAM){
			// keep skin name
			if(ci->team == TEAM_BLUE)
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname));
			else
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname));
			if(!CG_RegisterClientModelname(ci, DEFAULT_TEAM_MODEL, ci->skinname, teamname))
				cgerrorf("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinname);
		}else  if(!CG_RegisterClientModelname(ci, DEFAULT_MODEL, "default", teamname))
			cgerrorf("DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL);

		modelloaded = qfalse;
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
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->torsomodel = from->torsomodel;
	to->torsoskin = from->torsoskin;
	to->modelicon = from->modelicon;

	memcpy(to->animations, from->animations, sizeof(to->animations));
	memcpy(to->thrusttab, from->thrusttab, sizeof(to->thrusttab));
	to->nthrusttab = from->nthrusttab;
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
CG_PlayerAnimation
===============
*/
static void
CG_PlayerAnimation(centity_t *cent, int *torsoOld, int *torso, float *torsoBackLerp)
{
	clientInfo_t *ci;
	int clientNum;
	float speedScale;

	clientNum = cent->currstate.clientNum;

	if(cg_noPlayerAnims.integer){
		*torso = 0;
		return;
	}

	if(cent->currstate.powerups & (1 << PW_HASTE))
		speedScale = 1.5;
	else
		speedScale = 1;

	ci = &cgs.clientinfo[clientNum];

	CG_RunLerpFrame(ci->animations, &cent->pe.torso, cent->currstate.torsoAnim, speedScale);

	*torsoOld = cent->pe.torso.oldframe;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

static void
CG_WeaponAnimation(centity_t *cent, weaponInfo_t *weapinfo, int slot, int *oldframe, int *frame, int *backlerp)
{
	float speedScale;

	if(cent->currstate.powerups & (1 << PW_HASTE))
		speedScale = 1.5;
	else
		speedScale = 1;

	CG_RunLerpFrame(cgs.media.itemanims[finditemforweapon(cent->currstate.weapon[slot]) - bg_itemlist],
	   &cent->weaplerpframe[slot], cent->currstate.weapAnim[slot], 1.0f);
	*oldframe = cent->weaplerpframe[slot].oldframe;
	*frame = cent->weaplerpframe[slot].frame;
	*backlerp = cent->weaplerpframe[slot].backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
Ship always looks at cent->lerpangles.
*/
static void
CG_PlayerAngles(centity_t *cent, vec3_t ship[3])
{
	vec3_t shipangles;

	veccpy(cent->lerpangles, shipangles);

	AnglesToAxis(shipangles, ship);
}

/*
 * Returns position of a player's centity_t.
 * Correctly deals with errordecay if cent is ourself.
 */
float*
playerpos(centity_t *cent, vec3_t out)
{
	int t;
	float f;

	if(cent->currstate.number == cg.pps.clientNum){
		veccpy(cg.pps.origin, out);

		// add errordecay
		if(cg_errorDecay.value > 0){
			t = cg.time - cg.predictederrtime;
			f = (cg_errorDecay.value - t) / cg_errorDecay.value;
			if(f > 0 && f < 1)
				vecmad(out, f, cg.predictederr, out);
			else
				cg.predictederrtime = 0;
		}
	}else{
		veccpy(cent->lerporigin, out);
	}
	return out;
}

/*
 * Returns velocity of a player's centity_t.
 * Bypasses prediction if cent is ourself.
 */
float*
playervel(centity_t *cent, vec3_t out)
{
	if(cent->currstate.number == cg.pps.clientNum)
		veccpy(cg.pps.velocity, out);
	else
		evaltrajectorydelta(&cent->currstate.pos, cg.time, out);
	return out;
}

float*
playerangles(centity_t *cent, vec3_t out)
{
	if(cent->currstate.number == cg.pps.clientNum)
		veccpy(cg.pps.viewangles, out);
	else
		veccpy(cent->lerpangles, out);
	return out;
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

/*
===============
CG_PlayerFlag
===============
*/
static void
CG_PlayerFlag(centity_t *cent, refEntity_t *torso, qhandle_t flagmodel)
{
	clientInfo_t *ci;
	refEntity_t flag;

	// show the flag model
	memset(&flag, 0, sizeof(flag));
	veccpy(torso->lightingOrigin, flag.lightingOrigin);
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[cent->currstate.clientNum];
	CG_RunLerpFrame(ci->animations, &cent->pe.flag, 0, 1);
	flag.hModel = flagmodel;
	flag.oldframe = cent->pe.flag.oldframe;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AxisClear(flag.axis);
	rotentontag(&flag, torso, torso->hModel, "tag_flag");

	trap_R_AddRefEntityToScene(&flag);
}

static void
calcthrusterlight(centity_t *cent, vec4_t color)
{
	if(cent->flicker.interval == 0)
		cent->flicker.interval = (1000/15.0f);

	if(cent->currstate.forwardmove == 0 &&
	   cent->currstate.rightmove == 0 &&
	   cent->currstate.upmove == 0){
		// initial fade-in
		VectorSet4(cent->flicker.a, 0, 0, 0, 1.0f);
		VectorSet4(cent->flicker.b, 0.5f*(0.9f + 0.1*crandom()),
		   0.5f*(0.4f + 0.05f*crandom()), 0.005f*crandom(), 0.0f);
		cent->flicker.time = cg.time;
	}else if(cent->flicker.time + cent->flicker.interval < cg.time){
		// begin transition to next colour
		Vector4Copy(cent->flicker.b, cent->flicker.a);
		VectorSet4(cent->flicker.b, 0.5f*(0.9f + 0.1*crandom()),
		   0.5f*(0.4f + 0.05f*crandom()), 0.005f*crandom(),
		   1.0f+0.1f*crandom());
		cent->flicker.time = cg.time;
	}

	lerpcolour(cent->flicker.a, cent->flicker.b, color,
	   (float)(cg.time - cent->flicker.time)/cent->flicker.interval);
}

static void
CG_PlayerThrusters(centity_t *cent, refEntity_t *ship)
{
	clientInfo_t *ci;
	qboolean f, b, u, d, l, r;
	thrusterparms_t *tp;
	sfxHandle_t thrustsound, thrustbacksound, idlesound;
	vec3_t forward, right, up;
	vec3_t pos, v;
	vec4_t clr;
	refEntity_t re;
	int i, j;

	ci = &cgs.clientinfo[cent->currstate.clientNum];
	f = cent->currstate.forwardmove > 0;
	b = cent->currstate.forwardmove < 0;
	u = cent->currstate.upmove > 0;
	d = cent->currstate.upmove < 0;
	l = cent->currstate.rightmove < 0;
	r = cent->currstate.rightmove > 0;
	playerpos(cent, pos);


	memset(&re, 0, sizeof re);
	
	re.hModel = cgs.media.thrustFlameModel;
	re.nonNormalizedAxes = qtrue;
	CG_RunLerpFrame(cgs.media.thrustFlameAnims, &cent->lf, ANIM_IDLE, 1.0f);
	re.frame = cent->lf.frame;
	re.oldframe = cent->lf.oldframe;
	re.backlerp = cent->lf.backlerp;

	//
	// thruster sounds
	//
	if(cent->currstate.number == cg.snap->ps.clientNum){
		thrustsound = cgs.media.thrustSound;
		thrustbacksound = cgs.media.thrustBackSound;
		idlesound = cgs.media.thrustIdleSound;
	}else{
		thrustsound = cgs.media.thrustOtherSound;
		thrustbacksound = cgs.media.thrustOtherBackSound;
		idlesound = cgs.media.thrustOtherIdleSound;
	}


	if((cent->currstate.number != cg.snap->ps.clientNum && cg_enemyThrustSounds.integer) ||
	   (cent->currstate.number == cg.snap->ps.clientNum && cg_ownThrustSounds.integer)){
		if(b || (l || r) || (u || d)){
			trap_S_AddLoopingSound(cent->currstate.number, pos,
			   vec3_origin, thrustbacksound);
		}else if(f){
			trap_S_AddLoopingSound(cent->currstate.number, pos,
			   vec3_origin, thrustsound);
		}else{
			trap_S_AddLoopingSound(cent->currstate.number, pos,
			   vec3_origin, idlesound);
		}
	}

	//
	// lights
	//
	calcthrusterlight(cent, clr);
	anglevecs(cent->lerpangles, forward, right, up);
	if(f){
		vecmad(pos, -40, forward, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}else if(b){
		vecmad(pos, 40, forward, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}
	if(r){
		vecmad(pos, -36, right, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}else if(l){
		vecmad(pos, 36, right, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}
	if(u){
		vecmad(pos, -36, up, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}else if(d){
		vecmad(pos, 36, up, v);
		trap_R_AddLightToScene(v, 200*clr[3], clr[0], clr[1], clr[2]);
	}

	//
	// thruster flame
	//
	if(!cg.thirdperson && cent->currstate.clientNum == cg.pps.clientNum)
		return;

	for(i = 0; i < ci->nthrusttab; i++){
		qboolean enable;

		enable = qfalse;
		tp = &ci->thrusttab[i];
		for(j = 0; tp->dirs[j][0] != '\0'; j++){
			int ok, total;
			qboolean invert;
			char *p;

			ok = total = 0;
			invert = qfalse;
			for(p = tp->dirs[j]; *p != '\0'; p++){
				switch(*p){
				case '!':
					invert = qtrue;
					break;
				case 'f':
					total++;
					if(f) ok++;
					break;
				case 'b':
					total++;
					if(b) ok++;
					break;
				case 'u':
					total++;
					if(u) ok++;
					break;
				case 'd':
					total++;
					if(d) ok++;
					break;
				case 'l':
					total++;
					if(l) ok++;
					break;
				case 'r':
					total++;
					if(r) ok++;
					break;
				default:
					break;
				}
			}
			if(ok == total)
				enable = invert? qfalse : qtrue;
		}
		if(enable){
			vec3_t vel, smokepos;
			vec3_t angles;

			// add flame
			vecclear(angles);
			AnglesToAxis(angles, re.axis);
			rotentontag(&re, ship, ship->hModel, tp->tagname);
			vecmul(re.axis[0], 2, vel);
			trap_R_AddRefEntityToScene(&re);

			// add smoke plume
			vecmad(re.origin, 10, re.axis[0], smokepos);
			if(cg.time - cent->trailtime >= 16){
				cent->trailtime = cg.time;
				CG_ParticleThrustPlume(cent, smokepos, vel);
			}
		}
	}
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

static void
addquadlight(centity_t *cent)
{
	vec4_t clr;

	if(cent->quadflicker.interval == 0)
		cent->quadflicker.interval = (1000/12.0f);

	if(cent->quadflicker.time + cent->quadflicker.interval < cg.time){
		// begin transition to next colour
		Vector4Copy(cent->quadflicker.b, cent->quadflicker.a);
		VectorSet4(cent->quadflicker.b, 0.5f*(0.2f + 0.01f*crandom()),
		   0.5f*(0.2f + 0.01f*crandom()), 1.0f, 1.0f+0.2f*crandom());
		cent->quadflicker.time = cg.time;
	}

	lerpcolour(cent->quadflicker.a, cent->quadflicker.b, clr,
	   (float)(cg.time - cent->quadflicker.time)/cent->quadflicker.interval);
	trap_R_AddLightToScene(cent->lerporigin, 300*clr[3], clr[0], clr[1], clr[2]);
}

/*
===============
CG_PlayerPowerups
===============
*/
static void
CG_PlayerPowerups(centity_t *cent, refEntity_t *torso)
{
	int powerups;

	powerups = cent->currstate.powerups;
	if(!powerups)
		return;

	// quad gives a dlight
	if(powerups & (1 << PW_QUAD))
		addquadlight(cent);

	// flight plays a looped sound
	if(powerups & (1 << PW_FLIGHT))
		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, cgs.media.flightSound);

	// redflag
	if(powerups & (1 << PW_REDFLAG)){
		CG_PlayerFlag(cent, torso, cgs.media.redFlagModel);
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f);
	}

	// blueflag
	if(powerups & (1 << PW_BLUEFLAG)){
		CG_PlayerFlag(cent, torso, cgs.media.blueFlagModel);
		trap_R_AddLightToScene(cent->lerporigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0);
	}

	// neutralflag
	if(powerups & (1 << PW_NEUTRALFLAG)){
		CG_PlayerFlag(cent, torso, cgs.media.neutralFlagModel);
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
	// offset upward relative to viewer
	vecmad(ent.origin, 38, cg.refdef.viewaxis[2], ent.origin);
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

/*
	for(i = cg_nawardlist-1; i >= 0; i--){
		char *playername;
		char s[MAX_STRING_CHARS];
		const char *info;
		sfxHandle_t sfx;

		sfx = 0;

		if(!cent->currvalid)
			break;	// if awardflags are lingering, don't keep queueing awards
		if(!(cent->currstate.awardflags & (1<<cg_awardlist[i].award)))
			continue;
		if(cg_awardlist[i].sfx != nil && *cg_awardlist[i].sfx != 0)
			sfx = trap_S_RegisterSound(cg_awardlist[i].sfx, qtrue);
		info = getconfigstr(CS_PLAYERS + cent->currstate.number);
		playername = Info_ValueForKey(info, "n");
		Com_sprintf(s, sizeof s, "%s %s\n", playername, cg_awardlist[i].msg);

		pushreward(sfx, 0, s, 1);
		break;
	}
*/

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
		      0, alpha, alpha, alpha, 1, qfalse, 24, qtrue);

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
	refEntity_t torso;
	int clientNum;
	int renderfx;
	qboolean shadow;
	float shadowPlane;
	refEntity_t powerup;
	int t;
	float c;
	vec3_t angles;
	vec3_t pos;

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

	memset(&torso, 0, sizeof(torso));

	// get the rotation information
	CG_PlayerAngles(cent, torso.axis);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &torso.oldframe, &torso.frame, &torso.backlerp);

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
	// add the torso
	torso.hModel = ci->torsomodel;
	torso.customSkin = ci->torsoskin;
	if(cg_brightskins.integer){
		if(cgs.gametype >= GT_TEAM){
			if(ci->team == cgs.clientinfo[cg.pps.clientNum].team)
				Byte4Copy(cg.teamRGBA, torso.shaderRGBA);
			else
				Byte4Copy(cg.enemyRGBA, torso.shaderRGBA);
		}else
			Byte4Copy(cg.enemyRGBA, torso.shaderRGBA);
	}

	playerpos(cent, pos);
	
	veccpy(pos, torso.origin);
	veccpy(pos, torso.lightingOrigin);

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	addrefentitywithpowerups(&torso, &cent->currstate, ci->team);

#ifdef MISSIONPACK

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
		veccpy(pos, powerup.origin);

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

	// add the gun / barrel / flash
	addplayerweap(&torso, nil, cent, ci->team, 0);
	addplayerweap(&torso, nil, cent, ci->team, 1);

	// add powerups floating behind the player
	CG_PlayerPowerups(cent, &torso);

	CG_PlayerThrusters(cent, &torso);
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

	CG_ClearLerpFrame(cgs.clientinfo[cent->currstate.clientNum].animations, &cent->pe.torso, cent->currstate.torsoAnim);

	evaltrajectory(&cent->currstate.pos, cg.time, cent->lerporigin);
	evaltrajectory(&cent->currstate.apos, cg.time, cent->lerpangles);

	veccpy(cent->lerporigin, cent->raworigin);
	veccpy(cent->lerpangles, cent->rawangles);

	memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
	cent->pe.torso.yaw = cent->rawangles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitch = cent->rawangles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if(cg_debugPosition.integer)
		cgprintf("%i ResetPlayerEntity yaw=%f\n", cent->currstate.number, cent->pe.torso.yaw);
}
