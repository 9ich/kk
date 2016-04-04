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
	{"Black",		0x000000},
	{"White",		0xffffff},
	{"Amethyst",		0x9966cc},
	{"Apple",		0x8db600},
	{"Aquamarine",		0x7fffd4},
	{"Blue",		0x273be2},
	{"Brown",		0x987654},
	{"Cream",		0xfffdd0},
	{"Cyan",		0x00ffff},
	{"DkBlue",		0x1034a6},
	{"DkGreen",		0x006b3c},
	{"DkGrey",		0x999999},
	{"DkLavender",		0x734f96},
	{"Green",		0x00ff7f},
	{"Indigo",		0x4b0082},
	{"LtBlue",		0x00bfff},
	{"LtGreen",		0x3fff00},
	{"LtGrey",		0xd3d3d3},
	{"LtMagenta",		0xf49ac2},
	{"LtOrange",		0xff9f00},
	{"Magenta",		0xff00ff},
	{"Orange",		0xff6600},
	{"Pink",		0xec3b83},
	{"Purple",		0x660099},
	{"Red",			0xe32636},
	{"Teal",		0x008080},
	{"Violet",		0x9f00ff},
	{"Yellow",		0xffbf00},
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
	defs(ctab, sizeof ctab/sizeof *ctab);
	return 0;
}
