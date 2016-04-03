#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"

typedef struct
{
	float		xscale;
	float		yscale;
	float		bias;
	int		frametime;
	int		realtime;
	qhandle_t	whiteShader;
	qhandle_t	charset;
	qhandle_t	charsetProp;
} drawStatic_t;

extern drawStatic_t drawstuff;

extern void	adjustcoords(float *x, float *y, float *w, float *h);
