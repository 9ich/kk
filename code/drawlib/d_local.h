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

// defined as syscalls by modules
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
