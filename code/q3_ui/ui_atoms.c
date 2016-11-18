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

/*
User interface building blocks and support functions.
Coordinates are 640*480 virtual values
*/

#include "ui_local.h"

uiStatic_t uis;
extern vec4_t focusclr;

char *
Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));
	return buffer;
}

void QDECL
Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

char *
UI_Cvar_VariableString(const char *var_name)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(var_name, buffer, sizeof(buffer));
	return buffer;
}

float
UI_ClampCvar(float min, float max, float value)
{
	if(value < min)
		return min;
	if(value > max)
		return max;
	return value;
}

void
startdemoloop(void)
{
	trap_Cmd_ExecuteText(EXEC_APPEND, "d1\n");
}

/*
This should be the ONLY way the menu system is brought up.
*/
void
setactivemenu(uiMenuCommand_t menu)
{
	cacheui();

	switch(menu){
	case UIMENU_NONE:
		dismissui();
		return;
	case UIMENU_MAIN: {
		char buf[MAX_STRING_CHARS];

		uis.sp = -1;
		clearkeys();
		trap_Cvar_VariableStringBuffer("com_errormessage", buf, sizeof buf);
		if(*buf != '\0')
			push(errormenu);
		else
			push(mainmenu);
		return;
	}
	case UIMENU_INGAME:
		trap_Cvar_Set("cl_paused", "1");
		uis.sp = -1;
		push(ingamemenu);
		return;
	default:
#ifndef NDEBUG
		Com_Printf("setactivemenu: bad enum %d\n", menu);
#endif
		break;
	}
}

void
keyevent(int key, int down)
{
	if(key < 0 || key >= MAX_KEYS)
		return;
	uis.keys[key] = down;
	uis.keyshadow[key] = qfalse;
}

void
charevent(int ch)
{
	if(uis.texti < TEXTLEN-2)
		uis.text[uis.texti++] = ch;
	uis.text[TEXTLEN-1] = '\0';
	uis.keys[tolower(ch)] = qtrue;
}

void
mouseevent(int dx, int dy)
{
	uis.cursorx += dx * SCREEN_HEIGHT/uis.glconfig.vidHeight;
	uis.cursory += dy * SCREEN_HEIGHT/uis.glconfig.vidHeight;
	uis.cursorx = Com_Clamp(0, screenwidth(), uis.cursorx);
	uis.cursory = Com_Clamp(0, screenheight(), uis.cursory);
}

void
cacheui(void)
{
	uis.cursor = trap_R_RegisterShaderNoMip("menu/art/3_cursor2");
	uis.menuBackShader = trap_R_RegisterShaderNoMip("menuback");
	uis.fieldUpdateSound = trap_S_RegisterSound("sound/misc/menu2", qfalse);
	// precache shaders
	trap_R_RegisterShaderNoMip("menu/art/tick");
	trap_R_RegisterShaderNoMip("menu/art/left");
	trap_R_RegisterShaderNoMip("menu/art/right");
	drawlibinit();
}

qboolean
consolecommand(int realTime)
{
	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;

	return qfalse;
}

void
shutdown(void)
{
}

void
init(void)
{
	registercvars();

	trap_GetGlconfig(&uis.glconfig);

	// for 640x480 virtualized screen
	uis.xscale = uis.glconfig.vidWidth * (1.0/640.0);
	uis.yscale = uis.glconfig.vidHeight * (1.0/480.0);
	if(uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640){
		// widescreen
		uis.bias = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0/480.0)));
		uis.xscale = uis.yscale;
	}else
		uis.bias = 0;
	cacheui();
	uis.firstdraw = qtrue;
	uis.sp = -1;
	uis.fullscreen = qfalse;
}

static void
drawfps(void)
{
	enum {FRAMES = 4};
	static int prevtimes[FRAMES], index, previous;
	int i, total, t, frametime, fps;
	char *s;

	t = trap_Milliseconds();
	frametime = t - previous;
	previous = t;
	prevtimes[index % FRAMES] = frametime;
	index++;
	if(index > FRAMES){
		total = 0;
		for(i = 0; i < FRAMES; i++)
			total += prevtimes[i];
		if(total <= 0)
			total = 1;
		fps = 1000 * FRAMES / total;
		setalign("right");
		s = va("%i", fps);
		drawstring(screenwidth() - 2, 2, s, FONT4, 16, CText);
		setalign("");
	}
}

void
push(void (*drawfunc)(void))
{
	if(uis.sp >= NSTACK)
		Com_Error(ERR_FATAL, "ui stack overflow");
	uis.sp++;
	uis.stk[uis.sp] = drawfunc;
	trap_Key_SetCatcher(KEYCATCH_UI);
	clearfocus();
}

void
pop(void)
{
	if(uis.sp < 0)
		Com_Error(ERR_FATAL, "ui stack underflow");
	uis.sp--;
	if(uis.sp < 0)
		dismissui();
	clearfocus();
}

void
dismissui(void)
{
	uis.sp = -1;
	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
	trap_Cvar_Set("cl_paused", 0);
	uis.fullscreen = qfalse;
}

void
refresh(int realtime)
{
	uis.frametime = realtime - uis.realtime;
	uis.realtime = realtime;

	if(!(trap_Key_GetCatcher() & KEYCATCH_UI)){
		uis.fullscreen = qfalse;
		return;
	}

	drawlibbeginframe(realtime, uis.glconfig.vidWidth, uis.glconfig.vidHeight);
	Vector4Copy(CWFocus, focusclr);
	focusclr[3] = cos((float)(uis.realtime%1000) / 1000);	// pulse
	focusclr[3] = 0.5f + (0.5f*(1+sin(M_TAU * 3 * (float)(uis.realtime%1000) / 1000)) * 0.5f);

	if(uis.firstdraw){
		uis.cursorx = 0.5f*screenwidth();
		uis.cursory = 0.5f*screenheight();
		uis.firstdraw = qfalse;
	}
	uis.fullscreen = qfalse;

	updatecvars();
	clearfocuslist();

	if(uis.sp >= 0)
		uis.stk[uis.sp]();

	if(ui_drawfps.integer)
		drawfps();

	// draw cursor
	setcolour(nil);
	setalign("mid center");
	drawpic(uis.cursorx, uis.cursory, 32, 32, uis.cursor);

	setalign("topleft");
	if(0 || uis.debug)
		// cursor coordinates
		drawstring(0, 0, va("(%.2f,%.2f)", uis.cursorx, uis.cursory),
		   FONT2, 16, CMagenta);
	setalign("");

	// finish the frame
	if(!uis.keys[K_MOUSE1])
		Q_strncpyz(uis.active, "", sizeof uis.active);
	memset(uis.text, 0, TEXTLEN);
	memset(uis.keys+'0', 0, '9'-'0');
	memset(uis.keys+'A', 0, 'Z'-'A');
	memset(uis.keyshadow+'0', 0, '9'-'0');
	memset(uis.keyshadow+'A', 0, 'Z'-'A');
	uis.texti = 0;
	cyclefocus();
}

qboolean
mouseover(float x, float y, float w, float h)
{
	aligncoords(&x, &y, &w, &h);

	if(uis.cursorx < x || uis.cursory < y ||
	   uis.cursorx > x+w || uis.cursory > y+h)
		return qfalse;
	return qtrue;
}

/*
Appends whitespace-separated widget ids to focus order list.
Cleared every frame.
*/
void
focusorder(const char *s)
{
	char *p, buf[1024], *tok;

	Q_strncpyz(buf, s, sizeof buf);
	p = buf;
	for(; uis.nfoc < ARRAY_LEN(uis.foclist); uis.nfoc++){
		tok = COM_Parse(&p);
		if(*tok == '\0')
			break;
		Q_strncpyz(uis.foclist[uis.nfoc], tok, sizeof uis.foclist[uis.nfoc]);
	}
}

void
setfocus(const char *id)
{
	int i;

	Q_strncpyz(uis.focus, id, sizeof uis.focus);
	for(i = 0; i < uis.nfoc; i++)
		if(strcmp(uis.foclist[i], uis.focus) == 0){
			uis.foci = i;
			break;
		}
}

/*
Sets the default focus.  Persists over frames.
*/
void
defaultfocus(const char *id)
{
	if(*uis.defaultfocus != '\0')
		return;
	Q_strncpyz(uis.defaultfocus, id, sizeof uis.defaultfocus);
	setfocus(id);
}

void
cyclefocus(void)
{
	if(keydown(K_UPARROW)){
		uis.foci = (uis.foci-1 >= 0)? uis.foci-1 : uis.nfoc-1;
		Q_strncpyz(uis.focus, uis.foclist[uis.foci], sizeof uis.focus);
	}else if(keydown(K_DOWNARROW)){
		uis.foci = (uis.foci+1) % uis.nfoc;
		Q_strncpyz(uis.focus, uis.foclist[uis.foci], sizeof uis.focus);
	}
}

/*
Clears focus list for beginning of frame.
*/
void
clearfocuslist(void)
{
	uis.nfoc = 0;
}

/*
Clears focus for transition between menus.
*/
void
clearfocus(void)
{
	uis.foci = 0;
	*uis.focus = '\0';
	*uis.defaultfocus = '\0';
}

/*
Returns whether there is a pending keypress on k this frame.  A key is
no longer pending when its state has been set qtrue in the uis.keyshadow
array.
*/
qboolean
keydown(int k)
{
	if(uis.keyshadow[k])
		return qfalse;
	if(uis.keys[k])
		uis.keyshadow[k] = qtrue;
	return uis.keys[k];
}

void
clearkeys(void)
{
	memset(uis.keys, 0, sizeof uis.keys);
	memset(uis.keyshadow, 0, sizeof uis.keyshadow);
}
