// General entity animation functions.

#include "cg_local.h"

/*
may include ANIM_TOGGLEBIT
*/
void
CG_SetLerpFrameAnimation(animation_t *anims, lerpFrame_t *lf, int newanim)
{
	animation_t *anim;

	lf->animnum = newanim;
	newanim &= ~ANIM_TOGGLEBIT;

	if(newanim < 0 || newanim >= MAX_TOTALANIMATIONS)
		cgerrorf("%d: bad animation number", newanim);

	anim = &anims[newanim];

	lf->animation = anim;
	lf->animtime = lf->frametime + anim->initiallerp;

	if(cg_debugAnim.integer)
		cgprintf("anim: %i\n", newanim);
}

/*
Advances a lerpFrame_t.

cg.time should be between oldframetime and frametime after exit
*/
void
CG_RunLerpFrame(animation_t *anims, lerpFrame_t *lf, int newanim, float speedscale)
{
	int f, nframes;
	animation_t *anim;

	// debugging tool to get no animations
	if(cg_animSpeed.integer == 0){
		lf->oldframe = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if(newanim != lf->animnum || !lf->animation)
		CG_SetLerpFrameAnimation(anims, lf, newanim);

	// if we have passed the current frame, move it to
	// oldframe and calculate a new frame
	if(cg.time >= lf->frametime){
		lf->oldframe = lf->frame;
		lf->oldframetime = lf->frametime;

		// get the next frame based on the animation
		anim = lf->animation;
		if(!anim->framelerp){
			if(cg_debugAnim.integer)
				cgprintf("anim: anim->framelerp == 0\n");
			return;					// shouldn't happen
		}
		if(cg.time < lf->animtime)
			lf->frametime = lf->animtime;	// initial lerp
		else
			lf->frametime = lf->oldframetime + anim->framelerp;
		f = (lf->frametime - lf->animtime) / anim->framelerp;
		f *= speedscale;	// adjust for haste, etc

		nframes = anim->nframes;
		if(anim->flipflop)
			nframes *= 2;
		if(f >= nframes){
			f -= nframes;
			if(anim->loopframes){
				f %= anim->loopframes;
				f += anim->nframes - anim->loopframes;
			}else{
				f = nframes - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frametime = cg.time;
			}
		}
		if(anim->reversed)
			lf->frame = anim->firstframe + anim->nframes - 1 - f;
		else if(anim->flipflop && f>=anim->nframes)
			lf->frame = anim->firstframe + anim->nframes - 1 - (f%anim->nframes);
		else
			lf->frame = anim->firstframe + f;
		if(cg.time > lf->frametime){
			lf->frametime = cg.time;
			if(cg_debugAnim.integer)
				cgprintf("Clamp lf->frametime\n");
		}
	}

	if(lf->frametime > cg.time + 200)
		lf->frametime = cg.time;

	if(lf->oldframetime > cg.time)
		lf->oldframetime = cg.time;
	// calculate current lerp value
	if(lf->frametime == lf->oldframetime)
		lf->backlerp = 0;
	else
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldframetime) / (lf->frametime - lf->oldframetime);
}

void
CG_ClearLerpFrame(animation_t *anims, lerpFrame_t *lf, int animnum)
{
	lf->frametime = lf->oldframetime = cg.time;
	CG_SetLerpFrameAnimation(anims, lf, animnum);
	lf->oldframe = lf->frame = lf->animation->firstframe;
}

/*
Reads a configuration file containing animation counts and rates.
models/misc/thrustflame/animation.cfg, etc.
See also cg_players.c:/CG_ParsePlayerAnimationFile/
*/
qboolean
CG_ParseAnimationFile(const char *filename, animation_t *anims)
{
	char *p, *prev;
	int len;
	int i;
	char *token;
	float fps;
	char text[20000];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
		return qfalse;
	if(len >= sizeof(text) - 1){
		cgprintf("%s: file too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	p = text;

	while(1){
		prev = p;	// so we can unget
		token = COM_Parse(&p);
		if(!token)
			break;

		// if it is a number, start parsing animations
		if(token[0] >= '0' && token[0] <= '9'){
			p = prev;	// unget the token
			break;
		}
		Com_Printf("%s: unknown token '%s'\n", filename, token);
	}

	// read information for each animation
	for(i = 0; i < MAX_ANIMATIONS; i++){
		// first frame
		token = COM_Parse(&p);
		if(!*token)
			break;
		anims[i].firstframe = atoi(token);

		// numframes
		token = COM_Parse(&p);
		if(!*token)
			break;
		anims[i].nframes = atoi(token);

		anims[i].reversed = qfalse;
		anims[i].flipflop = qfalse;
		// if nframes is negative the animation is reversed
		if(anims[i].nframes < 0){
			anims[i].nframes = -anims[i].nframes;
			anims[i].reversed = qtrue;
		}

		// looping frames
		token = COM_Parse(&p);
		if(!*token)
			break;
		anims[i].loopframes = atoi(token);

		// framerate
		token = COM_Parse(&p);
		if(!*token)
			break;
		fps = atof(token);
		if(fps == 0)
			fps = 1;
		anims[i].framelerp = 1000 / fps;
		anims[i].initiallerp = 1000 / fps;
	}
	if(cg_debugAnim.integer)
		cgprintf("anim: parsed %i animations\n", i);
	return qtrue;
}
