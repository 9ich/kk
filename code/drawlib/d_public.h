/*
Drawlib is a 2D drawing package for cgame and ui.
It replaces their own redundant drawing functions.
*/

#ifndef __D_PUBLIC_H__
#define __D_PUBLIC_H__

#include "d_colourdecls.h"

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

extern void	drawlibinit(void);
extern void	drawlibbeginframe(int realtime, float xscale, float yscale, float bias);
extern void	adjustcoords(float *x, float *y, float *w, float *h);
extern void	drawnamedpic(float x, float y, float w, float h, const char *picname);
extern void	drawpic(float x, float y, float w, float h, qhandle_t hShader);
extern void	fillrect(float x, float y, float width, float height, const float *color);
extern void	drawrect(float x, float y, float width, float height, const float *color);
extern void	setcolour(const float *rgba);
extern void	lerpcolour(vec4_t a, vec4_t b, vec4_t c, float t);
extern void	drawpropstr(int x, int y, const char *str, int style, vec4_t color);
extern void	drawpropstrwrapped(int x, int ystart, int xmax, int ystep, const char *str, int style, vec4_t color);
extern int	propstrwidth(const char *str, int slicebegin, int sliceend);
extern float	propstrsizescale(int style);
extern void	drawstr(int x, int y, const char *str, int style, vec4_t color);
extern void	drawstr2(int x, int y, const char *str, vec4_t color, int charw, int charh);
extern void	drawstrwrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color);
extern void	drawchar(int x, int y, int ch, int style, vec4_t color);

#endif // __D_PUBLIC_H__
