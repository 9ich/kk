/*
Drawlib is a drawing package for cgame and ui.
It replaces their own redundant drawing functions.
*/

#ifndef __D_PUBLIC_H__
#define __D_PUBLIC_H__

#include "d_colourdecls.h"

enum
{
	FONT1,
	FONT2,
	FONT3,
	FONT4,
	NUMFONTS
};

// drop shadows
#define Shadowalpha	0.7f

extern float *CText;	// normal text
extern float *CWBorder;	// widget border
extern float *CWBody;	// widget body
extern float *CWText;	// widget text
extern float *CWHot;	// widget text when hot
extern float *CWActive;	// widget text when active
extern float *CWFocus;	// widget outline when active
extern float *CWShadow;

void	drawlibinit(void);
void	drawlibbeginframe(int realtime, float vidwidth, float vidheight);

float	screenwidth(void);
float	screenheight(void);
float	centerleft(void);
float	centerright(void);
void	pushalign(const char *s);
void	popalign(int n);
void	setalign(const char *s);
const char	*getalign(void);
void	aligncoords(float *x, float *y, float *w, float *h);
void	scalecoords(float *x, float *y, float *w, float *h);
void	adjustcoords(float *x, float *y, float *w, float *h);
void	stretchcoords(float *x, float *y, float *w, float *h);

void	setcolour(const float *rgba);
void	lerpcolour(vec4_t a, vec4_t b, vec4_t c, float t);
void	colormix(vec4_t a, vec4_t b, vec4_t dst);
void	coloralpha(vec4_t dst, vec4_t src, float alpha);
void	drawnamedpic(float x, float y, float w, float h, const char *picname);
void	drawpic(float x, float y, float w, float h, qhandle_t hShader);
void	fillrect(float x, float y, float width, float height, const float *color);
void	drawrect(float x, float y, float width, float height, const float *color);
void	drawstring(float x, float y, const char *str, int font, float size, vec4_t color);
void	drawmultiline(float x, float y, float xmax, const char *str, int font, float size, vec4_t color);
float	stringwidth(const char *str, int font, float size, int slicebegin, int sliceend);
float	stringheight(const char *str, int font, float size);
void	truncstringtowidth(char *str, int font, float size, float maxw);

#endif // __D_PUBLIC_H__
