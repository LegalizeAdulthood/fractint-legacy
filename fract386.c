/*
	FRACT386 - 386-specific Mandelbrot/Julia Fractal Generator
	C-code		Version 4.0			   By Bert Tyler
					Mouse support by Michael Kaufman
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void main(int argc, char *argv[]);
int cputype(void);
int initasmvars(void);
int getakey(void);
void setvideomode(int a, int b, int c, int d);
int calcdots(void);
void drawbox(void);
void palette(int);

struct videoinfo {		/* All we need to know about a Video Adapter */
	char	name[40];	/* Adapter name (IBM EGA, etc)		*/
	int	videomodeax;	/* begin with INT 10H, AX=(this)	*/
	int	videomodebx;	/*		...and BX=(this)	*/
	int	videomodecx;	/*		...and CX=(this)	*/
	int	videomodedx;	/*		...and DX=(this)	*/
				/* NOTE:  IF AX==BX==CX==0, SEE BELOW	*/
	int	dotmode;	/* video access method used by asm code	*/
				/*	1 == BIOS 10H, AH=12,13 (SLOW)	*/
				/*	2 == access like EGA/VGA	*/
				/*	3 == access like MCGA		*/
	int	xdots;		/* number of dots across the screen	*/
	int	ydots;		/* number of dots down the screen	*/
	int	colors;		/* number of colors available		*/
	char	comment[40];	/* Comments (UNTESTED, etc)		*/
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

*/

struct videoinfo videomode[] = {
/*
	Feel free to add your favorite video adapter to the following table.
	Just remember that only the first 60 entries get displayed and
	assigned Function keys.

--Adapter/Mode------|---INT 10H---|Dot-|-Resolution-|------Comments-----------
-----Name-----------|-AX--BX-CX-DX|Mode|X-|-Y-|Color|-------------------------
*/
"IBM Low-Rez EGA",   0x0d, 0, 0, 0, 2, 320, 200, 16, "Quick, but chunky",
"IBM 16-Color EGA",  0x10, 0, 0, 0, 2, 640, 350, 16, "Slower, but lots nicer",
"IBM 256-Color MCGA",0x13, 0, 0, 0, 3, 320, 200,256, "Quick, LOTS of colors",
"IBM 16-Color VGA",  0x12, 0, 0, 0, 2, 640, 480, 16, "Nice, high resolution",
"IBM 4-Color CGA",   0x05, 0, 0, 0, 1, 320, 200,  4, "(Ugh, Yuck, Bleah)",
"IBM Hi-Rez B&W CGA",0x06, 0, 0, 0, 1, 640, 200,  2, "(Hi-Rez Ugh, Yuck)",
"IBM B&W EGA",       0x0f, 0, 0, 0, 2, 640, 350,  2, "(Monochrome EGA)",
"IBM B&W VGA",       0x11, 0, 0, 0, 2, 640, 480,  2, "(Monochrome VGA)",
"IBM Med-Rez EGA",   0x0e, 0, 0, 0, 2, 640, 200, 16, "(Silly, but there!)",
"IBM VGA (+tweaked+)",  0, 0, 0, 1, 2, 704, 528, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 2, 2, 720, 540, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 3, 2, 736, 552, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 4, 2, 752, 564, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 5, 2, 768, 576, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 6, 2, 784, 588, 16, "Register Compatables ONLY",
"IBM VGA (+tweaked+)",  0, 0, 0, 7, 2, 800, 600, 16, "Register Compatables ONLY",
"COMPAQ Portable 386",0x40,0, 0, 0, 1, 640, 400,  2, "TESTED OK by Michael Kaufman",
"COMPAQ, DELL VGA",  0x5e, 0, 0, 0, 1, 640, 400,256, "UNTESTED: may not work",
"COMPAQ, DELL VGA",  0x5f, 0, 0, 0, 1, 640, 480,256, "UNTESTED: may not work",
"COMPAQ, DELL VGA",  0x58, 0, 0, 0, 2, 800, 600, 16, "UNTESTED: may not work",
"COMPAQ, DELL VGA",  0x59, 0, 0, 0, 1, 800, 600,  2, "UNTESTED: may not work",
"Paradise, AST VGA", 0x5e, 0, 0, 0, 1, 640, 400,256, "UNTESTED: may not work",
"Paradise, AST VGA", 0x5f, 0, 0, 0, 1, 640, 480,256, "UNTESTED: may not work",
"Paradise, AST VGA", 0x58, 0, 0, 0, 2, 800, 600, 16, "Tested OK by Chris Green",
"Paradise, AST VGA", 0x59, 0, 0, 0, 1, 800, 600,  2, "UNTESTED: may not work",
"Paradise EGA-480",  0x50, 0, 0, 0, 1, 640, 480, 16, "UNTESTED: may not work",
"Video-7 Vram VGA", 0x6f05,0x60,0,0,2, 752, 410, 16, "Tested OK by Ira Emus",
"Video-7 Vram VGA", 0x6f05,0x61,0,0,2, 720, 540, 16, "Tested OK by Ira Emus",
"Video-7 Vram VGA", 0x6f05,0x62,0,0,2, 800, 600, 16, "Tested OK by Ira Emus",
"Video-7 Vram VGA", 0x6f05,0x63,0,0,1,1024, 768,  2, "Tested OK by Ira Emus",
"Video-7 Vram VGA", 0x6f05,0x64,0,0,1,1024, 768,  4, "Tested OK by Ira Emus",
"Video-7  w/512K ", 0x6f05,0x65,0,0,1,1024, 768, 16, "Tested OK by Ira Emus",
"Video-7 Vram VGA", 0x6f05,0x66,0,0,1, 640, 400,256, "Tested OK by Ira Emus",
"Video-7  w/512K ", 0x6f05,0x67,0,0,1, 640, 480,256, "Tested OK by Ira Emus",
"Video-7  w/512K ", 0x6f05,0x68,0,0,1, 720, 540,256, "Tested OK by Ira Emus",
"Video-7  w/512K ", 0x6f05,0x69,0,0,1, 800, 600,256, "Tested OK by Ira Emus",
"Orchid, STB (VEGA?)",0x2d,0, 0, 0, 1, 640, 350,256, "UNTESTED: may not work",
"Orchid, STB (VEGA?)",0x2e,0, 0, 0, 1, 640, 480,256, "UNTESTED: may not work",
"Orchid, STB (VEGA?)",0x29,0, 0, 0, 2, 800, 600, 16, "UNTESTED: may not work",
"Orchid, STB (VEGA?)",0x30,0, 0, 0, 1, 800, 600,256, "UNTESTED: may not work",
"Orchid, STB (VEGA?)",0x37,0, 0, 0, 1,1024, 768, 16, "Tested OK (STB) by David Mills",
"GENOA"             ,0x2d, 0, 0, 0, 1, 640, 350,256, "UNTESTED: may not work",
"GENOA, SIGMA"      ,0x2e, 0, 0, 0, 1, 640, 480,256, "UNTESTED: may not work",
"GENOA, SIGMA"      ,0x29, 0, 0, 0, 2, 800, 600, 16, "UNTESTED: may not work",
"GENOA, SIGMA"      ,0x30, 0, 0, 0, 1, 800, 600,256, "UNTESTED: may not work",
"GENOA"             ,0x37, 0, 0, 0, 1,1024, 768, 16, "UNTESTED: may not work",
"STB (VEGA?)"       ,0x36, 0, 0, 0, 1, 960, 720, 16, "UNTESTED: may not work",
"VEGA VGA (?)",      0x5e, 0, 0, 0, 1, 640, 400,256, "UNTESTED: may not work",
"VEGA VGA (?)",      0x27, 0, 0, 0, 1, 720, 512, 16, "UNTESTED: may not work",
"VEGA VGA (?)",      0x2f, 0, 0, 0, 1, 720, 512,256, "UNTESTED: may not work",
"ATI EGA Wonder",    0x51, 0, 0, 0, 1, 640, 480, 16, "UNTESTED: may not work",
"ATI EGA Wonder",    0x52, 0, 0, 0, 1, 800, 560, 16, "UNTESTED: may not work",
"AT&T 6300",         0x41, 0, 0, 0, 1, 640, 200, 16, "UNTESTED: may not work",
"AT&T 6300",         0x40, 0, 0, 0, 1, 640, 400,  2, "TESTED OK by Michael Kaufman",
"AT&T 6300",         0x42, 0, 0, 0, 1, 640, 400, 16, "UNTESTED: may not work",
"Everex EVGA",       0x70, 2, 0, 0, 2, 800, 600, 16, "UNTESTED: may not work",
"Everex EVGA",       0x70,19, 0, 0, 1, 512, 480,256, "UNTESTED: may not work",
"Everex EVGA",       0x70,21, 0, 0, 1, 640, 350,256, "UNTESTED: may not work",
"END",                  3, 0, 0, 0, 0,   0,   0,  0, "Marks END of the List"
	};

int	maxvideomode;		/* size of the above list */
int	adapter;		/* Video Adapter chosen from above list	*/

char *fkeys[] = {		/* Function Key names for display table */
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"SF1","SF2","SF3","SF4","SF5","SF6","SF7","SF8","SF9","SF10",
	"CF1","CF2","CF3","CF4","CF5","CF6","CF7","CF8","CF9","CF10",
	"AF1","AF2","AF3","AF4","AF5","AF6","AF7","AF8","AF9","AF10",
	"Alt-1","Alt-2","Alt-3","Alt-4","Alt-5",
	"Alt-6","Alt-7","Alt-8","Alt-9","Alt-0",
	"Alt-Q","Alt-W","Alt-E","Alt-R","Alt-T",
	"Alt-Y","Alt-U","Alt-I","Alt-O","Alt-P",
	"END"};
char *accessmethod[] = {" ","B"," "," "}; /* BIOS access indicator */

#define	FUDGEFACTOR	29		/* fudge all values up by 2**this */
#define MAXHISTORY	100		/* save this many historical rcds */

long	history[MAXHISTORY][6];	/* for historical (HOME Key) purposes */

/*
   the following variables are out here only so
   that the assembler routine can get at them easily
*/
	int	dotmode;			/* video access method      */
						/* 1 == use the BIOS (ugh)  */
						/* 2 == access like EGA/VGA */
						/* 3 == access like MCGA    */
	int	xdots, ydots;			/* # of dots on the screen  */
	int	colors;				/* maximum colors available */
	int	maxit;				/* try this many iterations */
	int	ixmin, ixmax, iymin, iymax;	/* corners of the zoom box  */

	int	julia;				/* if == 0, use Mandelbrot  */
	int	numpasses;			/* 0 if single-pass, else 1 */
	long	creal, cimag;			/* real, imag'ry parts of C */
	long	lx0[2048], ly0[2048];		/* x, y grid (2K x 2K max)  */

void main(argc,argv)
int argc;
char *argv[];
{
	long	jxmin, jxmax, jymin, jymax;	/* "Julia mode" entry point */
	long	xmin, xmax, ymin, ymax;		/* screen corner values */
	long	delx, dely;			/* screen pixel increments */
	long	fudge;				/* 2**fudgefactor	*/
	double	atof(),xxmin,xxmax,yymin,yymax;	/* (printf) screen corners */
	double	ccreal, ccimag;			/* Julia Set Constant */
	int	xstep, ystep;			/* zoom-box increment values */
	int	axmode, bxmode, cxmode, dxmode;	/* video mode (BIOS ##) */
	int	historyptr;			/* pointer into history tbl */
	int	zoomoff;			/* = 0 when zoom is disabled */
	int	kbdchar;			/* keyboard key-hit value */
	int	more, kbdmore;			/* continuation variables */
	int	i,j;				/* temporary loop counters */

if ((i=abs(cputype())) != 386) {		/* oops. not a 386 */
	printf("\n\n\007");
	printf(" I'm sorry.   This program requires an 80386-based PC, and my\n");
	printf(" poking around has determined that this is more like an 80%d.", i);
	printf("\n\n");
	exit(1);
	}

initasmvars();	     /* this function sets up the various internal asm vars */

for (maxvideomode = 0;				/* get size of adapter list */
	strcmp(videomode[maxvideomode].name,"END") != 0
		&& maxvideomode < 60;	/* that's all the Fn keys we got! */
	maxvideomode++);

fudge = 1; fudge = fudge << FUDGEFACTOR;	/* fudged value for printfs */

restart:				/* insert key re-starts here */

maxit = 150;					/* max iterations */
numpasses = 0;					/* single-pass mode */

julia = 0;					/* use the Mandelbrot set */
creal = 0; cimag = 0;				/* dummy C-values         */
						/* grid for full Mandel set */
xmax =  4; xmax = xmax << (FUDGEFACTOR-2);	/* xmax == 1.0		*/
xmin = -8; xmin = xmin << (FUDGEFACTOR-2);	/* xmin == -2.0		*/
ymax =  6; ymax = ymax << (FUDGEFACTOR-2);	/* ymax == 1.5		*/
ymin = -6; ymin = ymin << (FUDGEFACTOR-2);	/* ymin == -1.5		*/

if (argc == 3 || argc == 7) {			/* Julia Set Override */
	julia = 1;
	ccreal = atof(argv[1]);
	ccimag = atof(argv[2]);
	if (ccreal < -1.99 || ccreal > 1.99) ccreal = .0001;
	if (ccimag < -1.99 || ccimag > 1.99) ccimag = .0001;
	ccreal = ccreal * fudge; creal = ccreal;
	ccimag = ccimag * fudge; cimag = ccimag;
	jxmin = xmin;  jxmax = xmax;  jymin = ymin;  jymax = ymax;
	xmax =  7; xmax = xmax << (FUDGEFACTOR-2);	/* xmax =  1.75 */
	xmin = -7; xmin = xmin << (FUDGEFACTOR-2);	/* xmin = -1.75 */
	ymax =  7; ymax = ymax << (FUDGEFACTOR-2);	/* ymax =  1.75 */
	ymin = -7; ymin = ymin << (FUDGEFACTOR-2);	/* ymin = -1.75 */
	}

if (argc == 5 || argc == 7) {			/* starting grid override */
	xxmin = atof(argv[argc-4]);
	xxmax = atof(argv[argc-3]);
	yymin = atof(argv[argc-2]);
	yymax = atof(argv[argc-1]);
	/* keep it reasonable, guys  -- max-min MUST be < 4 */
	if (xxmin < -2.0  || xxmin > 1.99) xxmin = -2.0;
	if (xxmax < -2.0  || xxmax > 1.99) xxmax =  1.99;
	if (yymin < -1.99 || yymin > 1.99) yymin = -1.99;
	if (yymax < -1.99 || yymax > 1.99) yymax =  1.99;
	xxmin = xxmin * fudge; xmin = xxmin;
	xxmax = xxmax * fudge; xmax = xxmax + 100;
	yymin = yymin * fudge; ymin = yymin - 100;
	yymax = yymax * fudge; ymax = yymax;
	}

setvideomode(3,0,0,0);			/* switch to text mode */

printf("\n\n\n");
printf("FRACT386          Version 4.0                                  by Bert Tyler\n");
printf("                                            Mouse Support by Michael Kaufman\n");
printf("Some of the useful keys you can hit while this program is running are:\n");
printf("       (For a complete list, and the Mouse Commands, see the README file)\n");
printf("  PageUp            Shrink the Zoom Box (Zoom in)  \n");
printf("  PageDown          Expand the Zoom Box (Zoom out) \n");
printf("  Cursor Keys       Move the Zoom Box around (Panning)\n");
printf("  Ctrl-Cursor-Keys  Pan as above, but quickly(may require an Enhanced KBD)\n");
printf("  End or Enter      Redraw the Screen or area inside the Zoom Box \n");
printf("  F1,F2,F3,F4...    Select a new Video Mode and THEN Redraw\n");
printf("                    (See Instructions Below for a complete Video Mode list)\n");
printf("  Home              Redraw Previous screen (you can 'back out' recursively)\n");
printf("  Tab               Display the Screen or Zoom-Box X and Y Coordinates\n");
printf("  Spacebar          Mandelbrot/Julia Set toggle (read the README file first)\n");
printf("  Insert            Restart the program (at this screen)\n");
printf("  Delete or Esc     Stop the program and return to MSDOS\n");
printf("\n");
printf("Hitting any key while the program is drawing will stop the drawing immediately\n");
printf("to perform the function.   This means you can begin Zooming and Panning (or\n");
printf("anything else) as soon as you see something interesting.  If you haven't yet\n");
printf("hit a key when the screen is finished, the program will beep and wait for you.\n");
printf("\n");
printf("Please Select your Video Mode by hitting a Function Key (F1, F2, F3, F4,...) \n");
printf("    (Or hit the <ENTER> key to see a complete list of supported Video modes) \n");
printf("The program will begin with the full Mandelbrot Set as a starting point. ");

j = maxvideomode;				/* init for video modes scan */
for (;;) {					/* wait for valid Fn Key */
	kbdchar = getakey();
	if (kbdchar >= 1059 && kbdchar < 1069	/* F1 - F10 */
	 && kbdchar < 1059 + maxvideomode) {
		adapter = kbdchar - 1059;	/* use this adapter initially */
		break;
		}
	if (kbdchar >= 1084 && kbdchar < 1114	/* SF1 - AF10 */
	 && kbdchar < 1074 + maxvideomode) {
		adapter = kbdchar - 1074;	/* use this adapter initially */
		break;
		}
	if (kbdchar >= 1120 && kbdchar < 1130	/* Alt-1 thru Alt-0 */
	 && kbdchar < 1080 + maxvideomode) {
		adapter = kbdchar - 1080;	/* use this adapter initially */
		break;
		}
	if (kbdchar >= 1016 && kbdchar < 1026	/* Alt-Q thru Alt-P */
	 && kbdchar < 966 + maxvideomode) {
		adapter = kbdchar - 966;	/* use this adapter initially */
		break;
		}
	if (kbdchar == 1082) goto restart;	/* restart pgm on Insert Key */
	if (kbdchar == 1000) exit(0);		/* Control-C */
	if (kbdchar == 27) exit(0);		/* exit to DOS on Escape Key */
	if (kbdchar == 1083) exit(0);		/* exit to DOS on Delete Key */
	if (kbdchar == '1' || kbdchar == '2') {	/* select single or dual-pass */
		numpasses = kbdchar - '1';
		continue;
		}
	if (kbdchar == 13) {			/* <ENTER> key: display adapter list */
		j += 14;			/* display next page */
		if (j >= maxvideomode) j = 0;	/* (or 1st page)     */
		printf("\n\n\nThe current list of supported Video Adapters and Modes includes:\n\n");
		printf("      %-20s   Resolution Colors   %-30s\n\n",
			"Video Adapter & Mode", "     Comments");
		for (i = j; i < maxvideomode && i < j + 14; i++) {
			printf("%-5s %-20s%6d x%4d%5d %1s   %-30s\n",fkeys[i],
				videomode[i].name,
				videomode[i].xdots,
				videomode[i].ydots,
				videomode[i].colors,
				accessmethod[videomode[i].dotmode],
				videomode[i].comment
				);
			}
		for (; i < j+15; i++) printf("\n");
printf("    (Hit the <ENTER> key to see another page of supported Video modes) \n\n");
printf("Please Select your Video Mode by hitting a Function Key (F1, F2, F3, F4,...) \n");
printf("    (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) \n");
printf("    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)\n");
printf("The program will begin with the full Mandelbrot Set as a starting point. ");
		}
	}

historyptr = 0;					/* initialize history ptr */
history[0][0] = -1;
zoomoff = 1;					/* zooming is enabled */

more = 1;
while (more) {					/* eternal loop */

						/* collect adapter info */
	axmode  = videomode[adapter].videomodeax; /* video mode (BIOS call) */
	bxmode  = videomode[adapter].videomodebx; /* video mode (BIOS call) */
	cxmode  = videomode[adapter].videomodecx; /* video mode (BIOS call) */
	dxmode  = videomode[adapter].videomodedx; /* video mode (BIOS call) */
	dotmode = videomode[adapter].dotmode;	/* assembler dot read/write */
	xdots   = videomode[adapter].xdots;	/* # dots across the screen */
	ydots   = videomode[adapter].ydots;	/* # dots down the screen   */
	colors  = videomode[adapter].colors;	/* # colors available */

	xstep   = xdots / 40;			/* zoom-box increment: across */
	ystep   = ydots / 40;			/* zoom-box increment: down */
	if (xdots == 640 && ydots == 350)	/* zoom-box adjust:  640x350 */
		{ xstep = 16; ystep =  9; }
	if (xdots == 720 && ydots == 512)	/* zoom-box adjust:  720x512 */
		{ xstep = 20; ystep = 15; }
	if (xdots * 3 == ydots * 4)		/* zoom-box adjust:  VGA Tweaks */
		{ xstep = 16; ystep = 12; }
	if (xdots == 1024 && ydots == 768)	/* zoom-box adjust: 1024x768 */
		{ xstep = 32; ystep = 24; }

	delx = ((xmax - xmin) / (xdots - 1));	/* calculate the stepsizes */
	dely = ((ymax - ymin) / (ydots - 1));

	if (delx <= 0 || dely <= 0) {		/* oops.  zoomed too far */
		zoomoff = 0;
		dely = 1;
		delx = 1;
		}

	lx0[0] = xmin;				/* fill up the x, y grids */
	ly0[0] = ymax;
	for (i = 1; i < xdots; i++ )
		lx0[i] = lx0[i-1] + delx;
	for (i = 1; i < ydots; i++ )
		ly0[i] = ly0[i-1] - dely;

	if (zoomoff == 0) {
		xmax = lx0[xdots-1];		/* re-set xmax and ymax */
		ymin = ly0[ydots-1];
		}

	if (history[0][0] == -1)			/* initialize the history file */
		for (i = 0; i < MAXHISTORY; i++) {
			history[i][0] = xmax;
			history[i][1] = xmin;
			history[i][2] = ymax;
			history[i][3] = ymin;
			history[i][4] = creal;
			history[i][5] = cimag;
		}

	if (history[historyptr][0] != xmax  ||	/* save any (new) zoom data */
	    history[historyptr][1] != xmin  ||
	    history[historyptr][2] != ymax  ||
	    history[historyptr][3] != ymin  ||
	    history[historyptr][4] != creal ||
	    history[historyptr][5] != cimag ) {
		if (++historyptr == MAXHISTORY) historyptr = 0;
		history[historyptr][0] = xmax;
		history[historyptr][1] = xmin;
		history[historyptr][2] = ymax;
		history[historyptr][3] = ymin;
		history[historyptr][4] = creal;
		history[historyptr][5] = cimag;
		}

	setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */

	if (calcdots() == 0)			/* draw the fractal */
		printf("\007");			/* finished!  wake up! */

	ixmin = 0;  ixmax = xdots-1;		/* initial zoom box */
	iymin = 0;  iymax = ydots-1;

	kbdmore = 1;
	while (kbdmore == 1) {			/* loop through cursor keys */
		kbdchar = getakey();
		switch (kbdchar) {
			case 32:			/* spacebar */
				if (julia == 0) {	/* switch to Julia */
					julia = 1;
					creal = (xmax + xmin) / 2;
					cimag = (ymax + ymin) / 2;
					jxmin = xmin;  jxmax = xmax;
					jymin = ymin;  jymax = ymax;
					xmax =  7; 	/* xmax ==  1.75 */
 					xmin = -7; 	/* xmin == -1.75 */
					ymax =  7; 	/* ymax ==  1.75 */
					ymin = -7; 	/* ymin == -1.75 */
					xmax = xmax << (FUDGEFACTOR-2);
					xmin = xmin << (FUDGEFACTOR-2);
					ymax = ymax << (FUDGEFACTOR-2);
					ymin = ymin << (FUDGEFACTOR-2);
					}
				else	{		/* switch to Mandel */
					julia = 0;
					creal = 0;
					cimag = 0;
					xmin = jxmin;  xmax = jxmax;
					ymin = jymin;  ymax = jymax;
					}
				kbdmore = 0;
				break;
			case 1071:			/* home */
				if (--historyptr < 0)
					historyptr = MAXHISTORY-1;
				xmax  = history[historyptr][0];
				xmin  = history[historyptr][1];
				ymax  = history[historyptr][2];
				ymin  = history[historyptr][3];
				creal = history[historyptr][4];
				cimag = history[historyptr][5];
				julia = 0;
				if (creal != 0 || cimag != 0) julia = 1;
				zoomoff = 1;
				kbdmore = 0;
				break;
			case 9:				/* tab */
				ccreal = creal; ccreal = ccreal / fudge;
				ccimag = cimag; ccimag = ccimag / fudge;
				xxmax = xmax; xxmax = xxmax / fudge;
				xxmin = xmin; xxmin = xxmin / fudge;
				yymax = ymax; yymax = yymax / fudge;
				yymin = ymin; yymin = yymin / fudge;
				if (julia == 0 )
					printf("The Current Mandelbrot Boundaries are: \n");
				else	{
					printf("The Current Julia Set Boundaries are: \n");
					printf("    Creal = %12.9f \n",ccreal);
					printf("    Cimag = %12.9f \n",ccimag);
					}
				printf("     Xmin = %12.9f \n",xxmin);
				printf("     Xmax = %12.9f \n",xxmax);
				printf("     Ymin = %12.9f \n",yymin);
				printf("     Ymax = %12.9f \n",yymax);
				continue;	
			break;
			case '<':			/* lower iter maximum */
			case ',':
				if (maxit > 50) maxit -= 50;
				maxit -= 50;		/* for fall-thru */
			case '>':			/* raise iter maximum */
			case '.':
				if (maxit < 1000) maxit += 50;
				printf("The Iteration Maximum is now %d\n", maxit);
				continue;
				break;
			case '+':			/* rotate palette */
				palette(+1);
				continue;
				break;
			case '-':			/* rotate palette */
				palette(-1);
				continue;
				break;
			case '1':			/* single-pass mode */
			case '2':			/* dual-pass mode */
				numpasses = kbdchar - '1';
				kbdmore = 0;
				break;
			case 13:			/* Enter */
			case 1079:			/* end */
				kbdmore = 0;
				break;
			case 1082:			/* insert */
				goto restart;
				break;
			case 1000:			/* Control-C */
			case 27:			/* Escape */
			case 1083:			/* delete */
				more = 0; kbdmore = 0;
				break;
			case 1075:			/* cursor left */
				if (zoomoff == 1 && ixmin >= 1)
					{ ixmin -= 1; ixmax -= 1; }
					break;
			case 1077:			/* cursor right */
				if (zoomoff == 1 && ixmax < xdots - 1)
					{ ixmin += 1; ixmax += 1; }
					break;
			case 1072:			/* cursor up */
				if (zoomoff == 1 && iymin >= 1)
					{ iymin -= 1; iymax -= 1; }
					break;
			case 1080:			/* cursor down */
				if (zoomoff == 1 && iymax < ydots - 1)
					{ iymin += 1; iymax += 1; }
					break;
			case 1115:			/* Ctrl-cursor left */
				if (zoomoff == 1 && ixmin >= 5)
					{ ixmin -= 5; ixmax -= 5; }
					break;
			case 1116:			/* Ctrl-cursor right */
				if (zoomoff == 1 && ixmax < xdots - 5)
					{ ixmin += 5; ixmax += 5; }
					break;
			case 1141:			/* Ctrl-cursor up */
				if (zoomoff == 1 && iymin >= 5)
					{ iymin -= 5; iymax -= 5; }
					break;
			case 1145:			/* Ctrl-cursor down */
				if (zoomoff == 1 && iymax < ydots - 5)
					{ iymin += 5; iymax += 5; }
					break;
			case 1073:			/* page up */
				if (zoomoff == 1 
				  && ixmax - ixmin > 3 * xstep
				  && iymax - iymin > 3 * ystep) {
					/* 640x350 Zoom-In Klooge:  Adjust the
					   Zoom-Box on the initial Zoom-In */
					if (xdots == 640 && ydots == 350
					  && iymin == 0  && iymax == ydots-1) {
						iymin -= 5;
						iymax += 5;
						}
					/* 720x512 Zoom-In Klooge:  Adjust the
					   Zoom-Box on the initial Zoom-In */
					if (xdots == 720 && ydots == 512
					  && iymin == 0  && iymax == ydots-1) {
						iymin -= 14;
						iymax += 14;
						}
					ixmin += xstep;	ixmax -= xstep;
					iymin += ystep;	iymax -= ystep;
					}
					break;
			case 1081:			/* page down */
				if (zoomoff == 1
				 && ixmin >= xstep && ixmax < xdots - xstep
				 && iymin >= ystep && iymax < ydots - ystep) {
					ixmin -= xstep;	ixmax += xstep;
					iymin -= ystep;	iymax += ystep;
					}
					break;
			default:		/* other (maybe a valid Fn key) */
				if (kbdchar >= 1059 && kbdchar < 1069
				 && kbdchar < 1059 + maxvideomode) {
					adapter = kbdchar - 1059;
					kbdmore = 0;
					}
				else if (kbdchar >= 1084 && kbdchar < 1114
				 && kbdchar < 1074 + maxvideomode) {
					adapter = kbdchar - 1074;
					kbdmore = 0;
					}
				else if (kbdchar >= 1120 && kbdchar < 1130
				 && kbdchar < 1080 + maxvideomode) {
					adapter = kbdchar - 1080;
					kbdmore = 0;
					}
				else if (kbdchar >= 1016 && kbdchar < 1026
				 && kbdchar < 966 + maxvideomode) {
					adapter = kbdchar - 966;
					kbdmore = 0;
					}
				else
					continue;
				break;
			}

		if (zoomoff == 1 && kbdmore == 1) {	/* draw a zoom box? */
			xmin = lx0[ixmin];
			xmax = lx0[ixmax];
			ymin = ly0[iymax];
			ymax = ly0[iymin];
			drawbox();			/* (yup) */
			}
		}

	}

setvideomode(3,0,0,0);			/* all done.  return to text mode. */

}