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

/* Routines defined in prompts1.c */

extern	int prompt_checkkey(int curkey);
extern	long get_file_entry(int,char *,char *,char *,char *);

/* Routines used in prompts1.c */

	int get_corners(void);
	int edit_ifs_params(void );
	int lccompare(VOIDCONSTPTR, VOIDCONSTPTR); /* Needed in prompts1.c PAV */

/* Routines in this module	*/

static	int findfirst(char *path);
static  int check_f6_key(int curkey,int choice);
static	int findnext(void );
	int splitpath(char *template,char *drive,char *dir,char *fname,char *ext);
        int makepath(char *template,char *drive,char *dir,char *fname,char *ext);
static	void fix_dirname(char *dirname);
static	int expand_dirname(char *dirname,char *drive);
static	int filename_speedstr(int, int, int, char *, int);
static	int isadirectory(char *s);
static  int check_f6_key(int curkey,int choice);

extern int dotmode;
extern int orbit_delay;
extern char diskfilename[];

extern char *fract_dir1, *fract_dir2;

#ifndef XFRACT
extern int strncasecmp(char *s,char *t,int ct);
#endif

extern char temp1[256];   /* temporary strings	      */

extern	double	xxmin,xxmax;	/* initial corner values    */
extern	double	yymin,yymax;	/* initial corner values    */
extern	BYTE usemag;

extern	int AntiAliasing;
extern double zzmin, zzmax, ttmin, ttmax;
extern int Transparent3D;

extern	double	xx3rd,yy3rd;	/* initial corner values    */
extern	int	invert; 	/* non-zero if inversion active */
extern	double	inversion[3];	/* radius, xcenter, ycenter */
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
extern	int	decomp[];	/* decomposition parameters */
extern	int	usr_distest;	/* distance estimator option */
extern	int	distestwidth;
extern	char	usr_stdcalcmode; /* '1', '2', 'g', 'b' */
extern	char    overwrite; 	/* overwrite= flag */
extern	int	soundflag;	/* sound option */
extern	int	LogFlag;	/* non-zero if logarithmic palettes */
extern	int	usr_biomorph;	/* Biomorph flag */
#if 0
//extern	long	xmin, xmax, ymin, ymax; /* screen corner values */
#endif
extern	int	xdots, ydots;	/* coordinates of dots on the screen  */
extern	int	colors; 	/* maximum colors available */
extern	int	row, col;
extern	int	viewwindow;
extern	float	viewreduction;
extern	int	viewcrop;
extern	float	finalaspectratio;
extern	int	viewxdots,viewydots;
extern	int	textcbase;
extern	int	textrow,textcol;
extern	int	resave_flag;	/* resaving after a timed save */
extern	int	started_resaves;
extern	char	boxy[];
extern	int	rotate_lo,rotate_hi;
extern	int	rangeslen;
extern float  screenaspect;

extern  int  cmdarg(char *,int);

extern char CommandFile[];
extern char CommandName[];
extern float far *ifs_defn;
extern int ifs_type;
extern int ifs_changed;
extern int initbatch;		/* 1 if batch run (no kbd)  */

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
extern char s_period[];

char commandmask[13] = {"*.par"};

void prompts2_overlay() { }	/* for restore_active_ovly */

#if 0
/* --------------------------------------------------------------------- */

extern int promptfkeys;

int edit_ifs_params()	/* prompt for IFS params */
{
   int totcols;
   int i, j, k, numlines, ret;
   FILE *tempfile;
   char msg[81];
   char filename[81];
   float ftemp;
   int oldhelpmode;
   int low, hi;

   if (!ifs_defn && !ifsload())
      return(-1);

   totcols = (ifs_type == 0) ? IFSPARM : IFS3DPARM;
   ret = 0;
   oldhelpmode = helpmode;
   helpmode = HT_IFS;

   low = 0;

   for ( ;; ) {
static char far ifshdg2[]={"2D IFS Parameters"};
static char far ifshdg3[]={"3D IFS Parameters"};
static char far ifsparmmsg1[]={"#    a     b     c     d     e     f"};
static char far ifsparmmsg2[]={"     g     h     i     j     k     l"};
static char far ifsprompt[]={"\
Enter the number of the line you want to edit,\n\
S to save, F6 for corners, or ENTER to end ==>"};
      int leftcol,promptrow,promptcol;
#define IFS_NUM 12

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

      hi = low+IFS_NUM;
      if (hi>numlines) hi = numlines;
      for (i = low; i < hi; i++) {
	 sprintf(msg,"%2d", i+1);
	 putstring(5+i-low,leftcol,C_GENERAL_HI,msg);
	 for (j = 0; j < totcols; j++) {
	    sprintf(msg,"%6.2f",ifs_defn[(i*totcols)+j]);
	    putstring(-1,-1,C_GENERAL_MED,msg);
	    }
	 }
      if (hi<numlines) {
	 putstring(5+IFS_NUM,leftcol,C_GENERAL_HI,"(more)");
      }

      textcbase = 14;
      putstring(5+i-low+1,0,C_GENERAL_HI,ifsprompt);
      promptrow = textrow;
      promptcol = textcol + textcbase + 1;
      temp1[0] = textcbase = 0;
      promptfkeys = 1<<6;
      i = input_field(0,C_GENERAL_INPUT,temp1,2,promptrow,promptcol,
	      prompt_checkkey);
      if (i<0) {
	  break;
      } else if (i==PAGE_UP) {
	  low -= IFS_NUM;
	  if (low<0) low=0;
      } else if (i==UP_ARROW) {
	  low -= 1;
	  if (low<0) low=0;
      } else if (i==DOWN_ARROW) {
	  low += 1;
	  if (low+IFS_NUM>numlines) low=numlines-IFS_NUM;
	  if (low<0) low=0;
      } else if (i==PAGE_DOWN) {
	  low += IFS_NUM;
	  if (low+IFS_NUM>numlines) low=numlines-IFS_NUM;
	  if (low<0) low=0;
      } else if (i==F6) {
	  if (get_corners()) {
	      ret = 1;
	  }
      } else if (i==0) {
	  if (temp1[0]==0) break;
      } else {
	  continue;
      }

      putstring(promptrow,promptcol,C_GENERAL_HI,temp1);
      if (temp1[0] == 's' || temp1[0] == 'S') {
	 stackscreen();
	 filename[0] = 0;
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
      if (i >= 0 && i < numlines) {
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
#endif
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
#define LOADCHOICES(X)     {\
   static char far tmp[] = { X };\
   choices[++k]= tmp;\
   }
int get_toggles()
{
   static char far hdg[]={"        Basic Options\n\
(not all combinations make sense)"};
   char far *choices[20];
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

   ENTER_OVLY(OVLY_PROMPTS2);

   k = -1;

   LOADCHOICES("Passes (1, 2, g[uessing], b[oundary trace], t[esseral])");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 3;
   uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
   uvalues[k].uval.ch.list = calcmodes;
   uvalues[k].uval.ch.val = (usr_stdcalcmode == '1') ? 0
			  : (usr_stdcalcmode == '2') ? 1
                          : (usr_stdcalcmode == 'g') ? 2
                          : (usr_stdcalcmode == 'b') ? 3 :4 ;
   old_usr_stdcalcmode = usr_stdcalcmode;

   LOADCHOICES("Floating Point Algorithm");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = usr_floatflag;

   LOADCHOICES("Maximum Iterations (2 to 32767)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_maxit = maxit;

   LOADCHOICES("Inside Color (<nnn>,maxiter,zmag,bof60,bof61,epscr,star,per)");
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
   else if(inside == -102)
      strcpy(uvalues[k].uval.sval,s_period);
   else if(inside == -1)
      strcpy(uvalues[k].uval.sval,s_maxiter);
   else
      sprintf(uvalues[k].uval.sval,"%d",inside);
   old_inside = inside;

   LOADCHOICES("Outside Color (<nnn>,iter,real,imag,mult,summ)");
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

   LOADCHOICES("Savename (.GIF implied)");
   uvalues[k].type = 's';
   strcpy(prevsavename,savename);
   strcpy(uvalues[k].uval.sval,savename);

   LOADCHOICES("File Overwrite ('overwrite=')");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = overwrite;

   LOADCHOICES("Sound (no, yes, x, y, z)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 3;
   uvalues[k].uval.ch.llen = 5;
   uvalues[k].uval.ch.list = soundmodes;
   uvalues[k].uval.ch.val = 1 + (old_soundflag = soundflag);

   if (rangeslen == 0) {
      LOADCHOICES("Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt)");
      uvalues[k].type = 'i';
      }
   else {
      LOADCHOICES("Log Palette (n/a, ranges= parameter is in effect)");
      uvalues[k].type = '*';
      }
   uvalues[k].uval.ival = old_logflag = LogFlag;

   LOADCHOICES("Biomorph Color (-1 means OFF)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_biomorph = usr_biomorph;

   LOADCHOICES("Decomp Option (2,4,8,..,256, 0=OFF)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_decomp = decomp[0];

   LOADCHOICES("Fill Color (normal,<nnn>) (works with passes=t and =b)");
   uvalues[k].type = 's';
   if(fillcolor < 0)
      strcpy(uvalues[k].uval.sval,s_normal);
   else
      sprintf(uvalues[k].uval.sval,"%d",fillcolor);
   old_fillcolor = fillcolor;
 
   LOADCHOICES("Orbit delay (0 = none)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = orbit_delay;

/*
   LOADCHOICES("Antialiasing (0 to 8)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = AntiAliasing;
*/

   oldhelpmode = helpmode;
   helpmode = HELPXOPTS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,0,NULL);
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
   else if(strncmp(strlwr(uvalues[k].uval.sval),s_period,3)==0)
      inside = -102;
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
   static char far hdg[]={"       Extended Options\n\
(not all combinations make sense)"};
   char far *choices[20];
   int oldhelpmode;

   struct fullscreenvalues uvalues[25];
   int i, j, k;

   int old_rotate_lo,old_rotate_hi;
   int old_usr_distest,old_distestwidth;
   double old_potparam[3],old_inversion[3];

   ENTER_OVLY(OVLY_PROMPTS2);

   /* fill up the choices (and previous values) arrays */
   k = -1;

   LOADCHOICES("Look for finite attractor (0=no,>0=yes,<0=phase)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ch.val = finattract;

   LOADCHOICES("Potential Max Color (0 means off)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_potparam[0] = potparam[0];

   LOADCHOICES("          Slope");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = old_potparam[1] = potparam[1];

   LOADCHOICES("          Bailout");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_potparam[2] = potparam[2];

   LOADCHOICES("          16 bit values");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = pot16bit;

   LOADCHOICES("Distance Estimator (0=off, <0=edge, >0=on):");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_usr_distest = usr_distest;

   LOADCHOICES("          width factor:");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_distestwidth = distestwidth;



   LOADCHOICES("Inversion radius or \"auto\" (0 means off)");
   LOADCHOICES("          center X coordinate or \"auto\"");
   LOADCHOICES("          center Y coordinate or \"auto\"");
   k = k - 3;
   for (i= 0; i < 3; i++) {
      uvalues[++k].type = 's';
      if ((old_inversion[i] = inversion[i]) == AUTOINVERT)
	 sprintf(uvalues[k].uval.sval,"auto");
      else
	 sprintf(uvalues[k].uval.sval,"%g",inversion[i]);
      }
   LOADCHOICES("  (use fixed radius & center when zooming)");
   uvalues[k].type = '*';

   LOADCHOICES("Color cycling from color (0 ... 254)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_lo = rotate_lo;

   LOADCHOICES("              to   color (1 ... 255)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_hi = rotate_hi;

   oldhelpmode = helpmode;
   helpmode = HELPYOPTS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,0,NULL);
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

   potparam[1] = uvalues[++k].uval.dval;
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
   static char far hdg[]={"View Window Options"};
   char far *choices[8];

   int oldhelpmode;
   struct fullscreenvalues uvalues[25];
   int i, k;
   float old_viewreduction,old_aspectratio;
   int old_viewwindow,old_viewcrop,old_viewxdots,old_viewydots;

   ENTER_OVLY(OVLY_PROMPTS2);
   stackscreen();

   old_viewwindow    = viewwindow;
   old_viewcrop      = viewcrop;
   old_viewreduction = viewreduction;
   old_aspectratio   = finalaspectratio;
   old_viewxdots     = viewxdots;
   old_viewydots     = viewydots;

get_view_restart:
   /* fill up the previous values arrays */
   k = -1;

   LOADCHOICES("Preview display? (no for full screen)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = viewwindow;

   LOADCHOICES("Auto window size reduction factor");
   uvalues[k].type = 'f';
   uvalues[k].uval.dval = viewreduction;

   LOADCHOICES("Final media overall aspect ratio, y/x");
   uvalues[k].type = 'f';
   uvalues[k].uval.dval = finalaspectratio;

   LOADCHOICES("Crop starting coordinates to new aspect ratio?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = viewcrop;

   LOADCHOICES("Explicit size x pixels (0 for auto size)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = viewxdots;

   LOADCHOICES("              y pixels (0 to base on aspect ratio)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = viewydots;

   LOADCHOICES("");
   uvalues[k].type = '*';

   LOADCHOICES("Press F4 to reset view parameters to defaults.");
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPVIEW;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,16,NULL);
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
      finalaspectratio = screenaspect;
      goto get_view_restart;
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;

   viewwindow = uvalues[++k].uval.ch.val;

   viewreduction = uvalues[++k].uval.dval;

   if ((finalaspectratio = uvalues[++k].uval.dval) == 0)
      finalaspectratio = screenaspect;

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

/*
    get_cmd_string() is called from FRACTINT.C whenever the 'g' key
    is pressed.  Return codes are:
	-1  routine was ESCAPEd - no need to re-generate the image.
	 0  parameter changed, no need to regenerate
	>0  parameter changed, regenerate
*/

int get_cmd_string()
{
   int oldhelpmode;
   int i;
   char cmdbuf[61];

   ENTER_OVLY(OVLY_PROMPTS2);

   oldhelpmode = helpmode;
   helpmode = HELPCOMMANDS;
   cmdbuf[0] = 0;
   i = field_prompt(0,"Enter command string to use.",NULL,cmdbuf,60,NULL);
   helpmode = oldhelpmode;
   if (i >= 0 && cmdbuf[0] != 0)
       i = cmdarg(cmdbuf, 2);

   EXIT_OVLY;
   return(i);
}


/* --------------------------------------------------------------------- */

int Distribution = 30, Offset = 0, Slope = 25;
long con;

static char far sf1[] = {"Star Density in Pixels per Star"};
static char far sf2[] = {"Percent Clumpiness"};
static char far sf3[] = {"Ratio of Dim stars to Bright"};
static char far *starfield_prompts[] = {sf1,sf2,sf3};

static double starfield_values[4] = {
	30.0,100.0,5.0,0.0
	};

char GreyFile[] = "altern.map";

int starfield(void)
{
   int c;
   extern char busy;
   busy = 1;
   if (starfield_values[0] <   1.0) starfield_values[0] =   1.0;
   if (starfield_values[0] > 100.0) starfield_values[0] = 100.0;
   if (starfield_values[1] <   1.0) starfield_values[1] =   1.0;
   if (starfield_values[1] > 100.0) starfield_values[1] = 100.0;
   if (starfield_values[2] <   1.0) starfield_values[2] =   1.0;
   if (starfield_values[2] > 100.0) starfield_values[2] = 100.0;

   Distribution = (int)(starfield_values[0]);
   con	= (long)(((starfield_values[1]) / 100.0) * (1L << 16));
   Slope = (int)(starfield_values[2]);

   if (ValidateLuts(GreyFile) != 0) {
      static char far msg[]={"Unable to load ALTERN.MAP"};
      stopmsg(0,msg);
      busy = 0;
      return(-1);
      }
   spindac(0,1);		 /* load it, but don't spin */
   for(row = 0; row < ydots; row++) {
      for(col = 0; col < xdots; col++) {
	 if(keypressed()) {
	    buzzer(1);
	    busy = 0;
	    return(1);
	    }
	 c = getcolor(col, row);
         if(c == inside)
            c = colors-1;
	 putcolor(col, row, GausianNumber(c, colors));
      }
   }
   buzzer(0);
   busy = 0;
   return(0);
}

int get_starfield_params(void) {
   static char far hdg[]={"Starfield Parameters"};
   struct fullscreenvalues uvalues[3];
   int oldhelpmode, status;
   int i;

   ENTER_OVLY(OVLY_PROMPTS2);

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
   i = fullscreen_prompt(hdg,3,starfield_prompts,uvalues,0,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      unstackscreen();
      EXIT_OVLY;
      return(-1);
      }
   unstackscreen();

   for (i = 0; i < 3; i++)
      starfield_values[i] = uvalues[i].uval.dval;

   EXIT_OVLY;
   return(0);
}

int get_a_number(double *x, double *y)
{
   static char far hdg[]={"Set Cursor Coordinates"};
   double x1,y2;
   char far *choices[2];

   int oldhelpmode;
   struct fullscreenvalues uvalues[2];
   int i, k;

   ENTER_OVLY(OVLY_PROMPTS2);
   stackscreen();

   /* fill up the previous values arrays */
   k = -1;

   LOADCHOICES("X coordinate at cursor");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = *x;

   LOADCHOICES("Y coordinate at cursor");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = *y;

   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,25,NULL);
   if (i < 0) {
      unstackscreen();
      EXIT_OVLY;
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;

   *x = uvalues[++k].uval.dval;
   *y = uvalues[++k].uval.dval;

   unstackscreen();
   EXIT_OVLY;
   return(i);
}

/* --------------------------------------------------------------------- */

int get_commands()		/* execute commands from file */
{
   int ret;
   FILE *parmfile;
   long point;
   int oldhelpmode;
   ENTER_OVLY(OVLY_PROMPTS2);
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
   static char far goodbyemessage[]={"   Thank You for using FRACTINT"};
   extern BYTE exitmode;
   extern int mode7text;
   extern int made_dsktemp;
#ifndef XFRACT
   union REGS r;
#endif

#ifdef WINFRACT
   return;
#endif

   setvideotext();
#ifdef XFRACT
   UnixDone();
   printf("\n\n\n%s\n",goodbyemessage); /* printf takes far pointer */
#else
   r.h.al = (mode7text == 0) ? exitmode : 7;
   r.h.ah = 0;
   int86(0x10, &r, &r);
   printf("\n\n\n%Fs\n",goodbyemessage); /* printf takes far pointer */
#endif
   movecursor(6,0);
   discardgraphics(); /* if any emm/xmm tied up there, release it */
   stopslideshow();
#ifndef XFRACT
   if (made_dsktemp)
      remove(diskfilename);
#endif
   end_help();
   if (initbatch == 3) /* exit with error code for batch file */
     exit(2);
   else if (initbatch == 4)
     exit(1);
   else
     exit(0);
}


/* --------------------------------------------------------------------- */

#ifdef XFRACT
static char searchdir[FILE_MAX_DIR];
static char searchname[FILE_MAX_PATH];
static char searchext[FILE_MAX_EXT];
static DIR *currdir = NULL;
#endif
static int  findfirst(char *path)       /* Find 1st file (or subdir) meeting path/filespec */
{
#ifndef XFRACT
     union REGS regs;
     regs.h.ah = 0x1A;		   /* Set DTA to filedata */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     regs.h.ah = 0x4E;		   /* Find 1st file meeting path */
     regs.x.dx = (unsigned)path;
     regs.x.cx = FILEATTR;
     intdos(&regs, &regs);
     return(regs.x.ax); 	   /* Return error code */
#else
     if (currdir != NULL) {
         closedir(currdir);
         currdir = NULL;
     }
     splitpath(path,NULL,searchdir,searchname,searchext);
     if (searchdir[0]=='\0') {
         currdir = opendir(".");
     } else {
         currdir = opendir(searchdir);
     }
     if (currdir==NULL) {
         return -1;
     } else {
         return findnext();
     }
#endif
}

static int  findnext()		/* Find next file (or subdir) meeting above path/filespec */
{
#ifndef XFRACT
     union REGS regs;
     regs.h.ah = 0x4F;		   /* Find next file meeting path */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     return(regs.x.ax);
#else
#ifdef DIRENT
     struct dirent *dirEntry;
#else
     struct direct *dirEntry;
#endif
     struct stat sbuf;
     char thisname[FILE_MAX_PATH];
     char tmpname[FILE_MAX_PATH];
     char thisext[FILE_MAX_EXT];
     while (1) {
         dirEntry = readdir(currdir);
         if (dirEntry == NULL) {
             closedir(currdir);
             currdir = NULL;
             return -1;
         } else if (dirEntry->d_ino != 0) {
             splitpath(dirEntry->d_name,NULL,NULL,thisname,thisext);
             if ((searchname[0]=='*' || strcmp(searchname,thisname)==0) &&
                     (searchext[0]=='*' || strcmp(searchext,thisext)==0)) {
                 strncpy(DTA.filename,dirEntry->d_name,20);
                 DTA.filename[20]=='\0';
                 strcpy(tmpname,searchdir);
                 strcat(tmpname,"/");
                 strcat(tmpname,dirEntry->d_name);
                 stat(tmpname,&sbuf);
                 if ((sbuf.st_mode&S_IFMT)==S_IFREG) {
                     DTA.attribute = 0;
                 } else if ((sbuf.st_mode&S_IFMT)==S_IFDIR) {
                     DTA.attribute = SUBDIR;
                 } else {
                     continue;
                 }
                 DTA.size = sbuf.st_size;
                 return 0;
             }
         }
     }
#endif
}

int lccompare(VOIDCONSTPTR arg1, VOIDCONSTPTR arg2) /* for qsort */
{
   return(strncasecmp(*((char **)arg1),*((char **)arg2),40));
}

static char *masks[] = {"*.pot","*.gif"};
static int speedstate;

int getafilename(char *hdg,char *template,char *flname)
{
   static char far instr[]={"Press F6 for default or environment directory"};
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

   ENTER_OVLY(OVLY_PROMPTS2);

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
      strcpy(flname,DOTSLASH);
   splitpath(flname ,drive,dir,fname,ext);
   makepath(filename,""   ,"" ,fname,ext);
   retried = 0;
retry_dir:
   if (dir[0] == 0)
      strcpy(dir,".");
   expand_dirname(dir,drive);
   makepath(tmpmask,drive,dir,"","");
   fix_dirname(tmpmask);
   if (retried == 0 && strcmp(dir,SLASH) && strcmp(dir,DOTSLASH))
   {
      tmpmask[(j = strlen(tmpmask) - 1)] = 0; /* strip trailing \ */
      if (strchr(tmpmask,'*') || strchr(tmpmask,'?')
	|| findfirst(tmpmask) != 0
	|| (DTA.attribute & SUBDIR) == 0)
      {
         strcpy(dir,DOTSLASH);
	 ++retried;
	 goto retry_dir;
      }
      tmpmask[j] = SLASHC;
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
#ifndef XFRACT
	 strlwr(DTA.filename);
#endif
	 if(strcmp(DTA.filename,".."))
            strcat(DTA.filename,SLASH);
	 strncpy(choices[++filecount]->name,DTA.filename,13);
	 choices[filecount]->name[12] = '\0';
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
   if(notroot == 0 && dir[0] && dir[0] != SLASHC) /* must be in root directory */
   {
      splitpath(tmpmask,drive,dir,fname,ext);
      strcpy(dir,SLASH);
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
   i = fullscreen_choice(8,temp1,NULL,instr,filecount,(char **)choices,
          attributes,5,99,12,i,NULL,speedstr,filename_speedstr,check_f6_key);
   if (i==-F6)
   {
      static int lastdir=0;
      if (lastdir==0)
      {
	 strcpy(dir,fract_dir1);
      }
      else
      {
	 strcpy(dir,fract_dir2);
      }
      fix_dirname(dir);
       makepath(flname,drive,dir,"","");
       lastdir = 1-lastdir;
       goto restart;
   }
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
	    if(strcmp(dir,DOTSLASH) == 0)
	       strcpy(dir,DOTDOTSLASH);
	    else
	    {
	       char *s;
	       if(s = strrchr(dir,SLASHC)) /* trailing slash */
	       {
		  *s = 0;
		  if(s = strrchr(dir,SLASHC))
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
	    if (strchr(speedstr,SLASHC) == NULL)
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

static int check_f6_key(int curkey,int choice)
{
   if (curkey == F6)
      return 0-F6;
   return 0;
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
      if(strchr(s,SLASHC)) /* we'll guess it is a directory */
	 return(1);
      else
	 return(0);  /* no slashes - we'll guess it's a file */
   }
   else if(DTA.attribute & SUBDIR)
      return(1);   /* we're SURE it's a directory */
   else
      return(0);
}


#ifndef XFRACT	/* This routine moved to unix.c so we can use it in hc.c */
int splitpath(char *template,char *drive,char *dir,char *fname,char *ext)
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
      tmp = strrchr(template,SLASHC);
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
      if(tmp < strrchr(template,SLASHC) || tmp < strrchr(template,':'))
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
#endif

makepath(char *template,char *drive,char *dir,char *fname,char *ext)
{
#ifndef XFRACT
   strcpy(template,drive);
   strcat(template,dir);
#else
   strcpy(template,dir);
#endif
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
   if(length == 0 || dirname[length-1] != SLASHC)
      strcat(dirname,SLASH);
}

static int expand_dirname(char *dirname,char *drive)
{
   fix_dirname(dirname);
   if (dirname[0] != SLASHC) {
      char buf[81],curdir[81];
#ifndef XFRACT
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
#else
      getwd(curdir);
#endif
      strcat(curdir,SLASH);
      while (strncmp(dirname,DOTSLASH,2) == 0) {
	 strcpy(buf,&dirname[2]);
	 strcpy(dirname,buf);
	 }
      while (strncmp(dirname,DOTDOTSLASH,3) == 0) {
	 char *s;
	 curdir[strlen(curdir)-1] = 0; /* strip trailing slash */
	 if (s = strrchr(curdir,SLASHC))
	    *s = 0;
	 strcat(curdir,SLASH);
	 strcpy(buf,&dirname[3]);
	 strcpy(dirname,buf);
	 }
      strcpy(buf,dirname);
      dirname[0] = 0;
      if (curdir[0] != SLASHC)
	 strcpy(dirname,SLASH);
      strcat(dirname,curdir);
      strcat(dirname,buf);
      }
   return(0);
}

#define LOADPROMPTS(X)     {\
   static char far tmp[] = { X };\
   prompts[++nump]= tmp;\
   }

int get_corners()
{
   struct fullscreenvalues values[15];
   char far *prompts[15];
   static char far xprompt[]={"          X"};
   static char far yprompt[]={"          Y"};
   static char far zprompt[]={"          Z"};
   int i,nump,prompt_ret;
   int cmag,transp3d;
   double Xctr,Yctr,Mag;
   BYTE ousemag;
   double oxxmin,oxxmax,oyymin,oyymax,oxx3rd,oyy3rd;
   double ozzmin,ozzmax,ottmin,ottmax;
   /* note that hdg[15] is used for non-transparent heading: */
   static char far hdg[]={"Transparent 3d Image Coordinates"};
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

   nump = -1;
   if (cmag) {
      LOADPROMPTS("Center X");
      values[nump].uval.dval = Xctr;
      LOADPROMPTS("Center Y");
      values[nump].uval.dval = Yctr;
      LOADPROMPTS("Magnification");
      values[nump].uval.dval = Mag;
      LOADPROMPTS("");
      values[nump].type = '*';
      LOADPROMPTS("Press F7 to switch to \"corners\" mode");
      values[nump].type = '*';
      }

   else {
      LOADPROMPTS("Top-Left Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = xxmin;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = yymax;
      if (transp3d) {
    	 prompts[++nump] = zprompt;
	     values[nump].uval.dval = zzmin;
	  }
      LOADPROMPTS("Bottom-Right Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = xxmax;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = yymin;
      if (transp3d) {
	 prompts[++nump] = zprompt;
	 values[nump].uval.dval = zzmax;
	 }
      if (transp3d) {
	 LOADPROMPTS("Time Step");
	 values[nump].type = '*';
	 LOADPROMPTS("          From");
	 values[nump].uval.dval = ttmin;
	 LOADPROMPTS("          To");
	 values[nump].uval.dval = ttmax;
	 }
      else {
	 if (xxmin == xx3rd && yymin == yy3rd)
	    xx3rd = yy3rd = 0;
	 LOADPROMPTS("Bottom-left (zeros for top-left X, bottom-right Y)");
	 values[nump].type = '*';
	 prompts[++nump] = xprompt;
	 values[nump].uval.dval = xx3rd;
	 prompts[++nump] = yprompt;
	 values[nump].uval.dval = yy3rd;
	 LOADPROMPTS("Press F7 to switch to \"center-mag\" mode");
	 values[nump].type = '*';
	 }
      }

   LOADPROMPTS("Press F4 to reset to type default values");
   values[nump].type = '*';

   oldhelpmode = helpmode;
   helpmode = HELPCOORDS;
   prompt_ret = fullscreen_prompt((transp3d) ? hdg : &hdg[15],
		     nump+1, prompts, values, 0,
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
      if (viewcrop && finalaspectratio != screenaspect)
	 aspectratio_crop(screenaspect,finalaspectratio);
      goto gc_loop;
      }

   if (cmag) {
      if ( values[0].uval.dval != Xctr
	|| values[1].uval.dval != Yctr
	|| values[2].uval.dval != Mag) {
	 double radius,width;
	 radius = 1.0 / values[2].uval.dval;
	 width = radius * (1.0 / screenaspect);
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
