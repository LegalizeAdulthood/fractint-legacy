  /* FRACTINT.H - common structures and values for the FRACTINT routines */

/* defines should match typelist indexes */
#define MANDEL      0
#define JULIA       1
#define NEWTBASIN   2
#define LAMBDA      3
#define MANDELFP    4
#define NEWTON      5
#define JULIAFP     6
#define PLASMA      7
#define LAMBDASINE  8
#define TEST        9


#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

char *typelist[] = 
    {"mandel","julia","newtbasin","lambda","mandelfp","newton",
     "juliafp","plasma","lambdasine","test",NULL};

#else

extern char *typelist[];

#endif

#define MAXPIXELS 2049		/* Maximum pixel count across/down the screen */

struct videoinfo {		/* All we need to know about a Video Adapter */
	char	name[25];	/* Adapter name (IBM EGA, etc)		*/
	int	videomodeax;	/* begin with INT 10H, AX=(this)	*/
	int	videomodebx;	/*		...and BX=(this)	*/
	int	videomodecx;	/*		...and CX=(this)	*/
	int	videomodedx;	/*		...and DX=(this)	*/
				/* NOTE:  IF AX==BX==CX==0, SEE BELOW	*/
	int	dotmode;	/* video access method used by asm code	*/
				/*	1 == BIOS 10H, AH=12,13 (SLOW)	*/
				/*	2 == access like EGA/VGA	*/
				/*	3 == access like MCGA		*/
				/*	4 == Tseng-like  SuperVGA*256	*/
				/*	5 == P'dise-like SuperVGA*256	*/
				/*	6 == Vega-like   SuperVGA*256	*/
				/*	7 == "Tweaked" IBM-VGA ...*256	*/
				/*	8 == "Tweaked" SuperVGA ...*256	*/
				/*	9 == Targa Format		*/
	int	xdots;		/* number of dots across the screen	*/
	int	ydots;		/* number of dots down the screen	*/
	int	colors;		/* number of colors available		*/
	char	comment[25];	/* Comments (UNTESTED, etc)		*/
	};

/* NOTE:  if videomode[abc]x == 0, 'setvideomode' assumes it has an IBM (or
	register compatable) adapter and tweaks the registers directly
	to get one of the following modes (based on the value of videomodedx):

		1		704 x 528 x 16
		2		720 x 540 x 16
		3		736 x 552 x 16
		4		752 x 564 x 16
		5		768 x 576 x 16
		6		784 x 588 x 16
		7		800 x 600 x 16
		8		360 x 480 x 16

*/

#define INFO_ID         "Fractal"
#define FRACTAL_INFO   struct fractal_info

struct fractal_info {			/*  for saving data in GIF file     */
	char info_id[8];		/* Unique identifier for info block */
	int iterations;
	int fractal_type;           /* 0=Mandelbrot 1=Julia 2= ... */
	double	xmin;
	double	xmax;
	double	ymin;
	double	ymax;
	double	creal;
	double	cimag;
	int	videomodeax;
	int	videomodebx;
	int	videomodecx;
	int	videomodedx;
	int	dotmode;
	int	xdots;
	int	ydots;
	int	colors;
	int	future[10];	/* for stuff we haven't thought of yet */
	};

#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

struct videoinfo videomode[76] = {
/*
	Feel free to add your favorite video adapter to the following table.
	Just remember that only the first 76 entries get displayed and
	assigned Function keys.

--Adapter/Mode--------------|---INT 10H---|Dot-|-Resolution-|------Comments-----------
-----Name-------------------|-AX--BX-CX-DX|Mode|X-|-Y-|Color|-------------------------
*/
"IBM Low-Rez EGA",           0x0d, 0, 0, 0, 2, 320, 200, 16, "Quick but chunky",
"IBM 16-Color EGA",          0x10, 0, 0, 0, 2, 640, 350, 16, "Slower but lots nicer",
"IBM 256-Color MCGA",        0x13, 0, 0, 0, 3, 320, 200,256, "Quick and LOTS of colors",
"IBM 16-Color VGA",          0x12, 0, 0, 0, 2, 640, 480, 16, "Nice high resolution",
"IBM 4-Color CGA",           0x05, 0, 0, 0, 1, 320, 200,  4, "(Ugh - Yuck - Bleah)",
"IBM Hi-Rez B&W CGA",        0x06, 0, 0, 0, 1, 640, 200,  2, "(Hi-Rez Ugh - Yuck)",
"IBM B&W EGA",               0x0f, 0, 0, 0, 2, 640, 350,  2, "(Monochrome EGA)",
"IBM B&W VGA",               0x11, 0, 0, 0, 2, 640, 480,  2, "(Monochrome VGA)",
"IBM Med-Rez EGA",           0x0e, 0, 0, 0, 2, 640, 200, 16, "(Silly but it's there!)",
"IBM VGA (non-std/no text)",    0, 0, 0, 8, 7, 360, 480,256, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 1, 2, 704, 528, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 2, 2, 720, 540, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 3, 2, 736, 552, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 4, 2, 752, 564, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 5, 2, 768, 576, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 6, 2, 784, 588, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",          0, 0, 0, 7, 2, 800, 600, 16, "Register Compatables ONLY",
"VESA Standard interface",   0x6a, 0, 0, 0, 2, 800, 600, 16, "UNTESTED: may not work",
"COMPAQ Portable 386",       0x40, 0, 0, 0, 1, 640, 400,  2, "OK: Michael Kaufman",
"Video-7 Vram VGA",        0x6f05,0x60,0,0, 2, 752, 410, 16, "OK: Ira Emus",
"Video-7 Vram VGA",        0x6f05,0x61,0,0, 2, 720, 540, 16, "OK: Ira Emus",
"Video-7 Vram VGA",        0x6f05,0x62,0,0, 2, 800, 600, 16, "OK: Ira Emus",
"Video-7 Vram VGA",        0x6f05,0x63,0,0, 1,1024, 768,  2, "OK: Ira Emus",
"Video-7 Vram VGA",        0x6f05,0x64,0,0, 1,1024, 768,  4, "OK: Ira Emus",
"Video-7 Vram VGA w/512K", 0x6f05,0x65,0,0, 1,1024, 768, 16, "OK: Ira Emus",
"Video-7 Vram VGA",        0x6f05,0x66,0,0, 6, 640, 400,256, "OK: Michael Kaufman",
"Video-7  w/512K ",        0x6f05,0x67,0,0, 6, 640, 480,256, "UNTESTED: may not work",
"Video-7  w/512K ",        0x6f05,0x68,0,0, 6, 720, 540,256, "UNTESTED: may not work",
"Video-7  w/512K ",        0x6f05,0x69,0,0, 6, 800, 600,256, "UNTESTED: may not work",
"Orchid/STB/GENOA/SIGMA",    0x2e, 0, 0, 0, 4, 640, 480,256, "OK: Monte Davis",
"Orchid/STB/GENOA/SIGMA",    0x29, 0, 0, 0, 2, 800, 600, 16, "OK: Monte Davis",
"Orchid/STB/GENOA/SIGMA",    0x30, 0, 0, 0, 4, 800, 600,256, "OK: Monte Davis",
"Orchid/STB/GENOA/SIGMA",    0x37, 0, 0, 0, 1,1024, 768, 16, "OK: David Mills",
"GENOA/STB",                 0x2d, 0, 0, 0, 4, 640, 350,256, "OK: Timothy Wegner",
"GENOA",                     0x27, 0, 0, 0, 2, 720, 512, 16, "OK: Timothy Wegner",
"GENOA",                     0x2f, 0, 0, 0, 4, 720, 512,256, "OK: Timothy Wegner",
"GENOA",                     0x7c, 0, 0, 0, 2, 512, 512, 16, "OK: Timothy Wegner",
"GENOA",                     0x7d, 0, 0, 0, 4, 512, 512,256, "OK: Timothy Wegner",
"STB",                       0x36, 0, 0, 0, 1, 960, 720, 16, "UNTESTED: may not work",
"Everex EVGA",               0x70, 0, 0, 0, 2, 640, 480, 16, "OK: Travis Harrison",
"Everex EVGA",               0x70, 1, 0, 0, 2, 752, 410, 16, "OK: Travis Harrison",
"Everex EVGA",               0x70, 2, 0, 0, 2, 800, 600, 16, "OK: Travis Harrison",
"Everex EVGA",               0x70,17, 0, 0, 1,1280, 350,  4, "OK: Travis Harrison",
"Everex EVGA",               0x70,18, 0, 0, 1,1280, 600,  4, "OK: Travis Harrison",
"Everex EVGA",               0x70,19, 0, 0, 1, 640, 350,256, "OK: Travis Harrison",
"Everex EVGA",               0x70,20, 0, 0, 1, 640, 400,256, "OK: Travis Harrison",
"Everex EVGA",               0x70,21, 0, 0, 1, 512, 480,256, "OK: Travis Harrison",
"ATI EGA Wonder",            0x51, 0, 0, 0, 1, 640, 480, 16, "UNTESTED: may not work",
"ATI EGA Wonder",            0x52, 0, 0, 0, 1, 800, 560, 16, "UNTESTED: may not work",
"ATI VGA Wonder",            0x54, 0, 0, 0, 2, 800, 600, 16, "OK: Henry So",
"ATI VGA Wonder",            0x61, 0, 0, 0, 1, 640, 400,256, "OK: Henry So",
"ATI VGA Wonder (512K)",     0x62, 0, 0, 0, 1, 640, 480,256, "OK: Henry So",
"ATI VGA Wonder (512K)",     0x63, 0, 0, 0, 1, 800, 600,256, "OK: Henry So",
"Paradise EGA-480",          0x50, 0, 0, 0, 1, 640, 480, 16, "UNTESTED: may not work",
"Pdise/AST/COMPAQ/DELL VGA", 0x5e, 0, 0, 0, 5, 640, 400,256, "UNTESTED: may not work",
"Pdise/AST/COMPAQ/DELL VGA", 0x5f, 0, 0, 0, 5, 640, 480,256, "UNTESTED: may not work",
"Pdise/AST/COMPAQ/DELL VGA", 0x58, 0, 0, 0, 2, 800, 600, 16, "OK: by Chris Green",
"Pdise/AST/COMPAQ/DELL VGA", 0x59, 0, 0, 0, 1, 800, 600,  2, "UNTESTED: may not work",
"AT&T 6300",                 0x41, 0, 0, 0, 1, 640, 200, 16, "UNTESTED: may not work",
"AT&T 6300",                 0x40, 0, 0, 0, 1, 640, 400,  2, "OK: Michael Kaufman",
"AT&T 6300",                 0x42, 0, 0, 0, 1, 640, 400, 16, "OK: Colby Norton",
"END",                          3, 0, 0, 0, 0,   0,   0,  0, "Marks END of the List"
	};

int	maxvideomode;		/* size of the above list */

#else

extern struct videoinfo videomode[];
extern int maxvideomode;

#endif

/*	help screens */

#define HELPAUTHORS	1
#define HELPMAIN	2
#define	HELPCYCLING	3
#define HELPMOUSE	4
#define	HELPCMDLINE	5
#define HELPFRACTALS    6
#define	HELPVIDEO	7
#define HELPMENU	98
#define	HELPEXIT	99

#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

int helpmode;

#else

extern int helpmode;

#endif

#define DEFAULTFRACTALTYPE	".fra"
#define	CRIPPLEDFRACTALTYPE	".gif"
