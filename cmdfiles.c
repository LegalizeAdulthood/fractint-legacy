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
int  load_commands(FILE *);
void set_3d_defaults(void);

static int  cmdfile(FILE *,int);
static int  next_command(char *,int,FILE *,char *,int *,int);
static int  next_line(FILE *,char *,int);
static int  cmdarg(char *,int);
static void argerror(char *);
static void initvars_run(void);
static void initvars_restart(void);
static void initvars_fractal(void);
static void initvars_3d(void);
static void reset_ifs_defn(void);
static void parse_textcolors(char *value);
static int  parse_colors(char *value);
static int  parse_printer(char *value);

extern int  makedoc_msg_func(int,int);

/* variables defined by the command line/files processor */
int		showdot;	/* color to show crawling graphics cursor */
char	temp1[256];		/* temporary strings	    */
char	readname[80];		/* name of fractal input file */
char	gifmask[13] = {""};
char	PrintName[80]={"fract001.prn"}; /* Name for print-to-file */
char	savename[80]={"fract001"};      /* save files using this name */
char	autoname[80]={"auto.key"};      /* record auto keystrokes here */
int	potflag=0;		/* continuous potential enabled? */
int	pot16bit;		/* store 16 bit continuous potential values */
int	gif87a_flag;		/* 1 if GIF87a format, 0 otherwise */
int	askvideo;		/* flag for video prompting */
char	floatflag;
int	biomorph;		/* flag for biomorph */
int	usr_biomorph;
int	forcesymmetry;		/* force symmetry */
int	showfile;		/* zero if file display pending */
int	rflag, rseed;		/* Random number seeding flag and value */
int	decomp[2];		/* Decomposition coloring */
int	distest;
int	distestwidth;
char overwrite = 0;	/* 0 if file overwrite not allowed */
int	soundflag;		/* 0 if sound is off, 1 if on */
int	basehertz;		/* sound=x/y/x hertz value */
int	debugflag;		/* internal use only - you didn't see this */
int	timerflag;		/* you didn't see this, either */
int	cyclelimit;		/* color-rotator upper limit */
int	inside; 		/* inside color: 1=blue     */
int	fillcolor; 		/* fillcolor: -1=normal     */
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
int	rotate_lo,rotate_hi;	/* cycling color range	    */
int far *ranges;		/* iter->color ranges mapping */
int	rangeslen = 0;		/* size of ranges array     */
char far *mapdacbox = NULL;	/* map= (default colors)    */
int	colorstate;		/* 0, dacbox matches default (bios or map=) */
				/* 1, dacbox matches no known defined map   */
				/* 2, dacbox matches the colorfile map	    */
int	colorpreloaded; 	/* if dacbox preloaded for next mode select */
extern int Targa_Overlay;
int Targa_Out = 0;
char	colorfile[80];		/* from last <l> <s> or colors=@filename    */

/* TARGA+ variables */
int	TPlusFlag;		/* Use the TARGA+ if found  */
int	MaxColorRes;		/* Default Color Resolution if available */
int	PixelZoom;		/* TPlus Zoom Level */
int	NonInterlaced;		/* Non-Interlaced video flag */

/* 3D Transparency Variables, MCP 5-30-91 */
double xcoord, ycoord, zcoord, tcoord;
double zzmin, zzmax;		/* initial depth corner values */
double ttmin, ttmax;		/* initial time coordinates */
int Transparent3D, SolidCore, MultiDrawing;
int tpdepth, tptime;
unsigned CoreRed, CoreGreen, CoreBlue, NumFrames;

/* AntiAliasing variables, MCP 6-6-91 */
int AntiAliasing, Shadowing;

int	orbitsave = 0;		/* for IFS and LORENZ to output acrospin file */
int orbit_delay;                /* clock ticks delating orbit release */
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
extern char light_name[];
extern int BRIEF;
extern int RAY;

extern unsigned char back_color[];
extern unsigned char dacbox[256][3];
extern struct videoinfo far videotable[];
extern int fpu;
extern int iit;

extern double potparam[];	/* potential parameters  */
extern int Printer_Resolution, LPTNumber,
	   Printer_Type, Printer_Titleblock,
	   Printer_ColorXlat, Printer_SetScreen,
	   Printer_SFrequency, Printer_SAngle, Printer_SStyle,
	   Printer_RFrequency, Printer_RAngle, Printer_RStyle,
	   Printer_GFrequency, Printer_GAngle, Printer_GStyle,
	   Printer_BFrequency, Printer_BAngle, Printer_BStyle,
	   EPSFileType, ColorPS,
	   Print_To_File,Printer_CRLF;		/* for printer functions */

int	transparent[2]; 	/* transparency min/max values */
int	LogFlag;		/* Logarithmic palette flag: 0 = no */

unsigned char exitmode = 3;	/* video mode on exit */

extern int video_type;
extern int svga_type;           /* for forcing a specific SVGA adapter */
extern int mode7text;
extern int textsafe;
extern int vesa_detect;

int        bios_palette;        /* set to 1 to force BIOS palette updates */
int        escape_exit;         /* set to 1 to avoid the "are you sure?" screen */

extern int   viewwindow;
extern float viewreduction;
extern int   viewcrop;
extern float finalaspectratio;
extern int   viewxdots,viewydots;

extern char MAP_name[];
extern int mapset;

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

int first_init=1;		/* first time into cmdfiles? */
static int init_rseed;
static char initcorners,initparams;
struct fractalspecificstuff far *curfractalspecific;

char FormFileName[80];		/* file to find (type=)formulas in */
char FormName[ITEMNAMELEN+1];	/* Name of the Formula (if not null) */
char LFileName[80];		/* file to find (type=)L-System's in */
char LName[ITEMNAMELEN+1];	/* Name of L-System */
char CommandFile[80];		/* file to find command sets in */
char CommandName[ITEMNAMELEN+1];/* Name of Command set */
char CommandComment1[57];	/* comments for command set */
char CommandComment2[57];
char IFSFileName[80];		/* file to find (type=)IFS in */
char IFSName[ITEMNAMELEN+1];	/* Name of the IFS def'n (if not null) */
float far *ifs_defn = NULL;	/* ifs parameters */
int  ifs_changed;		/* nonzero if parameters have been edited */
int  ifs_type;			/* 0=2d, 1=3d */
int  slides = 0;		/* 1 autokey=play, 2 autokey=record */

unsigned char txtcolor[]={
      BLUE*16+L_WHITE,	  /* C_TITLE	       title background */
      BLUE*16+L_GREEN,	  /* C_TITLE_DEV       development vsn foreground */
      GREEN*16+YELLOW,	  /* C_HELP_HDG        help page title line */
      WHITE*16+BLACK,	  /* C_HELP_BODY       help page body */
      GREEN*16+GRAY,	  /* C_HELP_INSTR      help page instr at bottom */
      WHITE*16+BLUE,	  /* C_HELP_LINK       help page links */
      CYAN*16+BLUE,	  /* C_HELP_CURLINK    help page current link */
      WHITE*16+GRAY,	  /* C_PROMPT_BKGRD    prompt/choice background */
      WHITE*16+BLACK,	  /* C_PROMPT_TEXT     prompt/choice extra info */
      BLUE*16+WHITE,	  /* C_PROMPT_LO       prompt/choice text */
      BLUE*16+L_WHITE,	  /* C_PROMPT_MED      prompt/choice hdg2/... */
      BLUE*16+YELLOW,	  /* C_PROMPT_HI       prompt/choice hdg/cur/... */
      GREEN*16+L_WHITE,   /* C_PROMPT_INPUT    fullscreen_prompt input */
      CYAN*16+L_WHITE,	  /* C_PROMPT_CHOOSE   fullscreen_prompt choice */
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
/* start of string literals cleanup */
char s_iter[]    = "iter";
char s_real[]    = "real";
char s_mult[]     = "mult";
char s_sum[]     = "summ";
char s_imag[]    = "imag";
char s_zmag[]    = "zmag";
char s_bof60[]   = "bof60";
char s_bof61[]   = "bof61";
char s_maxiter[] =  "maxiter";
char s_epscross[] =  "epsilon cross";
char s_startrail[] =  "star trail";
char s_normal[] =  "normal";


void cmdfiles_overlay() { }	/* for restore_active_ovly */

/*
	cmdfiles(argc,argv) process the command-line arguments
		it also processes the 'sstools.ini' file and any
		indirect files ('fractint @myfile')
*/

int cmdfiles(int argc,char *argv[])
{
   int	   i;
   char    curarg[141];
   char    tempstring[101];
   char    *sptr;
   FILE    *initfile;

   ENTER_OVLY(OVLY_CMDFILES);

   if (first_init) initvars_run();	/* once per run initialization */
   initvars_restart();			/* <ins> key initialization */
   initvars_fractal();			/* image initialization */

   findpath("sstools.ini", tempstring); /* look for SSTOOLS.INI */
   if (tempstring[0] != 0)		/* found it! */
      if ((initfile = fopen(tempstring,"r")) != NULL)
	 cmdfile(initfile,1);		/* process it */

   for (i = 1; i < argc; i++) { 	/* cycle through args */
      strcpy(curarg,argv[i]);
      if (curarg[0] == ';')             /* start of comments? */
	 break;
      if (curarg[0] != '@') {           /* simple command? */
	 if (strchr(curarg,'=') == NULL) { /* not xxx=yyy, so check for gif */
	    strcpy(tempstring,curarg);
	    if (strchr(curarg,'.') == NULL)
	       strcat(tempstring,".gif");
	    if ((initfile = fopen(tempstring,"rb"))) {
	       fread(tempstring,6,1,initfile);
	       if ( tempstring[0] == 'G'
		 && tempstring[1] == 'I'
		 && tempstring[2] == 'F'
		 && tempstring[3] >= '8' && tempstring[3] <= '9'
		 && tempstring[4] >= '0' && tempstring[4] <= '9') {
		  strcpy(readname,curarg);
		  curarg[0] = showfile = 0;
		  }
	       fclose(initfile);
	       }
	    }
	 if (curarg[0])
	    cmdarg(curarg,0);		/* process simple command */
	 }
      else if ((sptr = strchr(curarg,'/'))) { /* @filename/setname? */
	 *sptr = 0;
	 strcpy(CommandFile,&curarg[1]);
	 strcpy(CommandName,sptr+1);
	 find_file_item(CommandFile,CommandName,&initfile);
	 cmdfile(initfile,3);
	 }
      else {				/* @filename */
	 if ((initfile = fopen(&curarg[1],"r")) == NULL)
	    argerror(curarg);
	 cmdfile(initfile,0);
	 }
      }

   if (first_init == 0) {
      initmode = -1; /* don't set video when <ins> key used */
      showfile = 1;  /* nor startup image file		    */
      }

   first_init = 0;
   EXIT_OVLY;
   return(0);
}


int load_commands(FILE *infile)
{
   /* when called, file is open in binary mode, positioned at the */
   /* '(' or '{' following the desired parameter set's name       */
   int ret;
   ENTER_OVLY(OVLY_CMDFILES);
   initcorners = initparams = 0; /* reset flags for type= */
   ret = cmdfile(infile,2);
   EXIT_OVLY;
   return ret;
}


static void initvars_run()		/* once per run init */
{
   init_rseed = (int)time(NULL);
}

static void initvars_restart()		/* <ins> key init */
{
   gif87a_flag = 0;			/* turn on GIF89a processing */
   askvideo = 1;			/* turn on video-prompt flag */
   overwrite = 0;			/* don't overwrite           */
   soundflag = -1;			/* sound is on		     */
   basehertz = 440;			/* basic hertz rate	     */
   initbatch = 0;			/* not in batch mode	     */
   initsavetime = 0;			/* no auto-save 	     */
   initmode = -1;			/* no initial video mode     */
   viewwindow = 0;			/* no view window	     */
   viewreduction = 4.2;
   viewcrop = 1;
   finalaspectratio = SCREENASPECT;
   viewxdots = viewydots = 0;
   orbit_delay = 0;                     /* full speed orbits */
   debugflag = 0;			/* debugging flag(s) are off */
   timerflag = 0;			/* timer flags are off	     */
   strcpy(FormFileName,"fractint.frm"); /* default formula file      */
   FormName[0] = 0;
   strcpy(LFileName,"fractint.l");
   LName[0] = 0;
   strcpy(CommandFile,"fractint.par");
   CommandName[0] = CommandComment1[0] = CommandComment2[0] = 0;
   strcpy(IFSFileName,"fractint.ifs");
   IFSName[0] = 0;
   reset_ifs_defn();
   rflag = 0;				/* not a fixed srand() seed */
   rseed = init_rseed;
   strcpy(readname,".\\");              /* initially current directory */
   showfile = 1;
   /* next should perhaps be fractal re-init, not just <ins> ? */
   initcyclelimit=55;			/* spin-DAC default speed limit */
   mapset = 0;				/* no map= name active */
   if (mapdacbox) {
      farmemfree(mapdacbox);
      mapdacbox = NULL;
      }
   TPlusFlag = 1;
   MaxColorRes = 8;
   PixelZoom = 0;
   NonInterlaced = 0;

   Transparent3D = 0;
   SolidCore = 1;
   CoreRed   = 128;
   CoreGreen = 128;
   CoreBlue  = 128;
   zzmin = -1.5;
   zzmax = 1.5;
   ttmin = 0.0;
   ttmax = 0.0;
   NumFrames = 1;
   tpdepth = tptime = 0;

   AntiAliasing = 0;
   Shadowing = 0;

   Printer_Type = 2;			/* assume an IBM/EPSON	  */
   Printer_Resolution = 60;		/* assume low resolution  */
   Printer_Titleblock = 0;		/* assume no title block  */
   Printer_ColorXlat = 0;		/* assume positive image  */
   Printer_SetScreen = 0;               /* assume default screen  */
   Printer_SFrequency = 45;             /* New screen frequency K */
   Printer_SAngle = 45;                 /* New screen angle     K */
   Printer_SStyle = 1;                  /* New screen style     K */
   Printer_RFrequency = 45;             /* New screen frequency R */
   Printer_RAngle = 75;                 /* New screen angle     R */
   Printer_RStyle = 1;                  /* New screen style     R */
   Printer_GFrequency = 45;             /* New screen frequency G */
   Printer_GAngle = 15;                 /* New screen angle     G */
   Printer_GStyle = 1;                  /* New screen style     G */
   Printer_BFrequency = 45;             /* New screen frequency B */
   Printer_BAngle = 0;                  /* New screen angle     B */
   Printer_BStyle = 1;                  /* New screen style     B */
   Print_To_File = 0;			/* No print-to-file	  */
   Printer_CRLF = 0;			/* Assume CR+LF 	  */
   EPSFileType = 0;			/* Assume no save to .EPS */
   LPTNumber = 1;			/* assume LPT1 */
   ColorPS = 0;                         /* Assume NO Color PostScr*/

}

static void initvars_fractal()		/* init vars affecting calculation */
{
   int i;
   bios_palette = 0;                    /* don't force use of a BIOS palette */
   escape_exit = 0;                     /* don't disable the "are you sure?" screen */
   usr_periodicitycheck = 1;		/* turn on periodicity	  */
   inside = 1;				/* inside color = blue	  */
   fillcolor = -1;			/* no special fill color */
   usr_biomorph = -1;			/* turn off biomorph flag */
   outside = -1;			/* outside color = -1 (not used) */
   maxit = 150; 			/* initial maxiter	  */
   usr_stdcalcmode = 'g';               /* initial solid-guessing */
   usr_floatflag = 0;			/* turn off the float flag */
   finattract = 0;			/* disable finite attractor logic */
   fractype = 0;			/* initial type Set flag  */
   curfractalspecific = &fractalspecific[0];
   initcorners = initparams = 0;
   bailout = 0; 			/* no user-entered bailout */
   useinitorbit = 0;
   for (i = 0; i < 4; i++) param[i] = 0.0;     /* initial parameter values */
   for (i = 0; i < 3; i++) potparam[i]	= 0.0; /* initial potential values */
   for (i = 0; i < 3; i++) inversion[i] = 0.0;	/* initial invert values */
   initorbit[0] = initorbit[1] = 0.0;	/* initial orbit values */
   invert = 0;
   decomp[0] = decomp[1] = 0;
   usr_distest = 0;
   distestwidth = 71;
   forcesymmetry = 999; 		/* symmetry not forced */
   xx3rd = xxmin = -2.5; xxmax = 1.5;	/* initial corner values  */
   yy3rd = yymin = -1.5; yymax = 1.5;	/* initial corner values  */
   pot16bit = potflag = 0;
   LogFlag = 0; 			/* no logarithmic palette */
   set_trig_array(0,"sin");             /* trigfn defaults */
   set_trig_array(1,"sqr");
   set_trig_array(2,"sinh");
   set_trig_array(3,"cosh");
   if (rangeslen) {
      farmemfree((char far *)ranges);
      rangeslen = 0;
      }
   usemag = 0;				/* use corners, not center-mag */

   colorstate = colorpreloaded = 0;
   rotate_lo = 1; rotate_hi = 255;	/* color cycling default range */

   display3d = 0;			/* 3D display is off	    */
   overlay3d = 0;			/* 3D overlay is off	    */

   initvars_3d();
}

static void initvars_3d()		/* init vars affecting 3d */
{
   RAY     = 0;
   BRIEF   = 0;
   SPHERE = FALSE;
   preview = 0;
   showbox = 0;
   xadjust = 0;
   yadjust = 0;
   eyeseparation = 0;
   glassestype = 0;
   previewfactor = 20;
   red_crop_left   = 4;
   red_crop_right  = 0;
   blue_crop_left  = 0;
   blue_crop_right = 4;
   red_bright	  = 80;
   blue_bright	 = 100;
   transparent[0] = transparent[1] = 0; /* no min/max transparency */
   set_3d_defaults();
}

static void reset_ifs_defn()
{
   if (ifs_defn) {
      farmemfree((char far *)ifs_defn);
      ifs_defn = NULL;
      }
}


static int cmdfile(FILE *handle,int mode)
   /* mode = 0 command line @filename	      */
   /*	     1 sstools.ini		      */
   /*	     2 <@> command after startup      */
   /*	     3 command line @filename/setname */
{
   /* note that cmdfile could be open as text OR as binary */
   /* binary is used in @ command processing for reasonable speed note/point */
   int i;
   int lineoffset = 0;
   int changeflag = 0; /* &1 fractal stuff chgd, &2 3d stuff chgd */
   char linebuf[513],cmdbuf[1001];
   if (mode == 2 || mode == 3) {
      while ((i = getc(handle)) != '{' && i != EOF) { }
      CommandComment1[0] = CommandComment2[0] = 0;
      }
   linebuf[0] = 0;
   while (next_command(cmdbuf,1000,handle,linebuf,&lineoffset,mode) > 0) {
      if ((mode == 2 || mode == 3) && strcmp(cmdbuf,"}") == 0) break;
      if ((i = cmdarg(cmdbuf,mode)) < 0) break;
      changeflag |= i;
      }
   fclose(handle);
   return changeflag;
}

static int next_command(char *cmdbuf,int maxlen,
		      FILE *handle,char *linebuf,int *lineoffset,int mode)
{
   int cmdlen = 0;
   char *lineptr;
   lineptr = linebuf + *lineoffset;
   while(1) {
      while (*lineptr <= ' ' || *lineptr == ';') {
	 if (cmdlen) {			/* space or ; marks end of command */
	    cmdbuf[cmdlen] = 0;
	    *lineoffset = lineptr - linebuf;
	    return cmdlen;
	    }
	 while (*lineptr && *lineptr <= ' ')
	    ++lineptr;			/* skip spaces and tabs */
	 if (*lineptr == ';' || *lineptr == 0) {
	    if (*lineptr == ';'
	      && (mode == 2 || mode == 3)
	      && (CommandComment1[0] == 0 || CommandComment2[0] == 0)) {
	       /* save comment */
	       while (*(++lineptr)
		 && (*lineptr == ' ' || *lineptr == '\t')) { }
	       if (*lineptr) {
		  if (strlen(lineptr) > 56)
		     *(lineptr+56) = 0;
		  if (CommandComment1[0] == 0)
		     strcpy(CommandComment1,lineptr);
		  else
		     strcpy(CommandComment2,lineptr);
		  }
	       }
	    if (next_line(handle,linebuf,mode) != 0)
	       return(-1); /* eof */
	    lineptr = linebuf; /* start new line */
	    }
	 }
      if (*lineptr == '\\'              /* continuation onto next line? */
	&& *(lineptr+1) == 0) {
	 if (next_line(handle,linebuf,mode) != 0) {
	    argerror(cmdbuf);		/* missing continuation */
	    return(-1);
	    }
	 lineptr = linebuf;
	 while (*lineptr && *lineptr <= ' ')
	    ++lineptr;			/* skip white space @ start next line */
	 continue;			/* loop to check end of line again */
	 }
      cmdbuf[cmdlen] = *(lineptr++);	/* copy character to command buffer */
      if (++cmdlen >= maxlen) { 	/* command too long? */
	 argerror(cmdbuf);
	 return(-1);
	 }
      }
}

static int next_line(FILE *handle,char *linebuf,int mode)
{
   int toolssection;
   char tmpbuf[10];
   toolssection = 0;
   while (file_gets(linebuf,512,handle) >= 0) {
      if (mode == 1 && linebuf[0] == '[') {     /* check for [fractint] */
	 strncpy(tmpbuf,&linebuf[1],9);
	 tmpbuf[9] = 0;
	 strlwr(tmpbuf);
	 toolssection = strncmp(tmpbuf,"fractint]",9);
	 continue;				/* skip tools section heading */
	 }
      if (toolssection == 0) return(0);
      }
   return(-1);
}


/*
  cmdarg(string,mode) processes a single command-line/command-file argument
    return:
      -1 error, >= 0 ok
      if ok, return value:
	| 1 means fractal parm has been set
	| 2 means 3d parm has been set
	| 4 means 3d=yes specified
	| 8 means reset specified
*/

static int cmdarg(char *curarg,int mode) /* process a single argument */
{
   char    variable[21];		/* variable name goes here   */
   char    *value;			/* pointer to variable value */
   int	   valuelen;			/* length of value	     */
   int	   numval;			/* numeric value of arg      */
#define NONNUMERIC -32767
   char    charval;			/* first character of arg    */
   int	   yesnoval;			/* 0 if 'n', 1 if 'y', -1 if not */
   double  ftemp;
   int	   i, j, k, l;
   char    *argptr,*argptr2;
   int	   totparms;			/* # of / delimited parms    */
   int	   intparms;			/* # of / delimited ints     */
   int	   floatparms;			/* # of / delimited floats   */
   int	   intval[64];			/* pre-parsed integer parms  */
   double  floatval[16];		/* pre-parsed floating parms */
   char    tmpc;
   int	   lastarg;

   argptr = curarg;
   while (*argptr) {			/* convert to lower case */
      if (*argptr >= 'A' && *argptr <= 'Z')
	 *argptr += 'a' - 'A';
      if (*argptr == '=' && strncmp(curarg,"colors=",7) == 0)
	 break; 			/* don't convert colors=value */
      ++argptr;
      }

   if ((value = strchr(&curarg[1],'='))) {
      if ((j = (value++) - curarg) > 1 && curarg[j-1] == ':')
	 --j;				/* treat := same as =	  */
      }
   else
      value = curarg + (j = strlen(curarg));
   if (j > 20) goto badarg;		/* keyword too long */
   strncpy(variable,curarg,j);		/* get the variable name  */
   variable[j] = 0;			/* truncate variable name */
   valuelen = strlen(value);		/* note value's length    */
   charval = value[0];			/* first letter of value  */
   yesnoval = -1;			/* note yes|no value	  */
   if (charval == 'n') yesnoval = 0;
   if (charval == 'y') yesnoval = 1;

   argptr = value;
   numval = totparms = intparms = floatparms = 0;
   while (*argptr) {			/* count and pre-parse parms */
      lastarg = 0;
      if ((argptr2 = strchr(argptr,'/')) == NULL) {     /* find next '/' */
	 argptr2 = argptr + strlen(argptr);
	 *argptr2 = '/';
	 lastarg = 1;
	 }
      if (totparms == 0) numval = NONNUMERIC;
      i = -1;
      if (sscanf(argptr,"%d%c",&j,&tmpc) > 0            /* got an integer */
	&& tmpc == '/') {
	 ++floatparms; ++intparms;
	 if (totparms < 16) floatval[totparms] = j;
	 if (totparms < 64) intval[totparms] = j;
	 if (totparms == 0) numval = j;
	 }
      else if (sscanf(argptr,"%lg%c",&ftemp,&tmpc) > 0  /* got a float */
	     && tmpc == '/') {
	 ++floatparms;
	 if (totparms < 16) floatval[totparms] = ftemp;
	 }
      ++totparms;
      argptr = argptr2; 				/* on to the next */
      if (lastarg)
	 *argptr = 0;
      else
	 ++argptr;
      }

   if (mode != 2) {	/* these commands are allowed only at startup */

      if (strcmp(variable,"batch") == 0 ) {     /* batch=?      */
	 if (yesnoval < 0) goto badarg;
	 initbatch = yesnoval;
	 return 3;
	 }

      if (strcmp(variable,"adapter") == 0 ) {   /* adapter==?     */

         if (strncmp(value,"aheada",6) == 0) svga_type = 1;
         if (strncmp(value,"ati"   ,3) == 0) svga_type = 2;
         if (strncmp(value,"chi"   ,3) == 0) svga_type = 3;
         if (strncmp(value,"eve"   ,3) == 0) svga_type = 4;
         if (strncmp(value,"gen"   ,3) == 0) svga_type = 5;
         if (strncmp(value,"ncr"   ,3) == 0) svga_type = 6;
         if (strncmp(value,"oak"   ,3) == 0) svga_type = 7;
         if (strncmp(value,"par"   ,3) == 0) svga_type = 8;
         if (strncmp(value,"tri"   ,3) == 0) svga_type = 9;
         if (strncmp(value,"tseng3",6) == 0) svga_type = 10;
         if (strncmp(value,"tseng4",6) == 0) svga_type = 11;
         if (strncmp(value,"vid"   ,3) == 0) svga_type = 12;
         if (strncmp(value,"aheadb",6) == 0) svga_type = 13;
         if (strncmp(value,"null"  ,4) == 0) svga_type = 14; /* for testing only */
         if (svga_type != 0) return 3;

	 video_type = 5;			/* assume video=vga */
	 if (strcmp(value,"egamono") == 0) {
	    video_type = 3;
	    mode7text = 1;
	    }
	 else if (strcmp(value,"hgc")) {             /* video = hgc */
	    video_type = 1;
	    mode7text = 1;
	    }
	 else if (strcmp(value,"ega"))               /* video = ega */
	    video_type = 3;
	 else if (strcmp(value,"cga"))               /* video = cga */
	    video_type = 2;
	 else if (strcmp(value,"mcga"))              /* video = mcga */
	    video_type = 4;
	 else if (strcmp(value,"vga"))               /* video = vga */
	    video_type = 5;
	 else
	    goto badarg;
	 return 3;
	 }

      if (strcmp(variable,"textsafe") == 0 ) {  /* textsafe==? */
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
	       goto badarg;
	    }
	 return 3;
	 }

      if (strcmp(variable, "vesadetect") == 0) {
	 if (yesnoval < 0) goto badarg;
	 vesa_detect = yesnoval;
	 return 3;
	 }

      if (strcmp(variable, "biospalette") == 0) {
         if (yesnoval < 0) goto badarg;
         bios_palette = yesnoval;
         return 3;
         }

      if (strcmp(variable, "exitnoask") == 0) {
         if (yesnoval < 0) goto badarg;
         escape_exit = yesnoval;
         return 3;
         }

      if (strcmp(variable,"fpu") == 0) {
	 if (strcmp(value,"iit") == 0) {
	    fpu = 387;
	    iit = 1;
	    return 0;
	    }
	 if (strcmp(value,"noiit") == 0) {
	    iit = -2;
	    return 0;
	    }
	 if (strcmp(value,"387") == 0) {
	    fpu = 387;
	    iit = -2;
	    return 0;
	    }
	 goto badarg;
	 }

      if (strcmp(variable,"makedoc") == 0) {
	 print_document(*value ? value : "fractint.doc", makedoc_msg_func, 0);
	 goodbye();
	 }

      } /* end of commands allowed only at startup */

   if (strcmp(variable,"reset") == 0) {
      initvars_fractal();
      return 9;
      }

   if (strcmp(variable,"filename") == 0) {      /* filename=?     */
      if (charval == '.') {
	 if (valuelen > 4) goto badarg;
	 gifmask[0] = '*';
	 gifmask[1] = 0;
	 strcat(gifmask,value);
	 return 0;
	 }
      if (valuelen > 79) goto badarg;
      if (mode == 2 && display3d == 0) /* can't do this in @ command */
	 goto badarg;
      strcpy(readname,value);
      showfile = 0;
      return 3;
      }

   if (strcmp(variable,"video") == 0) {         /* video=? */
      if (active_system == 0) {
	 if ((k = check_vidmode_keyname(value)) == 0) goto badarg;
	 initmode = -1;
	 for (i = 0; i < MAXVIDEOTABLE; ++i) {
	    if (videotable[i].keynum == k) {
	       initmode = i;
	       break;
	       }
	    }
	 if (initmode == -1) goto badarg;
	 }
      return 3;
      }

   if (strcmp(variable,"map") == 0 ) {         /* map=, set default colors */
      if (valuelen > 79 || SetColorPaletteName(value) != 0) goto badarg;
      mapset = 1;
      strcpy(MAP_name,value);
      return 0;
      }

   if (strcmp(variable,"colors") == 0) {       /* colors=, set current colors */
      if (parse_colors(value) < 0) goto badarg;
      return 0;
      }

   if (strcmp(variable, "tplus") == 0) {       /* Use the TARGA+ if found? */
      if (yesnoval < 0) goto badarg;
      TPlusFlag = yesnoval;
      return 0;
      }

   if (strcmp(variable, "noninterlaced") == 0) {
      if (yesnoval < 0) goto badarg;
      NonInterlaced = yesnoval;
      return 0;
      }

   if (strcmp(variable, "maxcolorres") == 0) { /* Change default color resolution */
      if (numval == 1 || numval == 4 || numval == 8 ||
			numval == 16 || numval == 24) {
	 MaxColorRes = numval;
	 return 0;
	 }
      goto badarg;
      }

   if (strcmp(variable, "pixelzoom") == 0) {
      if (numval < 5)
	 PixelZoom = numval;
      return 0;
      }

   /* keep this for backward compatibility */
   if (strcmp(variable,"warn") == 0 ) {         /* warn=? */
      if (yesnoval < 0) goto badarg;
      overwrite = yesnoval ^ 1;
      return 0;
      }
   if (strcmp(variable,"overwrite") == 0 ) {    /* overwrite=? */
      if (yesnoval < 0) goto badarg;
      overwrite = yesnoval;
      return 0;
      }

   if (strcmp(variable,"gif87a") == 0 ) {       /* gif87a=? */
      if (yesnoval < 0) goto badarg;
      gif87a_flag = yesnoval;
      return 0;
      }

   if (strcmp(variable,"savetime") == 0) {      /* savetime=? */
      initsavetime = numval;
      return 0;
      }

   if (strcmp(variable,"autokey") == 0) {       /* autokey=? */
      if (strcmp(value,"record")==0)
	 slides=2;
      else if (strcmp(value,"play")==0)
	 slides=1;
      else
	 goto badarg;
      return 0;
      }

   if(strcmp(variable, "solidcore") == 0) {
      SolidCore = yesnoval;
      return(0);
      }

   if(strcmp(variable, "antialias") == 0) {
      if(numval < 0 || numval > 8)
	 goto badarg;
      AntiAliasing = numval;
      return(0);
      }

   if(strcmp(variable, "transparent3d") == 0) {
      Transparent3D = yesnoval;
      return(0);
      }

   if(strcmp(variable, "corecolor") == 0) {
      if(floatparms != totparms || totparms != 3)
	 goto badarg;
      CoreRed	= (int)floatval[0];
      CoreGreen = (int)floatval[1];
      CoreBlue	= (int)floatval[2];
      return(0);
      }

   if(strcmp(variable, "mdcorners") == 0) {
      if(floatparms != totparms || totparms < 2 || totparms > 4)
	 goto badarg;
      zzmin = floatval[0];
      zzmax = floatval[1];
      if(totparms >= 3)
	 ttmin = floatval[2];
      if(totparms == 4)
	 ttmax = floatval[3];
      return(0);
      }

   if(strcmp(variable, "numframes") == 0) {
      NumFrames = numval;
      return(0);
      }

   if (strcmp(variable,"autokeyname") == 0) {   /* autokeyname=? */
      strcpy(autoname,value);
      return 0;
      }

   if (strcmp(variable,"type") == 0 ) {         /* type=? */
      if (value[valuelen-1] == '*')
	 value[--valuelen] = 0;
      /* kludge because type ifs3d has an asterisk in front */
      if(strcmp(value,"ifs3d")==0)
         value[3]=0;
      for (k = 0; fractalspecific[k].name != NULL; k++)
	 if (strcmp(value,fractalspecific[k].name) == 0)
	    break;
      if (fractalspecific[k].name == NULL) goto badarg;
      curfractalspecific = &fractalspecific[fractype = k];
      if (initcorners == 0) {
	 xx3rd = xxmin = curfractalspecific->xmin;
	 xxmax	       = curfractalspecific->xmax;
	 yy3rd = yymin = curfractalspecific->ymin;
	 yymax	       = curfractalspecific->ymax;
	 }
      if (initparams == 0)
	 for (k = 0; k < 4; ++k) {
	    param[k] = curfractalspecific->paramvalue[k];
	    roundfloatd(&param[k]);
	    }
      return 1;
      }

   if (strcmp(variable,"inside") == 0 ) {       /* inside=? */
      if(strcmp(value,s_zmag)==0)
	 inside = -59;
      else if(strcmp(value,s_bof60)==0)
	 inside = -60;
      else if(strcmp(value,s_bof61)==0)
	 inside = -61;
      else if(strncmp(value,s_epscross,3)==0)
	 inside = -100;
      else if(strncmp(value,s_startrail,4)==0)
	 inside = -101;
      else if(strcmp(value,s_maxiter)==0)
	 inside = -1;
      else if(numval == NONNUMERIC)
	 goto badarg;
      else
	 inside = numval;
      return 1;
      }
   if (strcmp(variable,"fillcolor") == 0 ) {       /* fillcolor */
      if(strcmp(value,s_normal)==0)
	 fillcolor = -1;
      else if(numval == NONNUMERIC)
	 goto badarg;
      else
	 fillcolor = numval;
      return 1;
      }

   if (strcmp(variable,"finattract") == 0 ) {   /* finattract=? */
      if (yesnoval < 0) goto badarg;
      finattract = yesnoval;
      return 1;
      }

   if (strcmp(variable,"function") == 0) {      /* function=?,? */
      k = 0;
      while (*value && k < 4) {
	 if(set_trig_array(k++,value)) goto badarg;
	 if ((value = strchr(value,'/')) == NULL) break;
	 ++value;
	 }
      return 1;
      }

   if (strcmp(variable,"outside") == 0 ) {      /* outside=? */
      if(strcmp(value,s_iter)==0)
	 outside = -1;
      if(strcmp(value,s_real)==0)
	 outside = -2;
      else if(strcmp(value,s_imag)==0)
	 outside = -3;
      else if(strcmp(value,s_mult)==0)
	 outside = -4;
      else if(strcmp(value,s_sum)==0)
	 outside = -5;

      else if(numval == NONNUMERIC)
	 goto badarg;
      else if(numval < -5 || numval > 255) goto badarg;
      else outside = numval;
      return 1;
      }

   if (strcmp(variable,s_maxiter) == 0) {       /* maxiter=? */
      if (numval < 2) goto badarg;
      maxit = numval;
      return 1;
      }

   if (strcmp(variable,"iterincr") == 0)        /* iterincr=? */
      return 0;

   if (strcmp(variable,"passes") == 0) {        /* passes=? */
      if ( charval != '1' && charval != '2'
	&& charval != 'g' && charval != 'b'
	&& charval != 't')
	 goto badarg;
      usr_stdcalcmode = charval;
      return 1;
      }

   if (strcmp(variable,"cyclelimit") == 0 ) {   /* cyclelimit=? */
      if (numval <= 1 || numval > 256) goto badarg;
      initcyclelimit = numval;
      return 0;
      }

   if (strcmp(variable,"cyclerange") == 0) {
      if (totparms < 2) intval[1] = 255;
      if (totparms < 1) intval[0] = 1;
      if (totparms != intparms
	|| intval[0] < 0 || intval[1] > 255 || intval[0] > intval[1])
	 goto badarg;
      rotate_lo = intval[0];
      rotate_hi = intval[1];
      return 0;
      }

   if (strcmp(variable,"ranges") == 0) {
      int i,j,entries,prev;
      int tmpranges[128];
      if (totparms != intparms) goto badarg;
      entries = prev = i = 0;
      while (i < totparms) {
	 if ((j = intval[i++]) < 0) { /* striping */
	    if ((j = 0-j) < 1 || j >= 16384 || i >= totparms) goto badarg;
	    tmpranges[entries++] = -1; /* {-1,width,limit} for striping */
	    tmpranges[entries++] = j;
	    j = intval[i++];
	    }
	 if (j < prev) goto badarg;
	 tmpranges[entries++] = prev = j;
	 }
      if (prev == 0) goto badarg;
      if ((ranges = (int far *)farmemalloc(2L*entries)) == NULL) {
	 static char far msg[] = {"Insufficient memory for ranges="};
	 stopmsg(0,msg);
	 return(-1);
	 }
      rangeslen = entries;
      for (i = 0; i < rangeslen; ++i)
	 ranges[i] = tmpranges[i];
      return 1;
      }

   if (strcmp(variable,"savename") == 0) {      /* savename=? */
      if (valuelen > 79) goto badarg;
      if (first_init || mode == 2)
	 strcpy(savename,value);
      return 0;
      }

   if (strcmp(variable,"exitmode") == 0) {      /* exitmode=? */
      sscanf(value,"%x",&numval);
      exitmode = numval;
      return 0;
      }

   if (strcmp(variable,"textcolors") == 0) {
      parse_textcolors(value);
      return 0;
      }

   if (strcmp(variable,"potential") == 0) {     /* potential=? */
      k = 0;
      while (k < 3 && *value) {
	 potparam[k++] = atoi(value);
	 if ((value = strchr(value,'/')) == NULL) k = 99;
	 ++value;
	 }
      pot16bit = 0;
      if (k < 99) {
	 if (strcmp(value,"16bit")) goto badarg;
	 pot16bit = 1;
	 }
      return 1;
      }

   if (strcmp(variable,"params") == 0) {        /* params=?,? */
      if (totparms != floatparms || totparms > 4)
	 goto badarg;
      for (k = 0; k < 4; ++k)
	 param[k] = (k < totparms) ? floatval[k] : 0.0;
      initparams = 1;
      return 1;
      }

   if (strcmp(variable,"initorbit") == 0) {     /* initorbit=?,? */
      if(strcmp(value,"pixel")==0)
	 useinitorbit = 2;
      else {
	 if (totparms != 2 || floatparms != 2) goto badarg;
	 initorbit[0] = floatval[0];
	 initorbit[1] = floatval[1];
	 useinitorbit = 1;
	 }
      return 1;
      }

   if (strcmp(variable,"corners") == 0) {       /* corners=?,?,?,? */
      if (floatparms != totparms || (totparms != 4 && totparms != 6))
	 goto badarg;
      usemag = 0;
      initcorners = 1;
      xx3rd = xxmin = floatval[0];
      xxmax =	      floatval[1];
      yy3rd = yymin = floatval[2];
      yymax =	      floatval[3];
      if (totparms == 6) {
	 xx3rd =      floatval[4];
	 yy3rd =      floatval[5];
	 }
      return 1;
      }

   if (strcmp(variable,"center-mag") == 0) {    /* center-mag=?,?,? */
      double Xctr, Yctr,Magnification,Ratio,Height, Width,Radius;
      if (totparms != floatparms
	|| (totparms != 0 && totparms != 3)
	|| (totparms == 3 && floatval[2] <= 0.0))
	 goto badarg;
      usemag = 1;
      if (totparms == 0) return 0;
      initcorners = 1;
      Xctr = floatval[0];
      Yctr = floatval[1];
      Magnification = floatval[2];
      Radius = 1.0 / Magnification;
      Ratio = .75;	/* inverse aspect ratio of screen  */
      /* calculate bounds */
      Height = 2.0 * Radius;
      Width = Height / Ratio;
      yymax = Yctr + Radius;
      yy3rd = yymin = Yctr - Radius;
      xxmax = Xctr + Width / 2.0;
      xx3rd = xxmin = Xctr - Width / 2.0;
      return 1;
      }

   if (strcmp(variable,"invert") == 0) {        /* invert=?,?,? */
      if (totparms != floatparms || (totparms != 1 && totparms != 3))
	 goto badarg;
      invert = ((inversion[0] = floatval[0]) != 0.0) ? totparms : 0;
      if (totparms == 3) {
	 inversion[1] = floatval[1];
	 inversion[2] = floatval[2];
	 }
      return 1;
      }

   if (strcmp(variable,"askvideo") == 0 ) {     /* askvideo=?   */
      if (yesnoval < 0) goto badarg;
      askvideo = yesnoval;
      return 0;
      }

   if (strcmp(variable,"ramvideo") == 0 )       /* ramvideo=?   */
      return 0; /* just ignore and return, for old time's sake */

   if (strcmp(variable,"float") == 0 ) {        /* float=? */
      if (yesnoval < 0) goto badarg;
      usr_floatflag = yesnoval;
      return 3;
      }

   if (strcmp(variable,"biomorph") == 0 ) {     /* biomorph=? */
      usr_biomorph = numval;
      return 1;
      }

   if (strcmp(variable,"orbitsave") == 0 ) {     /* orbitsave=? */
      if (yesnoval < 0) goto badarg;
      orbitsave = yesnoval;
      return 1;
      }

   if (strcmp(variable,"bailout") == 0 ) {      /* bailout=? */
      if (numval < 4 || numval > 32000) goto badarg;
      bailout = numval;
      return 1;
      }

   if (strcmp(variable,"symmetry") == 0 ) {     /* symmetry=? */
      if     (strcmp(value,"xaxis" )==0) forcesymmetry = XAXIS;
      else if(strcmp(value,"yaxis" )==0) forcesymmetry = YAXIS;
      else if(strcmp(value,"xyaxis")==0) forcesymmetry = XYAXIS;
      else if(strcmp(value,"origin")==0) forcesymmetry = ORIGIN;
      else if(strcmp(value,"pi"    )==0) forcesymmetry = PI_SYM;
      else if(strcmp(value,"none"  )==0) forcesymmetry = NOSYM;
      else goto badarg;
      return 1;
      }

   if (strcmp(variable,"printer") == 0 ) {      /* printer=? */
      if (parse_printer(value) < 0) goto badarg;
      return 0;
      }

   if (strcmp(variable,"printfile") == 0) {     /* print-to-file? SWT */
      if (valuelen > 79) goto badarg;
      Print_To_File = 1;
      strcpy(PrintName,value);
      return 0;
      }

   if(strcmp(variable, "colorps") == 0) {
      ColorPS = yesnoval;
      return(0);
      }

   if (strcmp(variable,"epsf") == 0) {          /* EPS type? SWT */
      Print_To_File = 1;
      EPSFileType = numval;
      Printer_Type = 5;
      if (strcmp(PrintName,"fract001.prn")==0)
	 strcpy(PrintName,"fract001.eps");
      return 0;
      }

   if (strcmp(variable,"title") == 0) {         /* Printer title block? SWT */
      if (yesnoval < 0) goto badarg;
      Printer_Titleblock = yesnoval;
      return 0;
      }

   if (strcmp(variable,"translate") == 0) {     /* Translate color? SWT */
      Printer_ColorXlat=0;
      if (charval == 'y')
	 Printer_ColorXlat=1;
      else if (numval > 1 || numval < -1)
	 Printer_ColorXlat=numval;
      return 0;
      }

   if (strcmp(variable,"plotstyle") == 0) {     /* plot style? SWT */
      Printer_SStyle = numval;
      return 0;
      }

   if (strcmp(variable,"halftone") == 0) {      /* New halftoning? SWT */
      if (totparms != intparms) goto badarg;
      Printer_SetScreen=1;
      if ((totparms >  0) && ( intval[ 0] >= 0))
					  Printer_SFrequency = intval[ 0];
      if ((totparms >  1) && ( intval[ 1] >= 0))
					  Printer_SAngle     = intval[ 1];
      if ((totparms >  2) && ( intval[ 2] >= 0))
					  Printer_SStyle     = intval[ 2];
      if ((totparms >  3) && ( intval[ 3] >= 0))
					  Printer_RFrequency = intval[ 3];
      if ((totparms >  4) && ( intval[ 4] >= 0))
					  Printer_RAngle     = intval[ 4];
      if ((totparms >  5) && ( intval[ 5] >= 0))
					  Printer_RStyle     = intval[ 5];
      if ((totparms >  6) && ( intval[ 6] >= 0))
					  Printer_GFrequency = intval[ 6];
      if ((totparms >  7) && ( intval[ 7] >= 0))
					  Printer_GAngle     = intval[ 7];
      if ((totparms >  8) && ( intval[ 8] >= 0))
					  Printer_GStyle     = intval[ 8];
      if ((totparms >  9) && ( intval[ 9] >= 0))
					  Printer_BFrequency = intval[ 9];
      if ((totparms > 10) && ( intval[10] >= 0))
					  Printer_BAngle     = intval[10];
      if ((totparms > 11) && ( intval[11] >= 0))
					  Printer_BStyle     = intval[11];
      return 0;
      }

   if (strcmp(variable,"linefeed") == 0) {      /* Use LF for printer */
      if      (strcmp(value,"cr")   == 0) Printer_CRLF = 1;
      else if (strcmp(value,"lf")   == 0) Printer_CRLF = 2;
      else if (strcmp(value,"crlf") == 0) Printer_CRLF = 0;
      else goto badarg;
      return 0;
      }

   if (strcmp(variable,"comport") == 0 ) {      /* Set the COM parameters */
      if ((value=strchr(value,'/')) == NULL) goto badarg;
      switch (atoi(++value)) {
	 case 110:  l = 0;   break;
	 case 150:  l = 32;  break;
	 case 300:  l = 64;  break;
	 case 600:  l = 96;  break;
	 case 1200: l = 128; break;
	 case 2400: l = 160; break;
	 case 4800: l = 192; break;
	 case 9600:
	 default:   l = 224; break;
	 }
      if ((value=strchr(value,'/')) == NULL) goto badarg;
      for (k=0; k < strlen(value); k++) {
	 switch (value[k]) {
	    case '7':  l |= 2;  break;
	    case '8':  l |= 3;  break;
	    case 'o':  l |= 8;  break;
	    case 'e':  l |= 24; break;
	    case '2':  l |= 4;  break;
	    }
	 }
#ifndef WINFRACT
      _bios_serialcom(0,numval-1,l);
#endif
      return 0;
      }

   if (strcmp(variable,"sound") == 0 ) {        /* sound=? */
      soundflag = 0;
      if (strncmp(value,"ye",2) == 0)
	 soundflag = -1;
      if (charval == 'x')
	 soundflag = 1;
      if (charval == 'y')
	 soundflag = 2;
      if (charval == 'z')
	 soundflag = 3;
      return 0;
      }

   if (strcmp(variable,"hertz") == 0) {         /* Hertz=? */
      if (numval < 200 || numval > 10000) goto badarg;
      basehertz = numval;
      return 0;
      }

   if (strcmp(variable,"periodicity") == 0 ) {  /* periodicity=? */
      usr_periodicitycheck=1;
      if (charval == 'n')
	 usr_periodicitycheck=0;
      else if (charval == 'y')
	 usr_periodicitycheck=1;
      else if (charval == 's')   /* 's' for 'show' */
	 usr_periodicitycheck=-1;
      else if(numval == NONNUMERIC)
	 goto badarg;
      else if(numval != 0)
	 usr_periodicitycheck=numval;
      return 1;
      }

   if (strcmp(variable,"logmap") == 0 ) {       /* logmap=? */
      if (charval == 'y')
	 LogFlag = 1;				/* palette is logarithmic */
      else if (charval == 'n')
	 LogFlag = 0;
      else if (charval == 'o')
	 LogFlag = -1;				/* old log palette */
      else
	 LogFlag = numval;
      return 1;
      }

   if (strcmp(variable,"debugflag") == 0
     || strcmp(variable,"debug") == 0) {        /* internal use only */
      debugflag = numval;
      timerflag = debugflag & 1;		/* separate timer flag */
      debugflag -= timerflag;
      return 0;
      }

   if (strcmp(variable, "rseed") == 0) {
      rseed = numval;
      rflag = 1;
      return 1;
      }

   if (strcmp(variable, "orbitdelay") == 0) {
      orbit_delay = numval;
      return 0;
      }

   if (strcmp(variable, "showdot") == 0) {
      showdot=numval;
      if(showdot<0)
         showdot=0;
      return 0;
      }

   if (strcmp(variable, "decomp") == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      decomp[0] = intval[0];
      decomp[1] = 0;
      if (totparms > 1) /* backward compatibility */
	 bailout = decomp[1] = intval[1];
      return 1;
      }

   if (strcmp(variable, "distest") == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      usr_distest = intval[0];
      distestwidth = 71;
      if (totparms > 1)
	 distestwidth = intval[1];
      return 1;
      }

   if (strcmp(variable,"formulafile") == 0) {   /* formulafile=? */
      if (valuelen > 79) goto badarg;
      strcpy(FormFileName,value);
      return 1;
      }

   if (strcmp(variable,"formulaname") == 0) {   /* formulaname=? */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(FormName,value);
      return 1;
      }

   if (strcmp(variable,"lfile") == 0) {
      if (valuelen > 79) goto badarg;
      strcpy(LFileName,value);
      return 1;
      }

   if (strcmp(variable,"lname") == 0) {
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(LName,value);
      return 1;
      }

   if (strcmp(variable,"ifsfile") == 0) {
      if (valuelen > 79) goto badarg;
      strcpy(IFSFileName,value);
      reset_ifs_defn();
      return 1;
      }

   if (strcmp(variable,"ifs") == 0
     || strcmp(variable,"ifs3d") == 0) {        /* ifs3d for old time's sake */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(IFSName,value);
      reset_ifs_defn();
      return 1;
      }

   if (strcmp(variable,"parmfile") == 0) {
      if (valuelen > 79) goto badarg;
      strcpy(CommandFile,value);
      return 0;
      }

   if (strcmp(variable,"stereo") == 0) {        /* stereo=? */
      if ((numval<0) || (numval>3)) goto badarg;
      glassestype = numval;
      return 3;
      }

   if (strcmp(variable,"rotation") == 0) {      /* rotation=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XROT = intval[0];
      YROT = intval[1];
      ZROT = intval[2];
      return 3;
      }

   if (strcmp(variable,"perspective") == 0) {   /* perspective=? */
      if (numval == NONNUMERIC) goto badarg;
      ZVIEWER = numval;
      return 3;
      }

   if (strcmp(variable,"xyshift") == 0) {       /* xyshift=?/?  */
      if (totparms != 2 || intparms != 2) goto badarg;
      XSHIFT = intval[0];
      YSHIFT = intval[1];
      return 3;
      }

   if (strcmp(variable,"interocular") == 0) {   /* interocular=? */
      eyeseparation = numval;
      return 3;
      }

   if (strcmp(variable,"converge") == 0) {      /* converg=? */
      xadjust = numval;
      return 3;
      }

   if (strcmp(variable,"crop") == 0) {          /* crop=? */
      if (totparms != 4 || intparms != 4
	|| intval[0] < 0 || intval[0] > 100
	|| intval[1] < 0 || intval[1] > 100
	|| intval[2] < 0 || intval[2] > 100
	|| intval[3] < 0 || intval[3] > 100)
	  goto badarg;
      red_crop_left   = intval[0];
      red_crop_right  = intval[1];
      blue_crop_left  = intval[2];
      blue_crop_right = intval[3];
      return 3;
      }

   if (strcmp(variable,"bright") == 0) {        /* bright=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      red_bright  = intval[0];
      blue_bright = intval[1];
      return 3;
      }

   if (strcmp(variable,"xyadjust") == 0) {      /* trans=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      xtrans = intval[0];
      ytrans = intval[1];
      return 3;
      }

   if (strcmp(variable,"3d") == 0) {            /* 3d=?/?/..    */
      if (yesnoval < 0) goto badarg;
      display3d = yesnoval;
      initvars_3d();
      return (display3d) ? 6 : 2;
      }

   if (strcmp(variable,"sphere") == 0 ) {       /* sphere=? */
      if (yesnoval < 0) goto badarg;
      SPHERE = yesnoval;
      return 2;
      }

   if (strcmp(variable,"scalexyz") == 0) {      /* scalexyz=?/?/? */
      if (totparms < 2 || intparms != totparms) goto badarg;
      XSCALE = intval[0];
      YSCALE = intval[1];
      if (totparms > 2) ROUGH = intval[2];
      return 2;
      }

   /* "rough" is really scale z, but we add it here for convenience */
   if (strcmp(variable,"roughness") == 0) {     /* roughness=?  */
      ROUGH = numval;
      return 2;
      }

   if (strcmp(variable,"waterline") == 0) {     /* waterline=?  */
      if (numval<0) goto badarg;
      WATERLINE = numval;
      return 2;
      }

   if (strcmp(variable,"filltype") == 0) {      /* filltype=?   */
      if (numval < -1 || numval > 6) goto badarg;
      FILLTYPE = numval;
      return 2;
      }

   if (strcmp(variable,"lightsource") == 0) {   /* lightsource=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XLIGHT = intval[0];
      YLIGHT = intval[1];
      ZLIGHT = intval[2];
      return 2;
      }

   if (strcmp(variable,"smoothing") == 0) {     /* smoothing=?  */
      if (numval<0) goto badarg;
      LIGHTAVG = numval;
      return 2;
      }

   if (strcmp(variable,"latitude") == 0) {      /* latitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      THETA1 = intval[0];
      THETA2 = intval[1];
      return 2;
      }

   if (strcmp(variable,"longitude") == 0) {     /* longitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      PHI1 = intval[0];
      PHI2 = intval[1];
      return 2;
      }

   if (strcmp(variable,"radius") == 0) {        /* radius=? */
      if (numval < 0) goto badarg;
      RADIUS = numval;
      return 2;
      }

   if (strcmp(variable,"transparent") == 0) {   /* transparent? */
      if (totparms != intparms || totparms < 1) goto badarg;
      transparent[1] = transparent[0] = intval[0];
      if (totparms > 1) transparent[1] = intval[1];
      return 2;
      }

   if (strcmp(variable,"preview") == 0) {       /* preview? */
      if (yesnoval < 0) goto badarg;
      preview = yesnoval;
      return 2;
      }

   if (strcmp(variable,"showbox") == 0) {       /* showbox? */
      if (yesnoval < 0) goto badarg;
      showbox = yesnoval;
      return 2;
      }

   if (strcmp(variable,"coarse") == 0) {        /* coarse=? */
      if (numval < 3 || numval > 2000) goto badarg;
      previewfactor = numval;
      return 2;
      }

   if (strcmp(variable,"randomize") == 0) {     /* RANDOMIZE=? */
      if (numval<0 || numval>7) goto badarg;
      RANDOMIZE = numval;
      return 2;
      }

   if (strcmp(variable,"ambient") == 0) {       /* ambient=? */
      if (numval<0||numval>100) goto badarg;
      Ambient = numval;
      return 2;
      }

   if (strcmp(variable,"haze") == 0) {          /* haze=? */
      if (numval<0||numval>100) goto badarg;
      haze = numval;
      return 2;
      }

   if (strcmp(variable,"fullcolor") == 0) {     /* fullcolor=? */
      if (yesnoval < 0) goto badarg;
      Targa_Out = yesnoval;
      return 2;
      }
   if (strcmp(variable,"targa_out") == 0) {     /* Targa Out? */
      if (yesnoval < 0) goto badarg;
      Targa_Out = yesnoval;
      return 2;
      }

   if (strcmp(variable,"targa_overlay") == 0) {         /* Targa Overlay? */
      if (yesnoval < 0) goto badarg;
      Targa_Overlay = yesnoval;
      return 2;
      }

   if (strcmp(variable,"background") == 0) {     /* background=?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
                for (i=0;i<3;i++)
                        if (intval[i] & 0xff)
                                goto badarg;
      back_color[0] = intval[0];
      back_color[1] = intval[1];
      back_color[2] = intval[2];
      return 2;
      }

   if (strcmp(variable,"lightname") == 0) {     /* lightname=?   */
      if (valuelen > 79) goto badarg;
      if (first_init || mode == 2)
	 strcpy(light_name,value);
      return 0;
      }

   if (strcmp(variable,"ray") == 0) {           /* RAY=? */
      if (numval < 0 || numval > 6) goto badarg;
      RAY = numval;
      return 2;
      }

   if (strcmp(variable,"brief") == 0) {         /* BRIEF? */
      if (yesnoval < 0) goto badarg;
      BRIEF = yesnoval;
      return 2;
      }


badarg:
   argerror(curarg);
   return(-1);

}

/* Some routines broken out of above so compiler doesn't run out of heap: */

static void parse_textcolors(char *value)
{
   int i,j,k,hexval;
   if (strcmp(value,"mono") == 0) {
      for (k = 0; k < sizeof(txtcolor); ++k)
	 txtcolor[k] = BLACK*16+WHITE;
   /* C_HELP_CURLINK = C_PROMPT_INPUT = C_CHOICE_CURRENT = C_GENERAL_INPUT
		     = C_AUTHDIV1 = C_AUTHDIV2 = WHITE*16+BLACK; */
      txtcolor[6] = txtcolor[12] = txtcolor[13] = txtcolor[14] = txtcolor[20]
		  = txtcolor[27] = txtcolor[28] = WHITE*16+BLACK;
      /* C_TITLE = C_HELP_HDG = C_HELP_LINK = C_PROMPT_HI = C_CHOICE_SP_KEYIN
		 = C_GENERAL_HI = C_DVID_HI = C_STOP_ERR
		 = C_STOP_INFO = BLACK*16+L_WHITE; */
      txtcolor[0] = txtcolor[2] = txtcolor[5] = txtcolor[11] = txtcolor[16]
		  = txtcolor[17] = txtcolor[22] = txtcolor[24]
		  = txtcolor[25] = BLACK*16+L_WHITE;
      }
   else {
      k = 0;
      while ( k < sizeof(txtcolor)) {
	 if (*value == 0) break;
	 if (*value != '/') {
	    sscanf(value,"%x",&hexval);
	    i = (hexval / 16) & 7;
	    j = hexval & 15;
	    if (i == j || (i == 0 && j == 8)) /* force contrast */
	       j = 15;
	    txtcolor[k] = i * 16 + j;
	    if ((value = strchr(value,'/')) == NULL) break;
	    }
	 ++value;
	 ++k;
	 }
      }
}

static int parse_colors(char *value)
{
   int i,j,k;
   if (*value == '@') {
      if (strlen(value) > 80 || ValidateLuts(&value[1]) != 0) goto badcolor;
      if (display3d) {
        mapset = 1;
        strcpy(MAP_name,&value[1]);
        }
      else {
        strcpy(colorfile,&value[1]);
        colorstate = 2;
        }
      }
   else {
      int smooth;
      i = smooth = 0;
      while (*value) {
	 if (i >= 256) goto badcolor;
	 if (*value == '<') {
	    if (i == 0 || smooth
	      || (smooth = atoi(value+1)) < 2
	      || (value = strchr(value,'>')) == NULL)
	       goto badcolor;
	    i += smooth;
	    ++value;
	    }
	 else {
	    for (j = 0; j < 3; ++j) {
	       if ((k = *(value++)) < '0')  goto badcolor;
	       else if (k <= '9')       k -= '0';
	       else if (k < 'A')            goto badcolor;
	       else if (k <= 'Z')       k -= ('A'-10);
	       else if (k < '_' || k > 'z') goto badcolor;
	       else			k -= ('_'-36);
	       dacbox[i][j] = k;
	       if (smooth) {
		  int start,spread,cnum;
		  start = i - (spread = smooth + 1);
		  cnum = 0;
		  if ((k - (int)dacbox[start][j]) == 0) {
		     while (++cnum < spread)
			dacbox[start+cnum][j] = k;
		     }
		  else {
		     while (++cnum < spread)
			dacbox[start+cnum][j] =
			   ( cnum	     *dacbox[i][j]
			   + (i-(start+cnum))*dacbox[start][j]
			   + spread/2 )
			   / spread;
		     }
		  }
	       }
	    smooth = 0;
	    ++i;
	    }
	 }
      if (smooth) goto badcolor;
      while (i < 256)  { /* zap unset entries */
	 dacbox[i][0] = dacbox[i][1] = dacbox[i][2] = 40;
	 ++i;
	 }
      colorstate = 1;
      }
   colorpreloaded = 1;
   return(0);
badcolor:
   return(-1);
}

static int parse_printer(char *value)
{
   int k;
   if (value[0]=='h' && value[1]=='p')
      Printer_Type=1;			     /* HP LaserJet	       */
   if (value[0]=='i' && value[1]=='b')
      Printer_Type=2;			     /* IBM Graphics	       */
   if (value[0]=='e' && value[1]=='p')
      Printer_Type=2;			     /* Epson (model?)	       */
   if (value[0]=='c' && value[1]=='o')
      Printer_Type=3;			     /* Star (Epson-Comp?) color */
   if (value[0]=='p') {
      if (value[1]=='a')
	 Printer_Type=4;		     /* HP Paintjet (color)    */
      if ((value[1]=='o' || value[1]=='s')) {
	 Printer_Type=5;		     /* PostScript  SWT */
	 if (value[2]=='h' || value[2]=='l')
	    Printer_Type=6;
	 }
      if (value[1]=='l')
	 Printer_Type=7;		     /* HP Plotter (semi-color) */
      }
   if (Printer_Type == 1)		     /* assume low resolution */
      Printer_Resolution = 75;
   else
      Printer_Resolution = 60;
   if (EPSFileType > 0) 		     /* EPS save - force type 5 */
      Printer_Type = 5;
   if ((Printer_Type == 5) || (Printer_Type == 6))
      Printer_Resolution = 150; 	     /* PostScript def. res. */
   if ((value=strchr(value,'/'))) {
      if ((k=atoi(++value)) >= 0) Printer_Resolution=k;
      if ((value=strchr(value,'/'))) {
	 if ((k=atoi(++value))> 0) LPTNumber = k;
	 if (k < 0) {
	    Print_To_File = 1;
	    LPTNumber = 1;
	    }
	 }
      }
   return(0);
}



static void argerror(char *badarg)	/* oops. couldn't decode this */
{
   static char far argerrmsg1[]={"\
Oops. I couldn't understand the argument:\n  "};
   static char far argerrmsg2[]={"\n\n\
(see the Startup Help screens or documentation for a complete\n\
 argument list with descriptions)"};
   char msg[300];
   if (strlen(badarg) > 70) badarg[70] = 0;
   if (active_system == 0 /* DOS */
     && first_init)	  /* & this is 1st call to cmdfiles */
      sprintf(msg,"%Fs%s%Fs",argerrmsg1,badarg,argerrmsg2);
   else
      sprintf(msg,"%Fs%s",argerrmsg1,badarg);
   stopmsg(0,msg);
}

void set_3d_defaults()
{
   ENTER_OVLY(OVLY_CMDFILES);
   ROUGH     = 30;
   WATERLINE = 0;
   ZVIEWER   = 0;
   XSHIFT    = 0;
   YSHIFT    = 0;
   xtrans    = 0;
   ytrans    = 0;
   LIGHTAVG  = 0;
   Ambient   = 20;
   RANDOMIZE = 0;
   haze      = 0;
   back_color[0] = 51; back_color[1] = 153; back_color[2] = 200;
   if(SPHERE) {
      PHI1	=  180;
      PHI2	=  0;
      THETA1	=  -90;
      THETA2	=  90;
      RADIUS	=  100;
      FILLTYPE	= 2;
      XLIGHT	= 1;
      YLIGHT	= 1;
      ZLIGHT	= 1;
      }
   else {
      XROT	= 60;
      YROT	= 30;
      ZROT	= 0;
      XSCALE	= 90;
      YSCALE	= 90;
      FILLTYPE	= 0;
      if (active_system != 0)
	 FILLTYPE = 2;
      XLIGHT	= 1;
      YLIGHT	= -1;
      ZLIGHT	= 1;
      }
    EXIT_OVLY;
}
