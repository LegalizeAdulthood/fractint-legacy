/*
	Various routines that prompt for things.
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#ifndef __TURBOC__
#include <malloc.h>
#endif
#include "fractint.h"
#include "fractype.h"

struct fullscreenvalues
{
   int type;   /* 'd' for decimal, 's' for string, '*' for comment */
   union
   {
      double dval;
      char   sval[16];
   } uval;
};

/* Routines in this module	*/

extern	int fullscreen_prompt(char *hdg,int numprompts,char * far *prompts,struct fullscreenvalues values[],int options,int fkeymask);
extern	void prompts_overlay(void );
extern	int get_fracttype(void );
extern	int get_3d_params(void );
extern	int get_ifs_params(void );
extern	int get_toggles(void );
extern	int get_toggles2(void );
extern	int get_view_params(void );
extern	int get_starfield_params(void );
extern	void goodbye(void );
extern	int getafilename(char *hdg,char *template,char *flname);
extern	int select_video_mode(void);

static	int prompt_checkkey(int curkey);
static	int compare(void const *i,void const *j);
static	void clear_line(int row,int start,int stop,int color);
static	int select_fracttype(int t);
static	int get_fract_params(int newfractype,int oldfractype);
static	int get_lsys_name(void );
static	int get_formula_name(void );
static	int get_ifs3d_params(void );
static	int get_light_params(void );
static	int check_mapfile(void );
static	int get_funny_glasses_params(void );
static	int findfirst(char *path);
static	int findnext(void );
static	int lccompare(void const *, void const *);
static	int splitpath(char *template,char *drive,char *dir,char *fname,char *ext);
static	int makepath(char *template,char *drive,char *dir,char *fname,char *ext);
static	void fix_dirname(char *dirname);
static	int expand_dirname(char *dirname,char *drive);
static	int filename_speedstr(int, int, int, char *, int);
static	int isadirectory(char *s);
static	void format_item(int choice,char *buf);
static	int check_modekey(int curkey);


extern char *strig[];
extern int bailout;
extern int dotmode;

extern int fullscreen_choice(
	     int options, char *hdg, char *hdg2, char *instr, int numchoices,
	     char **choices, int *attributes, int boxwidth, int boxdepth,
	     int colwidth, int current,
	     void (*formatitem)(), int (*showdetail)(),
	     char *speedstring, int (*speedprompt)(), int (*checkkey)());
extern int strncasecmp(char *s,char *t,int ct);
extern int input_field(int options, int attr, char *fld,
	     int len, int row, int col, int (*checkkey)(int) );

/* options for fullscreen_prompt: */
#define PROMPTHELP 1

char funnyglasses_map_name[80];
extern char temp[], temp1[256];   /* temporary strings	      */
extern int mapset;
extern int previewfactor;
extern int xtrans, ytrans;
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern char showbox; /* flag to show box and vector in preview */
extern int debugflag;
extern int whichimage;
extern int xadjust;
extern int eyeseparation;
extern int glassestype;
extern findpath();
char   MAP_name[80] = "";
extern ValidateLuts();	/* read the palette file */
int    mapset = 0;
extern	int	overlay3d;	    /* 3D overlay flag: 0 = OFF */
extern	int	lookatmouse;
extern	int full_color;
extern	int haze;
extern	int RANDOMIZE;
extern	char light_name[];
extern	int Ambient;

extern	int	init3d[20];	/* '3d=nn/nn/nn/...' values */
extern	double	xxmin,xxmax;	/* initial corner values    */
extern	double	yymin,yymax;	/* initial corner values    */
extern	double	xx3rd,yy3rd;	/* initial corner values    */
extern	int	invert; 	/* non-zero if inversion active */
extern	double	inversion[3];	/* radius, xcenter, ycenter */
extern	long	fudge;		/* 2**fudgefactor	*/
extern	int	bitshift;	/* fudgefactor		*/
extern	double	param[4];	/* up to four parameters    */
extern	int	pot16bit;
extern	int	disk16bit;
extern	double	potparam[3];	/* three potential parameters*/
extern	int	fractype;	/* if == 0, use Mandelbrot  */
extern	char	usr_floatflag;	/* floating-point fractals? */
extern	int	maxit;		/* try this many iterations */
extern	int	inside; 	/* inside color */
extern	int	outside;	/* outside color */
extern	int	finattract;	/* finite attractor switch */
extern	char	savename[80];	/* save files using this name */
extern	char	ifsfilename[80];    /* IFS code file */
extern	char	ifs3dfilename[80];  /* IFS 3D code file */
extern	char	preview;	/* 3D preview mode flag */
extern	int	decomp[];	/* decomposition parameters */
extern	int	usr_distest;	/* distance estimator option */
extern	int	transparent[];	/* transparency values */
extern	char	usr_stdcalcmode; /* '1', '2', 'g', 'b' */
extern	char overwrite; 	/* overwrite= flag */
extern	int	soundflag;	/* sound option */
extern	int	LogFlag;	/* non-zero if logarithmic palettes */
extern	int	biomorph;	/* Biomorph flag */
extern	long	xmin, xmax, ymin, ymax; /* screen corner values */
extern	int	calc_status;	/* calc status: complete, resumable, ... */
extern	int	xdots, ydots;	/* coordinates of dots on the screen  */
extern	int	colors; 	/* maximum colors available */
extern	int	row, col;
extern	int	viewwindow;
extern	float	viewreduction;
extern	int	viewcrop;
extern	float	finalaspectratio;
extern	int	viewxdots,viewydots;
extern	void	aspectratio_crop(float,float);
extern	int	textcbase;
extern	int	textrow,textcol;
extern	int	resave_flag;	/* resaving after a timed save */
extern	char	boxy[];
extern	char	*fkeys[];
extern	int	kbdkeys[];
extern	int	video_type;
extern	int	adapter;

/* Define command keys */

#define   PAGE_UP	 1073
#define   PAGE_DOWN	 1081
#define   CTL_HOME	 1119
#define   CTL_END	 1117
#define   LEFT_ARROW	 1075
#define   RIGHT_ARROW	 1077
#define   UP_ARROW	 1072
#define   DOWN_ARROW	 1080
#define   LEFT_ARROW_2	 1115
#define   RIGHT_ARROW_2  1116
#define   UP_ARROW_2	 1141
#define   DOWN_ARROW_2	 1145
#define   HOME		 1071
#define   END		 1079
#define   ENTER 	 13
#define   ENTER_2	 1013
#define   ESC		 27
#define   SPACE 	 32
#define   F1		 1059
#define   F2		 1060
#define   F3		 1061
#define   F4		 1062
#define   F5		 1063
#define   F6		 1064
#define   F7		 1065
#define   F8		 1066
#define   F9		 1067
#define   F10		 1068

/* fullscreen_choice options */
#define CHOICERETURNKEY 1
#define CHOICEMENU	2
#define CHOICEHELP	4

/* speed key state values */
#define MATCHING	 0	/* string matches list - speed key mode */
#define TEMPLATE	-2	/* wild cards present - buiding template */
#define SEARCHPATH	-3	/* no match - building path search name */

#define   FILEATTR	 0x37	   /* File attributes; select all but volume labels */
#define   HIDDEN	 2
#define   SYSTEM	 4
#define   SUBDIR	 16
#define   MAXNUMFILES	 300

struct				   /* Allocate DTA and define structure */
{
     char path[21];			/* DOS path and filespec */
     char attribute;			/* File attributes wanted */
     int  ftime;			/* File creation time */
     int  fdate;			/* File creation date */
     long size; 			/* File size in bytes */
     char filename[13]; 		/* Filename and extension */
} DTA;				   /* Disk Transfer Area */

/* --------------------------------------------------------------------- */

char ifsmask[13]   = {"*.ifs"};
char ifs3dmask[13] = {"*.ifs"};
char gifmask[13]   = {""};
char formmask[13]  = {"*.frm"};
char lsysmask[13]  = {"*.l"};

static crtcols = 80;		/* constant for now */

void prompts_overlay() { }	/* for restore_active_ovly */


/* --------------------------------------------------------------------- */

static int promptfkeys;

int fullscreen_prompt(	/* full-screen prompting routine */
	char *hdg,		/* heading, lines separated by \n */
	int numprompts, 	/* there are this many prompts (max) */
	char * far *prompts,	/* array of prompting pointers */
	struct fullscreenvalues values[], /* array of values */
	int options,		/* help available, more bits for future */
	int fkeymask		/* bit n on if Fn to cause return */
	)
{
   char temp2[24][16];		/* string value of answers go here */
   char *hdgscan;
   int titlelines,titlewidth,titlerow;
   int promptwidth;
   int boxrow,boxlines;
   int boxcol,boxwidth;
   int promptrow,promptcol,valuecol;
   int curchoice;
   int done, i, j;
   int savelookatmouse;
static char far instr1[]  = {"Use the cursor keys to select values to change"};
static char far instr2[]  = {"Type in any replacement values you wish to use"};
static char far instr3a[] = {"Press ENTER when finished (or ESCAPE to back out)"};
static char far instr3b[] = {"Press ENTER when finished, ESCAPE to back out, or F1 for help"};

   ENTER_OVLY(OVLY_PROMPTS);

   if (numprompts <= 0) {	/* ?? nothing to do! */
      EXIT_OVLY;
      return(0);
      }

   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   promptfkeys = fkeymask;
   helptitle(); 		       /* clear screen, display title line  */
   setattr(1,0,C_PROMPT_BKGRD,24*80);  /* init rest of screen to background */

   hdgscan = hdg;		       /* count title lines, find widest */
   i = titlewidth = 0;
   titlelines = 1;
   while (*hdgscan) {
      if (*(hdgscan++) == '\n') {
	 ++titlelines;
	 i = -1;
	 }
      if (++i > titlewidth)
	 titlewidth = i;
      }

   i = numprompts + titlelines + 3;    /* total number of rows required */
   j = (25 - i) / 2;		       /* top row of it all when centered */
   if (i == 22) j = 2;		       /* adjust this particular case */
   j -= j / 4;			       /* higher is better if lots extra */
   boxlines = numprompts;
   titlerow = 1 + j;
   promptrow = boxrow = titlerow + titlelines;
   if (titlerow > 2) {		       /* room for blank between title & box? */
      --titlerow;
      --boxrow;
      ++boxlines;
      }
   if (boxrow + boxlines < 22)	       /* room for blank at bottom of box? */
      ++boxlines;

   promptwidth = 0;		       /* find the widest prompt */
   for (i = 0; i < numprompts; i++) {
      if (promptwidth < strlen(prompts[i]))
	 promptwidth = strlen(prompts[i]);
      }
   if ((boxwidth = 2 + promptwidth + 4 + 15 + 2) > 80)
      boxwidth = 80;
   boxcol = (80 - boxwidth) / 2;       /* center the box */
   boxcol -= (90 - boxwidth) / 20;
   promptcol = boxcol + 2;
   valuecol = boxcol + boxwidth - 17;
   if (boxwidth <= 76) {	       /* make margin a bit wider if we can */
      boxwidth += 2;
      --boxcol;
      }
   if ((i = titlewidth + 2 - boxwidth) > 0) { /* expand box for title */
      if (boxwidth + i > 80)
	 i = 80 - boxwidth;
      boxwidth += i;
      boxcol -= i / 2;
      }

   for (i = titlerow; i < boxrow; ++i) /* display the heading */
      setattr(i,boxcol,C_PROMPT_HI,boxwidth);
   textcbase = (80 - titlewidth) / 2;  /* set left margin for putstring */
   textcbase -= (90 - titlewidth) / 20;
   putstring(titlerow,0,C_PROMPT_HI,hdg);
   textcbase = 0;

   i = boxrow + boxlines;	       /* display the footing */
   if (i < 22) ++i;
   if (numprompts > 1)
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,instr1);
   putstringcenter(i++,0,80,C_PROMPT_BKGRD,instr2);
   putstringcenter(i,  0,80,C_PROMPT_BKGRD,
	 (options & PROMPTHELP) ? instr3b : instr3a);

   for (i = 0; i < boxlines; ++i)      /* display empty box */
      setattr(boxrow+i,boxcol,C_PROMPT_LO,boxwidth);
   for (i = 0; i < numprompts; i++) {  /* display initial values */
      putstring(promptrow+i, promptcol, C_PROMPT_LO, prompts[i]);
      if (values[i].type      == 'd')
	 sprintf(temp2[i], "%-10g", values[i].uval.dval);
      else if (values[i].type == '*')
	 temp2[i][0] = 0;
      else		   /* == 's' */
	 sprintf(temp2[i], "%-10s",  values[i].uval.sval);
      putstring(promptrow+i, valuecol, C_PROMPT_LO, temp2[i]);
      }

   curchoice = done = 0;
   while (values[curchoice].type == '*') ++curchoice;

   while (!done) {

      putstring(promptrow+curchoice, promptcol, C_PROMPT_HI, prompts[curchoice]);
      putstring(promptrow+curchoice, valuecol, C_PROMPT_INPUT, "               ");

      i = input_field( (values[curchoice].type == 'd') ? 1 : 0,
	     C_PROMPT_INPUT, temp2[curchoice], 15,
	     promptrow+curchoice,valuecol,prompt_checkkey);

      putstring(promptrow+curchoice, promptcol, C_PROMPT_LO, prompts[curchoice]);
      putstring(promptrow+curchoice, valuecol, C_PROMPT_LO,  "               ");
      putstring(promptrow+curchoice, valuecol, C_PROMPT_LO,  temp2[curchoice]);

      switch(i) {
	 case 0:  /* enter  */
	    done = 13;
	    break;
	 case -1: /* escape */
	 case F2:
	 case F3:
	 case F4:
	 case F5:
	 case F6:
	 case F7:
	 case F8:
	 case F9:
	 case F10:
	    done = i;
	    break;
	 case PAGE_UP:
	    curchoice = -1;
	 case DOWN_ARROW:
	 case DOWN_ARROW_2:
	    do {
	       if (++curchoice >= numprompts) curchoice = 0;
	       } while (values[curchoice].type == '*');
	    break;
	 case PAGE_DOWN:
	    curchoice = numprompts;
	 case UP_ARROW:
	 case UP_ARROW_2:
	    do {
	       if (--curchoice < 0) curchoice = numprompts - 1;
	       } while (values[curchoice].type == '*');
	    break;
	 }
      }

   if (done >= 0)
      for (i = 0; i < numprompts; i++) {	/* fill in the results */
	 if (values[i].type == 'd')
	    values[i].uval.dval = atof(temp2[i]);
	 else
	   strncpy(values[i].uval.sval,temp2[i],16);
	 }
   movecursor(25,80);
   lookatmouse = savelookatmouse;
   EXIT_OVLY;
   return(done);
}

static int prompt_checkkey(int curkey)
{
   switch(curkey) {
      case PAGE_UP:
      case DOWN_ARROW:
      case DOWN_ARROW_2:
      case PAGE_DOWN:
      case UP_ARROW:
      case UP_ARROW_2:
	 return(curkey);
      case F2:
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
      case F10:
	 if (promptfkeys & (1<<(curkey+1-F1)) )
	    return(curkey);
      }
   return(0);
}


/* --------------------------------------------------------------------- */

/* compare for sort of type table */
static int compare(const void *i, const void *j)
{
   return(strcmp(fractalspecific[(int)*((unsigned char*)i)].name,
	       fractalspecific[(int)*((unsigned char*)j)].name));
}

/* --------------------------------------------------------------------- */

static void clear_line(int row, int start, int stop, int color) /* clear part of a line */
{
   int col;
   for(col=start;col<= stop;col++)
      putstring(row,col,color," ");
}

/* --------------------------------------------------------------------- */

int get_fracttype()		/* prompt for and select fractal type */
{
   int done,i,oldfractype,t;
   ENTER_OVLY(OVLY_PROMPTS);
   done = -1;
   oldfractype = fractype;
   while (1) {
      if ((t = select_fracttype(fractype)) < 0)
	 break;
      if ((i = get_fract_params(t, fractype)) == 0) { /* ok, all done */
	 done = 0;
	 break;
	 }
      if (i > 0) /* can't return to prior image anymore */
	 done = 1;
      }
   if (done < 0)
      fractype = oldfractype;
   EXIT_OVLY;
   return(done);
}

static int select_fracttype(int t) /* subrtn of get_fracttype, separated */
				   /* so that storage gets freed up	 */
{
   int oldhelpmode;
   int numtypes, done;
   int i, j;
#define MAXFTYPES 200
   char tname[40];
   struct CHOICE {
      char name[15];
      int  num;
      }  *choices[MAXFTYPES];
   int attributes[MAXFTYPES];

   /* steal existing array for "choices" */
   choices[0] = (struct CHOICE *)boxy;
   attributes[0] = 1;
   for (i = 1; i < MAXFTYPES; ++i) {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
      }

   /* setup context sensitive help */
   oldhelpmode = helpmode;
   helpmode = HELPFRACTALS;

   i = j = -1;
   while(fractalspecific[++i].name) {
      if (fractalspecific[i].name[0] == '*')
	 continue;
      strcpy(choices[++j]->name,fractalspecific[i].name);
      choices[j]->name[14] = 0; /* safety */
      choices[j]->num = i;	/* remember where the real item is */
      }
   numtypes = j + 1;
   qsort(choices,numtypes,sizeof(char *),lccompare); /* sort list */
   j = 0;
   for (i = 0; i < numtypes; ++i) /* find starting choice in sorted list */
      if (choices[i]->num == t || choices[i]->num == fractalspecific[t].tofloat)
	 j = i;

   tname[0] = 0;
   done = fullscreen_choice(CHOICEHELP,"Select a Fractal Type",NULL,NULL,
			    numtypes,(char **)choices,attributes,0,0,0,j,
			    NULL,NULL,tname,NULL,NULL);
   if (done >= 0)
      done = choices[done]->num;
   helpmode = oldhelpmode;
   return(done);
}

extern char LFileName[];	/* file to find the formulas in */
extern char LName[];		/* Name of the Formula (if not null) */
extern char FormFileName[];	/* file to find the formulas in */
extern char FormName[]; 	/* Name of the Formula (if not null) */

static int get_fract_params(	/* prompt for fractal parameters */
	int newfractype,	/* new fractal type */
	int oldfractype 	/* previous fractal type */
	)
{
int numparams,numtrig;
int i,ret;
struct fullscreenvalues paramvalues[10];
char *choices[10];
int oldbailout;
int promptnum;
char msg[81];

static char *trg[] = {"First Function","Second Function",
		      "Third Function","Fourth Function"};

fractype = newfractype;
ret = -1;

if (fractype == LSYSTEM) {
   ret = 1; /* about to pass point of no return to prior image */
   if (getafilename("Select L-System File",lsysmask,LFileName) < 0
     || get_lsys_name() < 0)
      return(1);
   }

if (fractype == FORMULA || fractype == FFORMULA) {
   ret = 1; /* about to pass point of no return to prior image */
   if (getafilename("Select Formula File",formmask,FormFileName) < 0
     || get_formula_name() < 0)
      return(1);
   }

 for ( i = 0; i < 4; i++)
 {
	 paramvalues[i].type = 'd';
	 paramvalues[i].uval.dval = fractalspecific[fractype].paramvalue[i];
 }
for (numparams = 0; numparams < 4; numparams++) {
	if (fractalspecific[fractype].param[numparams][0] == 0) break;
	choices[numparams] = fractalspecific[fractype].param[numparams];
	}
 numtrig = (fractalspecific[fractype].flags >> 6) & 7;

  for(i=0; i<numtrig; i++)
  {
    paramvalues[i+numparams].type = 's';
    strcpy(paramvalues[i+numparams].uval.sval,trigfn[trigndx[i]].name);
    choices[i+numparams] = trg[i];
  }
  if(fabs(potparam[2]) == 0.0
   && fractalspecific[fractype].orbit_bailout)
  {
     promptnum = numparams+numtrig+1;
     choices[numparams+numtrig] = "Bailout value (0 means use default)";
     paramvalues[numparams+numtrig].type = 'd';
     paramvalues[numparams+numtrig].uval.dval = oldbailout = bailout;
  }
  else	/* under these conditions bailout is ignored, so why ask for it? */
     promptnum = numparams+numtrig;

 sprintf(msg,"Options for fractal type %s",
	     (fractalspecific[fractype].name[0] != '*')
	      ? fractalspecific[fractype].name
	      : &fractalspecific[fractype].name[1] );
 if (fullscreen_prompt(msg,promptnum,choices,paramvalues,0,0) < 0)
    return(ret);

 for ( i = 0; i < 4; i++)
	 param[i] = fractalspecific[fractype].paramvalue[i];
 for ( i = 0; i < numparams; i++)
	 param[i] = paramvalues[i].uval.dval;
 for ( i = 0; i < numtrig; i++)
 {
    paramvalues[i+numparams].uval.sval[4] = 0;
    if(paramvalues[i+numparams].uval.sval[3]==32)
	paramvalues[i+numparams].uval.sval[3] = 0;
    set_trig_array(i, paramvalues[i+numparams].uval.sval);
 }

  if ((potparam[0] == 0.0 || potparam[2] == 0.0)
   && fractalspecific[fractype].orbit_bailout)
  {
     bailout = paramvalues[numparams+numtrig].uval.dval;

     if (bailout != 0 && (bailout < 4 || bailout > 32000))
	bailout = oldbailout;
  }

 if (newfractype != oldfractype) {
	 invert = 0;
	 inversion[0] = inversion[1] = inversion[2] = 0;
	 }

xxmin = fractalspecific[fractype].xmin;
xxmax = fractalspecific[fractype].xmax;
yymin = fractalspecific[fractype].ymin;
yymax = fractalspecific[fractype].ymax;
xx3rd = xxmin;
yy3rd = yymin;
for ( i = 0; i < 4; i++)
   fractalspecific[newfractype].paramvalue[i] = param[i];
if (viewcrop && finalaspectratio != SCREENASPECT)
   aspectratio_crop(SCREENASPECT,finalaspectratio);

return(0);

}

/* --------------------------------------------------------------------- */

static int get_lsys_name()	/* get the Lsystem formula name */
{
   int Litem;
#define MAXLSYS 100
   int numformulas, i;
   FILE *File;
   char *choices[MAXLSYS];
   int attributes[MAXLSYS];
   char buf[201];

   choices[0] = boxy;
   attributes[0] = 1;
   for (i = 1; i < MAXLSYS; i++) {
      attributes[i] = 1;
      choices[i] = choices[i-1]+21;
      }

   if ((File = fopen(LFileName, "rt")) == NULL) {
      sprintf(buf,"I Can't find %s", LFileName);
      stopmsg(1,buf);
      LName[0] = 0;
      return(-1);
      }

   numformulas = 0;
   while (1) {
      int c;
      *choices[numformulas] = 0;
      if (fscanf(File, " %20[^ \n\t({]", choices[numformulas]) == EOF)
	 break;
      while(c = getc(File)) {
	 if(c == EOF || c == '{' || c == '\n')
	    break;
	 }
      if(c == EOF)
	 break;
      else if(c != '\n') {
skipcomments:
	 if(fscanf(File, "%200[^}]", buf) == EOF) break;
	 if (getc(File) != '}') goto skipcomments;
	 if (stricmp(choices[numformulas],"") != 0 &&
	     stricmp(choices[numformulas],"comment") != 0)
		 if (++numformulas >= MAXLSYS) break;
	 }
      }
   fclose(File);

   qsort(choices,numformulas,sizeof(char *),lccompare); /* sort type list */
   do {
      strcpy(buf,LName); /* preset to last choice made */
      if ((Litem = fullscreen_choice(0,"L-System Formula Selection",NULL,NULL,
				  numformulas,choices,attributes,
				  0,0,0,0,NULL,NULL,buf,NULL,NULL)) < 0)
	 return(-1);
      strcpy(LName, choices[Litem]);
      } while (LLoad());
   return(0);
}

/* --------------------------------------------------------------------- */

/*
  Read a formula file, picking off the formula names.
  Formulas use the format "  name = { ... }  name = { ... } "
*/

static int get_formula_name()	 /* get the fractal formula name */
{
#define MAXFORMULA 100
   int numformulas, i;
   FILE *File;
   char *choices[MAXFORMULA];
   int attributes[MAXFORMULA];
   char buf[201];

   choices[0] = boxy;
   attributes[0] = 1;
   for (i = 1; i < MAXFORMULA; i++) {
      attributes[i] = 1;
      choices[i] = choices[i-1]+21;
      }

   if ((File = fopen(FormFileName, "rt")) == NULL) {
      sprintf(buf,"I Can't find %s", FormFileName);
      stopmsg(1,buf);
      FormName[0] = 0;
      return(-1);
   }

   numformulas = 0;
   while (1) {
      int c;
      *choices[numformulas] = 0;
      if (fscanf(File, " %20[^ \n\t({]", choices[numformulas]) == EOF)
	 break;
      while(c = getc(File)) {
	 if(c == EOF || c == '{' || c == '\n')
	    break;
      }
      if(c == EOF)
	 break;
      else if(c != '\n'){
skipcomments:
	 if(fscanf(File, "%200[^}]", buf) == EOF) break;
	 if (getc(File) != '}') goto skipcomments;
	 if (stricmp(choices[numformulas],"") != 0 &&
	     stricmp(choices[numformulas],"comment") != 0)
		 if (++numformulas >= MAXFORMULA) break;
      }
   }
   fclose(File);

   qsort(choices,numformulas,sizeof(char *),lccompare); /* sort type list */
   do {
      strcpy(buf,FormName); /* preset to last choice made */
      if ((i = fullscreen_choice(0,"Formula Selection",NULL,NULL,
			      numformulas,choices,attributes,
			      0,0,0,0,NULL,NULL,buf,NULL,NULL)) < 0)
	 return(-1);
      strcpy(FormName, choices[i]);
      } while (RunForm(FormName));

return(0);
}

/* --------------------------------------------------------------------- */

static int get_ifs3d_params()	/* prompt for 3D IFS parameters */
{
   char *s;
int k;
static char *ifs3d_prompts[] = {
   "X-axis rotation in degrees",
   "Y-axis rotation in degrees",
   "Z-axis rotation in degrees",
   "Perspective distance [1 - 999, 0 for no persp]",
   "X shift with perspective (positive = right)",
   "Y shift with perspective (positive = up   )",
   "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo)",
   ""
   };
   struct fullscreenvalues uvalues[20];

   k = 0;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = XROT;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = YROT;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = ZROT;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = ZVIEWER;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = XSHIFT;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = YSHIFT;
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = glassestype;

   if (fullscreen_prompt("3D Parameters",k,ifs3d_prompts,uvalues,0,0) < 0)
      return(-1);
   k = 0;
   XROT    =  uvalues[k++].uval.dval;
   YROT    =  uvalues[k++].uval.dval;
   ZROT    =  uvalues[k++].uval.dval;
   ZVIEWER =  uvalues[k++].uval.dval;
   XSHIFT  =  uvalues[k++].uval.dval;
   YSHIFT  =  uvalues[k++].uval.dval;
   glassestype = uvalues[k++].uval.dval;
   if (glassestype)
      if (get_funny_glasses_params() || check_mapfile())
	 return(-1);
   return(0);
}

/* --------------------------------------------------------------------- */

int get_3d_params()	/* prompt for 3D parameters */
{
   char *choices[10];
   int attributes[10];
   char c;
   int numprompts;
   int sphere,filltype;
   char *s;
   char *prompts3d[25];
   struct fullscreenvalues uvalues[25];
   int i, j, k;

   ENTER_OVLY(OVLY_PROMPTS);

restart_1:

   k=-1;

   prompts3d[++k]= "Preview Mode?                        (yes or no)";
   uvalues[k].type = 's';
   if(preview)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   prompts3d[++k]= "    Show Box?                        (yes or no)";
   uvalues[k].type = 's';
   if(showbox)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   prompts3d[++k]= "Coarseness, preview/grid (in y dir)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = previewfactor;

   prompts3d[++k]= "Spherical Projection?                (yes or no)";
   uvalues[k].type = 's';
   if(sphere = SPHERE)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   prompts3d[++k]= "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = glassestype;

   if (fullscreen_prompt("3D Mode Selection",k+1,prompts3d,uvalues,0,0) < 0) {
      EXIT_OVLY;
      return(-1);
      }

   k=0;

   if((c = *(uvalues[k++].uval.sval))=='y' || c == 'Y')
      preview = 1;
   else
      preview = 0;

   if((c = *(uvalues[k++].uval.sval))=='y' || c == 'Y')
      showbox = 1;
   else
      showbox = 0;

   previewfactor  = uvalues[k++].uval.dval;

   if((c = *(uvalues[k++].uval.sval))=='y' || c == 'Y')
      sphere = 1;
   else
      sphere = 0;

   glassestype = uvalues[k++].uval.dval;

   /* check ranges */
   if(preview)
      preview = 1;
   else
      preview = 0;

   if(previewfactor < 8)
      previewfactor = 8;
   else if(previewfactor > 128)
      previewfactor = 128;

   if(sphere && !SPHERE)
   {
      SPHERE = TRUE;
      set_3d_defaults();
   }
   else if(!sphere && SPHERE)
   {
      SPHERE = FALSE;
      set_3d_defaults();
   }

   if(glassestype < 0)
      glassestype = 0;
   else if(glassestype > 3)
      glassestype = 3;
   if(glassestype)
      whichimage = 1;

   k=0;
   choices[k++] = "make a surface grid";
   choices[k++] = "just draw the points";
   choices[k++] = "connect the dots (wire frame)";
   choices[k++] = "surface fill (colors interpolated)";
   choices[k++] = "surface fill (colors not interpolated)";
   choices[k++] = "solid fill (bars up from \"ground\")";
   if(SPHERE)
      choices[k++] = "light source";
   else
   {
      choices[k++] = "light source before transformation";
      choices[k++] = "light source after transformation";
   }

   for (i = 0; i < k; ++i)
      attributes[i] = 1;
   if ((i = fullscreen_choice(0,"Select 3D Fill Type",NULL,NULL,
			      k,choices,attributes,
			      0,0,0,FILLTYPE+1,NULL,NULL,NULL,NULL,NULL)) < 0)
      goto restart_1;
   FILLTYPE = i-1;

   if(glassestype)
   {
      if(get_funny_glasses_params())
	 goto restart_1;
   }

   if (check_mapfile())
      goto restart_1;

restart_3:

   if(SPHERE)
   {
      prompts3d[0] = "Longitude start (degrees)";
      prompts3d[1] = "Longitude stop  (degrees)";
      prompts3d[2] = "Latitude start  (degrees)";
      prompts3d[3] = "Latitude stop   (degrees)";
      prompts3d[4] = "Radius scaling factor in pct";
   }
   else
   {
      prompts3d[0] = "X-axis rotation in degrees";
      prompts3d[1] = "Y-axis rotation in degrees";
      prompts3d[2] = "Z-axis rotation in degrees";
      prompts3d[3] = "X-axis scaling factor in pct";
      prompts3d[4] = "Y-axis scaling factor in pct";
   }
   uvalues[0].uval.dval   = XROT      ;
   uvalues[0].type = 'd';
   uvalues[1].uval.dval   = YROT      ;
   uvalues[1].type = 'd';
   uvalues[2].uval.dval   = ZROT      ;
   uvalues[2].type = 'd';
   uvalues[3].uval.dval   = XSCALE    ;
   uvalues[3].type = 'd';
   uvalues[4].uval.dval   = YSCALE    ;
   uvalues[4].type = 'd';
   k = 5;
   prompts3d[k] = "Surface Roughness scaling factor in pct";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = ROUGH     ;

   prompts3d[k]= "'Water Level' (minimum color value)";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = WATERLINE ;

   prompts3d[k]= "Perspective distance [1 - 999, 0 for no persp]";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = ZVIEWER   ;

   prompts3d[k]= "X shift with perspective (positive = right)";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = XSHIFT    ;

   prompts3d[k]= "Y shift with perspective (positive = up   )";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = YSHIFT    ;

   prompts3d[k]= "Image non-perspective X adjust (positive = right)";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = xtrans    ;

   prompts3d[k]= "Image non-perspective Y adjust (positive = up)";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = ytrans    ;

   prompts3d[k]= "First transparent color";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = transparent[0];

   prompts3d[k]= "Last transparent color";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = transparent[1];

   prompts3d[k]= "Randomize Colors      (0 - 7, '0' disables)";
   uvalues[k].type = 'd';
   uvalues[k++].uval.dval = RANDOMIZE;

   if (ILLUMINE)
   {
	prompts3d[k]= "Color/Mono Images With Light Source   (1 = Color)";
	uvalues[k].type = 'd';
	uvalues[k++].uval.dval = full_color;
   }

   if (SPHERE)
      s = "              Sphere 3D Parameters\n\
Sphere is on its side; North pole to right\n\
Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right";
   else
      s = "              Planar 3D Parameters\n\
Pre-rotation X axis is screen top; Y axis is left side\n\
Pre-rotation Z axis is coming at you out of the screen!";

   if (fullscreen_prompt(s,k,prompts3d,uvalues,0,0) < 0)
      goto restart_1;

   k = 0;
   XROT       = uvalues[k++].uval.dval;
   YROT       = uvalues[k++].uval.dval;
   ZROT       = uvalues[k++].uval.dval;
   XSCALE     = uvalues[k++].uval.dval;
   YSCALE     = uvalues[k++].uval.dval;
   ROUGH      = uvalues[k++].uval.dval;
   WATERLINE  = uvalues[k++].uval.dval;
   ZVIEWER    = uvalues[k++].uval.dval;
   XSHIFT     = uvalues[k++].uval.dval;
   YSHIFT     = uvalues[k++].uval.dval;
   xtrans     = uvalues[k++].uval.dval;
   ytrans     = uvalues[k++].uval.dval;
   transparent[0] = uvalues[k++].uval.dval;
   transparent[1] = uvalues[k++].uval.dval;
   RANDOMIZE  = uvalues[k++].uval.dval;
   if (RANDOMIZE >= 7) RANDOMIZE = 7;
   if (RANDOMIZE <= 0) RANDOMIZE = 0;

   if (ILLUMINE)
   {
	full_color = uvalues[k++].uval.dval;
	if(get_light_params())
	    goto restart_3;
   }

EXIT_OVLY;
return(0);
}

/* --------------------------------------------------------------------- */
static int get_light_params()
{
   char *prompts3d[10];
   double values[10];
   struct fullscreenvalues uvalues[10];
   char *s;
   int k;

   /* defaults go here */

   k = -1;
   prompts3d[++k]= "X value light vector";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = XLIGHT    ;

   prompts3d[++k]= "Y value light vector";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = YLIGHT    ;

   prompts3d[++k]= "Z value light vector";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = ZLIGHT    ;

   prompts3d[++k]= "Light Source Smoothing Factor";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = LIGHTAVG  ;

   prompts3d[++k]= "Ambient Light      (0 - 100, '0' = 'Black' shadows";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = Ambient;

   if (full_color)
   {
	prompts3d[++k]	= "Haze Factor        (0 - 100, '0' disables)";
	uvalues[k].type = 'd';
	uvalues[k].uval.dval= haze;

	prompts3d[++k]= "Full Color Light File Name (if not light001.tga)";
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval,light_name);
   }

   prompts3d[++k]= "";

   if (fullscreen_prompt("Light Source Parameters",
			 k,prompts3d,uvalues,0,0) < 0)
      return(-1);

   k = 0;
      XLIGHT   = uvalues[k++].uval.dval;
      YLIGHT   = uvalues[k++].uval.dval;
      ZLIGHT   = uvalues[k++].uval.dval;
      LIGHTAVG = uvalues[k++].uval.dval;
      Ambient  = uvalues[k++].uval.dval;
      if (Ambient >= 100) Ambient = 100;
      if (Ambient <= 0) Ambient = 0;
   if (full_color)
   {
	haze  =  uvalues[k++].uval.dval;
	if (haze >= 100) haze = 100;
	if (haze <= 0) haze = 0;
	strcpy(light_name,uvalues[k].uval.sval);
   }
   return(0);
}

/* --------------------------------------------------------------------- */


static int check_mapfile()
{
    char msg[81];
    strcpy(temp1,"*");
    if (mapset)
       strcpy(temp1,MAP_name);
    while ((FILLTYPE >= 5) || overlay3d || (mapset == 1) || glassestype) {
       if (glassestype == 0 || glassestype == 3) {
	  if (field_prompt(0,"\
Enter name of .MAP file to use,\n\
or '*' to use palette from the image about to be loaded.",
		  NULL,temp1,60,NULL) < 0)
	     return(-1);
	  if (temp1[0] == '*') {
	     mapset = 0;
	     break;
	     }
	  }
       else
	  strcpy(temp1,funnyglasses_map_name);
       if (ValidateLuts(temp1) != 0) { /* Oops, somethings wrong */
	  if (glassestype == 2) {
	     mapset = 2; /* Need to generate glasses2.map */
	     break;
	     }
	  if (glassestype == 1) {
	     mapset = 3; /* Need to generate glasses1.map */
	     break; /* compute palette on the fly goes here */
	     }
	  sprintf (msg,"Sorry, cannot open %s", temp1);
	  stopmsg(1,msg);
	  continue;
	  }
       /* File OK */
       mapset = 1;
       strcpy (MAP_name,temp1);
       break; /* Done */
       }
    return(0);
}

static int get_funny_glasses_params()
{
   char *prompts3d[10];
   double values[10];
   struct fullscreenvalues uvalues[10];
   char *s;
   int k;

   /* defaults */
   if(ZVIEWER == 0)
      ZVIEWER = 150;
   if(eyeseparation == 0)
   {
      if(fractype==IFS3D || fractype==LLORENZ3D || fractype==FPLORENZ3D)
      {
	 eyeseparation =  2;
	 xadjust       = -2;
      }
      else
      {
	 eyeseparation =  3;
	 xadjust       =  0;
      }
   }

   if(glassestype == 1)
      strcpy(funnyglasses_map_name,"glasses1.map");
   else if(glassestype == 2)
   {
      if(FILLTYPE == -1)
	 strcpy(funnyglasses_map_name,"grid.map");
      else
	 strcpy(funnyglasses_map_name,"glasses2.map");
   }

   k = -1;
   prompts3d[++k]  = "Interocular distance (as % of screen)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval= eyeseparation;

   prompts3d[++k]= "Convergence adjust (positive = spread greater)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = xadjust;

   prompts3d[++k]= "Left  red image crop (% of screen)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = red_crop_left;

   prompts3d[++k]= "Right red image crop (% of screen)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = red_crop_right;

   prompts3d[++k]= "Left  blue image crop (% of screen)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = blue_crop_left;

   prompts3d[++k]= "Right blue image crop (% of screen)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = blue_crop_right;

   prompts3d[++k]= "Red brightness factor (%)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = red_bright;

   prompts3d[++k]= "Blue brightness factor (%)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = blue_bright;

   if(glassestype != 3)
   {
      prompts3d[++k]= "Map File name (if not default)";
      uvalues[k].type = 's';
      strcpy(uvalues[k].uval.sval,funnyglasses_map_name);
   }

   if (fullscreen_prompt("Funny Glasses Parameters",
			 k+1,prompts3d,uvalues,0,0) < 0)
      return(-1);

   k = 0;
   eyeseparation   =  uvalues[k++].uval.dval;
   xadjust	   =  uvalues[k++].uval.dval;
   red_crop_left   =  uvalues[k++].uval.dval;
   red_crop_right  =  uvalues[k++].uval.dval;
   blue_crop_left  =  uvalues[k++].uval.dval;
   blue_crop_right =  uvalues[k++].uval.dval;
   red_bright	   =  uvalues[k++].uval.dval;
   blue_bright	   =  uvalues[k++].uval.dval;

   if(glassestype != 3)
      strcpy(funnyglasses_map_name,uvalues[k].uval.sval);
   return(0);
}

/* --------------------------------------------------------------------- */

int get_ifs_params()		/* prompt for IFS params */
{
   char *choices[6];
   int attributes[6];
   char *filename;
   char *maskname;
   float far *initarray;
   char ifstype;
   int totrows, totcols;
   int i, j, k, numlines;
   FILE *tempfile;
   char msg[81];
   int oldhelpmode;
   int ret;

   ENTER_OVLY(OVLY_PROMPTS);

   ret = 0;
   stackscreen();		/* switch to text mode */
   oldhelpmode = helpmode;
   helpmode = -1;

 /* PB, removed next 2 to retain modified values
   ifsgetfile();
   ifs3dgetfile();
  */

   choices[0] = "2D IFS Codes";
   choices[1] = "3D IFS Codes";
   choices[2] = "3D Transform Parameters";
   for (i = 0; i < 3; ++i) attributes[i] = 0;
   if(fractype == IFS)
      j = 0;
   else if (fractype == IFS3D)
      j = 1;
   else
      j = 2;

   i = fullscreen_choice(0,"IFS and 3D Parameters",NULL,NULL,3,choices,attributes,
			 0,0,0,j,NULL,NULL,NULL,NULL,NULL);
   switch(i) {
      case 0:
	 ifstype = '2';
	 filename = ifsfilename;
	 maskname = ifsmask;
	 initarray = &initifs[0][0];
	 totrows = NUMIFS;
	 totcols = IFSPARM;
	 break;
      case 1:
	 ifstype = '3';
	 maskname = ifs3dmask;
	 filename = ifs3dfilename;
	 initarray = &initifs3d[0][0];
	 totrows = NUMIFS;
	 totcols = IFS3DPARM;
	 break;
      case 2:
	 ret = get_ifs3d_params();
	 goto get_ifs_exit;
      default:
	 ret = -1;
	 goto get_ifs_exit;
      }

   /* edit table */

   if (fractype != IFS && fractype != IFS3D)
      ret = -1; /* PB: this is ugly, I haven't really made this whole   */
		/*     routine smart enough, but it works out tolerably */

   for ( ;; ) {
static char far ifshdg2[]={"2D IFS Parameters"};
static char far ifshdg3[]={"3D IFS Parameters"};
static char far ifsparmmsg1[]={"#    a     b     c     d     e     f"};
static char far ifsparmmsg2[]={"     g     h     i     j     k     l"};
static char far ifsprompt[]={"\
Enter the number of the line you want to edit\n\
or R to start from another (.IFS) file, or S to\n\
save your edits in a file, or ENTER to end ==>"};
      int leftcol,promptrow,promptcol;

      for (numlines = 0; numlines < totrows; numlines++) /* find the first zero entry */
	 if (initarray[(numlines * totcols) + totcols - 1] <= 0.0001) break;

      helptitle();
      setattr(1,0,C_PROMPT_BKGRD,24*80); /* init rest of screen to background */
      putstringcenter(2,0,80,C_GENERAL_HI,(ifstype == '2') ? ifshdg2 : ifshdg3);
      leftcol = (ifstype == '2') ? 15 : 0;
      putstring(4,leftcol+1,C_GENERAL_HI,ifsparmmsg1);
      if (ifstype == '3')
	 putstring(-1,-1,C_GENERAL_HI,ifsparmmsg2);
      putstring(-1,-1,C_GENERAL_HI,"   prob \n\n");

      if ((k = numlines) > 12) k = 12;
      for (i = 0; i < k; i++) {
	 sprintf(msg,"%2d", i+1);
	 putstring(5+i,leftcol,C_GENERAL_HI,msg);
	 for (j = 0; j < totcols; j++) {
	    sprintf(msg,"%6.2f", (float )initarray[(i*totcols)+j]);
	    putstring(-1,-1,C_GENERAL_MED,msg);
	    }
	 }

      textcbase = 14;
      putstring(5+i+1,0,C_GENERAL_HI,ifsprompt);
      promptrow = textrow;
      promptcol = textcol + textcbase + 1;
      temp1[0] = textcbase = 0;
      if (input_field(0,C_GENERAL_INPUT,temp1,2,promptrow,promptcol,NULL) < 0
	|| temp1[0] == 0)
	 break; /* ESCAPE or ENTER */

      putstring(promptrow,promptcol,C_GENERAL_HI,temp1);
      if (temp1[0] == 's' || temp1[0] == 'S') {
	 stackscreen();
	 i = field_prompt(0,"Enter the name of the .IFS file to save:",
			 NULL,filename,60,NULL);
	 unstackscreen();
	 if (i != -1) {
	    if (strchr(filename,'.') == NULL)
	       strcat(filename,".ifs");
	    if ((tempfile=fopen(filename,"w")) != NULL) {
	       for (i = 0; i < numlines; i++) {
		  for (j = 0; j < totcols; j++)
		     fprintf(tempfile, "%6.2f", (float)initarray[(i*totcols)+j]);
		  fprintf(tempfile, "\n");
		  }
	       fclose(tempfile);
	       }
	    else
	       stopmsg(0,"Could not create file");
	    }
	 continue;
	 }
      if (temp1[0] == 'r' || temp1[0] == 'R') {
	 if (getafilename("Select IFS File to Edit",maskname,filename) >= 0)
	    if (ifstype == '3')
	       ifs3dgetfile();
	    else
	       ifsgetfile();
	 continue;
	 }
      i = atoi(temp1) - 1;
      if (i >= 0 && i <= numlines) {
	 for (j = 0; j < totcols; j++) {
	    if(j < totcols-1)
		    sprintf(msg,"Parameter %c",'a'+j);
	    else
		    sprintf(msg,"Probability");
	    putstring(promptrow+2,25,C_GENERAL_HI,msg);
	    sprintf(temp1,"%6.2f",(float)initarray[(i*totcols)+j]);
	    if (input_field(1,C_GENERAL_INPUT,temp1,6,
			    textrow,textcol+1,NULL) < 0)
	       break;
	    initarray[(i*totcols)+j] = atof(temp1);
	    }
	 memset(msg,' ',80); msg[81] = 0;
	 putstring(promptrow+2,0,C_PROMPT_BKGRD,msg);
	 }
      }

get_ifs_exit:
   unstackscreen();
   helpmode = oldhelpmode;
   EXIT_OVLY;
   return(ret);
}

/* --------------------------------------------------------------------- */
/*
	get_toggles() is called from FRACTINT.C whenever the 'x' key
	is pressed.  This routine prompts for several options,
	sets the appropriate variables, and returns the following code
	to the calling routine:

	-1  routine was ESCAPEd - no need to re-generate the image.
	 0  minor variable changed (such as "overwrite=").  No need to
	    re-generate the image.
	 1  major variable changed (such as "inside=").  Re-generate
	    the image.
	 2  Floating-point toggle changed.  FRACTINT.C takes special
	    actions in this instance, as the algorithm itself changes.

	Finally, remember to insert variables in the list *and* check
	for them in the same order!!!
*/

int get_toggles()
{
   int numcols; 		/* number of columns */
   int startrow;		/* upper left corner of list */
   int startcol;		/* upper left corner of list */
   char *choices[20];
   char c;
   int numprompts;
   char *s;
   int oldhelpmode;
   char prevsavename[81];

   double values[25], oldvalues[25];

   struct fullscreenvalues uvalues[25];
   int i, j, k;

   ENTER_OVLY(OVLY_PROMPTS);

   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] =  "Passes (1, 2, g[uessing], or b[oundary trace])";
   uvalues[k].type = 's';
   oldvalues[k] = usr_stdcalcmode;
   uvalues[k].uval.sval[0] = usr_stdcalcmode;
   uvalues[k].uval.sval[1] = 0;

   k++;
   choices[k] =  "Floating Point Algorithm";
   uvalues[k].type = 's';
   oldvalues[k] = usr_floatflag;
   if (usr_floatflag)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Maximum Iterations (2 to 32767)";
   uvalues[k].type = 'd';
   oldvalues[k] = maxit;
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Inside Color (<nnn>, maxiter, bof60, bof61)";
   uvalues[k].type = 's';
   if(inside == -60)
      strcpy(uvalues[k].uval.sval,"bof60");
   else if(inside == -61)
      strcpy(uvalues[k].uval.sval,"bof61");
   else if(inside == -1)
      strcpy(uvalues[k].uval.sval,"maxiter");
   else
      sprintf(uvalues[k].uval.sval,"%d",inside);
   oldvalues[k] = inside;

   k++;
   choices[k] = "Outside Color (-1 means none)";
   uvalues[k].type = 'd';
   oldvalues[k] = outside;
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Savename (.GIF implied)";
   uvalues[k].type = 's';
   oldvalues[k] = 0;
   strcpy(prevsavename,savename);
   strcpy(uvalues[k].uval.sval,savename);

   k++;
   choices[k] = "File Overwrite ('overwrite=')";
   uvalues[k].type = 's';
   oldvalues[k] = overwrite;
   if (overwrite)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Sound (no, yes, x, y, z)";
   uvalues[k].type = 's';
   oldvalues[k] = soundflag;
   strcpy(uvalues[k].uval.sval,"no");
   if (soundflag == -1)
      strcpy(uvalues[k].uval.sval,"yes");
   if (soundflag == 1)
      strcpy(uvalues[k].uval.sval,"x");
   if (soundflag == 2)
      strcpy(uvalues[k].uval.sval,"y");
   if (soundflag == 3)
      strcpy(uvalues[k].uval.sval,"z");

   k++;
   choices[k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = LogFlag;

   k++;
   choices[k] = "Distance Estimator Method (0 means off):";
   uvalues[k].type = 'd';
   oldvalues[k] = usr_distest;
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
   uvalues[k].type = 'd';
   oldvalues[k] = decomp[0];
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Biomorph Color (-1 means OFF)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = biomorph;

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = -1;
   i = fullscreen_prompt("        Basic Options\n"
			 "(not all combinations make sense)",
	 k+1,choices,uvalues,0,0);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      EXIT_OVLY;
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   c = uvalues[++k].uval.sval[0];
   if (c == 'B') c = 'b';
   if (c == 'G') c = 'g';
   if (c == '1' || c == '2' || c == 'g' || c == 'b')
      usr_stdcalcmode = c;
   if (oldvalues[k] != usr_stdcalcmode && j < 1) j = 1;

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      usr_floatflag = 1;
   else
      usr_floatflag = 0;
   if (usr_floatflag != oldvalues[k]) j = 2;

   ++k;
   maxit = uvalues[k].uval.dval;
   if (uvalues[k].uval.dval < 2) maxit = 2;
   if (uvalues[k].uval.dval > 32767) maxit = 32767;
   if (maxit != oldvalues[k] && j < 1) j = 1;

   if(strncmp(strlwr(uvalues[++k].uval.sval),"bof60",5)==0)
      inside = -60;
   else if(strncmp(strlwr(uvalues[k].uval.sval),"bof61",5)==0)
      inside = -61;
   else if(strncmp(strlwr(uvalues[k].uval.sval),"maxiter",5)==0)
      inside = -1;
   else
      inside = atoi(uvalues[k].uval.sval);
   if (inside != oldvalues[k] && j < 1) j = 1;

   outside = uvalues[++k].uval.dval;
   if (outside != oldvalues[k] && j < 1) j = 1;

   strcpy(savename,uvalues[++k].uval.sval);
   if (strcmp(savename,prevsavename))
      resave_flag = 0; /* forget pending increment if there is one */

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      overwrite = 1;
   else if (uvalues[k].uval.sval[0] == 'n' || uvalues[k].uval.sval[0] == 'N')
      overwrite = 0;

   if (strnicmp(uvalues[++k].uval.sval,"yes",2) == 0) {
      soundflag = -1;
      uvalues[k].uval.sval[0] = 'q';
      }
   else
      soundflag = 0;
   if (uvalues[k].uval.sval[0] == 'x' || uvalues[k].uval.sval[0] == 'X')
      soundflag = 1;
   if (uvalues[k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      soundflag = 2;
   if (uvalues[k].uval.sval[0] == 'z' || uvalues[k].uval.sval[0] == 'Z')
      soundflag = 3;
   if (soundflag != oldvalues[k] && (soundflag > 1 || oldvalues[k] > 1) &&
      j < 1) j = 1;

   LogFlag = uvalues[++k].uval.dval;
   if (LogFlag != oldvalues[k] && j < 1) j = 1;

   ++k;
   usr_distest = (uvalues[k].uval.dval > 32000) ? 32000 : uvalues[k].uval.dval;
   if (usr_distest != oldvalues[k]) j = 2;

   decomp[0] = uvalues[++k].uval.dval;
   if (decomp[0] != oldvalues[k] && j < 1) j = 1;

   biomorph = uvalues[++k].uval.dval;
   if (biomorph != oldvalues[k] && j < 1) j = 1;

   EXIT_OVLY;
   return(j);
}

/*
	get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
   int numcols; 		/* number of columns */
   int startrow;		/* upper left corner of list */
   int startcol;		/* upper left corner of list */
   char *choices[20];
   char c;
   int numprompts;
   char *s;
   int oldhelpmode;
   char prevsavename[81];

   double values[25], oldvalues[25];

   struct fullscreenvalues uvalues[25];
   int i, j, k;

   ENTER_OVLY(OVLY_PROMPTS);

   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] = "Look for finite attractor";
   uvalues[k].type = 's';
   oldvalues[k] = finattract;
   if (finattract)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Potential Max Color (0 means off)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[0];

   k++;
   choices[k] = "          Slope";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[1];

   k++;
   choices[k] = "          Bailout";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[2];

   k++;
   choices[k] = "          16 bit values";
   uvalues[k].type = 's';
   oldvalues[k] = pot16bit;
   if (pot16bit)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   choices[k+1] = "Inversion radius or \"auto\" (0 means off)";
   choices[k+2] = "          center X coordinate or \"auto\"";
   choices[k+3] = "          center Y coordinate or \"auto\"";
   for (i = 0; i < 3; i++) {
      uvalues[++k].type = 's';
      oldvalues[k] = inversion[i];
      if (inversion[i] == AUTOINVERT)
	 sprintf(uvalues[k].uval.sval,"auto");
      else
	 sprintf(uvalues[k].uval.sval,"%g",inversion[i]);
      }
   k++;
   choices[k] = "  (use fixed radius & center when zooming)",
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = -1;
   i = fullscreen_prompt("       Extended Doodads\n"
			 "(not all combinations make sense)",
	 k+1,choices,uvalues,0,0);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      EXIT_OVLY;
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      finattract = 1;
   else
      finattract = 0;
   if (finattract != oldvalues[k]) j = 2;

   potparam[0] = uvalues[++k].uval.dval;
   if (potparam[0] != oldvalues[k] && j < 1) j = 1;

   potparam[1] = uvalues[++k].uval.dval;
   if (potparam[0] != 0.0 && potparam[1] != oldvalues[k] && j < 1) j = 1;

   potparam[2] = uvalues[++k].uval.dval;
   if (potparam[0] != 0.0 && potparam[2] != oldvalues[k] && j < 1) j = 1;

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      pot16bit = 1;
   else
      pot16bit = 0;
   if (pot16bit != oldvalues[k])
      if (pot16bit) { /* turned it on */
	 if (potparam[0] != 0.0) j = 2;
	 }
      else /* turned it off */
	 if (dotmode != 11) /* ditch the disk video */
	    enddisk();
	 else /* keep disk video, but ditch the fraction part at end */
	    disk16bit = 0;

   for (i = 0; i < 3; i++) {
      if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
	 inversion[i] = AUTOINVERT;
      else
	 inversion[i] = atof(uvalues[k].uval.sval);
      if (oldvalues[k] != inversion[i]
	&& (i == 0 || inversion[0] != 0.0))
	 j = 1;
      }
   invert = (inversion[0] == 0.0) ? 0 : 3;

   EXIT_OVLY;
   return(j);
}

/* --------------------------------------------------------------------- */
/*
    get_view_params() is called from FRACTINT.C whenever the 'v' key
    is pressed.  Return codes are:
	-1  routine was ESCAPEd - no need to re-generate the image.
	 0  minor variable changed.  No need to re-generate the image.
	 1  View changed.  Re-generate the image.
*/

int get_view_params()
{
   char *choices[20];
   int oldhelpmode;
   double values[25], oldvalues[25];
   struct fullscreenvalues uvalues[25];
   int i, j, k;
   float oldaspectratio;

   ENTER_OVLY(OVLY_PROMPTS);
   stackscreen();

get_view_restart:
   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] = "Preview display? (no for full screen)";
   uvalues[k].type = 's';
   oldvalues[k] = viewwindow;
   if (viewwindow)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Auto window size reduction factor";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = viewreduction;

   k++;
   choices[k] = "Final media overall aspect ratio, y/x";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = oldaspectratio = finalaspectratio;

   k++;
   choices[k] = "Crop starting coordinates to new aspect ratio?";
   uvalues[k].type = 's';
   oldvalues[k] = viewcrop;
   if (viewcrop)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "explicit size x pixels (0 for auto size)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = viewxdots;
   k++;
   choices[k] = "              y pixels (0 to base on aspect ratio)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = viewydots;

   k++;
   choices[k] = "";
   uvalues[k].type = '*';
   k++;
   choices[k] = "Press F4 to reset view parameters to defaults.";
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPVIEW;
   i = fullscreen_prompt("View Window Options",k+1,choices,uvalues,
			 PROMPTHELP,16);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      unstackscreen();
      EXIT_OVLY;
      return(-1);
      }
   if (i == F4) {
      viewwindow = viewxdots = viewydots = 0;
      viewreduction = 4.2;
      viewcrop = 1;
      finalaspectratio = SCREENASPECT;
      goto get_view_restart;
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      viewwindow = 1;
   else
      viewwindow = 0;
   if (viewwindow != oldvalues[k]) j = 1;

   viewreduction = uvalues[++k].uval.dval;
   if (viewreduction != oldvalues[k] && viewwindow) j = 1;

   if ((finalaspectratio = uvalues[++k].uval.dval) == 0)
      finalaspectratio = SCREENASPECT;
   if (finalaspectratio != oldvalues[k] && viewwindow) j = 1;

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      viewcrop = 1;
   else
      viewcrop = 0;

   viewxdots = uvalues[++k].uval.dval;
   if (viewxdots != oldvalues[k] && viewwindow) j = 1;
   viewydots = uvalues[++k].uval.dval;
   if (viewydots != oldvalues[k] && viewwindow && viewxdots) j = 1;

   if (finalaspectratio != oldaspectratio && viewcrop)
      aspectratio_crop(oldaspectratio,finalaspectratio);

   unstackscreen();
   EXIT_OVLY;
   return(j);
}


/* --------------------------------------------------------------------- */

int Distribution = 30, Offset = 0, Slope = 25;
long con;

static char *starfield_prompts[4] = {
	"Star Density in Pixels per Star",
	"Percent Clumpiness",
	"Ratio of Dim stars to Bright",
	""
	};
static double starfield_values[4] = {
	30.0,100.0,25.0,0.0
	};

int get_starfield_params(void) {
   static char StarMap[] = "altern.map";
   int i, c;
   struct fullscreenvalues uvalues[3];

   ENTER_OVLY(OVLY_PROMPTS);

   if(colors < 255) {
      stopmsg(0,"starfield requires 256 color mode");
      EXIT_OVLY;
      return(-1);
   }
   for (i = 0; i < 3; i++) {
      uvalues[i].uval.dval = starfield_values[i];
      uvalues[i].type = 'd';
   }
   stackscreen();
   if (fullscreen_prompt("Starfield Parameters",
			 3,starfield_prompts,uvalues,0,0) < 0) {
      unstackscreen();
      EXIT_OVLY;
      return(-1);
      }
   unstackscreen();

   for (i = 0; i < 3; i++)
      starfield_values[i] = uvalues[i].uval.dval;

   if (starfield_values[0] <   1.0) starfield_values[0] =   1.0;
   if (starfield_values[0] > 100.0) starfield_values[0] = 100.0;
   if (starfield_values[1] <   1.0) starfield_values[1] =   1.0;
   if (starfield_values[1] > 100.0) starfield_values[1] = 100.0;
   if (starfield_values[2] <   1.0) starfield_values[2] =   1.0;
   if (starfield_values[2] > 100.0) starfield_values[2] = 100.0;

   Distribution = (int)(starfield_values[0]);
   con	= (long)(((starfield_values[1]) / 100.0) * (1L << 16));
   Slope = (int)(starfield_values[2]);

   if(ValidateLuts(StarMap) != 0) {
      char msg[200];
      sprintf(msg,"Could not load color map %s",StarMap);
      stopmsg(0,msg);
      EXIT_OVLY;
      return(-1);
      }
   spindac(0,1);		 /* load it, but don't spin */

   for(row = 0; row < ydots; row++) {
      for(col = 0; col < xdots; col++) {
	 if(check_key()) {
	    buzzer(1);
	    EXIT_OVLY;
	    return(1);
	    }
	 c = getcolor(col, row);
	 putcolor(col, row, GausianNumber(c, colors));
      }
   }
   buzzer(0);
   EXIT_OVLY;
   return(0);
}

/* --------------------------------------------------------------------- */

void goodbye()			/* we done.  Bail out */
{
   extern unsigned char exitmode;
   extern int mode7text;
   extern int made_dsktemp;
   union REGS r;
   static char goodbyemessage[]={"   Thank You for using FRACTINT"};
   setvideomode(3,0,0,0);
   r.h.al = (mode7text == 0) ? exitmode : 7;
   r.h.ah = 0;
   int86(0x10, &r, &r);
   printf("\n\n\n%s\n",goodbyemessage); /* printf takes far pointer */
   movecursor(6,0);
   discardgraphics(); /* if any emm/xmm tied up there, release it */
   if (made_dsktemp)
      remove("FRACTINT.DSK");
   exit(0);
}


/* --------------------------------------------------------------------- */

static int  findfirst(char *path)	/* Find 1st file (or subdir) meeting path/filespec */
{
     union REGS regs;
     regs.h.ah = 0x1A;		   /* Set DTA to filedata */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     regs.h.ah = 0x4E;		   /* Find 1st file meeting path */
     regs.x.dx = (unsigned)path;
     regs.x.cx = FILEATTR;
     intdos(&regs, &regs);
     return(regs.x.ax); 	   /* Return error code */
}

static int  findnext()		/* Find next file (or subdir) meeting above path/filespec */
{
     union REGS regs;
     regs.h.ah = 0x4F;		   /* Find next file meeting path */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     return(regs.x.ax);
}

static int lccompare(void const *arg1, void const *arg2) /* for qsort */
{
   return(strncasecmp(*((char **)arg1),*((char **)arg2),40));
}


static char *masks[] = {"*.pot","*.gif"};
#define FILE_MAX_PATH  80
#define FILE_MAX_DIR   80
#define FILE_MAX_DRIVE	3
#define FILE_MAX_FNAME	9
#define FILE_MAX_EXT	5
static int speedstate;

int getafilename(char *hdg,char *template,char *flname)
{
   int masklen;
   char filename[13];
   char speedstr[81];
   char buf[120];
   char tmpmask[FILE_MAX_PATH];   /* used to locate next file in list */
   static int numtemplates = 1;
   int i,j;
   int out;
   int retried;
   struct CHOICE
   {
      char name[13];
      char type;
   }
   *choices[MAXNUMFILES];
   int attributes[MAXNUMFILES];
   int filecount;   /* how many files */
   int dircount;    /* how many directories */
   int notroot;     /* not the root directory */

   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];

   ENTER_OVLY(OVLY_PROMPTS);

   /* steal existing array for "choices" */
   choices[0] = (struct CHOICE *)boxy;
   attributes[0] = 1;
   for(i=1;i<MAXNUMFILES;i++)
   {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
   }

restart:  /* return here if template or directory changes */

   tmpmask[0] = 0;
   if(flname[0] == 0)
      strcpy(flname,".\\");
   splitpath(flname ,drive,dir,fname,ext);
   makepath(filename,""   ,"" ,fname,ext);
   retried = 0;
retry_dir:
   if (dir[0] == 0)
      strcpy(dir,".");
   expand_dirname(dir,drive);
   makepath(tmpmask,drive,dir,"","");
   fix_dirname(tmpmask);
   if (retried == 0 && strcmp(dir,"\\") && strcmp(dir,".\\"))
   {
      tmpmask[(j = strlen(tmpmask) - 1)] = 0; /* strip trailing \ */
      if (strchr(tmpmask,'*') || strchr(tmpmask,'?')
	|| findfirst(tmpmask) != 0
	|| (DTA.attribute & SUBDIR) == 0)
      {
	 strcpy(dir,".\\");
	 ++retried;
	 goto retry_dir;
      }
      tmpmask[j] = '\\';
   }
   if(template[0])
   {
      numtemplates = 1;
      splitpath(template,NULL,NULL,fname,ext);
   }
   else
      numtemplates = sizeof(masks)/sizeof(masks[0]);
   filecount = -1;
   dircount  = 0;
   notroot   = 0;
   j = 0;
   masklen = strlen(tmpmask);
   strcat(tmpmask,"*.*");
   out = findfirst(tmpmask);
   while(out == 0 && filecount < MAXNUMFILES)
   {
      if((DTA.attribute & SUBDIR) && strcmp(DTA.filename,"."))
      {
	 strlwr(DTA.filename);
	 if(strcmp(DTA.filename,".."))
	    strcat(DTA.filename,"\\");
	 strncpy(choices[++filecount]->name,DTA.filename,13);
	 choices[filecount]->type = 1;
	 dircount++;
	 if(strcmp(DTA.filename,"..")==0)
	    notroot = 1;
      }
      out = findnext();
   }
   tmpmask[masklen] = 0;
   if(template[0])
      makepath(tmpmask,drive,dir,fname,ext);
   do
   {
      if(numtemplates > 1)
	 strcpy(&(tmpmask[masklen]),masks[j]);
      out = findfirst(tmpmask);
      while(out == 0 && filecount < MAXNUMFILES)
      {
	 if(!(DTA.attribute & SUBDIR))
	 {
	    strlwr(DTA.filename);
	    strncpy(choices[++filecount]->name,DTA.filename,13);
	    choices[filecount]->type = 0;
	 }
	 out = findnext();
      }
   }
   while (++j < numtemplates);
   if (++filecount == 0)
   {
      strcpy(choices[filecount]->name,"*nofiles*");
      choices[filecount]->type = 0;
      ++filecount;
   }
   qsort(choices,filecount,sizeof(char *),lccompare); /* sort type list */
   if(notroot == 0) /* must be in root directory */
   {
      splitpath(tmpmask,drive,dir,fname,ext);
      strcpy(dir,"\\");
      makepath(tmpmask,drive,dir,fname,ext);
   }
   if(numtemplates > 1)
      strcat(tmpmask," *.pot");
   strcpy(buf,hdg);
   strcat(buf,"\nTemplate: ");
   strcat(buf,tmpmask);
/**if(filecount - dircount == 0)   PB removed this, makes max size too big
      strcat(buf,"  No files found, please re-enter"); **/
   strcpy(speedstr,filename);
   if (speedstr[0] == 0)
   {
      for (i=0; i<filecount; i++) /* find first file */
	 if (choices[i]->type == 0)
	    break;
      if (i >= filecount)
	 i = 0;
   }
   i = fullscreen_choice(0,buf,NULL,NULL,
			 filecount,(char **)choices,attributes,5,99,12,
			 i,NULL,NULL,speedstr,filename_speedstr,NULL);
   if (i < 0)
   {
      EXIT_OVLY;
      return(-1);
   }
   if(speedstr[0] == 0 || speedstate == MATCHING)
   {
      if(choices[i]->type)
      {
	 if(strcmp(choices[i]->name,"..") == 0) /* go up a directory */
	 {
	    if(strcmp(dir,".\\") == 0)
	       strcpy(dir,"..\\");
	    else
	    {
	       char *s;
	       if(s = strrchr(dir,'\\')) /* trailing slash */
	       {
		  *s = 0;
		  if(s = strrchr(dir,'\\'))
		     *(s+1) = 0;
	       }
	    }
	 }
	 else  /* go down a directory */
	    strcat(dir,choices[i]->name);
	 fix_dirname(dir);
	 makepath(flname,drive,dir,"","");
	 goto restart;
      }
      splitpath(choices[i]->name,NULL,NULL,fname,ext);
      makepath(flname,drive,dir,fname,ext);
   }
   else
   {
      if (speedstate == SEARCHPATH
	&& strchr(speedstr,'*') == 0 && strchr(speedstr,'?') == 0
	&& findfirst(speedstr) == 0
	&& (DTA.attribute & SUBDIR)) /* it is a directory */
	 speedstate = TEMPLATE;
      if(speedstate == TEMPLATE)
      {
	 /* extract from tempstr the pathname and template information,
	    being careful not to overwrite drive and directory if not
	    newly specified */
	 char drive1[FILE_MAX_DRIVE];
	 char dir1[FILE_MAX_DIR];
	 char fname1[FILE_MAX_FNAME];
	 char ext1[FILE_MAX_EXT];
	 splitpath(speedstr,drive1,dir1,fname1,ext1);
	 if(drive1[0])
	    strcpy(drive,drive1);
	 if(dir1[0])
	    strcpy(dir,dir1);
	 makepath(flname,drive,dir,fname1,ext1);
	 if(strchr(fname1,'*') || strchr(fname1,'?') ||
	     strchr(ext1  ,'*') || strchr(ext1  ,'?'))
	    makepath(template,"","",fname1,ext1);
	 else if(isadirectory(flname))
	    fix_dirname(flname);
	 goto restart;
      }
      else /* speedstate == SEARCHPATH */
      {
	 char fullpath[80];
	 if (strchr(speedstr,'.') == NULL)
	    strcat(speedstr,".gif");
	 findpath(speedstr,fullpath);
	 if(fullpath[0])
	    strcpy(flname,fullpath);
	 else
	 {  /* failed, make diagnostic useful: */
	    strcpy(flname,speedstr);
	    if (strchr(speedstr,'\\') == NULL)
	    {
	       splitpath(speedstr,NULL,NULL,fname,ext);
	       makepath(flname,drive,dir,fname,ext);
	    }
	 }
      }
   }
   EXIT_OVLY;
   return(0);
}

static int filename_speedstr(int row, int col, int vid,
			     char *speedstring, int speed_match)
{
   extern char speed_prompt[];
   char *prompt;
   int i,match;
   if ( strchr(speedstring,':')
     || strchr(speedstring,'*') || strchr(speedstring,'*')
     || strchr(speedstring,'?')) {
      speedstate = TEMPLATE;  /* template */
      prompt = "File Template";
      }
   else if (speed_match) {
      speedstate = SEARCHPATH; /* does not match list */
      prompt = "Search Path for";
      }
   else {
      speedstate = MATCHING;
      prompt = speed_prompt;
      }
   putstring(row,col,vid,prompt);
   return(strlen(prompt));
}

static int isadirectory(char *s)
{
   int out;
   if(strchr(s,'*') || strchr(s,'?'))
      return(0); /* for my purposes, not a directory */
   if(findfirst(s) != 0) /* couldn't find it */
   {
      /* any better ideas?? */
      if(strchr(s,'\\')) /* we'll guess it is a directory */
	 return(1);
      else
	 return(0);  /* no slashes - we'll guess it's a file */
   }
   else if(DTA.attribute & SUBDIR)
      return(1);   /* we're SURE it's a directory */
   else
      return(0);
}

static splitpath(char *template,char *drive,char *dir,char *fname,char *ext)
{
   int length;
   int len;
   int offset;
   char *tmp;

   if(drive)
      drive[0] = 0;
   if(dir)
      dir[0]   = 0;
   if(fname)
      fname[0] = 0;
   if(ext)
      ext[0]   = 0;

   if((length = strlen(template)) == 0)
      return(0);
   offset = 0;

   /* get drive */
   if(length >= 2)
      if(template[1] == ':')
      {
	 if(drive)
	 {
	    drive[0] = template[offset++];
	    drive[1] = template[offset++];
	    drive[2] = 0;
	 }
	 else
	 {
	    offset++;
	    offset++;
	 }
      }

   /* get dir */
   if(offset < length)
   {
      tmp = strrchr(template,'\\');
      if(tmp)
      {
	 tmp++;  /* first character after slash */
	 len = tmp - &template[offset];
	 if(len >=0 && len < 80 && dir)
	    strncpy(dir,&template[offset],len);
	 if(len < 80 && dir)
	    dir[len] = 0;
	 offset += len;
      }
   }
   else
      return(0);

   /* get fname */
   if(offset < length)
   {
      tmp = strrchr(template,'.');
      if(tmp < strrchr(template,'\\') || tmp < strrchr(template,':'))
	 tmp = 0; /* in this case the '.' must be a directory */
      if(tmp)
      {
	 tmp++; /* first character past "." */
	 len = tmp - &template[offset];
	 if((len > 0) && (offset+len < length) && fname)
	 {
	    strncpy(fname,&template[offset],len);
	    fname[len] = 0;
	 }
	 offset += len;
	 if((offset < length) && ext)
	    strcpy(ext,&template[offset]);
      }
      else if((offset < length) && fname)
	 strcpy(fname,&template[offset]);
   }
   return(0);
}

static makepath(char *template,char *drive,char *dir,char *fname,char *ext)
{
   strcpy(template,drive);
   strcat(template,dir);
   strcat(template,fname);
   strcat(template,ext);
   return(0);
}


/* fix up directory names */
static void fix_dirname(char *dirname)
{
   int length;
   /* scrub white space from end for safety */
   length = strlen(dirname); /* index of last character */
   while (--length >= 0 && isspace(dirname[length])) { }
   dirname[++length] = 0;
   /* make sure dirname ends with a slash */
   if(length == 0 || dirname[length-1] != '\\')
      strcat(dirname,"\\");
}

static int expand_dirname(char *dirname,char *drive)
{
   fix_dirname(dirname);
   if (dirname[0] != '\\') {
      char buf[81],curdir[81];
      union REGS regs;
      struct SREGS sregs;
      curdir[0] = 0;
      regs.h.ah = 0x47; /* get current directory */
      regs.h.dl = 0;
      if (drive[0] && drive[0] != ' ')
	 regs.h.dl = tolower(drive[0])-'a'+1;
      regs.x.si = (unsigned int) &curdir[0];
      segread(&sregs);
      intdosx(&regs, &regs, &sregs);
      strcat(curdir,"\\");
      while (strncmp(dirname,".\\",2) == 0) {
	 strcpy(buf,&dirname[2]);
	 strcpy(dirname,buf);
	 }
      while (strncmp(dirname,"..\\",3) == 0) {
	 char *s;
	 curdir[strlen(curdir)-1] = 0; /* strip trailing slash */
	 if (s = strrchr(curdir,'\\'))
	    *s = 0;
	 strcat(curdir,"\\");
	 strcpy(buf,&dirname[3]);
	 strcpy(dirname,buf);
	 }
      strcpy(buf,dirname);
      dirname[0] = 0;
      if (curdir[0] != '\\')
	 strcpy(dirname,"\\");
      strcat(dirname,curdir);
      strcat(dirname,buf);
      }
   return(0);
}


int select_video_mode()
{
   int	attributes[MAXVIDEOMODES];
   int oldhelpmode;
   int i;
   ENTER_OVLY(OVLY_PROMPTS);
   oldhelpmode = helpmode;
   helpmode = -1; /* disable help */
   for (i = 0; i < MAXVIDEOMODES; ++i)
      attributes[i] = 1;
   if (adapter < 0) {
      switch (video_type) { /* set up a reasonable default (we hope) */
	 case 2:  i = 3; break;  /* cga */
	 case 3:  i = 0; break;  /* ega */
	 default: i = 1; break;  /* mcga/vga? */
	 }
      }
   else
      i = adapter;
   if ((i = fullscreen_choice(0,"Select Video Mode",
 "key...name......................xdot.ydot.clr.comment..................",
		  NULL,maxvideomode,NULL,attributes,
		  1,16,71,i,format_item,NULL,NULL,NULL,check_modekey)) == -1)
      return(-1);
   if (i < 0) /* returned key value */
      i = -100 - i;
   else
      i = kbdkeys[i];
   if (adapter >= 0) /* reset to real current table entry */
      fromvideotable(adapter);
   helpmode = oldhelpmode;
   EXIT_OVLY;
   return(i);
}

static void format_item(int choice,char *buf)
{
   fromvideotable(choice);
   sprintf(buf,"%-5s %-25s %4d %4d %3d %-25s",  /* 71 chars */
	   fkeys[choice], videoentry.name, videoentry.xdots, videoentry.ydots,
	   videoentry.colors, videoentry.comment);
}

static int check_modekey(int curkey)
{
   int i;
   for (i = 0; i < maxvideomode; ++i)
      if (curkey == kbdkeys[i])
	 return(-100-curkey);
   return(0);
}


