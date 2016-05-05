#include "ui_local.h"

enum
{
	NRES	= 128,
	MARGIN	= 30
};

static char *fallbackres[] = {
	"320x240",
	"400x300",
	"512x384",
	"640x480",
	"800x600",
	"960x720",
	"1024x768",
	"1152x864",
	"1280x1024",
	"1600x1200",
	"2048x1536",
	"856x480",
	nil
};

static char *fallbackhz[] = {
	"60",
	"75",
	"85",
	"100",
	"120",
	"144",
	"160"
};

static struct
{
	float	r;
	char	*s;
} knownratios[] = {
	{4/3.0f,	"4:3"},
	{16/10.0f,	"16:10"},
	{16/9.0f,	"16:9"},
	{21/9.0f,	"21:9"},
	{5/4.0f,	"5:4"},
	{3/2.0f,	"3:2"},
	{14/9.0f,	"14:9"},
	{5/3.0f,	"5:3"}
};

static struct
{
	int	k, alt;
	char	*name;
	char	*cmd;
} binds[] = {
	// these are the defaults, overridden by initinputmenu
	{'w',		K_UPARROW,	"Forward",	"+forward"},
	{'s',		K_DOWNARROW,	"Back",		"+back"},
	{'a',		-1,		"Left",		"+moveleft"},
	{'d',		-1,		"Right",	"+moveright"},
	{K_SPACE,	-1,		"Up",		"+moveup"},
	{K_SHIFT,	-1,		"Down",		"+movedown"},
	{'q',		-1,		"Roll left",	"+rollleft"},
	{'e',		-1,		"Roll right",	"+rollright"},
	{K_LEFTARROW,	-1,		"Turn left",	"+left"},
	{K_RIGHTARROW,	-1,		"Turn right",	"+right"},
	{K_MOUSE1,	-1,		"Attack",	"+attack"},
	{K_TAB,		-1,		"Scoreboard",	"+scores"},
	{K_MOUSE3,	-1,		"Zoom",		"+zoom"},
	{'y',		-1,		"Chat",		"messagemode"},
	{'t',		-1,		"Team chat",	"messagemode2"},
	{0}
};

static char *texqualities[] = {"Pretty Nice", "Gravel"};
static char *geomqualities[] = {"Low", "High"};
static char *sndqualities[] = {"Default", "Low", "Normal", "Silly"};

static char *ratios[NRES];
static char ratiobuf[NRES][16];
static char resbuf[MAX_STRING_CHARS];
static char *detectedres[NRES];
static char *detectedhz[NRES];
static char **resolutions = fallbackres;
static char **refreshrates = fallbackhz;

// video options
static struct
{
	qboolean	initialized, dirty, needrestart;
	qboolean	usedesktopres;
	char		*reslist[NRES];
	int		nres, resi;
	char		*ratlist[ARRAY_LEN(knownratios) + 1];
	int		nrat, rati;
	char		*hzlist[NRES];
	int		nhz, hzi;
	qboolean	fullscr;
	qboolean	vsync;
	float		fov;
	qboolean	drawfps;
	qboolean	thirdperson;
	int		texquality;
	int		gquality;
	float		gamma;
} vo;

// sound options
static struct
{
	qboolean	initialized, dirty, needrestart;
	int		qual;
	float		vol;
	float		muvol;
	qboolean	doppler;
} so;

// input options (see also binds)
static struct
{
	qboolean	initialized, dirty, needrestart;
	float		sens;
} io;

// waiting-for-user-to-press-key-to-bind menu
static struct
{
	int	i;		// binds[i]
	int	whichkey;	// 0 = binds[i].k, 1 = binds[i].alt
} bw;

static void
menubackground(void)
{
	vec4_t clr;

	//drawpic(0, 0, screenwidth(), screenheight(), uis.menuBackShader);
	colormix(CSlateBlue, CBlack, clr);
	fillrect(0, 0, screenwidth(), screenheight(), clr);
}

static void
optionsbuttons(void)
{
	const float spc = 35;
	float x, y;

	focusorder(".o.v .o.s .o.i .o.d .o.bk");

	setalign("left");
	x = MARGIN;
	y = 160;
	if(button(".o.v", x, y, "Video")){
		pop();
		vo.initialized = qfalse;
		push(videomenu);
	}
	y += spc;
	if(button(".o.s", x, y, "Sound")){
		pop();
		so.initialized = qfalse;
		push(soundmenu);
	}
	y += spc;
	if(button(".o.i", x, y, "Input")){
		pop();
		io.initialized = qfalse;
		push(inputmenu);
	}
	y += spc;
	if(button(".o.d", x, y, "Defaults"))
		push(defaultsmenu);
	y += spc;

	setalign("");

	pushalign("bottomleft");
	// escape can also take us back
	if(keydown(K_ESCAPE) || button(".o.bk", 10, screenheight()-10, "Back")){
		vo.initialized = qfalse;
		so.initialized = qfalse;
		io.initialized = qfalse;
		pop();
	}
	popalign(1);
}

static void
optionname(const char *name, float y)
{
	pushalign("right");
	drawstring(420, y, name, FONT3, 20, CText);
	popalign(1);
}

static qboolean
acceptbutton(const char *id)
{
	qboolean clicked;

	focusorder(id);
	pushalign("bottomright");
	clicked = button(id, screenwidth()-10, screenheight()-10, "Accept");
	popalign(1);
	return clicked;
}

static void
getmodes(void)
{
	int i;
	char *p;

	Q_strncpyz(resbuf, UI_Cvar_VariableString("r_availablemodes"), sizeof resbuf);
	if(*resbuf == '\0')
		return;
	for(p = resbuf, i = 0; p != nil && i < ARRAY_LEN(detectedres)-1; i++){
		// XxY
		detectedres[i] = p;

		// @Hz
		p = strchr(p, '@');
		*p++ = '\0';
		detectedhz[i] = p;

		// next
		p = strchr(p, ' ');
		if(p != nil)
			*p++ = '\0';
	}
	detectedres[i] = nil;
	detectedhz[i] = nil;
	if(i > 0){
		resolutions = detectedres;
		refreshrates = detectedhz;
	}
}

static void
calcratios(void)
{
	int i, r;

	for(r = 0; resolutions[r]; r++){
		float w, h;
		char *x, str[sizeof ratiobuf[0]];

		// calculate resolution's aspect ratio
		x = strchr(resolutions[r], 'x') + 1;
		Q_strncpyz(str, resolutions[r], x-resolutions[r]);
		w = (float)atoi(str);
		h = (float)atoi(x);
		str[0] = '\0';
		for(i = 0; i < ARRAY_LEN(knownratios); i++){
			// close enough?
			if(fabs(knownratios[i].r - w/h) < 0.02f){
				Q_strncpyz(str, knownratios[i].s, sizeof str);
				break;
			}
		}
		// unrecognized ratio?
		if(*str == '\0')
			Com_sprintf(str, sizeof str, "%.2f:1", w/h);
		Q_strncpyz(ratiobuf[r], str, sizeof ratiobuf[r]);
		ratios[r] = ratiobuf[r];
	}
	ratios[r] = nil;
}

void
placeholder(void)
{
	focusorder(".p.b");
	defaultfocus(".p.b");

	uis.fullscreen = qtrue;
	menubackground();
	pushalign("center");
	if(keydown(K_ESCAPE) || button(".p.b", screenwidth()/2, 210, "Go away"))
		pop();
	popalign(1);
}

void
quitmenu(void)
{
	focusorder(".qm.yes .qm.no");
	defaultfocus(".qm.yes");

	uis.fullscreen = qtrue;
	menubackground();

	pushalign("right");
	if(button(".qm.yes", screenwidth()/2 - 20, 210, "Quit"))
		trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
	pushalign("left");
	if(keydown(K_ESCAPE) || button(".qm.no", screenwidth()/2 + 20, 210, "Cancel"))
		pop();
	popalign(2);
}

// video options

/*
Builds list of supported resolutions & refresh rates for the current aspect ratio.
*/
static void
mkmodelists(const char *ratio)
{
	int i, j;

	vo.nres = vo.nhz = 0;
	memset(vo.reslist, 0, sizeof vo.reslist);
	memset(vo.hzlist, 0, sizeof vo.hzlist);
	for(i = 0; resolutions[i] != nil; i++){
		if(Q_stricmp(ratios[i], ratio) != 0)
			continue;	// res not in this aspect ratio

		for(j = 0; j < vo.nres; j++)
			if(Q_stricmp(vo.reslist[j], resolutions[i]) == 0)
				break;	// res already in list
		if(j == vo.nres)
			vo.reslist[vo.nres++] = resolutions[i];

		for(j = 0; j < vo.nhz; j++)
			if(Q_stricmp(vo.hzlist[j], refreshrates[i]) == 0)
				break;	// refresh rate already in list
		if(j == vo.nhz)
			vo.hzlist[vo.nhz++] = refreshrates[i];
	}
}

/*
Builds list of aspect ratios
*/
static void
mkratlist(void)
{
	int i, j;

	for(i = 0; ratios[i] != nil; i++){
		for(j = 0; j < vo.nrat; j++)
			if(Q_stricmp(vo.ratlist[j], ratios[i]) == 0)
				break;	// ratio already in list
		if(j == vo.nrat)
				vo.ratlist[vo.nrat++] = ratios[i];
	}
}

static void
initvideomenu(void)
{
	int i, w, h, hz;
	char resstr[16], hzstr[16], *ratio, buf[MAX_STRING_CHARS];

	memset(&vo, 0, sizeof vo);

	vo.texquality = 0;
	trap_Cvar_VariableStringBuffer("r_texturemode", buf, sizeof buf);
	if(Q_stricmp(buf, "gl_nearest") == 0)
		vo.texquality = 1;
	vo.vsync = (qboolean)trap_Cvar_VariableValue("r_swapinterval");
	vo.fov = trap_Cvar_VariableValue("cg_fov");
	vo.drawfps = trap_Cvar_VariableValue("cg_drawfps");
	vo.thirdperson = trap_Cvar_VariableValue("cg_thirdperson");
	vo.fullscr = (qboolean)trap_Cvar_VariableValue("r_fullscreen");
	vo.gamma = trap_Cvar_VariableValue("r_gamma");
	vo.initialized = qtrue;

	// grab the current mode
	if(trap_Cvar_VariableValue("r_mode") == -2){
		vo.usedesktopres = qtrue;
	}

	getmodes();
	calcratios();
	mkratlist();

	w = trap_Cvar_VariableValue("r_customwidth");
	h = trap_Cvar_VariableValue("r_customheight");
	hz = trap_Cvar_VariableValue("r_displayrefresh");
	Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
	Com_sprintf(hzstr, sizeof hzstr, "%d", hz);
	ratio = ratios[0];
	for(i = 0; resolutions[i] != nil; i++)
		if(Q_stricmp(resolutions[i], resstr) == 0)
			ratio = ratios[i];
	mkmodelists(ratio);

	// init menu values
	vo.resi = vo.rati = vo.hzi = 0;
	for(i = 0; vo.reslist[i] != nil; i++)
		if(Q_stricmp(vo.reslist[i], resstr) == 0)
			vo.resi = i;
	for(i = 0; vo.ratlist[i] != nil; i++)
		if(Q_stricmp(vo.ratlist[i], ratio) == 0)
			vo.rati = i;
	for(i = 0; vo.hzlist[i] != nil; i++)
		if(Q_stricmp(vo.hzlist[i], hzstr) == 0)
			vo.hzi = i;
}

static void
savevideochanges(void)
{
	char w[32], *h;

	if(!vo.dirty && !vo.needrestart)
		return;

	if(!vo.usedesktopres){
		h = strchr(vo.reslist[vo.resi], 'x') + 1;
		Q_strncpyz(w, vo.reslist[vo.resi], h-vo.reslist[vo.resi]);
		trap_Cvar_Set("r_customwidth", w);
		trap_Cvar_Set("r_customheight", h);
		trap_Cvar_SetValue("r_mode", -1);
	}else{
		trap_Cvar_SetValue("r_mode", -2);
	}
	trap_Cvar_SetValue("cg_fov", (int)vo.fov);
	trap_Cvar_SetValue("cg_drawfps", (int)vo.drawfps);
	trap_Cvar_SetValue("cg_thirdperson", (int)vo.thirdperson);
	trap_Cvar_SetValue("r_fullscreen", (int)vo.fullscr);
	trap_Cvar_SetValue("r_swapinterval", (int)vo.vsync);
	trap_Cvar_SetValue("r_gamma", vo.gamma);
	if(Q_stricmp(texqualities[vo.texquality], "PS1") == 0)
		trap_Cvar_Set("r_texturemode", "GL_NEAREST");
	else
		trap_Cvar_Set("r_texturemode", "GL_LINEAR_MIPMAP_LINEAR");

	if(vo.needrestart)
		trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
	vo.initialized = qfalse;
}

void
videomenu(void)
{
	const float spc = 24;
	float xx, y;
	int i;

	if(!vo.initialized)
		initvideomenu();

	focusorder(".v.udr .v.res .v.rat .v.hz .v.fs .v.gamma .v.fov .v.vs"
	   " .v.fps .v.tq .v.gq .v.3rd");
	defaultfocus(".v.udr");

	uis.fullscreen = qtrue;
	menubackground();
	optionsbuttons();

	xx = 440;
	y = 100;

	optionname("Use desktop resolution", y);
	if(checkbox(".v.udr", xx, y, &vo.usedesktopres)){
		vo.needrestart = qtrue;
	}
	y += spc;

	if(!vo.usedesktopres){
		optionname("Resolution", y);
		if(textspinner(".v.res", xx, y, vo.reslist, &vo.resi, vo.nres))
		vo.needrestart = qtrue;
		y += spc;

		optionname("Aspect ratio", y);
		if(textspinner(".v.rat", xx, y, vo.ratlist, &vo.rati, vo.nrat)){
			int w, h, hz;
			char resstr[16], hzstr[16];

			vo.needrestart = qtrue;

			// show res & hz combinations for the selected ratio
			mkmodelists(vo.ratlist[vo.rati]);

			// set the textspinner indices
			w = trap_Cvar_VariableValue("r_customwidth");
			h = trap_Cvar_VariableValue("r_customheight");
			hz = trap_Cvar_VariableValue("r_displayrefresh");
			Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
			Com_sprintf(hzstr, sizeof hzstr, "%d", hz);
			vo.resi = vo.hzi = 0;
			for(i = 0; i < vo.nres; i++)
			if(Q_stricmp(vo.reslist[i], resstr) == 0)
					vo.resi = i;
			for(i = 0; i < vo.nhz; i++)
				if(Q_stricmp(vo.hzlist[i], hzstr) == 0)
					vo.hzi = i;
		}
		y += spc;

		optionname("Refresh rate", y);
		if(textspinner(".v.hz", xx, y, vo.hzlist, &vo.hzi, vo.nhz))
			vo.needrestart = qtrue;
		y += spc;
	}

	optionname("Fullscreen", y);
	if(checkbox(".v.fs", xx, y, &vo.fullscr))
		vo.needrestart = qtrue;
	y += spc;

	optionname("Gamma", y);
	if(slider(".v.gamma", xx, y, 0, 4, &vo.gamma, "%.1f"))
		vo.needrestart = qtrue;
	y += spc;

	optionname("Field of view", y);
	if(slider(".v.fov", xx, y, 85, 130, &vo.fov, "%.0f"))
		vo.dirty = qtrue;
	y += spc;

	optionname("Vertical sync", y);
	if(checkbox(".v.vs", xx, y, &vo.vsync))
		vo.needrestart = qtrue;
	y += spc;

	optionname("Show framerate", y);
	if(checkbox(".v.fps", xx, y, &vo.drawfps))
		vo.dirty = qtrue;
	y += spc;

	optionname("Texture quality", y);
	if(textspinner(".v.tq", xx, y, texqualities, &vo.texquality, ARRAY_LEN(texqualities)))
		vo.needrestart = qtrue;
	y += spc;

	optionname("Geometry quality", y);
	if(textspinner(".v.gq", xx, y, geomqualities, &vo.gquality, ARRAY_LEN(geomqualities)))
		vo.dirty = qtrue;
	y += spc;

	if(vo.dirty || vo.needrestart){
		if(acceptbutton(".v.accept")){
			savevideochanges();
			pop();
		}
	}
}

// sound options

static void
initsoundmenu(void)
{
	int freq;

	memset(&so, 0, sizeof so);
	freq = (int)trap_Cvar_VariableValue("s_sdlspeed");
	switch(freq){
	case 11025:
		so.qual = 1;
		break;
	case 22050:
		so.qual = 2;
		break;
	case 44100:
		so.qual = 3;
		break;
	default:
		so.qual = 0;
	}
	so.vol = trap_Cvar_VariableValue("s_volume");
	so.muvol = trap_Cvar_VariableValue("s_musicvolume");
	so.doppler = (qboolean)trap_Cvar_VariableValue("s_doppler");
	so.initialized = qtrue;
}

static void
savesoundchanges(void)
{
	switch(so.qual){
	case 0:
		trap_Cvar_SetValue("s_sdlspeed", 0);
		break;
	case 1:
		trap_Cvar_SetValue("s_sdlspeed", 11025);
		break;
	case 2:
		trap_Cvar_SetValue("s_sdlspeed", 22050);
		break;
	default:
		trap_Cvar_SetValue("s_sdlspeed", 44100);
	}
	trap_Cvar_SetValue("s_doppler", (float)so.doppler);
	if(so.needrestart)
		trap_Cmd_ExecuteText(EXEC_APPEND, "snd_restart");
	so.initialized = qfalse;
}

void
soundmenu(void)
{
	const float spc = 24;
	float xx, y;

	if(!so.initialized)
		initsoundmenu();

	focusorder(".s.qual .s.vol .s.muvol .s.dop");
	defaultfocus(".s.qual");

	uis.fullscreen = qtrue;
	menubackground();
	optionsbuttons();
	xx = 440;
	y = 100;

	optionname("Quality", y);
	if(textspinner(".s.qual", xx, y, sndqualities, &so.qual, ARRAY_LEN(sndqualities)))
		so.needrestart = qtrue;
	y += spc;

	optionname("Master volume", y);
	if(slider(".s.vol", xx, y, 0.0f, 1.0f, &so.vol, "%.2f"))
		trap_Cvar_SetValue("s_volume", so.vol);
	y += spc;

	optionname("Music volume", y);
	if(slider(".s.muvol", xx, y, 0.0f, 1.0f, &so.muvol, "%.2f"))
		trap_Cvar_SetValue("s_musicvolume", so.muvol);
	y += spc;

	optionname("Doppler effect", y);
	if(checkbox(".s.dop", xx, y, &so.doppler))
		so.dirty = qtrue;
	y += spc;

	if(so.dirty || so.needrestart){
		if(acceptbutton("s.accept")){
			savesoundchanges();
			pop();
		}
	}
}

// input options

static void
bindwaitmenu(void)
{
	char s[MAX_STRING_CHARS];
	int i;
	qboolean *p;

	uis.fullscreen = qtrue;
	menubackground();

	setalign("center");
	Com_sprintf(s, sizeof s, "Press a key to bind to %s", binds[bw.i].name);
	drawstring(screenwidth()/2, 200, s, FONT2, 24, CText);
	drawstring(screenwidth()/2, 230, "BACKSPACE to unbind", FONT2, 24, CText);
	drawstring(screenwidth()/2, 260, "ESC to cancel", FONT2, 24, CText);
	setalign("");

	if(keydown(K_ESCAPE)){
		bw.i = -1;
		bw.whichkey = -1;
		pop();
		return;
	}
	if((p = memchr(uis.keys, qtrue, sizeof uis.keys)) != nil){
		i = p - uis.keys;
		if(bw.whichkey == 0 && i != binds[bw.i].alt){
			binds[bw.i].alt = binds[bw.i].k;
			binds[bw.i].k = i;
			io.dirty = qtrue;
		}else if(i != binds[bw.i].k){
			binds[bw.i].alt = i;
			io.dirty = qtrue;
		}
		bw.i = -1;
		bw.whichkey = -1;
		pop();
	}
}

static void
lookupbind(char *cmd, int *keys, int keyslen)
{
	int i, n;
	char buf[MAX_STRING_CHARS];

	for(i = 0; i < keyslen; i++)
		keys[i] = -1;
	n = 0;
	for(i = 0; i < ARRAY_LEN(uis.keys) && n < keyslen; i++){
		trap_Key_GetBindingBuf(i, buf, sizeof buf);
		if(*buf == '\0')
			continue;
		if(Q_stricmp(buf, cmd) == 0)
			keys[n++] = i;
	}
}

static void
initinputmenu(void)
{
	int i, k[2];

	memset(&io, 0, sizeof io);

	for(i = 0; binds[i].name != nil; i++){
		lookupbind(binds[i].cmd, k, ARRAY_LEN(k));
		binds[i].k = k[0];
		binds[i].alt = k[1];
		if(binds[i].alt >= K_MOUSE1 && binds[i].alt <= K_MWHEELUP){
			int tmp;

			// swap so that mouse buttons go in the first column
			tmp = binds[i].k;
			binds[i].k = binds[i].alt;
			binds[i].alt = tmp;
		}
	}

	io.sens = trap_Cvar_VariableValue("sensitivity");

	io.initialized = qtrue;
}

static void
saveinputchanges(void)
{
	int i;

	for(i = 0; binds[i].name != nil; i++){
		if(binds[i].k != -1)
			trap_Key_SetBinding(binds[i].k, binds[i].cmd);
		if(binds[i].alt != -1)
			trap_Key_SetBinding(binds[i].alt, binds[i].cmd);
	}

	trap_Cvar_SetValue("sensitivity", io.sens);

	io.initialized = qfalse;
}

void
inputmenu(void)
{
	const float spc = 24;
	float xx, xxx, y;
	int i;

	if(!io.initialized)
		initinputmenu();

	focusorder(".io.s");
	defaultfocus(".io.s");

	uis.fullscreen = qtrue;
	menubackground();
	xx = 440;
	xxx = 520;
	y = 50;

	optionname("Sensitivity", y);
	if(slider(".io.s", xx, y, 0.1f, 10.0f, &io.sens, "%.1f"))
		io.dirty = qtrue;
	y += spc;

	for(i = 0; binds[i].name != nil; i++){
		char id[IDLEN], *p;

		Com_sprintf(id, sizeof id, ".io.%s.k", binds[i].name);
		p = id;
		while((p = strchr(p, ' ')) != nil)
			*p = '-';
		focusorder(id);
		optionname(binds[i].name, y);
		if(keybinder(id, xx, y, binds[i].k)){
			bw.i = i;
			bw.whichkey = 0;
			clearkeys();
			push(bindwaitmenu);
		}

		Com_sprintf(id, sizeof id, ".io.%s.alt", binds[i].name);
		p = id;
		while((p = strchr(p, ' ')) != nil)
			*p = '-';
		focusorder(id);
		if(keybinder(id, xxx, y, binds[i].alt)){
			bw.i = i;
			bw.whichkey = 1;
			clearkeys();
			push(bindwaitmenu);
		}
		y += spc;
	}

	optionsbuttons();

	if(io.dirty || io.needrestart){
		if(acceptbutton(".io.accept")){
			saveinputchanges();
			pop();
		}
	}
}

// misc menus

void
defaultsmenu(void)
{
	focusorder(".dm.yes .dm.no");
	defaultfocus(".dm.no");

	uis.fullscreen = qtrue;
	menubackground();

	setalign("center");
	drawstring(screenwidth()/2, 200, "This will erase your config file.",
	   FONT2, 24, CRed);

	setalign("right");
	if(button(".dm.yes", screenwidth()/2 - 20, 320, "Reset")){
		trap_Cmd_ExecuteText(EXEC_APPEND, "cvar_restart\n");
		trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
	}

	setalign("left");
	if(button(".dm.no", screenwidth()/2 + 20, 320, "Cancel"))
		pop();

	setalign("");
}

void
errormenu(void)
{
	char buf[MAX_STRING_CHARS];
	qboolean *p;

	if((p = memchr(uis.keys, qtrue, sizeof uis.keys)) != nil){
		trap_Cvar_Set("com_errormessage", "");
		pop();
		push(mainmenu);
		return;
	}
	uis.fullscreen = qtrue;
	menubackground();
	trap_Cvar_VariableStringBuffer("com_errormessage", buf, sizeof buf);
	drawstring(20, 180, "error:", FONT2, 20, CRed);
	drawmultiline(20, 220, screenwidth()-20, buf, FONT2, 16, CText);
}

void
mainmenu(void)
{
	float spc = 35;
	float x, y;


	uis.fullscreen = qtrue;
	menubackground();

	focusorder(".mm.mp .mm.opt .mm.q");
	defaultfocus(".mm.mp");

	setalign("left");

	x = MARGIN;
	y = 190;
	if(button(".mm.mp", x, y, "Multiplayer"))
		push(placeholder);
	y += spc;
	if(button(".mm.opt", x, y, "Options"))
		push(videomenu);
	y += spc;
	if(button(".mm.q", x, y, "Quit"))
		push(quitmenu);
}

void
ingamemenu(void)
{
	const float spc = 35;
	float x, y;

	focusorder(".im.r .im.opt .im.qm .im.q");
	defaultfocus(".im.r");

	setalign("left");

	x = MARGIN;
	y = 180;
	if(keydown(K_ESCAPE) || button(".im.r", x, y, "Resume")){
		pop();
		trap_Cvar_Set("cl_paused", "0");
	}
	y += spc;
	if(button(".im.opt", x, y, "Options"))
		push(videomenu);
	y += spc;
	if(button(".im.qm", x, y, "Quit to menu"))
		trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
	y += spc;
	if(button(".im.q", x, y, "Quit"))
		push(quitmenu);

	setalign("");
}
