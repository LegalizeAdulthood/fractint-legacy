/*
	FRACTINT - Integer Mandelbrot/Julia Fractal Generator
			Main Routine
		Original/Primary Author:  Bert Tyler
		Mandelbrot/Julia/32-bit Integer Fractals by Bert Tyler
		Newton/Lamda/Other Fractals by Timothy Wegner
		Super-VGA support by Timothy Wegner and John Bridges
		Mouse support by Michael Kaufman
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PUTTHEMHERE 1			/* stuff common external arrays here */

#include "fractint.h"

void main(int argc, char *argv[]);
int getakey(void);
void setvideomode(int a, int b, int c, int d);
void drawbox(int);
void spindac(int, int);

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
	"Alt-A","Alt-S","Alt-D","Alt-F","Alt-G",
	"Alt-H","Alt-J","Alt-K","Alt-L",
	"Alt-Z","Alt-X","Alt-C","Alt-V","Alt-B","Alt-N","Alt-M",
	"END"};
int kbdkeys[] = {		/* Function Keystrokes for above names */
	1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068,
	1084, 1085, 1086, 1087, 1088, 1089, 1090, 1091, 1092, 1093,
	1094, 1095, 1096, 1097, 1098, 1099, 1100, 1101, 1102, 1103,
	1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113,
	1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129,
	1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025,
	1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038,
	1044, 1045, 1046, 1047, 1048, 1049, 1050,
	0};

char *accessmethod[] =
	{" ","B"," "," "," "," "," "," "," "}; /* BIOS access indicator */

struct fractal_info save_info, read_info; /*  for saving data in file */

char    readname[81];     /* name of fractal input file */
int     showfile;        /* has file been displayed yet? */
int	warn;		/* 0 if savename warnings off, 1 if warnings on */

#define	FUDGEFACTOR	29		/* fudge all values up by 2**this */
#define MAXHISTORY	50		/* save this many historical rcds */

long	history[MAXHISTORY][7];	/* for historical (HOME Key) purposes */
char	savename[80];		/* save files using this name */

int	debugflag;		/* internal use only - you didn't see this */
int	timerflag;		/* you didn't see this, either */
/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
	int	dotmode;			/* video access method      */
	int	oktoprint;			/* 0 if printf() won't work */
	int	cyclelimit;			/* color-rotator upper limit */
	int	xdots, ydots;			/* # of dots on the screen  */
	int	colors;				/* maximum colors available */
	int	maxit;				/* try this many iterations */
	int	ixmin, ixmax, iymin, iymax;	/* corners of the zoom box  */
	int	boxcount;			/* 0 if no zoom-box yet     */

	int	inside;				/* inside color: 1=blue     */
	int	fractype;			/* if == 0, use Mandelbrot  */
	int	numpasses;			/* 0 if single-pass, else 1 */
	int	solidguessing;			/* 0 if disabled, else 1    */
	long	creal, cimag;			/* real, imag'ry parts of C */
	long	lx0[MAXPIXELS], ly0[MAXPIXELS];	/* x, y grid                */
	long	delx, dely;			/* screen pixel increments */
	double	param[4];			/* up to four parameters    */
	long	fudge;				/* 2**fudgefactor	*/

	int	hasconfig;			/* = 0 if 'fractint.cfg'    */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	extraseg;		/* used by Save-to-DISK routines */
extern int	cpu;			/* cpu type			*/
extern int	lookatmouse;		/* used to activate non-button mouse movement */

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
	int	i, j, k;			/* temporary loop counters */

	int	initbatch;			/* 1 if batch run (no kbd)  */
	int	initmode;			/* initial video mode       */
	int	inititer;			/* initial value of maxiter */
	int	initincr;			/* initial maxiter incrmnt  */
	int	initpass;			/* initial pass mode        */
	int	initsolidguessing;		/* initial solid-guessing md*/
	int	initfractype;			/* initial type set flag    */
	int	initcyclelimit;			/* initial cycle limit      */
	int	initcorners;			/* initial flag: corners set*/
	double	initxmin,initxmax;		/* initial corner values    */
	double	initymin,initymax;		/* initial corner values    */
	double	initparam[4];			/* initial parameters       */
	char	temp1[81], temp2[81], *slash;	/* temporary strings        */
	int	displaypage;			/* signon display page      */

hasconfig = readconfig();			/* read fractint.cfg, if any */

for (maxvideomode = 0;				/* get size of adapter list */
	strcmp(videomode[maxvideomode].name,"END") != 0
		&& maxvideomode < 76;	/* that's all the Fn keys we got! */
	maxvideomode++);

warn = 0;					/* no warnings on savename */
initbatch = 0;					/* not in batch mode      */
initmode = -1;					/* no initial video mode  */
inside = 1;					/* inside color = blue    */
inititer = 150;					/* initial maxiter        */
initincr = 50;					/* initial iter increment */
initpass = 2;					/* initial dual-pass mode */
initsolidguessing = 1;				/* initial solid-guessing */
initfractype = 0;				/* initial type Set flag  */
initcorners = 0;				/* initial flag: no corners */
for (i = 0; i < 4; i++) initparam[i] = 0;	/* initial parameter values */
initxmin = -2.5; initxmax = 1.5;		/* initial corner values  */
initymin = -1.5; initymax = 1.5;		/* initial corner values  */
strcpy(savename,"fract001");			/* initial save filename  */
initcyclelimit=55;				/* spin-DAC default speed limit */
oktoprint = 1;					/* default:  printf() ok  */

*readname= NULL;                                  /* initial input filename */

for (i = 1; i < argc; i++) {			/* cycle through args	*/
	strcpy(temp1,argv[i]);
	strlwr(temp1);				/* using lower case	*/
	for (j = 1; j < strlen(temp1) && temp1[j] != '='; j++) ;
	if (j < strlen(temp1)) k = atoi(&temp1[j+1]);

	if (j == strlen(temp1)){
        strcpy(readname,temp1);
		showfile = 1;
		}
	else if (strncmp(temp1,"batch=",j) == 0 ) {		/* batch=?	*/
		k = temp1[j+1];
		if (k == 'c' || k == 'C') {		/* config run */
			makeconfig();
			goodbye();
			}
		if (k == 'y' || k == 'Y')
			initbatch = 1;
		}
	else if (strncmp(temp1,"warn=",j) == 0 ) {		/* warn=?	*/
		k = temp1[j+1];
		if (k == 'y' || k == 'Y')
			warn = 1;
		}
	else if (strncmp(temp1,"type=",j) == 0 ) {		/* type=?	*/
		for (k = 0; typelist[k] != NULL; k++)
			if (strcmp(&temp1[j+1],typelist[k]) == 0)
				break;
		if (typelist[k] == NULL) argerror(temp1);
		initfractype = k;
		}
	else if (strncmp(temp1,"inside=",j) == 0 ) {		/* inside=?	*/
		inside = k;
		}
	else if (strncmp(temp1,"maxiter=",j) == 0) {	/* maxiter=?	*/
		if (k < 10 || k > 1000) argerror(temp1);
		inititer = k;
		}
	else if (strncmp(temp1,"iterincr=",j) == 0) {	/* iterincr=?	*/
		if (k <= 0 || k > inititer) argerror(temp1);
		initincr = k;
		}
	else if (strncmp(temp1,"passes=",j) == 0) {	/* passes=?	*/
		initsolidguessing = 0;
		if ( temp1[j+1] == 'g') {		/* solid-guessing */
			k = 2;
			initsolidguessing = 1;
			}
		if (k < 1 || k > 2) argerror(temp1);
		initpass = k;
		}
	else if (strncmp(temp1,"cyclelimit=",j) == 0 ) {/* cyclelimit=?	*/
		if (k > 1 && k <= 256)
			initcyclelimit = k;
		}
	else if (strncmp(temp1,"savename=",j) == 0) {	/* savename=?	*/
		strcpy(savename,&temp1[9]);
		}
	else if (strncmp(temp1,"video=",j) == 0) {	/* video=?	*/
		for (k = 0; k < maxvideomode; k++) {
			strcpy(temp2,fkeys[k]);
			strlwr(temp2);
			if (strcmp(&temp1[j+1],temp2) == 0)
				break;
			}
			if (k == maxvideomode) argerror(temp1);
		initmode = k;
		}
	else if (strncmp(temp1,"params=",j) == 0) {	/* params=?,?	*/
		if (initcorners == 0) {
			initxmin = -2.0; initxmax = 2.0;
			initymin = -1.5; initymax = 1.5;
			}
		k = 0;
		slash = strchr(temp1,'=');
		while ( k < 4) {
			initparam[k++] = atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		}
	else if (strncmp(temp1,"corners=",j) == 0) {	/* corners=?,?,?,? */
	        initcorners = 1;
		slash = temp1;
		initxmin=atof(&temp1[j+1]);
		if ((slash = strchr(slash,'/')) == NULL) argerror(temp1);
		initxmax=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(temp1);
		initymin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(temp1);
		initymax=atof(++slash);
		}
	else if (strncmp(temp1,"debugflag=",j) == 0) {	/* internal use only */
		debugflag = k;
		}
	else argerror(temp1);
	}

restorestart:

if(*readname){  /* This is why readname initialized to null string */
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
 		initmode = getGIFmode(&read_info);
                if(initmode < maxvideomode && hasconfig != 0 
			&& initbatch == 0){
                        char c;
                	printf("fractal file contains following mode:\n\n");
 			               
			printf("%-6s%-25s%5d x%4d%5d %1s  %-25s\n\n",
			        fkeys[initmode],
				videomode[initmode].name,
				videomode[initmode].xdots,
				videomode[initmode].ydots,
				videomode[initmode].colors,
				accessmethod[videomode[initmode].dotmode],
				videomode[initmode].comment);
                        printf("Legal for this machine? ([Y] or N) --> ");
                	while((c=getch())!='y' && c!='Y' && c!='n' && c!='N' && c!=13);
                	if( c == 'n' || c == 'N'){
                		initmode = -1;
                	   	showfile = 1; /* pretend already displayed */
                           	}
			}
		else 	{
			/* initmode bad */
			if (initmode >= maxvideomode) {
				initmode = -1;
        	       		showfile = 1; /* pretend already displayed */
				}
                       	}
		initcorners = 1;
      		}
	else	{
		printf("\007");
		*readname = NULL;
		}
	}

timerflag = debugflag & 1;		/* separate timer from debug flag */
debugflag -= timerflag;

restart:                                /* insert key re-starts here */

lookatmouse = 0;			/* de-activate full mouse-checking */

maxit = inititer;				/* max iterations */
numpasses = initpass-1;				/* single/dual-pass mode */
solidguessing = initsolidguessing;		/* solid-guessing mode */

fractype = initfractype;			/* use the default set   */
cyclelimit = initcyclelimit;			/* default cycle limit	 */
for (i = 0; i < 4; i++) param[i] = initparam[i];/* use the default params*/
ccreal = param[0]; ccimag = param[1];		/* default C-values      */
xxmin = initxmin; xxmax = initxmax;		/* default corner values */
yymin = initymin; yymax = initymax;		/* default corner values */

if (xxmin > xxmax) { ftemp = xxmax; xxmax = xxmin; xxmin = ftemp; }
if (yymin > yymax) { ftemp = yymax; yymax = yymin; yymin = ftemp; }

/* set some reasonable limits on the numbers (or the algorithms will fail) */

fudge = 1; fudge = fudge << (FUDGEFACTOR-3);	/* fudged value for printfs */
if (xxmax - xxmin > 31.99) xxmax = xxmin + 31.99;	/* avoid overflow */
if (yymax - yymin > 31.99) yymax = yymin + 31.99;
if (fractype == MANDEL || fractype == MANDELFP || 
	fractype == JULIA || fractype == JULIAFP) {
	fudge = 1; fudge = fudge << FUDGEFACTOR;	/* fudged value for printfs */
	if (ccreal < -1.99 || ccreal > 1.99) ccreal = 1.99;
	if (ccimag < -1.99 || ccimag > 1.99) ccimag = 1.99;
	if (xxmax - xxmin > 3.99) xxmax = xxmin + 3.99;
	if (yymax - yymin > 3.99) yymax = yymin + 3.99;
	}

ccreal = ccreal * fudge; creal = ccreal;
ccimag = ccimag * fudge; cimag = ccimag;
xxmin = xxmin * fudge; xmin = xxmin;
xxmax = xxmax * fudge; xmax = xxmax;
yymin = yymin * fudge; ymin = yymin;
yymax = yymax * fudge; ymax = yymax;

setvideomode(3,0,0,0);			/* switch to text mode */

if (extraseg == 0) {			/* oops.  not enough memory     */
	printf("\007 I'm sorry, but you don't have enough free memory \n");
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
	while (kbdchar == 13) {			/* ENTER calls help, here */
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
	if (kbdchar == 'r' || kbdchar == 'R') {	/* restore old image	*/
		setvideomode(3,0,0,0);	/* switch to text mode */
			printf("\n Restart from what file? ");
			gets(readname);
			goto restorestart;
			}
	if (kbdchar == 't' || kbdchar == 'T') {	/* set fractal type */
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
			if (typelist[i] == NULL)
				printf("\007\nPlease try that again");
			else
				j = i;
			}
		if (temp1[0] == 0) goto restart;
		initfractype = j;
		for ( i = 0; i < 4; i++)
			initparam[i] = 0.0;
		printf("\n\n Please enter any parameters that apply (an empty response bails out) \n");
		for (i = 0; i < 4; i++) {
			printf(" Parameter #%d => ", i);
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
		goto restart;
		}
	else printf("\007");
	}

historyptr = 0;					/* initialize history ptr */
history[0][0] = -1;
zoomoff = 1;					/* zooming is enabled */

helpmode = HELPMAIN;				/* switch help modes */

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

	if (delx <= 1 || dely <= 1) {		/* oops.  zoomed too far */
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

        setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */

	if(*readname && showfile==0) {
		/*
		only requirements for gifview: take file name in global
		variable "readname" - link to frasmint's video (I used putcolor),
		and should NOT set video mode (that's done here)
		*/
		gifview();
		showfile = 1;
 		}
	else	{
		if (calcfract() == 0)	/* draw the fractal using "C" */
			printf("\007");		/* finished!  wake up! */
		}

	ixmin = 0;  ixmax = xdots-1;		/* initial zoom box */
	iymin = 0;  iymax = ydots-1;
	boxcount = 0;				/* no zoom box yet  */

	if (fractype == PLASMA && cpu > 88) {
		cyclelimit = 256;		/* plasma clouds need quick spins */
		daccount = 256;
		daclearn = 1;
		}

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
					if (typelist[i] == NULL)
						printf("\007\nPlease try that again");
					else
						j = i;
					}
				if (temp1[0] == 0) break;
				initfractype = j;
				for ( i = 0; i < 4; i++)
					initparam[i] = 0.0;
				printf("\n\n Please enter any parameters that apply (an empty response bails out) \n");
				for (i = 0; i < 4; i++) {
					printf(" Parameter #%d => ", i);
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
				else printf("\007"); }		/* no switch */
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
					videomode[adapter].videomodeax;
		                save_info.videomodebx   =
					videomode[adapter].videomodebx;
		                save_info.videomodecx   =
					videomode[adapter].videomodecx;
		                save_info.videomodedx   =
					videomode[adapter].videomodedx;
		                save_info.dotmode	=
					videomode[adapter].dotmode;
		                save_info.xdots		=
					videomode[adapter].xdots;
		                save_info.ydots		=
					videomode[adapter].ydots;
		                save_info.colors	=
					videomode[adapter].colors;
				for (i = 0; i < 10; i++)
					save_info.future[i] = 0;

				drawbox(0);		/* clobber zoom-box */
				ixmin = 0;  ixmax = xdots-1;
				iymin = 0;  iymax = ydots-1;
                                savetodisk(savename);
				continue;
                                break;
			case 'r':			/* restore-from */
			case 'R':
				setvideomode(3,0,0,0);	/* switch to text mode */
				printf("\n Restart from what file? ");
				gets(readname);
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
				fprintf(batch," corners=%g/%g/%g/%g", xxmin, xxmax, yymin, yymax);
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

goodbye();				/* we done. */

}

argerror(badarg)			/* oops.   couldn't decode this */
char *badarg;
{

printf("\7\nOops.   I couldn't understand the argument '%s'\n\n",badarg);
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
