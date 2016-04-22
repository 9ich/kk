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
#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../ui/ui_public.h"
#include "../drawlib/d_public.h"
#include "../client/keycodes.h"

extern vmCvar_t ui_drawCrosshair;
extern vmCvar_t ui_drawCrosshairNames;
extern vmCvar_t ui_drawfps;

// ui_main.c
extern void	cacheui(void);
extern void	registercvars(void);
extern void	updatecvars(void);

// ui_connect.c
extern void	drawconnectscreen(qboolean overlay);

// ui_atoms.c
enum
{
	IDLEN		= 64,
	TEXTLEN		= 64,
	NSTACK		= 32,
	NFOCLIST	= 128
};

typedef struct
{
	int		frametime;
	int		realtime;
	float		cursorx;
	float		cursory;
	glconfig_t	glconfig;
	qboolean	debug;
	qboolean	fullscreen;
	qboolean	keys[MAX_KEYS];		// keys down this refresh
	qboolean	keyshadow[MAX_KEYS];	// keys processed this refresh
	char		text[TEXTLEN];		// text entered this refresh
	int		texti;			// text index
	char		hot[IDLEN];		// id hovered over
	char		active[IDLEN];		// id being clicked (and held) on
	char		focus[IDLEN];		// id with keyboard focus
	char		defaultfocus[IDLEN];
	char		foclist[NFOCLIST][IDLEN];	// per-frame list of explicit focus order
	int		foci;			// foclist index
	int		nfoc;			// foclist len
	void(*stk[NSTACK])(void);		// menu drawing stack
	int		sp;			// stack pointer

	qhandle_t	cursor;
	qhandle_t	menuBackShader;
	sfxHandle_t	fieldUpdateSound;
	float		xscale;
	float		yscale;
	float		bias;
	qboolean	demoversion;
	qboolean	firstdraw;
} uiStatic_t;

extern void	init(void);
extern void	shutdown(void);
extern void	keyevent(int key, int down);
extern void	charevent(int key);
extern void	mouseevent(int dx, int dy);
extern void	refresh(int realtime);
extern qboolean consolecommand(int realTime);
extern float	UI_ClampCvar(float min, float max, float value);
extern qboolean mouseover(float x, float y, float width, float height);
extern void	setactivemenu(uiMenuCommand_t menu);
extern void	push(void (*drawfunc)(void));
extern void	pop(void);
extern void	dismissui(void);
extern void	focusorder(const char *s);
extern void	setfocus(const char *id);
extern void	defaultfocus(const char *id);
extern void	cyclefocus(void);
extern void	clearfocuslist(void);
extern void	clearfocus(void);
extern qboolean	keydown(int k);
extern void	clearkeys(void);
extern char	*Argv(int arg);
extern char	*UI_Cvar_VariableString(const char *var_name);
extern void	refresh(int time);
extern void	startdemoloop(void);

extern uiStatic_t uis;

// ui_widgets.c
qboolean	button(const char *id, int x, int y, const char *label);
qboolean	checkbox(const char *id, int x, int y, qboolean *state);
qboolean	slider(const char *id, int x, int y, float min, float max, float *val, const char *displayfmt);
qboolean	textfield(const char *id, int x, int y, int width, char *buf, int *caret, int sz);
qboolean	textspinner(const char *id, int x, int y, char **opts, int *i, int nopts);
qboolean	keybinder(const char *id, int x, int y, int key);

// ui_menus.c
void		mainmenu(void);
void		ingamemenu(void);
void		quitmenu(void);
void		videomenu(void);
void		soundmenu(void);
void		inputmenu(void);
void		defaultsmenu(void);
void		errormenu(void);
void		placeholder(void);

// ui_syscalls.c
void		trap_Print(const char *string);
void		trap_Error(const char *string) __attribute__((noreturn));
int		trap_Milliseconds(void);
void		trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void		trap_Cvar_Update(vmCvar_t *vmCvar);
void		trap_Cvar_Set(const char *var_name, const char *value);
float		trap_Cvar_VariableValue(const char *var_name);
void		trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void		trap_Cvar_SetValue(const char *var_name, float value);
void		trap_Cvar_Reset(const char *name);
void		trap_Cvar_Create(const char *var_name, const char *var_value, int flags);
void		trap_Cvar_InfoStringBuffer(int bit, char *buffer, int bufsize);
int		trap_Argc(void);
void		trap_Argv(int n, char *buffer, int bufferLength);
void		trap_Cmd_ExecuteText(int exec_when, const char *text);	// don't use EXEC_NOW!
int		trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void		trap_FS_Read(void *buffer, int len, fileHandle_t f);
void		trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void		trap_FS_FCloseFile(fileHandle_t f);
int		trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int		trap_FS_Seek(fileHandle_t f, long offset, int origin);	// fsOrigin_t
qhandle_t	trap_R_RegisterModel(const char *name);
qhandle_t	trap_R_RegisterSkin(const char *name);
qhandle_t	trap_R_RegisterShaderNoMip(const char *name);
void		trap_R_ClearScene(void);
void		trap_R_AddRefEntityToScene(const refEntity_t *re);
void		trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void		trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void		trap_R_RenderScene(const refdef_t *fd);
void		trap_R_SetColor(const float *rgba);
void		trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
void		trap_UpdateScreen(void);
int		trap_CM_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName);
void		trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
sfxHandle_t	trap_S_RegisterSound(const char *sample, qboolean compressed);
void		trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
void		trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
void		trap_Key_SetBinding(int keynum, const char *binding);
qboolean	trap_Key_IsDown(int keynum);
qboolean	trap_Key_GetOverstrikeMode(void);
void		trap_Key_SetOverstrikeMode(qboolean state);
void		trap_Key_ClearStates(void);
int		trap_Key_GetCatcher(void);
void		trap_Key_SetCatcher(int catcher);
void		trap_GetClipboardData(char *buf, int bufsize);
void		trap_GetClientState(uiClientState_t *state);
void		trap_GetGlconfig(glconfig_t *glconfig);
int		trap_GetConfigString(int index, char *buff, int buffsize);
int		trap_LAN_GetServerCount(int source);
void		trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen);
void		trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen);
int		trap_LAN_GetPingQueueCount(void);
int		trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen);
void		trap_LAN_ClearPing(int n);
void		trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime);
void		trap_LAN_GetPingInfo(int n, char *buf, int buflen);
int		trap_MemoryRemaining(void);
void		trap_GetCDKey(char *buf, int buflen);
void		trap_SetCDKey(char *buf);

qboolean	trap_VerifyCDKey(const char *key, const char *chksum);

void		trap_SetPbClStatus(int status);
#endif
