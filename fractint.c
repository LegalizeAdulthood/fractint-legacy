
/*
	FRACTINT - The Ultimate Fractal Generator
			Main Routine
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef __TURBOC__
#include <dos.h>
#include <malloc.h>
#endif

#define PUTTHEMHERE 1			/* stuff common external arrays here */

#include "fractint.h"

int	adapter;		/* Video Adapter chosen from list in ...h */

char *accessmethod[] =
	{" ","B"," "," "," ",
	 " "," "," "," "," ",
	 " "," "," "," "," "}; /* BIOS access indicator */

struct fractal_info save_info, read_info; /*  for saving data in file */

extern	char    readname[81];     /* name of fractal input file */
extern	int     showfile;        /* has file been displayed yet? */

#define	FUDGEFACTOR	29		/* fudge all values up by 2**this */
#define FUDGEFACTOR2	24		/* (or maybe this)		  */

#ifndef __TURBOC__
#define MAXHISTORY	50		/* save this many historical rcds */
long	far history[MAXHISTORY][7];	/* for historical (HOME Key) purposes */
#else
#define MAXHISTORY	10		/* save this many historical rcds */
long	history[MAXHISTORY][7];		/* for historical (HOME Key) purposes */
#endif

extern	char	savename[80];		/* save files using this name */

extern	int	debugflag;		/* internal use only - you didn't see this */
/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
	int	dotmode;			/* video access method      */
	int	oktoprint;			/* 0 if printf() won't work */
	int	xdots, ydots;			/* # of dots on the screen  */
	int	colors;				/* maximum colors available */
	int	maxit;				/* try this many iterations */
	int	ixmin, ixmax, iymin, iymax;	/* corners of the zoom box  */
	int	boxcount;			/* 0 if no zoom-box yet     */

	int	fractype;			/* if == 0, use Mandelbrot  */
	int	numpasses;			/* 0 if single-pass, else 1 */
	int	solidguessing;			/* 0 if disabled, else 1    */
	long	creal, cimag;			/* real, imag'ry parts of C */
	long	delx, dely;			/* screen pixel increments */
	double	param[4];			/* up to four parameters    */
	double	potparam[4];		/* four potential parameters TW 6-18-89*/
	long	fudge;				/* 2**fudgefactor	*/
	int	bitshift;			/* fudgefactor		*/

	int	hasconfig;			/* = 0 if 'fractint.cfg'    */
	int	diskisactive;			/* disk-video drivers flag  */
	int	diskvideo;			/* disk-video access flag   */

extern	long	lx0[MAXPIXELS], ly0[MAXPIXELS];	/* x, y grid                */

extern	int	inside;				/* inside color: 1=blue     */
extern	int	cyclelimit;			/* color-rotator upper limit */
extern	int	display3d;			/* 3D display flag: 0 = OFF */
extern	int	overlay3d;			/* 3D overlay flag: 0 = OFF */
extern	int	init3d[20];			/* '3d=nn/nn/nn/...' values */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern unsigned char olddacbox[256][3]; /* backup copy of the Video-DAC */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	extraseg;		/* used by Save-to-DISK routines */
extern int	cpu;			/* cpu type			*/
extern int	fpu;			/* fpu type			*/
extern int	lookatmouse;		/* used to activate non-button mouse movement */
extern int	out_line();		/* called in decoder */
extern	int	outlin16();		/* called in decoder */
extern int	line3d();		/* called in decoder */
extern int	(*outln)();		/* called in decoder */
extern	int	filetype;		/* GIF or other */

/* variables defined by the command line/files processor */

extern	int	initbatch;		/* 1 if batch run (no kbd)  */
extern	int	initmode;		/* initial video mode       */
extern	int	inititer;		/* initial value of maxiter */
extern	int	initincr;		/* initial maxiter incrmnt  */
extern	int	initpass;		/* initial pass mode        */
extern	int	initsolidguessing;	/* initial solid-guessing md*/
extern	int	initfractype;		/* initial type set flag    */
extern	int	initcyclelimit;		/* initial cycle limit      */
extern	int	initcorners;		/* initial flag: corners set*/
extern	double	initxmin,initxmax;	/* initial corner values    */
extern	double	initymin,initymax;	/* initial corner values    */
extern	double	initparam[4];		/* initial parameters       */
extern	double	initpot[4];		/* potential parameters -TW 6/25/89  */


void main(argc,argv)
int argc;
char *argv[];
{
	long	jxmin, jxmax, jymin, jymax;	/* "Julia mode" entry point */
	long	xmin, xmax, ymin, ymax;		/* screen corner values */
	double	atof(), ftemp;			/* floating point stuff    */
	double	ccreal,ccimag;			/* Julia Set Parameters    */
	double	xxmin,xxmax,yymin,yymax;	/* (printf) screen corners */
	int	xstep, ystep;			/* zoom-box increment values */
	int	axmode, bxmode, cxmode, dxmode;	/* video mode (BIOS ##) */
	int	historyptr;			/* pointer into history tbl */
	int	zoomoff;			/* = 0 when zoom is disabled */
	int	kbdchar;			/* keyboard key-hit value */
	int	more, kbdmore;			/* continuation variables */
	int	i, j, k, l;			/* temporary loop counters */

	char	temp1[81];			/* temporary strings        */
	int	displaypage;			/* signon display page      */
	int	savedac;			/* save-the-Video DAC flag  */
	int	olddotmode;			/* temp copy of dotmode	    */
	long	huge *xxxalloc;			/* Quick-C klooge (extraseg) */

initasmvars();					/* initialize ASM stuff */

hasconfig = readconfig();			/* read fractint.cfg, if any */

maxvideomode = initvideotable();		/* get the size of video table */
if ( maxvideomode >= MAXVIDEOMODES)		/* that's all the Fn keys we got! */
	maxvideomode = MAXVIDEOMODES;
	
cmdfiles(argc,argv);				/* process the command-line */

if (debugflag == 8088) cpu = debugflag;	/* for testing purposes */

savedac = 0;				/* don't save the VGA DAC */
diskisactive = 0;			/* disk-video is inactive */
diskvideo = 0;				/* disk driver is not in use */
setvideomode(3,0,0,0);			/* switch to text mode */

restorestart:

if(*readname){  /* This is why readname initialized to null string */
	if (!overlay3d)			/* overlays use the existing mode */
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
 		showfile = 0;
	if (!overlay3d) {		/* don't worry about the video if overlay */
		if (read_info.info_id[0] == 'G') {
			buzzer(2);
			printf("You have selected an unknown (non-fractal) GIF image\n");
			printf("I am treating this image as a PLASMA CLOUD of unknown video type\n");
			}
		if (read_info.dotmode == 11) {
			buzzer(2);
			printf("You have selected a fractal image generated in 'no-video' mode\n");
			}
		if (read_info.info_id[0] == 'G' || read_info.dotmode == 11) {
			printf("with resolution %d x %d x %d\n",
				read_info.xdots, read_info.ydots, read_info.colors);
			}
		if (read_info.info_id[0] != 'G' && read_info.dotmode != 11)
	 		initmode = getGIFmode(&read_info);
		if (initmode >= maxvideomode) initmode = -1;
		if (read_info.info_id[0] != 'G' && read_info.dotmode != 11
			&& initmode >= 0
			&& hasconfig && ! initbatch ){
                        char c;
			fromvideotable(initmode);
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
				accessmethod[videoentry.dotmode],
				videoentry.comment);
                        printf("Legal for this machine? ([Y] or N) --> ");
                	while((c=getch())!='y' && c!='Y' && c!='n' && c!='N' && c!=13);
                	if( c == 'n' || c == 'N')
                		initmode = -1;
			}
		else 	{
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
				*readname = NULL; /* failed ... zap filename */
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
		if (initmode >= 0 && display3d && ! initbatch) {
			printf("\n\n");
		    if(SPHERE)
		       strcpy(temp1,"yes");
		    else
		       strcpy(temp1,"no");
            printf("\n\nSphere Projection? (y or n) (if not %3s) ",temp1);
			gets(temp1);
			if((*temp1 == 'y' || *temp1 == 'Y') && !SPHERE)
		            {
               /* reset sphere defaults */
               SPHERE    = TRUE;    
               PHI1      =  180;    
               PHI2      =  0;   
               THETA1    =  -90;   
               THETA2    =  90;   
               RADIUS    =  100;   
               ROUGH     =  30;   
               WATERLINE = 0;
               FILLTYPE  = 2;
               ZVIEWER   = 0;
               XSHIFT    = 0;
               YSHIFT    = 0;
               XLIGHT    = 0;
               YLIGHT    = 0;
               ZLIGHT    = 1;
               LIGHTAVG  = 1;
			}   
			else if((*temp1 == 'n' || *temp1 == 'N') && SPHERE)
            {
               SPHERE    = FALSE;    
               XROT      = 60;
               YROT      = 30;
               ZROT      = 0;
               XSCALE    = 90;
               YSCALE    = 90;
               ROUGH     = 30;   
               WATERLINE = 0;
               FILLTYPE  = 0;
               ZVIEWER   = 0;
               XSHIFT    = 0;
               YSHIFT    = 0;
               XLIGHT    = 0;
               YLIGHT    = 0;
               ZLIGHT    = 1;
               LIGHTAVG  = 1;
			}   
            if(SPHERE){

            /* BEGIN Sphere-specific parameters */
            printf("\n\nSphere is on its side; North pole to right\n");
            printf("180 degrees longitude is top; 0 degrees bottom\n");
            printf("-90 degrees latitude is left; 90 degrees right\n");
			printf("Longitude start (degrees) (if not %3d) ",PHI1);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) PHI1 = k;
			printf("Longitude stop  (degrees) (if not %3d) ",PHI2);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) PHI2 = k;
			printf("Latitude start  (degrees) (if not %3d) ",THETA1);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) THETA1 = k;
			printf("Latitude stop   (degrees) (if not %3d) ",THETA2);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) THETA2 = k;
			printf("Radius scaling factor in pct  (if not %3d)  ",RADIUS);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) RADIUS = k;
            }else{
            /* BEGIN Regular-3D-specific parameters */
			printf("X-axis rotation in degrees (if not %3d) ",XROT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) XROT = k;
			printf("Y-axis rotation in degrees (if not %3d) ",YROT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) YROT = k;
			printf("Z-axis rotation in degrees (if not %3d)  ",ZROT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) ZROT = k;
			printf("X-axis scaling factor in pct  (if not %3d)  ",XSCALE);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) XSCALE = k;
			printf("Y-axis scaling factor in pct  (if not %3d)  ",YSCALE);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) YSCALE = k;
			}
            /* Begin Common Parameters */
			printf("Surface Roughness scaling factor in pct  (if not %3d)  ",ROUGH);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) ROUGH = k;
			printf("'Water Level' (minimum color value)  (if not %3d)  ",WATERLINE);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) WATERLINE = k;
			printf("Line-Fill options are 0 - just draw the dots, 1 - connected dots (wire frame),\n");
			printf("  2 - surface-fill, 3 - surface fill (no interpolation), 4 - solid fill,\n");
			printf("  5 - light source before transformation, 6 - light source after transformation\n");
			printf("  Enter the Fill option  (if not %3d)  ",FILLTYPE);
			gets(temp1);
			k = atoi(temp1);
			if (k > 6) k = 6;
            if (k <= 0) k = 0;
			if (temp1[0] >19) FILLTYPE = k;
			printf("Perspective 'height' [1 - 999, 0 for no perspective] (if not %3d) ",ZVIEWER);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) ZVIEWER = k;
			printf("X shift (positive = right)(if not %3d) ",XSHIFT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) XSHIFT = k;
			printf("Y shift (positive = up    )(if not %3d) ",YSHIFT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) YSHIFT = k;
			if(ILLUMINE)
			{
			printf("X value light vector  )(if not %3d) ",XLIGHT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) XLIGHT = k;
			printf("Y value light vector  )(if not %3d) ",YLIGHT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) YLIGHT = k;
			printf("Z value light vector  )(if not %3d) ",ZLIGHT);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) ZLIGHT = k;
			printf("Light Source Smoothing Factor )(if not %3d) ",LIGHTAVG);
			gets(temp1);
			k = atoi(temp1);
			if (temp1[0] >19) LIGHTAVG = k;
			}
			}
		initcorners = 1;
      		}
	else	{
		buzzer(2);
		initmode = -1;
		*readname = NULL;
		}
	}

if (overlay3d && initmode < 0) {	/* overlay command failed */
	setforgraphics();		/* restore the graphics screen */
	overlay3d = 0;			/* forget overlays */
	display3d = 0;			/* forget 3D */
	goto resumeloop;		/* ooh, this is ugly */
	}


restart:                                /* insert key re-starts here */

savedac = 0;				/* don't save the VGA DAC */

lookatmouse = 0;			/* de-activate full mouse-checking */

maxit = inititer;				/* max iterations */
numpasses = initpass-1;				/* single/dual-pass mode */
solidguessing = initsolidguessing;		/* solid-guessing mode */

fractype = initfractype;			/* use the default set   */
cyclelimit = initcyclelimit;			/* default cycle limit	 */
for (i = 0; i < 4; i++) param[i] = initparam[i];/* use the default params*/
for (i = 0; i < 4; i++) potparam[i] = initpot[i];/* TW 6/25/89 */
ccreal = param[0]; ccimag = param[1];		/* default C-values      */
xxmin = initxmin; xxmax = initxmax;		/* default corner values */
yymin = initymin; yymax = initymax;		/* default corner values */

if (xxmin > xxmax) { ftemp = xxmax; xxmax = xxmin; xxmin = ftemp; }
if (yymin > yymax) { ftemp = yymax; yymax = yymin; yymin = ftemp; }

/* set some reasonable limits on the numbers (or the algorithms will fail) */

bitshift = FUDGEFACTOR2;			/* shift bits by this much */
if (xxmax - xxmin > 31.99) xxmax = xxmin + 31.99;	/* avoid overflow */
if (yymax - yymin > 31.99) yymax = yymin + 31.99;
if (fractype == MANDEL || fractype == MANDELFP || 
	fractype == JULIA || fractype == JULIAFP) {
	if (fabs(potparam[0]) < .0001		/* adjust shift bits if */
		|| debugflag == 1234)		/* not using potential  */
		bitshift = FUDGEFACTOR;
	if (ccreal < -1.99 || ccreal > 1.99) ccreal = 1.99;
	if (ccimag < -1.99 || ccimag > 1.99) ccimag = 1.99;
	if (xxmax - xxmin > 3.99) xxmax = xxmin + 3.99;
	if (yymax - yymin > 3.99) yymax = yymin + 3.99;
	}
fudge = 1; fudge = fudge << bitshift;		/* fudged value for printfs */

ccreal = ccreal * fudge; creal = ccreal;
ccimag = ccimag * fudge; cimag = ccimag;
xxmin = xxmin * fudge; xmin = xxmin;
xxmax = xxmax * fudge; xmax = xxmax;
yymin = yymin * fudge; ymin = yymin;
yymax = yymax * fudge; ymax = yymax;

#ifndef __TURBOC__
if (extraseg == 0) { 
	xxxalloc = (long huge *)halloc(16384L,4); 
	extraseg = FP_SEG(xxxalloc);
	}
#endif

if (extraseg == 0) {			/* oops.  not enough memory     */
	buzzer(2);
	printf(" I'm sorry, but you don't have enough free memory \n");
	printf(" to run this program.\n\n");
	exit(1);
	}

adapter = initmode;			/* set the video adapter up	*/
initmode = -1;				/* (once)			*/

helpmode = HELPAUTHORS;			/* use this help mode */
if (adapter < 0)
	help();				/* display the credits screen	*/
helpmode = HELPMENU;			/* now use this help mode */

while (adapter < 0) {			/* cycle through instructions	*/

	if (initbatch == 0) {			/* online-mode only, please */
		while (!keypressed()) ;		/* enable help */
		kbdchar = getakey();
		}
	else
		kbdchar = 27;
	while (kbdchar == 13 || kbdchar == 1013) { /* ENTER calls help, here */
		kbdchar = help();
		}
	adapter = -1;				/* no video adapter yet */
	for (k = 0; k < maxvideomode; k++)	/* search for an adapter */
		if (kbdchar == kbdkeys[k])
			adapter = k;		/* use this adapter */
	if (adapter >= 0) break;		/* bail out if we found one */
	if (kbdchar == 1082) goto restart;	/* restart pgm on Insert Key */
	if (kbdchar == 1000) goodbye();		/* Control-C */
	if (kbdchar == 27) goodbye();		/* exit to DOS on Escape Key */
	if (kbdchar == 1083) goodbye();		/* exit to DOS on Delete Key */
        if (kbdchar == 'd' || kbdchar == 'D') { /* shell to DOS */
        	clscr();
		printf("\n\nShelling to DOS - type 'exit' to return\n\n");
        	system("command.com");
        	goto restart;
        	}
	if (kbdchar == '1' || kbdchar == '2') {	/* select single or dual-pass */
		numpasses = kbdchar - '1';
		solidguessing = 0;
		continue;
		}
	if (kbdchar == 'g' || kbdchar == 'G') {	/* solid-guessing */
		numpasses = 1;
		solidguessing = 1;
		continue;
		}
	if (kbdchar == 'r' || kbdchar == 'R' ||	kbdchar == '3'
		|| kbdchar == 'o' || kbdchar == 'O') {	/* restore old image	*/
		display3d = 0;
		if (kbdchar == '3' || kbdchar == 'o' ||
			kbdchar == 'O') display3d = 1;
		setvideomode(3,0,0,0);	/* switch to text mode */
			printf("\n Restart from what file? ");
			gets(readname);
			goto restorestart;
			}
	if (kbdchar == 't' || kbdchar == 'T') {	/* set fractal type */
		olddotmode = dotmode;	/* save the old dotmode */
		dotmode = 1;		/* force a disable of any 8514/A */
		setvideomode(3,0,0,0);	/* switch to text mode */
		printf("\nSelect one of the available fractal types below\n\n");
		printf("(your current fractal type is %s)\n\n",typelist[initfractype]);
		for (i = 0; typelist[i] != NULL; i++) {
			printf("%15s", typelist[i]);
			if ((i & 3) == 3 ) printf("\n");
			}
		j = -1; while (j < 0) {
			printf("\n\n Enter your fractal type (an empty response bails out) ->");
			gets(temp1);
			if (temp1[0] == 0) break;
			strlwr(temp1);
			for (i = 0; typelist[i] != NULL && strcmp(typelist[i], temp1) != 0; i++) ;
			if (typelist[i] == NULL) {
				buzzer(2);
				printf("\nPlease try that again");
				}
			else
				j = i;
			}
		if (temp1[0] == 0) goto restart;
		initfractype = j;
		for ( i = 0; i < 4; i++)
			initparam[i] = 0.0;
		for ( i = 0; i < 4; i++) /* TW 6/19/89 */
			potparam[i] = 0.0;
		printf("\n\n Please enter any parameters that apply (an empty response bails out) \n\n");
		for (i = 0; i < 4; i++) {
			if (paramlist[initfractype][i][0] == 0) break;
			printf(" %s => ", paramlist[initfractype][i]);
			gets(temp1);
			initparam[i] = atof(temp1);
			if (temp1[0] == 0) i = 99;
			}
		initcorners = 1;
		initxmin = -2.5;
		initxmax =  1.5;
		initymin = -1.5;
		initymax =  1.5;
		if (initfractype != MANDEL && initfractype != MANDELFP) {
			initxmin = -2.0;
			initxmax =  2.0;
			}
		dotmode = olddotmode;
		goto restart;
		}
	else 
		buzzer(2);
	}

historyptr = 0;					/* initialize history ptr */
history[0][0] = -1;
zoomoff = 1;					/* zooming is enabled */

helpmode = HELPMAIN;				/* switch help modes */


more = 1;
while (more) {					/* eternal loop */

						/* collect adapter info */
	fromvideotable(adapter);
	axmode  = videoentry.videomodeax;	 /* video mode (BIOS call) */
	bxmode  = videoentry.videomodebx;	 /* video mode (BIOS call) */
	cxmode  = videoentry.videomodecx;	 /* video mode (BIOS call) */
	dxmode  = videoentry.videomodedx;	 /* video mode (BIOS call) */
	dotmode = videoentry.dotmode;		/* assembler dot read/write */
	xdots   = videoentry.xdots;		/* # dots across the screen */
	ydots   = videoentry.ydots;		/* # dots down the screen   */
	colors  = videoentry.colors;		/* # colors available */

	diskvideo = 0;				/* set diskvideo flag */
	if (dotmode == 11)			/* default assumption is disk */
		diskvideo = 1;

	memcpy(olddacbox,dacbox,256*3);		/* save the DAC */
	diskisactive = 1;		/* flag for disk-video routines */
	if (overlay3d) {
		setforgraphics();	/* restore old graphics image */
		overlay3d = 0;
		}
	else
	        setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */
	diskisactive = 0;		/* flag for disk-video routines */
	if (savedac) {
		memcpy(dacbox,olddacbox,256*3);		/* restore the DAC */
		spindac(0,1);
		}
	savedac = 1;				/* assume we save next time */

	if(*readname && showfile==0) {
		/*
		only requirements for gifview: take file name in global
		variable "readname" - link to frasmint's video (I used putcolor),
		and should NOT set video mode (that's done here)
		*/

		/* TIW 7/22/89 */
		if (display3d)			/* set up 3D decoding */
			outln = line3d;
        else if(filetype >= 1)
            outln = outlin16;
        else
            outln = out_line;
		{
		   extern int filetype;	
           if(filetype == 0)
              gifview();
           else
              tgaview();
        }
        /* END 7/22/89 */
		display3d = 0;			/* turn off 3D retrievals */
 		}

	xstep   = xdots / 40;			/* zoom-box increment: across */
	ystep   = ydots / 40;			/* zoom-box increment: down */
	if (xdots == 640 && ydots == 350)	/* zoom-box adjust:  640x350 */
		{ xstep = 16; ystep =  9; }
	if (xdots == 632 && ydots == 474)	/* zoom-box adjust:  632x474 */
		{ xstep = 16; ystep = 12; }
	if (xdots == 720 && ydots == 512)	/* zoom-box adjust:  720x512 */
		{ xstep = 20; ystep = 15; }
	if (xdots * 3 == ydots * 4)		/* zoom-box adjust:  VGA Tweaks */
		{ xstep = 16; ystep = 12; }
	if (xdots == 1024 && ydots == 768)	/* zoom-box adjust: 1024x768 */
		{ xstep = 32; ystep = 24; }
	if (xdots == 1016 && ydots == 762)	/* zoom-box adjust: 1018x762 */
		{ xstep = 32; ystep = 24; }

	delx = ((xmax - xmin) / (xdots - 1));	/* calculate the stepsizes */
	dely = ((ymax - ymin) / (ydots - 1));

	if (delx <= 1 || dely <= 1) {	/* oops.  zoomed too far */
		zoomoff = 0;
		dely = 1;
		delx = 1;
		}
	if (dotmode == 11 || fractype == PLASMA)	/* disable zooming */
		zoomoff = 0;

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
			history[i][6] = fractype;
		}

	if (history[historyptr][0] != xmax  ||	/* save any (new) zoom data */
	    history[historyptr][1] != xmin  ||
	    history[historyptr][2] != ymax  ||
	    history[historyptr][3] != ymin  ||
	    history[historyptr][4] != creal ||
	    history[historyptr][5] != cimag ||
	    history[historyptr][6] != fractype) {
		if (++historyptr == MAXHISTORY) historyptr = 0;
		history[historyptr][0] = xmax;
		history[historyptr][1] = xmin;
		history[historyptr][2] = ymax;
		history[historyptr][3] = ymin;
		history[historyptr][4] = creal;
		history[historyptr][5] = cimag;
		history[historyptr][6] = fractype;
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
		save_info.xmin         = xxmin;
		save_info.xmax         = xxmax;
		save_info.ymin         = yymin;
		save_info.ymax         = yymax;
		save_info.creal        = param[0];
		save_info.cimag        = param[1];
		save_info.videomodeax  = videoentry.videomodeax;
		save_info.videomodebx  = videoentry.videomodebx;
		save_info.videomodecx  = videoentry.videomodecx;
		save_info.videomodedx  = videoentry.videomodedx;
		save_info.dotmode	= videoentry.dotmode;
		save_info.xdots		= videoentry.xdots;
		save_info.ydots		= videoentry.ydots;
		save_info.colors	= videoentry.colors;
		for (i = 0; i < 10; i++)
		   save_info.future[i] = 0;

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
				kbdchar = 's';
				initbatch = 2;
				}
			else
				kbdchar = 27;	/* then, exit          */
			}
		switch (kbdchar) {
			case 't':			/* new fractal type */
			case 'T':
				olddotmode = dotmode;	/* save the old dotmode */
				dotmode = 1;		/* force a disable of any 8514/A */
				setvideomode(3,0,0,0);	/* switch to text mode */
				printf("\nSelect one of the available fractal types below\n\n");
				printf("(your current fractal type is %s)\n\n",typelist[fractype]);
				for (i = 0; typelist[i] != NULL; i++) {
					printf("%15s", typelist[i]);
					if ((i & 3) == 3 ) printf("\n");
					}
				j = -1; while (j < 0) {
					printf("\n\n Enter your fractal type (an empty response bails out) ->");
					gets(temp1);
					if (temp1[0] == 0) goto restart;
					strlwr(temp1);
					for (i = 0; typelist[i] != NULL && strcmp(typelist[i], temp1) != 0; i++) ;
					if (typelist[i] == NULL) {
						printf("\nPlease try that again");
						buzzer(2);
						}
					else
						j = i;
					}
				if (temp1[0] == 0) break;
				initfractype = j;
				savedac = 0;
				for ( i = 0; i < 4; i++)
					initparam[i] = 0.0;
				printf("\n\n Please enter any parameters that apply (an empty response bails out)\n\n");
				for (i = 0; i < 4; i++) {
					if (paramlist[initfractype][i][0] == 0) break;
					printf(" %s => ", paramlist[initfractype][i]);
					gets(temp1);
					initparam[i] = atof(temp1);
					if (temp1[0] == 0) i = 99;
					}
				initcorners = 1;
				initxmin = -2.5;
				initxmax =  1.5;
				initymin = -1.5;
				initymax =  1.5;
				if (initfractype != MANDEL && initfractype != MANDELFP) {
					initxmin = -2.0;
					initxmax =  2.0;
					}
				initmode = adapter;
				dotmode = olddotmode;
				goto restart;
				break;
			case 32:			/* spacebar */
				if (fractype == MANDEL || fractype == MANDELFP) {	/* switch to Julia */
					if (fractype == MANDEL) fractype = JULIA;
					if (fractype == MANDELFP) fractype = JULIAFP;
					creal = (xmax + xmin) / 2;
					cimag = (ymax + ymin) / 2;
					param[0] = creal; param[0] /= fudge;
					param[1] = cimag; param[1] /= fudge;
					jxmin = xmin;  jxmax = xmax;
					jymin = ymin;  jymax = ymax;
					xxmax =  1.99;
 					xxmin = -1.99;
					yymax =   1.5;
					yymin =  -1.5;
					xxmin = xxmin * fudge; xmin = xxmin;
					xxmax = xxmax * fudge; xmax = xxmax + 100;
					yymin = yymin * fudge; ymin = yymin - 100;
					yymax = yymax * fudge; ymax = yymax;
					zoomoff = 1;
					}
				else { if (fractype == JULIA || fractype == JULIAFP) {	/* switch to Mandel */
					if (fractype == JULIA) fractype = MANDEL;
					if (fractype == JULIAFP) fractype = MANDELFP;
					creal = 0;
					cimag = 0;
					param[0] = 0;
					param[1] = 0;
					xmin = jxmin;  xmax = jxmax;
					ymin = jymin;  ymax = jymax;
					zoomoff = 1;
					}
				else buzzer(2); }		/* no switch */
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
				fractype = history[historyptr][6];
				param[0] = creal; param[0] /= fudge;
				param[1] = cimag; param[1] /= fudge;
				zoomoff = 1;
				kbdmore = 0;
				break;
			case 9:				/* tab */
				xxmax = xmax; xxmax = xxmax / fudge;
				xxmin = xmin; xxmin = xxmin / fudge;
				yymax = ymax; yymax = yymax / fudge;
				yymin = ymin; yymin = yymin / fudge;
				setfortext();
				printf("\n\nCurrent Fractal Type is: %-7s \n\n", typelist[fractype]);
				printf("   Param1 = %12.9f \n",param[0]);
				printf("   Param2 = %12.9f \n",param[1]);
				printf("\nCurrent Screen Corners are: \n\n");
				printf("     Xmin = %12.9f \n",xxmin);
				printf("     Xmax = %12.9f \n",xxmax);
				printf("     Ymin = %12.9f \n",yymin);
				printf("     Ymax = %12.9f \n",yymax);
				printf("\nCurrent Iteration Maximum = %d\n",maxit);
				printf("\nPress any key to continue...");
				getakey();
				setforgraphics();
				continue;	
			break;
			case 'd':			/* shell to MS-DOS */
			case 'D':
				setfortext();
				printf("\n\nShelling to DOS - type 'exit' to return\n\n");
				if (axmode == 0 || axmode > 7) {
					printf("Note:  Your graphics image is still squirreled away in your video\n");
					printf("adapter's memory.  Switching video modes (say, to get your cursor back)\n");
					printf("will clobber part of that image.  Sorry - it's the best we could do.\n\n");
					}
				system("command.com");
				setforgraphics();
				break;
			case '<':			/* lower iter maximum */
			case ',':
				if (maxit >= 10+initincr) maxit -= initincr;
				maxit -= initincr;	/* for fall-thru */
			case '>':			/* raise iter maximum */
			case '.':
				if (maxit <= 1000-initincr) maxit += initincr;
				continue;
				break;
			case 'c':			/* switch to cycling */
			case 'C':
				rotate(0);
				continue;
				break;
			case '+':			/* rotate palette */
				rotate(+1);
				continue;
				break;
			case '-':			/* rotate palette */
				rotate(-1);
				continue;
				break;
                        case 's':                       /* save-to-disk */
                        case 'S':
				drawbox(0);		/* clobber zoom-box */
				ixmin = 0;  ixmax = xdots-1;
				iymin = 0;  iymax = ydots-1;
				xmin = lx0[0];
				xmax = lx0[xdots-1];	/* re-set xmax and ymax */
				ymin = ly0[ydots-1];
				ymax = ly0[0];

                            /* convert parameters back to doubles */
                                xxmax = xmax; xxmax = xxmax / fudge;
                                xxmin = xmin; xxmin = xxmin / fudge;
                                yymax = ymax; yymax = yymax / fudge;
                                yymin = ymin; yymin = yymin / fudge;

                            /* set save parameters in save structure */
				strcpy(save_info.info_id, INFO_ID);
		                save_info.iterations   = maxit;
		                save_info.fractal_type = fractype;
		                save_info.xmin         = xxmin;
		                save_info.xmax         = xxmax;
		                save_info.ymin         = yymin;
		                save_info.ymax         = yymax;
		                save_info.creal        = param[0];
		                save_info.cimag        = param[1];
		                save_info.videomodeax   =
					videoentry.videomodeax;
		                save_info.videomodebx   =
					videoentry.videomodebx;
		                save_info.videomodecx   =
					videoentry.videomodecx;
		                save_info.videomodedx   =
					videoentry.videomodedx;
		                save_info.dotmode	=
					videoentry.dotmode;
		                save_info.xdots		=
					videoentry.xdots;
		                save_info.ydots		=
					videoentry.ydots;
		                save_info.colors	=
					videoentry.colors;
				for (i = 0; i < 10; i++)
					save_info.future[i] = 0;
				diskisactive = 1;	/* flag for disk-video routines */
                                savetodisk(savename);
				diskisactive = 0;	/* flag for disk-video routines */
				continue;
                                break;
			case 'o':			/* 3D overlay */
			case 'O':
				overlay3d = 1;
			case '3':			/* restore-from (3d) */
				display3d = 1;
			case 'r':			/* restore-from */
			case 'R':
				if (overlay3d) {
					setfortext();	/* save graphics image */
					printf("\n Overlay from");
					}
				else	{
					olddotmode = dotmode;	/* save the old dotmode */
					dotmode = 1;		/* in case of an 8514/A mode */
					setvideomode(3,0,0,0);	/* switch to text mode */
					printf("\n Restart from");
					}
				if (*readname)
					printf(" what file (if not %s)? ",
						readname);
				else
					printf(" what file? ");
				gets(temp1);
				if (temp1[0] != 0)
					strcpy(readname,temp1);
				dotmode = olddotmode;
				goto restorestart;
				break;
			case '1':			/* single-pass mode */
			case '2':			/* dual-pass mode */
				numpasses = kbdchar - '1';
				solidguessing = 0;
				kbdmore = 0;
				break;
			case 'g':			/* solid-guessing */
			case 'G':
				numpasses = 1;
				solidguessing = 1;
				kbdmore = 0;	
				break;
			case 'b':			/* make batch file */
			case 'B':
				{
				FILE *batch;
				xxmax = xmax; xxmax = xxmax / fudge;
				xxmin = xmin; xxmin = xxmin / fudge;
				yymax = ymax; yymax = yymax / fudge;
				yymin = ymin; yymin = yymin / fudge;
				batch=fopen("frabatch.bat","a");
				fprintf(batch,"fractint type=%s", typelist[fractype]);
				if (delx > 1000 && dely > 1000)
				fprintf(batch," corners=%g/%g/%g/%g",
					xxmin, xxmax, yymin, yymax);
				else
				fprintf(batch," corners=%+12.9f/%+12.9f/%+12.9f/%+12.9f",
					xxmin, xxmax, yymin, yymax);
				if (param[0] != 0.0 || param[1] != 0.0)
					fprintf(batch," params=%g/%g", param[0], param[1]);
				if (maxit != 150)
					fprintf(batch," maxiter=%d", maxit);
				if (initincr != 50)
					fprintf(batch," iterincr=%d", initincr);
				if (inside != 1)
					fprintf(batch," inside=%d", inside);
				fprintf(batch,"\n");
				if(batch != NULL) fclose(batch);
				}
				break;
                                /*  vvvvv  MDS 7/1/89  vvvvv  */
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
				kbdmore = 0;
				break;
			case 1082:			/* insert */
				dotmode = 1;
				setvideomode(3,0,0,0); 	/* force text mode */
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
			xmin = lx0[ixmin];
			xmax = lx0[ixmax];
			ymin = ly0[iymax];
			ymax = ly0[iymin];
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

argerror(badarg)			/* oops.   couldn't decode this */
char *badarg;
{

buzzer(2);
printf("\nOops.   I couldn't understand the argument '%s'\n\n",badarg);
printf("(see the Startup Help screens or FRACTINT.DOC for a complete argument list\n");
printf(" with descriptions):\n\n");
exit(1);

}

goodbye()
{
setvideomode(3,0,0,0);
printf("\n\nThank You for using FRACTINT\n\n");
exit(0);
}

