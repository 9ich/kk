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
Connection screen
*/

qboolean passwordNeeded = qfalse;

static connstate_t lastConnState;
static char lastLoadingText[MAX_INFO_VALUE];

static void
readablesize(char *buf, int bufsize, int value)
{
	if(value > 1024*1024*1024){	// gigs
		Com_sprintf(buf, bufsize, "%d", value / (1024*1024*1024));
		Com_sprintf(buf+strlen(buf), bufsize-strlen(buf), ".%02d GB",
			    (value % (1024*1024*1024))*100 / (1024*1024*1024));
	}else if(value > 1024*1024){	// megs
		Com_sprintf(buf, bufsize, "%d", value / (1024*1024));
		Com_sprintf(buf+strlen(buf), bufsize-strlen(buf), ".%02d MB",
			    (value % (1024*1024))*100 / (1024*1024));
	}else if(value > 1024)	// kilos
		Com_sprintf(buf, bufsize, "%d KB", value / 1024);
	else	// bytes
		Com_sprintf(buf, bufsize, "%d bytes", value);
}

// Assumes time is in msec
static void
printtime(char *buf, int bufsize, int time)
{
	time /= 1000;	// change to seconds

	if(time > 3600)	// in the hours range
		Com_sprintf(buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60);
	else if(time > 60)	// mins
		Com_sprintf(buf, bufsize, "%d min %d sec", time / 60, time % 60);
	else	// secs
		Com_sprintf(buf, bufsize, "%d sec", time);
}

static void
displaydownloadinfo(const char *downloadName)
{
	static char dlText[] = "Downloading:";
	static char etaText[] = "Estimated time left:";
	static char xferText[] = "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int width, leftWidth;
	const char *s;

	downloadSize = trap_Cvar_VariableValue("cl_downloadSize");
	downloadCount = trap_Cvar_VariableValue("cl_downloadCount");
	downloadTime = trap_Cvar_VariableValue("cl_downloadTime");

	leftWidth = stringwidth(dlText, FONT1, 16, 0, -1);
	width = stringwidth(etaText, FONT1, 16, 0, -1);
	if(width > leftWidth)
		leftWidth = width;
	width = stringwidth(xferText, FONT1, 16, 0, -1);
	if(width > leftWidth)
		leftWidth = width;
	leftWidth += 16;

	drawstring(centerleft(), 128, dlText, FONT1, 16, CText);
	drawstring(centerleft(), 160, etaText, FONT1, 16, CText);
	drawstring(centerleft(), 224, xferText, FONT1, 16, CText);

	if(downloadSize > 0)
		s = va("%s (%d%%)", downloadName, (int)((float)downloadCount * 100.0f / downloadSize));
	else
		s = downloadName;

	//drawpropstr(leftWidth, 128, s, style, CWhite);

	readablesize(dlSizeBuf, sizeof dlSizeBuf, downloadCount);
	readablesize(totalSizeBuf, sizeof totalSizeBuf, downloadSize);

	if(downloadCount < 4096 || !downloadTime){
		//drawpropstr(leftWidth, 160, "estimating", style, CWhite);
		//drawpropstr(leftWidth, 192,
		//	    va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, CWhite);
	}else{
		if((uis.realtime - downloadTime) / 1000)
			xferRate = downloadCount / ((uis.realtime - downloadTime) / 1000);
			//xferRate = (int)( ((float)downloadCount) / elapsedTime);
		else
			xferRate = 0;

		readablesize(xferRateBuf, sizeof xferRateBuf, xferRate);

		// Extrapolate estimated completion time
		if(downloadSize && xferRate){
			int n = downloadSize / xferRate;// estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			n = (n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000;

			printtime(dlTimeBuf, sizeof dlTimeBuf, n);
			//(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			//drawpropstr(leftWidth, 160,
			//	    dlTimeBuf, style, CWhite);
			//drawpropstr(leftWidth, 192,
			//	    va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, CWhite);
		}else{
			//drawpropstr(leftWidth, 160,
			//	    "estimating", style, CWhite);
			if(downloadSize)
				;
				//drawpropstr(leftWidth, 192,
				//	    va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, CWhite);
			else
				;
				//drawpropstr(leftWidth, 192,
				//	    va("(%s copied)", dlSizeBuf), style, CWhite);
		}

		if(xferRate)
			;
			//drawpropstr(leftWidth, 224,
			//	    va("%s/Sec", xferRateBuf), style, CWhite);
	}
}

/*
This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
*/
void
drawconnectscreen(qboolean overlay)
{
	uiClientState_t cstate;
	char info[MAX_INFO_VALUE];
	char s[MAX_STRING_CHARS];

	if(uis.keys[K_ESCAPE]){
		trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
		return;
	}

	uis.fullscreen = qfalse;
	drawlibbeginframe(uis.realtime, uis.glconfig.vidWidth, uis.glconfig.vidHeight);

	cacheui();

	if(!overlay){
		setcolour(CWhite);
		drawpic(0, 0, screenwidth(), screenheight(), uis.menuBackShader);
	}

	pushalign("center");

	// see what information we should display
	trap_GetClientState(&cstate);

	info[0] = '\0';
	if(trap_GetConfigString(CS_SERVERINFO, info, sizeof(info))){
		Com_sprintf(s, sizeof s, "Loading %s",
		   Info_ValueForKey(info, "mapname"));
		drawstring(screenwidth()/2, 16, s, FONT1, 32, CWhite);
	}

	Com_sprintf(s, sizeof s, "Connecting to %s", cstate.servername);
	drawstring(screenwidth()/2, 64, s, FONT1, 32, CText);
	drawstring(screenwidth()/2, 96, "Press ESC to abort", FONT1, 32, CText);

	// display global MOTD at bottom
	drawmultiline(centerleft(), screenheight()-32, centerright(),
	   Info_ValueForKey(cstate.updateInfoString, "motd"), FONT2, 16, CText);

	// print any server info (server full, bad version, etc)
	if(cstate.connState < CA_CONNECTED)
		drawmultiline(centerleft(), 192, centerright(), cstate.messageString,
		   FONT2, 16, CText);

	// display password field
	if(passwordNeeded){
	}

	if(lastConnState > cstate.connState)
		lastLoadingText[0] = '\0';
	lastConnState = cstate.connState;

	switch(cstate.connState){
	case CA_CONNECTING:
		Com_sprintf(s, sizeof s, "Awaiting challenge - %i",
		   cstate.connectPacketCount);
		break;
	case CA_CHALLENGING:
		Com_sprintf(s, sizeof s, "Awaiting connection - %i",
		   cstate.connectPacketCount);
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

		trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName,
		   sizeof(downloadName));
		if(*downloadName){
			displaydownloadinfo(downloadName);
			return;
		}
		}
		Q_strncpyz(s, "Awaiting gamestate", sizeof s);
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	drawstring(screenwidth()/2, 128, s, FONT1, 32, CText);

	popalign(1);

	// password required / connection rejected information goes here
}
