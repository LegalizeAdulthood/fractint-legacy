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

extern struct complex initorbit;
extern char useinitorbit;
extern int RANDOMIZE;	      /* Color randomizing factor */
extern int full_color;	      /* Selects full color with light source fills */
extern int Ambient;	      /* Darkness of shadows in light source */
extern int haze;	      /* Amount of haze to factor in in full color */
extern char Light_Name[];     /* Name of full color .TGA file */
extern unsigned char usemag;
extern char potfile[];		/* potential filename */
extern int video_type;		/* coded value indicating video adapter type */
extern int tgaview();
extern int gifview();
extern void moveboxf(double,double);
extern void chgboxf(double,double);
extern void chgboxi(int,int);
struct fractal_info save_info, read_info; /*  for saving data in file */
extern int biomorph;
extern int askvideo;
extern int periodicitycheck;
extern int forcesymmetry;
extern	char	readname[];	/* name of fractal input file */
extern	int	showfile;	 /* has file been displayed yet? */
#define FUDGEFACTOR	29		/* fudge all values up by 2**this */
#define FUDGEFACTOR2	24		/* (or maybe this)		  */

#define MAXHISTORY	25		/* save this many historical rcds */
struct historystruct {			/* history structure */
	int fractype;			/* fractal type */
	double param[4];		/* parameters */
	double xxmin;			/* top left	*/
	double yymax;			/* top left	*/
	double xxmax;			/* bottom right */
	double yymin;			/* bottom right */
	double xx3rd;			/* bottom left	*/
	double yy3rd;			/* bottom left	*/
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
	double	dxsize, dysize; 		/* xdots-1, ydots-1	    */
	int	colors; 			/* maximum colors available */
	int	maxit;				/* try this many iterations */
	int	boxcount;			/* 0 if no zoom-box yet     */
	int	zrotate;			/* zoombox rotation	    */
	double	zbx,zby;			/* topleft of zoombox	    */
	double	zwidth,zdepth,zskew;		/* zoombox size & shape     */

	int	fractype;			/* if == 0, use Mandelbrot  */
	int	numpasses;			/* 0 if single-pass, else 1 */
	int	solidguessing;			/* 0 if disabled, else 1    */
	long	creal, cimag;			/* real, imag'ry parts of C */
	long	delx, dely;			/* screen pixel increments  */
	long	delx2, dely2;			/* screen pixel increments  */
	double	delxx, delyy;			/* screen pixel increments  */
	double	delxx2, delyy2; 		/* screen pixel increments  */
	long	delmin; 			/* for calcfrac/calcmand    */
	double	ddelmin;			/* same as a double	    */
	double	param[4];			/* up to four parameters    */
	double	potparam[3];		/* three potential parameters*/
	long	fudge;				/* 2**fudgefactor	*/
	int	bitshift;			/* fudgefactor		*/

	int	hasconfig;			/* = 0 if 'fractint.cfg'    */
	int	diskisactive;			/* disk-video drivers flag  */
	int	diskvideo;			/* disk-video access flag   */

	/* note that integer grid is set when integerfractal && !invert;    */
	/* otherwise the floating point grid is set; never both at once     */
	long	far *lx0, far *ly0;		/* x, y grid		    */
	long	far *lx1, far *ly1;		/* adjustment for rotate    */
	/* note that lx1 & ly1 values can overflow into sign bit; since     */
	/* they're used only to add to lx0/ly0, 2s comp straightens it out  */
	double far *dx0, far *dy0;		/* floating pt equivs */
	double far *dx1, far *dy1;
	int	integerfractal; 	/* TRUE if fractal uses integer math */

extern	int	inside; 			/* inside color: 1=blue     */
extern	int	outside;			/* outside color, if set    */
extern	int	finattract;			/* finite attractor option  */
extern	int	cyclelimit;			/* color-rotator upper limit */
extern	int	display3d;			/* 3D display flag: 0 = OFF */
extern	int	overlay3d;			/* 3D overlay flag: 0 = OFF */
extern	int	init3d[20];			/* '3d=nn/nn/nn/...' values */
extern	int	boxcolor;			/* zoom box color */

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
extern int	lookatmouse;		/* used to select mouse mode	*/
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
extern	double	initx3rd,inity3rd;	/* initial corner values    */
extern	double	initparam[4];		/* initial parameters	    */
extern	int	LogFlag;		/* non-zero if logarithmic palettes */
extern	int	transparent[];
extern	int	decomp[];
extern	int	distest;		/* non-zero if distance estimator   */

extern char showbox;	      /* flag to show box and vector in preview */
extern char FormFileName[];   /* file to find (type=)formulas in */
extern char FormName[];       /* Name of the Formula (if not null) */
extern int bailout;	      /* user input bailout value */
extern int mapset;	      /* indicates new map */
extern int warn;	      /* 0 if savename warnings off, 1 if on */
extern unsigned initsavetime; /* timed save interval */
extern char MAP_name[];       /* map file name */

extern int boundarytraceflag; /* boundary tracing (0 = off, 1 = on) */

extern int rflag, rseed;
int	comparegif=0;			/* compare two gif files flag */
int	timedsave=0;			/* when doing a timed save */
extern long saveticks, savebase;	/* timed save vars for general.asm */
extern long readticker();		/* read bios ticker */
int	resave_flag=0;			/* tells encoder not to incr filename */

/* for historical reasons (before rotation):	     */
/*    top    left  corner of screen is (xxmin,yymax) */
/*    bottom left  corner of screen is (xx3rd,yy3rd) */
/*    bottom right corner of screen is (xxmax,yymin) */
double	xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* selected screen corners  */
long	xmin, xmax, ymin, ymax, x3rd, y3rd;  /* integer equivs		 */
double	sxmin,sxmax,symin,symax,sx3rd,sy3rd; /* displayed screen corners */
double	plotmx1,plotmx2,plotmy1,plotmy2;     /* real->screen multipliers */

int calc_status; /* -1 no fractal		    */
		 /*  0 parms changed, recalc reqd   */
		 /*  1 actively calculating	    */
		 /*  2 interrupted, resumable	    */
		 /*  3 interrupted, not resumable   */
		 /*  4 completed		    */
long calctime;

static void adjust_to_limits(double);
static void smallest_add(double *);
static int  ratio_bad(double,double);
int  key_count(int);
static void move_zoombox(int);
static void reset_initparms();

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

if (extraseg == 0		     /* oops.  not enough memory     */
  || (history = (struct historystruct far * ) farmemalloc((unsigned long)
	(MAXHISTORY * sizeof(*history)))) == NULL
  || (ly0 = (long far *) farmemalloc(2000L)) == NULL) {
	buzzer(2);
	printf(" I'm sorry, but you don't have enough free memory \n");
	printf(" to run this program.\n\n");
	exit(1);
	}
farmemfree((unsigned char far *)ly0); /* that was just to check for minimal workspace */

#ifdef __TURBOC__
      dx0 = MK_FP(extraseg,0);
#else
      FP_SEG(dx0)=extraseg;
      FP_OFF(dx0)=0;
#endif
      dy1 = (dx1 = (dy0 = dx0 + MAXPIXELS) + MAXPIXELS) + MAXPIXELS;
      lx0 = (long far *) dx0;
      ly1 = (lx1 = (ly0 = lx0 + MAXPIXELS) + MAXPIXELS) + MAXPIXELS;

diskisactive = 0;			/* disk-video is inactive */
diskvideo = 0;				/* disk driver is not in use */
setvideomode(3,0,0,0);			/* switch to text mode */
calc_status = -1;			/* no active fractal image */

if (debugflag == 10000) {		/* check for free memory */
	char *tempptr;
	unsigned char huge *fartempptr;
	unsigned i,i2;
	long j,j2;
	printf("\n CPU type is %d \n\n FPU type is %d \n\n", cpu, fpu);
	i = j = 0;
	i2 = 0x8000;
	while ((i2 >>= 1) != 0)
	       if ((tempptr = malloc(i+i2)) != NULL) {
		       free(tempptr);
		       i += i2;
		       }
	printf(" %d NEAR bytes free \n", i);
	j2 = 0x80000;
	while ((j2 >>= 1) != 0)
	       if ((fartempptr = (unsigned char huge *)farmemalloc(j+j2)) != NULL) {
		       farmemfree((void far*)fartempptr);
		       j += j2;
		       }
	printf(" %ld FAR bytes free \n\n press any key to continue...\n", j);
	getakey();
	}

main_routine();

}

/* "main_routine" is the main-command-level traffic cop */

main_routine()
{
	double	jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
	double	jxx3rd, jyy3rd;
	int	frommandel;			/* if julia entered from mandel */
	double	atof(), ftemp;			/* floating point stuff    */
	double	ccreal,ccimag;			/* Julia Set Parameters    */
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
historyptr = 0; 			/* initialize history ptr */
history[historyptr].fractype = -1;

restorestart:

lookatmouse = 0;			/* ignore mouse */

read_overlay(); 			/* read overlay/3D files, if reqr'd */

if (overlay3d && initmode < 0) {	/* overlay command failed */
	setforgraphics();		/* restore the graphics screen */
	overlay3d = 0;			/* forget overlays */
	display3d = 0;			/* forget 3D */
	if (calc_status > 0)
		calc_status = 0;
	goto resumeloop;		/* ooh, this is ugly */
	}


restart:				/* insert key re-starts here */

savedac = 0;				/* don't save the VGA DAC */

if (*readname==0 || showfile)
	if (calc_status > 0)		/* goto restart implies re-calc */
		calc_status = 0;

if (initbatch == 0)
	lookatmouse = -1073;		/* just mouse left button, == pgup */

maxit = inititer;				/* max iterations */
numpasses = initpass-1; 			/* single/dual-pass mode */
solidguessing = initsolidguessing;		/* solid-guessing mode */

fractype = initfractype;			/* use the default set	 */

if (distest)
	floatflag = 1;

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
xx3rd = initx3rd; yy3rd = inity3rd;		/* default corner values */
if (fractalspecific[fractype].flags&NOROTATE != 0) {
   /* ensure min<max and unrotated rectangle */
   if (xxmin > xxmax) { ftemp = xxmax; xxmax = xxmin; xxmin = ftemp; }
   if (yymin > yymax) { ftemp = yymax; yymax = yymin; yymin = ftemp; }
   xx3rd = xxmin; yy3rd = yymin;
}

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

fudge = 1; fudge = fudge << bitshift;	       /* fudged value for printfs */

frommandel = 0;

adapter = initmode;			/* set the video adapter up	*/
initmode = -1;				/* (once)			*/

helpmode = HELPAUTHORS; 	       /* use this help mode */
if (adapter < 0) {
	calc_status = -1;		/* no active fractal image */
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
		solidguessing = boundarytraceflag = 0;
		continue;
		}
	if (kbdchar == 'g' || kbdchar == 'G') { /* solid-guessing */
		numpasses = 1;
		boundarytraceflag = 0;
		solidguessing = 1;
		continue;
		}
	if (kbdchar == 'n' || kbdchar == 'N') { /* normal palette */
		LogFlag = 0;
		/* ChkLogFlag(); */
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
		get_obsolete(); 		/* display obsolete msg */
		goto restart;			/* restore the screen */
		}
	else
		buzzer(2);
	}

zoomoff = 1;					/* zooming is enabled */

helpmode = HELPMAIN;				/* switch help modes */


more = 1;
while (more) {					/* eternal loop */

	if (calc_status != 2 || (*readname && showfile==0)) {
						/* collect adapter info */
		fromvideotable(adapter);
		axmode	= videoentry.videomodeax; /* video mode (BIOS call) */
		bxmode	= videoentry.videomodebx; /* video mode (BIOS call) */
		cxmode	= videoentry.videomodecx; /* video mode (BIOS call) */
		dxmode	= videoentry.videomodedx; /* video mode (BIOS call) */
		dotmode = videoentry.dotmode;	/* assembler dot read/write */
		xdots	= videoentry.xdots;	/* # dots across the screen */
		ydots	= videoentry.ydots;	/* # dots down the screen   */
		colors	= videoentry.colors;	/* # colors available */
		if(debugflag==1984)
		{  /* quick calc of small image */
		   xdots = xdots>>2;
		   ydots = ydots>>2;
		}
		dxsize = xdots - 1;		/* convert just once now */
		dysize = ydots - 1;

		diskvideo = 0;			/* set diskvideo flag */
		if (dotmode == 11)		/* default assumption is disk */
			diskvideo = 2;

		memcpy(olddacbox,dacbox,256*3); /* save the DAC */
		diskisactive = 1;		/* flag for disk-video routines */
		if (overlay3d) {
			setforgraphics();	/* restore old graphics image */
			overlay3d = 0;
			}
		else
			setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */
		diskisactive = 0;		/* flag for disk-video routines */
		if (savedac) {
			memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
			if (dotmode != 11)
				spindac(0,1);
			}
		else if (dotmode == 11 && colors == 256) { /* disk video */
			FILE *dacfile;
			findpath("default.map",temp1);
			dacfile = fopen(temp1,"r");
			if (dacfile != NULL) {
				ValidateLuts(dacfile);	/* read the palette file */
				fclose(dacfile);
				}
			}
		}

	savedac = 1;				/* assume we save next time */

	if (initbatch == 0)
		lookatmouse = -1073;		/* mouse left button == pgup */

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
	else
	   calc_status = -1;
	if(status == -1 && keypressed())
	       getakey();
	/*	display3d = 0;			   turn off 3D retrievals */
		}

	integerfractal = fractalspecific[fractype].isinteger;

	zoomoff = 1;				/* zooming is enabled */
	if (dotmode == 11 || (fractalspecific[fractype].flags&NOZOOM) != 0)
		zoomoff = 0;	/* for these situations disable zooming */

	ccreal = param[0]; ccimag = param[1];

	adjust_to_limits(1.0); /* make sure all corners in valid range */

	delxx  = (xxmax - xx3rd) / dxsize; /* calculate stepsizes */
	delyy  = (yymax - yy3rd) / dysize;
	delxx2 = (xx3rd - xxmin) / dysize;
	delyy2 = (yy3rd - yymin) / dxsize;

	creal = ccreal * fudge; /* integer equivs for it all */
	cimag = ccimag * fudge;
	xmin  = xxmin * fudge;
	xmax  = xxmax * fudge;
	x3rd  = xx3rd * fudge;
	ymin  = yymin * fudge;
	ymax  = yymax * fudge;
	y3rd  = yy3rd * fudge;
	delx  = delxx * fudge;
	dely  = delyy * fudge;
	delx2 = delxx2 * fudge;
	dely2 = delyy2 * fudge;

	if (integerfractal && !invert)
	{
		if ( (delx  == 0 && delxx  != 0.0)
		  || (delx2 == 0 && delxx2 != 0.0)
		  || (dely  == 0 && delyy  != 0.0)
		  || (dely2 == 0 && delyy2 != 0.0) )
			goto expand_retry;
		lx0[0] = xmin;			/* fill up the x, y grids */
		ly0[0] = ymax;
		lx1[0] = ly1[0] = 0;
		for (i = 1; i < xdots; i++ )
		{
			lx0[i] = lx0[i-1] + delx;
			ly1[i] = ly1[i-1] - dely2;
		}
		for (i = 1; i < ydots; i++ )
		{
			ly0[i] = ly0[i-1] - dely;
			lx1[i] = lx1[i-1] + delx2;
		}
		/* past max res?  check corners within 10% of expected */
		if (  ratio_bad((double)lx0[xdots-1]-xmin,(double)xmax-x3rd)
		   || ratio_bad((double)ly0[ydots-1]-ymax,(double)y3rd-ymax)
		   || ratio_bad((double)lx1[(ydots>>1)-1],((double)x3rd-xmin)/2)
		   || ratio_bad((double)ly1[(xdots>>1)-1],((double)ymin-y3rd)/2) )
		{
expand_retry:		if (integerfractal &&	/* integer fractal type? */
			    fractalspecific[fractype].tofloat != NOFRACTAL)
				floatflag = 1;	/* switch to floating pt */
			else
				adjust_to_limits(2.0); /* double the size */
			reset_initparms();
			goto restart;
		}
		/* re-set corners to match reality */
		xmax = lx0[xdots-1] + lx1[ydots-1];
		ymin = ly0[ydots-1] + ly1[xdots-1];
		x3rd = xmin + lx1[ydots-1];
		y3rd = ly0[ydots-1];
		xxmin = (double)xmin / fudge;
		xxmax = (double)xmax / fudge;
		xx3rd = (double)x3rd / fudge;
		yymin = (double)ymin / fudge;
		yymax = (double)ymax / fudge;
		yy3rd = (double)y3rd / fudge;
	}
	else
	{
		/* set up dx0 and dy0 analogs of lx0 and ly0 */
		/* put fractal parameters in doubles */
		dx0[0] = xxmin; 		/* fill up the x, y grids */
		dy0[0] = yymax;
		dx1[0] = dy1[0] = 0;
		for (i = 1; i < xdots; i++ )
		{
			dx0[i] = dx0[i-1] + delxx;
			dy1[i] = dy1[i-1] - delyy2;
		}
		for (i = 1; i < ydots; i++ )
		{
			dy0[i] = dy0[i-1] - delyy;
			dx1[i] = dx1[i-1] + delxx2;
		}
		if (  ratio_bad(dx0[xdots-1]-xxmin,xxmax-xx3rd)
		   || ratio_bad(dy0[ydots-1]-yymax,yy3rd-yymax)
		   || ratio_bad(dx1[ydots-1],xx3rd-xxmin)
		   || ratio_bad(dy1[xdots-1],yymin-yy3rd))
			goto expand_retry;
		/* re-set corners to match reality */
		xxmax = dx0[xdots-1] + dx1[ydots-1];
		yymin = dy0[ydots-1] + dy1[xdots-1];
		xx3rd = xxmin + dx1[ydots-1];
		yy3rd = dy0[ydots-1];
	}

	/* for periodicity close-enough, and for unity: */
	/*     min(max(delx,delx2),max(dely,dely2)	*/
	ddelmin = fabs(delxx);
	if (fabs(delxx2) > ddelmin)
		ddelmin = fabs(delxx2);
	if (fabs(delyy) > fabs(delyy2)) {
		if (fabs(delyy) < ddelmin)
			ddelmin = fabs(delyy);
	}
	else
		if (fabs(delyy2) < ddelmin)
			ddelmin = fabs(delyy2);
	delmin = ddelmin * fudge;

	/* calculate factors which plot real values to screen co-ords */
	/* calcfrac.c plot_orbit routines have comments about this    */
	ftemp = (0.0-delyy2) * delxx2 * dxsize * dysize
		- (xxmax-xx3rd) * (yy3rd-yymax);
	plotmx1 = delxx2 * dxsize * dysize / ftemp;
	plotmx2 = (yy3rd-yymax) * dxsize / ftemp;
	plotmy1 = (0.0-delyy2) * dxsize * dysize / ftemp;
	plotmy2 = (xxmax-xx3rd) * dysize / ftemp;

	sxmin = xxmin; /* save 3 corners for zoom.c ref points */
	sxmax = xxmax;
	sx3rd = xx3rd;
	symin = yymin;
	symax = yymax;
	sy3rd = yy3rd;

	 if ((fractype == MANDEL || fractype == JULIA) && bitshift == 29)
	    decomp[1] = 0;	/* make the world safe for decomposition */

	if (history[0].fractype == -1)		/* initialize the history file */
		for (i = 0; i < MAXHISTORY; i++) {
			history[i].xxmax = xxmax;
			history[i].xxmin = xxmin;
			history[i].yymax = yymax;
			history[i].yymin = yymin;
			history[i].xx3rd = xx3rd;
			history[i].yy3rd = yy3rd;
			history[i].param[0] = param[0];
			history[i].param[1] = param[1];
			history[i].fractype = fractype;
		}

	if (history[historyptr].xxmax != xxmax	||	/* save any (new) zoom data */
	    history[historyptr].xxmin != xxmin	||
	    history[historyptr].yymax != yymax	||
	    history[historyptr].yymin != yymin	||
	    history[historyptr].xx3rd != xx3rd	||
	    history[historyptr].yy3rd != yy3rd	||
	    history[historyptr].param[0] != param[0] ||
	    history[historyptr].param[1] != param[1] ||
	    history[historyptr].fractype != fractype) {
		if (++historyptr == MAXHISTORY) historyptr = 0;
		history[historyptr].xxmax = xxmax;
		history[historyptr].xxmin = xxmin;
		history[historyptr].yymax = yymax;
		history[historyptr].yymin = yymin;
		history[historyptr].xx3rd = xx3rd;
		history[historyptr].yy3rd = yy3rd;
		history[historyptr].param[0] = param[0];
		history[historyptr].param[1] = param[1];
		history[historyptr].fractype = fractype;
		}

	if(*readname && showfile==0) {
		showfile = 1;
		if (initbatch == 1 && calc_status == 2)
			initbatch = -1; /* flag to finish calc before save */
		}
	else	{
		diskisactive = 1;	/* flag for disk-video routines */
	/* TW 07/21/89 - see below */
	/* set save parameters in save structure */
		setup_save_info();
		if (initsavetime != 0		/* autosave and resumable? */
		    && (fractalspecific[fractype].flags&NORESUME) == 0) {
			savebase = readticker(); /* calc's start time */
			saveticks = initsavetime * 1092; /* bios ticks/minute */
			if ((saveticks & 65535) == 0)
				++saveticks;	/* make low word nonzero */
			}

		if ((i = calcfract()) == 0)	/* draw the fractal using "C" */
			buzzer(0);		/* finished!! */
		saveticks = 0;			/* turn off autosave timer */
		if( dotmode == 9 || dotmode == 11 ) {	/* if TARGA or disk-video */
			if( dotmode == 11 )	/* TARGA already has some text up */
				home();
			else
				EndTGA();	/* make sure TARGA is OFF */
			if (i == 0)
				printf("Image has been completed");
			}

		diskisactive = 0;	/* flag for disk-video routines */
		}

	boxcount = 0;				/* no zoom box yet  */
	zwidth = 0;

	if (fractype == PLASMA && cpu > 88) {
		cyclelimit = 256;		/* plasma clouds need quick spins */
		daccount = 256;
		daclearn = 1;
		}

resumeloop:					/* return here on failed overlays */

	kbdmore = 1;
	while (kbdmore == 1) {			/* loop through cursor keys */
		if (timedsave != 0) {
			if (timedsave == 1) {	/* woke up for timed save */
				getakey();	/* eat the dummy char */
				kbdchar = 's';  /* do the save */
				timedsave = 2;
				}
			else {			/* save done, resume */
				timedsave = 0;
				resave_flag = 1;
				kbdchar = 13;
				}
			}
		else if (initbatch == 0) {	/* online only, please */
			lookatmouse = (zwidth == 0) ? -1073 : 3;
			while (!keypressed());	/* enables help */
			kbdchar = getakey();
			}
		else {				/* batch mode special  */
			if (initbatch == -1) {	/* finish calc */
				kbdchar = 13;
				initbatch = 1;
				}
			else if (initbatch == 1) { /* save-to-disk     */
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
				frommandel = 0;
				goto restart;
				break;
			case 'x':                       /* boundary tracing toggle */
			case 'X':
				setfortext();		/* switch to text mode */
				i = get_toggles();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				if (i > 0)		/* time to redraw? */
					kbdmore = calc_status = 0;
				if (i == 2) {		/* float change? */
					reset_initparms();
					goto restart;
					}
				break;
			case 'f':                       /* floating pt toggle */
			case 'F':
				if (floatflag == 0)
					floatflag = 1;
				else
					floatflag = 0;
				reset_initparms();
				goto restart;
			case 'e':                       /* new IFS parms    */
			case 'E':
				setfortext();		/* switch to text mode */
				get_ifs_params();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				calc_status = kbdmore = 0;
				break;
			case 'i':                       /* inversion parms    */
			case 'I':
				setfortext();		/* switch to text mode */
				get_invert_params();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				reset_initparms();
				invert = (inversion[0]==0.0) ? 0 : 3;
				goto restart;
			case 'q':                       /* decomposition parms    */
			case 'Q':
				setfortext();		/* switch to text mode */
				get_decomp_params();	/* get the parameters */
				setforgraphics();	/* back to graphics */
				kbdmore = calc_status = 0;
				break;
			case 'a':                       /* starfield parms    */
			case 'A':
				get_starfield_params(); /* get the parameters */
				calc_status = 0;
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

				   jxxmin = sxmin; jxxmax = sxmax;
				   jyymax = symax; jyymin = symin;
				   jxx3rd = sx3rd; jyy3rd = sy3rd;
				   frommandel = 1;

				   xxmin = fractalspecific[fractype].xmin;
				   xxmax = fractalspecific[fractype].xmax;
				   yymin = fractalspecific[fractype].ymin;
				   yymax = fractalspecific[fractype].ymax;
				   xx3rd = xxmin;
				   yy3rd = yymin;

				   if(biomorph != -1 && bitshift != 29) {
				      xxmin *= 3.0;
				      xxmax *= 3.0;
				      yymin *= 3.0;
				      yymax *= 3.0;
				      xx3rd *= 3.0;
				      yy3rd *= 3.0;
				      }

				   zoomoff = 1;
				   calc_status = 0;
				   kbdmore = 0;
				   }

				else if (fractalspecific[fractype].tomandel != NOFRACTAL) {
				   /* switch to corresponding Mandel set */
				   fractype = fractalspecific[fractype].tomandel;
				   if (frommandel) {
				      xxmin = jxxmin;  xxmax = jxxmax;
				      yymin = jyymin;  yymax = jyymax;
				      xx3rd = jxx3rd;  yy3rd = jyy3rd;
				      }
				   else {
				      ccreal = (fractalspecific[fractype].xmax - fractalspecific[fractype].xmin) / 2;
				      ccimag = (fractalspecific[fractype].ymax - fractalspecific[fractype].ymin) / 2;
				      xxmin = xx3rd = param[0] - ccreal;
				      xxmax = param[0] + ccreal;
				      yymin = yy3rd = param[1] - ccimag;
				      yymax = param[1] + ccimag;
				      }
				   ccreal = 0;
				   ccimag = 0;
				   param[0] = 0;
				   param[1] = 0;
				   zoomoff = 1;
				   calc_status = 0;
				   kbdmore = 0;
				   }

				else buzzer(2); 	/* no switch */
				break;
			case 1071:			/* home */
				if (--historyptr < 0)
					historyptr = MAXHISTORY-1;
				xxmax  = history[historyptr].xxmax;
				xxmin  = history[historyptr].xxmin;
				yymax  = history[historyptr].yymax;
				yymin  = history[historyptr].yymin;
				xx3rd  = history[historyptr].xx3rd;
				yy3rd  = history[historyptr].yy3rd;
				param[0] = history[historyptr].param[0];
				param[1] = history[historyptr].param[1];
				fractype = history[historyptr].fractype;
				ccreal = param[0]; ccimag = param[1];
				zoomoff = 1;
				reset_initparms();
				if (fractalspecific[fractype].isinteger != 0 &&
					fractalspecific[fractype].tofloat != NOFRACTAL)
					floatflag = 0;
				if (fractalspecific[fractype].isinteger == 0 &&
					fractalspecific[fractype].tofloat != NOFRACTAL)
					floatflag = 1;
				goto restart;
			case 'd':                       /* shell to MS-DOS */
			case 'D':
				setfortext();
				printf("\n\nShelling to DOS - type 'exit' to return\n\n");
				if (axmode == 0 || axmode > 7) {
static char far dosmsg[]={"\
Note:  Your graphics image is still squirreled away in your video\n\
adapter's memory.  Switching video modes (say, to get your cursor back)\n\
will clobber part of that image.  Sorry - it's the best we could do.\n\n"};
					helpmessage(dosmsg);
					}
				shell_to_dos();
				setforgraphics();
				calc_status = 0;
				break;
			case '<':                       /* lower iter maximum */
			case ',':
				if (maxit >= 10+initincr) maxit -= initincr;
				maxit -= initincr;	/* for fall-thru */
			case '>':                       /* raise iter maximum */
			case '.':
				if (maxit <= 32000-initincr) maxit += initincr;
				calc_status = 0;
				continue;
			case 'c':                       /* switch to cycling */
			case 'C':
				rotate(0);
				continue;
			case '+':                       /* rotate palette */
				rotate(+1);
				continue;
			case '-':                       /* rotate palette */
				rotate(-1);
				continue;
			case 's':                       /* save-to-disk */
			case 'S':
				saveanimage();
				resave_flag = 0;
				continue;
			case 'o':                       /* 3D overlay */
			case 'O':
				overlay3d = 1;
			case '3':                       /* restore-from (3d) */
				display3d = 1;
			case 'r':                       /* restore-from */
			case 'R':
				comparegif = 0;
				frommandel = 0;
				if(kbdchar == 'r' || kbdchar == 'R')
				{
				   if(debugflag == 50)
				   {
				      comparegif = overlay3d = 1;
				      if (initbatch == 2)
				      {
					 setfortext();	 /* save graphics image */
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
				resave_flag = 0;
				goto restorestart;
				break;
			case '1':                       /* single-pass mode */
			case '2':                       /* dual-pass mode */
				numpasses = kbdchar - '1';
				solidguessing = boundarytraceflag = 0;
				kbdmore = calc_status = 0;
				break;
			case 'g':                       /* solid-guessing */
			case 'G':
				numpasses = 1;
				solidguessing = 1;
				boundarytraceflag = 0;
				kbdmore = calc_status = 0;
				break;
			case 'n':                       /* normal palette */
			case 'N':
				LogFlag = 0;
				kbdmore = calc_status = 0;
				break;
			case 'l':                       /* log palette */
			case 'L':
				LogFlag = 1;
				kbdmore = calc_status = 0;
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
				init_pan_or_recalc(0);
				kbdmore = 0;
				break;
			case 10:			/* control-Enter */
			case 1010:			/* Control-Keypad Enter */
				init_pan_or_recalc(1);
				kbdmore = 0;
				zoomout(); /* calc corners for zooming out */
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
			case 1077:			/* cursor right */
			case 1072:			/* cursor up */
			case 1080:			/* cursor down */
			case 1115:			/* Ctrl-cursor left */
			case 1116:			/* Ctrl-cursor right */
			case 1141:			/* Ctrl-cursor up */
			case 1145:			/* Ctrl-cursor down */
				move_zoombox(kbdchar);
				break;
			case 1119:			/* Ctrl-home */
				if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0) {
					i = key_count(1119);
					if ((zskew -= 0.02*i) < -0.48)
						zskew = -0.48;
				}
				break;
			case 1117:			/* Ctrl-end */
				if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0) {
					i = key_count(1117);
					if ((zskew += 0.02*i) > 0.48)
						zskew = 0.48;
				}
				break;
			case 1132:			/* Ctrl-pgup */
				if (boxcount)
					chgboxi(0,-2*key_count(1132));
				break;
			case 1118:			/* Ctrl-pgdn */
				if (boxcount)
					chgboxi(0,2*key_count(1118));
				break;
			case 1073:			/* page up */
				if (zoomoff == 1)
					if (zwidth == 0) { /* start zoombox */
						zwidth = 1;
						if ((boxcolor=colors-1) > 15)
						    boxcolor = 15; /* white? */
						zdepth = 0.75;
						zrotate = zskew = 0;
						zbx = zby = 0;
					}
					else {
						i = key_count(1073);
						chgboxf(-0.036*i,-0.027*i);
					}
				break;
			case 1081:			/* page down */
				if (boxcount) {
					if (zwidth >= 1) /* end zoombox */
						zwidth = 0;
					else {
						i = key_count(1081);
						chgboxf(0.036*i,0.027*i);
					}
				}
				break;
			case 1142:			/* Ctrl-kpad- */
				if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0)
					zrotate += key_count(1142);
				break;
			case 1144:			/* Ctrl-kpad+ */
				if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0)
					zrotate -= key_count(1144);
				break;
			case 1146:			/* Ctrl-ins */
				boxcolor += key_count(1146);
				break;
			case 1147:			/* Ctrl-del */
				boxcolor -= key_count(1147);
				break;
			case 'h':                       /* obsolete command key */
			case 'H':
				setfortext();		/* switch to text mode */
				get_obsolete(); 	/* inform user */
				setforgraphics();	/* back to graphics */
				break;
			default:		/* other (maybe a valid Fn key) */
				for (k = 0; k < maxvideomode; k++)
					if (kbdchar == kbdkeys[k]) {
						adapter = k;
						kbdmore = 0;
						fromvideotable(adapter);
						if (videoentry.dotmode != 11
						  || videoentry.colors != colors)
							savedac = 0;
						calc_status = 0;
						}
				if (kbdmore != 0)
					continue;
				break;
			}

		if (zoomoff == 1 && kbdmore == 1) /* draw/clear a zoom box? */
			drawbox(1);
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

		if(read_info.version > 0)
	    {
			initparam[2]  = read_info.parm3;
			initparam[3]  = read_info.parm4;
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
		   outside	   = read_info.outside;
	    }

		calc_status = 0;      /* defaults if version < 4 */
		initx3rd = initxmin;
		inity3rd = initymin;
		distest = 0;
		calctime = 0;
		if(read_info.version > 3)
	    {
		   initx3rd = read_info.x3rd;
		   inity3rd = read_info.y3rd;
		   calc_status = read_info.calc_status;
		   boundarytraceflag = initsolidguessing = 0;
		   initpass = 2;
		   switch (read_info.stdcalcmode) {
		      case '1': initpass = 1;
				break;
		      case '2': initpass = 2;
				break;
		      case 'b': boundarytraceflag = 1;
				break;
		      default:	initsolidguessing = 1;
		      }
		   distest = read_info.distest;
		   floatflag = read_info.floatflag;
		   bailout = read_info.bailout;
		   calctime = read_info.calctime;
		   trigndx[0] = read_info.trigndx[0];
		   trigndx[1] = read_info.trigndx[1];
		   trigndx[2] = read_info.trigndx[2];
		   trigndx[3] = read_info.trigndx[3];
		   finattract = read_info.finattract;
	   initorbit.x	= read_info.initorbit[0];
	   initorbit.y	= read_info.initorbit[1];
	   useinitorbit = read_info.useinitorbit;
		   periodicitycheck = read_info.periodicity;
	    }
		if(read_info.version < 4) {
		   backwardscompat(); /* translate obsolete types */
		   if (LogFlag)
		      LogFlag = 2;
		   floatflag = (fractalspecific[initfractype].isinteger) ? 0 : 1;
		   }
		fractalspecific[initfractype].paramvalue[0] = initparam[0];
		fractalspecific[initfractype].paramvalue[1] = initparam[1];
		fractalspecific[initfractype].paramvalue[2] = initparam[2];
		fractalspecific[initfractype].paramvalue[3] = initparam[3];
		set_trig_pointers(-1);

		showfile = 0;
	if (!overlay3d) {		/* don't worry about the video if overlay */
		if ((hasconfig || askvideo) && read_info.info_id[0] == 'G') {
static char far unknownmsg[]={"\
You have selected an unknown (non-fractal) GIF image\n\
I am treating this image as a PLASMA CLOUD of unknown video type\n"};
			buzzer(2);
			helpmessage(unknownmsg);
			}
		if (read_info.dotmode == 11 && ! initbatch) {
static char far novidmsg[]={"\
You have selected a fractal image generated in 'no-video' mode\n"};
			buzzer(2);
			helpmessage(novidmsg);
			}
		if ((hasconfig || askvideo) && read_info.info_id[0] == 'G')
			printf("with resolution %d x %d x %d\n",
				read_info.xdots, read_info.ydots, read_info.colors);
		else
			initmode = getGIFmode(&read_info);
		if (initmode >= maxvideomode) initmode = -1;
		if (initmode >= 0 && (hasconfig || askvideo) && ! initbatch ){
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
			if (videoentry.dotmode == 11)
				printf("Restore in same disk video mode? ([Y] or N) --> ");
			else
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
		if (initmode == -1)
			calc_status = 0;
		if (initmode == -1 && ! initbatch) {
static char far selmodemsg[]={"\
\nPlease select a video mode with which to view or recreate this\n\
image by pressing the appropriate function key (or press the ENTER)\n\
key to bail out without displaying this image).  You can also press\n\
the F1 or '?' keys to get help on the available video modes)\n"};
			helpmessage(selmodemsg);
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
			if (initmode >= 0 && display3d == 0 && (
				read_info.xdots != videoentry.xdots ||
				read_info.ydots != videoentry.ydots)) {
					*readname = 0; /* force regen */
					return(0);
					}
				}

		if (initmode >= 0 && display3d) {
			calc_status = 0;
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
   double Xctr, Yctr, Magnification;
   FILE *batch;
   int numfn;
   batch = fopen("frabatch.bat", "a");
   if(batch == NULL)
      return(-1);
   fprintf(batch, "fractint");

   /********************************/
   /* universal parameters go here */
   /********************************/
   if(askvideo==0)
      fprintf(batch, " askvideo=no");
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
   if (inside == -1)
      fprintf(batch, " inside=maxiter");
   else if (inside == -60)
      fprintf(batch, " inside=bof60");
   else if (inside == -61)
      fprintf(batch, " inside=bof61");
   else if (inside != 1)
      fprintf(batch, " inside=%d", inside);
   if (finattract)
      fprintf(batch, " finattract=yes");
   if (outside != -1)
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
      fprintf(batch, (LogFlag == 1) ? " logmap=yes" : " logmap=old");
   if(rflag)
      fprintf(batch, " rseed=%d",rseed);

   showtrig(batch); /* this function is in prompts.c */

   /***********************************/
   /* fractal only parameters go here */
   /***********************************/
   if (display3d <= 0)	/* a fractal was generated */
   {
      if(periodicitycheck != 1)
         fprintf(batch, " periodicity=%d",periodicitycheck);

      if(potparam[0])
      {
	 fprintf(batch, " potential=%d/%d/%d",
	     (int)potparam[0],(int)potparam[1],(int)potparam[2]);
	 if(potfile[0])
	    fprintf(batch, "/%s", potfile);
      }
      if (fractalspecific[fractype].name[0] != '*')
	 fprintf(batch, " type=%s", fractalspecific[fractype].name);
      else
	 fprintf(batch, " type=%s", &fractalspecific[fractype].name[1]);
      if(usemag && cvtcentermag(&Xctr, &Yctr, &Magnification))
      {
	 if (delmin > 1000)
	    fprintf(batch, " center-mag=%g/%g/%g",
		Xctr,Yctr,Magnification);
	 else
	    fprintf(batch, " center-mag=%+20.17lf/%+20.17lf/%+20.17lf",
		Xctr,Yctr,Magnification);
      }
      else
      {
	 if (delmin > 1000) {
	    fprintf(batch, " corners=%g/%g/%g/%g",
		xxmin, xxmax, yymin, yymax);
	    if (xx3rd != xxmin || yy3rd != yymin)
	       fprintf(batch, "/%g/%g", xx3rd, yy3rd);
	 }
	 else {
	    fprintf(batch, " corners=%+20.17lf/%+20.17lf/%+20.17lf/%+20.17lf",
		xxmin, xxmax, yymin, yymax);
	    if (xx3rd != xxmin || yy3rd != yymin)
	       fprintf(batch, "/%+20.17lf/%+20.17lf", xx3rd, yy3rd);
	 }
      }

      if(param[0]!=0.0 || param[1]!=0.0 || param[2]!=0.0 || param[3]!=0.0)
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
      if (distest)
	 fprintf(batch, " distest=%d", distest);
      if (floatflag)
	 fprintf(batch, " float=yes");
      if (biomorph != -1)
	 fprintf(batch, " biomorph=%d", biomorph);
      if(bailout && fabs(potparam[2]) == 0.0 &&
	    (decomp[0] <= 0 || decomp[1] <= 0))
	 fprintf(batch, " bailout=%d",bailout);
      if (fractype == FORMULA || fractype == FFORMULA)
      {
	 fprintf(batch, " formulafile=%s",FormFileName);
	 fprintf(batch, " formulaname=%s",FormName);
      }
      if(useinitorbit == 2)
	 fprintf(batch, " initorbit=pixel");
      else if(useinitorbit == 1)
	 fprintf(batch, " initorbit=%g/%g",initorbit.x,initorbit.y);
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

      if (RANDOMIZE)
	 fprintf(batch, " randomize=%d",RANDOMIZE);
      if (full_color)
	 fprintf(batch, " fullcolor=yes");
      if (Ambient)
	 fprintf(batch, " ambient=%d",Ambient);
      if (haze)
	 fprintf(batch, " haze=%d",haze);
      if (full_color)
      {
	 /* strip path from file name */
	 char *p;
	 p = strrchr(Light_Name,'\\');
	 if(p)
	    fprintf(batch, " lightname=%s", p+1);
	 else
	    fprintf(batch, " lightname=%s", Light_Name);
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
   return(0);
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
   save_info.version	     = 4;
   save_info.iterations      = maxit;
   save_info.fractal_type    = fractype;
   save_info.xmin	     = xxmin;
   save_info.xmax	     = xxmax;
   save_info.ymin	     = yymin;
   save_info.ymax	     = yymax;
   save_info.creal	     = param[0];
   save_info.cimag	     = param[1];
   save_info.videomodeax     = videoentry.videomodeax;
   save_info.videomodebx     = videoentry.videomodebx;
   save_info.videomodecx     = videoentry.videomodecx;
   save_info.videomodedx     = videoentry.videomodedx;
   save_info.dotmode	     = videoentry.dotmode;
   save_info.xdots	     = videoentry.xdots;
   save_info.ydots	     = videoentry.ydots;
   save_info.colors	     = videoentry.colors;
   save_info.parm3	     = param[2];
   save_info.parm4	     = param[3];
   save_info.potential[0]    = potparam[0];
   save_info.potential[1]    = potparam[1];
   save_info.potential[2]    = potparam[2];
   save_info.rflag	     = rflag;
   save_info.rseed	     = rseed;
   save_info.inside	     = inside;
   save_info.logmap	     = LogFlag;
   save_info.invert[0]	     = inversion[0];
   save_info.invert[1]	     = inversion[1];
   save_info.invert[2]	     = inversion[2];
   save_info.decomp[0]	     = decomp[0];
   save_info.decomp[1]	     = decomp[1];
   save_info.biomorph	     = biomorph;
   save_info.symmetry	     = forcesymmetry;
   for (i = 0; i < 16; i++)
      save_info.init3d[i] = init3d[i];
   save_info.previewfactor   = previewfactor;
   save_info.xtrans	     = xtrans;
   save_info.ytrans	     = ytrans;
   save_info.red_crop_left   = red_crop_left;
   save_info.red_crop_right  = red_crop_right;
   save_info.blue_crop_left  = blue_crop_left;
   save_info.blue_crop_right = blue_crop_right;
   save_info.red_bright      = red_bright;
   save_info.blue_bright     = blue_bright;
   save_info.xadjust	     = xadjust;
   save_info.eyeseparation   = eyeseparation;
   save_info.glassestype     = glassestype;
   save_info.outside	     = outside;
   save_info.x3rd	     = xx3rd;
   save_info.y3rd	     = yy3rd;
   save_info.calc_status     = calc_status;
   if (boundarytraceflag)
      save_info.stdcalcmode  = 'b';
   else if (solidguessing)
      save_info.stdcalcmode  = 'g';
   else
      save_info.stdcalcmode  = '1' + numpasses;
   save_info.distest	     = distest;
   save_info.floatflag	     = floatflag;
   save_info.bailout	     = bailout;
   save_info.calctime	     = calctime;
   save_info.trigndx[0]      = trigndx[0];
   save_info.trigndx[1]      = trigndx[1];
   save_info.trigndx[2]      = trigndx[2];
   save_info.trigndx[3]      = trigndx[3];
   save_info.finattract      = finattract;
   save_info.initorbit[0]    = initorbit.x;
   save_info.initorbit[1]    = initorbit.y;
   save_info.useinitorbit    = useinitorbit;
   save_info.periodicity     = periodicitycheck;
   for (i = 0; i < sizeof(save_info.future)/sizeof(int); i++)
      save_info.future[i] = 0;
}

saveanimage()
{
   double ftemp;
   zwidth = 0;
   drawbox(0);		   /* clobber zoom-box */
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = yymax;
   yymin = yymin;
   yy3rd = sy3rd;
   setup_save_info();
   diskisactive = 1;	   /* flag for disk-video routines */
   savetodisk(savename);
   diskisactive = 0;	   /* flag for disk-video routines */
}

static void reset_initparms()
{  int i;
   inititer = maxit;
   initfractype = fractype;
   initmode = adapter;
   initxmin = xxmin;
   initxmax = xxmax;
   initymin = yymin;
   initymax = yymax;
   initx3rd = xx3rd;
   inity3rd = yy3rd;
   for (i = 0; i < 4; i++)
      initparam[i] = param[i];
}

adjust_corner()
{  double ftemp,ftemp2;
   /* make edges very near vert/horiz exact, to ditch rounding errs and */
   /* to avoid problems when delta per axis makes too large a ratio	*/
   if( (ftemp=fabs(xx3rd-xxmin)) < (ftemp2=fabs(xxmax-xx3rd)) ) {
      if (ftemp*10000 < ftemp2 && yy3rd != yymax)
	 xx3rd = xxmin;
   }
   else if (ftemp2*10000 < ftemp && yy3rd != yymin)
      xx3rd = xxmax;
   if( (ftemp=fabs(yy3rd-yymin)) < (ftemp2=fabs(yymax-yy3rd)) ) {
      if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
	 yy3rd = yymin;
      }
   else if (ftemp2*10000 < ftemp && xx3rd != xxmin)
      yy3rd = yymax;
}

static void adjust_to_limits(double expand)
{  double cornerx[4],cornery[4];
   double lowx,highx,lowy,highy,limit,ftemp;
   double centerx,centery,adjx,adjy;
   int i;
   limit = 32767.99;
   if (bitshift >= 24) limit = 31.99;
   if (bitshift >= 29) limit = 3.99;
   centerx = (xxmin+xxmax)/2;
   centery = (yymin+yymax)/2;
   if (xxmin == centerx) { /* ohoh, infinitely thin, fix it */
      smallest_add(&xxmax);
      xxmin -= xxmax-centerx;
      }
   if (yymin == centery) {
      smallest_add(&yymax);
      yymin -= yymax-centery;
      }
   if (xx3rd == centerx)
      smallest_add(&xx3rd);
   if (yy3rd == centery)
      smallest_add(&yy3rd);
   /* setup array for easier manipulation */
   cornerx[0] = xxmin; cornerx[1] = xxmax;
   cornerx[2] = xx3rd; cornerx[3] = xxmin+(xxmax-xx3rd);
   cornery[0] = yymax; cornery[1] = yymin;
   cornery[2] = yy3rd; cornery[3] = yymin+(yymax-yy3rd);
   /* if caller wants image size adjusted, do that first */
   if (expand != 1.0)
      for (i=0; i<4; ++i) {
	 cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
	 cornery[i] = centery + (cornery[i]-centery)*expand;
      }
   /* get min/max x/y values */
   lowx = highx = cornerx[0];
   lowy = highy = cornery[0];
   for (i=1; i<4; ++i) {
      if (cornerx[i] < lowx)  lowx  = cornerx[i];
      if (cornerx[i] > highx) highx = cornerx[i];
      if (cornery[i] < lowy)  lowy  = cornery[i];
      if (cornery[i] > highy) highy = cornery[i];
      }
   /* if image is too large, downsize it maintaining center */
   ftemp = highx-lowx;
   if (highy-lowy > ftemp) ftemp = highy-lowy;
   if ((ftemp = limit*2/ftemp) < 1.0)
      for (i=0; i<4; ++i) {
	 cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
	 cornery[i] = centery + (cornery[i]-centery)*ftemp;
	 }
   /* if any corner has x or y past limit, move the image */
   adjx = adjy = 0;
   for (i=0; i<4; ++i) {
      if (cornerx[i] > limit	 && (ftemp = cornerx[i] - limit) > adjx)
	 adjx = ftemp;
      if (cornerx[i] < 0.0-limit && (ftemp = cornerx[i] + limit) < adjx)
	 adjx = ftemp;
      if (cornery[i] > limit	 && (ftemp = cornery[i] - limit) > adjy)
	 adjy = ftemp;
      if (cornery[i] < 0.0-limit && (ftemp = cornery[i] + limit) < adjy)
	 adjy = ftemp;
      }
   if (calc_status == 2 && (adjx != 0 || adjy != 0))
      calc_status = 0;
   xxmin = cornerx[0] - adjx;
   xxmax = cornerx[1] - adjx;
   xx3rd = cornerx[2] - adjx;
   yymax = cornery[0] - adjy;
   yymin = cornery[1] - adjy;
   yy3rd = cornery[2] - adjy;
   adjust_corner(); /* make 3rd corner exact if very near other co-ords */
}

static void smallest_add(double *num)
{
   *num += *num * 5.0e-16;
}

static int ratio_bad(double actual, double desired)
{  double ftemp;
   if (desired != 0)
      if ((ftemp = actual / desired) < 0.9 || ftemp > 1.1)
	 return(1);
   return(0);
}

/* read keystrokes while = specified key, return 1+count;	*/
/* used to catch up when moving zoombox is slower than keyboard */
int key_count(int keynum)
{  int ctr;
   ctr = 1;
   while (keypressed() == keynum) {
      getakey();
      ++ctr;
      }
   return ctr;
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{  int vertical, horizontal, getmore;
   if (boxcount == 0)
      return;
   vertical = horizontal = 0;
   getmore = 1;
   while (getmore) {
      switch (keynum) {
	 case 1075:			/* cursor left */
	    --horizontal;
	    break;
	 case 1077:			/* cursor right */
	    ++horizontal;
	    break;
	 case 1072:			/* cursor up */
	    --vertical;
	    break;
	 case 1080:			/* cursor down */
	    ++vertical;
	    break;
	 case 1115:			/* Ctrl-cursor left */
	    horizontal -= 5;
	    break;
	 case 1116:			 /* Ctrl-cursor right */
	    horizontal += 5;
	    break;
	 case 1141:			/* Ctrl-cursor up */
	    vertical -= 5;
	    break;
	 case 1145:			/* Ctrl-cursor down */
	    vertical += 5;
	    break;
	 default:
	    getmore = 0;
	 }
      if (getmore) {
	 if (getmore == 2)		/* eat last key used */
	    getakey();
	 getmore = 2;
	 keynum = keypressed(); 	/* next pending key */
	 }
      }
   if (horizontal!=0)
      moveboxf((double)horizontal/dxsize,0.0);
   if (vertical!=0)
      moveboxf(0.0,(double)vertical*0.75/dysize);
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
