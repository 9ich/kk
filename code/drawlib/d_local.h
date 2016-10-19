#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "d_public.h"

enum { MAXALIGNSTACK	= 16 };

typedef struct
{
	qboolean	debug;
	float		vidw, vidh;	// display's resolution, from renderer
	float		vidaspect;
	float		vidscale;	// the scale to get from SCREEN_HEIGHT to vidh;
	int		frametime;
	int		realtime;
	qhandle_t	whiteShader;
	qhandle_t	charset;
	qhandle_t	charsetProp;
	int		alignstack;
	char		align[MAXALIGNSTACK][3];	// t/c/b, l/c/r, \0
} drawStatic_t;

typedef struct
{
	float		first;
	float		second;
	float		amount;
} kerning_t;

typedef struct
{
	int		w, h;		// whole charmap dimensions
	float		charh;		// height of characters
	float		charspc;	// spacing between characters
	qhandle_t	shader;		// the charmap
	int		map[128][6];	// offset & dimensions info
	kerning_t	kernings[1024];
	int		nkernings;
} charmap_t;

extern drawStatic_t draw;
extern charmap_t charmaps[NUMFONTS];
extern int font1map[128][6];
extern kerning_t font1kernings[506];
extern int font2map[128][6];
extern kerning_t font2kernings[292];
extern int font3map[128][6];
extern int font4map[128][6];

// d_font.c
extern void	registercharmap(int font, const char *filename);

// defined by modules
qhandle_t	trap_R_RegisterModel(const char *name);
qhandle_t	trap_R_RegisterSkin(const char *name);
qhandle_t	trap_R_RegisterShader(const char *name);
qhandle_t	trap_R_RegisterShaderNoMip(const char *name);
void		trap_R_ClearScene(void);
void		trap_R_AddRefEntityToScene(const refEntity_t *re);
void		trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void		trap_R_AddLightToScene(const vec3_t org, float intensity,
		   float r, float g, float b);
void		trap_R_RenderScene(const refdef_t *fd);
void		trap_R_SetColor(const float *rgba);
void		trap_R_DrawStretchPic(float x, float y, float w, float h, float s1,
		   float t1, float s2, float t2, qhandle_t hShader);
void		trap_UpdateScreen(void);
int		trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void		trap_FS_Read(void *buffer, int len, fileHandle_t f);
void		trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void		trap_FS_FCloseFile(fileHandle_t f);
int		trap_FS_Seek(fileHandle_t f, long offset, int origin);
