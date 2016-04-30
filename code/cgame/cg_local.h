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
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
#include "../drawlib/d_public.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define POWERUP_BLINKS			5

#define POWERUP_BLINK_TIME		1000
#define FADE_TIME			200
#define PULSE_TIME			200
#define DAMAGE_DEFLECT_TIME		100
#define DAMAGE_RETURN_TIME		400
#define DAMAGE_TIME			500
#define LAND_DEFLECT_TIME		150
#define LAND_RETURN_TIME		300
#define STEP_TIME			200
#define DUCK_TIME			100
#define PAIN_TWITCH_TIME		200
#define WEAPON_SELECT_TIME		1400
#define ITEM_SCALEUP_TIME		1000
#define ZOOM_TIME			150
#define ITEM_BLOB_TIME			200
#define MUZZLE_FLASH_TIME		20
#define SINK_TIME			1000	// time for fragments to sink into ground before going away
#define ATTACKER_HEAD_TIME		10000
#define REWARD_TIME			3000

#define PULSE_SCALE			1.5	// amount to scale up the icons when activating

#define MAX_STEP_CHANGE			32

#define MAX_VERTS_ON_POLY		10
#define MAX_MARK_POLYS			256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define ICON_SIZE			48
#define CHAR_WIDTH			32
#define CHAR_HEIGHT			32
#define TEXT_ICON_SPACE			4

#define TEAMCHAT_WIDTH			80
#define TEAMCHAT_HEIGHT			8

// very large characters
#define GIANT_WIDTH			32
#define GIANT_HEIGHT			48

#define NUM_CROSSHAIRS			10

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define DEFAULT_MODEL			"griever"
#define DEFAULT_TEAM_MODEL		"griever"
#define DEFAULT_TEAM_HEAD		"griever"

#define DEFAULT_REDTEAM_NAME		"Stroggs"
#define DEFAULT_BLUETEAM_NAME		"Pagans"

typedef enum
{
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum
{
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animtime to frametime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct
{
	int		oldframe;
	int		oldframetime;	// time when ->oldframe was exactly on

	int		frame;
	int		frametime;	// time when ->frame will be exactly on

	float		backlerp;

	float		yaw;
	qboolean	yawing;
	float		pitch;
	qboolean	pitching;

	int		animnum;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int		animtime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct
{
	lerpFrame_t	legs, torso, flag;
	int		paintime;
	int		paindir;	// flip from 0 to 1
	int		lightningfiring;

	int		railfiretime;

	// machinegun spinning
	float		barrelangle;
	int		barreltime;
	qboolean	barrelspin;
} playerEntity_t;

//=================================================

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s
{
	entityState_t	currstate;		// from cg.frame
	entityState_t	nextstate;		// from cg.nextFrame, if available
	qboolean	interpolate;		// true if next is valid to interpolate to
	qboolean	currvalid;		// true if cg.frame holds this entity

	int		muzzleflashtime;	// move to playerEntity?
	int		prevevent;
	int		teleportflag;

	int		trailtime;	// so missile trails can handle dropped initial packets
	int		dusttrailtime;
	int		misctime;

	int		snapshottime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int		errtime;	// decay the error from this time
	vec3_t		errorigin;
	vec3_t		errangles;

	qboolean	extrapolated;	// false if origin / angles is an interpolation
	vec3_t		raworigin;
	vec3_t		rawangles;

	vec3_t		beamend;

	// exact interpolated position of entity on this frame
	vec3_t		lerporigin;
	vec3_t		lerpangles;

	int		lastplume;	// last thruster plume time
} centity_t;

//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s
{
	struct markPoly_s	*prev, *next;
	int			time;
	qhandle_t		shader;
	qboolean		alphafade;	// fade alpha instead of rgb
	float			color[4];
	poly_t			poly;
	polyVert_t		verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum
{
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
#ifdef MISSIONPACK
	LE_KAMIKAZE,
	LE_INVULIMPACT,
	LE_INVULJUICED,
	LE_SHOWREFENTITY
#endif
} leType_t;

typedef enum
{
	LEF_PUFF_DONT_SCALE = 0x0001,	// do not scale size over time
	LEF_TUMBLE = 0x0002,		// tumble over time, used for ejecting shells
	LEF_SOUND1 = 0x0004,		// sound 1 for kamikaze
	LEF_SOUND2 = 0x0008		// sound 2 for kamikaze
} leFlag_t;

typedef enum
{
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;	// fragment local entities can leave marks on walls

typedef enum
{
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s
{
	struct localEntity_s	*prev, *next;
	leType_t		type;
	int			flags;

	int			starttime;
	int			endtime;
	int			fadeintime;

	float			liferate;	// 1.0 / (endtime - starttime)

	trajectory_t		pos;
	trajectory_t		angles;

	float			bouncefactor;	// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightcolor;

	leMarkType_t		marktype;	// mark to leave on fragment impact
	leBounceSoundType_t	bouncesoundtype;

	refEntity_t		refEntity;
} localEntity_t;

//======================================================================

typedef struct
{
	int		client;
	int		score;
	int		ping;
	int		time;
	int		scoreflags;
	int		powerups;
	int		accuracy;
	int		nimpressive;
	int		nexcellent;
	int		gauntletcount;
	int		ndefend;
	int		nassist;
	int		captures;
	qboolean	perfect;
	int		team;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS 32

typedef struct
{
	qboolean	infovalid;

	char		name[MAX_QPATH];
	team_t		team;

	int		botskill;	// 0 = not bot, 1-5 = bot

	vec3_t		color1;
	vec3_t		color2;

	byte		c1rgba[4];
	byte		c2rgba[4];

	int		score;		// updated by score servercmds
	int		location;	// location index for team mode
	int		health;		// you only get this info about your teammates
	int		armor;
	int		currweap;

	int		handicap;
	int		wins, losses;	// in tourney mode

	int		teamtask;	// task in teamplay (offence/defence)
	qboolean	teamleader;	// true when this is a team leader

	int		powerups;	// so can display quad/flag status

	int		medkitUsageTime;
	int		invulnerabilityStartTime;
	int		invulnerabilityStopTime;

	int		breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char		modelname[MAX_QPATH];
	char		skinname[MAX_QPATH];
	char		headmodelname[MAX_QPATH];
	char		headskinname[MAX_QPATH];
	char		redteam[MAX_TEAMNAME];
	char		blueteam[MAX_TEAMNAME];
	qboolean	deferred;

	qboolean	fixedlegs;	// true if legs yaw is always the same as torso yaw
	qboolean	fixedtorso;	// true if torso never changes yaw

	vec3_t		headoffset;	// move head in icon views
	footstep_t	footsteps;
	gender_t	gender;		// from model

	qhandle_t	torsomodel;
	qhandle_t	torsoskin;

	qhandle_t	modelicon;

	animation_t	animations[MAX_TOTALANIMATIONS];

	sfxHandle_t	sounds[MAX_CUSTOM_SOUNDS];
} clientInfo_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weapinfo_s
{
	qboolean	registered;
	gitem_t		*item;

	qhandle_t	handsmodel;	// the hands don't actually draw, they just position the weapon
	qhandle_t	model;
	qhandle_t	barrelmodel;
	qhandle_t	flashmodel;

	vec3_t		midpoint;	// so it will rotate centered instead of by tag

	float		flashdlight;
	vec3_t		flashcolor;
	sfxHandle_t	flashsnd[4];	// fast firing weapons randomly choose

	qhandle_t	icon;
	qhandle_t	ammoicon;

	qhandle_t	ammomodel;

	qhandle_t	missilemodel;
	sfxHandle_t	missilesound;
	void (*missileTrailFunc)(centity_t *, const struct weapinfo_s *wi);
	float		missilelight;
	vec3_t		missilelightcolor;
	int		missilerenderfx;

	void (*ejectbrass)(centity_t *);

	float		trailradius;
	float		trailtime;

	sfxHandle_t	rdysound;
	sfxHandle_t	firingsound;
} weaponInfo_t;

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct
{
	qboolean	registered;
	qhandle_t	models[MAX_ITEM_MODELS];
	qhandle_t	icon;
} itemInfo_t;

typedef struct
{
	int itemNum;
} powerupInfo_t;

#define MAX_SKULLTRAIL 10

typedef struct
{
	vec3_t	positions[MAX_SKULLTRAIL];
	int	numpositions;
} skulltrail_t;

#define MAX_REWARDSTACK 10
#define MAX_SOUNDBUFFER 20

//======================================================================

// all cg.steptime, cg.ducktime, cg.landtime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS 16

typedef struct
{
	int		clframe;	// incremented each frame

	int		clientNum;

	qboolean	demoplayback;
	qboolean	levelshot;		// taking a level menu screenshot
	int		deferredplayerloading;
	qboolean	loading;		// don't defer players at initial startup
	qboolean	intermissionstarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int		latestsnapnum;	// the number of snapshots the client system has received
	int		latestsnapttime;	// the time from latestsnapnum, so we don't need to read the snapshot yet

	snapshot_t	*snap;			// cg.snap->serverTime <= cg.time
	snapshot_t	*nextsnap;		// cg.nextsnap->serverTime > cg.time, or nil
	snapshot_t	activesnaps[2];

	float		frameinterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	teleportthisframe;
	qboolean	teleportnextframe;

	int		frametime;		// cg.time - cg.oldtime

	int		time;			// this is the time value that the client
	// is rendering at.
	int		oldtime;		// time at last frame, used for missile trails and prediction checking

	int		phystime;		// either cg.snap->time or cg.nextsnap->time

	int		timelimitwarnings;	// 5 min, 1 min, overtime
	int		fraglimitwarnings;

	qboolean	maprestart;		// set on a map restart to set back the weapon

	qboolean	thirdperson;	// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;	// true if prediction has hit a trigger_teleport
	playerState_t	pps;
	centity_t	pplayerent;
	qboolean	validpps;	// clear until the first call to predictplayerstate
	int		predictederrtime;
	vec3_t		predictederr;

	int		eventSequence;
	int		pevents[MAX_PREDICTED_EVENTS];

	float		stepchange;	// for stair up smoothing
	int		steptime;

	float		duckchange;	// for duck viewheight smoothing
	int		ducktime;

	float		landchange;	// for landing hard
	int		landtime;

	// input state sent to server
	int		weapsel;

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefviewangles;	// will be converted to refdef.viewaxis

	// zoom key
	qboolean	zoomed;
	int		zoomtime;
	float		zoomsens;

	// information screen text during loading
	char		infoscreentext[MAX_STRING_CHARS];

	// scoreboard
	int		scoresreqtime;
	int		nscores;
	int		selscore;
	int		teamscores[2];
	score_t		scores[MAX_CLIENTS];
	qboolean	showscores;
	qboolean	scoreboardshown;
	int		scorefadetime;
	char		killername[MAX_NAME_LENGTH];
	char		speclist[MAX_STRING_CHARS];	// list of names
	int		speclen;				// length of list
	float		specwidth;				// width in device units
	int		spectime;				// next time to offset
	int		specpaintx;			// current paint x
	int		specpaintx2;			// current paint x
	int		specoffset;			// current offset from start
	int		specpaintlen;			// current offset from start

#ifdef MISSIONPACK
	// skull trails
	skulltrail_t skulltrails[MAX_CLIENTS];
#endif

	// centerprinting
	int		centerprinttime;
	int		centerprintcharwidth;
	int		centerprinty;
	char		centerprint[1024];
	int		centerprintlines;

	// low ammo warning state
	int		lowAmmoWarning;	// 1 = low, 2 = empty

	// crosshair client ID
	int		xhairclientnum;
	int		xhairclienttime;

	// powerup active flashing
	int		powerupactive;
	int		poweruptime;

	// attacking player
	int		attackertime;
	int		voicetime;

	// reward medals
	int		rewardstack;
	int		rewardtime;
	int		nrewards[MAX_REWARDSTACK];
	qhandle_t	rewardshaders[MAX_REWARDSTACK];
	qhandle_t	rewardsounds[MAX_REWARDSTACK];
	char		rewardmsgs[MAX_REWARDSTACK][MAX_STRING_CHARS];

	// sound buffer mainly for announcer sounds
	int		sndbufin;
	int		sndbufout;
	int		sndtime;
	qhandle_t	sndbuf[MAX_SOUNDBUFFER];

#ifdef MISSIONPACK
	// for voice chat buffer
	int	voiceChatTime;
	int	voiceChatBufferIn;
	int	voiceChatBufferOut;
#endif

	// warmup countdown
	int	warmup;
	int	warmupcount;

	//==========================

	int	itempkup;
	int	itempkuptime;
	int	itempkupblendtime;	// the pulse around the crosshair is timed seperately

	int	weapseltime;
	int	weapanim;
	int	weapanimtime;

	// blend blobs
	float	dmgtime;
	float	dmgx, dmgy, dmgval;

	// view movement
	float	vdmgtime;
	float	vdmgpitch;
	float	vdmgroll;

	// temp working variables for player view
	int	nextorbittime;

	//qboolean cameramode;		// if rendering from a loaded camera

	// development tool
	refEntity_t	testmodelent;
	char		testmodelname[MAX_QPATH];
	qboolean	testgun;
} cg_t;

typedef struct
{
	int	award;
	char	*sfx;
	char	*shader;
	char	*msg;
} awardlist_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct
{
	qhandle_t	charsetShader;
	qhandle_t	charsetProp;
	qhandle_t	charsetPropGlow;
	qhandle_t	charsetPropB;
	qhandle_t	whiteShader;

#ifdef MISSIONPACK
	qhandle_t	redCubeModel;
	qhandle_t	blueCubeModel;
	qhandle_t	redCubeIcon;
	qhandle_t	blueCubeIcon;
#endif
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

#ifdef MISSIONPACK
	qhandle_t	overloadBaseModel;
	qhandle_t	overloadTargetModel;
	qhandle_t	overloadLightsModel;
	qhandle_t	overloadEnergyModel;

	qhandle_t	harvesterModel;
	qhandle_t	harvesterRedSkin;
	qhandle_t	harvesterBlueSkin;
	qhandle_t	harvesterNeutralModel;
#endif

	qhandle_t	armorModel;
	qhandle_t	armorIcon;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;

	qhandle_t	smoke2;

	qhandle_t	machinegunBrassModel;
	qhandle_t	shotgunBrassModel;

	qhandle_t	railRingsShader;
	qhandle_t	railCoreShader;

	qhandle_t	lightningShader;

	qhandle_t	friendShader;

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	selectShader;
	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;
	qhandle_t	hurtLeftShader;
	qhandle_t	hurtForwardShader;
	qhandle_t	hurtUpShader;

	qhandle_t	smokePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	shotgunSmokePuffShader;
	qhandle_t	plasmaBallShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;
#ifdef MISSIONPACK
	qhandle_t	nailPuffShader;
	qhandle_t	blueProxMine;
#endif

	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[5];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	hastePuffShader;
#ifdef MISSIONPACK
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;
#endif

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
	qhandle_t	bloodExplosionShader;

	// hominglauncher
	qhandle_t	lockingOnShader;
	qhandle_t	lockedOnShader;

	// special effects models
	qhandle_t	teleportEffectModel;
	qhandle_t	teleportEffectShader;
#ifdef MISSIONPACK
	qhandle_t	kamikazeEffectModel;
	qhandle_t	kamikazeShockWave;
	qhandle_t	kamikazeHeadModel;
	qhandle_t	kamikazeHeadTrail;
	qhandle_t	guardPowerupModel;
	qhandle_t	scoutPowerupModel;
	qhandle_t	doublerPowerupModel;
	qhandle_t	ammoRegenPowerupModel;
	qhandle_t	invulnerabilityImpactModel;
	qhandle_t	invulnerabilityJuicedModel;
	qhandle_t	medkitUsageModel;
	qhandle_t	dustPuffShader;
	qhandle_t	heartShader;
	qhandle_t	invulnerabilityPowerupModel;
#endif

	// scoreboard headers
	qhandle_t	scoreboardName;
	qhandle_t	scoreboardPing;
	qhandle_t	scoreboardScore;
	qhandle_t	scoreboardTime;

	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;

	// sounds

	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	//sfxHandle_t	sfx_railg;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;
#ifdef MISSIONPACK
	sfxHandle_t	sfx_proxexp;
	sfxHandle_t	sfx_nghit;
	sfxHandle_t	sfx_nghitflesh;
	sfxHandle_t	sfx_nghitmetal;
	sfxHandle_t	sfx_chghit;
	sfxHandle_t	sfx_chghitflesh;
	sfxHandle_t	sfx_chghitmetal;
	sfxHandle_t	kamikazeExplodeSound;
	sfxHandle_t	kamikazeImplodeSound;
	sfxHandle_t	kamikazeFarSound;
	sfxHandle_t	useInvulnerabilitySound;
	sfxHandle_t	invulnerabilityImpactSound1;
	sfxHandle_t	invulnerabilityImpactSound2;
	sfxHandle_t	invulnerabilityImpactSound3;
	sfxHandle_t	invulnerabilityJuicedSound;
	sfxHandle_t	obeliskHitSound1;
	sfxHandle_t	obeliskHitSound2;
	sfxHandle_t	obeliskHitSound3;
	sfxHandle_t	obeliskRespawnSound;
	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;
#endif
	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t	talkSound;
	sfxHandle_t	landSound;
	sfxHandle_t	fallSound;
	sfxHandle_t	jumpPadSound;

	sfxHandle_t	oneMinuteSound;
	sfxHandle_t	fiveMinuteSound;
	sfxHandle_t	suddenDeathSound;

	sfxHandle_t	threeFragSound;
	sfxHandle_t	twoFragSound;
	sfxHandle_t	oneFragSound;

	sfxHandle_t	hitSound;
	sfxHandle_t	hitSoundHighArmor;
	sfxHandle_t	hitSoundLowArmor;
	sfxHandle_t	hitTeamSound;

	sfxHandle_t	takenLeadSound;
	sfxHandle_t	tiedLeadSound;
	sfxHandle_t	lostLeadSound;

	sfxHandle_t	voteNow;
	sfxHandle_t	votePassed;
	sfxHandle_t	voteFailed;

	sfxHandle_t	watrInSound;
	sfxHandle_t	watrOutSound;
	sfxHandle_t	watrUnSound;

	sfxHandle_t	flightSound;
	sfxHandle_t	medkitSound;

#ifdef MISSIONPACK
	sfxHandle_t	weaponHoverSound;
#endif

	// teamplay sounds
	sfxHandle_t	captureAwardSound;
	sfxHandle_t	redScoredSound;
	sfxHandle_t	blueScoredSound;
	sfxHandle_t	redLeadsSound;
	sfxHandle_t	blueLeadsSound;
	sfxHandle_t	teamsTiedSound;

	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;
	sfxHandle_t	takenYourTeamSound;
	sfxHandle_t	takenOpponentSound;

	sfxHandle_t	redFlagReturnedSound;
	sfxHandle_t	blueFlagReturnedSound;
#ifdef MISSIONPACK
	sfxHandle_t	neutralFlagReturnedSound;
#endif
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t	yourTeamTookEnemyFlagSound;
	sfxHandle_t	youHaveFlagSound;
#ifdef MISSIONPACK
	sfxHandle_t	enemyTookTheFlagSound;
	sfxHandle_t	yourTeamTookTheFlagSound;
	sfxHandle_t	yourBaseIsUnderAttackSound;
#endif
	sfxHandle_t	holyShitSound;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

	// hominglauncher
	sfxHandle_t	lockingOnSound;
	sfxHandle_t	lockedOnSound;

	// thrust sounds
	sfxHandle_t	thrustSound;
	sfxHandle_t	thrustBackSound;
	sfxHandle_t	thrustIdleSound;
	// thrust sounds on other players
	sfxHandle_t	thrustOtherSound;
	sfxHandle_t	thrustOtherBackSound;
	sfxHandle_t	thrustOtherIdleSound;

#ifdef MISSIONPACK
	// new stuff
	qhandle_t	patrolShader;
	qhandle_t	assaultShader;
	qhandle_t	campShader;
	qhandle_t	followShader;
	qhandle_t	defendShader;
	qhandle_t	teamLeaderShader;
	qhandle_t	retrieveShader;
	qhandle_t	escortShader;
	qhandle_t	flagShaders[3];
	sfxHandle_t	countPrepareTeamSound;

	sfxHandle_t	ammoregenSound;
	sfxHandle_t	doublerSound;
	sfxHandle_t	guardSound;
	sfxHandle_t	scoutSound;

#endif

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
	sfxHandle_t	n_healthSound;
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;
} cgMedia_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct
{
	gameState_t	gameState;	// gamestate from server
	glconfig_t	glconfig;	// rendering configuration
	float		scrnxscale;	// derived from glconfig
	float		scrnyscale;
	float		scrnxbias;

	int		serverCommandSequence;	// reliable command stream counter
	int		nprocessedsnaps;			// the number of snapshots cgame has requested

	qboolean	srvislocal;	// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t	gametype;
	int		dmflags;
	int		teamflags;
	int		fraglimit;
	int		capturelimit;
	int		timelimit;
	int		maxclients;
	char		mapname[MAX_QPATH];
	char		redteam[MAX_QPATH];
	char		blueteam[MAX_QPATH];

	int		votetime;
	int		voteyes;
	int		voteno;
	qboolean	votemodified;	// beep whenever changed
	char		votestr[MAX_STRING_TOKENS];

	int		teamvotetime[2];
	int		teamvoteyes[2];
	int		teamvoteno[2];
	qboolean	teamVoteModified[2];	// beep whenever changed
	char		teamvotestr[2][MAX_STRING_TOKENS];

	int		levelStartTime;

	int		scores1, scores2;	// from configstrings
	int		redflag, blueflag;	// flag status from configstrings
	int		flagStatus;

	qboolean	newHud;

	// locally derived information from gamestate
	qhandle_t	gamemodels[MAX_MODELS];
	sfxHandle_t	gamesounds[MAX_SOUNDS];

	int		ninlinemodels;
	qhandle_t	inlinedrawmodel[MAX_MODELS];
	vec3_t		inlinemodelmidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char		teamchatmsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int		teamchatmsgtimes[TEAMCHAT_HEIGHT];
	int		teamchatpos;
	int		teamlastchatpos;

	int		cursorx;
	int		cursory;
	qboolean	evhandling;
	qboolean	mousecaptured;
	qboolean	sizinghud;
	void		*captureditem;
	qhandle_t	activecursor;

	// orders
	int		currentOrder;
	qboolean	orderPending;
	int		orderTime;
	int		currentVoiceClient;
	int		acceptOrderTime;
	int		acceptTask;
	int		acceptLeader;
	char		acceptVoice[MAX_NAME_LENGTH];

	// media
	cgMedia_t	media;
} cgs_t;

//==============================================================================

extern cgs_t cgs;
extern cg_t cg;
extern centity_t cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t cg_items[MAX_ITEMS];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];
extern awardlist_t cg_awardlist[];
extern int cg_nawardlist;

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_gibs;
extern vmCvar_t cg_drawDamageDir;
extern vmCvar_t cg_drawTimer;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawSpeedometer;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_draw3dIcons;
extern vmCvar_t cg_drawIcons;
extern vmCvar_t cg_drawAmmoWarning;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawRewards;
extern vmCvar_t cg_drawTeamOverlay;
extern vmCvar_t cg_teamOverlayUserinfo;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_footsteps;
extern vmCvar_t cg_addMarks;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_viewsize;
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_ignore;
extern vmCvar_t cg_simpleItems;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_zoomFov;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_lagometer;
extern vmCvar_t cg_drawAttacker;
extern vmCvar_t cg_synchronousClients;
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatHeight;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_forceModel;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_deferPlayers;
extern vmCvar_t cg_drawFriend;
extern vmCvar_t cg_teamChatsOnly;
#ifdef MISSIONPACK
extern vmCvar_t cg_noVoiceChats;
extern vmCvar_t cg_noVoiceText;
#endif
extern vmCvar_t cg_scorePlum;
extern vmCvar_t cg_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
//extern	vmCvar_t		cg_pmove_fixed;
extern vmCvar_t cg_cameraOrbit;
extern vmCvar_t cg_cameraOrbitDelay;
extern vmCvar_t cg_timescaleFadeEnd;
extern vmCvar_t cg_timescaleFadeSpeed;
extern vmCvar_t cg_timescale;
extern vmCvar_t cg_cameraMode;
extern vmCvar_t cg_smallFont;
extern vmCvar_t cg_bigFont;
extern vmCvar_t cg_noTaunt;
extern vmCvar_t cg_noProjectileTrail;
extern vmCvar_t cg_oldRail;
extern vmCvar_t cg_oldRocket;
extern vmCvar_t cg_oldPlasma;
extern vmCvar_t cg_trueLightning;
#ifdef MISSIONPACK
extern vmCvar_t cg_redTeamName;
extern vmCvar_t cg_blueTeamName;
extern vmCvar_t cg_currentSelectedPlayer;
extern vmCvar_t cg_currentSelectedPlayerName;
extern vmCvar_t cg_singlePlayer;
extern vmCvar_t cg_enableDust;
extern vmCvar_t cg_enableBreath;
extern vmCvar_t cg_singlePlayerActive;
extern vmCvar_t cg_recordSPDemo;
extern vmCvar_t cg_recordSPDemoName;
extern vmCvar_t cg_obeliskRespawnDelay;
#endif

// cg_main.c
const char *	getconfigstr(int index);
const char *	cgargv(int arg);

void QDECL	cgprintf(const char *msg, ...) __attribute__ ((format(printf, 1, 2)));
void QDECL	cgerrorf(const char *msg, ...) __attribute__ ((noreturn, format(printf, 1, 2)));

void		startmusic(void);

void		updatecvars(void);

int		xhairplayer(void);
int		getlastattacker(void);
void		CG_LoadMenus(const char *menuFile);
void		keyevent(int key, qboolean down);
void		mouseevent(int x, int y);
void		eventhandling(int type);
void		rankrunframe(void);
void		setscoresel(void *menu);
score_t *	getselscore(void);
void		mkspecstr(void);

// cg_view.c
void	CG_TestModel_f(void);
void	CG_TestGun_f(void);
void	CG_TestModelNextFrame_f(void);
void	CG_TestModelPrevFrame_f(void);
void	CG_TestModelNextSkin_f(void);
void	CG_TestModelPrevSkin_f(void);
void	CG_TestLight_f(void);
void	CG_ZoomDown_f(void);
void	CG_ZoomUp_f(void);
void	addbufferedsound(sfxHandle_t sfx);

void	drawframe(int serverTime, stereoFrame_t stereoview, qboolean demoplayback);

// cg_drawtools.c
void	drawbigstr(int x, int y, const char *s, float alpha);
void	drawfixedstr(int x, int y, const char *s, float alpha);
void	drawhudfield(float x, float y, const char *s, vec4_t color);
void	drawbigstrcolor(int x, int y, const char *s, vec4_t color);
void	drawsmallstr(int x, int y, const char *s, float alpha);
void	drawsmallstrcolor(int x, int y, const char *s, vec4_t color);

int	drawstrlen(const char *str);

float * fadecolor(int startMsec, int totalMsec);
float * teamcolor(int team);
void	tileclear(void);
void	colorforhealth(vec4_t hcolor);
void	getcolorforhealth(int health, int armor, vec4_t hcolor);
void	drawsides(float x, float y, float w, float h, float size);
void	drawtopbottom(float x, float y, float w, float h, float size);

// cg_draw.c, cg_newDraw.c
extern int sortedteamplayers[TEAM_MAXOVERLAY];
extern int nsortedteamplayers;
extern int drawTeamOverlayModificationCount;
extern char syschat[256];
extern char teamchat1[256];
extern char teamchat2[256];

void		lagometerframeinfo(void);
void		lagometersnapinfo(snapshot_t *snap);
void		centerprint(const char *str, int y, int charWidth);
void		drawactive(stereoFrame_t stereoview);
void		drawflag(float x, float y, float w, float h, int team, qboolean force2D);
void		drawteambg(int x, int y, int w, int h, float alpha, int team);
void		ownerdraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void		CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int		CG_Text_Width(const char *text, float scale, int limit);
int		CG_Text_Height(const char *text, float scale, int limit);
void		CG_SelectPrevPlayer(void);
void		CG_SelectNextPlayer(void);
float		CG_GetValue(int ownerDraw);
qboolean	ownerdrawvisible(int flags);
void		CG_RunMenuScript(char **args);
void		CG_ShowResponseHead(void);
void		CG_SetPrintString(int type, const char *p);
void		CG_InitTeamChat(void);
void		CG_GetTeamColor(vec4_t *color);
const char *	CG_GetGameStatusText(void);
const char *	CG_GetKillerText(void);
void		drawmodel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void		CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void		CG_CheckOrderPending(void);
const char *	CG_GameTypeString(void);
qboolean	CG_YourTeamHasFlag(void);
qboolean	CG_OtherTeamHasFlag(void);
qhandle_t	CG_StatusHandle(int task);

// cg_player.c
void		doplayer(centity_t *cent);
void		resetplayerent(centity_t *cent);
void		addrefentitywithpowerups(refEntity_t *ent, entityState_t *state, int team);
void		newclientinfo(int clientNum);
sfxHandle_t	customsound(int clientNum, const char *soundName);

// cg_predict.c
void	mksolidlist(void);
int	pointcontents(const vec3_t point, int passEntityNum);
void	cgtrace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
		 int skipNumber, int mask);
void	predictplayerstate(void);
void	loaddeferred(void);

// cg_events.c
void		chkevents(centity_t *cent);
const char *	placestr(int rank);
void		entevent(centity_t *cent, vec3_t position);
void		painevent(centity_t *cent, int health);

// cg_ents.c
void	setentsoundpos(centity_t *cent);
void	addpacketents(void);
void	drawbeam(centity_t *cent);
void	adjustposformover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out);

void	entontag(refEntity_t *entity, const refEntity_t *parent,
			       qhandle_t parentModel, char *tagName);
void	rotentontag(refEntity_t *entity, const refEntity_t *parent,
				      qhandle_t parentModel, char *tagName);

// cg_weapons.c
void	CG_NextWeapon_f(void);
void	CG_PrevWeapon_f(void);
void	CG_Weapon_f(void);

void	registerweap(int weaponNum);
void	registeritemgfx(int itemNum);

void	fireweap(centity_t *cent);
void	missilehitwall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType);
void	missilehitplayer(int weapon, vec3_t origin, vec3_t dir, int entityNum);
void	shotgunfire(entityState_t *es);
void	dobullet(vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum);

void	dorailtrail(clientInfo_t *ci, vec3_t start, vec3_t end);
void	grappletrail(centity_t *ent, const weaponInfo_t *wi);
void	addviewweap(playerState_t *ps);
void	addplayerweap(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team);
void	drawweapsel(void);

void	outofammochange(void);	// should this be in pmove?

// cg_marks.c
void	initmarkpolys(void);
void	addmarks(void);
void	impactmark(qhandle_t shader,
		      const vec3_t origin, const vec3_t dir,
		      float orientation,
		      float r, float g, float b, float a,
		      qboolean alphafade,
		      float radius, qboolean temporary);

// cg_localents.c
void		initlocalents(void);
localEntity_t * alloclocalent(void);
void		addlocalents(void);

// cg_effects.c
localEntity_t *smokepuff(const vec3_t p,
			    const vec3_t vel,
			    float radius,
			    float r, float g, float b, float a,
			    float duration,
			    int starttime,
			    int fadeintime,
			    int flags,
			    qhandle_t hShader);
void		bubbletrail(vec3_t start, vec3_t end, float spacing);
void		spawneffect(vec3_t org);
#ifdef MISSIONPACK
void		CG_KamikazeEffect(vec3_t org);
void		CG_ObeliskExplode(vec3_t org, int entityNum);
void		CG_ObeliskPain(vec3_t org);
void		CG_InvulnerabilityImpact(vec3_t org, vec3_t angles);
void		CG_InvulnerabilityJuiced(vec3_t org);
void		CG_LightningBoltBeam(vec3_t start, vec3_t end);
#endif
void		scoreplum(int client, vec3_t org, int score);

void		gibplayer(vec3_t playerOrigin);
void		CG_BigExplode(vec3_t playerOrigin);

void		bleed(vec3_t origin, int entityNum);

localEntity_t * explosion(vec3_t origin, vec3_t dir,
				 qhandle_t hModel, qhandle_t shader, int msec,
				 qboolean isSprite);

// cg_snapshot.c
void processsnaps(void);

// cg_info.c
void	loadingstr(const char *s);
void	loadingitem(int itemNum);
void	loadingclient(int clientNum);
void	drawinfo(void);

// cg_scoreboard.c
qboolean	drawoldscoreboard(void);
void		CG_DrawOldTourneyScoreboard(void);

// cg_consolecmds.c
qboolean	consolecmd(void);
void		initconsolesmds(void);

// cg_servercmds.c
void	execnewsrvcmds(int latestSequence);
void	parsesrvinfo(void);
void	setconfigvals(void);
void	shaderstatechanged(void);

// cg_playerstate.c
void	respawn(void);
void	pstransition(playerState_t *ps, playerState_t *ops);
void	chkpredictableevents(playerState_t *ps);
void	pushreward(sfxHandle_t sfx, qhandle_t shader, const char *msg, int nrewards);

//===============================================

// system traps
// These functions are how the cgame communicates with the main game system

// print message on the local console
void	trap_Print(const char *fmt);

// abort the game
void	trap_Error(const char *fmt) __attribute__((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the drawframe parameter
int	trap_Milliseconds(void);

// console variable interaction
void	trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void	trap_Cvar_Update(vmCvar_t *vmCvar);
void	trap_Cvar_Set(const char *var_name, const char *value);
void	trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

// ServerCommand and consolecmd parameter access
int	trap_Argc(void);
void	trap_Argv(int n, char *buffer, int bufferLength);
void	trap_Args(char *buffer, int bufferLength);

// filesystem access
// returns length of file
int	trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void	trap_FS_Read(void *buffer, int len, fileHandle_t f);
void	trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void	trap_FS_FCloseFile(fileHandle_t f);
int	trap_FS_Seek(fileHandle_t f, long offset, int origin);			// fsOrigin_t

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void trap_SendConsoleCommand(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand(const char *cmdName);
void		trap_RemoveCommand(const char *cmdName);

// send a string to the server over the network
void		trap_SendClientCommand(const char *s);

// force a screen update, only used during gamestate load
void		trap_UpdateScreen(void);

// model collision
void		trap_CM_LoadMap(const char *mapname);
int		trap_CM_NumInlineModels(void);
clipHandle_t	trap_CM_InlineModel(int index);	// 0 = world, 1+ = bmodels
clipHandle_t	trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
int		trap_CM_PointContents(const vec3_t p, clipHandle_t model);
int		trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);
void		trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
				 const vec3_t mins, const vec3_t maxs,
				 clipHandle_t model, int brushmask);
void		trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
				     const vec3_t mins, const vec3_t maxs,
				     clipHandle_t model, int brushmask);
void		trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
					    const vec3_t mins, const vec3_t maxs,
					    clipHandle_t model, int brushmask,
					    const vec3_t origin, const vec3_t angles);
void trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
				     const vec3_t mins, const vec3_t maxs,
				     clipHandle_t model, int brushmask,
				     const vec3_t origin, const vec3_t angles);

// Returns the projection of a polygon onto the solid brushes in the world
int trap_CM_MarkFragments(int numPoints, const vec3_t *points,
			  const vec3_t projection,
			  int maxPoints, vec3_t pointBuffer,
			  int maxFragments, markFragment_t *fragmentBuffer);

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void	trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void	trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void	trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
void	trap_S_ClearLoopingSounds(qboolean killall);
void	trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void	trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void	trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
sfxHandle_t	trap_S_RegisterSound(const char *sample, qboolean compressed);		// returns buzz if not found
void		trap_S_StartBackgroundTrack(const char *intro, const char *loop);	// empty name stops music
void		trap_S_StopBackgroundTrack(void);

void		trap_R_LoadWorldMap(const char *mapname);

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel(const char *name);		// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin(const char *name);		// returns all white if not found
qhandle_t	trap_R_RegisterShader(const char *name);	// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip(const char *name);	// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void	trap_R_ClearScene(void);
void	trap_R_AddRefEntityToScene(const refEntity_t *re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void		trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);
void		trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void		trap_R_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b);
int		trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
void		trap_R_RenderScene(const refdef_t *fd);
void		trap_R_SetColor(const float *rgba);	// nil = 1,1,1,1
void		trap_R_DrawStretchPic(float x, float y, float w, float h,
				      float s1, float t1, float s2, float t2, qhandle_t hShader);
void		trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
int		trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
			       float frac, const char *tagName);
void		trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);
qboolean	trap_R_inPVS(const vec3_t p1, const vec3_t p2);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void trap_GetGlconfig(glconfig_t *glconfig);

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void trap_GetGameState(gameState_t *gamestate);

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean trap_GetServerCommand(int serverCommandNumber);

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int		trap_GetCurrentCmdNumber(void);

qboolean	trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);

// used for the weapon select and zoom
void		trap_SetUserCmdValue(int stateValue, float sensitivityScale);

// aids for VM testing
void		testPrintInt(char *string, int i);
void		testPrintFloat(char *string, float f);

int		trap_MemoryRemaining(void);
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown(int keynum);
int		trap_Key_GetCatcher(void);
void		trap_Key_SetCatcher(int catcher);
int		trap_Key_GetKey(const char *binding);

typedef enum
{
	SYSTEM_PRINT,
	CHAT_PRINT,
	TEAMCHAT_PRINT
} q3print_t;

int		trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status	trap_CIN_StopCinematic(int handle);
e_status	trap_CIN_RunCinematic(int handle);
void		trap_CIN_DrawCinematic(int handle);
void		trap_CIN_SetExtents(int handle, int x, int y, int w, int h);

int		trap_RealTime(qtime_t *qtime);
void		trap_SnapVector(float *v);

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken(char *buffer, int bufferSize);

void		CG_ClearParticles(void);
void		CG_AddParticles(void);
void		CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void		CG_ParticleSmoke(qhandle_t pshader, centity_t *cent);
void		CG_AddParticleShrapnel(localEntity_t *le);
void		CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent);
void		CG_ParticleBulletDebris(vec3_t org, vec3_t vel, int duration);
void		CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void		CG_ParticleDust(centity_t *cent, vec3_t origin, vec3_t dir);
void		CG_ParticleMisc(qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void		CG_ParticleExplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
extern qboolean initparticles;
int		CG_NewParticleArea(int num);

