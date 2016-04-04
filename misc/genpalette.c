/*
 * Converts the list of RGB colours below to vec4_t and writes
 * their declarations and definitions to standard output.
 *
 * genpalette | unexpand -a >palette.c
 */
 
#include <stdio.h>

typedef unsigned char	uchar;
typedef unsigned long	ulong;
typedef struct Clr	Clr;

struct Clr
{
	char	*name;
	ulong	c;
};

static Clr ctab[] = {
	{"AliceBlue",	0xF0F8FF},
	{"AntiqueWhite",	0xFAEBD7},
	{"Aqua",	0x00FFFF},
	{"Aquamarine",	0x7FFFD4},
	{"Azure",	0xF0FFFF},
	{"Beige",	0xF5F5DC},
	{"Bisque",	0xFFE4C4},
	{"Black",	0x000000},
	{"BlanchedAlmond",	0xFFEBCD},
	{"Blue",	0x0000FF},
	{"BlueViolet",	0x8A2BE2},
	{"Brown",	0xA52A2A},
	{"BurlyWood",	0xDEB887},
	{"CadetBlue",	0x5F9EA0},
	{"Chartreuse",	0x7FFF00},
	{"Chocolate",	0xD2691E},
	{"Coral",	0xFF7F50},
	{"CornflowerBlue",	0x6495ED},
	{"Cornsilk",	0xFFF8DC},
	{"Crimson",	0xDC143C},
	{"Cyan",	0x00FFFF},
	{"DarkBlue",	0x00008B},
	{"DarkCyan",	0x008B8B},
	{"DarkGoldenrod",	0xB8860B},
	{"DarkGray",	0xA9A9A9},
	{"DarkGreen",	0x006400},
	{"DarkKhaki",	0xBDB76B},
	{"DarkMagenta",	0x8B008B},
	{"DarkOliveGreen",	0x556B2F},
	{"DarkOrange",	0xFF8C00},
	{"DarkOrchid",	0x9932CC},
	{"DarkRed",	0x8B0000},
	{"DarkSalmon",	0xE9967A},
	{"DarkSeaGreen",	0x8FBC8F},
	{"DarkSlateBlue",	0x483D8B},
	{"DarkSlateGray",	0x2F4F4F},
	{"DarkTurquoise",	0x00CED1},
	{"DarkViolet",	0x9400D3},
	{"DeepPink",	0xFF1493},
	{"DeepSkyBlue",	0x00BFFF},
	{"DimGray",	0x696969},
	{"DodgerBlue",	0x1E90FF},
	{"FireBrick",	0xB22222},
	{"FloralWhite",	0xFFFAF0},
	{"ForestGreen",	0x228B22},
	{"Fuchsia",	0xFF00FF},
	{"Gainsboro",	0xDCDCDC},
	{"GhostWhite",	0xF8F8FF},
	{"Gold",	0xFFD700},
	{"Goldenrod",	0xDAA520},
	{"Gray",	0x808080},
	{"Green",	0x008000},
	{"GreenYellow",	0xADFF2F},
	{"Honeydew",	0xF0FFF0},
	{"HotPink",	0xFF69B4},
	{"IndianRed",	0xCD5C5C},
	{"Indigo",	0x4B0082},
	{"Ivory",	0xFFFFF0},
	{"Khaki",	0xF0E68C},
	{"Lavender",	0xE6E6FA},
	{"LavenderBlush",	0xFFF0F5},
	{"LawnGreen",	0x7CFC00},
	{"LemonChiffon",	0xFFFACD},
	{"LightBlue",	0xADD8E6},
	{"LightCoral",	0xF08080},
	{"LightCyan",	0xE0FFFF},
	{"LightGoldenrodYellow",	0xFAFAD2},
	{"LightGreen",	0x90EE90},
	{"LightGrey",	0xD3D3D3},
	{"LightSalmon",	0xFFA07A},
	{"LightSeaGreen",	0x20B2AA},
	{"LightSkyBlue",	0x87CEFA},
	{"LightSlateGray",	0x778899},
	{"LightSteelBlue",	0xB0C4DE},
	{"LightYellow",	0xFFFFE0},
	{"Lime",	0x00FF00},
	{"LimeGreen",	0x32CD32},
	{"Linen",	0xFAF0E6},
	{"Magenta",	0xFF00FF},
	{"Maroon",	0x800000},
	{"MediumAquamarine",	0x66CDAA},
	{"MediumBlue",	0x0000CD},
	{"MediumOrchid",	0xBA55D3},
	{"MediumPurple",	0x9370DB},
	{"MediumSeaGreen",	0x3CB371},
	{"MediumSlateBlue",	0x7B68EE},
	{"MediumSpringGreen",	0x00FA9A},
	{"MediumTurquoise",	0x48D1CC},
	{"MediumVioletRed",	0xC71585},
	{"MidnightBlue",	0x191970},
	{"MintCream",	0xF5FFFA},
	{"MistyRose",	0xFFE4E1},
	{"Moccasin",	0xFFE4B5},
	{"NavajoWhite",	0xFFDEAD},
	{"Navy",	0x000080},
	{"OldLace",	0xFDF5E6},
	{"Olive",	0x808000},
	{"OliveDrab",	0x6B8E23},
	{"Orange",	0xFFA500},
	{"OrangeRed",	0xFF4500},
	{"Orchid",	0xDA70D6},
	{"PaleGoldenrod",	0xEEE8AA},
	{"PaleGreen",	0x98FB98},
	{"PaleTurquoise",	0xAFEEEE},
	{"PaleVioletRed",	0xDB7093},
	{"PapayaWhip",	0xFFEFD5},
	{"PeachPuff",	0xFFDAB9},
	{"Peru",	0xCD853F},
	{"Pink",	0xFFC0CB},
	{"Plum",	0xDDA0DD},
	{"PowderBlue",	0xB0E0E6},
	{"Purple",	0x800080},
	{"Red",		0xFF0000},
	{"RosyBrown",	0xBC8F8F},
	{"RoyalBlue",	0x4169E1},
	{"SaddleBrown",	0x8B4513},
	{"Salmon",	0xFA8072},
	{"SandyBrown",	0xF4A460},
	{"SeaGreen",	0x2E8B57},
	{"Seashell",	0xFFF5EE},
	{"Sienna",	0xA0522D},
	{"Silver",	0xC0C0C0},
	{"SkyBlue",	0x87CEEB},
	{"SlateBlue",	0x6A5ACD},
	{"SlateGray",	0x708090},
	{"Snow",	0xFFFAFA},
	{"SpringGreen",	0x00FF7F},
	{"SteelBlue",	0x4682B4},
	{"Tan",		0xD2B48C},
	{"Teal",	0x008080},
	{"Thistle",	0xD8BFD8},
	{"Tomato",	0xFF6347},
	{"Turquoise",	0x40E0D0},
	{"Violet",	0xEE82EE},
	{"Wheat",	0xF5DEB3},
	{"White",	0xFFFFFF},
	{"WhiteSmoke",	0xF5F5F5},
	{"Yellow",	0xFFFF00},
	{"YellowGreen",	0x9ACD32},
	{"LightPink",	0xFFB6C1},
};

static void
conv(ulong c, double v[])
{
	const double s = 1.0/0xff;
	uchar r, g, b;

	b = c & 0xff;
	g = (c>>8) & 0xff;
	r = (c>>16) & 0xff;
	v[0] = r*s;
	v[1] = g*s;
	v[2] = b*s;
	v[3] = 1.0;
}

static void
decls(Clr *tab, int n)
{
	int i;

	for(i = 0; i < n; i++)
		printf("extern vec4_t C%s;\n", tab[i].name);
}

static void
defs(Clr *tab, int n)
{
	double v[4];
	int i;

	for(i = 0; i < n; i++){
		conv(tab[i].c, v);
		printf("vec4_t C%-16s= {%1.3ff, %1.3ff, %1.3ff, %1.3ff};\n",
		   tab[i].name, v[0], v[1], v[2], v[3]);
	}
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	decls(ctab, sizeof ctab/sizeof *ctab);
	printf("\n");
	printf("// https://www.colorcodehex.com/html-color-names.html\n");
	printf("// http://cng.seas.rochester.edu/CNG/docs/x11color.html\n\n");
	defs(ctab, sizeof ctab/sizeof *ctab);
	return 0;
}
