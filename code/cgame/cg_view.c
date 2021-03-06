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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

enum { MAX_TESTLIGHTS = 32 };

static struct 
{
	vec3_t	pt;
	float	intensity;
	float	r, g, b;
} testlights[MAX_TESTLIGHTS];
static int ntestlights;
static int testlightsi;

/*
Creates an entity in front of the current position, which
can then be moved around
*/
void
CG_TestModel_f(void)
{
	vec3_t angles;
	int i;

	i = cg.testmodeli;

	memset(&cg.testmodelent[i], 0, sizeof(cg.testmodelent[i]));
	if(trap_Argc() < 2)
		return;

	Q_strncpyz(cg.testmodelname[i], cgargv(1), MAX_QPATH);
	cg.testmodelent[i].hModel = trap_R_RegisterModel(cg.testmodelname[i]);

	if(trap_Argc() == 3){
		cg.testmodelent[i].backlerp = atof(cgargv(2));
		cg.testmodelent[i].frame = 1;
		cg.testmodelent[i].oldframe = 0;
	}
	if(!cg.testmodelent[i].hModel){
		cgprintf("Can't register model\n");
		return;
	}

	vecmad(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testmodelent[i].origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefviewangles[1];
	angles[ROLL] = 0;

	angles2axis(angles, cg.testmodelent[i].axis);
	cg.testgun = qfalse;

	cg.testmodeli = (cg.testmodeli + 1) % ARRAY_LEN(cg.testmodelent);
	cg.ntestmodels = MIN(cg.ntestmodels + 1, ARRAY_LEN(cg.testmodelent));
}

void
CG_TestExplosion_f(void)
{
	memset(&cg.testparticles, 0, sizeof cg.testparticles);
	if(trap_Argc() != 6){
		cgprintf("usage: testparticleexplosion [shader] [speed] [dur] [startsize] [endsize]\n");
		return;
	}

	cg.testparticles.typ = "explosion";
	Q_strncpyz(cg.testparticles.shader, cgargv(1), sizeof cg.testparticles.shader);
	vecmad(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testparticles.pos);
	vecmul(cg.refdef.viewaxis[1], atof(cgargv(2)), cg.testparticles.vel);
	cg.testparticles.dur = atof(cgargv(3));
	cg.testparticles.startsize = atof(cgargv(4));
	cg.testparticles.endsize = atof(cgargv(5));
}

/*
Replaces the current view weapon with the given model
*/
void
CG_TestGun_f(void)
{
	CG_TestModel_f();
	cg.testgun = qtrue;
	cg.testmodelent[0].renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}

void
CG_TestModelNextFrame_f(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(cg.testmodelent); i++){
		if(cg.testmodelent[i].hModel != 0){
			cg.testmodelent[i].frame++;
			cgprintf("frame %i\n", cg.testmodelent[i].frame);
		}
	}
}

void
CG_TestModelPrevFrame_f(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(cg.testmodelent); i++){
		if(cg.testmodelent[i].hModel != 0){
			cg.testmodelent[i].frame--;
			if(cg.testmodelent[i].frame < 0)
				cg.testmodelent[i].frame = 0;
			cgprintf("frame %i\n", cg.testmodelent[i].frame);
		}
	}
}

void
CG_TestModelNextSkin_f(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(cg.testmodelent); i++){
		if(cg.testmodelent[i].hModel != 0){
			cg.testmodelent[i].skinNum++;
			cgprintf("skin %i\n", cg.testmodelent[i].skinNum);
		}
	}
}

void
CG_TestModelPrevSkin_f(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(cg.testmodelent); i++){
		if(cg.testmodelent[i].hModel != 0){
			cg.testmodelent[i].skinNum--;
			if(cg.testmodelent[i].skinNum < 0)
				cg.testmodelent[i].skinNum = 0;
			cgprintf("skin %i\n", cg.testmodelent[i].skinNum);
		}
	}
}

void
CG_TestLight_f(void)
{
	char s[32];

	if(trap_Argc() < 5){
		cgprintf("usage: testlight intensity r g b");
		return;
	}
	if(ntestlights < MAX_TESTLIGHTS)
		ntestlights++;
	veccpy(cg.snap->ps.origin, testlights[testlightsi].pt);
	trap_Argv(1, s, sizeof s);
	testlights[testlightsi].intensity = atof(s);
	trap_Argv(2, s, sizeof s);
	testlights[testlightsi].r = atof(s);
	trap_Argv(3, s, sizeof s);
	testlights[testlightsi].g = atof(s);
	trap_Argv(4, s, sizeof s);
	testlights[testlightsi].b = atof(s);
	testlightsi = (testlightsi+1) % MAX_TESTLIGHTS;
}

static void
addtestmodel(void)
{
	int i;
	int k;

	k = cg.testmodeli;

	for(k = 0; k < cg.ntestmodels; k++){
		// re-register the model, because the level may have changed
		cg.testmodelent[k].hModel = trap_R_RegisterModel(cg.testmodelname[k]);
		if(!cg.testmodelent[k].hModel){
			cgprintf("Can't register model\n");
		}
	}

	// if testing a gun, set the origin relative to the view origin
	if(cg.testgun){
		veccpy(cg.refdef.vieworg, cg.testmodelent[0].origin);
		veccpy(cg.refdef.viewaxis[0], cg.testmodelent[0].axis[0]);
		veccpy(cg.refdef.viewaxis[1], cg.testmodelent[0].axis[1]);
		veccpy(cg.refdef.viewaxis[2], cg.testmodelent[0].axis[2]);

		// allow the position to be adjusted
		for(i = 0; i<3; i++){
			cg.testmodelent[0].origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testmodelent[0].origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testmodelent[0].origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	for(k = 0; k < cg.ntestmodels; k++){
		refEntity_t re;

		if(cg.testmodelent[k].hModel == 0)
			continue;

		memcpy(&re, &cg.testmodelent[k], sizeof re);
		vecmul(re.axis[0], cg_testmodelscale.value, re.axis[0]);
		vecmul(re.axis[1], cg_testmodelscale.value, re.axis[1]);
		vecmul(re.axis[2], cg_testmodelscale.value, re.axis[2]);
		re.nonNormalizedAxes = qtrue;

		trap_R_AddRefEntityToScene(&re);
	}
}

static void
addtestparticles(void)
{
	testparticles_t *p;

	if(cg.testparticles.typ == nil)
		return;

	p = &cg.testparticles;
	if(strcmp(p->typ, "explosion") == 0){
		particleexplosion(p->shader, p->pos, p->vel, p->dur,
		   p->startsize, p->endsize);
	}
}

//============================================================================

/*
Sets the coordinates of the rendered window
*/
static void
calcvrect(void)
{
	int size;

	// the intermission should allways be full screen
	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		size = 100;
	else{
		// bound normal viewsize
		if(cg_viewsize.integer < 30){
			trap_Cvar_Set("cg_viewsize", "30");
			size = 30;
		}else if(cg_viewsize.integer > 100){
			trap_Cvar_Set("cg_viewsize", "100");
			size = 100;
		}else
			size = cg_viewsize.integer;

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================

#define FOCUS_DISTANCE 512
static void
offset3rdpersonview(void)
{
	vec3_t forward, right, up;
	vec3_t view;
	vec3_t focusAngles;
	trace_t trace;
	static vec3_t mins = {-4, -4, -4};
	static vec3_t maxs = {4, 4, 4};
	vec3_t focusPoint;
	float focusDist;

	veccpy(cg.refdefviewangles, focusAngles);

	anglevecs(focusAngles, forward, nil, nil);

	vecmad(cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint);

	veccpy(cg.refdef.vieworg, view);

	anglevecs(cg.refdefviewangles, forward, right, up);

	vecmad(view, -cg_thirdPersonRange.value, forward, view);

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if(!cg_cameraMode.integer){
		cgtrace(&trace, cg.refdef.vieworg, mins, maxs, view, cg.pps.clientNum, MASK_SOLID);
		veccpy(trace.endpos, view);
	}

	veccpy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	vecsub(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1)
		focusDist = 1;	// should never happen
	if(cg_thirdPerson.integer == 1){
		cg.refdefviewangles[PITCH] = -180 / M_PI*atan2(focusPoint[2],
			focusDist);
		cg.refdefviewangles[YAW] -= cg_thirdPersonAngle.value;
	}
}

static void
offset1stpersonview(void)
{
	float *origin;
	float *angles;
	float ratio;
	float delta;
	float f;
	int timeDelta;

	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		return;

	origin = cg.refdef.vieworg;
	angles = cg.refdefviewangles;

	// add angles based on damage kick
	if(cg.dmgtime){
		ratio = cg.time - cg.dmgtime;
		if(ratio < DAMAGE_DEFLECT_TIME){
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.vdmgpitch;
			angles[ROLL] += ratio * cg.vdmgroll;
		}else{
			ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
			if(ratio > 0){
				angles[PITCH] += ratio * cg.vdmgpitch;
				angles[ROLL] += ratio * cg.vdmgroll;
			}
		}
	}

//===================================

	// add view height
	origin[2] += cg.pps.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.ducktime;
	if(timeDelta < DUCK_TIME)
		cg.refdef.vieworg[2] -= cg.duckchange
					* (DUCK_TIME - timeDelta) / DUCK_TIME;

	// add fall height
	delta = cg.time - cg.landtime;
	if(delta < LAND_DEFLECT_TIME){
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landchange * f;
	}else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME){
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef.vieworg[2] += cg.landchange * f;
	}

	// pivot the eye based on a neck length
#if 0
	{
#define NECK_LENGTH 8
		vec3_t forward, up;

		cg.refdef.vieworg[2] -= NECK_LENGTH;
		anglevecs(cg.refdefviewangles, forward, nil, up);
		vecmad(cg.refdef.vieworg, 3, forward, cg.refdef.vieworg);
		vecmad(cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg);
	}
#endif
}

//======================================================================

void
CG_ZoomDown_f(void)
{
	if(cg.zoomed)
		return;
	cg.zoomed = qtrue;
	cg.zoomtime = cg.time;
}

void
CG_ZoomUp_f(void)
{
	if(!cg.zoomed)
		return;
	cg.zoomed = qfalse;
	cg.zoomtime = cg.time;
}

/*
Fixed fov at intermissions, otherwise account for fov variable and zooms.
*/
#define WAVE_AMPLITUDE	1
#define WAVE_FREQUENCY	0.4

static int
calcfov(void)
{
	float x;
	float phase;
	float v;
	int contents;
	float fov_x, fov_y;
	float zoomfov;
	float f;
	int inwater;
	
	fov_x = cg_fov.value;
		if(fov_x < 1)
			fov_x = 1;
		else if(fov_x > 160)
			fov_x = 160;

	// account for zooms
	zoomfov = cg_zoomFov.value;
	if(zoomfov < 1)
		zoomfov = 1;
	else if(zoomfov > 160)
		zoomfov = 160;

	if(cg.zoomed){
		f = (cg.time - cg.zoomtime) / (float)ZOOM_TIME;
		if(f > 1.0)
			fov_x = zoomfov;
		else
			fov_x = fov_x + f * (zoomfov - fov_x);
	}else{
		f = (cg.time - cg.zoomtime) / (float)ZOOM_TIME;
		if(f <= 1.0)
			fov_x = zoomfov + f * (fov_x - zoomfov);
	}

	x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = pointcontents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}else
		inwater = qfalse;


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if(!cg.zoomed)
		cg.zoomsens = 1;
	else
		cg.zoomsens = cg.refdef.fov_y / 75.0;

	return inwater;
}

/*
Sets cg.refdef view values
*/
static int
calcviewvalues(void)
{
	playerState_t *ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	calcvrect();

	ps = &cg.pps;
/*
        if (cg.cameramode) {
                vec3_t origin, angles;
                if (trap_getCameraInfo(cg.time, &origin, &angles)) {
                        veccpy(origin, cg.refdef.vieworg);
                        angles[ROLL] = 0;
                        veccpy(angles, cg.refdefviewangles);
                        AnglesToAxis( cg.refdefviewangles, cg.refdef.viewaxis );
                        return CG_CalcFov();
                } else {
                        cg.cameramode = qfalse;
                }
        }
*/
	// intermission view
	if(ps->pm_type == PM_INTERMISSION){
		veccpy(ps->origin, cg.refdef.vieworg);
		veccpy(ps->viewangles, cg.refdefviewangles);
		angles2axis(cg.refdefviewangles, cg.refdef.viewaxis);
		return calcfov();
	}

	veccpy(ps->origin, cg.refdef.vieworg);
	if(cg_thirdPerson.integer != 2)
		veccpy(ps->viewangles, cg.refdefviewangles);
	if(cg_thirdPerson.integer == 3)
		vecset(ps->viewangles, 0, 0, 0);

	if(cg_cameraOrbit.integer)
		if(cg.time > cg.nextorbittime){
			cg.nextorbittime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	// add error decay
	if(cg_errorDecay.value > 0){
		int t;
		float f;

		t = cg.time - cg.predictederrtime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if(f > 0 && f < 1)
			vecmad(cg.refdef.vieworg, f, cg.predictederr, cg.refdef.vieworg);
		else
			cg.predictederrtime = 0;
	}

	if(cg.thirdperson)
		// back away from character
		offset3rdpersonview();
	else
		// offset for local bobbing and kicks
		offset1stpersonview();

	// position eye relative to origin
	angles2axis(cg.refdefviewangles, cg.refdef.viewaxis);

	if(cg.hyperspace)
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;

	// field of view
	return calcfov();
}

static void
poweruptimersounds(void)
{
	int i;
	int t;

	// powerup timers going away
	for(i = 0; i < MAX_POWERUPS; i++){
		t = cg.snap->ps.powerups[i];
		if(t <= cg.time)
			continue;
		if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
			continue;
		if((t - cg.time) / POWERUP_BLINK_TIME != (t - cg.oldtime) / POWERUP_BLINK_TIME)
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound);
	}
}

void
addbufferedsound(sfxHandle_t sfx)
{
	if(!sfx)
		return;
	cg.sndbuf[cg.sndbufin] = sfx;
	cg.sndbufin = (cg.sndbufin + 1) % MAX_SOUNDBUFFER;
	if(cg.sndbufin == cg.sndbufout)
		cg.sndbufout++;
}

static void
playbufferedsounds(void)
{
	if(cg.sndtime < cg.time)
		if(cg.sndbufout != cg.sndbufin && cg.sndbuf[cg.sndbufout]){
			trap_S_StartLocalSound(cg.sndbuf[cg.sndbufout], CHAN_ANNOUNCER);
			cg.sndbuf[cg.sndbufout] = 0;
			cg.sndbufout = (cg.sndbufout + 1) % MAX_SOUNDBUFFER;
			cg.sndtime = cg.time + 750;
		}
}

//=========================================================================

static void
drawlockon(void)
{
	static int prevacquire = ENTITYNUM_NONE;
	static int prevlock = ENTITYNUM_NONE;
	refEntity_t ent;
	centity_t *enemy;

	if(cg.scoreboardshown)
		return;

	if(cg.snap->ps.lockontarget == ENTITYNUM_NONE){
		prevacquire = ENTITYNUM_NONE;
		prevlock = ENTITYNUM_NONE;
		return;
	}

	enemy = &cg_entities[cg.snap->ps.lockontarget];
	
	memset(&ent, 0, sizeof(ent));
	veccpy(enemy->lerporigin, ent.origin);

	ent.reType = RT_SPRITE;
	ent.renderfx = 0;

	if(cg.snap->ps.lockontime - cg.snap->ps.lockonstarttime < HOMING_SCANWAIT){
		// acquiring
		float frac;

		ent.customShader = cgs.media.lockingOnShader;
		MAKERGBA(ent.shaderRGBA, 255, 255, 255, 200);
		// shrink indicator as lock-on is acquired
		frac = 1.0f - (float)((cg.time - cg.snap->ps.lockonstarttime)) / HOMING_SCANWAIT;
		frac = Com_Clamp(0.0f, 1.0f, frac);
		frac = frac*frac*frac;
		ent.rotation = frac*90;
		ent.radius = 64 + frac*120;
		trap_R_AddRefEntityToScene(&ent);

		if(cg.snap->ps.lockontarget != prevacquire)
			trap_S_StartLocalSound(cgs.media.lockingOnSound, CHAN_ANNOUNCER);
		prevacquire = cg.snap->ps.lockontarget;
		prevlock = ENTITYNUM_NONE;
	}else{
		// locked on
		ent.customShader = cgs.media.lockedOnShader;
		MAKERGBA(ent.shaderRGBA, 237, 115, 101, 200);
		ent.radius = 2*72;
		trap_R_AddRefEntityToScene(&ent);

		if(cg.snap->ps.lockontarget != prevlock)
			trap_S_StartLocalSound(cgs.media.lockedOnSound, CHAN_ANNOUNCER);
		prevlock = cg.snap->ps.lockontarget;
		prevacquire = ENTITYNUM_NONE;
	}
}

static void
drawtestlights(void)
{
	int i;

	for(i = 0; i < ntestlights; i++){
		trap_R_AddLightToScene(testlights[i].pt, testlights[i].intensity,
		   testlights[i].r, testlights[i].g, testlights[i].b);
	}
}

/*
Generates and draws a game scene and status information at the given time.
*/
void
drawframe(int serverTime, stereoFrame_t stereoview, qboolean demoplayback)
{
	int inwater;

	cg.time = serverTime;
	cg.demoplayback = demoplayback;

	// update cvars
	updatecvars();

	drawlibbeginframe(cg.time, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight);

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if(cg.infoscreentext[0] != 0){
		drawinfo();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextsnap
	processsnaps();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)){
		drawinfo();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue(cg.weapsel[0], cg.weapsel[1], cg.weapsel[2], cg.zoomsens);

	// this counter will be bumped for every valid scene we generate
	cg.clframe++;

	// update cg.pps
	predictplayerstate();

	// decide on third person view
	cg.thirdperson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	// build cg.refdef
	inwater = calcviewvalues();

	// build the render lists
	if(!cg.hyperspace){
		addpacketents();	// adter calcViewValues, so predicted player state is correct
		addmarks();
		addparticles();
		addlocalents();
	}
	addviewweap(&cg.pps);

	// add buffered sounds
	playbufferedsounds();


	// finish up the rest of the refdef
	if(cg.testmodelent[0].hModel)
		addtestmodel();
	addtestparticles();
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

	// warning sounds when powerup is wearing off
	poweruptimersounds();

	// update audio positions
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.pps.velocity, cg.refdef.viewaxis, inwater);

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if(stereoview != STEREO_RIGHT){
		cg.frametime = cg.time - cg.oldtime;
		if(cg.frametime < 0)
			cg.frametime = 0;
		cg.oldtime = cg.time;
		lagometerframeinfo();
	}
	if(cg_timescale.value != cg_timescaleFadeEnd.value){
		if(cg_timescale.value < cg_timescaleFadeEnd.value){
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}else{
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if(cg_timescaleFadeSpeed.value)
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
	}

	drawlockon();

	drawtestlights();

	// actually issue the rendering calls
	drawactive(stereoview);

	if(cg_stats.integer)
		cgprintf("cg.clframe:%i\n", cg.clframe);
	if(cg_debugPosition.integer)
		cgprintf("yaw=%f pitch=%f roll=%f\n", cg.refdefviewangles[YAW],
		   cg.refdefviewangles[PITCH], cg.refdefviewangles[ROLL]);

}
