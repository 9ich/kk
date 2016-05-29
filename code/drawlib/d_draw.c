#include "d_local.h"
#include "d_colours.h"

static vec4_t vshadow	= {0.000f, 0.000f, 0.100f, Shadowalpha};

float *CText		= CGoldenrod;	// normal text
float *CWBorder		= CBlack;	// widget border
float *CWBody		= CRoyalBlue;	// widget body
float *CWText		= CGoldenrod;	// widget text
float *CWHot		= CLightBlue;	// widget text when hot
float *CWActive		= CHoneydew;	// widget text when active
float *CWFocus		= CGoldenrod;	// widget outline when active
float *CWShadow		= vshadow;

drawStatic_t draw;

void
drawlibinit(void)
{
	draw.whiteShader = trap_R_RegisterShaderNoMip("white");
	draw.charset = trap_R_RegisterShaderNoMip("gfx/2d/bigchars");
	draw.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop");

	registercharmap(FONT1, 512, 512, 64, 3, "fonts/font1", (int**)font1map,
	   font1kernings, ARRAY_LEN(font1kernings));
	registercharmap(FONT2, 512, 512, 64, 3, "fonts/font2", (int**)font2map,
	   font2kernings, ARRAY_LEN(font2kernings));
	registercharmap(FONT3, 1024, 512, 84, 3, "fonts/font3", (int**)font3map,
	   nil, 0);
	registercharmap(FONT4, 1024, 512, 64, 3, "fonts/font4", (int**)font4map,
	   nil, 0);
}

void
drawlibbeginframe(int realtime, float vidwidth, float vidheight)
{
	draw.frametime = realtime - draw.realtime;
	draw.realtime = realtime;
	draw.vidw = vidwidth;
	draw.vidh = vidheight;
	draw.vidaspect = vidwidth/vidheight;
	draw.vidscale = vidheight/SCREEN_HEIGHT;
	draw.alignstack = 0;
	draw.align[0][0] = 't';
	draw.align[0][1] = 'l';
	draw.align[0][2] = '\0';
	draw.debug = qfalse;

	if(draw.debug)
		// draw a rectangle around the 4:3 safe zone in the centre of
		// the screen
		drawrect(centerleft(), 0, centerright()-centerleft(), screenheight(), CRed);
}

void
setalign(const char *s)
{
	int i;

	i = draw.alignstack;
	if(*s == '\0'){
		draw.align[i][0] = 't';
		draw.align[i][1] = 'l';
		return;
	}

	if(strstr(s, "bottom") != nil)
		draw.align[i][0] = 'b';
	else if(strstr(s, "mid") != nil)
		draw.align[i][0] = 'c';
	else
		draw.align[i][0] = 't';

	if(strstr(s, "right") != nil)
		draw.align[i][1] = 'r';
	else if(strstr(s, "center") != nil)
		draw.align[i][1] = 'c';
	else
		draw.align[i][1] = 'l';
}

const char*
getalign(void)
{
	return &draw.align[draw.alignstack][0];
}

void
pushalign(const char *s)
{
	int i;

	if(draw.alignstack < MAXALIGNSTACK)
		draw.alignstack++;
	i = draw.alignstack;

	if(*s == '\0'){
		draw.align[i][0] = 't';
		draw.align[i][1] = 'l';
		return;
	}

	if(strstr(s, "bottom") != nil)
		draw.align[i][0] = 'b';
	else if(strstr(s, "mid") != nil)
		draw.align[i][0] = 'c';
	else
		draw.align[i][0] = 't';

	if(strstr(s, "right") != nil)
		draw.align[i][1] = 'r';
	else if(strstr(s, "center") != nil)
		draw.align[i][1] = 'c';
	else
		draw.align[i][1] = 'l';
}

void
popalign(int n)
{
	// keep index within range
	draw.alignstack = MIN(MAX(0, draw.alignstack - n), MAXALIGNSTACK);
}

/*
Scales virtual coordinates to real display resolution.
*/
void
scalecoords(float *x, float *y, float *w, float *h)
{
	// scale to fit display height
	*x = *x/SCREEN_WIDTH * draw.vidh*(SCREEN_WIDTH/SCREEN_HEIGHT);
	*y = *y/SCREEN_HEIGHT * draw.vidh;
	*w = *w/SCREEN_WIDTH * draw.vidh*(SCREEN_WIDTH/SCREEN_HEIGHT);
	*h = *h/SCREEN_HEIGHT * draw.vidh;
}

/*
Shifts virtual coordinates according to the current alignment mode.
*/
void
aligncoords(float *x, float *y, float *w, float *h)
{
	if(draw.align[draw.alignstack][0] == 'c')
		*y -= 0.5f * *h;
	else if(draw.align[draw.alignstack][0] == 'b')
		*y -= *h;
	if(draw.align[draw.alignstack][1] == 'c')
		*x -= 0.5f * *w;
	else if(draw.align[draw.alignstack][1] == 'r')
		*x -= *w;
}

void
adjustcoords(float *x, float *y, float *w, float *h)
{
	aligncoords(x, y, w, h);
	scalecoords(x, y, w, h);
}

/*
Stretches x, y, w, h up to display's real resolution, regardless
of aspect ratio, selected pane, and alignment.
*/
void
stretchcoords(float *x, float *y, float *w, float *h)
{
	*x = *x/SCREEN_WIDTH * draw.vidw;
	*y = *y/SCREEN_HEIGHT * draw.vidh;
	*w = *w/SCREEN_WIDTH * draw.vidw;
	*h = *h/SCREEN_HEIGHT * draw.vidh;
}

/*
Returns the width of the screen in virtual coordinates.
*/
float
screenwidth(void)
{
	return SCREEN_HEIGHT * draw.vidaspect;
}

/*
Returns the height of the screen in virtual coordinates.
*/
float
screenheight(void)
{
	return SCREEN_HEIGHT;
}

/*
Returns the left side of a 640*480 virtual region in the centre of the screen.
This makes it more convenient to write centered UIs and HUDs.
*/
float
centerleft(void)
{
	return 0.5f*(screenwidth() - screenheight()*(SCREEN_WIDTH/SCREEN_HEIGHT));
}

float
centerright(void)
{
	return screenwidth() - 0.5f*(screenwidth() -
	   screenheight()*(SCREEN_WIDTH/SCREEN_HEIGHT));
}

void
drawnamedpic(float x, float y, float width, float height, const char *picname)
{
	qhandle_t hShader;

	hShader = trap_R_RegisterShaderNoMip(picname);
	adjustcoords(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void
drawpic(float x, float y, float w, float h, qhandle_t hShader)
{
	float s0, s1, t0, t1;

	if(w < 0){	// flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	}else{
		s0 = 0;
		s1 = 1;
	}

	if(h < 0){	// flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	}else{
		t0 = 0;
		t1 = 1;
	}

	adjustcoords(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

void
fillrect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	adjustcoords(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, draw.whiteShader);

	trap_R_SetColor(nil);
}

void
drawrect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	adjustcoords(&x, &y, &width, &height);

	trap_R_DrawStretchPic(x, y, width, 1, 0, 0, 0, 0, draw.whiteShader);
	trap_R_DrawStretchPic(x, y, 1, height, 0, 0, 0, 0, draw.whiteShader);
	trap_R_DrawStretchPic(x, y + height - 1, width, 1, 0, 0, 0, 0, draw.whiteShader);
	trap_R_DrawStretchPic(x + width - 1, y, 1, height, 0, 0, 0, 0, draw.whiteShader);

	trap_R_SetColor(nil);
}

void
setcolour(const float *rgba)
{
	trap_R_SetColor(rgba);
}

void
lerpcolour(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for(i = 0; i<4; i++){
		c[i] = a[i] + t*(b[i]-a[i]);
		if(c[i] < 0)
			c[i] = 0;
		else if(c[i] > 1.0)
			c[i] = 1.0;
	}
}

void
colormix(vec4_t a, vec4_t b, vec4_t dst)
{
	dst[0] = 0.5f*(a[0] + b[0]);
	dst[1] = 0.5f*(a[1] + b[1]);
	dst[2] = 0.5f*(a[2] + b[2]);
	dst[3] = 0.5f*(a[3] + b[3]);
}
