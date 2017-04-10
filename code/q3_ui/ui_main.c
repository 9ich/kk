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

#include "ui_local.h"

/*
 * vmMain must be the very first function in the very first C file
 * compiled into the ui.qvm module.
 * 
 * This is the only way control passes into the module.
 */
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch(command){
	case UI_GETAPIVERSION:
		return UI_API_VERSION;
	case UI_INIT:
		init();
		return 0;
	case UI_SHUTDOWN:
		shutdown();
		return 0;
	case UI_KEY_EVENT:
		keyevent(arg0, arg1);
		return 0;
	case UI_CHAR_EVENT:
		charevent(arg0);
		return 0;
	case UI_MOUSE_EVENT:
		mouseevent(arg0, arg1);
		return 0;
	case UI_REFRESH:
		refresh(arg0);
		return 0;
	case UI_IS_FULLSCREEN:
		return uis.fullscreen;
	case UI_SET_ACTIVE_MENU:
		setactivemenu(arg0);
		return 0;
	case UI_CONSOLE_COMMAND:
		return consolecommand(arg0);
	case UI_DRAW_CONNECT_SCREEN:
		drawconnectscreen(arg0);
		return 0;
	case UI_HASUNIQUECDKEY:	// mod authors need to observe this
		return qtrue;	// change this to qfalse for mods!
	}
	return -1;
}

typedef struct
{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
} cvarTable_t;

vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawfps;

static cvarTable_t cvarTable[] = {
	// sets default options if modules have never run before
	{&ui_drawCrosshair, "cg_crosshair", "4", CVAR_ARCHIVE},
	{&ui_drawfps, "cg_drawfps", "1", CVAR_ARCHIVE},
	{nil, "cg_fov", "90", CVAR_ARCHIVE},
	{nil, "cg_thirdperson", "0", CVAR_ARCHIVE}
};

void
registercvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < ARRAY_LEN(cvarTable); i++, cv++)
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString,
		   cv->cvarFlags);
}

void
updatecvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < ARRAY_LEN(cvarTable); i++, cv++){
		if(cv->vmCvar == nil)
			continue;
		trap_Cvar_Update(cv->vmCvar);
	}
}
