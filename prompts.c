/*
	Various routines that prompt for things.
	(Also, odds and ends that don't git anywhere else)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <time.h>

#include "fractint.h"
#include "fractype.h"

extern double rqlim;
extern char *strig[];
extern char far inversionmessage[];		/* located in FARMSG.ASM */
extern int bailout;

/* common strings - why keep several copies around? */
char string001[] = {"  Use the cursor keys to select values to change"};
char string002[] = {"  Type in any replacement values you wish to use"};
char string003[] = {"Press the ENTER when finished (or ESCAPE to back out)"};
char string004[] = {"...Press any key to continue.."};

struct fullscreenvalues
{
   char type;	/* 'd' or 's' */
   union
   {
      double dval;
      char   sval[16];
   } uval;
};
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
extern		findpath();
FILE	*dacfile;
char	MAP_name[80];
extern		ValidateLuts();  /* read the palette file */
int	mapset = 0;
extern	int	overlay3d;	    /* 3D overlay flag: 0 = OFF */
extern	int	lookatmouse;
extern	int full_color;
extern	int haze;
extern	int RANDOMIZE;
extern	char light_name[];
extern	int Ambient;

extern	int	initmode;	/* initial video mode	    */
extern	int	initfractype;	/* initial type set flag    */
extern	int	initbatch;	/* 1 if batch run (no kbd)  */
extern	int	init3d[20];	/* '3d=nn/nn/nn/...' values */
extern	int	initcorners;		/* initial flag: corners set*/
extern	double	initxmin,initxmax;	/* initial corner values    */
extern	double	initymin,initymax;	/* initial corner values    */
extern	double	initx3rd,inity3rd;	/* initial corner values    */
extern	double	initparam[4];		/* initial parameters	    */
extern	int	display3d;	/* 3D display flag: 0 = OFF */
extern	int	invert; 	/* non-zero if inversion active */
extern	double	inversion[3];	/* radius, xcenter, ycenter */
extern	long	fudge;		/* 2**fudgefactor	*/
extern	int	bitshift;	/* fudgefactor		*/
extern	double	param[4];	/* up to four parameters    */
extern	char	potfile[];	/* potential filename */
extern	double	potparam[3];	/* three potential parameters*/
extern	int	fractype;	/* if == 0, use Mandelbrot  */
extern	char	floatflag;	/* floating-point fractals? */
extern	int	maxit;		/* try this many iterations */
extern	int	inside; 	/* inside color */
extern	int	outside;	/* outside color */
extern	int	finattract;	/* finite attractor switch */
extern	char	savename[80];	/* save files using this name */
extern	char	ifsfilename[80];    /* IFS code file */
extern	char	ifs3dfilename[80];  /* IFS 3D code file */
extern	char	preview;	/* 3D preview mode flag */
extern	int	decomp[];	/* decomposition parameters */
extern	int	distest;	/* distance estimator option */
extern	int	rflag, rseed;	/* Plasma-Cloud seed values */
extern	int	fractype;	/* current fractal type */
extern	int	transparent[];	/* transparency values */
extern	int	numpasses;	/* 0 if single-pass, else 1 */
extern	int	solidguessing;	/* 0 if disabled, else 1    */
extern	int  boundarytraceflag; /* boundary trace option */
extern	int	warn;		/* warn=yes flag */
extern	int	soundflag;	/* sound option */
extern	int	LogFlag;	/* non-zero if logarithmic palettes */
extern	int	biomorph;	/* Biomorph flag */
extern	unsigned char dacbox[256][3];	 /* Video-DAC (filled in by SETVIDEO) */
extern	unsigned char olddacbox[256][3]; /* backup copy of the Video-DAC */
extern	long	xmin, xmax, ymin, ymax; /* screen corner values */
extern	int	calc_status;	/* calc status: complete, resumable, ... */
extern	int	xdots, ydots;	/* coordinates of dots on the screen  */
extern	int	colors; 	/* maximum colors available */
extern	int	row, col;

/* Define command keys */

#define   PAGE_UP	 1073
#define   PAGE_DOWN	 1081
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

/* --------------------------------------------------------------------- */

unsigned video_seg;		   /* Current video display segment */
int crtrows, crtcols;		   /* Lines per page, columns/row */
int isatextmode;		   /* 1 if a text mode, 0 if graphics */

fullscreen_setup()		/* set up for full-screen prompting */
{
   int	video_mode;		 /* current video mode */
   unsigned char far *lowptr;	 /* Low memory pointer */

   /* setup stuff for video */
#ifdef __TURBOC__
   lowptr = MK_FP(0, 0x449);		/* Set for low memory */
#else
   FP_SEG(lowptr) = 0;			   /* Set for low memory */
   FP_OFF(lowptr) = 0x449;		   /* Get video mode */
#endif
   isatextmode = 1;			   /* assume text */
   if ((video_mode = *lowptr) >= 0 && video_mode < 4)
      video_seg = 0xB800; /* Define video segment */
   else if (video_mode == 7)		   /* If monochrome */
      video_seg = 0xB000;
   else {				   /* graphics mode */
      video_seg = 0xB800;		   /* might be wrong, but not used */
      isatextmode = 0;
      }

#ifdef __TURBOC__
   lowptr = MK_FP(0, 0x484);		   /* get screen rows -1 */
#else
   FP_SEG(lowptr) = 0;
   FP_OFF(lowptr) = 0x484;
#endif
   crtrows = (int)(*lowptr);

#ifdef __TURBOC__
   lowptr = MK_FP(0, 0x44a);		   /* get cols/row */
#else
   FP_SEG(lowptr) = 0;
   FP_OFF(lowptr) = 0x44a;
#endif
   crtcols = (int)(*lowptr);

   if (video_mode == 7)
   {
      crtrows = 24; /* Since some mono adapters don't update lower RAM */
      crtcols = 80;
   }
   /* end setup stuff for video */
}

/* --------------------------------------------------------------------- */

fullscreen_prompt(		/* full-screen prompting routine */
	int startrow,		/* start on this row */
	int numprompts, 	/* there are this many prompts (max) */
	char * far *prompts,	/* array of prompting pointers */
	double values[] 	/* array of (initial) values */
	)
{
char temp2[24][15];		/* string value of answers go here */
int row, col, c, done, i, j, k;

if (numprompts <= 0)		/* ?? nothing to do! */
	return(0);

fullscreen_setup();		/* set up for full-screen prompting */

movecursor(25,81);		/* kill cursor */

putstring(22,10,7,string001,0);
putstring(23,10,7,string002,0);
putstring(24,10,7,string003,0);

col = 0;
for (i = 0; i < numprompts; i++) {	/* calculate initial display */
	if (col < strlen(prompts[i]))
		col = strlen(prompts[i]);
	}

col+= 20;	/* answers start in this column */
if (col > 65) col = 65;

for (i = 0; i < numprompts; i++) {	/* generate initial display */
	sprintf(temp2[i], "%g", values[i]);
	temp2[i][14] = 0;		/* safety first! */
	putstring(startrow+i, 5, 7, prompts[i], 0);
	putstring(startrow+i, col, 7, temp2[i], 0);
	}

i = 0;
k = 0;
done = 0;
while (!done) {
	putstring(startrow+i, col-1, 112, "\032              \033", 0);
	putstring(startrow+i, col, 112, temp2[i], 0);
	while (! keypressed());
	c = getakey();
	putstring(startrow+i, col-1, 7,   "                ", 0);
	putstring(startrow+i, col, 7,	temp2[i], 0);

	switch(c) {
	case DOWN_ARROW:
	case DOWN_ARROW_2:
	case RIGHT_ARROW:
	case RIGHT_ARROW_2:
		k = 0;
		if(i < numprompts-1)
			i++;
		else
			i = 0;
		break;
	case UP_ARROW:
	case UP_ARROW_2:
	case LEFT_ARROW:
	case LEFT_ARROW_2:
		k = 0;
		if(i > 0)
			i--;
		else
			i = numprompts-1;
		break;
	case HOME:
	case PAGE_UP:
		k = 0;
		i=0;
		break;
	case END:
	case PAGE_DOWN:
		k = 0;
		i=numprompts-1;
		break;
	case ESC:
		done = -1;
		break;
	case SPACE:
	case F1:
	case F2:
	case F3:
	case F4:
	case F5:
	case F6:
	case F7:
	case F8:
	case F9:
	case F10:
	case ENTER:
	case ENTER_2:
		done = 1;
		break;
	default:
		if ((c == '-' || c == '+' || c == '.' ||
			(c >= '0' && c <= '9')) &&
			k < 14) {
			temp2[i][k++] = c;
			temp2[i][k] = 0;
			}
		if ((c == 8 || c == 127 || c == 1083) && k > 0) {
			k--;
			temp2[i][k] = 0;
			}
		break;
		}
	}

if (done >= 0)
	for (i = 0; i < numprompts; i++)	/* fill in the results */
		values[i] = atof(temp2[i]);

return(done);
}

fullscreen_prompt2(		/* full-screen prompting routine */
	int startrow,		/* start on this row */
	int numprompts, 	/* there are this many prompts (max) */
	char * far *prompts,	/* array of prompting pointers */
	struct fullscreenvalues values[]		/* array of (initial) values */
	)
{
char temp2[24][15];		/* string value of answers go here */
int row, col, c, done, i, j, k;

if (numprompts <= 0)		/* ?? nothing to do! */
	return(0);

fullscreen_setup();		/* set up for full-screen prompting */

movecursor(25,81);		/* kill cursor */

putstring(22,10,7,string001,0);
putstring(23,10,7,string002,0);
putstring(24,10,7,string003,0);

col = 0;
for (i = 0; i < numprompts; i++)
{	/* calculate initial display */
    if(values[i].type == 'd')
	   sprintf(temp2[i], "%-10g", values[i].uval.dval);
    else if(values[i].type == 's')
	   sprintf(temp2[i], "%-10s",  values[i].uval.sval);
    else
    {
       /* this shouldn't happen */
       if(debugflag)
       {
	  movecursor(0,0);
	  printf(" %d %c\n",i,values[i].type);
	  getch();
       }
    }
	if (col < strlen(prompts[i]))
		col = strlen(prompts[i]);
}

col+= 20;	/* answers start in this column */
if (col > 65) col = 65;

for (i = 0; i < numprompts; i++) {	/* generate initial display */
	putstring(startrow+i, 5, 7, prompts[i], 0);
	putstring(startrow+i, col, 7, temp2[i], 0);
	}

i = 0;
k = 0;
done = 0;
while (!done) {
	putstring(startrow+i, col-1, 112, "\032              \033", 0);
	putstring(startrow+i, col, 112, temp2[i], 0);
	while (! keypressed());
	c = getakey();
	putstring(startrow+i, col-1, 7,   "                ", 0);
	putstring(startrow+i, col, 7,	temp2[i], 0);

	switch(c) {
	case DOWN_ARROW:
	case DOWN_ARROW_2:
	case RIGHT_ARROW:
	case RIGHT_ARROW_2:
		k = 0;
		if(i < numprompts-1)
			i++;
		else
			i = 0;
		break;
	case UP_ARROW:
	case UP_ARROW_2:
	case LEFT_ARROW:
	case LEFT_ARROW_2:
		k = 0;
		if(i > 0)
			i--;
		else
			i = numprompts-1;
		break;
	case HOME:
	case PAGE_UP:
		k = 0;
		i=0;
		break;
	case END:
	case PAGE_DOWN:
		k = 0;
		i=numprompts-1;
		break;
	case ESC:
		done = -1;
		break;
	case SPACE:
	case F1:
	case F2:
	case F3:
	case F4:
	case F5:
	case F6:
	case F7:
	case F8:
	case F9:
	case F10:
	case ENTER:
	case ENTER_2:
		done = 1;
		break;
	default:
	if(values[i].type=='d')
	{
	   if ((c == '-' || c == '+' || c == '.' ||
			   (c >= '0' && c <= '9')) && k < 14)
	    {
			   temp2[i][k++] = c;
			   temp2[i][k] = 0;
			}
	    else if ((c == 8 || c == 127 || c == 1083) && k > 0)
	    {
			   k--;
			   temp2[i][k] = 0;
		    }
		}
		else if( c >= 32 && c < 127)
	{
			temp2[i][k++] = c;
			temp2[i][k] = 0;
	}
	else if ((c == 8 || c == 127 || c == 1083) && k > 0)
	{
	    k--;
			temp2[i][k] = 0;
		}
		break;
	}
    }
    if (done >= 0)
	for (i = 0; i < numprompts; i++)	/* fill in the results */
    {
       if(values[i].type == 'd')
	      values[i].uval.dval = atof(temp2[i]);
       else if(values[i].type == 's')
	   strncpy(values[i].uval.sval,temp2[i],16);
    }
movecursor(0,0);

return(done);
}

/* --------------------------------------------------------------------- */

/* prompt for and select choices */
fullscreen_choice(int numchoices,	  /* How many choices in list  */
		  char *choices[1],	  /* array of choices	       */
		  int numcols,		  /* how many columns	       */
		  int startrow, 	  /* upper left corner of list */
		  int startcol, 	  /* upper left corner of list */
		  int current)		  /* start with this item      */
{
   char *s;
   int done;
   int colwidth;		 /* width of a column */
   int c;
   int crow, ccol, i, j, k, k1;
   int attribute;

   fullscreen_setup();			/* set up full-screen values */
   colwidth = 22;		  /* width of a column */
   if(numcols == 1)
      colwidth = 1;


   numchoices--;

   /* display the list */
   k = current;
   for(i=0;i<=numchoices;i++)
   {
      ccol  = startcol + (i%numcols)*colwidth;
      crow  = startrow + i/(numcols);
      putstring(crow,ccol,7,choices[i],0);
   }
   movecursor(25,81); /* kill cursor */

   putstring(22,(crtcols-50)/2,7,
      "Select one of the available choices above",
      0);
   putstring(23,(crtcols-66)/2,7,
      "Use your cursor keypad, HOME, and END to move to your chosen item",
      0);
   putstring(24,(crtcols-66)/2,7,
      "Press ENTER or the Space Bar when finished (or ESCAPE to back out)",
      0);

   i = k;    /* start with current type highlighted */
   ccol  = startcol + (i%numcols)*colwidth;
   crow  = startrow + i/(numcols);
   done = 0;
   while(!done)
   {
      /* write type in reverse video */
      putstring(crow,ccol-2,112,"\032\032", 0);
      putstring(crow,ccol,112,choices[i],0);
      while (! keypressed());
      c = getakey();

      /* write type back to normal video */
      putstring(crow,ccol-2,7,"  ", 0);
      putstring(crow,ccol, 7,choices[i],0);

      switch(c)
      {
      case DOWN_ARROW:
      case DOWN_ARROW_2:
	 i += numcols;
	 if(i > numchoices)
	    i = i%numcols;
	 break;
      case UP_ARROW:
      case UP_ARROW_2:
	 i -= numcols;
	 if(i < 0)
	    i = (numchoices/numcols)*numcols+i%numcols;
	 break;
      case HOME:
	 i=0;
	 break;
      case END:
	 i=numchoices;
	 break;
      case RIGHT_ARROW:
      case RIGHT_ARROW_2:
	 i ++;
	 if(i > numchoices)
	    i = 0;
	 break;
      case LEFT_ARROW:
      case LEFT_ARROW_2:
	 i --;
	 if(i < 0)
	    i = numchoices;
	 break;
      case ENTER:
      case ENTER_2:
      case SPACE:
      case F1:
      case F2:
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
      case F10:
	 done = 1;
	 break;
      case ESC:
	 done = -1;
	 break;
      default:
	 continue;
      }
      ccol = startcol + (i%numcols)*colwidth;
      crow = startrow + i/(numcols);
   }
   setclear();
   if (done < 0)
      return(done);
   else
      return(i);
}

/* --------------------------------------------------------------------- */

/* compare for sort of type table */
compare(const void *i, const void *j)
{
   return(strcmp(fractalspecific[(int)*((unsigned char*)i)].name,
	       fractalspecific[(int)*((unsigned char*)j)].name));
}

/* --------------------------------------------------------------------- */

clear_line(int row, int start, int stop, int color) /* clear part of a line */
{
   int col;
   for(col=start;col<= stop;col++)
      putstring(row,col,color," ",0);
}

/* --------------------------------------------------------------------- */
get_fracttype(int t)		/* prompt for and select fractal type */
{
   char *speed_pt;
   int speed_col;
   char tname[40];	    /* target name for speed key */
   int cct;		    /* which character for speed key */
   char *s;
   int oldhelpmode;
   int done;
   int numtypes;		 /* length of list    */
   int numcols; 		 /* number of columns */
   int colwidth;		 /* width of a column */
   int startrow, startcol;	 /* upper left corner of list */
   int c;
   int crow, ccol, i, j, k, k1;
   int attribute;

   unsigned char onthelist[256];	/* list of REAL name locations */

restart:
   lookatmouse = 2; /* don't need to save/restore old value, main will reset */
   speed_col = 15;  /* column to start speed key prompt */
   speed_pt = "Speed key string ->";
   *tname = cct = 0;
   setvideomode(3,0,0,0);		/* switch to text mode */

   fullscreen_setup();			/* set up full-screen values */

   /* setup context sensitive help */
   oldhelpmode = helpmode;
   helpmode = HELPFRACTALS;

   /* set up details of how type list is displayed on the screen */
   numcols  =  5;		  /* number of columns */
   colwidth = 16;		  /* width of a column */
   startrow =  5;		  /* upper left corner of list */
   startcol =  1;		  /* upper left corner of list */

   i = -1;
   j = -1;
   k = 0;
   while(fractalspecific[++i].name)
   {
      if (fractalspecific[i].name[0] == '*')
	 continue;
      onthelist[++j] = i;	/* remember where the real item is */
      if (strcmp(fractalspecific[onthelist[j]].name,
		 fractalspecific[t].name) == 0 ||
	  (fractalspecific[t].name[0] == '*' &&
	  strcmp(fractalspecific[onthelist[j]].name,
		 fractalspecific[fractalspecific[t].tofloat].name) == 0))
	  k = j;
   }
   k1 = onthelist[k]; /* remember index of current type */
   numtypes = j;
   onthelist[numtypes+1] = 0;	/* allow index to overshoot harmlessly */
   qsort(onthelist,1+numtypes,1,compare); /* sort type list */

   putstring(2,(crtcols-50)/2,7,
      "Select one of the available fractal types below",
      0);

   /* display the list */
   for(i=0;i<=numtypes;i++)
   {
      if(onthelist[i] == k1)
	 k = i;  /* this is the current type */
      ccol  = startcol + (i%numcols)*colwidth;
      crow  = startrow + i/(numcols);
      putstring(crow,ccol,7,fractalspecific[onthelist[i]].name,0);
   }
   movecursor(25,81); /* kill cursor */

   putstring(22,(crtcols-66)/2,7,
      "Use your cursor keypad, HOME, and END to move to your chosen item",
      0);
   putstring(23,(crtcols-66)/2,7,
      "(or press the F1 key for a short description of each fractal type)",
      0);
   putstring(24,(crtcols-66)/2,7,
      "Press <ENTER> to select or <ESC> to return to the previous screen",
      0);

   i = k;    /* start with current type highlighted */
   ccol  = startcol + (i%numcols)*colwidth;
   crow  = startrow + i/(numcols);
   done = 0;
   while(!done)
   {
      /* write type in reverse video */
      putstring(crow,ccol,112,fractalspecific[onthelist[i]].name,0);

      while (! keypressed()); /* wait for a keystroke */
      c = getakey();

      /* write type back to normal video */
      putstring(crow,ccol, 7,fractalspecific[onthelist[i]].name,0);

      /* speed key case */
      if( (33 <= c && c <= 126) || (c==8 && cct) )
      {
	 int match;
	 c = tolower(c);
	 if(c==8) /* backspace */
	 {
	    putstring(21,speed_col+strlen(speed_pt)+ --cct,7," ",0);
	    tname[cct] = 0;
	    if(cct == 0) /* backspaced out of speed mode */
	       clear_line(21,speed_col,crtcols-1,7);
	 }
	 else
	 {
	    if(cct<39) /* add char to speed key buffer */
	    {
	       tname[cct] = c;
	       tname[++cct] = 0;
	    }
	 }

	 /* write speed key buffer monitor window */
	 putstring(21,speed_col,7,speed_pt,0);
	 putstring(21,speed_col+strlen(speed_pt),112,tname,0);

	 /* locate matching type */
	 i = 0; /* start at top of list */
	 while(i <= numtypes &&
	     (match=strncmp(tname,fractalspecific[onthelist[i]].name,cct))>0)
	    i++;
	 if(match < 0 && i > 0)  /* oops - overshot    */
	    i--;
	 else if(i > numtypes) /* bumped end of list */
	    i = numtypes;
      }
      else
      {
	 if(c==27 && cct)
	    c = 0; /* exit speed key mode */
	 *tname = cct = 0;
	 clear_line(21,speed_col,crtcols-1,7); /* zap speed key window */
      }
      switch(c)
      {
      case DOWN_ARROW:
      case DOWN_ARROW_2:
	 i += numcols;
	 if(i > numtypes)
	    i = i%numcols;
	 break;
      case UP_ARROW:
      case UP_ARROW_2:
	 i -= numcols;
	 if(i < 0)
	    i = (numtypes/numcols)*numcols+i%numcols;
	 break;
      case HOME:
	 i=0;
	 break;
      case END:
	 i=numtypes;
	 break;
      case RIGHT_ARROW:
      case RIGHT_ARROW_2:
	 i ++;
	 if(i > numtypes)
	    i = 0;
	 break;
      case LEFT_ARROW:
      case LEFT_ARROW_2:
	 i --;
	 if(i < 0)
	    i = numtypes;
	 break;
      case ENTER:
      case ENTER_2:
      case SPACE:
      case F1:
      case F2:
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
      case F10:
	 done = 1;
	 break;
      case ESC:
	 done = -1;
	 break;
      default:
	 break;
      }
      /* calculate position of type on screen */
      ccol = startcol + (i%numcols)*colwidth;
      crow = startrow + i/(numcols);
   }
   clscr();
   helpmode = oldhelpmode;
   if (done >= 0) {
      if (get_fract_params(onthelist[i], t) < 0)
	goto restart;
      return(onthelist[i]);
      }
   return(-1);
}

/* --------------------------------------------------------------------- */

get_fract_params(		/* prompt for fractal parameters */
	int newfractype,	/* new fractal type */
	int oldfractype 	/* previous fractal type */
	)
{
int numparams,numtrig;
int i;
struct fullscreenvalues paramvalues[10];
char *choices[10];
int oldbailout;
int promptnum;

static char *trg[] = {"First Function","Second Function",
		      "Third Function","Fourth Function"};

initfractype = newfractype;

if (strcmp(fractalspecific[initfractype].param[3],
	"Imaginary portion of p2") == 0)
	if (get_formula_name() < 0)	/* scan for the formula name */
		return(-1);

 for ( i = 0; i < 4; i++)
 {
	 initparam[i] = fractalspecific[initfractype].paramvalue[i];
	 paramvalues[i].type = 'd';
	 paramvalues[i].uval.dval = initparam[i];
 }
for (numparams = 0; numparams < 4; numparams++) {
	if (fractalspecific[initfractype].param[numparams][0] == 0) break;
	choices[numparams] = fractalspecific[initfractype].param[numparams];
	if (numparams == 0) {
		printf("\n\n Select your options for fractal type %s",
			fractalspecific[initfractype].name[0] != '*' ?
			fractalspecific[initfractype].name	     :
			&fractalspecific[initfractype].name[1]	     );
		printf("\n\n Please enter any parameters that apply \n\n");
		}
	}
 numtrig = get_trig_num(fractalspecific[initfractype].flags);

  for(i=0; i<numtrig; i++)
  {
    paramvalues[i+numparams].type = 's';
    strcpy(paramvalues[i+numparams].uval.sval,trigfn[trigndx[i]].name);
    choices[i+numparams] = trg[i];
  }
  if(fabs(potparam[2]) == 0.0 && (decomp[0] <= 0 || decomp[1] <= 0)
   && fractalspecific[initfractype].orbit_bailout)
  {
     promptnum = numparams+numtrig+1;
     choices[numparams+numtrig] = "Bailout value (0 means use default)";
     paramvalues[numparams+numtrig].type = 'd';
     paramvalues[numparams+numtrig].uval.dval = oldbailout = bailout;
  }
  else	/* under these conditions bailout is ignored, so why ask for it? */
     promptnum = numparams+numtrig;
 if (fullscreen_prompt2(10,promptnum,choices, paramvalues) < 0)
		 return(-1);
 for ( i = 0; i < numparams; i++)
	 initparam[i] = paramvalues[i].uval.dval;
 for ( i = 0; i < numtrig; i++)
 {
    paramvalues[i+numparams].uval.sval[4] = 0;
    if(paramvalues[i+numparams].uval.sval[3]==32)
	paramvalues[i+numparams].uval.sval[3] = 0;
    set_trig_array(i, paramvalues[i+numparams].uval.sval);
 }

  if(fabs(potparam[2]) == 0.0 && (decomp[0] <= 0 || decomp[1] <= 0)
   && fractalspecific[initfractype].orbit_bailout)
  {
     bailout = paramvalues[numparams+numtrig].uval.dval;

     if (bailout != 0 && (bailout < 4 || bailout > 32000))
	bailout = oldbailout;
  }

 if (newfractype != oldfractype) {
	 invert = 0;
	 inversion[0] = inversion[1] = inversion[2] = 0;
	 for ( i = 0; i < 3; i++)
		 potparam[i] = 0.0;
	 };

initcorners = 1;
initxmin = fractalspecific[initfractype].xmin;
initxmax = fractalspecific[initfractype].xmax;
initymin = fractalspecific[initfractype].ymin;
initymax = fractalspecific[initfractype].ymax;
initx3rd = initxmin;
inity3rd = initymin;
for ( i = 0; i < 4; i++)
   fractalspecific[newfractype].paramvalue[i] = initparam[i];

return(0);

}

/* --------------------------------------------------------------------- */

/*
  Read a formula file, picking off the formula names.
  Formulas use the format "  name = { ... }  name = { ... } "
*/

extern char FormFileName[];	/* file to find the formulas in */
extern char FormName[]; 	/* Name of the Formula (if not null) */

get_formula_name()	/* get the fractal formula name */
{
   int numcols; 		 /* number of columns */
   int startrow, startcol;	 /* upper left corner of list */
   char *choices[41], choicetext[41][21];
   char fullfilename[200];	/* Full file name */
   int numformulas, i;
   FILE *File;

   FormName[0] = 0;		/* start by declaring failure */
   for (i = 0; i < 41; i++) {
      choicetext[i][0] = 0;
      choices[i] = choicetext[i];
      }

   setclear();
   printf("\n\n Enter the name of your Formula File (if not %s) .. ",
      FormFileName);
   gets(temp1);
   if (temp1[0] != 0) strcpy(FormFileName, temp1);
      if (strchr(FormFileName,'.') == NULL)
	 strcat(FormFileName,".frm");

   findpath(FormFileName, fullfilename);	/* BDT get full path name */

   if((File = fopen(fullfilename, "rt")) == NULL) {
     buzzer(2);
     printf("\n?? I Can't find %s\n", FormFileName);
     printf("\n\n%s",string004);
     getakey();
     return(-1);
   }

   numformulas = 0;
   while(fscanf(File, " %20[^ \n\t({]", choices[numformulas]) != EOF) {
      int c;

      while(c = getc(File)) {
	 if(c == EOF || c == '{' || c == '\n')
	    break;
      }
      if(c == EOF)
	 break;
      else if(c != '\n'){
	 numformulas++;
	 if (numformulas >= 41-1) break;
skipcomments:
	 if(fscanf(File, "%200[^}]", fullfilename) == EOF) break;
	 if (getc(File) != '}') goto skipcomments;
	 if (stricmp(choices[numformulas-1],"") == 0 ||
	     stricmp(choices[numformulas-1],"comment") == 0)
		 numformulas--;
      }
   }
   fclose(File);

   /* set up details of how type list is displayed on the screen */
   numcols  =  3;		  /* number of columns */
   startrow =  5;		  /* upper left corner of list */
   startcol = 10;		  /* upper left corner of list */

   if ((i = fullscreen_choice(numformulas, choices,numcols,startrow,startcol,0)) >= 0)
      strcpy(FormName, choices[i]);

   if (RunForm(FormName)) {
       FormName[0] = 0; 	/* declare failure */
       buzzer(2);
       printf("\n\n%s",string004);
       getakey();
       return(-1);
       }

return(0);
}

/* --------------------------------------------------------------------- */

get_decomp_params()		/* prompt for decomposition parameters */
{
static char *decomp_prompts[] = {
   "Number of Colors  (0 for no decomposition)",
   "New Bailout Value (0 for default value)",
   ""
   };
double values[2];

    memcpy(olddacbox,dacbox,256*3);	   /* save the DAC */
    setvideomode(3,0,0,0);		   /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);	   /* restore the DAC */

   values[0] = decomp[0];
   values[1] = 0;
   printf("\n\nPlease enter your Decomposition Parameters:\n");
   if (fullscreen_prompt(10,2,decomp_prompts,values) < 0)
      return(-1);
   decomp[0] = values[0];
   decomp[1] = values[1];
   return(0);
}

/* --------------------------------------------------------------------- */

get_invert_params()		/* prompt for inversion parameters */
{
extern int StandardFractal();
extern int calcmand();
static char *invert_prompts[] = {
   "Radius of inversion or \"auto\")",
   "X Center inversion  or \"auto\"",
   "Y Center inversion  or \"auto\"",
   ""
   };
struct fullscreenvalues values[4];
int i;
   if(fractalspecific[initfractype].calctype != StandardFractal &&
      fractalspecific[initfractype].calctype != calcmand)
      return(0);

    memcpy(olddacbox,dacbox,256*3);	   /* save the DAC */
    setvideomode(3,0,0,0);		   /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);	   /* restore the DAC */

   helpmessage(inversionmessage);

   for(i=0;i<3;i++)
   {
       values[i].type = 's';
       if(inversion[i] == AUTOINVERT)
	  sprintf(values[i].uval.sval,"auto");
       else
	  sprintf(values[i].uval.sval,"%g",inversion[i]);
   }
   if (fullscreen_prompt2(10,3,invert_prompts,values) < 0)
      return(-1);
   for(i=0;i<3;i++)
      if(values[i].uval.sval[0] == 'a' || values[i].uval.sval[0] == 'A')
	 inversion[i] = AUTOINVERT;
      else
	 inversion[i] = atof(values[i].uval.sval);
   invert = 3;
   return(0);
}

/* --------------------------------------------------------------------- */

get_ifs3d_params()		/* prompt for 3D IFS parameters */
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
   double values[sizeof(ifs3d_prompts)/sizeof(char *)];

    fullscreen_setup();
    memcpy(olddacbox,dacbox,256*3);	   /* save the DAC */
    setvideomode(3,0,0,0);		   /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);	   /* restore the DAC */

   k = 0;
   values[k++] = XROT;
   values[k++] = YROT;
   values[k++] = ZROT;
   values[k++] = ZVIEWER;
   values[k++] = XSHIFT;
   values[k++] = YSHIFT;
   values[k++] = glassestype;
   s = "3D Parameters";
      putstring(0,(crtcols-strlen(s))/2,7,s,0);

   if (fullscreen_prompt(3,k,ifs3d_prompts,values) < 0)
      return(-1);
   k = 0;
   XROT    =  values[k++];
   YROT    =  values[k++];
   ZROT    =  values[k++];
   ZVIEWER =  values[k++];
   XSHIFT  =  values[k++];
   YSHIFT  =  values[k++];
   glassestype = values[k++];
   if(glassestype)
   {
      if(get_funny_glasses_params())
	 return(-1);
      check_mapfile();
   }
   return(0);
}

/* --------------------------------------------------------------------- */

get_3d_params() 	/* prompt for 3D parameters */
{
   int numcols; 		/* number of columns */
   int startrow;		/* upper left corner of list */
   int startcol;		/* upper left corner of list */
   char *choices[8];
   char c;
   int numprompts;
   int sphere,filltype;
   char *s;

   char *prompts3d[25];
   double values[25];

   struct fullscreenvalues uvalues[25];
   int i, j, k;

   if (initmode < 0 || !display3d || initbatch)
      return(0);

restart_1:

   fullscreen_setup(); /* just to get value of crtcols */
   setclear();

   s = "3D Mode Selection";
   putstring(0,(crtcols-strlen(s))/2,7,s,0);

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

   prompts3d[++k]= "";

   if (fullscreen_prompt2(3,k,prompts3d,uvalues) < 0)
      return(-1);

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
   numcols  =  1;		  /* number of columns */
   startrow = 12;		  /* upper left corner of list */
   startcol = 23;		  /* upper left corner of list */

   s = "Select Fill Type";
   putstring(startrow-2,(crtcols-strlen(s))/2,7,s,0);
   clear_line(22,10,60,7);
   clear_line(23,10,60,7);
   clear_line(24,10,60,7);

   i = FILLTYPE+1;

   if ((i = fullscreen_choice(k, choices,numcols,startrow,startcol,i)) >= 0)
      FILLTYPE = i-1;
   else goto restart_1;

   if(glassestype)
   {
      if(get_funny_glasses_params())
	 goto restart_1;
   }

   check_mapfile();

restart_3:

   setclear();

   if(SPHERE)
   {
      s = "Sphere 3D Parameters";
      putstring(0,(crtcols-strlen(s))/2,7,s,0);
      s = "Sphere is on its side; North pole to right";
      putstring(1,(crtcols-strlen(s))/2,7,s,0);
      s = "180 deg long. is top; 0 deg bottom;-90 deg lat. is left; 90 deg. is right";
      putstring(2,(crtcols-strlen(s))/2,7,s,0);

      prompts3d[0] = "Longitude start (degrees)";
      prompts3d[1] = "Longitude stop  (degrees)";
      prompts3d[2] = "Latitude start  (degrees)";
      prompts3d[3] = "Latitude stop   (degrees)";
      prompts3d[4] = "Radius scaling factor in pct";

   }
   else
   {
      s = "Planar 3D Parameters";
      putstring(0,(crtcols-strlen(s))/2,7,s,0);
      s = "Pre-rotation X axis is screen top; Y axis is the left side";
      putstring(1,(crtcols-strlen(s))/2,7,s,0);
      s = "Pre-rotation Z axis is coming right at you out of the screen!";
      putstring(2,(crtcols-strlen(s))/2,7,s,0);

      prompts3d[0] = "X-axis rotation in degrees";
      prompts3d[1] = "Y-axis rotation in degrees";
      prompts3d[2] = "Z-axis rotation in degrees";
      prompts3d[3] = "X-axis scaling factor in pct";
      prompts3d[4] = "Y-axis scaling factor in pct";
   }
   values[0]   = XROT	   ;
   values[1]   = YROT	   ;
   values[2]   = ZROT	   ;
   values[3]   = XSCALE    ;
   values[4]   = YSCALE    ;
   k = 5;
   prompts3d[k]= "Surface Roughness scaling factor in pct";
   values[k++] = ROUGH	   ;

   prompts3d[k]= "'Water Level' (minimum color value)";
   values[k++] = WATERLINE ;

   prompts3d[k]= "Perspective distance [1 - 999, 0 for no persp]";
   values[k++] = ZVIEWER   ;

   prompts3d[k]= "X shift with perspective (positive = right)";
   values[k++] = XSHIFT    ;

   prompts3d[k]= "Y shift with perspective (positive = up   )";
   values[k++] = YSHIFT    ;

   prompts3d[k]= "Image non-perspective X adjust (positive = right)";
   values[k++] = xtrans    ;

   prompts3d[k]= "Image non-perspective Y adjust (positive = up)";
   values[k++] = ytrans    ;

   prompts3d[k]= "First transparent color";
   values[k++] = transparent[0];

   prompts3d[k]= "Last transparent color";
   values[k++] = transparent[1];

   prompts3d[k]= "Randomize Colors      (0 - 7, '0' disables)";
   values[k++] = RANDOMIZE;

   if (ILLUMINE)
   {
	prompts3d[k]= "Color/Mono Images With Light Source   (1 = Color)";
	values[k++] = full_color;
   }

   if (fullscreen_prompt(4,k,prompts3d,values) < 0)
      goto restart_1;

   k = 0;
   XROT       = values[k++];
   YROT       = values[k++];
   ZROT       = values[k++];
   XSCALE     = values[k++];
   YSCALE     = values[k++];
   ROUGH      = values[k++];
   WATERLINE  = values[k++];
   ZVIEWER    = values[k++];
   XSHIFT     = values[k++];
   YSHIFT     = values[k++];
   xtrans     = values[k++];
   ytrans     = values[k++];
   transparent[0] = values[k++];
   transparent[1] = values[k++];
   RANDOMIZE  = values[k++];
   if (RANDOMIZE >= 7) RANDOMIZE = 7;
   if (RANDOMIZE <= 0) RANDOMIZE = 0;

   if (ILLUMINE)
   {
	full_color = values[k++];
	if(get_light_params())
	    goto restart_3;
   }

return(0);
}

/* --------------------------------------------------------------------- */
get_light_params()
{
   char *prompts3d[10];
   double values[10];
   struct fullscreenvalues uvalues[10];
   char *s;
   int k;

   /* defaults go here */

   setclear();

   s = "Light Source Parameters";
   putstring(0,(crtcols-strlen(s))/2,7,s,0);

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

   if (fullscreen_prompt2(3,k,prompts3d,uvalues) < 0)
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


check_mapfile()
{
    char temp[81];
    while ((FILLTYPE >= 5) || overlay3d || (mapset == 1) || glassestype)
    {
	setclear();
	movecursor(5,5);
	if (mapset && ((glassestype==0) || (glassestype==3)))
	{
static char far replmapmsg[]={"\
, has been previously selected.\n\
Press \"CR\" to continue using it.\n\
Enter \"*\" to use the palette from the image about to be loaded.\n  ==> "};
	    printf("\n\nA replacement .MAP file, %s",MAP_name);
	    helpmessage(replmapmsg);
	    gets (temp1);
	    if (temp1[0] == '\0') /* CR pressed */
		break;
	    if (temp1[0] == '\x2A') /* "*" pressed */
	    {
		mapset = 0;
		break;
	    }
	}
	else if((glassestype==0) || (glassestype==3))
	{
static char far usemapmsg[]={"\
\n\nEnter name of .MAP file to use, or \n\
Press \"CR\" or enter \"*\" to use palette from the image about to be loaded.\n\
  ==> "};
	    movecursor(5,5);
	    helpmessage(usemapmsg);
	    gets (temp1);
	    if (temp1[0] == '\0' || temp1[0] == '\x2A')
	    { /* CR or * pressed */
		mapset = 0;
		break;
	    }
	}
	if(0 < glassestype && glassestype < 3)
	   strcpy(temp1,funnyglasses_map_name);
	if (strchr(temp1,'.') == NULL) /* Did name have an extention? */
	    strcat(temp1,".map"); /* No? Then add .map */
	findpath(temp1,temp); /* Find complete path name */
	dacfile = fopen(temp,"r"); /* Open it */
	if (dacfile == NULL)
	{ /* Oops, somethings wrong */
	    if(glassestype == 2)
	    {
		mapset = 2; /* Need to generate glasses2.map */
		break;
	    }
	    else if (glassestype == 1)
	    {
		mapset = 3; /* Need to generate glasses1.map */
		break; /* compute palette on the fly goes here */
	    }
	    buzzer(2);
	    printf ("\nSorry, cannot open %s \n", temp1);
	    printf (string004);
	    getch();
	    setclear();
	    continue;
	}
	/* File has been opened OK */
	ValidateLuts(dacfile);	/* read the palette file to make sure its ok */
	fclose(dacfile); /* close it */
	mapset = 1;
	strcpy (MAP_name,temp);
	break; /* Done */
    }
    return(0);
}

get_funny_glasses_params()
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
      strcpy(funnyglasses_map_name,"glasses2.map");

   setclear();

   s = "Funny Glasses Parameters";
   putstring(0,(crtcols-strlen(s))/2,7,s,0);

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

   prompts3d[++k]= "";

   if (fullscreen_prompt2(3,k,prompts3d,uvalues) < 0)
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

get_ifs_params()		/* prompt for IFS params */
{
char *s;
int numcols;		      /* number of columns */
int startrow, startcol;       /* upper left corner of list */
char *choices[4], choicetext[4][50];
char *filename;
float far *initarray;
static char ifstype = '2';
int totrows, totcols;
int i, j, numlines;
FILE *tempfile;

for (i = 0; i < 4; i++)
    choices[i] = choicetext[i];

setfortext();			/* switch to text mode */

memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
setvideomode(3,0,0,0);		       /* clear the screen entirely */
memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */


ifsgetfile();			/* read in any "IFS=" file */

ifs3dgetfile(); 		/* read in any "IFS3D=" file */

filename = ifsfilename;
initarray = &initifs[0][0];
totrows = NUMIFS;
totcols = IFSPARM;

/* assume if an IFS type is already selected, user wants to edit that type */
ifstype = 0;
if(fractype == IFS)
   ifstype = '2';
else if (fractype == IFS3D)
   ifstype = '3';
else if (calc_status >= 0) {   /* there's a fractal image active */
   ifstype = 't';
   get_ifs3d_params();
   return(0);
   }
   strcpy(choices[0],"2D IFS Codes");
   strcpy(choices[1],"3D IFS Codes");
   strcpy(choices[2],"3D Transform Parameters");

   /* set up details of how type list is displayed on the screen */
   numcols  =  1;		  /* number of columns */
   startrow =  5;		  /* upper left corner of list */
   startcol = 30;		  /* upper left corner of list */
   fullscreen_setup(); /* just to get value of crtcols */
   setclear();

   s = "Edit Options";
   putstring(0,(crtcols-strlen(s))/2,7,s,0);
   if(ifstype == '2')
      i = 0;
   else if(ifstype == '3')
      i = 1;
  else
      i = 2;
    i = fullscreen_choice(3, choices,numcols,startrow,startcol,i);
    switch(i)
    {
    case 0:
       ifstype = '2';
       break;
    case 1:
       ifstype = '3';
       break;
    case 2:
       printf("\n\n");
       get_ifs3d_params();
       return(0);
       break;
     default:
       return(0);
       break;
    }
if (ifstype == '3') {
   filename = ifs3dfilename;
   initarray = &initifs3d[0][0];
   totrows = NUMIFS;
   totcols = IFS3DPARM;
   }

for ( ;; ) {

    for (numlines = 0; numlines < totrows; numlines++)	  /* find the first zero entry */
	if (initarray[(numlines * totcols) + totcols - 1] <= 0.0001) break;

    memcpy(olddacbox,dacbox,256*3);	   /* save the DAC */
    setvideomode(3,0,0,0);		   /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);	   /* restore the DAC */

{
static char far ifsparmmsg1[]={"\
 Your current IFS Parameters are: \n\n\
#   a	  b	c     d     e	  f"};
static char far ifsparmmsg2[]={"\
     g	   h	 i     j     k	   l"};
    helpmessage(ifsparmmsg1);
    if (ifstype == '3')
	helpmessage(ifsparmmsg2);
    printf("    prob \n\n");
}

    for (i = 0; i < numlines; i++) {
	printf("%1d", i+1);
	for (j = 0; j < totcols; j++)
	    printf("%6.2f", (float )initarray[(i*totcols)+j]);
	printf("\n");
	}

{
static char far ifseditmsg1[]={"\
\n\n Enter the number of the line you want to edit\n\
  (or RESTORE to start from another (.IFS) file, or SAVE to\n"};
static char far ifseditmsg2[]={"\
   save your edits in an (.IFS) file, or TRANSFORM to alter\n\
   3D transformation values, or <ENTER> to end) ==> "};
static char far ifseditmsg3[]={"\
   save your edits in an (.IFS) file, or <ENTER> to end) ==> "};
    helpmessage(ifseditmsg1);
    if(ifstype == '3')
       helpmessage(ifseditmsg2);
    else
       helpmessage(ifseditmsg3);
}
    gets(temp1);
    if (ifstype == '3' && (temp1[0] == 't' || temp1[0] == 'T')) {
       get_ifs3d_params();
       continue;
       }
    if (temp1[0] == 's' || temp1[0] == 'S') {
       printf("\n\n enter the name of your new .IFS file ==> ");
       gets(filename);
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
       return(0);
       }
    if (temp1[0] == 'r' || temp1[0] == 'R') {
       printf("\n\n enter the name of your new .IFS file ==> ");
       gets(filename);
       if (strchr(filename,'.') == NULL)
	  strcat(filename,".ifs");
       if (ifstype == '3')
	  ifs3dgetfile();
       else
	  ifsgetfile();
       continue;
       }
    i = atoi(temp1) - 1;
    if (temp1[0] == 0 || i < 0 || i > numlines) return(0);

    for (j = 0; j < totcols; j++) {
	printf(" Parameter %2d (if not %6.2f) ", j, (float)initarray[(i*totcols)+j]);
	gets(temp1);
	if (temp1[0] != 0)
	    initarray[(i*totcols)+j] = atof(temp1);
	}
    }
}

/* --------------------------------------------------------------------- */
/*
	get_toggles() is called from FRACTINT.C whenever the 'x' key
	is pressed.  This routine prompts for several options,
	sets the appropriate variables, and returns the following code
	to the calling routine:

	-1  routine was ESCAPEd - no need to re-generate the image.
	 0  minor variable changed (such as "warn=").  No need to
	    re-generate the image.
	1   major variable changed (such as "inside=").  Re-generate
	    the image.
	2   Floating-point toggle changed.  FRACTINT.C takes special
	    actions in this instance, as the algorithm itself changes.

	Finally, remember to insert variables in the list *and* check
	for them in the same order!!!
*/

get_toggles()
{
   int numcols; 		/* number of columns */
   int startrow;		/* upper left corner of list */
   int startcol;		/* upper left corner of list */
   char *choices[20];
   char c;
   int numprompts;
   char *s;
   int oldhelpmode;

   double values[25], oldvalues[25];

   struct fullscreenvalues uvalues[25];
   int i, j, k, ipasses;

   /* fill up the choices (and previous values) arrays */
   k = -1;

   k++;
   choices[k] =  "Floating Point Algorithm";
   uvalues[k].type = 's';
   oldvalues[k] = floatflag;
   if (floatflag)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] =  "Passes (1, 2, g[uessing], or b[oundary trace])";
   uvalues[k].type = 's';
   oldvalues[k] = boundarytraceflag*100 + numpasses + solidguessing;
   /* passes klooge */
   strcpy(uvalues[k].uval.sval,"1");
   if (boundarytraceflag == 1)
      uvalues[k].uval.sval[0] = 'b';
   else if (solidguessing == 1)
	 uvalues[k].uval.sval[0] = 'g';
   else
	 uvalues[k].uval.sval[0] = '1' + numpasses;

   k++;
   choices[k] = "Biomorph Color (-1 means OFF)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = biomorph;

   k++;
   choices[k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
   uvalues[k].type = 'd';
   oldvalues[k] = decomp[0];
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Maximum Iterations (10 to 32000)";
   uvalues[k].type = 'd';
   oldvalues[k] = maxit;
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Inside Color (nnnn, maxiter, bof60, bof61)";
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
   choices[k] = "Look for finite attractor";
   uvalues[k].type = 's';
   oldvalues[k] = finattract;
   if (finattract)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Logarithmic Palettes";
   uvalues[k].type = 's';
   oldvalues[k] = LogFlag;
   if (LogFlag == 1)
      strcpy(uvalues[k].uval.sval,"yes");
   else if (LogFlag == 2)
      strcpy(uvalues[k].uval.sval,"old");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "File Overwrite ('warn=')";
   uvalues[k].type = 's';
   oldvalues[k] = warn;
   if (warn)
      strcpy(uvalues[k].uval.sval,"yes");
   else
      strcpy(uvalues[k].uval.sval,"no");

   k++;
   choices[k] = "Savename (.GIF implied)";
   uvalues[k].type = 's';
   oldvalues[k] = 0;
   strcpy(uvalues[k].uval.sval,savename);

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
/*
   k++;
   choices[k] = "Bailout value (0 means use default)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = bailout;
*/
   k++;
   choices[k] = "Distance Estimator Method (0 means off):";
   uvalues[k].type = 'd';
   oldvalues[k] = distest;
   uvalues[k].uval.dval = oldvalues[k];

   k++;
   choices[k] = "Potential param #1 (max color)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[0];

   k++;
   choices[k] = "Potential param #2 (slope)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[1];

   k++;
   choices[k] = "Potential param #3 (bailout)";
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = oldvalues[k] = potparam[2];

   k++;
   choices[k] = "Potential savefile name";
   uvalues[k].type = 's';
   oldvalues[k] = 0;
   strcpy(uvalues[k].uval.sval,potfile);

   fullscreen_setup();		/* set up for full-screen prompting */

   putstring(0,10,7,"The following options and doodads are in effect.",0);
   putstring(1,10,7,"Note that not all combinations make sense.",0);

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPAUTHORS;
   i = fullscreen_prompt2(4,k+1,choices,uvalues);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0)
      return(-1);

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      floatflag = 1;
   else
      floatflag = 0;
   if (floatflag != oldvalues[k]) j = 2;

   /* passes= klooge */
   if (uvalues[++k].uval.sval[0] == 'b' || uvalues[k].uval.sval[0] == 'B') {
      boundarytraceflag = 1;
      }
   else {
      boundarytraceflag = 0;
   }
   if (uvalues[k].uval.sval[0] == '1' || uvalues[k].uval.sval[0] == '2') {
      numpasses = uvalues[k].uval.sval[0] - '1';
      solidguessing = 0;
      }
   if (uvalues[k].uval.sval[0] == 'g' || uvalues[k].uval.sval[0] == 'G') {
      numpasses = 1;
      solidguessing = 1;
      }
   ipasses = boundarytraceflag*100 + numpasses + solidguessing;
   if (ipasses != oldvalues[k] && j < 1) j = 1;

   biomorph = uvalues[++k].uval.dval;
   if (biomorph != oldvalues[k] && j < 1) j = 1;

   decomp[0] = uvalues[++k].uval.dval;
   if (decomp[0] != oldvalues[k] && j < 1) j = 1;

   maxit = uvalues[++k].uval.dval;
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

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      finattract = 1;
   else
      finattract = 0;
   if (finattract != oldvalues[k]) j = 2;

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      LogFlag = 1;
   else if (uvalues[k].uval.sval[0] == 'o' || uvalues[k].uval.sval[0] == 'O')
      LogFlag = 2;
   else
      LogFlag = 0;
   if (LogFlag != oldvalues[k] && j < 1) j = 1;

   if (uvalues[++k].uval.sval[0] == 'y' || uvalues[k].uval.sval[0] == 'Y')
      warn = 1;
   else
      warn = 0;

   strcpy(savename,uvalues[++k].uval.sval);

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
/*
   bailout = uvalues[++k].uval.dval;
   if (bailout != 0 && (bailout < 4 || bailout > 32000))
      bailout = oldvalues[k];
   if (bailout != oldvalues[k] && j < 1) j = 1;
*/
   distest = uvalues[++k].uval.dval;
   if (distest != oldvalues[k]) j = 2;

   potparam[0] = uvalues[++k].uval.dval;
   if (potparam[0] != oldvalues[k] && j < 1) j = 1;

   potparam[1] = uvalues[++k].uval.dval;
   if (potparam[0] != 0.0 && potparam[1] != oldvalues[k] && j < 1) j = 1;

   potparam[2] = uvalues[++k].uval.dval;
   if (potparam[0] != 0.0 && potparam[2] != oldvalues[k] && j < 1) j = 1;

   strcpy(potfile,uvalues[++k].uval.sval);
   /* fullscreenprompt2 inserts blanks */
   if(potfile[0] == 32)
      potfile[0] = 0;

   return(j);
}

/*  **** the routines below this point are *not* prompting routines ***  */


/* --------------------------------------------------------------------- */

get_obsolete()			/* notify user of obsolete switches */
{
static char far obsmsg[]={"\
\n\n\nWell, the key you pressed *used* to be a valid command key,\n\
but it has since become obsolete.  Please use the following keys\n\
instead.\n\n\
  F1	  for help\n\n\
  x or X  to set a number of options\n\
\n\n"};
helpmessage(obsmsg);
printf(string004);
getakey();
}

/* --------------------------------------------------------------------- */

tab_display()			/* display the status of the current image */
{
int numfn;
extern double xxmin, xxmax, xx3rd, yymin, yymax, yy3rd;
extern long calctime, timer_start;
int i;
double Xctr, Yctr, Magnification;

if (calc_status < 0)		/* no active fractal image */
	return (0);		/* (no TAB on the credits screen) */

if (calc_status == 1)		/* next assumes CLK_TCK is 10^n, n>=2 */
	calctime += (clock() - timer_start) / (CLK_TCK/100);

setfortext();

printf("\n\nCurrent Fractal Type is: %-15s  ",
	fractalspecific[fractype].name[0] == '*' ?
	&fractalspecific[fractype].name[1] :
	fractalspecific[fractype].name
	);

switch (calc_status) {
case 0: printf("(parms chgd since generated)\n");
	break;
case 1: printf("(still being generated)\n");
	break;
case 2: printf("(interrupted, resumable)\n");
	break;
case 3: printf("(interrupted, non-resumable)\n");
	break;
case 4: printf("(image completed)\n");
}

showtrig(stdout);
if (fractype == FORMULA || fractype == FFORMULA)
	printf("Formula name: %s\n",FormName);

if (calc_status == 1 || calc_status == 2)
{
	if (fractalspecific[fractype].flags&INFCALC)
	    printf("Note: this type runs forever.\n");
	if (fractalspecific[fractype].flags&NORESUME || potfile[0] != 0)
	{
	    printf("Note: can't resume this type after interrupts other than <tab> and <F1>\n");
	    if (fractalspecific[fractype].flags&NORESUME == 0)
		printf("      (Due to generation of potential file.)\n");
	}
}

if (helpmode == HELPCYCLING)
	printf("%s  %s\n",
		"                                        ",
		"(You are in color-cycling mode)");

printf("\nCalculation time:%3ld:%02ld:%02ld.%02ld", calctime/360000,
	(calctime%360000)/6000, (calctime%6000)/100, calctime%100);
if (floatflag)
	printf("             Floating-point flag is activated");
printf("\n\n");

for(i=0;i<4;i++) {
	printf("   Param%1d = %12.9f ",i+1,param[i]);
	if ((i & 1) != 0)
		printf("\n");
	}

printf("\nCurrent Corners:        X                     Y\n");
printf(" top-left      %20.16f  %20.16f\n",xxmin,yymax);
printf(" bottom-right  %20.16f  %20.16f\n",xxmax,yymin);
adjust_corner(); /* make bottom left exact if very near exact */
if (cvtcentermag(&Xctr, &Yctr, &Magnification))
   printf("\nCenter: %20.16f  %20.16f Mag: %20.16f\n",Xctr,Yctr,Magnification);
else if (xxmin != xx3rd || yymin != yy3rd)
   printf(" bottom-left   %20.16f  %20.16f\n",xx3rd,yy3rd);

printf("\nCurrent Iteration Maximum = %d  Effective Bailout = %f\n",maxit,rqlim);

if (fractype == PLASMA)
	printf("\nCurrent 'rseed=' value is %d\n",rseed);

if(invert) {
	extern double f_radius,f_xcenter,f_ycenter;
	printf("\nCurrent Inversion parameters are: \n");
	printf("   radius = %12.9f \n",f_radius);
	printf("  xcenter = %12.9f \n",f_xcenter);
	printf("  ycenter = %12.9f \n",f_ycenter);
	}

printf("\n%s",string004);
getakey();
setforgraphics();

timer_start = clock(); /* tab display was "time out" */

return(0);
}

showtrig(FILE *batch) /* display active trig functions */
{
   int numfn;
   if(numfn=get_trig_num(fractalspecific[initfractype].flags))
   {
      int i;
      fprintf(batch, " function=%s",trigfn[trigndx[0]].name);
      i = 0;
      while(++i < numfn)
	 fprintf(batch, "/%s",trigfn[trigndx[i]].name);
      if(batch == stdout)
	 fprintf(batch, "\n");
   }
   return(0);
}

/* --------------------------------------------------------------------- */

ifsgetfile()		/* read in IFS parameters */
{
   FILE  *ifsfile;		/* IFS code file pointer */
   float localifs[IFSPARM];
   int i, j;

   /* read in IFS codes from file */
   if (ifsfilename[0] != 0) {
      findpath(ifsfilename, temp1);
      if ( (ifsfile = fopen( temp1,"r" )) != NULL ) {
	 i = -1;
	 while (fgets(temp1, 155, ifsfile) != NULL) {
	    if (++i >= NUMIFS) break;
	    sscanf(temp1," %f %f %f %f %f %f %f",
	       &localifs[0], &localifs[1], &localifs[2], &localifs[3],
	       &localifs[4], &localifs[5], &localifs[6]  );
	    for (j = 0; j < IFSPARM; j++) {
	       initifs[i][j]   = localifs[j];
	       initifs[i+1][j] = 0.0;
	       }
	    }
	 fclose(ifsfile);
	 }
      ifsfilename[0] = 0;
      }
}

/* --------------------------------------------------------------------- */

ifs3dgetfile()		/* read in 3D IFS parameters */
{
   FILE  *ifsfile;		/* IFS code file pointer */
   float localifs[IFS3DPARM];
   int i, j;

   /* read in IFS codes from file */
   if (ifs3dfilename[0] != 0) {
      findpath(ifs3dfilename, temp1);
      if ( (ifsfile = fopen( temp1,"r" )) != NULL ) {
	 i = -1;
	 while (fgets(temp1, 155, ifsfile) != NULL) {
	    if (++i >= NUMIFS) break;
	    sscanf(temp1," %f %f %f %f %f %f %f %f %f %f %f %f %f",
	       &localifs[ 0], &localifs[ 1], &localifs[ 2],
	       &localifs[ 3], &localifs[ 4], &localifs[ 5],
	       &localifs[ 6], &localifs[ 7], &localifs[ 8],
	       &localifs[ 9], &localifs[10], &localifs[11],
	       &localifs[12]
	       );
	    for (j = 0; j < IFS3DPARM; j++) {
	       initifs3d[i][j]	 = localifs[j];
	       initifs3d[i+1][j] = 0.0;
	       }
	    }
	 fclose(ifsfile);
	 }
      ifs3dfilename[0] = 0;
      }
}


/* --------------------------------------------------------------------- */

static int Distribution = 30, Offset = 0, Slope = 25;
static long con, distfactor;

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
   double values[3];
   extern int loadPalette;

   if(colors < 255) {
      buzzer(2);
      return(-1);
   }
   setfortext();		/* switch to text mode */
   fullscreen_setup();		/* set up for full-screen prompting */

   for (i = 0; i < 3; i++)
      values[i] = starfield_values[i];
   printf("\n\nPlease enter your Starfield Parameters:\n");
   if (fullscreen_prompt(10,3,starfield_prompts,values) < 0) {
      setforgraphics(); 	/* back to graphics */
      return(-1);
      }
   setforgraphics();		/* back to graphics */

   for (i = 0; i < 3; i++)
      starfield_values[i] = values[i];

   if (starfield_values[0] <   1.0) starfield_values[0] =   1.0;
   if (starfield_values[0] > 100.0) starfield_values[0] = 100.0;
   if (starfield_values[1] <   1.0) starfield_values[1] =   1.0;
   if (starfield_values[1] > 100.0) starfield_values[1] = 100.0;
   if (starfield_values[2] <   1.0) starfield_values[2] =   1.0;
   if (starfield_values[2] > 100.0) starfield_values[2] = 100.0;

   Distribution = (int)(starfield_values[0]);
   con	= (long)(((starfield_values[1]) / 100.0) * (1L << 16));
   Slope = (int)(starfield_values[2]);

   SetColorPaletteName(StarMap);
   if(!loadPalette)
      return(-1);
   spindac(0,1);		 /* load it, but don't spin */

   for(row = 0; row < ydots; row++) {
      for(col = 0; col < xdots; col++) {
	 if(check_key()) {
	    buzzer(1);
	    return(-1);
	    }
	 c = getcolor(col, row);
	 putcolor(col, row, GausianNumber(c, colors));
      }
   }
   buzzer(0);
   return(0);
}

int GausianNumber(int Probability, int Range) {
   int n, r;
   long Accum = 0, p;

   p = divide((long)Probability << 16, (long)Range << 16, 16);
   p = multiply(p, con, 16);
   p = multiply((long)Distribution << 16, p, 16);
   if(!(rand() % (Distribution - (int)(p >> 16) + 1))) {
      for(n = 0; n < Slope; n++)
	 Accum += rand();
      Accum /= Slope;
      r = (int)(multiply((long)Range << 15, Accum, 15) >> 14);
      r = r - Range;
      if(r < 0)
	 r = -r;
      return(Range - r + Offset);
   }
   return(Offset);
}


/* --------------------------------------------------------------------- */

extern char far argerrormessage[];	/* display argument error and die */

argerror(badarg)			/* oops.   couldn't decode this */
char *badarg;
{

setvideomode(3,0,0,0);
buzzer(2);
printf("\nOops.   I couldn't understand the argument '%s'\n\n",badarg);
helpmessage(argerrormessage);
exit(1);

}

/* --------------------------------------------------------------------- */

extern char far goodbyemessage[];

goodbye()			/* we done.  Bail out */
{
union REGS r;
setvideomode(3,0,0,0);
r.h.al = 3;
r.h.ah = 0;
int86(0x10, &r, &r);
helpmessage(goodbyemessage);
exit(0);
}

get_trig_num(int i) /* decode number of trig functions */
{
  i = i >> 6;
  i &= 7;
  return(i);
}
