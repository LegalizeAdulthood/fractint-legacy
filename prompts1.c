/*
	Various routines that prompt for things.
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef XFRACT
#include <dos.h>
#elif !defined(__386BSD__)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#endif
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif

#ifdef __hpux
#include <sys/param.h>
#define getwd(a) getcwd(a,MAXPATHLEN)
#endif

#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"
#include "prototyp.h"

extern int Targa_Overlay;
extern int Targa_Out;
extern BYTE back_color[];
extern float  screenaspect;

/* Routines defined in prompts2.c */

extern	int get_corners(void);
extern	int edit_ifs_params(void );
extern	int lccompare(VOIDCONSTPTR, VOIDCONSTPTR);

/* Routines used in prompts2.c */

	int prompt_checkkey(int curkey);
	long get_file_entry(int,char *,char *,char *,char *);

/* Routines in this module	*/

static	int prompt_valuestring(char *buf,struct fullscreenvalues *val);
static	int input_field_list(int attr,char *fld,int vlen,char **list,int llen,
			     int row,int col,int (*checkkey)(int));
static	int select_fracttype(int t);
static	int sel_fractype_help(int curkey, int choice);
	int select_type_params(int newfractype,int oldfractype);
static	void set_default_parms(void);
static	long gfe_choose_entry(int,char *,char *,char *);
static	int check_gfe_key(int curkey,int choice);
static	void load_entry_text(FILE *entfile,char far *buf,int maxlines);
static	void format_parmfile_line(int,char *);
static	int get_light_params(void );
static	int check_mapfile(void );
static	int get_funny_glasses_params(void );

int julibrot;   /* flag for julibrot */

extern int numtrigfn;
extern int bailout;

extern struct moreparams far moreparams[];
static char funnyglasses_map_name[16];
extern char temp1[256];   /* temporary strings	      */
extern int previewfactor;
extern int xtrans, ytrans;
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern char showbox; /* flag to show box and vector in preview */
extern SEGTYPE extraseg;
extern int whichimage;
extern int xadjust;
extern int eyeseparation;
extern int glassestype;
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

extern	double	xx3rd,yy3rd;	/* initial corner values    */
extern	int	invert; 	/* non-zero if inversion active */
extern	double	inversion[3];	/* radius, xcenter, ycenter */
extern	double	param[];	    /* up to six parameters    */
extern	double	potparam[3];	/* three potential parameters*/
extern	int	fractype;	/* if == 0, use Mandelbrot  */
extern	char	preview;	/* 3D preview mode flag */
extern	int	transparent[];	/* transparency values */
extern	int	usr_biomorph;	/* Biomorph flag */
extern	int	viewcrop;
extern	float	finalaspectratio;
extern	int	textcbase;
extern	char	boxy[];
extern	int	display3d;

extern char LFileName[];	/* file to find the formulas in */
extern char LName[];		/* Name of the Formula (if not null) */
extern char FormFileName[];
extern char FormName[];
extern char IFSFileName[];
extern char IFSName[];
extern float far *ifs_defn;
extern int ifs_type;
extern int ifs_changed;

/* fullscreen_choice options */
#define CHOICEHELP	4

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS	   2
#define GETPARM    3


char ifsmask[13]     = {"*.ifs"};
char formmask[13]    = {"*.frm"};
char lsysmask[13]    = {"*.l"};
char Glasses1Map[] = "glasses1.map";

void prompts1_overlay() { }	/* for restore_active_ovly */


/* --------------------------------------------------------------------- */

int promptfkeys;

int fullscreen_prompt(	/* full-screen prompting routine */
	char far *hdg,		/* heading, lines separated by \n */
	int numprompts, 	/* there are this many prompts (max) */
	char far **prompts,     /* array of prompting pointers */
	struct fullscreenvalues *values, /* array of values */
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
   int anyinput;
   int savelookatmouse;
   int curtype, curlen;
   char buf[81];
#ifndef XFRACT
static char far instr1[]  = {"Use " UPARR " and " DNARR " to select values to change"};
static char far instr2a[]  = {"Type in replacement value for selected field"};
static char far instr2b[]  = {"Use " LTARR " or " RTARR " to change value of selected field"};
#else
/* Some compilers don't accept "a" "b", so we have to fill in UPARR ourself.  */
static char far instr1[]  = {"Use up(K) and down(J) to select values to change"};
static char far instr2a[]  = {"Type in replacement value for selected field"};
static char far instr2b[]  = {"Use left(H) or right(L) to change value of selected field"};
#endif
static char far instr3a[] = {"Press ENTER when finished (or ESCAPE to back out)"};
static char far instr3b[] = {"Press ENTER when finished, ESCAPE to back out, or F1 for help"};
static char far instr0a[] = {"No changeable parameters; press ENTER to exit"};
static char far instr0b[] = {"No changeable parameters; press ENTER to exit or F1 for help"};

   ENTER_OVLY(OVLY_PROMPTS1);

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
   maxfldwidth = maxpromptwidth = maxcomment = anyinput = 0;
   for (i = 0; i < numprompts; i++) {
      if (values[i].type == 'y') {
	 static char *noyes[2] = {"no","yes"};
	 values[i].type = 'l';
	 values[i].uval.ch.vlen = 3;
	 values[i].uval.ch.list = noyes;
	 values[i].uval.ch.llen = 2;
	 }
      j = far_strlen(prompts[i]);
      if (values[i].type == '*') {
	 if (j > maxcomment)	 maxcomment = j;
	 }
      else {
         anyinput = 1;
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
#ifndef XFRACT
#define S1 '\xC4'
#define S2 "\xC0"
#define S3 "\xD9"
#define S4 "\xB3"
#define S5 "\xDA"
#define S6 "\xBF"
#else
#define S1 '-'
#define S2 "+" /* ll corner */
#define S3 "+" /* lr corner */
#define S4 "|"
#define S5 "+" /* ul corner */
#define S6 "+" /* ur corner */
#endif
      memset(buf,S1,80); buf[boxwidth-2] = 0;
      textcbase = boxcol + 1;
      putstring(extrarow,0,C_PROMPT_BKGRD,buf);
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,buf);
      --textcbase;
      putstring(extrarow,0,C_PROMPT_BKGRD,S5);
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,S2);
      textcbase += boxwidth - 1;
      putstring(extrarow,0,C_PROMPT_BKGRD,S6);
      putstring(extrarow+extralines-1,0,C_PROMPT_BKGRD,S3);
      textcbase = boxcol;
      for (i = 1; i < extralines-1; ++i) {
         putstring(extrarow+i,0,C_PROMPT_BKGRD,S4);
         putstring(extrarow+i,boxwidth-1,C_PROMPT_BKGRD,S4);
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

   if (!anyinput) {
      putstringcenter(instrrow,0,80,C_PROMPT_BKGRD,
        (helpmode > 0) ? instr0b : instr0a);
      movecursor(25,80);
      while (1) {
        while (!keypressed()) { }
        done = getakey();
        switch(done) {
           case ESC:
              done = -1;
           case ENTER:
           case ENTER_2:
              goto fullscreen_exit;
           case F2:
           case F3:
           case F4:
           case F5:
           case F6:
           case F7:
           case F8:
           case F9:
           case F10:
              if (promptfkeys & (1<<(done+1-F1)) )
                 goto fullscreen_exit;
           }
        }
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
	 if (curtype == 'D') j = 7;
	 if (curtype == 'f') j = 1;
	 i = input_field(j, C_PROMPT_INPUT, buf, curlen,
		promptrow+curchoice,valuecol,prompt_checkkey);
	 switch (values[curchoice].type) {
	    case 'd':
	    case 'D':
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

fullscreen_exit:
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
	 i = 16;    /* cellular needs 16 (was 15)*/
	 while (1) {
	    sprintf(buf,"%.*g",i,val->uval.dval);
	    if (strlen(buf) <= ret) break;
	    --i;
	    }
	 break;
      case 'D':
	 if (val->uval.dval<0) { /* We have to round the right way */
	     sprintf(buf,"%ld",(long)(val->uval.dval-.5));
	 }
	 else {
	     sprintf(buf,"%ld",(long)(val->uval.dval+.5));
	 }
	 ret = 20;
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

int prompt_checkkey(int curkey)
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
            if (nonalpha(curkey)) {
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
static int compare(const VOIDPTR i, const VOIDPTR j)
{
   return(strcmp(fractalspecific[(int)*((BYTE*)i)].name,
	       fractalspecific[(int)*((BYTE*)j)].name));
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
   ENTER_OVLY(OVLY_PROMPTS1);
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
   static char far head1[] = {"Select a Fractal Type"};
   static char far head2[] = {"Select Orbit Algorithm for Julibrot"};
   static char far instr[] = {"Press F2 for a description of the highlighted type"};
   char head[40];
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

   if(julibrot)
      far_strcpy(head,head2);
   else
      far_strcpy(head,head1);    
   if (t == IFS3D) t = IFS;
   i = j = -1;
   while(fractalspecific[++i].name) {
      if(julibrot)
      {
        int isinteger;
        isinteger = curfractalspecific->isinteger; 
	if (!((fractalspecific[i].flags & OKJB) && *fractalspecific[i].name != '*'))
	   continue;
      }
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
   done = fullscreen_choice(CHOICEHELP+8,head,NULL,instr,numtypes,
         (char **)choices,attributes,0,0,0,j,NULL,tname,NULL,sel_fractype_help);
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

int select_type_params(	/* prompt for new fractal type parameters */
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
#ifndef XFRACT
         "Current IFS parameters have been edited but not saved.\n"
         "Continue to replace them with new selection, cancel to keep them."};
#else
         "Current IFS parameters have been edited but not saved.\n\
Continue to replace them with new selection, cancel to keep them."};
#endif
      helpmode = HT_IFS;
      if (!ifs_defn || !ifs_changed || !stopmsg(22,ifsmsg))
	 if (get_file_entry(GETIFS,"IFS",ifsmask,IFSFileName,IFSName) < 0) {
	    ret = 1;
	    goto sel_type_exit;
	    }
      }

/* Added the following to accommodate fn bifurcations.  JCO 7/2/92 */
   if(((fractype == BIFURCATION) || (fractype == LBIFURCATION)) &&
     !((oldfractype == BIFURCATION) || (oldfractype == LBIFURCATION)))
        set_trig_array(0,"ident");
   if(((fractype == BIFSTEWART) || (fractype == LBIFSTEWART)) &&
     !((oldfractype == BIFSTEWART) || (oldfractype == LBIFSTEWART)))
        set_trig_array(0,"ident");
   if(((fractype == BIFLAMBDA) || (fractype == LBIFLAMBDA)) &&
     !((oldfractype == BIFLAMBDA) || (oldfractype == LBIFLAMBDA)))
        set_trig_array(0,"ident");
   if(((fractype == BIFEQSINPI) || (fractype == LBIFEQSINPI)) &&
     !((oldfractype == BIFEQSINPI) || (oldfractype == LBIFEQSINPI)))
        set_trig_array(0,"sin");
   if(((fractype == BIFADSINPI) || (fractype == LBIFADSINPI)) &&
     !((oldfractype == BIFADSINPI) || (oldfractype == LBIFADSINPI)))
        set_trig_array(0,"sin");

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
   if (viewcrop && finalaspectratio != screenaspect)
      aspectratio_crop(screenaspect,finalaspectratio);
   for (i = 0; i < 4; i++) {
      param[i] = curfractalspecific->paramvalue[i];
      if (fractype != CELLULAR) /* don't round cellular */
         roundfloatd(&param[i]);
   }
   if(curfractalspecific->flags&MORE) {
      int extra;
      if((extra=find_extra_param(fractype)) > -1)
         for(i=0;i<MAXPARAMS-4;i++) {
            param[i+4] = moreparams[extra].paramvalue[i];
         }
   }
}

#define MAXFRACTALS 25 
extern int neworbittype, num_fractal_types;

int build_fractal_list(int fractals[], int *last_val, char *nameptr[])
{
    int numfractals,i;

    numfractals = 0;
    for (i = 0; i < num_fractal_types; i++)
    {
        int isinteger;
        isinteger = curfractalspecific->isinteger; 
	if ((fractalspecific[i].flags & OKJB) && *fractalspecific[i].name != '*')
	{
	    fractals[numfractals] = i;
	    if (i == neworbittype || i == fractalspecific[neworbittype].tofloat)
		*last_val = numfractals;
	    nameptr[numfractals] = fractalspecific[i].name;
	    numfractals++;
	    if (numfractals >= MAXFRACTALS)
		break;
	}
    }
    return (numfractals);
}
char far v00[] = {"Orbit Algorithm"};
char far v0a[] = {"From cx (real part)"};
char far v1a[] = {"From cy (imaginary part)"};
char far v2a[] = {"To   cx (real part)"};
char far v3a[] = {"To   cy (imaginary part)"};

/* 4D Mandelbrot */
char far v0b[] = {"From cj (3rd dim)"};
char far v1b[] = {"From ck (4th dim)"};
char far v2b[] = {"To   cj (3rd dim)"};
char far v3b[] = {"To   ck (4th dim)"};

/* 4D Julia */
char far v0c[] = {"From zj (3rd dim)"};
char far v1c[] = {"From zk (4th dim)"};
char far v2c[] = {"To   zj (3rd dim)"};
char far v3c[] = {"To   zk (4th dim)"};

char far v4[] = {"Number of z pixels"};
char far v5[] = {"Location of z origin"};
char far v6[] = {"Depth of z"};
char far v7[] = {"Screen height"};
char far v8[] = {"Screen width"};
char far v9[] = {"Distance to Screen"};
char far v10[] = {"Distance between eyes"};
char far v11[] = {"3D Mode"};
char *juli3Doptions[] = {"monocular","lefteye","righteye","red-blue"};

/* --------------------------------------------------------------------- */
int get_fract_params(int caller)	/* prompt for type-specific parms */
{
   char far *v0 = v0a;
   char far *v1 = v1a;
   char far *v2 = v2a;
   char far *v3 = v3a;
   char far *juliorbitname = NULL;
   extern double mxmaxfp, mxminfp, mymaxfp, myminfp, originfp;
   extern double depthfp, heightfp, widthfp, distfp, eyesfp;
   extern int zdots; 
   char *nameptr[MAXFRACTALS];
   int fractals[MAXFRACTALS];
   int i,j,k;
   int curtype,numparams,numtrig,last_val;
   struct fullscreenvalues paramvalues[30];
   char far *choices[30];
   int oldbailout;
   int extra;
   int numextra;
   int promptnum;
   char msg[120];
   char *typename, *tmpptr;
   char bailoutmsg[50];
   int ret = 0;
   int oldhelpmode;
   static char far t11[] = {"Function (if needed by orbit formula)"};
   static char far t1[] = {"First Function"};
   static char far t2[] = {"Second Function"};
   static char far t3[] = {"Third Function"};
   static char far t4[] = {"Fourth Function"};
   static char far *trg[] = {t1, t2, t3, t4};
   extern char tstack[4096];
   char *filename,*entryname;
   FILE *entryfile;
   char *trignameptr[25];
   extern int  juli3Dmode;
   struct fractalspecificstuff far *jborbit;
   struct fractalspecificstuff far *savespecific;
   int firstparm;
   double oldparam[MAXPARAMS];
   ENTER_OVLY(OVLY_PROMPTS1);
   firstparm = 0;
   if(fractype==JULIBROT || fractype==JULIBROTFP)
      julibrot = 1;
   else
      julibrot = 0;
   curtype = fractype;
   if (curfractalspecific->name[0] == '*'
     && (i = fractalspecific->tofloat) != NOFRACTAL
     && fractalspecific[i].name[0] != '*')
      curtype = i;
   curfractalspecific = &fractalspecific[curtype];
#if 0
   if (curtype == IFS || curtype == IFS3D) {
      ret = ((caller) ? edit_ifs_params() : 0);
      goto gfp_exit;
      }
#endif
   tstack[0] = 0;
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
	 load_entry_text(entryfile,tstack,16);
	 fclose(entryfile);
	 }
      }
   else if (i >= 0) {
      int c,lines;
      if (i = read_help_topic(i,0,2000,tstack) > 0) i = 0;
      tstack[2000-i] = 0;
      i = j = lines = 0; k = 1;
      while ((c = tstack[i++])) {
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
	    tstack[j++] = c;
	    }
	 }
      while (--j >= 0 && tstack[j] == '\n') { }
      tstack[j+1] = 0;
      }
gfp_top:   
   promptnum = 0;
   if (julibrot)
   {
      i = select_fracttype(neworbittype);
      if (i < 0) 
      {
         if (ret == 0)
            ret = -1;
         julibrot = 0;
         goto gfp_exit;
      }
      else
         neworbittype = i;
      jborbit = &fractalspecific[neworbittype];
      juliorbitname = jborbit->name;
   }
   promptnum = 0;

   if(julibrot)
   {
      savespecific = curfractalspecific;
      curfractalspecific = jborbit;
      firstparm = 2; /* in most case Julibrot does not need first two parms */
      if(neworbittype == QUATJULFP     ||   /* all parameters needed */ 
         neworbittype == HYPERCMPLXJFP)
         firstparm = 0; 
      if(neworbittype == QUATFP        ||   /* no parameters needed */
         neworbittype == HYPERCMPLXFP)
         firstparm = 4; 
   }   
   numparams = 0;
   
   for (i = firstparm; i < 4; i++) 
   {
      char tmpbuf[30];
      if (curfractalspecific->param[i][0] == 0) 
         break;
      numparams++;
      choices[promptnum] = curfractalspecific->param[i];
      paramvalues[promptnum].type = 'd';
   
      if (choices[promptnum][0] == '+') 
      {
         choices[promptnum]++;
         paramvalues[promptnum].type = 'D';
      }
      sprintf(tmpbuf,"%.17g",param[i]);
      paramvalues[promptnum].uval.dval = atof(tmpbuf);
      oldparam[i] = paramvalues[promptnum++].uval.dval;
   }
   numextra = 0;
   if(curfractalspecific->flags&MORE && !julibrot)
   {
      if((extra=find_extra_param(fractype))<-1)
      {
         char msg[30];
         sprintf(msg,"find_extra_param error");
         stopmsg(0,msg);
      }
      else
      for (i=0;i<MAXPARAMS-4;i++) 
      {
         char tmpbuf[30];
         if (moreparams[extra].param[i][0] == 0) 
            break;
         numextra++;
         choices[promptnum] = moreparams[extra].param[i];
         paramvalues[promptnum].type = 'd';
         if (choices[promptnum][0] == '+') 
         {
            choices[promptnum]++;
            paramvalues[promptnum].type = 'D';
         }
         sprintf(tmpbuf,"%.17g",param[i+4]);
         paramvalues[promptnum].uval.dval = atof(tmpbuf);
         oldparam[i+4] = paramvalues[promptnum++].uval.dval;
      }
   }
   numtrig = (curfractalspecific->flags >> 6) & 7;
   if(curtype==FORMULA || curtype==FFORMULA ) {
      extern char maxfn;
      numtrig = maxfn;
      }

   if ((i = numtrigfn) > 27) i = 27;
   while (--i >= 0)
      trignameptr[i] = trigfn[i].name;
   for (i = 0; i < numtrig; i++) {
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = trigndx[i];
      paramvalues[promptnum].uval.ch.llen = numtrigfn;
      paramvalues[promptnum].uval.ch.vlen = 6;
      paramvalues[promptnum].uval.ch.list = trignameptr;
      choices[promptnum++] = trg[i];
      }
   if (*(typename = curfractalspecific->name) == '*')
        ++typename;

   if (curfractalspecific->orbit_bailout)
      if (potparam[0] != 0.0 && potparam[2] != 0.0) 
      {
	 paramvalues[promptnum].type = '*';
	 choices[promptnum++] = "Bailout: continuous potential (Y screen) value in use";
      }
      else 
      {
         static char far bailoutstr[] = {"Bailout value (0 means use default)"};
	 choices[promptnum] = bailoutstr;
	 paramvalues[promptnum].type = 'i';
	 paramvalues[promptnum++].uval.ival = oldbailout = bailout;
	 paramvalues[promptnum].type = '*';
	 i = curfractalspecific->orbit_bailout;
	 tmpptr = typename;
	 if (usr_biomorph != -1) 
	 {
	    i = 100;
	    tmpptr = "biomorph";
	 }
	 sprintf(bailoutmsg,"    (%s default is %d)",tmpptr,i);
	 choices[promptnum++] = bailoutmsg;
      }
   if (julibrot)
   {
      switch(neworbittype)
      {
      case QUATFP:
      case HYPERCMPLXFP:
          v0 = v0b; v1 = v1b; v2 = v2b; v3 = v3b;
          break;
      case QUATJULFP:
      case HYPERCMPLXJFP:
          v0 = v0c; v1 = v1c; v2 = v2c; v3 = v3c;
          break;
      default:
          v0 = v0a; v1 = v1a; v2 = v2a; v3 = v3a;
         break;
      }

      curfractalspecific = savespecific;
      paramvalues[promptnum].uval.dval = mxmaxfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v0;
      paramvalues[promptnum].uval.dval = mymaxfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v1;
      paramvalues[promptnum].uval.dval = mxminfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v2;
      paramvalues[promptnum].uval.dval = myminfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v3;
      paramvalues[promptnum].uval.ival = zdots;
      paramvalues[promptnum].type = 'i';
      choices[promptnum++] = v4;

      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = juli3Dmode;
      paramvalues[promptnum].uval.ch.llen = 4;
      paramvalues[promptnum].uval.ch.vlen = 9;
      paramvalues[promptnum].uval.ch.list = juli3Doptions;
      choices[promptnum++] = v11;

      paramvalues[promptnum].uval.dval = eyesfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v10;
      paramvalues[promptnum].uval.dval = originfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v5;
      paramvalues[promptnum].uval.dval = depthfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v6;
      paramvalues[promptnum].uval.dval = heightfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v7;
      paramvalues[promptnum].uval.dval = widthfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v8;
      paramvalues[promptnum].uval.dval = distfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v9;
   }

   if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
   {
      extern int  major_method;
      extern int  minor_method;

#ifdef RANDOM_RUN
      static char far JIIMstr1[] =
	"Breadth first, Depth first, Random Walk, Random Run?";
      static char *JIIMmethod[] = {"Breadth", "Depth", "Walk", "Run"};
#else
      static char far JIIMstr1[] =
	"Breadth first, Depth first, Random Walk";
      static char *JIIMmethod[] = {"Breadth", "Depth", "Walk"};
#endif
      static char far JIIMstr2[] =
	"Left first or Right first?";
      static char *JIIMleftright[] = {"Left", "Right"};

      choices[promptnum] = JIIMstr1;
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.list = JIIMmethod;
      paramvalues[promptnum].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
      paramvalues[promptnum].uval.ch.llen = 4;
#else
      paramvalues[promptnum].uval.ch.llen = 3; /* disable random run */
#endif
      paramvalues[promptnum++].uval.ch.val  = major_method;

      choices[promptnum] = JIIMstr2;
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.list = JIIMleftright;
      paramvalues[promptnum].uval.ch.vlen = 5;
      paramvalues[promptnum].uval.ch.llen = 2;
      paramvalues[promptnum++].uval.ch.val  = minor_method;
   }

   if (caller				/* <z> command ? */
      && (display3d > 0 || promptnum == 0))
      {
       static char far msg[]={"Current type has no type-specific parameters"};
       stopmsg(20,msg);
       goto gfp_exit;
       }
   if(julibrot)
   sprintf(msg,
	   "Julibrot Parameters (orbit= %s)\n(Press F6 for corner parameters)",
	   juliorbitname);
   else
   sprintf(msg,
	   "Parameters for fractal type %s\n(Press F6 for corner parameters)",
	   typename);

   while (1) 
   {
      oldhelpmode = helpmode;
      helpmode = curfractalspecific->helptext;
      i = fullscreen_prompt(msg,promptnum,choices,paramvalues,0,0x40,tstack);
      helpmode = oldhelpmode;
      if (i < 0) 
      {
         if(julibrot)
           goto gfp_top;
	 if (ret == 0)
	    ret = -1;
	 goto gfp_exit;
      }
      if (i != F6) 
         break;
      if (get_corners() > 0)
	 ret = 1;
     }
     promptnum = 0;
     for ( i = firstparm; i < numparams+firstparm; i++) 
     {
        if (oldparam[i] != paramvalues[promptnum].uval.dval) 
        {
           param[i] = paramvalues[promptnum].uval.dval;
   	   ret = 1;
        }
        ++promptnum;
    }
     for(i=0;i<numextra;i++)
     {
        if(oldparam[i+4] != paramvalues[promptnum].uval.dval)
        {
           param[i+4] = paramvalues[promptnum].uval.dval;
           ret = 1;
        }
        ++promptnum;
     }

   for ( i = 0; i < numtrig; i++) 
   {
      if (paramvalues[promptnum].uval.ch.val != trigndx[i]) 
      {
	 set_trig_array(i,trigfn[paramvalues[promptnum].uval.ch.val].name);
	 ret = 1;
      }
      ++promptnum;
   }

   if(julibrot)
   {
      savespecific = curfractalspecific;
      curfractalspecific = jborbit;
   }   

   if (curfractalspecific->orbit_bailout)
      if (potparam[0] != 0.0 && potparam[2] != 0.0) 
	 promptnum++;
      else 
      {
         bailout = paramvalues[promptnum++].uval.ival;
#ifndef XFRACT
         if (bailout != 0 && (bailout < 4 || bailout > 32000))
#else /* We have big integers, so why not allow big bailouts? */
         if (bailout != 0 && (bailout < 4))
#endif
	    bailout = oldbailout;
         if (bailout != oldbailout)
	    ret = 1;
	 promptnum++;
      }

     if (julibrot)
     {
	mxmaxfp    = paramvalues[promptnum++].uval.dval;
	mymaxfp    = paramvalues[promptnum++].uval.dval;
	mxminfp    = paramvalues[promptnum++].uval.dval;
	myminfp    = paramvalues[promptnum++].uval.dval;
	zdots      = paramvalues[promptnum++].uval.ival;
        juli3Dmode = paramvalues[promptnum++].uval.ch.val;
	eyesfp     = paramvalues[promptnum++].uval.dval;
	originfp   = paramvalues[promptnum++].uval.dval;
	depthfp    = paramvalues[promptnum++].uval.dval;
	heightfp   = paramvalues[promptnum++].uval.dval;
	widthfp    = paramvalues[promptnum++].uval.dval;
	distfp     = paramvalues[promptnum++].uval.dval;
        ret = 1;  /* force new calc since not resumable anyway */
     }
      if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
      {
	 extern int major_method, minor_method;

	 if (paramvalues[promptnum].uval.ch.val != major_method ||
	     paramvalues[promptnum+1].uval.ch.val != minor_method)
	    ret = 1;

	 major_method = paramvalues[promptnum  ].uval.ch.val;
	 minor_method = paramvalues[promptnum+1].uval.ch.val;
      }

gfp_exit:
   curfractalspecific = &fractalspecific[fractype];
   EXIT_OVLY;
   return(ret);
}

int find_extra_param(int type)
{
   int i,ret,curtyp;
   ret = -1;
   i= -1;
   while((curtyp=moreparams[++i].type) != type && curtyp != -1);
   if(curtyp == type)
     ret = i;
   return(ret);
}     


int check_orbit_name(char *orbitname)
{
   int i, numtypes, bad;
   char *nameptr[MAXFRACTALS];
   int fractals[MAXFRACTALS];
   int curtype,numparams,numtrig,last_val;

   ENTER_OVLY(OVLY_PROMPTS1);
   numtypes = build_fractal_list(fractals, &last_val, nameptr);
   bad = 1;
   for(i=0;i<numtypes;i++)
   {
      if(strcmp(orbitname,nameptr[i]) == 0)
      {
         neworbittype = fractals[i];
         bad = 0;
         break;
      }
   }
   EXIT_OVLY;
   return(bad);
}

/* --------------------------------------------------------------------- */

   static FILE *gfe_file;

long get_file_entry(int type,char *title,char *fmask,
			  char *filename,char *entryname)
{
   /* Formula, LSystem, etc type structure, select from file */
   /* containing definitions in the form    name { ... }     */
   int newfile,firsttry;
   long entry_pointer;
   extern char tstack[4096];
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
      setvbuf(gfe_file,tstack,_IOFBF,4096); /* improves speed when file is big */
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
   static char far instr[]={"Press F6 to select different file, F2 for details of highlighted choice"};
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
	    } while (c != '\n' && c != EOF && c != '\032');
	 if (c == EOF || c == '\032') break;
	 continue;
	 }
      name_offset = file_offset;
      len = 0; /* next equiv roughly to fscanf(..,"%40[^ \n\r\t({\032]",buf) */
      while (c != ' ' && c != '\t' && c != '('
	&& c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032') {
	 if (len < 40) buf[len++] = c;
	 c = getc(gfe_file);
	 ++file_offset;
	 }
      buf[len] = 0;
      while (c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032') {
	 c = getc(gfe_file);
	 ++file_offset;
	 }
      if (c == '{') {
	 while (c != '}' && c != EOF && c != '\032') {
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
	 if (c == EOF || c == '\032') break;
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
   i = fullscreen_choice(8,temp1,NULL,instr,numentries,(char **)choices,
                           attributes,boxwidth,boxdepth,colwidth,0,
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
   while ((c = fgetc(entfile)) != EOF && c != '\032') {
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
	    if (linelen == 76) *(buf++) = '\021';
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
   while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032') {
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
   static char far hdg[] = {"3D Parameters"};
   static char far p1[] = {"X-axis rotation in degrees"};
   static char far p2[] = {"Y-axis rotation in degrees"};
   static char far p3[] = {"Z-axis rotation in degrees"};
   static char far p4[] = {"Perspective distance [1 - 999, 0 for no persp]"};
   static char far p5[] = {"X shift with perspective (positive = right)"};
   static char far p6[] = {"Y shift with perspective (positive = up   )"};
   static char far p7[] = {"Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo)"};
   static char far *ifs3d_prompts[8] = {p1, p2, p3, p4, p5, p6, p7};
   struct fullscreenvalues uvalues[20];

   ENTER_OVLY(OVLY_PROMPTS1);
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
   i = fullscreen_prompt(hdg,k,ifs3d_prompts,uvalues,0,0,NULL);
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
/* These macros streamline the "save near space" campaign */ 

#define LOADPROMPTS3D(X)     {\
   static char far tmp[] = { X };\
   prompts3d[++k]= tmp;\
   }


int get_3d_params()	/* prompt for 3D parameters */
{
   static char far hdg[]={"3D Mode Selection"};
   static char far hdg1[]={"Select 3D Fill Type"};
   char *choices[10];
   int attributes[21];
   int sphere;
   char far *s;
   static char far s1[] = {"              Sphere 3D Parameters\n\
Sphere is on its side; North pole to right\n\
Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right"};
   static char s2[]={"              Planar 3D Parameters\n\
Pre-rotation X axis is screen top; Y axis is left side\n\
Pre-rotation Z axis is coming at you out of the screen!"};
   char far *prompts3d[21];
   struct fullscreenvalues uvalues[21];
   int i, k;
   int oldhelpmode;

   ENTER_OVLY(OVLY_PROMPTS1);

#ifdef WINFRACT
     {
     extern int far wintext_textmode;
    
     if (wintext_textmode != 2)  /* are we in textmode? */
         return(0);              /* no - prompts are already handled */
     }
#endif
restart_1:
	if (Targa_Out && overlay3d)
		Targa_Overlay = 1;

   k= -1;

   LOADPROMPTS3D("Preview Mode?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = preview;

   LOADPROMPTS3D("    Show Box?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = showbox;
   
   LOADPROMPTS3D("Coarseness, preview/grid/ray (in y dir)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = previewfactor;

   LOADPROMPTS3D("Spherical Projection?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = sphere = SPHERE;

   LOADPROMPTS3D("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = glassestype;

   LOADPROMPTS3D("Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = RAY;

   LOADPROMPTS3D("                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)");
   uvalues[k].type = '*';

   LOADPROMPTS3D("    Brief output?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = BRIEF;

   check_writefile(ray_name,".ray");
   LOADPROMPTS3D("    Output File Name");
   uvalues[k].type = 's';
   strcpy(uvalues[k].uval.sval,ray_name);

   LOADPROMPTS3D("Targa output?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = Targa_Out;

   oldhelpmode = helpmode;
   helpmode = HELP3DMODE;

   k = fullscreen_prompt(hdg,k+1,prompts3d,uvalues,0,0,NULL);
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
   {
      static char far msg[] = {
"DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n\
the online documentation."};
      if(RAY == 1)
         stopmsg(0,msg);
   }
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
   if (RAY > 7)
      RAY = 7;

   if (!RAY)
   {
      k = 0;
      choices[k++] = "make a surface grid";
      choices[k++] = "just draw the points";
      choices[k++] = "connect the dots (wire frame)";
      choices[k++] = "surface fill (colors interpolated)";
      choices[k++] = "surface fill (colors not interpolated)";
      choices[k++] = "solid fill (bars up from \"ground\")";
      if(SPHERE)
      {
	     choices[k++] = "light source";
      }
      else
      {
	     choices[k++] = "light source before transformation";
	     choices[k++] = "light source after transformation";
      }
      for (i = 0; i < k; ++i)
	 attributes[i] = 1;
      helpmode = HELP3DFILL;
      i = fullscreen_choice(CHOICEHELP,hdg1,NULL,NULL,k,choices,attributes,
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
      k = -1;
      LOADPROMPTS3D("Longitude start (degrees)");
      LOADPROMPTS3D("Longitude stop  (degrees)");
      LOADPROMPTS3D("Latitude start  (degrees)");
      LOADPROMPTS3D("Latitude stop   (degrees)");
      LOADPROMPTS3D("Radius scaling factor in pct");
   }
   else
   {
      k = -1;
      if (!RAY)
      {
	     LOADPROMPTS3D("X-axis rotation in degrees");
         LOADPROMPTS3D("Y-axis rotation in degrees");
	     LOADPROMPTS3D("Z-axis rotation in degrees");
      }
      LOADPROMPTS3D("X-axis scaling factor in pct");
      LOADPROMPTS3D("Y-axis scaling factor in pct");
   }
   k = -1;
   if (!(RAY && !SPHERE))
   {
      uvalues[++k].uval.ival   = XROT	 ;
      uvalues[k].type = 'i';
      uvalues[++k].uval.ival   = YROT	 ;
      uvalues[k].type = 'i';
      uvalues[++k].uval.ival   = ZROT	 ;
      uvalues[k].type = 'i';
   }
   uvalues[++k].uval.ival   = XSCALE    ;
   uvalues[k].type = 'i';

   uvalues[++k].uval.ival   = YSCALE    ;
   uvalues[k].type = 'i';

   LOADPROMPTS3D("Surface Roughness scaling factor in pct");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = ROUGH     ;

   LOADPROMPTS3D("'Water Level' (minimum color value)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = WATERLINE ;

   if(!RAY)
   {
      LOADPROMPTS3D("Perspective distance [1 - 999, 0 for no persp])");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = ZVIEWER	 ;

      LOADPROMPTS3D("X shift with perspective (positive = right)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = XSHIFT    ;
   
      LOADPROMPTS3D("Y shift with perspective (positive = up   )");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = YSHIFT    ;
   
      LOADPROMPTS3D("Image non-perspective X adjust (positive = right)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = xtrans    ;
   
      LOADPROMPTS3D("Image non-perspective Y adjust (positive = up)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = ytrans    ;
   
      LOADPROMPTS3D("First transparent color");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = transparent[0];
   
      LOADPROMPTS3D("Last transparent color");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = transparent[1];
   }

   LOADPROMPTS3D("Randomize Colors      (0 - 7, '0' disables)");
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = RANDOMIZE;

   if (SPHERE)
      s = s1;
   else
      s = s2;

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
   static char far hdg[]={"Light Source Parameters"};
   char far *prompts3d[13];
   struct fullscreenvalues uvalues[13];

   int k;
   int oldhelpmode;

   /* defaults go here */

   k = -1;

   if (ILLUMINE || RAY)
   {
   LOADPROMPTS3D("X value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = XLIGHT    ;

   LOADPROMPTS3D("Y value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = YLIGHT    ;

   LOADPROMPTS3D("Z value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = ZLIGHT    ;

		if (!RAY)
		{
   LOADPROMPTS3D("Light Source Smoothing Factor");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = LIGHTAVG  ;

   LOADPROMPTS3D("Ambient");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = Ambient;
		}
   }

   if (Targa_Out && !RAY)
   {
	LOADPROMPTS3D("Haze Factor        (0 - 100, '0' disables)");
	uvalues[k].type = 'i';
	uvalues[k].uval.ival= haze;

		if (!Targa_Overlay)
	check_writefile(light_name,".tga");
      LOADPROMPTS3D("Targa File Name  (Assume .tga)");
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval,light_name);

      LOADPROMPTS3D("Back Ground Color (0 - 255)");
      uvalues[k].type = '*';

      LOADPROMPTS3D("   Red");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[0];

      LOADPROMPTS3D("   Green");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[1];

      LOADPROMPTS3D("   Blue");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[2];

      LOADPROMPTS3D("Overlay Targa File? (Y/N)");
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = Targa_Overlay;

   }

   LOADPROMPTS3D("");

   oldhelpmode = helpmode;
   helpmode = HELP3DLIGHT;
   k = fullscreen_prompt(hdg,k,prompts3d,uvalues,0,0,NULL);
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
   extern BYTE dacbox[256][3];
   extern BYTE olddacbox[256][3];
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
   static char far hdg[]={"Funny Glasses Parameters"};
   char far *prompts3d[10];

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
      strcpy(funnyglasses_map_name,Glasses1Map);
   else if(glassestype == 2)
   {
      if(FILLTYPE == -1)
	 strcpy(funnyglasses_map_name,"grid.map");
      else
	 strcpy(funnyglasses_map_name,"glasses2.map");
   }

   k = -1;
   LOADPROMPTS3D("Interocular distance (as % of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival= eyeseparation;

   LOADPROMPTS3D("Convergence adjust (positive = spread greater)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = xadjust;

   LOADPROMPTS3D("Left  red image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_left;

   LOADPROMPTS3D("Right red image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_right;

   LOADPROMPTS3D("Left  blue image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_left;

   LOADPROMPTS3D("Right blue image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_right;

   LOADPROMPTS3D("Red brightness factor (%)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_bright;

   LOADPROMPTS3D("Blue brightness factor (%)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_bright;

   if(glassestype == 1 || glassestype == 2)
   {
      LOADPROMPTS3D("Map File name");
      uvalues[k].type = 's';
      strcpy(uvalues[k].uval.sval,funnyglasses_map_name);
   }

   oldhelpmode = helpmode;
   helpmode = HELP3DGLASSES;
   k = fullscreen_prompt(hdg,k+1,prompts3d,uvalues,0,0,NULL);
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
