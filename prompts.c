/*
	Various routines that prompt for things.
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"

/* Routines in this module	*/

extern	int fullscreen_prompt(char *hdg,int numprompts,
	       char * far *prompts,struct fullscreenvalues values[],
	       int options,int fkeymask,char far *extrainfo);
extern	void prompts_overlay(void );
extern	int get_fracttype(void );
extern	int get_fract_params(int);
extern	int get_3d_params(void );
extern	int get_fract3d_params(void );
extern	int get_toggles(void );
extern	int get_toggles2(void );
extern	int get_view_params(void );
extern	int get_starfield_params(void );
extern	void goodbye(void );
extern	int getafilename(char *hdg,char *template,char *flname);
extern	int get_commands(void);

extern int Targa_Overlay;
extern int Targa_Out;
extern unsigned char back_color[];

static	int prompt_valuestring(char *buf,struct fullscreenvalues *val);
static	int prompt_checkkey(int curkey);
static	int input_field_list(int attr,char *fld,int vlen,char **list,int llen,
			     int row,int col,int (*checkkey)(int));
static	int select_fracttype(int t);
static	int sel_fractype_help(int curkey, int choice);
static	int get_corners(void);
static	int edit_ifs_params(void );
static	int select_type_params(int newfractype,int oldfractype);
static	void set_default_parms(void);
static	long get_file_entry(int,char *,char *,char *,char *);
static	long gfe_choose_entry(int,char *,char *,char *);
static	int check_gfe_key(int curkey,int choice);
static	void load_entry_text(FILE *entfile,char far *buf,int maxlines);
static	void format_parmfile_line(int,char *);
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

extern char *strig[];
extern int numtrigfn;
extern int bailout;
extern int dotmode;
extern int orbit_delay;

extern int fullscreen_choice(
	     int options, char *hdg, char *hdg2, char *instr, int numchoices,
	     char **choices, int *attributes, int boxwidth, int boxdepth,
	     int colwidth, int current, void (*formatitem)(),
	     char *speedstring, int (*speedprompt)(), int (*checkkey)());
extern int strncasecmp(char *s,char *t,int ct);
extern int field_prompt(int options, char *hdg, char *instr, char *fld,
	     int len, int (*checkkey)() );
extern int input_field(int options, int attr, char *fld,
	     int len, int row, int col, int (*checkkey)(int) );
extern int read_help_topic(int label_num, int off, int len, void far *buf);
extern int Formula();

static char funnyglasses_map_name[16];
extern char temp[], temp1[256];   /* temporary strings	      */
extern int mapset;
extern int previewfactor;
extern int xtrans, ytrans;
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern char showbox; /* flag to show box and vector in preview */
extern int debugflag;
extern int extraseg;
extern int whichimage;
extern int xadjust;
extern int eyeseparation;
extern int glassestype;
extern findpath();
char   MAP_name[80] = "";
int    mapset = 0;
extern	int	overlay3d;	    /* 3D overlay flag: 0 = OFF */
extern	int	lookatmouse;
extern	int haze;
extern	int RANDOMIZE;
extern	char light_name[];
extern	int Ambient;
extern	int RAY;
extern	int BRIEF;
extern	char ray_name[];

extern	int	init3d[20];	/* '3d=nn/nn/nn/...' values */
extern	double	xxmin,xxmax;	/* initial corner values    */
extern	double	yymin,yymax;	/* initial corner values    */
extern	unsigned char usemag;

extern	int AntiAliasing;
extern double zzmin, zzmax, ttmin, ttmax;
extern int Transparent3D;

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
extern	int	fillcolor; 	/* fill color */
extern	int	outside;	/* outside color */
extern	int	finattract;	/* finite attractor switch */
extern	char	savename[80];	/* save files using this name */
extern	char	ifsfilename[80];    /* IFS code file */
extern	char	ifs3dfilename[80];  /* IFS 3D code file */
extern	char	preview;	/* 3D preview mode flag */
extern	int	decomp[];	/* decomposition parameters */
extern	int	usr_distest;	/* distance estimator option */
extern	int	distestwidth;
extern	int	transparent[];	/* transparency values */
extern	char	usr_stdcalcmode; /* '1', '2', 'g', 'b' */
extern	char overwrite; 	/* overwrite= flag */
extern	int	soundflag;	/* sound option */
extern	int	LogFlag;	/* non-zero if logarithmic palettes */
extern	int	usr_biomorph;	/* Biomorph flag */
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
extern	int	started_resaves;
extern	char	boxy[];
extern	char	*fkeys[];
extern	int	kbdkeys[];
extern	int	video_type;
extern	int	adapter;
extern	int	rotate_lo,rotate_hi;
extern	int	display3d;
extern	int	rangeslen;

extern char LFileName[];	/* file to find the formulas in */
extern char LName[];		/* Name of the Formula (if not null) */
extern char FormFileName[];
extern char FormName[];
extern char IFSFileName[];
extern char IFSName[];
extern char CommandFile[];
extern char CommandName[];
extern float far *ifs_defn;
extern int ifs_type;
extern int ifs_changed;

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

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS	   2
#define GETPARM    3

/* --------------------------------------------------------------------- */
extern char s_iter[];
extern char s_real[];
extern char s_mult[];
extern char s_sum[];
extern char s_imag[];
extern char s_zmag[];
extern char s_bof60[];
extern char s_bof61[];
extern char s_maxiter[];
extern char s_epscross[];
extern char s_startrail[];
extern char s_normal[];

extern char gifmask[];
char ifsmask[13]     = {"*.ifs"};
char formmask[13]    = {"*.frm"};
char lsysmask[13]    = {"*.l"};
char commandmask[13] = {"*.par"};

void prompts_overlay() { }	/* for restore_active_ovly */


/* --------------------------------------------------------------------- */

static int promptfkeys;

int fullscreen_prompt(	/* full-screen prompting routine */
	char *hdg,		/* heading, lines separated by \n */
	int numprompts, 	/* there are this many prompts (max) */
	char * far *prompts,	/* array of prompting pointers */
	struct fullscreenvalues values[], /* array of values */
	int options,		/* for future */
	int fkeymask,		/* bit n on if Fn to cause return */
	char far *extrainfo	/* extra info box to display, \n separated */
	)
{
   char far *hdgscan;
   int titlelines,titlewidth,titlerow;
   int maxpromptwidth,maxfldwidth,maxcomment;
   int boxrow,boxlines;
   int boxcol,boxwidth;
   int extralines,extrawidth,extrarow;
   int instrrow;
   int promptrow,promptcol,valuecol;
   int curchoice;
   int done, i, j;
   int savelookatmouse;
   int curtype, curlen;
   char buf[81];
static char far instr1[]  = {"Use \x18 and \x19 to select values to change"};
static char far instr2a[]  = {"Type in replacement value for selected field"};
static char far instr2b[]  = {"Use \x1B or \x1A to change value of selected field"};
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
   extralines = extrawidth = i = 0;
   if ((hdgscan = extrainfo))
      if (*hdgscan == 0)
	 extrainfo = NULL;
      else { /* count extra lines, find widest */
	 extralines = 3;
	 while (*hdgscan) {
	    if (*(hdgscan++) == '\n') {
	       if (extralines + numprompts + titlelines >= 20) {
		   *hdgscan = 0; /* full screen, cut off here */
		   break;
		   }
	       ++extralines;
	       i = -1;
	       }
	    if (++i > extrawidth)
	       extrawidth = i;
	    }
	 }

   /* work out vertical positioning */
   i = numprompts + titlelines + extralines + 3; /* total rows required */
   j = (25 - i) / 2;		       /* top row of it all when centered */
   j -= j / 4;			       /* higher is better if lots extra */
   boxlines = numprompts;
   titlerow = 1 + j;
   promptrow = boxrow = titlerow + titlelines;
   if (titlerow > 2) {		       /* room for blank between title & box? */
      --titlerow;
      --boxrow;
      ++boxlines;
      }
   instrrow = boxrow+boxlines;
   if (instrrow + 3 + extralines < 25) {
      ++boxlines;    /* blank at bottom of box */
      ++instrrow;
      if (instrrow + 3 + extralines < 25)
	 ++instrrow; /* blank before instructions */
      }
   extrarow = instrrow + 2;
   if (numprompts > 1) /* 3 instructions lines */
      ++extrarow;
   if (extrarow + extralines < 25)
      ++extrarow;

   /* work out horizontal positioning */
   maxfldwidth = maxpromptwidth = maxcomment = 0;
   for (i = 0; i < numprompts; i++) {
      if (values[i].type == 'y') {
	 static char *noyes[2] = {"no","yes"};
	 values[i].type = 'l';
	 values[i].uval.ch.vlen = 3;
	 values[i].uval.ch.list = noyes;
	 values[i].uval.ch.llen = 2;
	 }
      j = strlen(prompts[i]);
      if (values[i].type == '*') {
	 if (j > maxcomment)	 maxcomment = j;
	 }
      else {
	 if (j > maxpromptwidth) maxpromptwidth = j;
	 j = prompt_valuestring(buf,&values[i]);
	 if (j > maxfldwidth)	 maxfldwidth = j;
	 }
      }
   boxwidth = maxpromptwidth + maxfldwidth + 2;
   if (maxcomment > boxwidth) boxwidth = maxcomment;
   if ((boxwidth += 4) > 80) boxwidth = 80;
   boxcol = (80 - boxwidth) / 2;       /* center the box */
   promptcol = boxcol + 2;
   valuecol = boxcol + boxwidth - maxfldwidth - 2;
   if (boxwidth <= 76) {	       /* make margin a bit wider if we can */
      boxwidth += 2;
      --boxcol;
      }
   if ((j = titlewidth) < extrawidth)
      j = extrawidth;
   if ((i = j + 4 - boxwidth) > 0) {   /* expand box for title/extra */
      if (boxwidth + i > 80)
	 i = 80 - boxwidth;
      boxwidth += i;
      boxcol -= i / 2;
      }
   i = (90 - boxwidth) / 20;
   boxcol    -= i;
   promptcol -= i;
   valuecol  -= i;

   /* display box heading */
   for (i = titlerow; i < boxrow; ++i)
      setattr(i,boxcol,C_PROMPT_HI,boxwidth);
   textcbase = boxcol + (boxwidth - titlewidth) / 2;
   putstring(titlerow,0,C_PROMPT_HI,hdg);

   /* display extra info */
   if (extrainfo) {
      memset(buf,'\xC4',80); buf[boxwidth-2] = 0;
      textcbase = boxcol + 1;
      putstring(extrarow,0,C_PROMPT_BKGRD,buf);
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,buf);
      --textcbase;
      putstring(extrarow,0,C_PROMPT_BKGRD,"\xDA");
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,"\xC0");
      textcbase += boxwidth - 1;
      putstring(extrarow,0,C_PROMPT_BKGRD,"\xBF");
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,"\xD9");
      textcbase = boxcol;
      for (i = 1; i < extralines-1; ++i) {
	 putstring(extrarow+i,0,C_PROMPT_BKGRD,"\xB3");
	 putstring(extrarow+i,boxwidth-1,C_PROMPT_BKGRD,"\xB3");
	 }
      textcbase += (boxwidth - extrawidth) / 2;
      putstring(extrarow+1,0,C_PROMPT_TEXT,extrainfo);
      }
   textcbase = 0;

   /* display empty box */
   for (i = 0; i < boxlines; ++i)
      setattr(boxrow+i,boxcol,C_PROMPT_LO,boxwidth);

   /* display initial values */
   for (i = 0; i < numprompts; i++) {
      putstring(promptrow+i, promptcol, C_PROMPT_LO, prompts[i]);
      prompt_valuestring(buf,&values[i]);
      putstring(promptrow+i, valuecol, C_PROMPT_LO, buf);
      }

   /* display footing */
   if (numprompts > 1)
      putstringcenter(instrrow++,0,80,C_PROMPT_BKGRD,instr1);
   putstringcenter(instrrow+1,0,80,C_PROMPT_BKGRD,
	 (helpmode > 0) ? instr3b : instr3a);

   curchoice = done = 0;
   while (values[curchoice].type == '*') ++curchoice;

   while (!done) {

      curtype = values[curchoice].type;
      curlen = prompt_valuestring(buf,&values[curchoice]);
      putstringcenter(instrrow,0,80,C_PROMPT_BKGRD,
		      (curtype == 'l') ? instr2b : instr2a);
      putstring(promptrow+curchoice,promptcol,C_PROMPT_HI,prompts[curchoice]);

      if (curtype == 'l') {
	 i = input_field_list(
		C_PROMPT_CHOOSE, buf, curlen,
		values[curchoice].uval.ch.list, values[curchoice].uval.ch.llen,
		promptrow+curchoice,valuecol, prompt_checkkey);
	 for (j = 0; j < values[curchoice].uval.ch.llen; ++j)
	    if (strcmp(buf,values[curchoice].uval.ch.list[j]) == 0) break;
	 values[curchoice].uval.ch.val = j;
	 }
      else {
	 j = 0;
	 if (curtype == 'i') j = 3;
	 if (curtype == 'd') j = 5;
	 if (curtype == 'f') j = 1;
	 i = input_field(j, C_PROMPT_INPUT, buf, curlen,
		promptrow+curchoice,valuecol,prompt_checkkey);
	 switch (values[curchoice].type) {
	    case 'd':
	       values[curchoice].uval.dval = atof(buf);
	       break;
	    case 'f':
	       values[curchoice].uval.dval = atof(buf);
	       roundfloatd(&values[curchoice].uval.dval);
	       break;
	    case 'i':
	       values[curchoice].uval.ival = atoi(buf);
	       break;
	    case 's':
	       strncpy(values[curchoice].uval.sval,buf,16);
	       break;
	    default: /* assume 0x100+n */
	       strcpy(values[curchoice].uval.sbuf,buf);
	    }
	 }

      putstring(promptrow+curchoice,promptcol,C_PROMPT_LO,prompts[curchoice]);
      j = strlen(buf);
      memset(&buf[j],' ',80-j); buf[curlen] = 0;
      putstring(promptrow+curchoice, valuecol, C_PROMPT_LO,  buf);

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

   movecursor(25,80);
   lookatmouse = savelookatmouse;
   EXIT_OVLY;
   return(done);
}

static int prompt_valuestring(char *buf,struct fullscreenvalues *val)
{  /* format value into buf, return field width */
   int i,ret;
   switch (val->type) {
      case 'd':
	 ret = 20;
	 i = 15;
	 while (1) {
	    sprintf(buf,"%.*g",i,val->uval.dval);
	    if (strlen(buf) <= ret) break;
	    --i;
	    }
	 break;
      case 'f':
	 sprintf(buf,"%.7g",val->uval.dval);
	 ret = 14;
	 break;
      case 'i':
	 sprintf(buf,"%d",val->uval.ival);
	 ret = 6;
	 break;
      case '*':
	 *buf = ret = 0;
	 break;
      case 's':
	 strncpy(buf,val->uval.sval,16);
	 buf[15] = 0;
	 ret = 15;
	 break;
      case 'l':
	 strcpy(buf,val->uval.ch.list[val->uval.ch.val]);
	 ret = val->uval.ch.vlen;
	 break;
      default: /* assume 0x100+n */
	 strcpy(buf,val->uval.sbuf);
	 ret = val->type & 0xff;
      }
   return ret;
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

static int input_field_list(
	int attr,	      /* display attribute */
	char *fld,	      /* display form field value */
	int vlen,	      /* field length */
	char **list,	      /* list of values */
	int llen,	      /* number of entries in list */
	int row,	      /* display row */
	int col,	      /* display column */
	int (*checkkey)(int)  /* routine to check non data keys, or NULL */
	)
{
   int initval,curval;
   char buf[81];
   int curkey;
   int i, j;
   int ret,savelookatmouse;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   for (initval = 0; initval < llen; ++initval)
      if (strcmp(fld,list[initval]) == 0) break;
   if (initval >= llen) initval = 0;
   curval = initval;
   ret = -1;
   while (1) {
      strcpy(buf,list[curval]);
      i = strlen(buf);
      while (i < vlen)
	 buf[i++] = ' ';
      buf[vlen] = 0;
      putstring(row,col,attr,buf);
      curkey = keycursor(row,col); /* get a keystroke */
      switch (curkey) {
	 case ENTER:
	 case ENTER_2:
	    ret = 0;
	    goto inpfldl_end;
	 case ESC:
	    goto inpfldl_end;
	 case RIGHT_ARROW:
	 case RIGHT_ARROW_2:
	    if (++curval >= llen)
	       curval = 0;
	    break;
	 case LEFT_ARROW:
	 case LEFT_ARROW_2:
	    if (--curval < 0)
	       curval = llen - 1;
	    break;
	 case F5:
	    curval = initval;
	    break;
	 default:
	    if (curkey < 32 || curkey >= 127) {
	       if (checkkey && (ret = (*checkkey)(curkey)))
		  goto inpfldl_end;
	       break;				     /* non alphanum char */
	       }
	    j = curval;
	    for (i = 0; i < llen; ++i) {
	       if (++j >= llen)
		  j = 0;
	       if ((*list[j] & 0xdf) == (curkey & 0xdf)) {
		  curval = j;
		  break;
		  }
	       }
	 }
      }
inpfldl_end:
   strcpy(fld,list[curval]);
   lookatmouse = savelookatmouse;
   return(ret);
}


/* --------------------------------------------------------------------- */

/* MCP 7-7-91, This is static code, but not called anywhere */
#ifdef DELETE_UNUSED_CODE

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

#endif

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
      if ((i = select_type_params(t, fractype)) == 0) { /* ok, all done */
	 done = 0;
	 break;
	 }
      if (i > 0) /* can't return to prior image anymore */
	 done = 1;
      }
   if (done < 0)
      fractype = oldfractype;
   curfractalspecific = &fractalspecific[fractype];
   EXIT_OVLY;
   return(done);
}

struct FT_CHOICE {
      char name[15];
      int  num;
      };
static struct FT_CHOICE **ft_choices; /* for sel_fractype_help subrtn */

static int select_fracttype(int t) /* subrtn of get_fracttype, separated */
				   /* so that storage gets freed up	 */
{
   int oldhelpmode;
   int numtypes, done;
   int i, j;
#define MAXFTYPES 200
   char tname[40];
   struct FT_CHOICE *choices[MAXFTYPES];
   int attributes[MAXFTYPES];

   /* steal existing array for "choices" */
   choices[0] = (struct FT_CHOICE *)boxy;
   attributes[0] = 1;
   for (i = 1; i < MAXFTYPES; ++i) {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
      }
   ft_choices = &choices[0];

   /* setup context sensitive help */
   oldhelpmode = helpmode;
   helpmode = HELPFRACTALS;

   if (t == IFS3D) t = IFS;
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
   done = fullscreen_choice(CHOICEHELP+8,"Select a Fractal Type",NULL,
			    "Press F2 for a description of the highlighted type",
			    numtypes,(char **)choices,attributes,0,0,0,j,
			    NULL,tname,NULL,sel_fractype_help);
   if (done >= 0)
      done = choices[done]->num;
   helpmode = oldhelpmode;
   return(done);
}

static int sel_fractype_help(int curkey,int choice)
{
   int oldhelpmode;
   if (curkey == F2) {
      oldhelpmode = helpmode;
      helpmode = fractalspecific[(*(ft_choices+choice))->num].helptext;
      help(0);
      helpmode = oldhelpmode;
      }
   return(0);
}

static int select_type_params(	/* prompt for new fractal type parameters */
	int newfractype,	/* new fractal type */
	int oldfractype 	/* previous fractal type */
	)
{
   int ret,oldhelpmode;

   oldhelpmode = helpmode;
   ret = 0;
   fractype = newfractype;
   curfractalspecific = &fractalspecific[fractype];

   if (fractype == LSYSTEM) {
      helpmode = HT_LSYS;
      if (get_file_entry(GETLSYS,"L-System",lsysmask,LFileName,LName) < 0) {
	 ret = 1;
	 goto sel_type_exit;
	 }
      }
   if (fractype == FORMULA || fractype == FFORMULA) {
      helpmode = HT_FORMULA;
      if (get_file_entry(GETFORMULA,"Formula",formmask,FormFileName,FormName) < 0) {
	 ret = 1;
	 goto sel_type_exit;
	 }
      }
   if (fractype == IFS || fractype == IFS3D) {
      static char far ifsmsg[] = {
         "Current IFS parameters have been edited but not saved.\n"
         "Continue to replace them with new selection, cancel to keep them."};
      helpmode = HT_IFS;
      if (!ifs_defn || !ifs_changed || !stopmsg(22,ifsmsg))
	 if (get_file_entry(GETIFS,"IFS",ifsmask,IFSFileName,IFSName) < 0) {
	    ret = 1;
	    goto sel_type_exit;
	    }
      }

   set_default_parms();

   if (get_fract_params(0) < 0)
      ret = 1;
   else {
      if (newfractype != oldfractype) {
	 invert = 0;
	 inversion[0] = inversion[1] = inversion[2] = 0;
	 }
      }

sel_type_exit:
   helpmode = oldhelpmode;
   return(ret);

}

static void set_default_parms()
{
   int i;
   xxmin = curfractalspecific->xmin;
   xxmax = curfractalspecific->xmax;
   yymin = curfractalspecific->ymin;
   yymax = curfractalspecific->ymax;
   xx3rd = xxmin;
   yy3rd = yymin;
   if (viewcrop && finalaspectratio != SCREENASPECT)
      aspectratio_crop(SCREENASPECT,finalaspectratio);
   for (i = 0; i < 4; i++) {
      param[i] = curfractalspecific->paramvalue[i];
      roundfloatd(&param[i]);
      }
}


/* --------------------------------------------------------------------- */

int get_fract_params(int caller)	/* prompt for type-specific parms */
{
   int i,j,k;
   int curtype,numparams,numtrig;
   struct fullscreenvalues paramvalues[12];
   char *choices[12];
   int oldbailout;
   int promptnum;
   char msg[120];
   char *typename, *tmpptr;
   char bailoutmsg[50];
   int ret = 0;
   int oldhelpmode;
   static char *trg[] = {"First Function","Second Function",
			 "Third Function","Fourth Function"};
   extern char dstack[4096];
   char *filename,*entryname;
   FILE *entryfile;
   char *trignameptr[25];

   ENTER_OVLY(OVLY_PROMPTS);

   curtype = fractype;
   if (curfractalspecific->name[0] == '*'
     && (i = fractalspecific->tofloat) != NOFRACTAL
     && fractalspecific[i].name[0] != '*')
      curtype = i;
   curfractalspecific = &fractalspecific[curtype];

   if (curtype == IFS || curtype == IFS3D) {
      ret = ((caller) ? edit_ifs_params() : 0);
      goto gfp_exit;
      }

   if (*(typename = curfractalspecific->name) == '*')
      ++typename;

   for (numparams = 0; numparams < 4; numparams++) {
      if (curfractalspecific->param[numparams][0] == 0) break;
      choices[numparams] = curfractalspecific->param[numparams];
      paramvalues[numparams].type = 'd';
      paramvalues[numparams].uval.dval = param[numparams];
      }

   numtrig = (curfractalspecific->flags >> 6) & 7;
   if(curtype==FORMULA || curtype==FFORMULA ) {
      extern char maxfn;
      numtrig = maxfn;
      }
   promptnum = numparams;

   if ((i = numtrigfn) > 25) i = 25;
   while (--i >= 0)
      trignameptr[i] = trigfn[i].name;
   for (i = 0; i < numtrig; i++) {
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = trigndx[i];
      paramvalues[promptnum].uval.ch.llen = numtrigfn;
      paramvalues[promptnum].uval.ch.vlen = 6;
      paramvalues[promptnum].uval.ch.list = trignameptr;
      choices[promptnum] = trg[i];
      ++promptnum;
      }

   if (curfractalspecific->orbit_bailout)
      if (potparam[0] != 0.0 && potparam[2] != 0.0) {
	 paramvalues[promptnum].type = '*';
	 choices[promptnum] = "Bailout: continuous potential (Y screen) value in use";
	 ++promptnum;
	 }
      else {
	 choices[promptnum] = "Bailout value (0 means use default)";
	 paramvalues[promptnum].type = 'i';
	 paramvalues[promptnum].uval.ival = oldbailout = bailout;
	 ++promptnum;
	 paramvalues[promptnum].type = '*';
	 i = curfractalspecific->orbit_bailout;
	 tmpptr = typename;
	 if (usr_biomorph != -1) {
	    i = 100;
	    tmpptr = "biomorph";
	    }
	 sprintf(bailoutmsg,"    (%s default is %d)",tmpptr,i);
	 choices[promptnum] = bailoutmsg;
	 ++promptnum;
	 }

   if (caller				/* <z> command ? */
      && (display3d > 0 || promptnum == 0)) {
       static char far msg[]={"Current type has no type-specific parameters"};
       stopmsg(20,msg);
       goto gfp_exit;
       }

   /* note: max of 9 prompt lines now, plus hdgs & instr of course */

   dstack[0] = 0;
   if ((i = curfractalspecific->helpformula) < -1) {
      if (i == -2) { /* special for formula */
	 filename = FormFileName;
	 entryname = FormName;
	 }
      else {	 /* -3, special for lsystem */
	 filename = LFileName;
	 entryname = LName;
	 }
      if (find_file_item(filename,entryname,&entryfile) == 0) {
	 load_entry_text(entryfile,dstack,16);
	 fclose(entryfile);
	 }
      }
   else if (i >= 0) {
      int c,lines;
      if (i = read_help_topic(i,0,2000,dstack) > 0) i = 0;
      dstack[2000-i] = 0;
      i = j = lines = 0; k = 1;
      while ((c = dstack[i++])) {
	 /* stop at ctl, blank, or line with col 1 nonblank, max 16 lines */
	 if (k && c == ' ' && ++k <= 5) { } /* skip 4 blanks at start of line */
	 else {
	    if (c == '\n') {
	       if (k) break; /* blank line */
	       if (++lines >= 16) break;
	       k = 1;
	       }
	    else if (c < 16) /* a special help format control char */
	       break;
	    else {
	       if (k == 1) /* line starts in column 1 */
		  break;
	       k = 0;
	       }
	    dstack[j++] = c;
	    }
	 }
      while (--j >= 0 && dstack[j] == '\n') { }
      dstack[j+1] = 0;
      }

   sprintf(msg,
	   "Parameters for fractal type %s\n(Press F6 for corner parameters)",
	   typename);
   while (1) {
      oldhelpmode = helpmode;
      helpmode = curfractalspecific->helptext;
      i = fullscreen_prompt(msg,promptnum,choices,paramvalues,0,0x40,dstack);
      helpmode = oldhelpmode;
      if (i < 0) {
	 if (ret == 0)
	    ret = -1;
	 goto gfp_exit;
	 }
      if (i != F6) break;
      if (get_corners() > 0)
	 ret = 1;
      }

   for ( i = 0; i < numparams; i++) {
      if (param[i] != paramvalues[i].uval.dval) {
	 param[i] = paramvalues[i].uval.dval;
	 ret = 1;
	 }
      }

   for ( i = 0; i < numtrig; i++) {
      j = numparams + i;
      if (paramvalues[j].uval.ch.val != trigndx[i]) {
	 set_trig_array(i,trigfn[paramvalues[j].uval.ch.val].name);
	 ret = 1;
	 }
      }

   if ((potparam[0] == 0.0 || potparam[2] == 0.0)
     && curfractalspecific->orbit_bailout) {
      bailout = paramvalues[numparams+numtrig].uval.ival;
      if (bailout != 0 && (bailout < 4 || bailout > 32000))
	 bailout = oldbailout;
      if (bailout != oldbailout)
	 ret = 1;
      }

gfp_exit:
   curfractalspecific = &fractalspecific[fractype];
   EXIT_OVLY;
   return(ret);
}

/* --------------------------------------------------------------------- */

   static FILE *gfe_file;

static long get_file_entry(int type,char *title,char *fmask,
			  char *filename,char *entryname)
{
   /* Formula, LSystem, etc type structure, select from file */
   /* containing definitions in the form    name { ... }     */
   int newfile,firsttry;
   long entry_pointer;
   extern char dstack[4096];
   newfile = 0;
   while (1) {
      firsttry = 0;
      /* pb: binary mode used here - it is more work, but much faster, */
      /*     especially when ftell or fgetpos is used		       */
      while (newfile || (gfe_file = fopen(filename, "rb")) == NULL) {
	 char buf[60];
	 newfile = 0;
	 if (firsttry) {
        extern char s_cantfind[];
	    sprintf(temp1,s_cantfind, filename);
	    stopmsg(0,temp1);
	    }
	 sprintf(buf,"Select %s File",title);
	 if (getafilename(buf,fmask,filename) < 0)
	    return -1;
	 firsttry = 1; /* if around open loop again it is an error */
	 }
      setvbuf(gfe_file,dstack,_IOFBF,4096); /* improves speed when file is big */
      newfile = 0;
      if ((entry_pointer = gfe_choose_entry(type,title,filename,entryname)) == -2) {
	 newfile = 1; /* go to file list, */
	 continue;    /* back to getafilename */
	 }
      if (entry_pointer == -1)
	 return -1;
      switch (type) {
	 case GETFORMULA:
	    if (RunForm(entryname) == 0) return 0;
	    break;
	 case GETLSYS:
	    if (LLoad() == 0) return 0;
	    break;
	 case GETIFS:
	    if (ifsload() == 0) {
	       fractype = (ifs_type == 0) ? IFS : IFS3D;
	       curfractalspecific = &fractalspecific[fractype];
	       set_default_parms(); /* to correct them if 3d */
	       return 0;
	       }
	    break;
	 case GETPARM:
	    return entry_pointer;
	 }
      }
}

   struct entryinfo {
      char name[ITEMNAMELEN+1];
      long point; /* points to the ( or the { following the name */
      };
   static struct entryinfo **gfe_choices; /* for format_getparm_line */
   static char *gfe_title;

static long gfe_choose_entry(int type,char *title,char *filename,char *entryname)
/* subrtn of get_file_entry, separated so that storage gets freed up */
{
#define MAXENTRIES 200
   int numentries, i;
   char buf[101];
   struct entryinfo *choices[MAXENTRIES];
   int attributes[MAXENTRIES];
   void (*formatitem)();
   int boxwidth,boxdepth,colwidth;
   long file_offset,name_offset;
   extern struct entryinfo boxx[MAXENTRIES];

   gfe_choices = &choices[0];
   gfe_title = title;
   for (i = 0; i < MAXENTRIES; i++) {
      attributes[i] = 1;
      choices[i] = &boxx[i];
      }
   numentries = 0;
   file_offset = -1;

   helptitle(); /* to display a clue when file big and next is slow */

   while (1) { /* scan the file for entry names */
      int c,len;
      do {
	 ++file_offset;
	 c = getc(gfe_file);
	 } while (c == ' ' /* skip white space */
	       || c == '\t' || c == '\n' || c == '\r');
      if (c == ';') {
	 do {
	    ++file_offset;
	    c = getc(gfe_file);
	    } while (c != '\n' && c != EOF && c != '\x1a');
	 if (c == EOF || c == '\x1a') break;
	 continue;
	 }
      name_offset = file_offset;
      len = 0; /* next equiv roughly to fscanf(..,"%40[^ \n\r\t({\x1a]",buf) */
      while (c != ' ' && c != '\t' && c != '('
	&& c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\x1a') {
	 if (len < 40) buf[len++] = c;
	 c = getc(gfe_file);
	 ++file_offset;
	 }
      buf[len] = 0;
      while (c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\x1a') {
	 c = getc(gfe_file);
	 ++file_offset;
	 }
      if (c == '{') {
	 while (c != '}' && c != EOF && c != '\x1a') {
	    c = getc(gfe_file);
	    ++file_offset;
	    }
	 if (c != '}') break;
	 buf[ITEMNAMELEN] = 0;
	 if (buf[0] != 0 && stricmp(buf,"comment") != 0) {
	    strcpy(boxx[numentries].name,buf);
	    boxx[numentries].point = name_offset;
	    if (++numentries >= MAXENTRIES) {
	       sprintf(buf,"Too many entries in file, first %d used",MAXENTRIES);
	       stopmsg(0,buf);
	       break;
	       }
	    }
	 }
      else
	 if (c == EOF || c == '\x1a') break;
      }

   if (numentries == 0) {
      static char far msg[]={"File doesn't contain any valid entries"};
      stopmsg(0,msg);
      fclose(gfe_file);
      return -2; /* back to file list */
      }

   qsort((char **)choices,numentries,sizeof(char *),lccompare);

   strcpy(buf,entryname); /* preset to last choice made */
   sprintf(temp1,"%s Selection\nFile: %s",title,filename);
   formatitem = NULL;
   boxwidth = colwidth = boxdepth = 0;
   if (type == GETPARM) {
      formatitem = format_parmfile_line;
      boxwidth = 1;
      boxdepth = 16;
      colwidth = 76;
      }
   i = fullscreen_choice(8,temp1,NULL,
      "Press F6 to select different file, F2 for details of highlighted choice",
			   numentries,(char **)choices,attributes,
			   boxwidth,boxdepth,colwidth,0,
			   formatitem,buf,NULL,check_gfe_key);
   fclose(gfe_file);
   if (i < 0) {
      if (i == 0-F6)
	 return -2; /* go back to file list */
      return -1;    /* cancel */
      }
   strcpy(entryname, choices[i]->name);
   return(choices[i]->point);
}

static int check_gfe_key(int curkey,int choice)
{
   char infhdg[60];
   char far *infbuf;

   if (curkey == F6)
      return 0-F6;
   if (curkey == F2) {
      infbuf = MK_FP(extraseg,0);
      fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
      load_entry_text(gfe_file,infbuf,16);
      strcpy(infhdg,gfe_title);
      strcat(infhdg," file entry:\n\n");
 /* ... instead, call help with buffer?  heading added */
      stackscreen();
      helptitle();
      setattr(1,0,C_GENERAL_MED,24*80);
      putstring(2,1,C_GENERAL_HI,infhdg);
      textcbase = 2; /* left margin is 2 */
      putstring(-1,0,C_GENERAL_MED,infbuf);
      putstring(-1,0,C_GENERAL_LO,"\n\n\nPress any key to return to selection list");
      textcbase = 0;
      movecursor(25,80);
      getakeynohelp();
      unstackscreen();
      }
   return 0;
}

static void load_entry_text(FILE *entfile,char far *buf,int maxlines)
{
   int linect,linelen,c;
   linect = linelen = 0;
   while ((c = fgetc(entfile)) != EOF && c != '\x1a') {
      if (c != '\r') {
	 if (c == '\t') {
	    while ((linelen % 8) != 7 && linelen < 75) { /* 76 wide max */
	       *(buf++) = ' ';
	       ++linelen;
	       }
	    c = ' ';
	    }
	 if (c == '\n') {
	    if (++linect > maxlines) break;
	    linelen = -1;
	    }
	 if (++linelen > 75) {
	    if (linelen == 76) *(buf++) = '\x11';
	    }
	 else
	    *(buf++) = c;
	 if (c == '}') break;
	 }
      }
   *buf = 0;
}

static void format_parmfile_line(int choice,char *buf)
{
   int c,i;
   char line[80];
   fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
   while (getc(gfe_file) != '{') { }
   while ((c = getc(gfe_file)) == ' ' || c == '\t' || c == ';') { }
   i = 0;
   while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\x1a') {
      line[i++] = (c == '\t') ? ' ' : c;
      c = getc(gfe_file);
      }
   line[i] = 0;
   sprintf(buf,"%-20s%-56s",gfe_choices[choice]->name,line);
}

/* --------------------------------------------------------------------- */

int get_fract3d_params() /* prompt for 3D fractal parameters */
{
   int i,k,ret,oldhelpmode;
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

   ENTER_OVLY(OVLY_PROMPTS);
   stackscreen();

   k = 0;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = XROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = YROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ZROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ZVIEWER;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = XSHIFT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = YSHIFT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = glassestype;

   oldhelpmode = helpmode;
   helpmode = HELP3DFRACT;
   i = fullscreen_prompt("3D Parameters",k,ifs3d_prompts,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      ret = -1;
      goto get_f3d_exit;
      }

   ret = k = 0;
   XROT    =  uvalues[k++].uval.ival;
   YROT    =  uvalues[k++].uval.ival;
   ZROT    =  uvalues[k++].uval.ival;
   ZVIEWER =  uvalues[k++].uval.ival;
   XSHIFT  =  uvalues[k++].uval.ival;
   YSHIFT  =  uvalues[k++].uval.ival;
   glassestype = uvalues[k++].uval.ival;
   if (glassestype < 0 || glassestype > 3) glassestype = 0;
   if (glassestype)
      if (get_funny_glasses_params() || check_mapfile())
	 ret = -1;

get_f3d_exit:
   unstackscreen();
   EXIT_OVLY;
   return(ret);
}

/* --------------------------------------------------------------------- */

int get_3d_params()	/* prompt for 3D parameters */
{
   char *choices[10];
   int attributes[21];
   int sphere;
   char *s;
   char *prompts3d[21];
   struct fullscreenvalues uvalues[21];
   int i, k;
   int oldhelpmode;

   ENTER_OVLY(OVLY_PROMPTS);

restart_1:
	if (Targa_Out && overlay3d)
		Targa_Overlay = 1;

   k=-1;

   prompts3d[++k]= "Preview Mode?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = preview;

   prompts3d[++k]= "    Show Box?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = showbox;

   prompts3d[++k]= "Coarseness, preview/grid/ray (in y dir)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = previewfactor;

   prompts3d[++k]= "Spherical Projection?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = sphere = SPHERE;

   prompts3d[++k]= "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = glassestype;

   prompts3d[++k]= "Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = RAY;
   prompts3d[++k]= "                4=MTV, 5=RAYSHADE, 6=ACROSPIN)";
   uvalues[k].type = '*';

   prompts3d[++k]= "    Brief output?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = BRIEF;

   check_writefile(ray_name,".ray");
   prompts3d[++k]= "    Output File Name";
   uvalues[k].type = 's';
   strcpy(uvalues[k].uval.sval,ray_name);

   prompts3d[++k]= "Targa output?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = Targa_Out;

   oldhelpmode = helpmode;
   helpmode = HELP3DMODE;

   k = fullscreen_prompt("3D Mode Selection",k+1,prompts3d,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0) {
      EXIT_OVLY;
      return(-1);
      }

   k=0;

   preview = uvalues[k++].uval.ch.val;

   showbox = uvalues[k++].uval.ch.val;

   previewfactor  = uvalues[k++].uval.ival;

   sphere = uvalues[k++].uval.ch.val;

   glassestype = uvalues[k++].uval.ival;

   RAY = uvalues[k++].uval.ival;
   k++;

   BRIEF = uvalues[k++].uval.ch.val;

   strcpy(ray_name,uvalues[k++].uval.sval);

   Targa_Out = uvalues[k++].uval.ch.val;

   /* check ranges */
   if(previewfactor < 2)
      previewfactor = 2;
   if(previewfactor > 2000)
      previewfactor = 2000;

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
   if(glassestype > 3)
      glassestype = 3;
   if(glassestype)
      whichimage = 1;

   if (RAY < 0)
      RAY = 0;
   if (RAY > 6)
      RAY = 6;

   if (!RAY)
   {
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
      helpmode = HELP3DFILL;
      i = fullscreen_choice(CHOICEHELP,"Select 3D Fill Type",NULL,NULL,
			      k,choices,attributes,
			      0,0,0,FILLTYPE+1,NULL,NULL,NULL,NULL);
      helpmode = oldhelpmode;
      if (i < 0)
	 goto restart_1;
      FILLTYPE = i-1;

      if(glassestype)
      {
	 if(get_funny_glasses_params())
            goto restart_1;
         }
         if (check_mapfile())
             goto restart_1;
      }
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
      i = 0;
      if (!RAY)
      {
	 prompts3d[i++] = "X-axis rotation in degrees";
	 prompts3d[i++] = "Y-axis rotation in degrees";
	 prompts3d[i++] = "Z-axis rotation in degrees";
      }
      prompts3d[i++] = "X-axis scaling factor in pct";
      prompts3d[i++] = "Y-axis scaling factor in pct";
   }
   k = 0;
   if (!(RAY && !SPHERE))
   {
      uvalues[k].uval.ival   = XROT	 ;
      uvalues[k++].type = 'i';
      uvalues[k].uval.ival   = YROT	 ;
      uvalues[k++].type = 'i';
      uvalues[k].uval.ival   = ZROT	 ;
      uvalues[k++].type = 'i';
   }
   uvalues[k].uval.ival   = XSCALE    ;
   uvalues[k++].type = 'i';
   uvalues[k].uval.ival   = YSCALE    ;
   uvalues[k++].type = 'i';
   prompts3d[k] = "Surface Roughness scaling factor in pct";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ROUGH     ;

   prompts3d[k]= "'Water Level' (minimum color value)";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = WATERLINE ;

   if(!RAY)
   {
      prompts3d[k]= "Perspective distance [1 - 999, 0 for no persp]";
      uvalues[k].type = 'i';
      uvalues[k++].uval.ival = ZVIEWER	 ;

   prompts3d[k]= "X shift with perspective (positive = right)";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = XSHIFT    ;

   prompts3d[k]= "Y shift with perspective (positive = up   )";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = YSHIFT    ;

   prompts3d[k]= "Image non-perspective X adjust (positive = right)";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = xtrans    ;

   prompts3d[k]= "Image non-perspective Y adjust (positive = up)";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ytrans    ;

   prompts3d[k]= "First transparent color";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = transparent[0];

   prompts3d[k]= "Last transparent color";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = transparent[1];
   }

   prompts3d[k]= "Randomize Colors      (0 - 7, '0' disables)";
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = RANDOMIZE;

   if (SPHERE)
      s = "              Sphere 3D Parameters\n\
Sphere is on its side; North pole to right\n\
Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right";
   else
      s = "              Planar 3D Parameters\n\
Pre-rotation X axis is screen top; Y axis is left side\n\
Pre-rotation Z axis is coming at you out of the screen!";

   helpmode = HELP3DPARMS;
   k = fullscreen_prompt(s,k,prompts3d,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      goto restart_1;

   k = 0;
   if (!(RAY && !SPHERE))
   {
      XROT    = uvalues[k++].uval.ival;
      YROT    = uvalues[k++].uval.ival;
      ZROT    = uvalues[k++].uval.ival;
   }
   XSCALE     = uvalues[k++].uval.ival;
   YSCALE     = uvalues[k++].uval.ival;
   ROUGH      = uvalues[k++].uval.ival;
   WATERLINE  = uvalues[k++].uval.ival;
   if (!RAY)
   {
      ZVIEWER = uvalues[k++].uval.ival;
   XSHIFT     = uvalues[k++].uval.ival;
   YSHIFT     = uvalues[k++].uval.ival;
   xtrans     = uvalues[k++].uval.ival;
   ytrans     = uvalues[k++].uval.ival;
   transparent[0] = uvalues[k++].uval.ival;
   transparent[1] = uvalues[k++].uval.ival;
   }
   RANDOMIZE  = uvalues[k++].uval.ival;
   if (RANDOMIZE >= 7) RANDOMIZE = 7;
   if (RANDOMIZE <= 0) RANDOMIZE = 0;

   if ((Targa_Out || ILLUMINE || RAY))
	if(get_light_params())
	    goto restart_3;

EXIT_OVLY;
return(0);
}

/* --------------------------------------------------------------------- */
static int get_light_params()
{
   char *prompts3d[13];
   struct fullscreenvalues uvalues[13];

   int k;
   int oldhelpmode;

   /* defaults go here */

   k = -1;

   if (ILLUMINE || RAY)
   {
   prompts3d[++k]= "X value light vector";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = XLIGHT    ;

   prompts3d[++k]= "Y value light vector";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = YLIGHT    ;

   prompts3d[++k]= "Z value light vector";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = ZLIGHT    ;

		if (!RAY)
		{
   prompts3d[++k]= "Light Source Smoothing Factor";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = LIGHTAVG  ;

			prompts3d[++k] = "Ambient";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = Ambient;
		}
   }

   if (Targa_Out && !RAY)
   {
	prompts3d[++k]	= "Haze Factor        (0 - 100, '0' disables)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival= haze;

		if (!Targa_Overlay)
	check_writefile(light_name,".tga");
      prompts3d[++k]= "Targa File Name  (Assume .tga)";
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval,light_name);

      prompts3d[++k]= "Back Ground Color (0 - 255)";
      uvalues[k].type = '*';

      prompts3d[++k]= "   Red";
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[0];

      prompts3d[++k]= "   Green";
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[1];

      prompts3d[++k]= "   Blue";
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[2];

      prompts3d[++k]= "Overlay Targa File? (Y/N)";
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = Targa_Overlay;

   }

   prompts3d[++k]= "";

   oldhelpmode = helpmode;
   helpmode = HELP3DLIGHT;
   k = fullscreen_prompt("Light Source Parameters",k,prompts3d,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      return(-1);

   k = 0;
   if (ILLUMINE)
   {
      XLIGHT   = uvalues[k++].uval.ival;
      YLIGHT   = uvalues[k++].uval.ival;
      ZLIGHT   = uvalues[k++].uval.ival;
      if (!RAY)
		{
      LIGHTAVG = uvalues[k++].uval.ival;
      Ambient  = uvalues[k++].uval.ival;
      if (Ambient >= 100) Ambient = 100;
      if (Ambient <= 0) Ambient = 0;
		}
   }

   if (Targa_Out && !RAY)
   {
	haze  =  uvalues[k++].uval.ival;
	if (haze >= 100) haze = 100;
	if (haze <= 0) haze = 0;
    	strcpy(light_name,uvalues[k++].uval.sval);
		/* In case light_name conflicts with an existing name it is checked
			again in line3d */
		k++;
    	back_color[0] = (char)uvalues[k++].uval.ival % 255;
    	back_color[1] = (char)uvalues[k++].uval.ival % 255;
    	back_color[2] = (char)uvalues[k++].uval.ival % 255;
    	Targa_Overlay = uvalues[k].uval.ch.val;
   }
   return(0);
}

/* --------------------------------------------------------------------- */


static int check_mapfile()
{
   extern unsigned char dacbox[256][3];
   extern unsigned char olddacbox[256][3];
   int askflag = 0;
   int i,oldhelpmode;
   strcpy(temp1,"*");
   if (mapset)
      strcpy(temp1,MAP_name);
   if (!(glassestype == 1 || glassestype == 2))
      askflag = 1;
   else
      strcpy(temp1,funnyglasses_map_name);
   while (TRUE) {
      if (askflag) {
	 oldhelpmode = helpmode;
	 helpmode = -1;
	 i = field_prompt(0,"\
Enter name of .MAP file to use,\n\
or '*' to use palette from the image to be loaded.",
		 NULL,temp1,60,NULL);
	 helpmode = oldhelpmode;
	 if (i < 0)
	    return(-1);
        if (temp1[0] == '*') {
            mapset = 0;
            break;
            }
        }
        memcpy(olddacbox,dacbox,256*3); /* save the DAC */
      i = ValidateLuts(temp1);
      memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
      if (i != 0) { /* Oops, somethings wrong */
         askflag = 1;
	 continue;
	 }
      mapset = 1;
      strcpy (MAP_name,temp1);
      break;
      }
   return(0);
}

static int get_funny_glasses_params()
{
   char *prompts3d[10];

   struct fullscreenvalues uvalues[10];

   int k;
   int oldhelpmode;

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
   uvalues[k].type = 'i';
   uvalues[k].uval.ival= eyeseparation;

   prompts3d[++k]= "Convergence adjust (positive = spread greater)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = xadjust;

   prompts3d[++k]= "Left  red image crop (% of screen)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_left;

   prompts3d[++k]= "Right red image crop (% of screen)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_right;

   prompts3d[++k]= "Left  blue image crop (% of screen)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_left;

   prompts3d[++k]= "Right blue image crop (% of screen)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_right;

   prompts3d[++k]= "Red brightness factor (%)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_bright;

   prompts3d[++k]= "Blue brightness factor (%)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_bright;

   if(glassestype == 1 || glassestype == 2)
   {
      prompts3d[++k]= "Map File name";
      uvalues[k].type = 's';
      strcpy(uvalues[k].uval.sval,funnyglasses_map_name);
   }

   oldhelpmode = helpmode;
   helpmode = HELP3DGLASSES;
   k = fullscreen_prompt("Funny Glasses Parameters",k+1,prompts3d,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      return(-1);

   k = 0;
   eyeseparation   =  uvalues[k++].uval.ival;
   xadjust	   =  uvalues[k++].uval.ival;
   red_crop_left   =  uvalues[k++].uval.ival;
   red_crop_right  =  uvalues[k++].uval.ival;
   blue_crop_left  =  uvalues[k++].uval.ival;
   blue_crop_right =  uvalues[k++].uval.ival;
   red_bright	   =  uvalues[k++].uval.ival;
   blue_bright	   =  uvalues[k++].uval.ival;

   if(glassestype == 1 || glassestype == 2)
      strcpy(funnyglasses_map_name,uvalues[k].uval.sval);
   return(0);
}

/* --------------------------------------------------------------------- */

static int edit_ifs_params()	/* prompt for IFS params */
{
   int totcols;
   int i, j, k, numlines, ret;
   FILE *tempfile;
   char msg[81];
   char filename[81];
   float ftemp;
   int oldhelpmode;

   if (!ifs_defn && !ifsload())
      return(-1);

   totcols = (ifs_type == 0) ? IFSPARM : IFS3DPARM;
   ret = 0;
   oldhelpmode = helpmode;
   helpmode = HT_IFS;

   for ( ;; ) {
static char far ifshdg2[]={"2D IFS Parameters"};
static char far ifshdg3[]={"3D IFS Parameters"};
static char far ifsparmmsg1[]={"#    a     b     c     d     e     f"};
static char far ifsparmmsg2[]={"     g     h     i     j     k     l"};
static char far ifsprompt[]={"\
Enter the number of the line you want to edit,\n\
S to save your edits in a file, or ENTER to end ==>"};
      int leftcol,promptrow,promptcol;

      for (numlines = 0; numlines < NUMIFS; numlines++) /* find the first zero entry */
	 if (ifs_defn[(numlines * totcols) + totcols - 1] <= 0.0001) break;

      helptitle();
      setattr(1,0,C_PROMPT_BKGRD,24*80); /* init rest of screen to background */
      putstringcenter(2,0,80,C_GENERAL_HI,(ifs_type == 0) ? ifshdg2 : ifshdg3);
      leftcol = (ifs_type == 0) ? 15 : 0;
      putstring(4,leftcol+1,C_GENERAL_HI,ifsparmmsg1);
      if (ifs_type != 0)
	 putstring(-1,-1,C_GENERAL_HI,ifsparmmsg2);
      putstring(-1,-1,C_GENERAL_HI,"   prob \n\n");

      if ((k = numlines) > 12) k = 12;
      for (i = 0; i < k; i++) {
	 sprintf(msg,"%2d", i+1);
	 putstring(5+i,leftcol,C_GENERAL_HI,msg);
	 for (j = 0; j < totcols; j++) {
	    sprintf(msg,"%6.2f",ifs_defn[(i*totcols)+j]);
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
		     fprintf(tempfile, "%6.2f", (float)ifs_defn[(i*totcols)+j]);
		  fprintf(tempfile, "\n");
		  }
	       fclose(tempfile);
	       ifs_changed = 0;
	       }
	    else {
	       static char far msg[]={"Could not create file"};
	       stopmsg(0,msg);
	       }
	    }
	 continue;
	 }
      i = atoi(temp1) - 1;
      if (i >= 0 && i <= numlines) {
	 for (j = 0; j < totcols; j++) {
	    if (j < totcols-1)
	       sprintf(msg,"Parameter %c",'a'+j);
	    else
	       sprintf(msg,"Probability");
	    putstring(promptrow+2,25,C_GENERAL_HI,msg);
	    sprintf(temp1,"%6.2f",(float)ifs_defn[k=(i*totcols)+j]);
	    if (input_field(1,C_GENERAL_INPUT,temp1,6,
			    textrow,textcol+1,NULL) < 0)
	       break;
	    if (ifs_defn[k] != (ftemp = atof(temp1))) {
	       ifs_defn[k] = ftemp;
	       ret = ifs_changed = 1;
	       }
	    }
	 memset(msg,' ',80); msg[81] = 0;
	 putstring(promptrow+2,0,C_PROMPT_BKGRD,msg);
	 }
      }

   helpmode = oldhelpmode;
   return(ret);
}

/* --------------------------------------------------------------------- */
/*
	get_toggles() is called from FRACTINT.C whenever the 'x' key
	is pressed.  This routine prompts for several options,
	sets the appropriate variables, and returns the following code
	to the calling routine:

	-1  routine was ESCAPEd - no need to re-generate the image.
	 0  nothing changed, or minor variable such as "overwrite=".
	    No need to re-generate the image.
	 1  major variable changed (such as "inside=").  Re-generate
	    the image.

	Finally, remember to insert variables in the list *and* check
	for them in the same order!!!
*/

int get_toggles()
{
   char *choices[20];
   int oldhelpmode;
   char prevsavename[81];
   struct fullscreenvalues uvalues[25];
   int i, j, k;
   char old_usr_stdcalcmode;
   int old_maxit,old_inside,old_outside,old_soundflag;
   int old_logflag,old_biomorph,old_decomp;
   int old_fillcolor;
   static char *calcmodes[] ={"1","2","g","b","t"};
   static char *soundmodes[5]={"yes","no","x","y","z"};

   ENTER_OVLY(OVLY_PROMPTS);

   k = -1;

   k++;
   choices[k] =  "Passes (1, 2, g[uessing], b[oundary trace], t[esseral])";
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 3;
   uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
   uvalues[k].uval.ch.list = calcmodes;
   uvalues[k].uval.ch.val = (usr_stdcalcmode == '1') ? 0
			  : (usr_stdcalcmode == '2') ? 1
                          : (usr_stdcalcmode == 'g') ? 2
                          : (usr_stdcalcmode == 'b') ? 3 :4 ;
   old_usr_stdcalcmode = usr_stdcalcmode;

   k++;
   choices[k] =  "Floating Point Algorithm";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = usr_floatflag;

   k++;
   choices[k] = "Maximum Iterations (2 to 32767)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_maxit = maxit;

   k++;
   choices[k] = "Inside Color (<nnn>,maxiter,zmag,bof60,bof61,epscr,star)";
   uvalues[k].type = 's';
   if(inside == -59)
      strcpy(uvalues[k].uval.sval,s_zmag);
   else if(inside == -60)
      strcpy(uvalues[k].uval.sval,s_bof60);
   else if(inside == -61)
      strcpy(uvalues[k].uval.sval,s_bof61);
   else if(inside == -100)
      strcpy(uvalues[k].uval.sval,s_epscross);
   else if(inside == -101)
      strcpy(uvalues[k].uval.sval,s_startrail);
   else if(inside == -1)
      strcpy(uvalues[k].uval.sval,s_maxiter);
   else
      sprintf(uvalues[k].uval.sval,"%d",inside);
   old_inside = inside;

   k++;
   choices[k] = "Outside Color (<nnn>,iter,real,imag,mult,summ)";
   uvalues[k].type = 's';
   if(outside == -1)
      strcpy(uvalues[k].uval.sval,s_iter);
   else if(outside == -2)
      strcpy(uvalues[k].uval.sval,s_real);
   else if(outside == -3)
      strcpy(uvalues[k].uval.sval,s_imag);
   else if(outside == -4)
      strcpy(uvalues[k].uval.sval,s_mult);
   else if(outside == -5)
      strcpy(uvalues[k].uval.sval,s_sum);
   else
      sprintf(uvalues[k].uval.sval,"%d",outside);
   old_outside = outside;

   k++;
   choices[k] = "Savename (.GIF implied)";
   uvalues[k].type = 's';
   strcpy(prevsavename,savename);
   strcpy(uvalues[k].uval.sval,savename);

   k++;
   choices[k] = "File Overwrite ('overwrite=')";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = overwrite;

   k++;
   choices[k] = "Sound (no, yes, x, y, z)";
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 3;
   uvalues[k].uval.ch.llen = 5;
   uvalues[k].uval.ch.list = soundmodes;
   uvalues[k].uval.ch.val = 1 + (old_soundflag = soundflag);

   k++;
   if (rangeslen == 0) {
      choices[k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt)";
      uvalues[k].type = 'i';
      }
   else {
      choices[k] = "Log Palette (n/a, ranges= parameter is in effect)";
      uvalues[k].type = '*';
      }
   uvalues[k].uval.ival = old_logflag = LogFlag;

   k++;
   choices[k] = "Biomorph Color (-1 means OFF)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_biomorph = usr_biomorph;

   k++;
   choices[k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_decomp = decomp[0];

   k++;
   choices[k] = "Fill Color (normal,<nnn>) (works with passes=t and =b)";
   uvalues[k].type = 's';
   if(fillcolor < 0)
      strcpy(uvalues[k].uval.sval,s_normal);
   else
      sprintf(uvalues[k].uval.sval,"%d",fillcolor);
   old_fillcolor = fillcolor;
 
   k++;
   choices[k] = "Orbit delay (0 = none)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = orbit_delay;

/*
   k++;
   choices[k] = "Antialiasing (0 to 8)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = AntiAliasing;
*/

   oldhelpmode = helpmode;
   helpmode = HELPXOPTS;
   i = fullscreen_prompt("        Basic Options\n"
			 "(not all combinations make sense)",
	 k+1,choices,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      EXIT_OVLY;
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   usr_stdcalcmode = calcmodes[uvalues[++k].uval.ch.val][0];
   if (old_usr_stdcalcmode != usr_stdcalcmode) j = 1;

   if (uvalues[++k].uval.ch.val != usr_floatflag) {
      usr_floatflag = uvalues[k].uval.ch.val;
      j = 1;
      }

   ++k;
   maxit = uvalues[k].uval.ival;
   if (maxit < 2) maxit = 2;

/* 'maxit' is an int so it is always <= 32767, MCP 12-3-91
   if (maxit > 32767) maxit = 32767;
*/

   if (maxit != old_maxit) j = 1;

   if(strncmp(strlwr(uvalues[++k].uval.sval),s_zmag,4)==0)
      inside = -59;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_bof60,5)==0)
      inside = -60;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_bof61,5)==0)
      inside = -61;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_epscross,3)==0)
      inside = -100;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_startrail,4)==0)
      inside = -101;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_maxiter,5)==0)
      inside = -1;
   else
      inside = atoi(uvalues[k].uval.sval);
   if (inside != old_inside) j = 1;

   if(strncmp(strlwr(uvalues[++k].uval.sval),s_real,4)==0)
      outside = -2;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_imag,4)==0)
      outside = -3;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_mult,4)==0)
      outside = -4;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_sum,4)==0)
      outside = -5;
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_iter,4)==0)
      outside = -1;
   else
      outside = atoi(uvalues[k].uval.sval);
   if (outside != old_outside) j = 1;

   strcpy(savename,uvalues[++k].uval.sval);
   if (strcmp(savename,prevsavename))
      resave_flag = started_resaves = 0; /* forget pending increment */

   overwrite = uvalues[++k].uval.ch.val;

   soundflag = uvalues[++k].uval.ch.val - 1;
   if (soundflag != old_soundflag && (soundflag > 1 || old_soundflag > 1))
      j = 1;

   LogFlag = uvalues[++k].uval.ival;
   if (LogFlag != old_logflag) j = 1;

   usr_biomorph = uvalues[++k].uval.ival;
   if (usr_biomorph != old_biomorph) j = 1;

   decomp[0] = uvalues[++k].uval.ival;
   if (decomp[0] != old_decomp) j = 1;

   if(strncmp(strlwr(uvalues[++k].uval.sval),s_normal,4)==0)
      fillcolor = -1;
   else
      fillcolor = atoi(uvalues[k].uval.sval);
   if (fillcolor != old_fillcolor) j = 1;

   orbit_delay = uvalues[++k].uval.ival;

/*
   if(AntiAliasing != uvalues[++k].uval.ival) j = 1;
   AntiAliasing = uvalues[k].uval.ival;
   if(AntiAliasing < 0) AntiAliasing = 0;
   if(AntiAliasing > 8) AntiAliasing = 8;
*/

   EXIT_OVLY;
   return(j);
}

/*
	get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
   char *choices[20];
   int oldhelpmode;

   struct fullscreenvalues uvalues[25];
   int i, j, k;

   int old_rotate_lo,old_rotate_hi;
   int old_usr_distest,old_distestwidth;
   double old_potparam[3],old_inversion[3];

   ENTER_OVLY(OVLY_PROMPTS);

   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] = "Look for finite attractor";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = finattract;

   k++;
   choices[k] = "Potential Max Color (0 means off)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_potparam[0] = potparam[0];

   k++;
   choices[k] = "          Slope";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_potparam[1] = potparam[1];

   k++;
   choices[k] = "          Bailout";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_potparam[2] = potparam[2];

   k++;
   choices[k] = "          16 bit values";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = pot16bit;

   k++;
   choices[k] = "Distance Estimator (0=off, <0=edge, >0=on):";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_usr_distest = usr_distest;
   k++;
   choices[k] = "          width factor:";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_distestwidth = distestwidth;

   choices[k+1] = "Inversion radius or \"auto\" (0 means off)";
   choices[k+2] = "          center X coordinate or \"auto\"";
   choices[k+3] = "          center Y coordinate or \"auto\"";
   for (i = 0; i < 3; i++) {
      uvalues[++k].type = 's';
      if ((old_inversion[i] = inversion[i]) == AUTOINVERT)
	 sprintf(uvalues[k].uval.sval,"auto");
      else
	 sprintf(uvalues[k].uval.sval,"%g",inversion[i]);
      }
   k++;
   choices[k] = "  (use fixed radius & center when zooming)",
   uvalues[k].type = '*';

   k++;
   choices[k] = "Color cycling from color (0 ... 254)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_lo = rotate_lo;
   k++;
   choices[k] = "              to   color (1 ... 255)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_hi = rotate_hi;

   oldhelpmode = helpmode;
   helpmode = HELPYOPTS;
   i = fullscreen_prompt("       Extended Doodads\n"
			 "(not all combinations make sense)",
	 k+1,choices,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      EXIT_OVLY;
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   if (uvalues[++k].uval.ch.val != finattract) {
      finattract = uvalues[k].uval.ch.val;
      j = 1;
      }

   potparam[0] = uvalues[++k].uval.ival;
   if (potparam[0] != old_potparam[0]) j = 1;

   potparam[1] = uvalues[++k].uval.ival;
   if (potparam[0] != 0.0 && potparam[1] != old_potparam[1]) j = 1;

   potparam[2] = uvalues[++k].uval.ival;
   if (potparam[0] != 0.0 && potparam[2] != old_potparam[2]) j = 1;

   if (uvalues[++k].uval.ch.val != pot16bit) {
      pot16bit = uvalues[k].uval.ch.val;
      if (pot16bit) { /* turned it on */
	 if (potparam[0] != 0.0) j = 1;
	 }
      else /* turned it off */
	 if (dotmode != 11) /* ditch the disk video */
	    enddisk();
	 else /* keep disk video, but ditch the fraction part at end */
	    disk16bit = 0;
      }

   ++k;
   usr_distest = (uvalues[k].uval.ival > 32000) ? 32000 : uvalues[k].uval.ival;
   if (usr_distest != old_usr_distest) j = 1;
   ++k;
   distestwidth = uvalues[k].uval.ival;
   if (usr_distest && distestwidth != old_distestwidth) j = 1;

   for (i = 0; i < 3; i++) {
      if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
	 inversion[i] = AUTOINVERT;
      else
	 inversion[i] = atof(uvalues[k].uval.sval);
      if (old_inversion[i] != inversion[i]
	&& (i == 0 || inversion[0] != 0.0))
	 j = 1;
      }
   invert = (inversion[0] == 0.0) ? 0 : 3;
   ++k;

   rotate_lo = uvalues[++k].uval.ival;
   rotate_hi = uvalues[++k].uval.ival;
   if (rotate_lo < 0 || rotate_hi > 255 || rotate_lo > rotate_hi) {
      rotate_lo = old_rotate_lo;
      rotate_hi = old_rotate_hi;
      }

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
   struct fullscreenvalues uvalues[25];
   int i, k;
   float old_viewreduction,old_aspectratio;
   int old_viewwindow,old_viewcrop,old_viewxdots,old_viewydots;

   ENTER_OVLY(OVLY_PROMPTS);
   stackscreen();

   old_viewwindow    = viewwindow;
   old_viewcrop      = viewcrop;
   old_viewreduction = viewreduction;
   old_aspectratio   = finalaspectratio;
   old_viewxdots     = viewxdots;
   old_viewydots     = viewydots;

get_view_restart:
   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] = "Preview display? (no for full screen)";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = viewwindow;

   k++;
   choices[k] = "Auto window size reduction factor";
   uvalues[k].type = 'f';
   uvalues[k].uval.dval = viewreduction;

   k++;
   choices[k] = "Final media overall aspect ratio, y/x";
   uvalues[k].type = 'f';
   uvalues[k].uval.dval = finalaspectratio;

   k++;
   choices[k] = "Crop starting coordinates to new aspect ratio?";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = viewcrop;

   k++;
   choices[k] = "explicit size x pixels (0 for auto size)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = viewxdots;
   k++;
   choices[k] = "              y pixels (0 to base on aspect ratio)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = viewydots;

   k++;
   choices[k] = "";
   uvalues[k].type = '*';
   k++;
   choices[k] = "Press F4 to reset view parameters to defaults.";
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPVIEW;
   i = fullscreen_prompt("View Window Options",k+1,choices,uvalues,0,16,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      viewwindow    = old_viewwindow;
      viewcrop	    = old_viewcrop;
      viewreduction = old_viewreduction;
      finalaspectratio = old_aspectratio;
      viewxdots     = old_viewxdots;
      viewydots     = old_viewydots;
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

   viewwindow = uvalues[++k].uval.ch.val;

   viewreduction = uvalues[++k].uval.dval;

   if ((finalaspectratio = uvalues[++k].uval.dval) == 0)
      finalaspectratio = SCREENASPECT;

   viewcrop = uvalues[++k].uval.ch.val;

   viewxdots = uvalues[++k].uval.ival;
   viewydots = uvalues[++k].uval.ival;

   if (finalaspectratio != old_aspectratio && viewcrop)
      aspectratio_crop(old_aspectratio,finalaspectratio);

   i = 0;
   if (viewwindow != old_viewwindow
      || (viewwindow
	 && (  viewreduction != old_viewreduction
	    || finalaspectratio != old_aspectratio
	    || viewxdots != old_viewxdots
	    || (viewydots != old_viewydots && viewxdots) ) ) )
      i = 1;

   unstackscreen();
   EXIT_OVLY;
   return(i);
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
   int oldhelpmode;

   ENTER_OVLY(OVLY_PROMPTS);

   if(colors < 255) {
      static char far msg[]={"starfield requires 256 color mode"};
      stopmsg(0,msg);
      EXIT_OVLY;
      return(-1);
   }
   for (i = 0; i < 3; i++) {
      uvalues[i].uval.dval = starfield_values[i];
      uvalues[i].type = 'f';
   }
   stackscreen();
   oldhelpmode = helpmode;
   helpmode = HELPSTARFLD;
   i = fullscreen_prompt("Starfield Parameters",
			 3,starfield_prompts,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
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

   if (ValidateLuts(StarMap) != 0) {
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

int get_commands()		/* execute commands from file */
{
   int ret;
   FILE *parmfile;
   long point;
   int oldhelpmode;
   ENTER_OVLY(OVLY_PROMPTS);
   ret = 0;
   oldhelpmode = helpmode;
   helpmode = HELPPARMFILE;
   if ((point = get_file_entry(GETPARM,"Parameter Set",
			       commandmask,CommandFile,CommandName)) >= 0
     && (parmfile = fopen(CommandFile,"rb"))) {
      fseek(parmfile,point,SEEK_SET);
      ret = load_commands(parmfile);
      }
   helpmode = oldhelpmode;
   EXIT_OVLY;
   return(ret);
}

/* --------------------------------------------------------------------- */

void goodbye()			/* we done.  Bail out */
{
   extern unsigned char exitmode;
   extern int mode7text;
   extern int made_dsktemp;
   union REGS r;
   static char far goodbyemessage[]={"   Thank You for using FRACTINT"};
   setvideotext();
   r.h.al = (mode7text == 0) ? exitmode : 7;
   r.h.ah = 0;
   int86(0x10, &r, &r);
   printf("\n\n\n%Fs\n",goodbyemessage); /* printf takes far pointer */
   movecursor(6,0);
   discardgraphics(); /* if any emm/xmm tied up there, release it */
   stopslideshow();
   if (made_dsktemp)
      remove("FRACTINT.DSK");
   end_help();
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
   if(notroot == 0 && dir[0] && dir[0] != '\\') /* must be in root directory */
   {
      splitpath(tmpmask,drive,dir,fname,ext);
      strcpy(dir,"\\");
      makepath(tmpmask,drive,dir,fname,ext);
   }
   if(numtemplates > 1)
      strcat(tmpmask," *.pot");
   strcpy(temp1,hdg);
   strcat(temp1,"\nTemplate: ");
   strcat(temp1,tmpmask);
   strcpy(speedstr,filename);
   if (speedstr[0] == 0)
   {
      for (i=0; i<filecount; i++) /* find first file */
	 if (choices[i]->type == 0)
	    break;
      if (i >= filecount)
	 i = 0;
   }
   i = fullscreen_choice(0,temp1,NULL,NULL,
			 filecount,(char **)choices,attributes,5,99,12,
			 i,NULL,speedstr,filename_speedstr,NULL);
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
      /* if (strchr(speedstr,'.') == NULL)
	    strcat(speedstr,".gif"); */
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

static int get_corners()
{
   struct fullscreenvalues values[15];
   char *prompts[15];
   static char xprompt[]={"          X"};
   static char yprompt[]={"          Y"};
   static char zprompt[]={"          Z"};
   int i,nump,prompt_ret;
   int cmag,transp3d;
   double Xctr,Yctr,Mag;
   unsigned char ousemag;
   double oxxmin,oxxmax,oyymin,oyymax,oxx3rd,oyy3rd;
   double ozzmin,ozzmax,ottmin,ottmax;
   /* note that hdg[15] is used for non-transparent heading: */
   static char hdg[]={"Transparent 3d Image Coordinates"};
   int oldhelpmode;

   transp3d = (Transparent3D && fractalspecific[fractype].orbitcalc == Formula);
   oldhelpmode = helpmode;
   ousemag = usemag;
   oxxmin = xxmin; oxxmax = xxmax;
   oyymin = yymin; oyymax = yymax;
   oxx3rd = xx3rd; oyy3rd = yy3rd;
   ozzmin = zzmin; ozzmax = zzmax;
   ottmin = ttmin; ottmax = ttmax;

gc_loop:
   for (i = 0; i < 15; ++i)
      values[i].type = 'd'; /* most values on this screen are type d */
   cmag = (!transp3d && usemag && cvtcentermag(&Xctr, &Yctr, &Mag));

   if (cmag) {
      values[0].uval.dval = Xctr;
      prompts[0] = "Center X";
      values[1].uval.dval = Yctr;
      prompts[1] = "Center Y";
      values[2].uval.dval = Mag;
      prompts[2] = "Magnification";
      values[3].type = '*';
      prompts[3] = "";
      values[4].type = '*';
      prompts[4] = "Press F7 to switch to \"corners\" mode";
      nump = 5;
      }

   else {
      nump = 0;
      values[nump].type = '*';
      prompts[nump++] = "Top-Left Corner";
      values[nump].uval.dval = xxmin;
      prompts[nump++] = xprompt;
      values[nump].uval.dval = yymax;
      prompts[nump++] = yprompt;
      if (transp3d) {
	 values[nump].uval.dval = zzmin;
	 prompts[nump++] = zprompt;
	 }
      values[nump].type = '*';
      prompts[nump++] = "Bottom-Right Corner";
      values[nump].uval.dval = xxmax;
      prompts[nump++] = xprompt;
      values[nump].uval.dval = yymin;
      prompts[nump++] = yprompt;
      if (transp3d) {
	 values[nump].uval.dval = zzmax;
	 prompts[nump++] = zprompt;
	 }
      if (transp3d) {
	 values[nump].type = '*';
	 prompts[nump++] = "Time Step";
	 values[nump].uval.dval = ttmin;
	 prompts[nump++] = "          From";
	 values[nump].uval.dval = ttmax;
	 prompts[nump++] = "          To";
	 }
      else {
	 if (xxmin == xx3rd && yymin == yy3rd)
	    xx3rd = yy3rd = 0;
	 values[nump].type = '*';
	 prompts[nump++] = "Bottom-left (zeros for top-left X, bottom-right Y)";
	 values[nump].uval.dval = xx3rd;
	 prompts[nump++] = xprompt;
	 values[nump].uval.dval = yy3rd;
	 prompts[nump++] = yprompt;
	 values[nump].type = '*';
	 prompts[nump++] = "Press F7 to switch to \"center-mag\" mode";
	 }
      }

   values[nump].type = '*';
   prompts[nump++] = "Press F4 to reset to type default values";

   oldhelpmode = helpmode;
   helpmode = HELPCOORDS;
   prompt_ret = fullscreen_prompt((transp3d) ? hdg : &hdg[15],
		     nump, prompts, values, 0,
		     (transp3d) ? 0x10 : 0x90, /* function keys */
		     NULL);
   helpmode = oldhelpmode;

   if (prompt_ret < 0) {
      usemag = ousemag;
      xxmin = oxxmin; xxmax = oxxmax;
      yymin = oyymin; yymax = oyymax;
      xx3rd = oxx3rd; yy3rd = oyy3rd;
      zzmin = ozzmin; zzmax = ozzmax;
      ttmin = ottmin; ttmax = ottmax;
      return -1;
      }

   if (prompt_ret == F4) { /* reset to type defaults */
      xx3rd = xxmin = curfractalspecific->xmin;
      xxmax	    = curfractalspecific->xmax;
      yy3rd = yymin = curfractalspecific->ymin;
      yymax	    = curfractalspecific->ymax;
      if (viewcrop && finalaspectratio != SCREENASPECT)
	 aspectratio_crop(SCREENASPECT,finalaspectratio);
      goto gc_loop;
      }

   if (cmag) {
      if ( values[0].uval.dval != Xctr
	|| values[1].uval.dval != Yctr
	|| values[2].uval.dval != Mag) {
	 double radius,width;
	 radius = 1.0 / values[2].uval.dval;
	 width = radius * (1.0 / SCREENASPECT);
	 yymax	       = values[1].uval.dval + radius;
	 yy3rd = yymin = values[1].uval.dval - radius;
	 xxmax	       = values[0].uval.dval + width;
	 xx3rd = xxmin = values[0].uval.dval - width;
	 }
      }

   else {
      nump = 1;
      xxmin = values[nump++].uval.dval;
      yymax = values[nump++].uval.dval;
      if (transp3d)
	 zzmin = values[nump++].uval.dval;
      nump++;
      xxmax = values[nump++].uval.dval;
      yymin = values[nump++].uval.dval;
      if (transp3d)
	 zzmax = values[nump++].uval.dval;
      nump++;
      if (transp3d) {
	 ttmin = values[nump++].uval.dval;
	 ttmax = values[nump++].uval.dval;
	 }
      else {
	 xx3rd = values[nump++].uval.dval;
	 yy3rd = values[nump++].uval.dval;
	 if (xx3rd == 0 && yy3rd == 0) {
	    xx3rd = xxmin;
	    yy3rd = yymin;
	    }
	 }
      }

   if (prompt_ret == F7) { /* toggle corners/center-mag mode */
      if (usemag == 0)
	 if (cvtcentermag(&Xctr, &Yctr, &Mag) == 0)
	 {
	    static char far msg[] = 
	       {"Corners rotated or stretched, can't use center-mag"};
	    stopmsg(0,msg);
	 }   
	 else
	    usemag = 1;
      else
	 usemag = 0;
      goto gc_loop;
      }

   return((xxmin == oxxmin && xxmax == oxxmax
	&& yymin == oyymin && yymax == oyymax
	&& xx3rd == oxx3rd && yy3rd == oyy3rd
	&& zzmin == ozzmin && zzmax == ozzmax
	&& ttmin == ottmin && ttmax == ottmax) ? 0 : 1);
}


