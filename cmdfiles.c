
/*
	Command-line / Command-File Parser Routines
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include "fractint.h"
#ifdef __TURBOC__
#include <dir.h>
#endif

/* routines in this module	*/

void cmdfiles_overlay(void);
int  cmdfiles(int argc, char *argv[]);
void set_3d_defaults(void);
int  readconfig(void);

static void cmdfile(FILE *);
static int  cmdarg(char *);
static void argerror(char *);
static void makeconfig(void);

/* variables defined by the command line/files processor */
char	temp1[256];		/* temporary strings	    */
char	ifsfilename[80];	/* IFS code file */
char	readname[80];		/* name of fractal input file */
char	PrintName[80];		/* Name for print-to-file */
int	potflag=0;		/* continuous potential enabled? */
int	pot16bit;		/* store 16 bit continuous potential values */
char	savename[80];		/* save files using this name */
char	ifs3dfilename[80];	/* IFS 3D code file */
int	gif87a_flag;		/* 1 if GIF87a format, 0 otherwise */
int	askvideo;		/* flag for video prompting */
char	floatflag;
int	biomorph;		/* flag for biomorph */
int	forcesymmetry;		/* force symmetry */
int	showfile;		/* zero if file display pending */
int	rflag, rseed;		/* Random number seeding flag and value */
int	decomp[2];		/* Decomposition coloring */
int	distest;
char overwrite = 0;	/* 0 if file overwrite not allowed */
int	soundflag;		/* 0 if sound is off, 1 if on */
int	basehertz;		/* sound=x/y/x hertz value */
int	debugflag;		/* internal use only - you didn't see this */
int	timerflag;		/* you didn't see this, either */
int	cyclelimit;		/* color-rotator upper limit */
int	inside; 		/* inside color: 1=blue     */
int	outside;		/* outside color    */
int	finattract;		/* finite attractor logic */
int	display3d;		/* 3D display flag: 0 = OFF */
int	overlay3d;		/* 3D overlay flag: 0 = OFF */
int	init3d[20];		/* '3d=nn/nn/nn/...' values */
int	initbatch;		/* 1 if batch run (no kbd)  */
unsigned initsavetime;		/* autosave minutes	    */
double	initorbit[2];		/* initial orbitvalue */
char	useinitorbit;		/* flag for initorbit */
int	initmode;		/* initial video mode	    */
int	initcyclelimit; 	/* initial cycle limit	    */
unsigned char usemag;		/* use center-mag corners   */
int	bailout;		/* user input bailout value */
double	inversion[3];		/* radius, xcenter, ycenter */
extern	int invert;
extern int fractype;		/* fractal type 	    */
extern double param[4]; 	/* initial parameters	    */
extern double xxmin,xxmax;	/* initial corner values    */
extern double yymin,yymax;	/* initial corner values    */
extern double xx3rd,yy3rd;	/* initial corner values    */
extern char usr_stdcalcmode;	/* '1', '2', 'g', 'b'       */
extern int maxit;		/* max iterations	    */
extern int usr_periodicitycheck; /* periodicity checking  1=on,0=off */
extern char usr_floatflag;	/* flag for float calcs */
extern int usr_distest; 	/* nonzero if distance estimator option */
extern char color_lakes;	/* finite attractor flag */
extern int haze;
extern int RANDOMIZE;
extern int Ambient;
extern int full_color;
extern char light_name[];
extern char back_color[];

extern double potparam[];	/* potential parameters  */
extern int Printer_Resolution, LPTNumber,
	   Printer_Type, Printer_Titleblock,
	   Printer_ColorXlat, Printer_SetScreen,
	   Printer_SFrequency, Printer_SAngle,
	   Printer_SStyle, EPSFileType,
	   Print_To_File;			/* for printer functions */

int	transparent[2]; 	/* transparency min/max values */
int	LogFlag;		/* Logarithmic palette flag: 0 = no */

unsigned char exitmode = 3;	/* video mode on exit */

char FormFileName[80];		/* file to find (type=)formulas in */
char FormName[40];		/* Name of the Formula (if not null) */

extern int video_type;
extern int mode7text;
extern int textsafe;

extern int   viewwindow;
extern float viewreduction;
extern int   viewcrop;
extern float finalaspectratio;
extern int   viewxdots,viewydots;

extern	char *fkeys[];		/* Function Key names for display table */

static	int toolsfile;		/* 1 if inside a TOOLS file, 0 otherwise */

extern char MAP_name[];
extern int mapset;
extern char loadPalette;

extern int eyeseparation; /* Occular Separation */
extern int glassestype;
extern int xadjust; /* Convergence */
extern int yadjust;
extern int xtrans, ytrans; /* X,Y shift with no perspective */
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern char showbox; /* flag to show box and vector in preview */
extern char preview;	    /* 3D preview mode flag */
extern int previewfactor; /* Coarsness */

static int first_init=1;	/* first time into cmdfiles? */
static int init_rseed;
static int initcorners; 	/* corners set flag */


void cmdfiles_overlay() { }	/* for restore_active_ovly */

char LFileName[80]; /* file to find (type=)L-System's in */
char LName[40];    /* Name of L-System */

unsigned char textcolor[]={
      BLUE*16+L_WHITE,	  /* C_TITLE	       title background */
      BLUE*16+L_GREEN,	  /* C_TITLE_DEV       development vsn foreground */
      GREEN*16+YELLOW,	  /* C_HELP_HDG        help page title line */
      WHITE*16+BLACK,	  /* C_HELP_BODY       help page body */
      GREEN*16+GRAY,	  /* C_HELP_INSTR      help page instr at bottom */
      WHITE*16+GRAY,	  /* C_PROMPT_BKGRD    prompt/choice background */
      BLUE*16+WHITE,	  /* C_PROMPT_LO       prompt/choice text */
      BLUE*16+L_WHITE,	  /* C_PROMPT_MED      prompt/choice hdg2/... */
      BLUE*16+YELLOW,	  /* C_PROMPT_HI       prompt/choice hdg/cur/... */
      GREEN*16+L_WHITE,   /* C_PROMPT_INPUT    fullscreen_prompt input */
      MAGENTA*16+L_WHITE, /* C_CHOICE_CURRENT  fullscreen_choice input */
      BLACK*16+WHITE,	  /* C_CHOICE_SP_INSTR speed key bar & instr */
      BLACK*16+L_MAGENTA, /* C_CHOICE_SP_KEYIN speed key value */
      WHITE*16+BLUE,	  /* C_GENERAL_HI      tab, thinking, IFS */
      WHITE*16+BLACK,	  /* C_GENERAL_MED */
      WHITE*16+GRAY,	  /* C_GENERAL_LO */
      BLACK*16+L_WHITE,   /* C_GENERAL_INPUT */
      WHITE*16+BLACK,	  /* C_DVID_BKGRD      disk video */
      BLACK*16+YELLOW,	  /* C_DVID_HI */
      BLACK*16+L_WHITE,   /* C_DVID_LO */
      RED*16+L_WHITE,	  /* C_STOP_ERR        stop message, error */
      GREEN*16+BLACK,	  /* C_STOP_INFO       stop message, info */
      BLUE*16+WHITE,	  /* C_TITLE_LOW       bottom lines of title screen */
      GREEN*16+BLACK,	  /* C_AUTHDIV1        title screen dividers */
      GREEN*16+GRAY,	  /* C_AUTHDIV2        title screen dividers */
      BLACK*16+L_WHITE,   /* C_PRIMARY	       primary authors */
      BLACK*16+WHITE	  /* C_CONTRIB	       contributing authors */
      };

extern int active_system;

/*
	cmdfiles(argc,argv) process the command-line arguments
		it also processes the 'sstools.ini' file and any
		indirect files ('fractint @myfile')
*/

int cmdfiles(argc,argv)
int argc;
char *argv[];
{
double	atof(), ftemp;			/* floating point stuff    */
int	i, j, k, l;			/* temporary loop counters */

char	curarg[141];			/* temporary strings	    */

char tempstring[101];			/* temporary strings	    */
FILE *initfile; 			/* for .INI, '@' files      */

ENTER_OVLY(OVLY_CMDFILES);

gif87a_flag = 0;			/* turn on GIF89a processing */

usr_periodicitycheck = 1;		/* turn on periodicity	  */
rflag = 0;				/* not a fixed srand() seed */
if (first_init)
	init_rseed = (int)time(NULL);
rseed = init_rseed;
usr_floatflag = 0;			/* turn off the float flag */
biomorph = -1;				/* turn off biomorph flag */
askvideo = 1;				/* turn on video-prompt flag */
overwrite = 0;			    /* don't overwrite */
soundflag = -1; 			/* sound is on		  */
basehertz = 440;			/* basic hertz rate */
initbatch = 0;				/* not in batch mode	  */
initsavetime = 0;			/* no auto-save 	  */
initmode = -1;				/* no initial video mode  */
inside = 1;				/* inside color = blue	  */
outside = -1;				/* outside color = -1 (not used) */
finattract = 0; 			/* disable finite attractor logic */
maxit = 150;				/* initial maxiter	  */
/* initincr = 50;      */		/* initial iter increment */
usr_stdcalcmode = 'g';                  /* initial solid-guessing */
fractype = 0;				/* initial type Set flag  */
initcorners = 0;			/* initial flag: no corners */
usemag = 0;				/* use corners, not center-mag */
bailout = 0;				/* no user-entered bailout */
for (i = 0; i < 4; i++) param[i] = FLT_MAX; /* initial parameter values */
for (i = 0; i < 3; i++) potparam[i]  = 0.0; /* initial potential values */
for (i = 0; i < 3; i++) inversion[i] = 0.0;  /* initial invert values */
invert = 0;
decomp[0] = decomp[1] = 0;
usr_distest = 0;
initorbit[0] = initorbit[1] = 0.0;	/* initial orbit values */
useinitorbit = 0;
forcesymmetry = 999;			/* symmetry not forced */

viewwindow = 0; 			/* full screen */
viewreduction = 4.2;
viewcrop = 1;
finalaspectratio = SCREENASPECT;
viewxdots = viewydots = 0;

xx3rd = xxmin = -2.5; xxmax = 1.5;	/* initial corner values  */
yy3rd = yymin = -1.5; yymax = 1.5;	/* initial corner values  */
pot16bit = potflag = 0;
initcyclelimit=55;			/* spin-DAC default speed limit */
transparent[0] = transparent[1] = 0;		/* no min/max transparency */
LogFlag = 0;					/* no logarithmic palette */
ifsfilename[0] = 0;				/* initially current directory */
ifs3dfilename[0] = 0;				/* initially current directory */
mapset = 0;					/* no map= name active */
loadPalette = 0;

debugflag = 0;					/* debugging flag(s) are off */
timerflag = 0;					/* timer flags are off	    */

display3d = 0;					/* 3D display is off	    */
overlay3d = 0;					/* 3D overlay is off	    */

/* 3D defaults */
SPHERE = FALSE;
preview = 0;
showbox = 0;
previewfactor = 20;
xadjust = 0;
yadjust = 0;
eyeseparation = 0;
glassestype = 0;
xtrans = 0;
ytrans = 0;
red_crop_left	= 4;
red_crop_right	= 0;
blue_crop_left	= 0;
blue_crop_right = 4;
red_bright     = 80;
blue_bright   = 100;
set_3d_defaults();

set_trig_array(0,"sin");                        /* trigfn defaults */
set_trig_array(1,"sqr");
set_trig_array(2,"sinh");
set_trig_array(3,"cosh");

strcpy(readname,".\\");                         /* initially current directory */
showfile = 1;

Printer_Type = 2;				/* assume an IBM/EPSON	  */
Printer_Resolution = 60;			/* assume low resolution  */
Printer_Titleblock = 0; 			/* assume no title block  */
Printer_ColorXlat = 0;				/* assume positive image  */
Printer_SetScreen = 0;				/* assume default screen  */
Printer_SFrequency = 0; 			/* New screen frequency   */
Printer_SAngle = 0;				/* New screen angle	  */
Printer_SStyle = 0;				/* New screen style	  */
Print_To_File = 0;				/* No print-to-file	  */
EPSFileType = 0;				/* Assume no save to .EPS */

LPTNumber = 1;					/* assume LPT1 */

strcpy(FormFileName,"fractint.frm");            /* default formula file */
strcpy(FormName,"");                            /* default formula name */

strcpy(LFileName,"fractint.l");
strcpy(LName,"");

if (first_init) {
	strcpy(savename,"fract001");            /* initial save filename  */
	strcpy(light_name,"light001.tga");      /* initial light filename */
	strcpy(PrintName,"fract001.prn");       /* initial print-to-file  */
	}


toolsfile = 1;					/* enable TOOLS processing */

findpath("sstools.ini", tempstring);            /* look for SSTOOLS.INI */
if (tempstring[0] != 0) 			/* found it! */
	if ((initfile = fopen(tempstring,"r")) != NULL)
		cmdfile(initfile);		/* process it */

toolsfile = 0;					/* disable TOOLS processing */

for (i = 1; i < argc; i++) {			/* cycle through args	*/
	strcpy(curarg,argv[i]);
	strlwr(curarg); 			/* using lower case	*/
	for (j = 1; j < strlen(curarg) && curarg[j] != '='; j++) ;

	if (j < strlen(curarg)) {		/* xxx=yyy argument? */
		cmdarg(curarg); 		/* process it */
		continue;
		}

	if (curarg[0] == ';')                   /* start of comments? */
		break;				/* we done! */

	if (curarg[0] == '@') {                 /* command indirection? */
		if ((initfile = fopen(&curarg[1],"r")) != NULL) {
			cmdfile(initfile);	/* process it */
			continue;
			}
		else argerror(curarg);		/* oops.  error. */
		}

	strcpy(readname,curarg);		/* else, assume a filename */
	showfile = 0;

	}

for (i = 0; i < 4; i++) {
	if (param[i] != FLT_MAX)
		fractalspecific[fractype].paramvalue[i] = param[i];
	else
		param[i] = fractalspecific[fractype].paramvalue[i];
	}

if (first_init == 0) {
	initmode = -1; /* don't set video when <ins> key used */
	showfile = 1;  /* nor startup image file	      */
	}

first_init = 0;
EXIT_OVLY;
return(0);					/* we done */
}

/*
	cmdfile(handle) processes a single command-file.
		if (toolsfile), it looks for '[...]' codes as well
*/

static void cmdfile(handle)		/* process a command file of some sort */
FILE *handle;
{
char line[513];
int toolssection;
int i, j;

toolssection = 1;			/* assume an implied [fractint] */

while (fgets(line,512,handle) != NULL) {	/* read thru a line at a time */
	i = strlen(line);
	if (i > 0 && line[i-1] == '\n') line[i-1] = 0;  /* strip trailing \n */
	else line[i+1] = 0;				/* add second null   */

	strlwr(line);				/* convert to lower case */
	if (toolsfile && line[0] == '[') {      /* TOOLS-style header */
		toolssection = 0;
		if (strncmp(line,"[fractint]",10) == 0)
			toolssection = 1;
		continue;			/* ignore this line in any case */
		}

	if (! toolssection) continue;		/* not our section */

	i = -1; 				/* get a running start */
	while (line[++i] != 0) {		/* scan through the line */
		if (line[i] <= ' ') continue;   /* white space */
		if (line[i] == ';') break;      /* comments */
		j = i;				/* argument starts here */
		while (line[++i] > ' ');        /* find the argument end */
		line[i] = 0;			/* force an end-of-string */
		if (j == 0 && strcmp(&line[j],"fractint") == 0)
			continue;		/* skip leading "fractint " */
		cmdarg(&line[j]);		/* process the argument */
		}
	}

fclose(handle);
}

/*
	cmdarg(string) processes a single command-line/command-file argument
		isolate 'variable=value' (or 'variable:=value') into
		its components and process it.
		All components have already been converted to lower case.
*/

static int cmdarg(char *curarg) 	/* process a single argument */
{
	char	variable[21];			/* variable name goes here */
	char	value[141];			/* variable value goes here*/
	int	numval; 			/* numeric value of arg    */
	char	charval;			/* character value of arg  */

	double	atof(), ftemp;			/* floating point stuff    */
	int	i, j, k, l;			/* temporary loop counters */
	char	*slash; 			/* temporary string ptr    */

	strlwr(curarg); 			/* using lower case	  */
	for (j = 1; j < strlen(curarg) && curarg[j] != '='; j++) ;
	if (j > 20 || j >= strlen(curarg))
		argerror(curarg);		/* oops.  '=' not found   */

	strncpy(variable,curarg,j);		/* get the variable name  */
	variable[j] = 0;			/* truncate it		  */
	if (j > 1 && variable[j-1] == ':')      /* strip any trailing ':' */
		variable[j-1] = 0;
	strcpy(value,&curarg[j+1]);		/* get the value string   */
	numval = atoi(value);			/* get any numeric value  */
	charval = value[0];			/* get any letter  value  */

	if (strcmp(variable,"filename") == 0) {         /* filename=?   */
		strcpy(readname,value); 		/* set up filename */
		showfile = 0;
		return(0);
		}
	if( strcmp(variable, "map") == 0 ) {    /* map option */
		if (SetColorPaletteName(value) != 0)
			argerror(curarg);
		mapset = 1;
		strcpy(MAP_name,value);
		return(0);
		}
	if (strcmp(variable,"batch") == 0 ) {           /* batch=?      */
		if (charval == 'c') {                   /* config run   */
			makeconfig();
			goodbye();
			}
		if (charval == 'y'){                    /* batch = yes  */
			initbatch = 1;
			return(0);
			}
		if (charval == 'n'){
			initbatch = 0;
			return(0);
			}
		}
    /* keep this for backward compatibility */
	if (strcmp(variable,"warn") == 0 ) {            /* warn=?       */
		if (charval == 'y')
			overwrite = 0;
		else if (charval == 'n')
			overwrite = 1;
	else
		    argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"overwrite") == 0 ) {            /* overwrite=?       */
		if (charval == 'n')
			overwrite = 0;
		else if (charval == 'y')
			overwrite = 1;
	else
		    argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"gif87a") == 0 ) {          /* gif87a=?     */
		if (charval == 'y')
			gif87a_flag = 1;
		return(0);
		}
	if (strcmp(variable,"savetime") == 0) {         /* savetime=?   */
		initsavetime = numval;
		return(0);
		}
	if (strcmp(variable,"type") == 0 ) {            /* type=?       */
		if (value[strlen(value)-1] == '*')
			value[strlen(value)-1] = 0;
		for (k = 0; fractalspecific[k].name != NULL; k++)
			if (strcmp(value,fractalspecific[k].name) == 0)
				break;
		if (fractalspecific[k].name == NULL) argerror(curarg);
		fractype = k;
		if (initcorners == 0) {
			xx3rd = xxmin = fractalspecific[fractype].xmin;
			xxmax	      = fractalspecific[fractype].xmax;
			yy3rd = yymin = fractalspecific[fractype].ymin;
			yymax	      = fractalspecific[fractype].ymax;
			}
		return(0);
		}
	if (strcmp(variable,"inside") == 0 ) {          /* inside=?     */
		if(strcmp(value,"bof60")==0)
			inside = -60;
		else if(strcmp(value,"bof61")==0)
			inside = -61;
		else if(strcmp(value,"maxiter")==0)
			inside = -1;
		else if(isalpha(*value))
			argerror(curarg);
		else
			inside = numval;
		return(0);
		}
	if (strcmp(variable,"finattract") == 0 ) {      /* finattract=? */
		if (charval == 'y')
			finattract = 1;
		return(0);
		}

	if (strcmp(variable,"function") == 0) {           /* function=?,?   */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 4) {
		    slash++;
		    if(set_trig_array(k++,slash)) {
		       argerror(curarg);
		       return(0);
		       }
		    if ((slash = strchr(slash,'/')) == NULL) k = 99;
		    }
		return(0);
		}
	if (strcmp(variable,"outside") == 0 ) {         /* outside=?    */
		if(numval < -1 || numval > 255)
		    argerror(curarg);
		else
		    outside = numval;
		return(0);
		}
	if (strcmp(variable,"maxiter") == 0) {          /* maxiter=?    */
		if (numval < 2 || numval > 32767) argerror(curarg);
		maxit = numval;
		return(0);
		}
	if (strcmp(variable,"iterincr") == 0) {         /* iterincr=?   */
	    /*	if (numval <= 0 || numval > 32000) argerror(curarg);
		initincr = numval;
	     */
		return(0);
		}
	if (strcmp(variable,"passes") == 0) {           /* passes=?     */
		if ( charval != '1' && charval != '2'
		  && charval != 'g' && charval != 'b')
		   argerror(curarg);
		usr_stdcalcmode = charval;
		return(0);
		}
	if (strcmp(variable,"cyclelimit") == 0 ) {      /* cyclelimit=? */
		if (numval > 1 && numval <= 256)
			initcyclelimit = numval;
		return(0);
		}
	if (strcmp(variable,"savename") == 0) {         /* savename=?   */
		if (first_init)
			strcpy(savename,value);
		return(0);
		}
	if (strcmp(variable,"video") == 0) {            /* video=?      */
		if (active_system != 0)
			return(0);
		for (k = 0; k < maxvideomode; k++) {
			strcpy(variable,fkeys[k]);
			strlwr(variable);
			if (strcmp(variable, value) == 0)
				break;
			}
		if (k == maxvideomode) argerror(curarg);
		initmode = k;
		return(0);
		}
	if (strcmp(variable,"adapter") == 0 ) {         /* adapter==?     */
		slash = strchr(curarg,'=');
		video_type = 5; 			/* assume video=vga */
		if (strcmp(&slash[1],"egamono") == 0) {
			video_type = 3;
			mode7text = 1;
			}
		else if (charval == 'h') {              /* video = hgc */
			video_type = 1;
			mode7text = 1;
			}
		else if (charval == 'e')                /* video = ega */
			video_type = 3;
		else if (charval == 'c')                /* video = cga */
			video_type = 2;
		else if (charval == 'm')                /* video = mcga */
			video_type = 4;
		return(0);
		}

	if (strcmp(variable,"textsafe") == 0 ) {        /* textsafe==?     */
		if (first_init) {
		    if (charval == 'n') /* no */
			textsafe = 2;
		    else if (charval == 'y') /* yes */
			textsafe = 1;
		    else if (charval == 'b') /* bios */
			textsafe = 3;
		    else if (charval == 's') /* save */
			textsafe = 4;
		    else
			argerror(curarg);
		    }
		return(0);
		}
	if (strcmp(variable,"exitmode") == 0) {         /* exitmode=?   */
		sscanf(value,"%x",&numval);
		exitmode = numval;
		return(0);
		}
	if (strcmp(variable,"textcolors") == 0) {
		slash = strchr(curarg,'=');
		if (strcmp(&slash[1],"mono") == 0) {
		   for (k = 0; k < sizeof(textcolor); ++k)
		      textcolor[k] = BLACK*16+WHITE;
		/* C_PROMPT_INPUT = C_CHOICE_CURRENT = C_GENERAL_INPUT
			   = C_AUTHDIV1 = C_AUTHDIV2 = WHITE*16+BLACK; */
		   textcolor[9] = textcolor[10] = textcolor[16]
			   = textcolor[23] = textcolor[24] = WHITE*16+BLACK;
		/* C_TITLE = C_HELP_HDG = C_PROMPT_HI = C_CHOICE_SP_KEYIN
			   = C_GENERAL_HI = C_DVID_HI = C_STOP_ERR
			   = C_STOP_INFO = BLACK*16+L_WHITE; */
		   textcolor[0] = textcolor[2] = textcolor[8] = textcolor[12]
			   = textcolor[13] = textcolor[18] = textcolor[20]
			   = textcolor[21] = BLACK*16+L_WHITE;
		   }
		else {
		   k = 0;
		   while ( k < sizeof(textcolor)) {
		      if (slash[1] && slash[1] != '/') {
			 sscanf(&slash[1],"%x",&numval);
			 i = (numval / 16) & 7;
			 j = numval & 15;
			 if (i == j || (i == 0 && j == 8)) /* force contrast */
			    j = 15;
			 textcolor[k] = i * 16 + j;
			 }
		      ++k;
		      if ((slash = strchr(&slash[1],'/')) == NULL) k = 99;
		      }
		   }
		return(0);
		}
	if (strcmp(variable,"potential") == 0) {        /* potential=?  */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 3) {
			potparam[k++] = atoi(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		pot16bit = 0;
		if (k < 99) {
			if (strcmp(++slash,"16bit"))
				argerror(curarg);
			pot16bit = 1;
			}
		return(0);
		}
	if (strcmp(variable,"params") == 0) {           /* params=?,?   */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 4) {
			param[k++] = atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		return(0);
		}
	if (strcmp(variable,"initorbit") == 0) {           /* initorbit=?,?   */
		k = 0;
		slash = strchr(curarg,'=');
		if(strcmp(slash+1,"pixel")==0)
		{
		   useinitorbit = 2;
		   return(0);
		}
		while ( k < 2) {
			initorbit[k++] = atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		useinitorbit = 1;
		return(0);
		}
	if (strcmp(variable,"corners") == 0) {          /* corners=?,?,?,? */
		initcorners = 1;
		slash = strchr(curarg,'=');
		xx3rd=xxmin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		xxmax=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		yy3rd=yymin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		yymax=atof(++slash);
		if ((slash = strchr(slash,'/')) != NULL) {
			xx3rd=atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
			yy3rd=atof(++slash);
			}
		return(0);
		}
	if (strcmp(variable,"center-mag") == 0) {          /* center-mag=?,?,? */
		double Xctr, Yctr,Magnification,Ratio,Height, Width,Radius;
		usemag = 1;
		slash = strchr(curarg,'=');
		Xctr=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL)
		   return(0);
		initcorners = 1;
		Yctr=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		if((Magnification=atof(++slash))<=0.0)
		    argerror(curarg);
		else
		    Radius = 1.0 / Magnification;

		if ((slash = strchr(slash,'/')) != NULL) argerror(curarg);
		Ratio = .75;	/* inverse aspect ratio of screen  */

		/* calculate bounds */
		Height = 2.0 * Radius;
		Width = Height / Ratio;
		yymax = Yctr + Radius;
		yy3rd = yymin = Yctr - Radius;
		xxmax = Xctr + Width / 2.0;
		xx3rd = xxmin = Xctr - Width / 2.0;
		return(0);
		}
	if (strcmp(variable,"3d") == 0) {               /* 3d=?/?/..    */
		display3d = 1;				/* turn on 3D */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 20) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') init3d[k] = l;
			/* reset sphere defaults, done here cause [0]==sphere */
			if (k == 0)
				set_3d_defaults();
			if ((slash = strchr(slash,'/')) == NULL) break;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"sphere") == 0 ) {          /* sphere=?     */
		if (charval == 'y')
			SPHERE	  = TRUE;
		else if (charval == 'n')
			SPHERE	  = FALSE;
		else
			argerror(curarg);		/* oops.  error. */
		return(0);
		}
	if (strcmp(variable,"rotation") == 0) {         /* rotation=?/?/?       */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XROT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"scalexyz") == 0) {         /* scalexyz=?/?/?       */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XSCALE+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	/* "rough" is really scale z, but we add it here for convenience */
	if (strcmp(variable,"roughness") == 0) {        /* roughness=?  */
		ROUGH = numval;
		return(0);
		}
	if (strcmp(variable,"waterline") == 0) {        /* waterline=?  */
		if (numval<0) argerror(curarg);
		WATERLINE = numval;
		return(0);
		}
	if (strcmp(variable,"filltype") == 0) {         /* filltype=?   */
		if (numval < -1 || numval > 6) argerror(curarg);
		FILLTYPE = numval;
		return(0);
		}
	if (strcmp(variable,"perspective") == 0) {      /* perspective=?        */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 1) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&ZVIEWER+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"xyshift") == 0) {          /* xyshift=?/?  */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XSHIFT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"lightsource") == 0) {      /* lightsource=?/?/?    */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XLIGHT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"smoothing") == 0) {        /* smoothing=?  */
		if (numval<0) argerror(curarg);
		LIGHTAVG = numval;
		return(0);
		}
	if (strcmp(variable,"latitude") == 0) {         /* latitude=?/? */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&THETA1+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"longitude") == 0) {        /* longitude=?/?        */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&PHI1+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return(0);
		}
	if (strcmp(variable,"radius") == 0) {           /* radius=?     */
		if (numval<0) argerror(curarg);
		RADIUS = numval;
		return(0);
		}
	if (strcmp(variable,"invert") == 0) {           /* invert=?,?,? */
		k = 0;
		slash = strchr(curarg,'=');
		while ( k < 3) {
			inversion[k++] = atof(++slash);
			if(inversion[0] != 0.0)
				invert = k; /* record highest inversion parameter set */
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		return(0);
		}
	if (strcmp(variable,"askvideo") == 0 ) {        /* askvideo=?   */
		if (charval == 'y')
			askvideo = 1;
		else if (charval == 'n')
			askvideo = 0;
		else
			argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"ramvideo") == 0 ) {        /* ramvideo=?   */
		return(0); /* just ignore and return, for old time's sake */
		}
	if (strcmp(variable,"float") == 0 ) {           /* float=?      */
		if (charval == 'y')
			usr_floatflag = 1;
		else if (charval == 'n')
			usr_floatflag = 0;
		else
			argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"biomorph") == 0 ) {        /* biomorph=?   */
		biomorph = numval;
		return(0);
		}
	if (strcmp(variable,"bailout") == 0 ) {         /* bailout=?    */
		if (numval < 4 || numval > 32000) argerror(curarg);
		bailout = numval;
		return(0);
		}
	if (strcmp(variable,"symmetry") == 0 ) {        /* symmetry=?   */
		if     (strcmp(value,"xaxis" )==0) forcesymmetry = XAXIS;
		else if(strcmp(value,"yaxis" )==0) forcesymmetry = YAXIS;
		else if(strcmp(value,"xyaxis")==0) forcesymmetry = XYAXIS;
		else if(strcmp(value,"origin")==0) forcesymmetry = ORIGIN;
		else if(strcmp(value,"pi"    )==0) forcesymmetry = PI_SYM;
		else if(strcmp(value,"none"  )==0) forcesymmetry = NOSYM;
		else argerror(curarg);
		return(0);
		}

	if (strcmp(variable,"printer") == 0 ) { /* printer=?    */
		if (charval=='h' && value[1]=='p')
		    Printer_Type=1;		  /* HP LaserJet	    */
		if (charval=='i' && value[1]=='b')
		    Printer_Type=2;		  /* IBM Graphics	    */
		if (charval=='e' && value[1]=='p')
		    Printer_Type=2;		  /* Epson (model?)	    */
		if (charval=='c' && value[1]=='o')
		    Printer_Type=3;		  /* Star (Epson-Comp?) color */
		if (charval=='p' && value[1]=='a')
		    Printer_Type=4;		  /* HP Paintjet (color)    */
		if (charval=='p' && (value[1]=='o' || value[1]=='s'))
		    {				  /* PostScript  SWT */
		    Printer_Type=5;
		    slash=strchr(curarg,'=');
		    if ((slash=strchr(slash,'/'))==NULL)
		       slash = &curarg[strlen(curarg)];
		    slash--;
		    if (*slash=='h' || *slash=='l')
		       Printer_Type=6;
		    }

		if (Printer_Type == 1)		/* assume low resolution */
			Printer_Resolution = 75;
		else
			Printer_Resolution = 60;

		if (EPSFileType > 0)	       /* EPS save - force type 5 */
			Printer_Type = 5;

		if ((Printer_Type == 5) || (Printer_Type == 6))
			Printer_Resolution = 150; /* PostScript def. res. */

		slash=strchr(curarg,'=');
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) > 0) Printer_Resolution=k;
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash))> 0) LPTNumber = k;
		if (k < 0) Print_To_File = 1;
		return(0);
		}

	if (strcmp(variable,"printfile") == 0) {   /* print-to-file? SWT */
		Print_To_File = 1;
		strcpy(PrintName,value);
		return(0);
		}

	if (strcmp(variable,"epsf") == 0) {   /* EPS type? SWT */
		Print_To_File = 1;
		EPSFileType = numval;
		Printer_Type = 5;
		if (strcmp(PrintName,"fract001.prn")==0)
			strcpy(PrintName,"fract001.eps");
		return(0);
		}

	if (strcmp(variable,"title") == 0) {   /* Printer title block? SWT */
		if (charval=='y') Printer_Titleblock=1;
		return(0);
		}

	if (strcmp(variable,"translate") == 0) {   /* Translate color? SWT */
		if (charval=='y')
		  {
		  Printer_ColorXlat=1;
		  return(0);
		  }
		if ((numval>1) || (numval<-1))
		  {
		  Printer_ColorXlat=numval;
		  return(0);
		  }
		Printer_ColorXlat=0;
		return(0);
		}

	if (strcmp(variable,"halftone") == 0) {   /* New halftoning? SWT */
		Printer_SetScreen=1;
		slash = strchr(curarg,'=');
		if ((k=atoi(++slash))>0) Printer_SFrequency=k;
		else Printer_SFrequency=80;
		if ((slash=strchr(slash,'/')) == NULL)
		  {
		  Printer_SAngle=45;
		  Printer_SStyle=0;
		  return(0);
		  }
		Printer_SAngle=atoi(++slash);
		if ((slash=strchr(slash,'/')) == NULL)
		  {
		  Printer_SStyle=0;
		  return(0);
		  }
		Printer_SStyle=atoi(++slash);
		return(0);
		}

	if (strcmp(variable,"comport") == 0 ) /* Set the COM parameters */
	{
		slash=strchr(curarg,'=');
		if ((slash=strchr(slash,'/')) == NULL) return(-1);
		switch (atoi(++slash))
		  {
		  case 110:
			l = 0;
			break;
		  case 150:
			l = 32;
			break;
		  case 300:
			l = 64;
			break;
		  case 600:
			l = 96;
			break;
		  case 1200:
			l = 128;
			break;
		  case 2400:
			l = 160;
			break;
		  case 4800:
			l = 192;
			break;
		  case 9600:
		  default:
			l = 224;
			break;
		  }
		if ((slash=strchr(slash,'/')) == NULL) return(-1);
		for (k=0; k < strlen(slash); k++)
		  {
		  switch (slash[k])
		    {
		    case '7':
			l |= 2;
			break;
		    case '8':
			l |= 3;
			break;
		    /* No parity = do nothing */
		    case 'o':
		    case 'O':
			l |= 8;
			break;
		    case 'e':
		    case 'E':
			l |= 24;
			break;
		    /* 1 stop bit = do nothing */
		    case '2':
			l |= 4;
			break;
		    }
		  }
#ifdef __TURBOC__
		bioscom(0,l,numval-1);
#else
		_bios_serialcom(0,numval-1,l);
#endif
		return(0);
	}


	if (strcmp(variable,"transparent") == 0) { /* transparent? */
		slash = strchr(curarg,'=');
		if ((k=atoi(++slash)) > 0) transparent[0] = k;
		transparent[1] = transparent[0];
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) > 0) transparent[1] = k;
		return(0);
		}
	if (strcmp(variable,"sound") == 0 ) {           /* sound=?      */
		soundflag = 0;				/* sound is off */
		if (strncmp(value,"ye",2) == 0)
			soundflag = -1; 		/* sound is on	*/
		if (charval == 'x')
			soundflag = 1;
		if (charval == 'y')
			soundflag = 2;
		if (charval == 'z')
			soundflag = 3;
		return(0);
		}

	if (strcmp(variable,"hertz") == 0) {   /* Hertz=? */
		if (numval < 200 || numval > 10000) argerror(curarg);
		basehertz = numval;
		return(0);
		}

	if (strcmp(variable,"periodicity") == 0 ) {     /* periodicity=?    */
		usr_periodicitycheck=1;
		if (charval == 'n')
			usr_periodicitycheck=0;
		else if (charval == 'y')
			usr_periodicitycheck=1;
		else if (charval == 's')   /* 's' for 'show' */
			usr_periodicitycheck=-1;
		else if(isalpha(*value))
			argerror(curarg);
		else if(numval != 0)
			usr_periodicitycheck=numval;
		return(0);
		}

	if (strcmp(variable,"logmap") == 0 ) {          /* logmap=?     */
		if (charval == 'y')
			LogFlag = 1;			/* palette is logarithmic */
		else if (charval == 'n')
			LogFlag = 0;
		else if (charval == 'o')
			LogFlag = -1;			/* old log palette */
		else
			LogFlag = numval;
		return(0);
		}
	if (strcmp(variable,"debugflag") == 0 ||
		 strcmp(variable,"debug") == 0) {       /* internal use only */
		debugflag = numval;
		timerflag = debugflag & 1;		/* separate timer flag */
		debugflag -= timerflag;
		return(0);
		}
	if (strcmp(variable,"ifs") == 0) {              /* ifs=?        */
		strcpy(ifsfilename,value);
		if (strchr(value,'.') == NULL)
			strcat(ifsfilename,".ifs");
		ifsgetfile();
		return(0);
		}
	if (strcmp(variable,"ifs3d") == 0) {            /* ifs3d=?      */
		strcpy(ifs3dfilename,value);
		if (strchr(value,'.') == NULL)
			strcat(ifs3dfilename,".ifs");
		ifs3dgetfile();
		return(0);
		}
	if (strcmp(variable,"ifscodes") == 0) {    /* ifscodes=?,?,?,? */
		int ifsindex;
		slash = strchr(curarg,'=');
		ifsindex=atoi(++slash) - 1;
		if(ifsindex < 0 || ifsindex > NUMIFS) argerror(curarg);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][0]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][1]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][2]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][3]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][4]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][5]=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(curarg);
		initifs[ifsindex][6]=atof(++slash);
		return(0);
		}
	if (strcmp(variable, "rseed") == 0) {
		rseed = numval;
		rflag = 1;
		return(0);
		}
	if (strcmp(variable, "decomp") == 0) {
		k = 0;
		slash = strchr(curarg,'=');
		while (k < 2) {
			decomp[k++] = atoi(++slash);
			if ((slash = strchr(slash,'/')) == NULL) break;
			if (k == 2) /* backward compatibility */
				bailout = decomp[1];
			}
		return(0);
		}
	if (strcmp(variable, "distest") == 0) {
		usr_distest = numval;
		return(0);
		}
	if (strcmp(variable,"formulafile") == 0) {      /* formulafile=?        */
		strcpy(FormFileName,value);
		return(0);
		}
	if (strcmp(variable,"formulaname") == 0) {      /* formulaname=?        */
		strcpy(FormName,value);
		return(0);
		}
	if (strcmp(variable,"lfile") == 0) {
		strcpy(LFileName,value);
		return(0);
	}
	if (strcmp(variable,"lname") == 0) {
		strcpy(LName,value);
		return(0);
	 }

	if (strcmp(variable,"preview") == 0) { /* preview? */
		if (charval == 'y')
			preview = 1;
		return(0);
		}
	if (strcmp(variable,"showbox") == 0) { /* showbox? */
		if (charval == 'y')
		showbox = 1;
		return(0);
		}
	if (strcmp(variable,"coarse") == 0) {  /* coarse=? */
		if (numval<1) argerror(curarg);
		previewfactor = numval;
		return(0);
		}
	if (strcmp(variable,"stereo") == 0) {  /* stereo=? */
		if ((numval<0) || (numval>3)) argerror(curarg);
		glassestype = numval;
		return(0);
		}
	if (strcmp(variable,"interocular") == 0) {  /* interocular=? */
		eyeseparation = numval;
		return(0);
		}
	if (strcmp(variable,"converge") == 0) {  /* converg=? */
		xadjust = numval;
		return(0);
		}
	if (strcmp(variable,"crop") == 0) {  /* crop=? */
		slash = strchr(curarg,'=');
		if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) red_crop_left = k;
		else argerror(curarg);
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) red_crop_right = k;
		else argerror(curarg);
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) blue_crop_left = k;
		else argerror(curarg);
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) blue_crop_right = k;
		else argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"bright") == 0) {  /* bright=? */
		slash = strchr(curarg,'=');
		if ((k=atoi(++slash)) >= 0) red_bright = k;
		else argerror(curarg);
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		if ((k=atoi(++slash)) >= 0) blue_bright = k;
		else argerror(curarg);
		return(0);
		}
	if (strcmp(variable,"xyadjust") == 0) { /* trans=? */
		slash = strchr(curarg,'=');
		xtrans=atoi(++slash);
		if ((slash=strchr(slash,'/')) == NULL) return(0);
		ytrans=atoi(++slash);
		return(0);
		}
	if (strcmp(variable,"randomize") == 0) {  /* RANDOMIZE=? */
		if (numval<0 || numval>7) argerror(curarg);
		RANDOMIZE = numval;
		return(0);
		}
	if (strcmp(variable,"ambient") == 0) {  /* ambient=? */
		if (numval<0||numval>100) argerror(curarg);
		Ambient = numval;
		return(0);
		}
	if (strcmp(variable,"haze") == 0) {  /* haze=? */
		if (numval<0||numval>100) argerror(curarg);
		haze = numval;
		return(0);
		}
	if (strcmp(variable,"fullcolor") == 0) {  /* fullcolor=? */
		if (charval != 'y' && charval != 'n') argerror(curarg);
		if (charval == 'y') full_color = 1;
		return(0);
		}
	if (strcmp(variable,"lightname") == 0) {         /* lightname=?   */
		if (first_init)
			strcpy(light_name,value);
		return(0);
		}

	argerror(curarg);
	return(-1);
}

static void argerror(char *badarg)	/* oops. couldn't decode this */
{
   static char far argerrormessage[]={"\
(see the Startup Help screens or documentation for a complete\n\
 argument list with descriptions)"};
   char msg[200];
   setvideomode(3,0,0,0);
   sprintf(msg,"Oops.  I couldn't understand the argument:\n  '%s'",badarg);
   if (active_system == 0) {   /* DOS version */
       buzzer(2);
       putstring(0,0,15,msg);
       putstring(3,0,7,argerrormessage);
       movecursor(8,0);
       exit(1);
       }
   else
       stopmsg(0,msg);	      /* Windows version */
}

void set_3d_defaults()
{
   ENTER_OVLY(OVLY_CMDFILES);
   if(SPHERE) {
      PHI1	=  180;
      PHI2	=  0;
      THETA1	=  -90;
      THETA2	=  90;
      RADIUS	=  100;
      ROUGH	=  30;
      WATERLINE = 0;
      FILLTYPE	= 2;
      ZVIEWER	= 0;
      XSHIFT	= 0;
      YSHIFT	= 0;
      xtrans	= 0;
      ytrans	= 0;
      XLIGHT	= 1;
      YLIGHT	= 1;
      ZLIGHT	= 1;
      LIGHTAVG	= 0;
      Ambient	= 20;
      RANDOMIZE = 0;
      haze	= 0;
      full_color= 0;
      back_color[0] = 51; back_color[1] = 153; back_color[2] = 200;
      }
   else {
      XROT	= 60;
      YROT	= 30;
      ZROT	= 0;
      XSCALE	= 90;
      YSCALE	= 90;
      ROUGH	= 30;
      WATERLINE = 0;
      FILLTYPE	= 0;
      if (active_system != 0)
	 FILLTYPE = 2;
      ZVIEWER	= 0;
      XSHIFT	= 0;
      YSHIFT	= 0;
      xtrans	= 0;
      ytrans	= 0;
      XLIGHT	= 1;
      YLIGHT	= -1;
      ZLIGHT	= 1;
      LIGHTAVG	= 0;
      Ambient	= 20;
      RANDOMIZE = 0;
      haze	= 0;
      full_color= 0;
      back_color[0] = 51; back_color[1] = 153; back_color[2] = 200;
      }
    EXIT_OVLY;
}

int readconfig()	/* search for, read, decode fractint.cfg file */
{
   char tempstring[101];
   FILE *cfgfile;
   int count, i, j, ax, bx, cx, dx, dotmode, xdots, ydots, colors, commas[10];
   int textsafe2;
   ENTER_OVLY(OVLY_CMDFILES);
   findpath("fractint.cfg",tempstring); /* look for FRACTINT.CFG */
   if (tempstring[0] == 0			     /* can't find the file */
     || (cfgfile = fopen(tempstring,"r")) == NULL) { /* can't open it */
      EXIT_OVLY;
      return(-1);
      }

   count = 0;				/* build a new videomode file */
   while (feof(cfgfile) == 0 && count < MAXVIDEOMODES) {   /* scan through strings */
      if (!fgets(tempstring, 100, cfgfile)) break;
      tempstring[strlen(tempstring)-1] = 0;
      if (tempstring[0] <= 32) continue;	/* comment line 	*/
      j = 9;
      for (i = 0; i <= j; i++) commas[i] = 0;
      for (i = strlen(tempstring); i >= 0 && j >= 0; i--)  /* check for commas */
	 if (tempstring[i] == ',') {
	    tempstring[i] = 0;
	    commas[--j] = i+1;
	    }
      sscanf(&tempstring[commas[0]],"%x",&ax);
      sscanf(&tempstring[commas[1]],"%x",&bx);
      sscanf(&tempstring[commas[2]],"%x",&cx);
      sscanf(&tempstring[commas[3]],"%x",&dx);
      dotmode	  = atoi(&tempstring[commas[4]]);
      xdots	  = atoi(&tempstring[commas[5]]);
      ydots	  = atoi(&tempstring[commas[6]]);
      colors	  = atoi(&tempstring[commas[7]]);
      textsafe2   = dotmode / 100;
      dotmode	 %= 100;
      if (  i >= 0 || j != 0 ||
	    dotmode < 0 || dotmode > 30 ||
	    textsafe2 < 0 || textsafe2 > 4 ||
	    xdots < 160 || xdots > 2048 ||
	    ydots < 160 || ydots > 2048 ||
	    (colors != 2 && colors != 4 && colors != 16 && colors != 256)
	   ) {
	 buzzer(2);
	 printf("\n\n There is a bad entry in fractint.cfg\n\n");
	 printf(" ==> %s \n", tempstring);
	 exit(-1);
	 }
      tempstring[commas[8]+26] = 0;
      if (commas[0] >= 25) tempstring[25] = 0;
      strcpy(videoentry.name,	 tempstring);
      strcpy(videoentry.comment, &tempstring[commas[8]]);
      if (tempstring[commas[8]] == ' ')
	 strcpy(videoentry.comment, &tempstring[commas[8]+1]);
      videoentry.videomodeax =	 ax;
      videoentry.videomodebx =	 bx;
      videoentry.videomodecx =	 cx;
      videoentry.videomodedx =	 dx;
      videoentry.dotmode     =	 textsafe2 * 100 + dotmode;
      videoentry.xdots	     =	 xdots;
      videoentry.ydots	     =	 ydots;
      videoentry.colors      =	 colors;
      tovideotable(count);
      count++;
      }

   if (count > 0) strcpy(videoentry.name, "END");
   videoentry.colors = 0;
   tovideotable(count);
   fclose(cfgfile);
   EXIT_OVLY;
   if (count <= 0) return(-1);
   return(0);			/* successful return	*/
}

static void makeconfig()	/* routine to build a FRACTINT.CFG file */
{
   char tempstring[101];
   FILE *cfgfile;
   int count;

   if (active_system != 0)  /* non-DOS version */
       return;

#ifdef __TURBOC__
   strcpy(tempstring,searchpath("fractint.cfg"));
#else
   _searchenv("fractint.cfg","PATH",tempstring);
#endif

   if (tempstring[0] != 0) {
      buzzer(2);
      printf("\n There is a FRACTINT.CFG file already located in the PATH.\n\n");
      printf(" I won't make another one until after you manually remove\n");
      printf(" the old one.  Safety first!\n\n");
      exit(-1);
      }

   if ((cfgfile = fopen("fractint.cfg","w")) == NULL)
      exit(-1); 			      /* ?? can't open file   */
   fprintf(cfgfile,"   Full FRACTINT.CFG File, built by a 'fractint batch=config' command\n\n");
   fprintf(cfgfile," name of adapter/mode    | AX | BX | CX | DX |mode|  x |  y |clrs| comments\n");
   fprintf(cfgfile," =============================================================================\n\n");
   for (count = 0; count < maxvideomode; count++) {	/* write the entries */
      fromvideotable(count);
#ifdef __TURBOC__
      fprintf(cfgfile,"%-25.25s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%4d, %-25.25s\n",
#else
      fprintf(cfgfile,"%-25s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%4d, %-25s\n",
#endif
	      videoentry.name,
	      videoentry.videomodeax,
	      videoentry.videomodebx,
	      videoentry.videomodecx,
	      videoentry.videomodedx,
	      videoentry.dotmode,
	      videoentry.xdots,
	      videoentry.ydots,
	      videoentry.colors,
	      videoentry.comment);
      }
   fclose(cfgfile);
}

