/*
	FRACTINT - The Ultimate Fractal Generator
			Main Routine
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <float.h>
#include <process.h>
#include <time.h>
#ifndef __TURBOC__
#include <malloc.h>
#endif

#define PUTTHEMHERE 1			/* stuff common external arrays here */

#include "fractint.h"
#include "fractype.h"

int	adapter;		/* Video Adapter chosen from list in ...h */

extern int video_type;		/* coded value indicating video adapter type */
extern int tgaview();
extern int gifview();
struct fractal_info save_info, read_info; /*  for saving data in file */
extern int biomorph;
extern int askvideo;
extern int forcesymmetry;
extern	char	readname[];	/* name of fractal input file */
extern	int	showfile;	 /* has file been displayed yet? */
#define FUDGEFACTOR	29		/* fudge all values up by 2**this */
#define FUDGEFACTOR2	24		/* (or maybe this)		  */

#define MAXHISTORY	25		/* save this many historical rcds */
struct historystruct {			/* history structure */
	int fractype;			/* fractal type */
	double param[4];		/* parameters */
	double xxmax;			/* right-most  point */
	double xxmin;			/* left-most   point */
	double yymax;			/* top-most    point */
	double yymin;			/* bottom-most point */
	} far *history;

/* yes, I *know* it's supposed to be compatible with Microsoft C,
   but some of the routines need to know if the "C" code
   has been compiled with Turbo-C.  This flag is a 1 if FRACTINT.C
   (and presumably the other routines as well) has been compiled
   with Turbo-C. */

int compiled_by_turboc;

extern	char	savename[];		/* save files using this name */

extern	char preview;	 /* 3D preview mode flag */

extern char floatflag;			/* floating-point fractals? */

extern	char	temp1[];			/* temporary strings	    */

extern	int	debugflag;		/* internal use only - you didn't see this */
/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
	int	dotmode;			/* video access method	    */
	int	oktoprint;			/* 0 if printf() won't work */
	int	xdots, ydots;			/* # of dots on the screen  */
	int	colors; 			/* maximum colors available */
	int	maxit;				/* try this many iterations */
	int	ixmin, ixmax, iymin, iymax;	/* corners of the zoom box  */
	int	boxcount;			/* 0 if no zoom-box yet     */

	int	fractype;			/* if == 0, use Mandelbrot  */
	int	numpasses;			/* 0 if single-pass, else 1 */
	int	solidguessing;			/* 0 if disabled, else 1    */
	long	creal, cimag;			/* real, imag'ry parts of C */
	long	delx, dely;			/* screen pixel increments */
	double	delxx, delyy;			/* screen pixel increments */
	double	param[4];			/* up to four parameters    */
	double	potparam[3];		/* three potential parameters*/
	long	fudge;				/* 2**fudgefactor	*/
	int	bitshift;			/* fudgefactor		*/

	int	hasconfig;			/* = 0 if 'fractint.cfg'    */
	int	diskisactive;			/* disk-video drivers flag  */
	int	diskvideo;			/* disk-video access flag   */
	int	diskprintfs;			/* disk-video access flag   */

extern	long	far *lx0, far *ly0;		/* x, y grid		    */
	double far *dx0, far *dy0;		/* floating pt equivs */
	int	integerfractal; 	/* TRUE if fractal uses integer math */

extern	int	inside; 			/* inside color: 1=blue     */
extern	int	outside; 			/* outside color, if set    */
extern	int	cyclelimit;			/* color-rotator upper limit */
extern	int	display3d;			/* 3D display flag: 0 = OFF */
extern	int	overlay3d;			/* 3D overlay flag: 0 = OFF */
extern	int	init3d[20];			/* '3d=nn/nn/nn/...' values */

extern	  int previewfactor;			/* for save_info */
extern	  int xtrans;
extern	  int ytrans;
extern	  int red_crop_left;
extern	  int red_crop_right;
extern	  int blue_crop_left;
extern	  int blue_crop_right;
extern	  int red_bright;
extern	  int blue_bright;
extern	  int xadjust;
extern	  int eyeseparation;
extern	  int glassestype;
extern	  int filetype;
extern	  int whichimage;


extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern unsigned char olddacbox[256][3]; /* backup copy of the Video-DAC */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	extraseg;		/* used by Save-to-DISK routines */
extern int	cpu;			/* cpu type			*/
extern int	fpu;			/* fpu type			*/
extern int	lookatmouse;		/* used to activate non-button mouse movement */
extern int	out_line();		/* called in decoder */
extern int	cmp_line();		/* for test purposes */
extern	int	outlin16();		/* called in decoder */
extern int	line3d();		/* called in decoder */
extern int	(*outln)();		/* called in decoder */
extern	int	filetype;		/* GIF or other */

/* variables defined by the command line/files processor */
extern	double	inversion[];
extern	int	invert; 		/* non-zero if inversion active */
extern	int	initbatch;		/* 1 if batch run (no kbd)  */
extern	int	initmode;		/* initial video mode	    */
extern	int	inititer;		/* initial value of maxiter */
extern	int	initincr;		/* initial maxiter incrmnt  */
extern	int	initpass;		/* initial pass mode	    */
extern	int	initsolidguessing;	/* initial solid-guessing md*/
extern	int	initfractype;		/* initial type set flag    */
extern	int	initcyclelimit; 	/* initial cycle limit	    */
extern	int	initcorners;		/* initial flag: corners set*/
extern	double	initxmin,initxmax;	/* initial corner values    */
extern	double	initymin,initymax;	/* initial corner values    */
extern	double	initparam[4];		/* initial parameters	    */
extern	int	LogFlag;		/* non-zero if logarithmic palettes */
extern int transparent[];
extern int decomp[];

extern char showbox;	      /* flag to show box and vector in preview */
extern char FormFileName[];   /* file to find (type=)formulas in */
extern char FormName[];       /* Name of the Formula (if not null) */
extern int bailout;	      /* user input bailout value */
extern int mapset;	      /* indicates new map */
extern int warn;	      /* 0 if savename warnings off, 1 if on */
extern int ramvideo;	      /* if zero, skip RAM-video for EXP-RAM */
extern char MAP_name[];       /* map file name */

extern int boundarytraceflag; /* boundary tracing (0 = off, 1 = on) */

extern int rflag, rseed;
int	comparegif=0;			/* compare two gif files flag */

/* the following is out here for the 'tab_display()' routine */
long	xmin, xmax, ymin, ymax; 	/* screen corner values */
double	xxmin,xxmax,yymin,yymax;	/* (printf) screen corners */
int tab_status; 			/* == 0 means no fractal on screen */
					/* == 1 means fractal in progress  */
					/* == 2 means fractal complete	   */

/* "main()" now does initialization only and then calls "main_routine()" */

void main(argc,argv)
int argc;
char *argv[];
{
long	huge *xxxalloc; 			/* Quick-C klooge (extraseg) */

initasmvars();					/* initialize ASM stuff */

hasconfig = readconfig();			/* read fractint.cfg, if any */

maxvideomode = initvideotable();		/* get the size of video table */
if ( maxvideomode >= MAXVIDEOMODES)		/* that's all the Fn keys we got! */
	maxvideomode = MAXVIDEOMODES;

cmdfiles(argc,argv);				/* process the command-line */

if (debugflag == 8088)		      cpu =  86; /* for testing purposes */
if (debugflag == 2870 && fpu >= 287 ) fpu = 287; /* for testing purposes */
if (debugflag ==  870 && fpu >=  87 ) fpu =  87; /* for testing purposes */
if (debugflag ==   70 && fpu >=   0 ) fpu =   0; /* for testing purposes */
if (debugflag >= 9002 && debugflag <= 9100)	 /* for testing purposes */
	if (video_type > (debugflag-9000)/2)	 /* adjust the video value */
		video_type = (debugflag-9000)/2;

#ifdef __TURBOC__
   compiled_by_turboc = 1;
#else
   compiled_by_turboc = 0;
#endif

/*
   This code is used for emergency purposes when we just *have* to
   get CodeView working and we don't *care* about the fact that any
   attempt to do something tricky like using help mode or a hi-rez
   video mode will require "extraseg" memory that we just don't have.

   Note: unless and until you can get CodeView to reside *entirely* in
   extended/expanded memory, run "codeview /s /e fractint debug=10000"
   and don't use help or any hi-rez video modes (they require the full
   92K of memory allocated to 'extraseg').

   Hopefully this code will go away when Microsoft C 6.0 is released
   with a version of CodeView that resides entirely in extended/expanded
   memory.
*/

#ifndef __TURBOC__
if (debugflag == 10000 && extraseg == 0) {
	xxxalloc = (long huge *)halloc(16384L,4);  /* try for 64K */
	extraseg = FP_SEG(xxxalloc);
	if (extraseg == 0) {
		xxxalloc = (long huge *)halloc(8192L,4); /* try for 32K */
		extraseg = FP_SEG(xxxalloc);
		}
	}
#endif

if (extraseg == 0) {			/* oops.  not enough memory	*/
	buzzer(2);
	printf(" I'm sorry, but you don't have enough free memory \n");
	printf(" to run this program.\n\n");
	exit(1);
	}

if ((history = (struct historystruct far * ) farmemalloc((unsigned long)
	(MAXHISTORY * sizeof(*history)))) == NULL) {
	buzzer(2);
	printf(" I'm sorry, but you don't have enough free memory \n");
	printf(" to run this program.\n\n");
	exit(1);
	}

#ifdef __TURBOC__
      dx0 = MK_FP(extraseg,0);
#else
      FP_SEG(dx0)=extraseg;
      FP_OFF(dx0)=0;
#endif
      dy0 = dx0 + MAXPIXELS;

diskisactive = 0;			/* disk-video is inactive */
diskvideo = 0;				/* disk driver is not in use */
diskprintfs = 1;			/* disk-video should display status */
setvideomode(3,0,0,0);			/* switch to text mode */
tab_status = 0; 			/* no active fractal image */

if (debugflag == 10000) {		/* check for free memory */
	char *tempptr;
	unsigned char huge *fartempptr;
	int i;
	long j;
	printf("\n CPU type is %d \n\n FPU type is %d \n", cpu, fpu);
	printf("\n checking for free memory ...\n\n");
	for (i = 100;(tempptr = malloc(i)) != NULL; i+=100)
		free(tempptr) ;
	printf(" %d NEAR bytes free \n", i-100);
	for (j = 10000;(fartempptr =
		(unsigned char huge *)farmemalloc(j)) != NULL; j+=10000)
			farmemfree((void far*)fartempptr) ;
	printf(" %ld FAR bytes free \n\n press any key to continue...\n", j-10000);
	getakey();
	}

main_routine();

}

/* "main_routine" is the main-command-level traffic cop */

main_routine()
{
	double	jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
	double	atof(), ftemp;			/* floating point stuff    */
	double	ccreal,ccimag;			/* Julia Set Parameters    */
	int	xstep, ystep;			/* zoom-box increment values */
	int	axmode, bxmode, cxmode, dxmode; /* video mode (BIOS ##) */
	int	historyptr;			/* pointer into history tbl */
	int	zoomoff;			/* = 0 when zoom is disabled */
	int	kbdchar;			/* keyboard key-hit value */
	int	more, kbdmore;			/* continuation variables */
	int	i, j, k, l;			/* temporary loop counters */
	int	displaypage;			/* signon display page	    */
	int	savedac;			/* save-the-Video DAC flag  */
	int	olddotmode;			/* temp copy of dotmode     */
	int	status;

savedac = 0;				/* don't save the VGA DAC */
restorestart:

read_overlay(); 			/* read overlay/3D files, if reqr'd */

if (overlay3d && initmode < 0) {	/* overlay command failed */
	setforgraphics();		/* restore the graphics screen */
	overlay3d = 0;			/* forget overlays */
	display3d = 0;			/* forget 3D */
	goto resumeloop;		/* ooh, this is ugly */
	}


restart:				/* insert key re-starts here */

savedac = 0;				/* don't save the VGA DAC */

lookatmouse = 0;			/* de-activate full mouse-checking */

maxit = inititer;				/* max iterations */
numpasses = initpass-1; 			/* single/dual-pass mode */
solidguessing = initsolidguessing;		/* solid-guessing mode */

fractype = initfractype;			/* use the default set	 */

if (floatflag) {				/* adjust for floating pt */
	if (fractalspecific[fractype].isinteger != 0 &&
		fractalspecific[fractype].tofloat != NOFRACTAL)
		fractype = fractalspecific[fractype].tofloat;
	}
else	{
	if (fractalspecific[fractype].isinteger == 0 &&
		fractalspecific[fractype].tofloat != NOFRACTAL)
		fractype = fractalspecific[fractype].tofloat;
	}

cyclelimit = initcyclelimit;			/* default cycle limit	 */
for (i = 0; i < 4; i++)
{
   if(initparam[i] != FLT_MAX)
   {
      param[i] = initparam[i];/* use the default params*/
      fractalspecific[fractype].paramvalue[i] = param[i];
   }
   else
      initparam[i] = param[i] = fractalspecific[fractype].paramvalue[i];
}
ccreal = param[0]; ccimag = param[1];		/* default C-values	 */
xxmin = initxmin; xxmax = initxmax;		/* default corner values */
yymin = initymin; yymax = initymax;		/* default corner values */

if (xxmin > xxmax) { ftemp = xxmax; xxmax = xxmin; xxmin = ftemp; }
if (yymin > yymax) { ftemp = yymax; yymax = yymin; yymin = ftemp; }

/* set some reasonable limits on the numbers (or the algorithms will fail) */

if(fractalspecific[fractype].isinteger > 1)
   bitshift = fractalspecific[fractype].isinteger;
else
   bitshift = FUDGEFACTOR2;			/* shift bits by this much */

if(fractalspecific[fractype].isinteger == 0) /* TW start */
{
   if((i = fractalspecific[fractype].tofloat) != NOFRACTAL)
      if(fractalspecific[i].isinteger > 1)
	 bitshift =  fractalspecific[i].isinteger; /* TW stop */
}

if (fractype == MANDEL || fractype == JULIA) {	/* adust shift bits if.. */
   if (fabs(potparam[0]) < .0001		/* not using potential */
       && (fractype != MANDEL ||		/* and not an int mandel */
       (ccreal == 0.0 && ccimag == 0.0))	/*  w/ "fudged" parameters */
       && ! invert				/* and not inverting */
       && biomorph == -1			/* and not biomorphing */
       && debugflag != 1234)			/* and not debugging */
       bitshift = FUDGEFACTOR;
   if (ccreal < -1.99 || ccreal > 1.99) ccreal = 1.99;
   if (ccimag < -1.99 || ccimag > 1.99) ccimag = 1.99;
   }

ftemp = 32767.99;				/* avoid corners overflow */
if (bitshift >= 24) ftemp = 31.99;
if (bitshift >= 29) ftemp = 3.99;
if (xxmax - xxmin > ftemp) xxmax = xxmin + ftemp;
if (yymax - yymin > ftemp) yymax = yymin + ftemp;

fudge = 1; fudge = fudge << bitshift;	       /* fudged value for printfs */

ftemp = ccreal * fudge; creal = ftemp;
ftemp = ccimag * fudge; cimag = ftemp;
ftemp = xxmin * fudge; xmin = ftemp;
ftemp = xxmax * fudge; xmax = ftemp;
ftemp = yymin * fudge; ymin = ftemp;
ftemp = yymax * fudge; ymax = ftemp;

jxxmin = xxmin; jxxmax = xxmax;
jyymin = yymin; jyymax = yymax;

adapter = initmode;			/* set the video adapter up	*/
initmode = -1;				/* (once)			*/

tab_status = 2; 		       /* fractal image is complete */

helpmode = HELPAUTHORS; 	       /* use this help mode */
if (adapter < 0) {
	tab_status = 0; 		/* no active fractal image */
	help(); 			/* display the credits screen */
}
helpmode = HELPMENU;		       /* now use this help mode */

while (adapter < 0) {			/* cycle through instructions	*/

	if (initbatch == 0) {			/* online-mode only, please */
		while (!keypressed()) ; 	/* enable help */
		kbdchar = getakey();
		}
	else
		kbdchar = 27;
	if (kbdchar == 32) continue;		/* spacebar stops scrolling */
	while (kbdchar == 13 || kbdchar == 1013) { /* ENTER calls help, here */
		kbdchar = help();
		}
	adapter = -1;				/* no video adapter yet */
	for (k = 0; k < maxvideomode; k++)	/* search for an adapter */
		if (kbdchar == kbdkeys[k])
			adapter = k;		/* use this adapter */
	if (adapter >= 0) break;		/* bail out if we found one */
	if (kbdchar == 1082) goto restart;	/* restart pgm on Insert Key */
	if (kbdchar == 1000) goodbye(); 	/* Control-C */
	if (kbdchar == 27) goodbye();		/* exit to DOS on Escape Key */
	if (kbdchar == 1083) goodbye(); 	/* exit to DOS on Delete Key */
	if (kbdchar == 'd' || kbdchar == 'D') { /* shell to DOS */
		clscr();
		printf("\n\nShelling to DOS - type 'exit' to return\n\n");
		shell_to_dos();
		goto restart;
		}
	if (kbdchar == '1' || kbdchar == '2') { /* select single or dual-pass */
		numpasses = kbdchar - '1';
		solidguessing = 0;
		continue;
		}
	if (kbdchar == 'g' || kbdchar == 'G') { /* solid-guessing */
		numpasses = 1;
		solidguessing = 1;
		continue;
		}
	if (kbdchar == 'n' || kbdchar == 'N') { /* normal palette */
		LogFlag = 0;
		ChkLogFlag();
		continue;
		}
	if (kbdchar == 'l' || kbdchar == 'L') { /* logarithmic palette */
		LogFlag = 1;
		continue;
		}
	if (kbdchar == 'r' || kbdchar == 'R' || kbdchar == '3'
		|| kbdchar == 'o' || kbdchar == 'O') {  /* restore old image    */
		display3d = 0;
		if (kbdchar == '3' || kbdchar == 'o' ||
			kbdchar == 'O') display3d = 1;
		setvideomode(3,0,0,0);	/* switch to text mode */
			printf("\n Restart from what file? ");
			gets(readname);
			goto restorestart;
			}
	if (kbdchar == 't' || kbdchar == 'T') { /* set fractal type */
		olddotmode = dotmode;	/* save the old dotmode */
		dotmode = 1;		/* force a disable of any 8514/A */
		if ((j = get_fracttype(initfractype)) < 0) goto restart;
		dotmode = olddotmode;
		goto restart;
		}
	if (kbdchar == 'x' || kbdchar == 'X') { /* generic toggle switch */
		clscr();
		get_toggles();			/* get the parameters */
		goto restart;			/* restore the screen */
		}
	if (kbdchar == 'f' || kbdchar == 'F') { /* floating pt toggle */
		if (floatflag == 0)
			floatflag = 1;
		else
			floatflag = 0;
		goto restart;
		}
	if (kbdchar == 'e' || kbdchar == 'E') { /* set IFS fractal parms */
		clscr();
		get_ifs_params();		/* get the parameters */
		goto restart;
		}
	if (kbdchar == 'h' || kbdchar == 'H') { /* obsolete command-key */
		clscr();
		get_obsolete();			/* display obsolete msg */
		goto restart;			/* restore the screen */
		}
	else
		buzzer(2);
	}

historyptr = 0; 				/* initialize history ptr */
history[historyptr].fractype = -1;
zoomoff = 1;					/* zooming is enabled */

helpmode = HELPMAIN;				/* switch help modes */


more = 1;
while (more) {					/* eternal loop */

						/* collect adapter info */
	fromvideotable(adapter);
	axmode	= videoentry.videomodeax;	 /* video mode (BIOS call) */
	bxmode	= videoentry.videomodebx;	 /* video mode (BIOS call) */
	cxmode	= videoentry.videomodecx;	 /* video mode (BIOS call) */
	dxmode	= videoentry.videomodedx;	 /* video mode (BIOS call) */
	dotmode = videoentry.dotmode;		/* assembler dot read/write */
	xdots	= videoentry.xdots;		/* # dots across the screen */
	ydots	= videoentry.ydots;		/* # dots down the screen   */
	colors	= videoentry.colors;		/* # colors available */

	diskvideo = 0;				/* set diskvideo flag */
	if (dotmode == 11)			/* default assumption is disk */
		diskvideo = 2;

	memcpy(olddacbox,dacbox,256*3); 	/* save the DAC */
	diskisactive = 1;		/* flag for disk-video routines */
	if (overlay3d) {
		setforgraphics();	/* restore old graphics image */
		overlay3d = 0;
		}
	else
		setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */
	diskisactive = 0;		/* flag for disk-video routines */
	if (savedac) {
		memcpy(dacbox,olddacbox,256*3); 	/* restore the DAC */
		spindac(0,1);
		}
	savedac = 1;				/* assume we save next time */

	if(*readname && showfile==0) {
		/*
		only requirements for gifview: take file name in global
		variable "readname" - link to frasmint's video (I used putcolor),
		and should NOT set video mode (that's done here)
		*/

	if (display3d)			/* set up 3D decoding */
	   outln = line3d;
	else if(filetype >= 1)
	   outln = outlin16;
	else if(comparegif)
	   outln = cmp_line;
	else
	   outln = out_line;

	if(filetype == 0)
	   status = funny_glasses_call(gifview);
	else
	   status = funny_glasses_call(tgaview);
	if(status==0)
	   buzzer(0);
	if(status == -1 && keypressed())
	       getakey();
	/*	display3d = 0;			   turn off 3D retrievals */
		}

	xstep	= xdots / 40;			/* zoom-box increment: across */
	ystep	= ydots / 40;			/* zoom-box increment: down */
	if (xdots == 640 && ydots == 350)	/* zoom-box adjust:  640x350 */
		{ xstep = 16; ystep =  9; }
	if (xdots == 720 && ydots == 348)	/* zoom-box adjust:  720x348 */
		{ xstep = 18; ystep =  9; }
	if (xdots == 632 && ydots == 474)	/* zoom-box adjust:  632x474 */
		{ xstep = 16; ystep = 12; }
	if (xdots == 720 && ydots == 512)	/* zoom-box adjust:  720x512 */
		{ xstep = 20; ystep = 15; }
	if (xdots * 3 == ydots * 4)		/* zoom-box adjust:  VGA Tweaks */
		{ xstep = 16; ystep = 12; }
	if (xdots == 1024 && ydots == 768)	/* zoom-box adjust: 1024x768 */
		{ xstep = 32; ystep = 24; }
	if (xdots == 1280 && ydots == 1024)	/* zoom-box adjust: 1280x1024 */
		{ xstep = 40; ystep = 32; }
	if (xdots == 1016 && ydots == 762)	/* zoom-box adjust: 1018x762 */
		{ xstep = 32; ystep = 24; }
	if (xdots == 720 && ydots == 348)	/* zoom-box adjust:  720x348 */
		{ xstep = 18; ystep =  9; }

	integerfractal = fractalspecific[fractype].isinteger;

	zoomoff = 1;				/* zooming is enabled */
        /* for these situations */
        if (dotmode == 11 || fractype == PLASMA || fractype == DIFFUSION) 
		zoomoff = 0;		/* disable zooming */

	ccreal = param[0]; ccimag = param[1];
	ftemp = ccreal * fudge; creal = ftemp;
	ftemp = ccimag * fudge; cimag = ftemp;
	ftemp = xxmin * fudge; xmin = ftemp;
	ftemp = xxmax * fudge; xmax = ftemp;
	ftemp = yymin * fudge; ymin = ftemp;
	ftemp = yymax * fudge; ymax = ftemp;
	if (bitshift >= 29 && (xxmax - xxmin) > 3.99) {
		ftemp = xxmax * fudge; xmax = ftemp - 100;
		}
	if (bitshift >= 29 && (yymax - yymin) > 3.99) {
		ftemp = yymin * fudge; ymin = ftemp + 100;
		}

	delx = ((xmax - xmin) / (xdots - 1)); /* calculate stepsizes */
	dely = ((ymax - ymin) / (ydots - 1)); /* calculate stepsizes */
	delxx = ((xxmax - xxmin) / (xdots - 1)); /* calculate stepsizes */
	delyy = ((yymax - yymin) / (ydots - 1)); /* calculate stepsizes */

	if (delxx <= 0.0 || delyy <= 0.0 ||	/* zoomed too far */
	   (integerfractal && (delx <= 1 || dely <= 1))) {
		zoomoff = 0;
		dely = 1;
		delx = 1;
		delyy = DBL_EPSILON;
		delxx = DBL_EPSILON;
		if (integerfractal &&		/* integer fractal type? */
			fractalspecific[fractype].tofloat != NOFRACTAL) {
			floatflag = 1;		/* switch to floating pt */
			inititer = maxit;
			initfractype = fractype;
			initmode = adapter;
			initxmin = xxmin;
			initxmax = xxmax;
			initymin = yymin;
			initymax = yymax;
			for (i = 0; i < 4; i++)
				initparam[i] = param[i];
			goto restart;
			}
		}

	lx0[0] = xmin;				/* fill up the x, y grids */
	ly0[0] = ymax;
	for (i = 1; i < xdots; i++ )
		lx0[i] = lx0[i-1] + delx;
	for (i = 1; i < ydots; i++ )
		ly0[i] = ly0[i-1] - dely;

	if ((!integerfractal) || invert)
	{
	/* set up dx0 and dy0 analogs of lx0 and ly0 */
	/* put fractal parameters in doubles */
	dx0[0] = xxmin; 			/* fill up the x, y grids */
	dy0[0] = yymax;
	for (i = 1; i < xdots; i++ )
		dx0[i] = dx0[i-1] + delxx;
	for (i = 1; i < ydots; i++ )
		dy0[i] = dy0[i-1] - delyy;
	}

	if (zoomoff == 0) {
		xmax = lx0[xdots-1];		/* re-set xmax and ymax */
		ymin = ly0[ydots-1];
		if (integerfractal) {
			xxmax = xmax / fudge;
			yymin = ymin / fudge;
		} else {
			xxmax = dx0[xdots-1];
			yymin = dy0[ydots-1];
			}
		}


	 if ((fractype == MANDEL || fractype == JULIA) && bitshift == 29)
	    decomp[1] = 0;	/* make the world safe for decomposition */

	if (history[0].fractype == -1)		/* initialize the history file */
		for (i = 0; i < MAXHISTORY; i++) {
			history[i].xxmax = xxmax;
			history[i].xxmin = xxmin;
			history[i].yymax = yymax;
			history[i].yymin = yymin;
			history[i].param[0] = param[0];
			history[i].param[1] = param[1];
			history[i].fractype = fractype;
		}

	if (history[historyptr].xxmax != xxmax	||	/* save any (new) zoom data */
	    history[historyptr].xxmin != xxmin	||
	    history[historyptr].yymax != yymax	||
	    history[historyptr].yymin != yymin	||
	    history[historyptr].param[0] != param[0] ||
	    history[historyptr].param[1] != param[1] ||
	    history[historyptr].fractype != fractype) {
		if (++historyptr == MAXHISTORY) historyptr = 0;
		history[historyptr].xxmax = xxmax;
		history[historyptr].xxmin = xxmin;
		history[historyptr].yymax = yymax;
		history[historyptr].yymin = yymin;
		history[historyptr].param[0] = param[0];
		history[historyptr].param[1] = param[1];
		history[historyptr].fractype = fractype;
		}

	if (diskvideo) {		/* disk-file video klooges */
		numpasses = 0;		/* single-pass mode */
		solidguessing = 0;	/* no solid-guessing */
		}

	if(*readname && showfile==0)
		showfile = 1;
	else	{
		diskisactive = 1;	/* flag for disk-video routines */
	/* TW 07/21/89 - see below */
	/* set save parameters in save structure */
		strcpy(save_info.info_id, INFO_ID);
		save_info.iterations   = maxit;
		save_info.fractal_type = fractype;
		save_info.xmin	       = xxmin;
		save_info.xmax	       = xxmax;
		save_info.ymin	       = yymin;
		save_info.ymax	       = yymax;
		save_info.creal        = param[0];
		save_info.cimag        = param[1];
		save_info.videomodeax  = videoentry.videomodeax;
		save_info.videomodebx  = videoentry.videomodebx;
		save_info.videomodecx  = videoentry.videomodecx;
		save_info.videomodedx  = videoentry.videomodedx;
		save_info.dotmode	= videoentry.dotmode;
		save_info.xdots 	= videoentry.xdots;
		save_info.ydots 	= videoentry.ydots;
		save_info.colors	= videoentry.colors;
	save_info.version = 1;
	save_info.parm3 	 = param[2];
	save_info.parm4 	 = param[3];
	save_info.potential[0]	 = potparam[0];
	save_info.potential[1]	 = potparam[1];
	save_info.potential[2]	 = potparam[2];
	save_info.rflag 	 = rflag;
	save_info.rseed 	 = rseed;
	save_info.inside	 = inside;
	save_info.logmap	 = LogFlag;
	save_info.invert[0]	 = inversion[0];
	save_info.invert[1]	 = inversion[1];
	save_info.invert[2]	 = inversion[2];
	save_info.decomp[0]	 = decomp[0];
	save_info.decomp[1]	 = decomp[1];
	save_info.biomorph	 = biomorph;
	save_info.symmetry	 = forcesymmetry;
	save_info.version = 2;
	for (i = 0; i < 16; i++)
		save_info.init3d[i] = init3d[i];
	save_info.previewfactor  = previewfactor;
	save_info.xtrans	 = xtrans;
	save_info.ytrans	 = ytrans;
	save_info.red_crop_left  = red_crop_left;
	save_info.red_crop_right = red_crop_right;
	save_info.blue_crop_left  = blue_crop_left;
	save_info.blue_crop_right = blue_crop_right;
	save_info.red_bright	  = red_bright;
	save_info.blue_bright	  = blue_bright;
	save_info.xadjust	  = xadjust;
	save_info.eyeseparation   = eyeseparation;
	save_info.glassestype	  = glassestype;
	save_info.version = 3;
	save_info.outside         = outside;

	for (i = 0; i < sizeof(save_info.future)/sizeof(int); i++)
	       save_info.future[i] = 0;

	tab_status = 1; 			/* fractal image in progress */

		if (calcfract() == 0)		/* draw the fractal using "C" */
			buzzer(0);		/* finished!! */
		if( dotmode == 9 || dotmode == 11 ) {	/* if TARGA or disk-video */
			if( dotmode == 11 )	/* TARGA already has some text up */
				home();
			else
				EndTGA();	/* make sure TARGA is OFF */
			printf("Image has been completed");
		}

	diskisactive = 0;	/* flag for disk-video routines */

	tab_status = 2; 			/* fractal image is complete */
	}

	ixmin = 0;  ixmax = xdots-1;		/* initial zoom box */
	iymin = 0;  iymax = ydots-1;
	boxcount = 0;				/* no zoom box yet  */

	if (fractype == PLASMA && cpu > 88) {
		cyclelimit = 256;		/* plasma clouds need quick spins */
		daccount = 256;
		daclearn = 1;
		}

resumeloop:					/* return here on failed overlays */

	kbdmore = 1;
	while (kbdmore == 1) {			/* loop through cursor keys */
		if (initbatch == 0) {		/* online only, please */
			lookatmouse = 1;	/* activate full mouse-checking */
			while (!keypressed());	/* enables help */
			kbdchar = getakey();
			lookatmouse = 0;	/* de-activate full mouse-checking */
			}
		else {				/* batch mode special  */
			if (initbatch == 1) {	/* first, save-to-disk */
				if (debugflag == 50)
					kbdchar = 'r';
				else
					kbdchar = 's';
				initbatch = 2;
				}
			else
				kbdchar = 27;	/* then, exit	       */
			}
		switch (kbdchar) {
			case 't':                       /* new fractal type */
			case 'T':
				olddotmode = dotmode;	/* save the old dotmode */
				dotmode = 1;		/* force a disable of any 8514/A */
				if ((j = get_fracttype(fractype)) < 0) goto restart;
				savedac = 0;
				initmode = adapter;
				dotmode = olddotmode;
				goto restart;
				break;
			case 'x':                       /* boundary tracing toggle */
			case 'X':
				setfortext();		/* switch to text mode */
				i = get_toggles();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				if (i > 0)		/* time to redraw? */
					kbdmore = 0;
				if (i == 2) {		/* float change? */
					inititer = maxit;
					initfractype = fractype;
					initmode = adapter;
					initxmin = xxmin;
					initxmax = xxmax;
					initymin = yymin;
					initymax = yymax;
					for (i = 0; i < 4; i++)
						initparam[i] = param[i];
					goto restart;
					}
				break;
			case 'f':                       /* floating pt toggle */
			case 'F':
				if (floatflag == 0)
					floatflag = 1;
				else
					floatflag = 0;
				inititer = maxit;
				initfractype = fractype;
				initmode = adapter;
				initxmin = xxmin;
				initxmax = xxmax;
				initymin = yymin;
				initymax = yymax;
				for (i = 0; i < 4; i++)
					initparam[i] = param[i];
				goto restart;
			case 'e':                       /* new IFS parms    */
			case 'E':
				setfortext();		/* switch to text mode */
				get_ifs_params();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				kbdmore = 0;
				break;
			case 'i':                       /* inversion parms    */
			case 'I':
		                setfortext();           /* switch to text mode */
		                get_invert_params();    /* get the parameters */
		                setforgraphics();       /* back to graphics */
		                inititer = maxit;
		                initfractype = fractype;
		                initmode = adapter;
		                initxmin = xxmin;
		                initxmax = xxmax;
		                initymin = yymin;
		                initymax = yymax;
		                for (i = 0; i < 4; i++)
		                        initparam[i] = param[i];
		                invert = (inversion[0]==0.0) ? 0 : 1;
		                goto restart;
				kbdmore = 0;
				break;
			case 'q':                       /* decomposition parms    */
			case 'Q':
				setfortext();		/* switch to text mode */
				get_decomp_params();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				kbdmore = 0;
				break;
			case 'a':                       /* starfield parms    */
			case 'A':
				get_starfield_params(); /* get the parameters */
				continue;
				break;
			case 32:			/* spacebar */
				if (fractalspecific[fractype].tojulia != NOFRACTAL
					&& ccreal == 0.0 && ccimag == 0.0) {
					/* switch to corresponding Julia set */
					fractype = fractalspecific[fractype].tojulia;
					ccreal = (xxmax + xxmin) / 2;
					ccimag = (yymax + yymin) / 2;
					param[0] = ccreal;
					param[1] = ccimag;

					if (integerfractal) {
					jxxmin = lx0[0]; jxxmax = lx0[xdots-1];
					jyymax = ly0[0]; jyymin = ly0[ydots-1];
					jxxmin /= fudge;  jxxmax /= fudge;
					jyymin /= fudge;  jyymax /= fudge;
					} else {
					jxxmin = dx0[0]; jxxmax = dx0[xdots-1];
					jyymax = dy0[0]; jyymin = dy0[ydots-1];
					}

					xxmin = fractalspecific[fractype].xmin;
					xxmax = fractalspecific[fractype].xmax;
					yymin = fractalspecific[fractype].ymin;
					yymax = fractalspecific[fractype].ymax;

					if(biomorph != -1 && bitshift != 29) {
						xxmin *= 3.0;
						xxmax *= 3.0;
						yymin *= 3.0;
						yymax *= 3.0;
						}

					zoomoff = 1;
					}

				else if (fractalspecific[fractype].tomandel != NOFRACTAL) {
					/* switch to corresponding Mandel set */
					fractype = fractalspecific[fractype].tomandel;
					ccreal = 0;
					ccimag = 0;
					param[0] = 0;
					param[1] = 0;
					xxmin = jxxmin;  xxmax = jxxmax;
					yymin = jyymin;  yymax = jyymax;
					zoomoff = 1;
					}

				else buzzer(2); 	/* no switch */
				kbdmore = 0;
				break;
			case 1071:			/* home */
				if (--historyptr < 0)
					historyptr = MAXHISTORY-1;
				xxmax  = history[historyptr].xxmax;
				xxmin  = history[historyptr].xxmin;
				yymax  = history[historyptr].yymax;
				yymin  = history[historyptr].yymin;
				param[0] = history[historyptr].param[0];
				param[1] = history[historyptr].param[1];
				fractype = history[historyptr].fractype;
				ccreal = param[0]; ccimag = param[1];
				zoomoff = 1;
				kbdmore = 0;
				break;
			case 'd':                       /* shell to MS-DOS */
			case 'D':
				setfortext();
				printf("\n\nShelling to DOS - type 'exit' to return\n\n");
				if (axmode == 0 || axmode > 7) {
					printf("Note:  Your graphics image is still squirreled away in your video\n");
					printf("adapter's memory.  Switching video modes (say, to get your cursor back)\n");
					printf("will clobber part of that image.  Sorry - it's the best we could do.\n\n");
					}
				shell_to_dos();
				setforgraphics();
				break;
			case '<':                       /* lower iter maximum */
			case ',':
				if (maxit >= 10+initincr) maxit -= initincr;
				maxit -= initincr;	/* for fall-thru */
			case '>':                       /* raise iter maximum */
			case '.':
				if (maxit <= 32000-initincr) maxit += initincr;
				if (LogFlag) {		/* log palettes? */
					LogFlag = 0;	/* clear them out */
					ChkLogFlag();
					LogFlag = 1;	/* and re-calculate */
					}
				continue;
				break;
			case 'c':                       /* switch to cycling */
			case 'C':
				rotate(0);
				continue;
				break;
			case '+':                       /* rotate palette */
				rotate(+1);
				continue;
				break;
			case '-':                       /* rotate palette */
				rotate(-1);
				continue;
				break;
			case 's':                       /* save-to-disk */
			case 'S':
		saveanimage();
				continue;
		break;
			case 'o':                       /* 3D overlay */
			case 'O':
				overlay3d = 1;
			case '3':                       /* restore-from (3d) */
				display3d = 1;
			case 'r':                       /* restore-from */
			case 'R':
				comparegif = 0;
				if(kbdchar == 'r' || kbdchar == 'R')
				{
				   if(debugflag == 50)
				   {
				      comparegif = overlay3d = 1;
				      if (initbatch == 2)
				      {
					 strcpy(readname,savename);
					 goto restorestart;
				      }
				   }
				   else
				      comparegif = overlay3d = 0;
				   display3d = 0;
				}
				olddotmode = dotmode;	/* save the old dotmode */
				if (overlay3d) {
					setfortext();	/* save graphics image */
					printf("\n Overlay from");
					}
				else	{
					dotmode = 1;		/* in case of an 8514/A mode */
					setvideomode(3,0,0,0);	/* switch to text mode */
					printf("\n Restart from");
					}
				if (*readname)
					printf(" what file (if not %s)? ",
						readname);
				else
					printf(" what file? ");
				temp1[0] = 0;
				gets(temp1);
				if (temp1[0] != 0)
					strcpy(readname,temp1);
				dotmode = olddotmode;
				goto restorestart;
				break;
			case '1':                       /* single-pass mode */
			case '2':                       /* dual-pass mode */
				numpasses = kbdchar - '1';
				solidguessing = 0;
				kbdmore = 0;
				break;
			case 'g':                       /* solid-guessing */
			case 'G':
				numpasses = 1;
				solidguessing = 1;
				kbdmore = 0;
				break;
			case 'n':                       /* normal palette */
			case 'N':
				LogFlag = 0;
				ChkLogFlag();
				kbdmore = 0;
				break;
			case 'l':                       /* log palette */
			case 'L':
				LogFlag = 1;
				kbdmore = 0;
				break;
			case 'b':                       /* make batch file */
			case 'B':
				make_batch_file();
				break;
			case 'p':
			case 'P':
				Print_Screen();
				if (!keypressed()) buzzer(0);
				 else {
					buzzer(1);
					getakey();
				 }
				continue;
				/*  ^^^^^  MDS 7/1/89  ^^^^^  */
			case 13:			/* Enter */
			case 1013:			/* Numeric-Keypad Enter */
			case 1079:			/* end */
				kbdmore = 0;
				break;
			case 10:			/* control-Enter */
			case 1010:			/* Control-Keypad Enter */
				if (integerfractal) {
				ftemp = 1.0 * delx;
				ftemp = (ftemp * xdots) / (ixmax-ixmin+1);
				xxmin = lx0[0] - (ftemp * ixmin);
				xxmax = xxmin  + (ftemp * (xdots-1));
				ftemp = 1.0 * dely;
				ftemp = (ftemp * ydots) / (iymax-iymin+1);
				yymax = ly0[0] + (ftemp * iymin);
				yymin = yymax  - (ftemp * (ydots-1));
				if (xxmin < -2.147e9 || xxmax > 2.147e9 ||
				  yymin < -2.147e9 || yymax > 2.147e9 ||
				  (xxmax-xxmin) > 2.147e9 ||
				  (yymax-yymin) > 2.147e9) {
					buzzer(2);
					break;
					}
				xmin = xxmin; xmax = xxmax;
				ymin = yymin; ymax = yymax;
                                xxmax = (double)xmax / fudge;
                                xxmin = (double)xmin / fudge;
                                yymax = (double)ymax / fudge;
                                yymin = (double)ymin / fudge;
				} else {
				ftemp = (delxx * xdots) / (ixmax-ixmin+1);
				xxmin = dx0[0] - (ftemp * ixmin);
				xxmax = xxmin  + (ftemp * (xdots-1));
				ftemp = (delyy * ydots) / (iymax-iymin+1);
				yymax = dy0[0] + (ftemp * iymin);
				yymin = yymax  - (ftemp * (ydots-1));
				}
				kbdmore = 0;
				break;
			case 1082:			/* insert */
				dotmode = 1;
				setvideomode(3,0,0,0);	/* force text mode */
				goto restart;
				break;
			case 1000:			/* Control-C */
			case 27:			/* Escape */
			case 1083:			/* delete */
				more = 0; kbdmore = 0;
				break;
			case 1075:			/* cursor left */
				if (zoomoff == 1 && ixmin >= 1) {
					ixmin -= 1;
					ixmax -= 1;
					}
				break;
			case 1077:			/* cursor right */
				if (zoomoff == 1 && ixmax < xdots - 1) {
					ixmin += 1;
					ixmax += 1;
					}
				break;
			case 1072:			/* cursor up */
				if (zoomoff == 1 && iymin >= 1) {
					iymin -= 1;
					iymax -= 1;
					}
				break;
			case 1080:			/* cursor down */
				if (zoomoff == 1 && iymax < ydots - 1) {
					iymin += 1;
					iymax += 1;
					}
				break;
			case 1115:			/* Ctrl-cursor left */
				if (zoomoff == 1 && ixmin >= 5) {
					ixmin -= 5;
					ixmax -= 5;
					}
				break;
			case 1116:			/* Ctrl-cursor right */
				if (zoomoff == 1 && ixmax < xdots - 5) {
					ixmin += 5;
					ixmax += 5;
					}
				break;
			case 1141:			/* Ctrl-cursor up */
				if (zoomoff == 1 && iymin >= 5) {
					iymin -= 5;
					iymax -= 5;
					}
				break;
			case 1145:			/* Ctrl-cursor down */
				if (zoomoff == 1 && iymax < ydots - 5) {
					iymin += 5;
					iymax += 5;
					}
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
					/* 720x348 Zoom-In Klooge:  Adjust the
					   Zoom-Box on the initial Zoom-In */
					if (xdots == 720 && ydots == 348
					  && iymin == 0  && iymax == ydots-1) {
						iymin -= 6;
						iymax += 6;
						}
					/* 720x512 Zoom-In Klooge:  Adjust the
					   Zoom-Box on the initial Zoom-In */
					if (xdots == 720 && ydots == 512
					  && iymin == 0  && iymax == ydots-1) {
						iymin -= 14;
						iymax += 14;
						}
					ixmin += xstep; ixmax -= xstep;
					iymin += ystep; iymax -= ystep;
					}
				break;
			case 1081:			/* page down */
				if (zoomoff == 1
				 && ixmin >= xstep && ixmax < xdots - xstep
				 && iymin >= ystep && iymax < ydots - ystep) {
					ixmin -= xstep; ixmax += xstep;
					iymin -= ystep; iymax += ystep;
					}
				break;
			case 'h':                       /* obsolete command key */
			case 'H':
				setfortext();		/* switch to text mode */
				get_obsolete();		/* inform user */
				setforgraphics();	/* back to graphics */
				break;
			default:		/* other (maybe a valid Fn key) */
				for (k = 0; k < maxvideomode; k++)
					if (kbdchar == kbdkeys[k]) {
						adapter = k;
						kbdmore = 0;
						savedac = 0;
						}
				if (kbdmore != 0)
					continue;
				break;
			}

		if (zoomoff == 1 && kbdmore == 1) {	/* draw a zoom box? */
			if (integerfractal) {
				xmin = lx0[ixmin];
				xmax = lx0[ixmax];
				ymin = ly0[iymax];
				ymax = ly0[iymin];
				ftemp = xmin; xxmin = ftemp / fudge;
				ftemp = xmax; xxmax = ftemp / fudge;
				ftemp = ymin; yymin = ftemp / fudge;
				ftemp = ymax; yymax = ftemp / fudge;
			} else {
				xxmin = dx0[ixmin];
				xxmax = dx0[ixmax];
				yymin = dy0[iymax];
				yymax = dy0[iymin];
			}
			if (ixmin > 0 || ixmax < xdots-1
				|| iymin > 0 || iymax < ydots-1)
				drawbox(1);
			else
				drawbox(0);
			}
		}
	}
	dotmode = 0;		/* no need to reinit TARGA 2 April 89 j mclain */
	goodbye();				/* we done. */

}

read_overlay()				/* read overlay/3D files, if reqr'd */
{
int kbdchar;
char accessmethod[2];			/* "B" if through the BIOS */

if(*readname){	/* This is why readname initialized to null string */
	if (!overlay3d) 		/* overlays use the existing mode */
		initmode = -1;		/* no viewing mode set yet */
	else
		initmode = adapter;	/* use previous adapter mode for overlays */
	if(find_fractal_info(readname,&read_info)==0){
		inititer   = read_info.iterations;
		initfractype  = read_info.fractal_type;
		initxmin   = read_info.xmin;
		initxmax   = read_info.xmax;
		initymin   = read_info.ymin;
		initymax   = read_info.ymax;
		initparam[0]   = read_info.creal;
		initparam[1]   = read_info.cimag;

		fractalspecific[initfractype].paramvalue[0] = initparam[0];
		fractalspecific[initfractype].paramvalue[1] = initparam[1];

		if(read_info.version > 0)
	    {
			initparam[2]  = read_info.parm3;
			initparam[3]  = read_info.parm4;
		fractalspecific[initfractype].paramvalue[2] = initparam[2];
		fractalspecific[initfractype].paramvalue[3] = initparam[3];
		potparam[0]   = read_info.potential[0];
		potparam[1]   = read_info.potential[1];
		potparam[2]   = read_info.potential[2];
		rflag	      = read_info.rflag;
		rseed	      = read_info.rseed;
		inside	      = read_info.inside;
		LogFlag       = read_info.logmap;
		inversion[0]  = read_info.invert[0];
		inversion[1]  = read_info.invert[1];
		inversion[2]  = read_info.invert[2];
		decomp[0]     = read_info.decomp[0];
		decomp[1]     = read_info.decomp[1];
		biomorph      = read_info.biomorph;
		forcesymmetry = read_info.symmetry;
	    }

		if(read_info.version > 1 && !display3d)
	    {
		       int i;
		       for (i = 0; i < 16; i++)
			      init3d[i] = read_info.init3d[i];
		   previewfactor   = read_info.previewfactor;
		   xtrans	   = read_info.xtrans;
		   ytrans	   = read_info.ytrans;
		   red_crop_left   = read_info.red_crop_left;
		   red_crop_right  = read_info.red_crop_right;
		   blue_crop_left  = read_info.blue_crop_left;
		   blue_crop_right = read_info.blue_crop_right;
		   red_bright	   = read_info.red_bright;
		   blue_bright	   = read_info.blue_bright;
		   xadjust	   = read_info.xadjust;
		   eyeseparation   = read_info.eyeseparation;
		   glassestype	   = read_info.glassestype;
	    }
		if(read_info.version > 2) {
		   outside         = read_info.outside;
	    }

		showfile = 0;
	if (!overlay3d) {		/* don't worry about the video if overlay */
		if ((hasconfig || askvideo) && read_info.info_id[0] == 'G') {
			buzzer(2);
			printf("You have selected an unknown (non-fractal) GIF image\n");
			printf("I am treating this image as a PLASMA CLOUD of unknown video type\n");
			}
		if (read_info.dotmode == 11 && ! initbatch) {
			buzzer(2);
			printf("You have selected a fractal image generated in 'no-video' mode\n");
			}
		if (((hasconfig || askvideo) && read_info.info_id[0] == 'G') ||
			(read_info.dotmode == 11 && ! initbatch)) {
			printf("with resolution %d x %d x %d\n",
				read_info.xdots, read_info.ydots, read_info.colors);
			}
		if ((!(hasconfig || askvideo) || read_info.info_id[0] != 'G') &&
			(read_info.dotmode != 11 || initbatch))
			initmode = getGIFmode(&read_info);
		if (initmode >= maxvideomode) initmode = -1;
		if (/* read_info.info_id[0] != 'G' &&*/ read_info.dotmode != 11
			&& initmode >= 0
			&& (hasconfig || askvideo) && ! initbatch ){
			char c;
			fromvideotable(initmode);
			strcpy(accessmethod," ");
			if (videoentry.dotmode == 1)
				accessmethod[0] = 'B';
			printf("fractal file contains following mode:\n\n");
#ifdef __TURBOC__
			printf("%-6.6s %-25.25s%5d x%4d%5d %1.1s  %-25.25s\n",
#else
			printf("%-6s %-25s%5d x%4d%5d %1s  %-25s\n",
#endif
				fkeys[initmode],
				videoentry.name,
				videoentry.xdots,
				videoentry.ydots,
				videoentry.colors,
				accessmethod,
				videoentry.comment);
			printf("Legal for this machine? ([Y] or N) --> ");
			while((c=getch())!='y' && c!='Y' && c!='n' && c!='N' && c!=13);
			if( c == 'n' || c == 'N')
				initmode = -1;
			}
		else	{
			/* initmode bad */
			if (initmode == -1 && initbatch)
				showfile = 1; /* pretend already displayed */
			}
		if (initmode == -1 && ! initbatch) {
			printf("\nPlease select a video mode with which to view this image by\n");
			printf("pressing the appropriate function key (or press the ENTER)\n");
			printf("key to bail out without displaying this image).  You can also press\n");
			printf("the 'h' or '?' keys to get help on the available video modes)\n");
			helpmode = HELPVIDEO;
			while (!keypressed());
			kbdchar = getakey();
			for (initmode = 0; initmode < maxvideomode; initmode++)
				if (kbdchar == kbdkeys[initmode]) break;
			if (initmode == maxvideomode) {
				*readname = 0; /* failed ... zap filename */
				initmode = -1;
				showfile = 1; /* pretend already displayed */
				}
		}
			if (initmode >= 0) fromvideotable(initmode);
			if (initmode >= 0 && (
				read_info.xdots != videoentry.xdots ||
				read_info.ydots != videoentry.ydots)) {
					initxmax = initxmin +
						((initxmax-initxmin)*videoentry.xdots)/
						read_info.xdots;
					initymin = initymax -
						((initymax-initymin)*videoentry.ydots)/
						read_info.ydots;
					if ((initfractype == MANDEL || initfractype == JULIA) &&
						((initxmax-initxmin) > 4.0 || (initymax-initymin) > 4.0)) {
						initfractype = PLASMA;
						initparam[0] = 0;
						}
					}
				}

		if (initmode >= 0 && display3d) {
			initfractype = PLASMA;
			initparam[0] = 0;
			}
		if (get_3d_params() < 0) {
			initmode = -1;
			*readname = 0;
			return(0);
			}
		initcorners = 1;
		}
	else	{
		buzzer(2);
		initmode = -1;
		*readname = 0;
		}
	}
}

make_batch_file()
{
FILE *batch;

batch = fopen("frabatch.bat", "a");
fprintf(batch, "fractint");

/********************************/
/* universal parameters go here */
/********************************/
if(askvideo==0)
   fprintf(batch, " askvideo=no");
if(ramvideo==0)
   fprintf(batch, " ramvideo=no");
if (warn)
   fprintf(batch, " warn=yes");
if (mapset && *MAP_name)
{
   /* strip path from file name */
   char *p;
   p = strrchr(MAP_name,'\\');
   if(p)
      fprintf(batch, " map=%s", p+1);
   else
      fprintf(batch, " map=%s", MAP_name);
}
if (inside != 1)
   fprintf(batch, " inside=%d", inside);
if (outside != 1)
   fprintf(batch, " outside=%d", outside);
if(forcesymmetry==XAXIS)
   fprintf(batch, " symmetry=xaxis");
else if(forcesymmetry==YAXIS)
   fprintf(batch, " symmetry=yaxis");
else if(forcesymmetry==XYAXIS)
   fprintf(batch, " symmetry=xyaxis");
else if(forcesymmetry==ORIGIN)
   fprintf(batch, " symmetry=origin");
else if(forcesymmetry==PI_SYM)
   fprintf(batch, " symmetry=pi");
else if(forcesymmetry==NOSYM)
   fprintf(batch, " symmetry=none");
if(LogFlag)
   fprintf(batch, " logmap=yes");
if(rflag)
   fprintf(batch, " rseed=%d",rseed);

/***********************************/
/* fractal only parameters go here */
/***********************************/
if (display3d <= 0)	/* a fractal was generated */
{
   if (fractalspecific[fractype].name[0] != '*')
      fprintf(batch, " type=%s", fractalspecific[fractype].name);
   else
      fprintf(batch, " type=%s", &fractalspecific[fractype].name[1]);
   if (delx > 1000 && dely > 1000)
      fprintf(batch, " corners=%g/%g/%g/%g",
	  xxmin, xxmax, yymin, yymax);
   else
      fprintf(batch, " corners=%+20.17lf/%+20.17lf/%+20.17lf/%+20.17lf",
	  xxmin, xxmax, yymin, yymax);
   fprintf(batch, " params=%g/%g/%g/%g",
       param[0], param[1], param[2], param[3]);
   if (maxit != 150)
      fprintf(batch, " maxiter=%d", maxit);
   if (initincr != 50)
      fprintf(batch, " iterincr=%d", initincr);
   if (invert)
      fprintf(batch, " invert=%g/%g/%g",
	  inversion[0], inversion[1], inversion[2]);
   if (decomp[0])
      fprintf(batch, " decomp=%d/%d", decomp[0], decomp[1]);
   if (floatflag)
      fprintf(batch, " float=yes");
   if (biomorph != -1)
      fprintf(batch, " biomorph=%d", biomorph);
   if(bailout)
      fprintf(batch, " bailout=yes");
   if(*FormName)
   {
      fprintf(batch, " formulafile=%s",FormFileName);
      fprintf(batch, " formulaname=%s",FormName);
   }
}
/**********************************/
/* line3d only parameters go here */
/**********************************/
if (display3d >= 1)	/* only for line3d */
{
   fprintf(batch, " 3d=yes");
   if (showfile && *readname)
      fprintf(batch, " filename=%s", readname);
   if (SPHERE)
   {
      fprintf(batch, " sphere=yes");
      fprintf(batch, " latitude=%d/%d", THETA1, THETA2);
      fprintf(batch, " longitude=%d/%d", PHI1, PHI2);
      fprintf(batch, " radius=%d", RADIUS);
   }
   if (FILLTYPE)
      fprintf(batch, " filltype=%d",
	  FILLTYPE);
   if (transparent[0] || transparent[1])
      fprintf(batch, " transparent=%d/%d",
	  transparent[0],transparent[1]);
   if (preview)
   {
      fprintf(batch, " preview=yes");
      if (showbox)
	 fprintf(batch, " showbox=yes");
      fprintf(batch, " coarse=%d",previewfactor);
   }
}

/***********************************/
/* universal 3d parameters go here */
/***********************************/
if (display3d)		/* universal 3d */
{
   if(!SPHERE)
   {
      fprintf(batch, " rotation=%d/%d/%d", XROT, YROT, ZROT);
      fprintf(batch, " scalexyz=%d/%d", XSCALE, YSCALE);
   }
   fprintf(batch, " roughness=%d", ROUGH);
   fprintf(batch, " waterline=%d", WATERLINE);
   fprintf(batch, " perspective=%d", ZVIEWER);
   fprintf(batch, " xyshift=%d/%d", XSHIFT, YSHIFT);
   if (FILLTYPE > 4)
      fprintf(batch, " lightsource=%d/%d/%d",
	  XLIGHT, YLIGHT, ZLIGHT);
   if (LIGHTAVG && FILLTYPE > 4)
      fprintf(batch, " smoothing=%d", LIGHTAVG);
   if(xtrans || ytrans)
      fprintf(batch, " xyadjust=%d/%d",xtrans,ytrans);
   if(glassestype)
   {
      fprintf(batch, " stereo=%d",glassestype);
      fprintf(batch, " interocular=%d",eyeseparation);
      fprintf(batch, " converge=%d",xadjust);
      fprintf(batch, " crop=%d/%d/%d/%d",
	  red_crop_left,red_crop_right,blue_crop_left,blue_crop_right);
      fprintf(batch, " bright=%d/%d",
	  red_bright,blue_bright);
   }
}

fprintf(batch, "\n");
if (batch != NULL)
   fclose(batch);

}

int shell_to_dos()
{
    char *comspec;

    if ((comspec = getenv("COMSPEC")) == NULL) {
	printf("Cannot find COMMAND.COM.\n");
	return -1;
    }
    putenv("PROMPT=Type 'EXIT' to return to FRACTINT.$_$p$g");
    return spawnl(P_WAIT, comspec, NULL);
}

setup_save_info()
{
int i;
			    /* set save parameters in save structure */
				strcpy(save_info.info_id, INFO_ID);
				save_info.iterations   = maxit;
				save_info.fractal_type = fractype;
				save_info.xmin	       = xxmin;
				save_info.xmax	       = xxmax;
				save_info.ymin	       = yymin;
				save_info.ymax	       = yymax;
				save_info.creal        = param[0];
				save_info.cimag        = param[1];
				save_info.videomodeax  = videoentry.videomodeax;
				save_info.videomodebx  = videoentry.videomodebx;
				save_info.videomodecx  = videoentry.videomodecx;
				save_info.videomodedx  = videoentry.videomodedx;
				save_info.dotmode	   = videoentry.dotmode;
				save_info.xdots 	   = videoentry.xdots;
				save_info.ydots 	   = videoentry.ydots;
				save_info.colors	   = videoentry.colors;
					    save_info.version	   = 1;     /* structure version number */
			save_info.parm3 	 = param[2];
			save_info.parm4 	 = param[3];
			save_info.potential[0]	 = potparam[0];
			save_info.potential[1]	 = potparam[1];
			save_info.potential[2]	 = potparam[2];
			save_info.rflag 	 = rflag;
			save_info.rseed 	 = rseed;
			save_info.inside	 = inside;
			save_info.logmap	 = LogFlag;
			save_info.invert[0]	 = inversion[0];
			save_info.invert[1]	 = inversion[1];
			save_info.invert[2]	 = inversion[2];
			save_info.version = 2;
			for (i = 0; i < 16; i++)
				save_info.init3d[i] = init3d[i];
			save_info.previewfactor  = previewfactor;
			save_info.xtrans	 = xtrans;
			save_info.ytrans	 = ytrans;
			save_info.red_crop_left  = red_crop_left;
			save_info.red_crop_right = red_crop_right;
			save_info.blue_crop_left  = blue_crop_left;
			save_info.blue_crop_right = blue_crop_right;
			save_info.red_bright	  = red_bright;
			save_info.blue_bright	  = blue_bright;
			save_info.xadjust	  = xadjust;
			save_info.eyeseparation   = eyeseparation;
			save_info.glassestype	  = glassestype;
			save_info.version = 3;
			save_info.outside    	  = outside;

			for (i = 0; i < sizeof(save_info.future)/sizeof(int); i++)
			   save_info.future[i] = 0;

}
saveanimage()
{
   double ftemp;
				drawbox(0);		/* clobber zoom-box */
				ixmin = 0;  ixmax = xdots-1;
				iymin = 0;  iymax = ydots-1;
				if (integerfractal) {
				ftemp = lx0[xdots-1]; xxmax = ftemp / fudge;
				ftemp = lx0[0];       xxmin = ftemp / fudge;
				ftemp = ly0[0];       yymax = ftemp / fudge;
				ftemp = ly0[ydots-1]; yymin = ftemp / fudge;
				} else {
				xxmin = dx0[0];
				xxmax = dx0[xdots-1];	/* re-set xmax and ymax */
				yymin = dy0[ydots-1];
				yymax = dy0[0];
				}

				setup_save_info();

				diskisactive = 1;	/* flag for disk-video routines */
		savetodisk(savename);
				diskisactive = 0;	/* flag for disk-video routines */

}

/* displays differences between current image file and new image */
/* Bert - suggest add this to video.asm */
cmp_line(unsigned char *pixels, int linelen)
{
   static errcount;
   static FILE *fp = NULL;
   extern int rowcount;
   int col;
   int oldcolor;
   char *timestring;
   time_t ltime;
   if(fp == NULL)
      fp = fopen("cmperr",(initbatch)?"a":"w");
   if(rowcount==0)
      errcount=0;
   for(col=0;col<linelen;col++)
   {
      oldcolor=getcolor(col,rowcount);
      if(oldcolor==pixels[col])
	 putcolor(col,rowcount,0);
      else
      {
	 if(oldcolor==0)
	    putcolor(col,rowcount,1);
	 ++errcount;
	 if(initbatch == 0)
	    fprintf(fp,"#%5d col %3d row %3d old %3d new %3d\n",
	       errcount,col,rowcount,oldcolor,pixels[col]);
      }
   }
   rowcount++;
   if(rowcount==ydots && initbatch)
   {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(fp,"%s compare to %s has %5d errs\n",timestring,readname,errcount);
   }
   return(0);
}
