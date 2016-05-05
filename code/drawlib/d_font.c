#include "d_local.h"
#include "d_public.h"
#include "d_fontmap.h"

charmap_t charmaps[NUMFONTS];

void
registercharmap(int font, int w, int h, float charh, float spc,
   const char *shader, int **map, kerning_t *kern, int nkern)
{
	charmaps[font].w = w;
	charmaps[font].h = h;
	charmaps[font].charh = charh;
	charmaps[font].charspc = spc;
	charmaps[font].shader = trap_R_RegisterShaderNoMip(shader);
	memcpy(charmaps[font].map, map, sizeof charmaps[font].map);
	charmaps[font].kernings = kern;
	charmaps[font].nkernings = nkern;
}

static int
kerningcmp(const void * a, const void * b)
{
	const kerning_t *ka, *kb;

	ka = a; kb = b;
	if(ka->first == kb->first)
		return ka->second - kb->second;
	return ka->first - kb->first;
}

static float
kerning(int font, int first, int second)
{
	kerning_t *ks, *p, key;
	size_t nk;

	ks = charmaps[font].kernings;
	nk = charmaps[font].nkernings;
	if(nk < 1 || ks == nil)
		return 0.0f;
	key.first = first;
	key.second = second;
	p = bsearch(&key, ks, nk, sizeof *ks, kerningcmp);
	if(p != nil)
		return p->amount;
	return 0.0f;
}

/*
Draws a string.
size is the height of the font in virtual pixels.
*/
void
drawstring(float x, float y, const char *str, int font, float size, vec4_t color)
{
	const char *p;
	float w, h;
	float scale, strwidth;
	float glyphrow, glyphcol, glyphwidth, glyphheight;
	vec4_t tmpclr;
	int c;

	trap_R_SetColor(color);

	scale = size/charmaps[font].charh;
	strwidth = stringwidth(str, font, size, 0, -1);

	// alignment offset
	if(draw.align[draw.alignstack][0] == 'c')
		y -= 0.5f * charmaps[font].charh * scale;
	else if(draw.align[draw.alignstack][0] == 'b')
		y -= charmaps[font].charh * scale;
	if(draw.align[draw.alignstack][1] == 'c')
		x -= 0.5f * strwidth;
	else if(draw.align[draw.alignstack][1] == 'r')
		x -= strwidth;

	w = 0;
	h = 0;
	p = str;
	while(*p){
		if(Q_IsColorString(p)){
			memcpy(tmpclr, g_color_table[ColorIndex(p[1])], sizeof tmpclr);
			tmpclr[3] = color[3];
			trap_R_SetColor(tmpclr);
			p += 2;
			continue;
		}
		c = *p & 0xFF;
		if(charmaps[font].map[c][2] != -1){
			float xx, yy, ww, hh;

			glyphcol = (float)charmaps[font].map[c][0] / charmaps[font].w;
			glyphrow = (float)charmaps[font].map[c][1] / charmaps[font].h;
			glyphwidth = (float)charmaps[font].map[c][2] / charmaps[font].w;
			glyphheight = (float)charmaps[font].charh / charmaps[font].h;
			w = (float)charmaps[font].map[c][2] * scale;
			h = charmaps[font].charh * scale;

			// apply kerning if possible
			if(*(p+1) != '\0')
				x += kerning(font, *p, *(p+1))*scale;
			// add xoffset and yoffset
			xx = x + charmaps[font].map[c][3]*scale;
			yy = y + charmaps[font].map[c][4]*scale;
			ww = w;
			hh = h;
			scalecoords(&xx, &yy, &ww, &hh);

			trap_R_DrawStretchPic(xx, yy, ww, hh, glyphcol, glyphrow,
			   glyphcol+glyphwidth, glyphrow+glyphheight, charmaps[font].shader);
		}else{
			w = 0;
		}
		// add xadvance
		x += charmaps[font].map[c][5]*scale;
		p++;
	}
	trap_R_SetColor(nil);
}

void
drawmultiline(float x, float y, float xmax, const char *str, int font, float size, vec4_t color)
{
	float width;
	char *s1, *s2, *s3;
	char c;
	char buf[1024];
	float scale;

	if(!str || str[0]=='\0')
		return;

	scale = size/charmaps[font].charh;

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while(1){
		do
			s3++;
		while(*s3!=' ' && *s3!='\0' && *s3!='\n');
		c = *s3;
		*s3 = '\0';
		width = stringwidth(s1, font, size, 0, -1);
		*s3 = c;
		if(c == '\n'){
			// force a line break
			width = xmax+1;
		}
		if(width > xmax){
			if(s1==s2)
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			*s2 = '\0';
			drawstring(x, y, s1, font, size, color);
			y += charmaps[font].charh * scale;
			if(c == '\0'){
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if(*s2 != '\0')	// if we are printing an overflowing line we have s2 == s3
					drawstring(x, y, s2, font, size, color);
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}else{
			s2 = s3;
			if(c == '\0'){	// we reached the end
				drawstring(x, y, s1, font, size, color);
				break;
			}
		}
	}
}

/*
Returns the width of a drawn string in terms of virtual coordinates.
sliceend=-1 means up to the terminating \0.
*/
float
stringwidth(const char *str, int font, float size, int slicebegin, int sliceend)
{
	const char *p;
	float w, scale;
	int c;

	scale = size/charmaps[font].charh;

	w = 0;
	p = str;
	while(*p){
		c = *p & 0xFF;
		if(charmaps[font].map[c][2] != -1){
			// apply kerning if possible
			if(*(p+1) != '\0')
				w += kerning(font, *p, *(p+1))*scale;
			// add xadvance
			w += (charmaps[font].map[c][5]*scale + 1*scale);
		}
		p++;
	}
	return w;
}

float
stringheight(const char *str, int font, float size)
{
	return size;
}

/*
 * Truncate drawn string to fit into maxw.
 */
void
truncstringtowidth(char *str, int font, float size, float maxw)
{
	char *p;

	if(*str == '\0' || stringwidth(str, font, size, 0, -1) < maxw)
		return;
	for(p = str+strlen(str)-1; p != str; p--){
		if(stringwidth(str, font, size, 0, -1) < maxw)
			break;
		*p = '\0';
	}
}
