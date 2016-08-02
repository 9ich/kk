#include "ui_local.h"

qboolean
button(const char *id, int x, int y, const char *label)
{
	float w = 48, h = 28;
	float propw;
	qboolean hot, changed;
	float *clr;

	hot = qfalse;
	propw = stringwidth(label, FONT1, 32, 0, -1);
	if(w < propw)
		w = propw;

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	clr = CWText;
	if(hot){
		clr = CWHot;
		if(strcmp(uis.active, id) == 0)
			clr = CWActive;
	}
	drawstring(x, y, label, FONT1, 32, clr);

	changed = !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;

	if(strcmp(id, uis.focus) == 0){
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
		if(keydown(K_ENTER))
			changed = qtrue;
	}
	return changed;
}

qboolean
slider(const char *id, int x, int y, float min, float max, float *val, const char *displayfmt)
{
	float w = 120, pad = 1, h = 12;
	float ix, iy, iw;
	qboolean hot, updated;
	float knobpos, mousepos;
	char s[32];
	float *clr;

	hot = qfalse;
	updated = qfalse;
	ix = x+pad;
	iy = y+pad;
	iw = w - 2*pad;
	*s = '\0';

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}
	*val = Com_Scale(*val, min, max, 0, max);
	if(strcmp(uis.active, id) == 0){
		float v;

		mousepos = uis.cursorx - ix;
		if(mousepos < 0)
			mousepos = 0;
		if(mousepos > iw)
			mousepos = iw;
		v = (mousepos * max) / iw;
		updated = (*val != v);
		*val = v;
	}

	fillrect(x, y, w, h, CWBody);
	drawrect(x, y, w, h, CWBorder);

	knobpos = (iw * *val) / max;
	*val = Com_Scale(*val, 0, max, min, max);

	if(hot || strcmp(uis.active, id) == 0)
		clr = CWHot;
	else
		clr = CWText;

	fillrect(ix, iy, knobpos, h-2*pad, clr);
	fillrect(ix+knobpos, iy, 1, h-2*pad, CWhite);

	if(*displayfmt != '\0'){
		Com_sprintf(s, sizeof s, displayfmt, *val);
		drawstring(x+w+6, iy, s, FONT2, 12, CWText);
		*val = atof(s);
	}

	if(strcmp(id, uis.focus) == 0){
		char buf[32], *p;
		float incr;

		drawrect(x-2, y-2, w+4, h+4, CWFocus);

		incr = 0.01f;
		if(*s != '\0' && (uis.keys[K_LEFTARROW] || uis.keys[K_RIGHTARROW])){
			// derive an increment value from the display string
			Q_strncpyz(buf, s, sizeof buf);
			for(p = buf; *p != '\0'; p++){
				if((*p >= '0' && *p <= '9') ||
				   (*p == '-' || *p == 'e' || *p == 'E'))
					*p = '0';
			}
			*(p - 1) = '1';
			incr = atof(buf);
		}
		if(keydown(K_LEFTARROW)){
			if(*val-incr >= min)
				*val -= incr;
			updated = qtrue;
		}else if(keydown(K_RIGHTARROW)){
			if(*val+incr <= max)
				*val += incr;
			updated = qtrue;
		}
	}	
	return updated;
}

qboolean
checkbox(const char *id, int x, int y, qboolean *state)
{
	const float w = 16, h = 16;
	qboolean hot, changed;

	hot = qfalse;
	changed = qfalse;

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			*state = !*state;
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	fillrect(x, y, w, h, CWBody);
	drawrect(x, y, w, h, CWBorder);
	if(*state){
		if(hot)
			setcolour(CWHot);
		else
			setcolour(CWText);
		drawnamedpic(x+2, y+2, w-4, h-4, "menu/art/tick");
		setcolour(nil);
	}

	changed = !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;

	if(strcmp(id, uis.focus) == 0){
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
		if(keydown(K_ENTER)){
			*state = !*state;
			changed = qtrue;
		}
		if(keydown(K_SPACE)){
			*state = !*state;
			changed = qtrue;
		}
	}
	return changed;
}

static qboolean
updatefield(char *buf, int *caret, int sz)
{
	char *p;
	int i;
	qboolean updated;

	updated = qfalse;

	for(p = uis.text, i = *caret; *p != '\0'; p++){
		switch(*p){
		case 1:	// ^A
			i = 0;
			break;
		case 3:	// ^C
			buf[0] = '\0';
			i = 0;
			break;
		case 5:	// ^E
			i = (int)(strchr(buf, '\0') - buf);
			break;
		case 8:	// backspace
			if(i > 0){
				if(i < sz)
					memcpy(buf+i-1, buf+i, sz-i);
				else
					buf[i-1] = '\0';
				i--;
			}
			break;
		default:
			if(i < sz-1 && strlen(buf) < sz-1){
				if(*p >= ' '){
					memcpy(buf+i+1, buf+i, sz-i-1);
					buf[i] = *p;
				}
				i++;
			}
		}
		updated = qtrue;
	}

	if(keydown(K_LEFTARROW))
		if(i > 0)
			i = i - 1;
	if(keydown(K_RIGHTARROW))
		if(i < sz-1 && buf[i] != '\0')
			i = i + 1;
	if(keydown(K_HOME))
		i = 0;
	if(keydown(K_END))
		i = (int)(strchr(buf, '\0') - buf);
	if(keydown(K_DEL)){
		buf[i] = '\0';
		if(i < sz-1)
			memcpy(buf+i, buf+i+1, sz-i-1);
		updated = qtrue;
	}
	*caret = i;
	return updated;
}

qboolean
textfield(const char *id, int x, int y, int width, char *buf, int *caret, int sz)
{
	const float w = width*SMALLCHAR_WIDTH;
	const float h = 16, pad = 4;
	qboolean hot, updated;
	int i, caretpos;

	hot = qfalse;
	updated = qfalse;

	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	if(strcmp(uis.focus, id) == 0)
		updated = updatefield(buf, caret, sz);

	if(strcmp(uis.active, id) == 0 || hot)
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWActive);
	else
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWHot);
	drawstring(x, y+2, buf, FONT2, 12, CWText);
	for(i = 0, caretpos = 0; i < *caret && buf[i] != '\0'; i++){
		if(Q_IsColorString(&buf[i]))
			i++;
		else
			caretpos += SMALLCHAR_WIDTH;
	}
	fillrect(x+caretpos, y, 2, h, CRed);
	if(updated)
		trap_S_StartLocalSound(uis.fieldUpdateSound, CHAN_LOCAL_SOUND);

	if(strcmp(uis.focus, id) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return updated;
}

static qboolean
spinnerbutton(const char *id, int x, int y, const char *shader)
{
	const float sz = 18;
	qboolean hot;

	hot = qfalse;

	if(mouseover(x, y, sz, sz)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1])
			Q_strncpyz(uis.active, id, sizeof uis.active);
	}

	if(hot)
		setcolour(CWHot);
	else
		setcolour(CWText);
	drawnamedpic(x, y, sz, sz, shader);
	setcolour(nil);
	return !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;
}

qboolean
textspinner(const char *id, int x, int y, char **opts, int *i, int nopts)
{
	const float w = 13*SMALLCHAR_WIDTH, h = 18, bsz = 18;
	qboolean changed;
	char bid[IDLEN];

	changed = qfalse;

	if(nopts > 1){
		Com_sprintf(bid, sizeof bid, "%s.prev", id);
		if(spinnerbutton(bid, x, y, "menu/art/left")){
			*i = (*i <= 0)? nopts-1 : *i-1;
			changed = qtrue;
		}
		Com_sprintf(bid, sizeof bid, "%s.next", id);
		if(spinnerbutton(bid, x+bsz+w, y, "menu/art/right")){
			*i = (*i + 1) % nopts;
			changed = qtrue;
		}
	}

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	fillrect(x+bsz, y, w, h, CWBody);
	drawrect(x+bsz, y, w, h, CWBorder);

	pushalign("center");
	drawstring(x+bsz+w/2, y+2, opts[*i], FONT2, 12, CText);
	popalign(1);

	if(strcmp(uis.focus, id) == 0){
		drawrect(x+bsz-2, y-2, w+4, h+4, CWFocus);
		if(keydown(K_LEFTARROW)){
			*i = (*i <= 0)? nopts-1 : *i-1;
			changed = qtrue;
		}else if(keydown(K_RIGHTARROW)){
			*i = (*i + 1) % nopts;
			changed = qtrue;
		}
	}		
	return changed;
}

/*
This only displays the key; binding the key is the caller's responsibility.
*/
qboolean
keybinder(const char *id, int x, int y, int key)
{
	const int width = 7;	// in chars
	const float w = width*SMALLCHAR_WIDTH;
	const float h = 16, pad = 4;
	char buf[32], *p;
	qboolean changed;

	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		if(strcmp(uis.active, "") == 0 && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	if(key == -1)
		*buf = '\0';
	else
		trap_Key_KeynumToStringBuf(key, buf, sizeof buf);

	Q_strlwr(buf);

	// strip "arrow" off e.g. "leftarrow"
	p = strstr(buf, "arrow");
	if(p != nil)
		*p = '\0';

	// truncate to field width
	buf[width-1] = '\0';

	fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWBody);
	drawrect(x-pad, y-pad, w+2*pad, h+2*pad, CWBorder);

	drawstring(x, y+2, buf, FONT2, 12, CWText);

	changed = !uis.keys[K_MOUSE1] && strcmp(uis.hot, id) == 0 &&
	   strcmp(uis.active, id) == 0;	
	
	if(strcmp(uis.focus, id) == 0){
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
		if(keydown(K_ENTER))
			changed = qtrue;
	}
	return changed;
}
