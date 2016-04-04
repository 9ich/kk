#include "d_local.h"
#include "d_public.h"
#include "d_colours.h"

static vec4_t vshadow	= {0.000f, 0.000f, 0.100f, Shadowalpha};

float *CText		= CLightBlue;	// normal text
float *CWBorder		= CBlack;	// widget border
float *CWBody		= CRoyalBlue;	// widget body
float *CWText		= CLightBlue;	// widget text
float *CWHot		= CGoldenrod;	// widget text when hot
float *CWActive		= CHoneydew;	// widget text when active
float *CWFocus		= CGoldenrod;	// widget outline when active
float *CWShadow		= vshadow;

drawStatic_t drawstuff;

void
drawlibinit(void)
{
	drawstuff.whiteShader = trap_R_RegisterShaderNoMip("white");
	drawstuff.charset = trap_R_RegisterShaderNoMip("gfx/2d/bigchars");
	drawstuff.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop");
}

void
drawlibbeginframe(int realtime, float xscale, float yscale, float bias)
{
	drawstuff.frametime = realtime - drawstuff.realtime;
	drawstuff.realtime = realtime;
	drawstuff.xscale = xscale;
	drawstuff.yscale = yscale;
	drawstuff.bias = bias;
}

/*
Adjusts for resolution and screen aspect ratio.
*/
void
adjustcoords(float *x, float *y, float *w, float *h)
{
#if 0
	// adjust for wide screens
	if(cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640)
		*x += 0.5 * (cgs.glconfig.vidWidth - (cgs.glconfig.vidHeight * 640 / 480));

#endif
	// expect valid pointers
	//*x = *x * drawstuff.xscale + drawstuff.bias;
	*x *= drawstuff.xscale;
	*y *= drawstuff.yscale;
	*w *= drawstuff.xscale;
	*h *= drawstuff.yscale;
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
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, drawstuff.whiteShader);

	trap_R_SetColor(nil);
}

void
drawrect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	adjustcoords(&x, &y, &width, &height);

	trap_R_DrawStretchPic(x, y, width, 1, 0, 0, 0, 0, drawstuff.whiteShader);
	trap_R_DrawStretchPic(x, y, 1, height, 0, 0, 0, 0, drawstuff.whiteShader);
	trap_R_DrawStretchPic(x, y + height - 1, width, 1, 0, 0, 0, 0, drawstuff.whiteShader);
	trap_R_DrawStretchPic(x + width - 1, y, 1, height, 0, 0, 0, 0, drawstuff.whiteShader);

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
