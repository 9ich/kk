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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION			BASEGAME

#define INFINITE			1000000

#define FRAMETIME			100	// msec
#define MULTIKILL_TIME			3000	// max interval between each multikill kill
#define REWARD_SPRITE_TIME		2000
#define AFK_TIME			10000	// player is inactive but not yet kicked
#define ROUND_END_WAIT			1000	// wait after a clan arena round has ended

#define INTERMISSION_DELAY_TIME		1000
#define SP_INTERMISSION_DELAY_TIME	5000

// gentity->flags
#define FL_GODMODE			0x00000010
#define FL_NOTARGET			0x00000020
#define FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS			0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client

// movers are things like doors, plats, buttons, etc
typedef enum
{
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

#define SP_PODIUM_MODEL "models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s	gentity_t;
typedef struct gclient_s	gclient_t;

struct gentity_s
{
	entityState_t	s;	// communicated by server to clients
	entityShared_t	r;	// shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;	// nil if not a client

	qboolean		inuse;

	char			*classname;	// set in QuakeEd
	int			spawnflags;	// set in QuakeEd

	qboolean		neverfree;	// if true, FreeEntity will only unlink
	// bodyque uses this

	int			flags;	// FL_* variables

	char			*model;
	char			*model2;
	int			freetime;	// level.time when the object was freed

	int			eventtime;	// events will be cleared EVENT_VALID_MSEC after set
	qboolean		freeafterevent;
	qboolean		unlinkAfterEvent;

	qboolean		physobj;	// if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float			physbounce;	// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;	// brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	// movers
	moverState_t	moverstate;
	int		soundpos1;
	int		sound1to2;
	int		sound2to1;
	int		soundpos2;
	int		soundloop;
	gentity_t	*parent;
	gentity_t	*nexttrain;
	gentity_t	*prevtrain;
	vec3_t		pos1, pos2;

	char		*message;

	int		timestamp;	// body queue sinking, etc

	char		*target;
	char		*targetname;
	char		*team;
	char		*targetshadername;
	char		*newtargetshadername;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int		nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int		paindebouncetime;
	int		flysounddebouncetime;	// wind tunnel
	int		lastmovetime;

	int		health;

	qboolean	takedmg;

	int		damage;
	int		splashdmg;	// quad will increase this without increasing radius
	int		splashradius;
	int		meansofdeath;
	int		splashmeansofdeath;
	float		flakradius;	// if >0, missile explodes within flakradius of enemies if not on course to hit enemy

	int		count;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;	// next entity in team
	gentity_t	*teammaster;	// master of the team

#ifdef MISSIONPACK
	int		kamikazeTime;
	int		kamikazeShockTime;
#endif

	int		watertype;
	int		waterlevel;

	int		noiseindex;

	// timing variables
	float		wait;
	float		random;

	gitem_t		*item;	// for bonus items

	int		homingtarget;	// hominglauncher

	// team_controlpoint
	float		cpcaprate;	// capturerate field
	int		cpstatus;	// CP_IDLE, etc.
	int		cpowner;	// team that owns this point
	float		cpprogress;	// progress towards a capture
	int		cplastredtime;	// last time that red was occupying this control point
	int		cplastbluetime;	// last time that blue was occupying this
	int		cpredplayers;	// number of red players occupying this
	int		cpblueplayers;	// number of blue players occupying this
	int		cporder;
	int		cpround;
	char		*cpattackingteam;

	// team_cp_round_timer
	int		cptimelimit;
	int		cpmaxtimelimit;
	int		cpsetuptime;
};

typedef enum
{
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum
{
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum
{
	TEAM_BEGIN,	// Beginning a team game, spawn at base
	TEAM_ACTIVE	// Now actively playing
} playerTeamStateState_t;

typedef struct
{
	playerTeamStateState_t	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	float			lasthurtcarrier;
	float			lastreturnedflag;
	float			flagsince;
	float			lastfraggedcarrier;
} playerTeamState_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in sessinit() / sessread() / sesswrite()
typedef struct
{
	team_t			team;
	int			specnum;		// for determining next-in-line to play
	spectatorState_t	specstate;
	int			specclient;	// for chasecam and follow mode
	int			wins, losses;		// tournament stats
	qboolean		teamleader;		// true when this client is a team leader
} clientSession_t;

#define MAX_NETNAME	36
#define MAX_VOTE_COUNT	3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at clientbegin()
typedef struct
{
	clientConnected_t	connected;
	usercmd_t		cmd;			// we would lose angles if not persistant
	qboolean		localclient;		// true if "ip" info key is "localhost"
	qboolean		initialspawn;		// the first spawn should be at a cool location
	qboolean		predictitempickup;	// based on cg_predictItems userinfo
	qboolean		pmovefixed;		//
	char			netname[MAX_NETNAME];
	int			maxhealth;		// for handicapping
	int			entertime;		// level.time the client entered the game
	playerTeamState_t	teamstate;		// status in teamplay games
	int			votecount;		// to prevent people from constantly calling votes
	int			teamvotecount;		// to prevent people from constantly calling votes
	qboolean		teaminfo;		// send team overlay updates?
} clientPersistant_t;

// this structure is cleared on each clientspawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s
{
	// ps MUST be the first element, because the server expects it
	playerState_t		ps;	// communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t	pers;
	clientSession_t		sess;

	qboolean		ready;		// player is ready to start the match
	qboolean		readytoexit;	// wishes to leave the intermission

	qboolean		noclip;

	int			lastcmdtime;	// level.time of last usercmd_t, for EF_CONNECTION
						// we can't just use pers.lastCommand.time, because
						// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latchedbuttons;

	vec3_t			oldorigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int		dmgarmor;		// damage absorbed by armor
	int		dmgblood;		// damage taken out of health
	int		dmgknockback;		// impact damage
	vec3_t		dmgfrom;		// origin for vector calculation
	qboolean	dmgfromworld;		// if true, don't use the dmgfrom vector

	int		accuratecount;		// for "impressive" reward sound

	int		accuracyshots;		// total number of shots
	int		accuracyhits;		// total number of hits

	int		lastkilledclient;	// last client that this client killed
	int		lasthurtclient;		// last client that damaged this client
	int		lasthurt_mod;		// type of damage the client did

	// timers
	int		respawntime;		// can respawn when time > this, force after g_forcerespwan
	int		inactivitytime;		// kick players when time > this
	qboolean	inactivitywarning;	// qtrue if the five seoond warning has been given
	int		afktime;		// player is inactive but not yet kicked

	int		airouttime;

	int		lastkilltime;		// for multiple kill rewards
	int		lastmultikilltime;
	int		lastmultikill;

	int		killsthislife;		// number of enemies killed since respawning, for AWARD_SADDAY

	qboolean	fireheld[WS_NUMSLOTS];		// used for hook
	gentity_t	*hook;			// grapple hook if out

	int		switchteamtime;		// time the player switched teams

	// residualtime is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int		residualtime;

#ifdef MISSIONPACK
	gentity_t	*persistantPowerup;
	int		portalID;
	int		ammoTimes[WP_NUM_WEAPONS];
	int		invulnerabilityTime;
#endif

	char		*areabits;
};

// this structure is cleared as each map is entered
#define MAX_SPAWN_VARS		64
#define MAX_SPAWN_VARS_CHARS	4096

typedef struct
{
	struct gclient_s	*clients;	// [maxclients]

	struct gentity_s	*entities;
	int			gentitySize;
	int			nentities;	// MAX_CLIENTS <= nentities <= ENTITYNUM_MAX_NORMAL

	int			warmuptime;	// restart match at this time
	int			warmupstate;	// WARMUP_*

	fileHandle_t		logfile;

	// store latched cvars here that we want to get at often
	int			maxclients;

	int			framenum;
	int			time;		// in msec
	int			prevtime;	// so movers can back up when blocked

	int			starttime;	// level.time the map was started

	int			teamscores[TEAM_NUM_TEAMS];
	int			lastteamlocationtime;	// last time of client team location update

	qboolean		newsess;		// don't use any old session data, because
							// we changed gametype

	qboolean		restarted;		// waiting for a map_restart to fire

	int			nconnectedclients;
	int			nnonspecclients;		// includes connecting clients
	int			nplayingclients;		// connected, non-spectators
	int			sortedclients[MAX_CLIENTS];	// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators

	int			snd_fry;			// sound index for standing in lava

	int			warmupmodificationcount;	// for detecting if g_warmup is changed

	// voting state
	char			votestr[MAX_STRING_CHARS];
	char			votedisplaystr[MAX_STRING_CHARS];
	int			votetime;		// level.time vote was called
	int			voteexectime;		// time the vote is executed
	int			voteyes;
	int			voteno;
	int			nvoters;		// set by calcranks

	// team voting state
	char			teamvotestr[2][MAX_STRING_CHARS];
	int			teamvotetime[2];	// level.time vote was called
	int			teamvoteyes[2];
	int			teamvoteno[2];
	int			nteamvoters[2];		// set by calcranks

	// spawn variables
	qboolean		spawning;			// the entspawn*() functions are valid
	int			nspawnvars;
	char			*spawnvars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			nspawnvarchars;
	char			spawnvarchars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int			intermissionqueued;	// intermission was qualified, but
	// wait INTERMISSION_DELAY_TIME before
	// actually going there so the last
	// frag can be watched.  Disable future
	// kills during this delay
	int		intermissiontime;	// time the intermission was started
	char		*changemap;
	qboolean	readytoexit;		// at least one client wants to exit
	int		exittime;
	vec3_t		intermissionpos;	// also used for spectator spawns
	vec3_t		intermissionangle;

	qboolean	loclinked;	// target_locations get linked
	gentity_t	*lochead;	// head of the location list

	// total kills for AWARD_FIRSTBLOOD
	int		totalkills;

	// rounded game types
	int		roundbegintime;		// time that pre-round countdown ended, if any
	int		roundendtime;
	int		round;

	// GT_CP
	int		attackingteam;
	int		numcprounds;
	int		roundtimelimit;
	int		maxroundtimelimit;
	int		setuptime;
	gentity_t	*cp[MAX_CONTROLPOINTS];

	int		portalSequence;
} level_locals_t;

// g_spawn.c
qboolean	spawnstr(const char *key, const char *defaultString, char **out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	spawnfloat(const char *key, const char *defaultString, float *out);
qboolean	spawnint(const char *key, const char *defaultString, int *out);
qboolean	spawnvec(const char *key, const char *defaultString, float *out);
void		spawnall(void);
char *		newstr(const char *string);

// g_cmds.c
void	Cmd_Score_f(gentity_t *ent);
void	stopfollowing(gentity_t *ent);
void	broadcastteamchange(gclient_t *client, int oldTeam);
void	setteam(gentity_t *ent, char *s);
void	Cmd_FollowCycle_f(gentity_t *ent, int dir);

// g_items.c
void		checkteamitems(void);
void		runitem(gentity_t *ent);
void		itemrespawn(gentity_t *ent);

void		useholdableitem(gentity_t *ent);
void		precacheitem(gitem_t *it);
gentity_t *	itemdrop(gentity_t *ent, gitem_t *item, float angle);
gentity_t *	itemlaunch(gitem_t *item, vec3_t origin, vec3_t velocity);
void		setrespawn(gentity_t *ent, float delay);
void		itemspawn(gentity_t *ent, gitem_t *item);
void		itemspawnfinish(gentity_t *ent);
void		weap_think(gentity_t *ent);
int		armorindex(gentity_t *ent);
void		addammo(gentity_t *ent, int weapon, int count);
void		item_touch(gentity_t *ent, gentity_t *other, trace_t *trace);

void		clearitems(void);
void		registeritem(gitem_t *item);
void		mkitemsconfigstr(void);

// g_utils.c
int		getmodelindex(char *name);
int		getsoundindex(char *name);
void		teamcmd(team_t team, char *cmd);
void		killbox(gentity_t *ent);
gentity_t *	findent(gentity_t *from, int fieldofs, const char *match);
gentity_t *	picktarget(char *targetname);
void		usetargets(gentity_t *ent, gentity_t *activator);
void		setmovedir(vec3_t angles, vec3_t movedir);

void		entinit(gentity_t *e);
gentity_t *	entspawn(void);
gentity_t *	enttemp(vec3_t origin, int event);
void		mksound(gentity_t *ent, int channel, int soundIndex);
void		entfree(gentity_t *e);
qboolean	numentsfree(void);

void		touchtriggers(gentity_t *ent);

float *		tv(float x, float y, float z);

float		vectoyaw(const vec3_t vec);

void		addpredictable(gentity_t *ent, int event, int eventParm);
void		addevent(gentity_t *ent, int event, int eventParm);
void		setorigin(gentity_t *ent, vec3_t origin);
void		addshaderremap(const char *oldShader, const char *newShader, float timeOffset);
const char *	mkshaderstateconfigstr(void);

// g_combat.c
qboolean	candamage(gentity_t *targ, vec3_t origin);
void		entdamage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
qboolean	radiusdamage(vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
int		invulneffect(gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir);
void		body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath);
void		tossclientitems(gentity_t *self);
#ifdef MISSIONPACK
void		tossclientpowerups(gentity_t *self);
#endif
void		tossclientcubes(gentity_t *self);
void		giveaward(gclient_t *cl, int award);

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK		0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008	// armor, shields, invulnerability, and godmode have no effect
#ifdef MISSIONPACK
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010	// armor, shields, invulnerability, and godmode have no effect
#endif

// g_missile.c
void		runmissile(gentity_t *ent);

gentity_t *	fire_plasma(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *	fire_grenade(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *	fire_rocket(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *	fire_bullet(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *	fire_homingrocket(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *	fire_bfg(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *	fire_grapple(gentity_t *self, vec3_t start, vec3_t dir);
#ifdef MISSIONPACK
gentity_t *	fire_nail(gentity_t *self, vec3_t start, vec3_t forward);
gentity_t *	fire_prox(gentity_t *self, vec3_t start, vec3_t aimdir);
#endif

// g_mover.c
void	runmover(gentity_t *ent);
void	doortrigger_touch(gentity_t *ent, gentity_t *other, trace_t *trace);

// g_trigger.c
void trigger_teleporter_touch(gentity_t *self, gentity_t *other, trace_t *trace);

// g_misc.c
void	teleportentity(gentity_t *player, vec3_t origin, vec3_t angles);
#ifdef MISSIONPACK
void	dropportalsrc(gentity_t *ent);
void	dropportaldest(gentity_t *ent);
#endif

// g_weapon.c
qboolean	logaccuracyhit(gentity_t *target, gentity_t *attacker);
void		calcmuzzlepoint(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void		snapvectortowards(vec3_t v, vec3_t to);
qboolean	chkgauntletattack(gentity_t *ent);
void		weapon_hook_free(gentity_t *ent);
void		weapon_hook_think(gentity_t *ent);
void		weapon_hominglauncher_scan(gentity_t *ent);

// g_client.c
int		getteamcount(int ignoreClientNum, team_t team);
int		getteamleader(int team);
team_t		pickteam(int ignoreClientNum);
void		setviewangles(gentity_t *ent, vec3_t angle);
gentity_t *	selectspawnpoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot);
void		clientrespawn(gentity_t *ent);
void		intermission(void);
void		clientspawn(gentity_t *ent);
void		player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void		addscore(gentity_t *ent, vec3_t origin, int score);
void		calcranks(void);
qboolean	maytelefrag(gentity_t *spot);

// g_svcmds.c
qboolean	consolecmd(void);
void		processipbans(void);
qboolean	filterpacket(char *from);

// g_weapon.c
void	fireweapon(gentity_t *ent, int slot);
#ifdef MISSIONPACK
void	startkamikaze(gentity_t *ent);
#endif

// g_cmds.c
void deathmatchscoreboardmsg(gentity_t *ent);

// g_main.c
void		clientintermission(gentity_t *ent);
void		findintermissionpoint(void);
void		setleader(int team, int client);
void		chkteamleader(int team);
void		runthink(gentity_t *ent);
void		addduelqueue(gclient_t *client);
void		beginroundwarmup(void);
qboolean	inmatchwarmup(void);
qboolean	inroundwarmup(void);
qboolean	inwarmup(void);
void QDECL	logprintf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void		sendscoreboard(void);
void QDECL	gprintf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void QDECL	errorf(const char *fmt, ...) __attribute__ ((noreturn, format(printf, 1, 2)));

// g_client.c
char *	clientconnect(int clientNum, qboolean firstTime, qboolean isBot);
void	clientuserinfochanged(int clientNum);
void	clientdisconnect(int clientNum);
void	clientbegin(int clientNum);
void	clientcmd(int clientNum);

// g_active.c
void	clientthink(int clientNum);
void	clientendframe(gentity_t *ent);
void	runclient(gentity_t *ent);

// g_team.c
qboolean	onsameteam(gentity_t *ent1, gentity_t *ent2);
void		ckhdroppedteamitem(gentity_t *dropped);
qboolean	chkobeliskattacked(gentity_t *obelisk, gentity_t *attacker);
int		numclientsoncp(const gentity_t *cp, int *red, int *blue);
int		clientsoncp(const gentity_t *cp, gentity_t **clients, int nclients);

// g_mem.c
void *	alloc(int size);
void	initmem(void);
void	Svcmd_GameMem_f(void);

// g_session.c
void	sessread(gclient_t *client);
void	sessinit(gclient_t *client, char *userinfo);

void	worldsessinit(void);
void	sesswrite(void);

// g_arenas.c
void	updateduel(void);
void	spawnonvictorypads(void);
void	Svcmd_AbortPodium_f(void);

// g_bot.c
void		initbots(qboolean restart);
char *		botinfo(int num);
char *		botinfobyname(const char *name);
void		chkbotspawn(void);
void		dequeuebotbegin(int clientNum);
qboolean	botconnect(int clientNum, qboolean restart);
void		Svcmd_AddBot_f(void);
void		Svcmd_BotList_f(void);
void		botinterbreed(void);

// ai_main.c
#define MAX_FILEPATH 144

//bot settings
typedef struct bot_settings_s
{
	char	characterfile[MAX_FILEPATH];
	float	skill;
	char	team[MAX_FILEPATH];
} bot_settings_t;

int	BotAISetup(int restart);
int	BotAIShutdown(int restart);
int	BotAILoadMap(int restart);
int	BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int	BotAIShutdownClient(int client, qboolean restart);
int	BotAIStartFrame(int time);
void	BotTestAAS(vec3_t origin);

#include "g_team.h"	// teamplay specific stuff

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define FOFS(x) ((size_t)&(((gentity_t*)0)->x))

extern vmCvar_t g_gametype;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;		// allow this many total, including spectators
extern vmCvar_t g_maxGameClients;	// allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_airAccel;
extern vmCvar_t g_airFriction;
extern vmCvar_t g_airIdleFriction;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_roundwarmup;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_blood;
extern vmCvar_t g_allowVote;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_obeliskHealth;
extern vmCvar_t g_obeliskRegenPeriod;
extern vmCvar_t g_obeliskRegenAmount;
extern vmCvar_t g_obeliskRespawnDelay;
extern vmCvar_t g_cubeTimeout;
extern vmCvar_t g_redteam;
extern vmCvar_t g_blueteam;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t g_rankings;
extern vmCvar_t g_enableDust;
extern vmCvar_t g_enableBreath;
extern vmCvar_t g_singlePlayer;
extern vmCvar_t g_proxMineTimeout;
extern vmCvar_t g_homingTracking;
extern vmCvar_t g_homingVariation;
extern vmCvar_t g_homingDivergenceProb;
extern vmCvar_t g_homingDivergence;
extern vmCvar_t g_homingPerfectDist;
extern vmCvar_t g_homingCone;
extern vmCvar_t g_homingPerfectCone;
extern vmCvar_t g_homingScanRange;
extern vmCvar_t g_homingLaunchAngle;
extern vmCvar_t g_homingCount;
extern vmCvar_t g_homingThinkWait;
extern vmCvar_t g_homingSpeed;
extern vmCvar_t g_homingAccel;
extern vmCvar_t g_homingDmg;
extern vmCvar_t g_homingSplashDmg;
extern vmCvar_t g_homingSplashRadius;
extern vmCvar_t g_rocketSpeed;
extern vmCvar_t g_rocketAccel;
extern vmCvar_t g_rocketDmg;
extern vmCvar_t g_rocketSplashDmg;
extern vmCvar_t g_rocketSplashRadius;
extern vmCvar_t g_plasmaSpeed;
extern vmCvar_t g_plasmaDmg;
extern vmCvar_t g_plasmaSplashDmg;
extern vmCvar_t g_plasmaSplashRadius;
extern vmCvar_t g_minigunSpeed;
extern vmCvar_t g_minigunDmg;
extern vmCvar_t g_minigunSpread;
extern vmCvar_t g_grenadeSpeed;
extern vmCvar_t g_grenadeDmg;
extern vmCvar_t g_grenadeSplashDmg;
extern vmCvar_t g_grenadeSplashRadius;

void		trap_Print(const char *text);
void		trap_Error(const char *text) __attribute__((noreturn));
int		trap_Milliseconds(void);
int		trap_RealTime(qtime_t *qtime);
int		trap_Argc(void);
void		trap_Argv(int n, char *buffer, int bufferLength);
void		trap_Args(char *buffer, int bufferLength);
int		trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void		trap_FS_Read(void *buffer, int len, fileHandle_t f);
void		trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void		trap_FS_FCloseFile(fileHandle_t f);
int		trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int		trap_FS_Seek(fileHandle_t f, long offset, int origin);	// fsOrigin_t
void		trap_SendConsoleCommand(int exec_when, const char *text);
void		trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags);
void		trap_Cvar_Update(vmCvar_t *cvar);
void		trap_Cvar_Set(const char *var_name, const char *value);
void		trap_Cvar_SetDescription(vmCvar_t *cv, const char *desc);
int		trap_Cvar_VariableIntegerValue(const char *var_name);
float		trap_Cvar_VariableValue(const char *var_name);
void		trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void		trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient);
void		trap_DropClient(int clientNum, const char *reason);
void		trap_SendServerCommand(int clientNum, const char *text);
void		trap_SetConfigstring(int num, const char *string);
void		trap_GetConfigstring(int num, char *buffer, int bufferSize);
void		trap_GetUserinfo(int num, char *buffer, int bufferSize);
void		trap_SetUserinfo(int num, const char *buffer);
void		trap_GetServerinfo(char *buffer, int bufferSize);
void		trap_SetBrushModel(gentity_t *ent, const char *name);
void		trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int		trap_PointContents(const vec3_t point, int passEntityNum);
qboolean	trap_InPVS(const vec3_t p1, const vec3_t p2);
qboolean	trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void		trap_AdjustAreaPortalState(gentity_t *ent, qboolean open);
qboolean	trap_AreasConnected(int area1, int area2);
void		trap_LinkEntity(gentity_t *ent);
void		trap_UnlinkEntity(gentity_t *ent);
int		trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
qboolean	trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);
int		trap_BotAllocateClient(void);
void		trap_BotFreeClient(int clientNum);
void		trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean	trap_GetEntityToken(char *buffer, int bufferSize);
void		trap_StatAdd(int clientnum, int stat, int incr);

int		trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void		trap_DebugPolygonDelete(int id);

int		trap_BotLibSetup(void);
int		trap_BotLibShutdown(void);
int		trap_BotLibVarSet(char *var_name, char *value);
int		trap_BotLibVarGet(char *var_name, char *value, int size);
int		trap_BotLibDefine(char *string);
int		trap_BotLibStartFrame(float time);
int		trap_BotLibLoadMap(const char *mapname);
int		trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int		trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int		trap_BotGetSnapshotEntity(int clientNum, int sequence);
int		trap_BotGetServerCommand(int clientNum, char *message, int size);
void		trap_BotUserCommand(int client, usercmd_t *ucmd);

int		trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int		trap_AAS_AreaInfo(int areanum, void /* struct aas_areainfo_s */ *info);
void		trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int		trap_AAS_Initialized(void);
void		trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float		trap_AAS_Time(void);

int		trap_AAS_PointAreaNum(vec3_t point);
int		trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int		trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int		trap_AAS_PointContents(vec3_t point);
int		trap_AAS_NextBSPEntity(int ent);
int		trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int		trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int		trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int		trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int		trap_AAS_AreaReachability(int areanum);

int		trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int		trap_AAS_EnableRoutingArea(int areanum, int enable);
int		trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
				      int goalareanum, int travelflags, int maxareas, int maxtime,
				      int stopevent, int stopcontents, int stoptfl, int stopareanum);

int	trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
				       void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
				       int type);
int	trap_AAS_Swimming(vec3_t origin);
int	trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);

void	trap_EA_Say(int client, char *str);
void	trap_EA_SayTeam(int client, char *str);
void	trap_EA_Command(int client, char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, vec3_t dir, float speed);
void	trap_EA_View(int client, vec3_t viewangles);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);

int	trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min, float max);
int	trap_Characteristic_Integer(int character, int index);
int	trap_Characteristic_BInteger(int character, int index, int min, int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int	trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int	trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int	trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
int	trap_BotNumInitialChats(int chatstate, char *type);
int	trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
int	trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int	trap_StringContains(char *str1, char *str2, int casesensitive);
int	trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, ulong context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, ulong context);
int	trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void	trap_BotSetChatGender(int chatstate, int gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int	trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int	trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int	trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int	trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int	trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int	trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int	trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int	trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int	trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int	trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int	trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void	/* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int	trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int	trap_BotReachabilityArea(vec3_t origin, int testground);
int	trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int	trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void	/* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int	trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int	trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon, void	/* struct weaponinfo_s */ *weaponinfo);
int	trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int	trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int	trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

void	trap_SnapVector(float *v);
