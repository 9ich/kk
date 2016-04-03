#include "d_local.h"
#include "d_public.h"
#include "d_fontmap.h"

static void
drawpropstr2(int x, int y, const char *str, vec4_t color, float scale, qhandle_t charset)
{
	const char *s;
	uchar ch;
	float ax, ay, aw, ah;
	float frow, fcol, fwidth, fheight;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * drawstuff.xscale + drawstuff.bias;
	ay = y * drawstuff.yscale;
	aw = 0;

	s = str;
	while(*s){
		ch = *s & 127;
		if(ch == ' ')
			aw = (float)PROP_SPACE_WIDTH * drawstuff.xscale * scale;
		else if(propMap[ch][2] != -1){
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * drawstuff.xscale * scale;
			ah = (float)PROP_HEIGHT * drawstuff.yscale * scale;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset);
		}
		ax += (aw + (float)PROP_GAP_WIDTH * drawstuff.xscale * scale);
		s++;
	}
	trap_R_SetColor(nil);
}

/*
Sliceend=-1 means up to the terminating \0.
*/
int
propstrwidth(const char *str, int slicebegin, int sliceend)
{
	int ch, charWidth, width, i;

	width = 0;
	for(i = slicebegin; i < sliceend || (sliceend == -1 && str[i] != '\0'); i++){
		ch = str[i] & 127;
		charWidth = propMap[ch][2];
		if(charWidth != -1){
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
	}
	width -= PROP_GAP_WIDTH;
	return width;
}


float
propstrsizescale(int style)
{
	if(style & UI_SMALLFONT)
		return PROP_SMALL_SIZE_SCALE;
	return 1.00;
}

void
drawpropstr(int x, int y, const char *str, int style, vec4_t color)
{
	vec4_t drawcolor;
	int width;
	float scale;

	scale = propstrsizescale(style);

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		width = propstrwidth(str, 0, -1) * scale;
		x -= width / 2;
		break;
	case UI_RIGHT:
		width = propstrwidth(str, 0, -1) * scale;
		x -= width;
		break;
	case UI_LEFT:
	default:
		break;
	}
	if(style & UI_DROPSHADOW){
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3] * Shadowalpha;
		drawpropstr2(x+2, y+2, str, drawcolor, scale, drawstuff.charsetProp);
	}
	if(style & UI_INVERSE){
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		drawpropstr2(x, y, str, drawcolor, scale, drawstuff.charsetProp);
		return;
	}
	if(style & UI_PULSE){
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		drawpropstr2(x, y, str, color, scale, drawstuff.charsetProp);

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin(drawstuff.realtime / PULSE_DIVISOR);
		drawpropstr2(x, y, str, drawcolor, scale, drawstuff.charsetProp);
		return;
	}
	drawpropstr2(x, y, str, color, scale, drawstuff.charsetProp);
}

void
drawpropstrwrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color)
{
	int width;
	char *s1, *s2, *s3;
	char c;
	char buf[1024];
	float scale;

	if(!str || str[0]=='\0')
		return;

	scale = propstrsizescale(style);

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while(1){
		do
			s3++;
		while(*s3!=' ' && *s3!='\0');
		c = *s3;
		*s3 = '\0';
		width = propstrwidth(s1, 0, -1) * scale;
		*s3 = c;
		if(width > xmax){
			if(s1==s2)
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			*s2 = '\0';
			drawpropstr(x, y, s1, style, color);
			y += ystep;
			if(c == '\0'){
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if(*s2 != '\0')	// if we are printing an overflowing line we have s2 == s3
					drawpropstr(x, y, s2, style, color);
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}else{
			s2 = s3;
			if(c == '\0'){	// we reached the end
				drawpropstr(x, y, s1, style, color);
				break;
			}
		}
	}
}

static void
drawstr2(int x, int y, const char *str, vec4_t color, int charw, int charh)
{
	const char *s;
	char ch;
	int forceColor = qfalse;
	vec4_t tempcolor;
	float ax, ay, aw, ah, frow, fcol;

	if(y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * drawstuff.xscale + drawstuff.bias;
	ay = y * drawstuff.yscale;
	aw = charw * drawstuff.xscale;
	ah = charh * drawstuff.yscale;

	s = str;
	while(*s){
		if(Q_IsColorString(s)){
			if(!forceColor){
				memcpy(tempcolor, g_color_table[ColorIndex(s[1])],
				   sizeof(tempcolor));
				tempcolor[3] = color[3];
				trap_R_SetColor(tempcolor);
			}
			s += 2;
			continue;
		}
		ch = *s & 255;
		if(ch != ' '){
			frow = (ch>>4)*0.0625f;
			fcol = (ch&15)*0.0625f;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + 0.0625f,
			   frow + 0.0625f, drawstuff.charset);
		}
		ax += aw;
		s++;
	}
	trap_R_SetColor(nil);
}

void
drawstr(int x, int y, const char *str, int style, vec4_t color)
{
	int len, charw, charh;
	vec4_t newcolor, lowlight;
	vec4_t dropcolor;
	float *drawcolor;

	if(!str)
		return;

	if((style & UI_BLINK) && ((drawstuff.realtime/BLINK_DIVISOR) & 1))
		return;

	if(style & UI_SMALLFONT){
		charw = SMALLCHAR_WIDTH;
		charh = SMALLCHAR_HEIGHT;
	}else if(style & UI_GIANTFONT){
		charw = GIANTCHAR_WIDTH;
		charh = GIANTCHAR_HEIGHT;
	}else{
		charw = BIGCHAR_WIDTH;
		charh = BIGCHAR_HEIGHT;
	}

	if(style & UI_PULSE){
		lowlight[0] = 0.8f*color[0];
		lowlight[1] = 0.8f*color[1];
		lowlight[2] = 0.8f*color[2];
		lowlight[3] = 0.8f*color[3];
		lerpcolour(color, lowlight, newcolor, 0.5+0.5*sin(drawstuff.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}else
		drawcolor = color;

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		len = strlen(str);
		x = x - len*charw/2;
		break;
	case UI_RIGHT:
		len = strlen(str);
		x = x - len*charw;
		break;
	default:
		break;
	}
	if(style & UI_DROPSHADOW){
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3] * Shadowalpha;
		drawstr2(x+2, y+2, str, dropcolor, charw, charh);
	}
	drawstr2(x, y, str, drawcolor, charw, charh);
}

void
drawstrwrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color)
{
	int len, width, charw, charh;
	vec4_t newcolor, lowlight, dropcolor;
	float *drawcolor;
	char *s1, *s2, *s3;
	char buf[1024];
	int c;

	if(str == nil)
		return;

	Q_strncpyz(buf, str, sizeof buf);
	s1 = s2 = s3 = buf;

	if(style & UI_SMALLFONT){
		charw = SMALLCHAR_WIDTH;
		charh = SMALLCHAR_HEIGHT;
	}else if(style & UI_GIANTFONT){
		charw = GIANTCHAR_WIDTH;
		charh = GIANTCHAR_HEIGHT;
	}else{
		charw = BIGCHAR_WIDTH;
		charh = BIGCHAR_HEIGHT;
	}

	if(style & UI_PULSE){
		lowlight[0] = 0.8f*color[0];
		lowlight[1] = 0.8f*color[1];
		lowlight[2] = 0.8f*color[2];
		lowlight[3] = 0.8f*color[3];
		lerpcolour(color, lowlight, newcolor, 0.5+0.5*sin(drawstuff.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}else
		drawcolor = color;

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		len = strlen(str);
		x = x - len*charw/2;
		break;
	case UI_RIGHT:
		len = strlen(str);
		x = x - len*charw;
		break;
	default:
		break;
	}

	dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
	dropcolor[3] = color[3] * Shadowalpha;

	for(;;){
		do{
			s3++;
		}while(*s3 != ' ' && *s3 != '\0');
		c = *s3;
		*s3 = '\0';
		width = charw*strlen(s1);
		*s3 = c;
		if(width > xmax){
			if(s1 == s2)	// overflow
				s2 = s3;
			*s2 = '\0';
			if(style & UI_DROPSHADOW)
				drawstr2(x+2, y+2, s1, dropcolor, charw, charh);
			drawstr2(x, y, s1, drawcolor, charw, charh);
			y += ystep;
			if(c == '\0'){
				s2++;
				if(*s2 != '\0'){
					if(style & UI_DROPSHADOW)
						drawstr2(x+2, y+2, s2, dropcolor, charw, charh);
					drawstr2(x, y, s2, drawcolor, charw, charh);
				}
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}else{
			s2 = s3;
			if(c == '\0'){
				if(style & UI_DROPSHADOW)
					drawstr2(x+2, y+2, s1, dropcolor, charw, charh);
				drawstr2(x, y, s1, drawcolor, charw, charh);
				break;
			}
		}
	}
}

void
drawchar(int x, int y, int ch, int style, vec4_t color)
{
	char buff[2];

	buff[0] = ch;
	buff[1] = '\0';
	drawstr(x, y, buff, style, color);
}
