/*
	FRACTINT - The Ultimate Fractal Generator
			Main Routine
*/

/* routines in this module	*/

void main(int argc, char *argv[]);
int  key_count(int);
int  cmp_line(),pot_line();

static void saveanimage(void);
static void clear_zoombox();
static void move_zoombox(int);
static int  call_line3d();

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dos.h>

#ifndef __TURBOC__
#include <malloc.h>
#endif

#define PUTTHEMHERE 1		/* stuff common external arrays here */

#include "fractint.h"
#include "fractype.h"

int	adapter;		/* Video Adapter chosen from list in ...h */

extern char gifmask[];

extern int pot16bit;		/* save 16 bit values for continuous potential */
extern int disk16bit;		/* disk video up & running with 16 bit values */
extern int video_type;		/* coded value indicating video adapter type */
extern int tgaview();
extern int gifview();
extern void moveboxf(double,double);
extern void chgboxi(int,int);
extern int biomorph;
extern int forcesymmetry;
extern	char	readname[];	/* name of fractal input file */
extern	int	showfile;	/* zero if display of file is pending */

#define MAXHISTORY	25	/* save this many historical rcds */
struct historystruct {		/* history structure */
	int fractype;		/* fractal type */
	double param[2];	/* parameters */
	double xxmin;		/* top left	*/
	double yymax;		/* top left	*/
	double xxmax;		/* bottom right */
	double yymin;		/* bottom right */
	double xx3rd;		/* bottom left	*/
	double yy3rd;		/* bottom left	*/
	} far *history;

#ifdef __TURBOC__

/* yes, I *know* it's supposed to be compatible with Microsoft C,
   but some of the routines need to know if the "C" code
   has been compiled with Turbo-C.  This flag is a 1 if FRACTINT.C
   (and presumably the other routines as well) has been compiled
   with Turbo-C. */
int compiled_by_turboc = 1;

/* set size to be used for overlays, a bit bigger than largest (help) */
unsigned _ovrbuffer = 50 * 64; /* that's 50k for overlays, counted in paragraphs */

#else

int compiled_by_turboc = 0;

#endif

extern char savename[]; 	/* save files using this name */
extern char preview;		/* 3D preview mode flag */
extern char temp1[];		/* temporary strings	    */
extern int  debugflag;		/* internal use only - you didn't see this */

/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
	int	active_system = 0;	/* 0 for DOS, WINFRAC for Windows */
	int	dotmode;		/* video access method	    */
	int	textsafe2;		/* textsafe override from videotable */
	int	oktoprint;		/* 0 if printf() won't work */
	int	sxdots,sydots;		/* # of dots on the physical screen    */
	int	sxoffs,syoffs;		/* physical top left of logical screen */
	int	xdots, ydots;		/* # of dots on the logical screen     */
	double	dxsize, dysize; 	/* xdots-1, ydots-1	    */
	int	colors; 		/* maximum colors available */
	int	maxit;			/* try this many iterations */
	int	boxcount;		/* 0 if no zoom-box yet     */
	int	zrotate;		/* zoombox rotation	    */
	double	zbx,zby;		/* topleft of zoombox	    */
	double	zwidth,zdepth,zskew;	/* zoombox size & shape     */

	int	fractype;		/* if == 0, use Mandelbrot  */
	char	stdcalcmode;		/* '1', '2', 'g', 'b'       */
	long	creal, cimag;		/* real, imag'ry parts of C */
	long	delx, dely;		/* screen pixel increments  */
	long	delx2, dely2;		/* screen pixel increments  */
	double	delxx, delyy;		/* screen pixel increments  */
	double	delxx2, delyy2; 	/* screen pixel increments  */
	long	delmin; 		/* for calcfrac/calcmand    */
	double	ddelmin;		/* same as a double	    */
	double	param[4];		/* up to four parameters    */
	double	potparam[3];		/* three potential parameters*/
	long	fudge;			/* 2**fudgefactor	*/
	int	bitshift;		/* fudgefactor		*/

	int	hasconfig;		/* = 0 if 'fractint.cfg'    */
	int	diskisactive;		/* disk-video drivers flag  */
	int	diskvideo;		/* disk-video access flag   */

	/* note that integer grid is set when integerfractal && !invert;    */
	/* otherwise the floating point grid is set; never both at once     */
	long	far *lx0, far *ly0;	/* x, y grid		    */
	long	far *lx1, far *ly1;	/* adjustment for rotate    */
	/* note that lx1 & ly1 values can overflow into sign bit; since     */
	/* they're used only to add to lx0/ly0, 2s comp straightens it out  */
	double far *dx0, far *dy0;	/* floating pt equivs */
	double far *dx1, far *dy1;
	int	integerfractal; 	/* TRUE if fractal uses integer math */

	/* usr_xxx is what the user wants, vs what we may be forced to do */
	char	usr_stdcalcmode;
	int	usr_periodicitycheck;
	int	usr_distest;
	char	usr_floatflag;

	int	viewwindow;		/* 0 for full screen, 1 for window */
	float	viewreduction;		/* window auto-sizing */
	int	viewcrop;		/* nonzero to crop default coords */
	float	finalaspectratio;	/* for view shape and rotation */
	int	viewxdots,viewydots;	/* explicit view sizing */

extern	int	inside; 		/* inside color: 1=blue     */
extern	int	outside;		/* outside color, if set    */
extern	int	cyclelimit;		/* color-rotator upper limit */
extern	int	display3d;		/* 3D display flag: 0 = OFF */
extern	int	overlay3d;		/* 3D overlay flag: 0 = OFF */
extern	int	boxcolor;		/* zoom box color */
extern	int	color_bright;		/* set by find_special_colors */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern unsigned char olddacbox[256][3]; /* backup copy of the Video-DAC */
extern unsigned char far *mapdacbox;	/* default dacbox when map= specified */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern char	loadPalette;
extern int	extraseg;		/* used by Save-to-DISK routines */
extern int	cpu;			/* cpu type			*/
extern int	fpu;			/* fpu type			*/
extern int	lookatmouse;		/* used to select mouse mode	*/
extern int	out_line();		/* called in decoder */
extern int	outlin16();		/* called in decoder */
extern int	line3d();		/* called in decoder */
extern int	(*outln)();		/* called in decoder */
extern int	filetype;		/* GIF or other */

/* variables defined by the command line/files processor */
extern	double	inversion[];
extern	int	invert; 		/* non-zero if inversion active */
extern	int	initbatch;		/* 1 if batch run (no kbd)  */
extern	int	initmode;		/* initial video mode	    */
extern	int	goodmode;		/* video.asm sets nonzero if mode ok */
extern	int	initcyclelimit; 	/* initial cycle limit	    */
extern	int	LogFlag;		/* non-zero if logarithmic palettes */
extern	int	reallyega;		/* == 0 if it's really an EGA */

extern char showbox;		/* flag to show box and vector in preview */
extern unsigned initsavetime;	/* timed save interval */

int	comparegif=0;			/* compare two gif files flag */
int	timedsave=0;			/* when doing a timed save */
extern long saveticks, savebase;	/* timed save vars for general.asm */
extern long readticker();		/* read bios ticker */
extern int  finishrow;			/* for general.asm timed save */
int	resave_flag=0;			/* tells encoder not to incr filename */
int	save_release,save_system;	/* from and for save files */
int	tabmode = 1;			/* tab display enabled */
extern	int release;			/* real current Fractint release */
extern	int got_status;
extern	int loaded3d;

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




void main(int argc, char *argv[])
{
   double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
   double  jxx3rd, jyy3rd;
   int	   frommandel;			/* if julia entered from mandel */
   int	   axmode, bxmode, cxmode, dxmode; /* video mode (BIOS ##)	*/
   int	   historyptr;			/* pointer into history tbl	*/
   int	   historyflag; 		/* are we backing off in history? */
   int	   zoomoff;			/* = 0 when zoom is disabled	*/
   int	   kbdchar;			/* keyboard key-hit value	*/
   int	   more, kbdmore;		/* continuation variables	*/
   int	   i, j, k, l;			/* temporary loop counters	*/
   double  ftemp;			/* fp temp			*/
   int	   savedac;			/* save-the-Video DAC flag	*/
   int	   olddotmode;			/* temp copy of dotmode 	*/

   initasmvars();			/* initialize ASM stuff */

   hasconfig = readconfig();		/* read fractint.cfg, if any */

   maxvideomode = initvideotable();	/* get the size of video table */
   if (maxvideomode >= MAXVIDEOMODES)	/* that's all the Fn keys we got! */
      maxvideomode = MAXVIDEOMODES;

   if (extraseg == 0		/* oops.  not enough memory */
     || (history = (struct historystruct far * ) farmemalloc((unsigned long)
		      (MAXHISTORY * sizeof(*history)))) == NULL
     || (ly0 = (long far *) farmemalloc(2000L)) == NULL) {
      buzzer(2);
      printf(" I'm sorry, but you don't have enough free memory \n");
      printf(" to run this program.\n\n");
      exit(1);
      }
   farmemfree((unsigned char far *)ly0); /* was just to check for min space */

#ifdef __TURBOC__
   dx0 = MK_FP(extraseg,0);
#else
   FP_SEG(dx0)=extraseg;
   FP_OFF(dx0)=0;
#endif
   dy1 = (dx1 = (dy0 = dx0 + MAXPIXELS) + MAXPIXELS) + MAXPIXELS;
   lx0 = (long far *) dx0;
   ly1 = (lx1 = (ly0 = lx0 + MAXPIXELS) + MAXPIXELS) + MAXPIXELS;

   historyptr = 0;			/* initialize history ptr */
   historyflag = 0;
   history[historyptr].fractype = -1;

restart:				/* insert key re-starts here */

   cmdfiles(argc,argv); 		/* process the command-line */

   if (debugflag == 8088)		 cpu =	86; /* for testing purposes */
   if (debugflag == 2870 && fpu >= 287 ) fpu = 287; /* for testing purposes */
   if (debugflag ==  870 && fpu >=  87 ) fpu =	87; /* for testing purposes */
   if (debugflag ==   70 && fpu >=   0 ) fpu =	 0; /* for testing purposes */

   adapter_detect();			/* check what video is really present */
   if (debugflag >= 9002 && debugflag <= 9100) /* for testing purposes */
      if (video_type > (debugflag-9000)/2)     /* adjust the video value */
	 video_type = (debugflag-9000)/2;

   diskisactive = 0;			/* disk-video is inactive */
   diskvideo = 0;			/* disk driver is not in use */
   setvideomode(3,0,0,0);		/* switch to text mode */
   calc_status = -1;			/* no active fractal image */
   savedac = 0; 			/* don't save the VGA DAC */

   if (debugflag == 10000)		/* check for free memory */
      showfreemem();

   helpmode = HELPAUTHORS;		/* use this help mode */
   if (showfile && initmode < 0)
      help();				/* display the credits screen */

restorestart:

   lookatmouse = 0;			/* ignore mouse */

   while (showfile <= 0) {		/* image is to be loaded */
      char *hdg;
      helpmode = -1;
      tabmode = 0;
      if (overlay3d)
	 hdg = "Select File for 3D Overlay";
      else if (display3d)
	 hdg = "Select File for 3D Transform";
      else
	 hdg = "Select File to Restore";
      if (showfile < 0 && getafilename(hdg,gifmask,readname) < 0) {
	 showfile = 1;		     /* cancelled */
	 initmode = -1;
	 break;
	 }
      showfile = 0;
      tabmode = 1;
      if (read_overlay() == 0)	     /* read hdr, get video mode */
	 break; 		     /* got it, exit */
      showfile = -1;		     /* retry */
      }

   helpmode = HELPMENU; 		/* now use this help mode */
   tabmode = 1;
   lookatmouse = 0;			/* ignore mouse */

   if (overlay3d && initmode < 0) {	/* overlay command failed */
      unstackscreen();			/* restore the graphics screen */
      overlay3d = 0;			/* forget overlays */
      display3d = 0;			/* forget 3D */
      if (calc_status > 0)
	 calc_status = 0;
      goto resumeloop;			/* ooh, this is ugly */
      }

imagestart:				/* calc/display a new image */

   savedac = 0; 			/* don't save the VGA DAC */
   got_status = -1;			/* for tab_display */

   if (showfile)
      if (calc_status > 0)		/* goto imagestart implies re-calc */
	 calc_status = 0;

   if (initbatch == 0)
      lookatmouse = -1073;		/* just mouse left button, == pgup */

   cyclelimit = initcyclelimit; 	/* default cycle limit	 */

   frommandel = 0;

   adapter = initmode;			/* set the video adapter up */
   initmode = -1;			/* (once)		    */

   while (adapter < 0) {		/* cycle through instructions */
      if (initbatch)				/* batch, nothing to do */
	 goodbye();
      kbdchar = main_menu(0);
      adapter = -1;				/* no video adapter yet */
      for (k = 0; k < maxvideomode; k++)	/* search for an adapter */
	 if (kbdchar == kbdkeys[k])
	    adapter = k; /* use this adapter */
      if (adapter >= 0) break;			/* bail out if we found one */
      if (kbdchar == 1082) goto restart;	/* restart pgm on Insert Key */
  /*  if (kbdchar == 1000) goodbye();  */	/* Control-C */
      if (kbdchar == 27) goodbye();		/* exit to DOS on Escape Key */
      if ('A' <= kbdchar && kbdchar <= 'Z')
	 kbdchar = tolower(kbdchar);
      if (kbdchar == 'd') {                     /* shell to DOS */
	 setclear();
	 printf("\n\nShelling to DOS - type 'exit' to return\n\n");
	 shell_to_dos();
	 goto imagestart;
	 }
 /*   if ( kbdchar == '1' || kbdchar == '2' || kbdchar == 'g') {
	 usr_stdcalcmode = kbdchar;
	 continue;
	 }
  */
      if (kbdchar == 'r' || kbdchar == '3' || kbdchar == 'o') {
	 display3d = 0;
	 if (kbdchar == '3' || kbdchar == 'o')
	    display3d = 1;
	 setvideomode(3,0,0,0); /* switch to text mode */
	 showfile = -1;
	 goto restorestart;
	 }
      if (kbdchar == 't') {                     /* set fractal type */
	 get_fracttype();
	 goto imagestart;
	 }
      if (kbdchar == 'x') {                     /* generic toggle switch */
	 get_toggles();
	 goto imagestart;
	 }
      if (kbdchar == 'y') {                     /* generic toggle switch */
	 get_toggles2();
	 goto imagestart;
	 }
      if (kbdchar == 'v') {                     /* view parameters */
	 get_view_params();
	 goto imagestart;
	 }
      if (kbdchar == 'f') {                     /* floating pt toggle */
	 if (usr_floatflag == 0)
	    usr_floatflag = 1;
	 else
	    usr_floatflag = 0;
	 goto imagestart;
	 }
      if (kbdchar == 'i') {                     /* set IFS fractal parms */
	 get_ifs_params(); /* get the parameters */
	 goto imagestart;
	 }
      buzzer(2);				/* unrecognized key */
      }

   zoomoff = 1; 		/* zooming is enabled */
   helpmode = HELPMAIN; 	/* now use this help mode */

   more = 1;
   while (more) {		/* eternal loop */

      if (calc_status != 2 || showfile == 0) {
	 fromvideotable(adapter);	   /* collect adapter info     */
	 axmode  = videoentry.videomodeax; /* video mode (BIOS call)   */
	 bxmode  = videoentry.videomodebx; /* video mode (BIOS call)   */
	 cxmode  = videoentry.videomodecx; /* video mode (BIOS call)   */
	 dxmode  = videoentry.videomodedx; /* video mode (BIOS call)   */
	 dotmode = videoentry.dotmode;	   /* assembler dot read/write */
	 xdots	 = videoentry.xdots;	   /* # dots across the screen */
	 ydots	 = videoentry.ydots;	   /* # dots down the screen   */
	 colors  = videoentry.colors;	   /* # colors available */
	 textsafe2 = dotmode / 100;
	 dotmode  %= 100;
	 sxdots  = xdots;
	 sydots  = ydots;
	 sxoffs = syoffs = 0;

	 diskvideo = 0; 		/* set diskvideo flag */
	 if (dotmode == 11)		/* default assumption is disk */
	    diskvideo = 2;

	 memcpy(olddacbox,dacbox,256*3); /* save the DAC */
	 diskisactive = 1;		/* flag for disk-video routines */
	 if (overlay3d) {
	    unstackscreen();		/* restore old graphics image */
	    overlay3d = 0;
	    }
	 else {
	    setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */
	    if (goodmode == 0) {
	       stopmsg(0,"That video mode is not available with your adapter.");
	       initmode = -1;
	       setvideomode(3,0,0,0); /* switch to text mode */
	       goto restorestart;
	       }
	    }
	 diskisactive = 0;		/* flag for disk-video routines */
	 if (savedac) {
	    memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
	    if (dotmode != 11)
	       spindac(0,1);
	    }
	 else /* reset DAC to defaults, which setvideomode has done for us */
	    if (loadPalette) { /* but there's a map=, so load that */
	       char far *saveptr;
	       char *dacptr;
	       int i;
	       saveptr = mapdacbox;
	       dacptr = &dacbox[0][0];
	       for (i = 0; i < 768; ++i)
		  *(dacptr++) = *(saveptr++);
	       if (dotmode != 11)
		  spindac(0,1);
	       }
	    else if (dotmode == 11 && colors == 256) {
	       /* disk video, setvideomode via bios didn't get it right, so: */
	       ValidateLuts("default"); /* read the default palette file */
	       }
	 if (viewwindow) {
	    ftemp = finalaspectratio
		    * (double)sydots / (double)sxdots / SCREENASPECT;
	    if ((xdots = viewxdots)) { /* xdots specified */
	       if ((ydots = viewydots) == 0) /* calc ydots? */
		  ydots = (double)xdots * ftemp + 0.5;
	       }
	    else
	       if (finalaspectratio <= SCREENASPECT) {
		  xdots = (double)sxdots / viewreduction + 0.5;
		  ydots = (double)xdots * ftemp + 0.5;
		  }
	       else {
		  ydots = (double)sydots / viewreduction + 0.5;
		  xdots = (double)ydots / ftemp + 0.5;
		  }
	    if (xdots > sxdots || ydots > sydots) {
	       stopmsg(0,"View window too large; using full screen.");
	       xdots = sxdots;
	       ydots = sydots;
	       }
	    else if (xdots <= sxdots/20 || ydots <= sydots/20) { /* so ssg works */
	       stopmsg(0,"View window too small; using full screen.");
	       xdots = sxdots;
	       ydots = sydots;
	       }
	    sxoffs = (sxdots - xdots) / 2;
	    syoffs = (sydots - ydots) / 3;
	    }
	 dxsize = xdots - 1;		/* convert just once now */
	 dysize = ydots - 1;
	 }

      savedac = 1;			/* assume we save next time */

      if (initbatch == 0)
	 lookatmouse = -1073;		/* mouse left button == pgup */

      if(showfile == 0) {		/* loading an image */
	 if (display3d) 		/* set up 3D decoding */
	    outln = call_line3d;
	 else if(filetype >= 1) 	/* old .tga format input file */
	    outln = outlin16;
	 else if(comparegif)		/* debug 50 */
	    outln = cmp_line;
	 else if(pot16bit) {		/* .pot format input file */
	    pot_startdisk();
	    outln = pot_line;
	    }
	 else				/* regular gif/fra input file */
	    outln = out_line;
	 if(filetype == 0)
	    i = funny_glasses_call(gifview);
	 else
	    i = funny_glasses_call(tgaview);
	 if(i == 0)
	    buzzer(0);
	 else {
	    calc_status = -1;
	    if (keypressed()) {
	       buzzer(1);
	       while (keypressed()) getakey();
	       texttempmsg("*** load incomplete ***");
	       }
	    }
	 }

      zoomoff = 1;			/* zooming is enabled */
      if (dotmode == 11 || (fractalspecific[fractype].flags&NOZOOM) != 0)
	 zoomoff = 0;			/* for these cases disable zooming */

      calcfracinit();

      sxmin = xxmin; /* save 3 corners for zoom.c ref points */
      sxmax = xxmax;
      sx3rd = xx3rd;
      symin = yymin;
      symax = yymax;
      sy3rd = yy3rd;

      if (history[0].fractype == -1)	/* initialize the history file */
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

      if ( history[historyptr].xxmax != xxmax  || /* save any (new) zoom data */
	   history[historyptr].xxmin != xxmin  ||
	   history[historyptr].yymax != yymax  ||
	   history[historyptr].yymin != yymin  ||
	   history[historyptr].xx3rd != xx3rd  ||
	   history[historyptr].yy3rd != yy3rd  ||
	   history[historyptr].param[0] != param[0] ||
	   history[historyptr].param[1] != param[1] ||
	   history[historyptr].fractype != fractype) {
	 if (historyflag == 0) /* if we're not backing off for <\> */
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
      historyflag = 0;

      if (display3d || showfile) {	/* paranoia: these vars don't get set */
	 save_system  = active_system;	/*   unless really doing some work,   */
	 save_release = release;	/*   so simple <r> + <s> keeps number */
	 }

      if(showfile == 0) {		/* image has been loaded */
	 showfile = 1;
	 if (initbatch == 1 && calc_status == 2)
	    initbatch = -1; /* flag to finish calc before save */
	 if (loaded3d)	    /* 'r' of image created with '3' */
	    display3d = 1;  /* so set flag for 'b' command */
	 }
      else {				/* draw an image */
	 diskisactive = 1;		/* flag for disk-video routines */
	 if (initsavetime != 0		/* autosave and resumable? */
	   && (fractalspecific[fractype].flags&NORESUME) == 0) {
	    savebase = readticker(); /* calc's start time */
	    saveticks = (long)initsavetime * 1092; /* bios ticks/minute */
	    if ((saveticks & 65535) == 0)
	       ++saveticks; /* make low word nonzero */
	    finishrow = -1;
	    }
	 if ((i = calcfract()) == 0)	/* draw the fractal using "C" */
	    buzzer(0); /* finished!! */
	 saveticks = 0; 		/* turn off autosave timer */
	 if (dotmode == 9) { /* TARGA */
	    EndTGA(); /* make sure TARGA is OFF */
	    if (i == 0)
	       printf("Image has been completed");
	    }
	 if (dotmode == 11 && i == 0) /* disk-video */
	    dvid_status(0,"Image has been completed");
	 diskisactive = 0;		/* flag for disk-video routines */
	 }

      boxcount = 0;			/* no zoom box yet  */
      zwidth = 0;

      if (fractype == PLASMA && cpu > 88) {
	 cyclelimit = 256;		/* plasma clouds need quick spins */
	 daccount = 256;
	 daclearn = 1;
	 }

resumeloop:				/* return here on failed overlays */

      kbdmore = 1;
      while (kbdmore == 1) {		/* loop through command keys */

	 if (timedsave != 0) {
	    if (timedsave == 1) {	/* woke up for timed save */
	       getakey();     /* eat the dummy char */
	       kbdchar = 's'; /* do the save */
	       resave_flag = 1;
	       timedsave = 2;
	       }
	    else {			/* save done, resume */
	       timedsave = 0;
	       resave_flag = 2;
	       kbdchar = 13;
	       }
	    }
	 else if (initbatch == 0) {	/* not batch mode */
	    lookatmouse = (zwidth == 0) ? -1073 : 3;
	    if (calc_status == 2 && zwidth == 0 && !keypressed())
	       kbdchar = 13;		/* no visible reason to stop, continue */
	    else {			/* wait for a real keystroke */
	       while (!keypressed()) { }  /* enables help */
	       kbdchar = getakey();
	       if (kbdchar == 27 || kbdchar == 'm' || kbdchar == 'M') {
		  stackscreen();
		  kbdchar = main_menu(1);
		  unstackscreen();
		  }
	       }
	    }
	 else { 			/* batch mode, fake next keystroke */
	    if (initbatch == -1) {	/* finish calc */
	       kbdchar = 13;
	       initbatch = 1;
	       }
	    else if (initbatch == 1) {	/* save-to-disk */
	       if (debugflag == 50)
		  kbdchar = 'r';
	       else
		  kbdchar = 's';
	       initbatch = 2;
	       }
	    else
	       kbdchar = 27;		/* done, exit */
	    }

	 if ('A' <= kbdchar && kbdchar <= 'Z')
	    kbdchar = tolower(kbdchar);

	 switch (kbdchar) {
	    case 't':                   /* new fractal type             */
	       clear_zoombox();
	       stackscreen();
	       if ((i = get_fracttype()) >= 0) {
		  discardscreen();
		  if (i == 0) {
		     initmode = adapter;
		     frommandel = 0;
		     }
		  goto imagestart;
		  }
	       unstackscreen();
	       break;
	    case 'x':                   /* invoke options screen        */
	    case 'y':
	       stackscreen();
	       if (kbdchar == 'x')
		  i = get_toggles();
	       else
		  i = get_toggles2();
	       if (i > 0) {		/* time to redraw? */
		  discardscreen();
		  kbdmore = calc_status = 0;
		  if (i == 2) { 	   /* float change? */
		     initmode = adapter;
		     goto imagestart;
		     }
		  }
	       else
		  unstackscreen();
	       break;
	    case 'v':                   /* invoke options screen        */
	       i = get_view_params();	/* get the parameters */
	       if (i > 0)		/* time to redraw? */
		  kbdmore = calc_status = 0;
	       break;
	    case 'f':                   /* floating pt toggle           */
	       if (usr_floatflag == 0)
		  usr_floatflag = 1;
	       else
		  usr_floatflag = 0;
	       initmode = adapter;
	       goto imagestart;
	    case 'i':                   /* new IFS parms                */
	       if (get_ifs_params() >= 0)    /* get the parameters */
		  calc_status = kbdmore = 0; /* time to redraw */
	       break;
	    case 'a':                  /* starfield parms               */
	       clear_zoombox();
	       if (get_starfield_params() >= 0) {
		  calc_status = 0;
		  continue;
		  }
	       break;
	    case 32:		       /* spacebar, toggle mand/julia	*/
	       if (fractalspecific[fractype].tojulia != NOFRACTAL
		 && param[0] == 0.0 && param[1] == 0.0) {
		  /* switch to corresponding Julia set */
		  fractype = fractalspecific[fractype].tojulia;
		  param[0] = (xxmax + xxmin) / 2;
		  param[1] = (yymax + yymin) / 2;
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
		     double ccreal,ccimag;
		     ccreal = (fractalspecific[fractype].xmax - fractalspecific[fractype].xmin) / 2;
		     ccimag = (fractalspecific[fractype].ymax - fractalspecific[fractype].ymin) / 2;
		     xxmin = xx3rd = param[0] - ccreal;
		     xxmax = param[0] + ccreal;
		     yymin = yy3rd = param[1] - ccimag;
		     yymax = param[1] + ccimag;
		     }
		  param[0] = 0;
		  param[1] = 0;
		  zoomoff = 1;
		  calc_status = 0;
		  kbdmore = 0;
		  }
	       else
		  buzzer(2); /* can't switch */
	       break;
	    case '\\':                 /* return to prev image    */
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
	       zoomoff = 1;
	       initmode = adapter;
	       if (fractalspecific[fractype].isinteger != 0 &&
		   fractalspecific[fractype].tofloat != NOFRACTAL)
		  usr_floatflag = 0;
	       if (fractalspecific[fractype].isinteger == 0 &&
		   fractalspecific[fractype].tofloat != NOFRACTAL)
		  usr_floatflag = 1;
	       historyflag = 1; /* avoid re-store parms due to rounding errs */
	       goto imagestart;
	    case 'd':                   /* shell to MS-DOS              */
	       stackscreen();
	       if (axmode == 0 || axmode > 7) {
static char far dosmsg[]={"\
Note:  Your graphics image is still squirreled away in your video\n\
adapter's memory.  Switching video modes (say, to get your cursor back)\n\
will clobber part of that image.  Sorry - it's the best we could do."};
		  putstring(0,0,7,dosmsg);
		  movecursor(6,0);
		  }
	       shell_to_dos();
	       unstackscreen();
	       calc_status = 0;
	       break;
      /*    case '<':
	    case ',':
	       if (maxit >= 10+initincr) maxit -= initincr;
	       maxit -= initincr;
	    case '>':
	    case '.':
	       if (maxit <= 32000-initincr) maxit += initincr;
	       calc_status = 0;
	       continue;
       */
	    case 'c':                   /* switch to color cycling      */
	       clear_zoombox();
	       rotate(0);
	       continue;
	    case '+':                   /* rotate palette               */
	       clear_zoombox();
	       rotate(+1);
	       continue;
	    case '-':                   /* rotate palette               */
	       clear_zoombox();
	       rotate(-1);
	       continue;
	    case 'e':                   /* switch to color editing      */
	       clear_zoombox();
	       if (dacbox[0][0] != 255 && !reallyega && colors > 16) {
		  int oldhelpmode;
		  oldhelpmode = helpmode;
		  helpmode = HELPXHAIR;
		  EditPalette();
		  helpmode = oldhelpmode;
		  }
	       continue;
	    case 's':                   /* save-to-disk                 */
	       if (resave_flag == 2)
		  resave_flag = 0;
	       saveanimage();
	       continue;
	    case 'o':                   /* 3D overlay                   */
	       clear_zoombox();
	       overlay3d = 1;
	    case '3':                   /* restore-from (3d)            */
	       display3d = 1;
	    case 'r':                   /* restore-from                 */
	       comparegif = 0;
	       frommandel = 0;
	       if(kbdchar == 'r')  {
		  if(debugflag == 50) {
		     comparegif = overlay3d = 1;
		     if (initbatch == 2) {
			stackscreen(); /* save graphics image */
			strcpy(readname,savename);
			showfile = 0;
			goto restorestart;
			}
		     }
		  else
		     comparegif = overlay3d = 0;
		  display3d = 0;
		  }
	       olddotmode = dotmode; /* save the old dotmode */
	       if (overlay3d)
		   stackscreen(); /* save graphics image */
	       else {
		   if (dotmode == 12) /* 8514/A mode */
		      dotmode = 1;
		   setvideomode(3,0,0,0); /* switch to text mode */
		   }
	       dotmode = olddotmode;
	       if (resave_flag) {
		  updatesavename(savename); /* do the pending increment */
		  resave_flag = 0;
		  }
	       showfile = -1;
	       goto restorestart;
      /*    case '1':
	    case '2':
	    case 'g':
	       usr_stdcalcmode = kbdchar;
	       kbdmore = calc_status = 0;
	       break;
       */
	    case 'b':                   /* make batch file              */
	       make_batch_file();
	       break;
	    case 'p':                   /* print current image          */
	       clear_zoombox();
	       Print_Screen();
	       if (!keypressed())
		  buzzer(0);
	       else {
		  buzzer(1);
		  getakey();
		  }
	       continue;
	    case 13:			/* Enter			*/
	    case 1013:			/* Numeric-Keypad Enter 	*/
	       if (zwidth != 0.0) {	/* do a zoom */
		  init_pan_or_recalc(0);
		  kbdmore = 0;
		  }
	       if (calc_status != 4)	/* don't restart if image complete */
		  kbdmore = 0;
	       break;
	    case 10:			/* control-Enter		*/
	    case 1010:			/* Control-Keypad Enter 	*/
	       init_pan_or_recalc(1);
	       kbdmore = 0;
	       zoomout(); /* calc corners for zooming out */
	       break;
	    case 1082:			/* insert			*/
	       dotmode = 1;
	       setvideomode(3,0,0,0); /* force text mode */
	       goto restart;
	    case 27:			/* Escape			*/
	       more = kbdmore = 0;
	       break;
	    case 1075:			/* cursor left			*/
	    case 1077:			/* cursor right 		*/
	    case 1072:			/* cursor up			*/
	    case 1080:			/* cursor down			*/
	    case 1115:			/* Ctrl-cursor left		*/
	    case 1116:			/* Ctrl-cursor right		*/
	    case 1141:			/* Ctrl-cursor up		*/
	    case 1145:			/* Ctrl-cursor down		*/
	       move_zoombox(kbdchar);
	       break;
	    case 1119:			/* Ctrl-home			*/
	       if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0) {
		  i = key_count(1119);
		  if ((zskew -= 0.02*i) < -0.48)
		     zskew = -0.48;
		  }
	       break;
	    case 1117:			/* Ctrl-end			*/
	       if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0) {
		  i = key_count(1117);
		  if ((zskew += 0.02*i) > 0.48)
		     zskew = 0.48;
		  }
	       break;
	    case 1132:			/* Ctrl-pgup			*/
	       if (boxcount)
		  chgboxi(0,-2*key_count(1132));
	       break;
	    case 1118:			/* Ctrl-pgdn			*/
	       if (boxcount)
		  chgboxi(0,2*key_count(1118));
	       break;
	    case 1073:			/* page up			*/
	       if (zoomoff == 1)
		  if (zwidth == 0) { /* start zoombox */
		     zwidth = zdepth = 1;
		     zrotate = zskew = 0;
		     zbx = zby = 0;
		     find_special_colors();
		     boxcolor = color_bright;
		     }
		  else
		     resizebox(0-key_count(1073));
	       break;
	    case 1081:			/* page down			*/
	       if (boxcount) {
		  if (zwidth >= .999 && zdepth >= 0.999) /* end zoombox */
		     zwidth = 0;
		  else
		     resizebox(key_count(1081));
		  }
	       break;
	    case 1142:			 /* Ctrl-kpad-			*/
	       if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0)
		  zrotate += key_count(1142);
	       break;
	    case 1144:			    /* Ctrl-kpad+		*/
	       if (boxcount && (fractalspecific[fractype].flags&NOROTATE) == 0)
		  zrotate -= key_count(1144);
	       break;
	    case 1146:			    /* Ctrl-ins 		*/
	       boxcolor += key_count(1146);
	       break;
	    case 1147:			    /* Ctrl-del 		*/
	       boxcolor -= key_count(1147);
	       break;
	    default:			   /* other (maybe a valid Fn key) */
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
	    } /* end of the big switch */

	 if (zoomoff == 1 && kbdmore == 1) /* draw/clear a zoom box? */
	    drawbox(1);
	 }
      }

   dotmode = 0;
   goodbye();				/* we done. */

}

static void saveanimage(void)
{
   clear_zoombox();
   diskisactive = 1;	   /* flag for disk-video routines */
   savetodisk(savename);
   diskisactive = 0;	   /* flag for disk-video routines */
}

static void clear_zoombox()
{
   zwidth = 0;
   drawbox(0);
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = symax;
   yymin = symin;
   yy3rd = sy3rd;
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
      moveboxf(0.0,(double)vertical/dysize);
}

/* displays differences between current image file and new image */
/* Bert - suggest add this to video.asm */
int cmp_line(unsigned char *pixels, int linelen)
{
   static errcount;
   static FILE *fp = NULL;
   extern int rowcount;
   int row,col;
   int oldcolor;
   char *timestring;
   time_t ltime;
   if(fp == NULL)
      fp = fopen("cmperr",(initbatch)?"a":"w");
   if((row = rowcount++) == 0)
      errcount = 0;
   if(pot16bit) { /* 16 bit info, ignore odd numbered rows */
      if((row & 1) != 0) return(0);
      row >>= 1;
      }
   for(col=0;col<linelen;col++) {
      oldcolor=getcolor(col,row);
      if(oldcolor==pixels[col])
	 putcolor(col,row,0);
      else {
	 if(oldcolor==0)
	    putcolor(col,row,1);
	 ++errcount;
	 if(initbatch == 0)
	    fprintf(fp,"#%5d col %3d row %3d old %3d new %3d\n",
	       errcount,col,row,oldcolor,pixels[col]);
	 }
      }
   if(row+1 == ydots && initbatch) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(fp,"%s compare to %s has %5d errs\n",timestring,readname,errcount);
      }
   return(0);
}

int pot_line(unsigned char *pixels, int linelen)
{
   extern int rowcount;
   int row,col,saverowcount;
   if (rowcount == 0)
      pot_startdisk();
   saverowcount = rowcount;
   row = (rowcount >>= 1);
   if ((saverowcount & 1) != 0) /* odd line */
      row += ydots;
   else 			/* even line */
      if (dotmode != 11) /* display the line too */
	 out_line(pixels,linelen);
   for (col = 0; col < xdots; ++col)
      writedisk(col+sxoffs,row+syoffs,*(pixels+col));
   rowcount = saverowcount + 1;
   return(0);
}

static int call_line3d(unsigned char *pixels, int linelen)
{
   /* this routine exists because line3d might be in an overlay */
   return(line3d(pixels,linelen));
}

