/*
	Various routines that prompt for things.
	(Also, odds and ends that don't git anywhere else)
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>

#include "fractint.h"

struct fullscreenvalues
{
   char type;   /* 'd' or 's' */
   union
   {
      double dval;
      char   sval[16];
   } uval;
};
char funnyglasses_map_name[80];
extern char MAP_name[],temp[],temp1[];   /* temporary strings        */
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
extern  FILE    *dacfile;
extern          findpath();
extern  char    MAP_name[];
extern          ValidateLuts();  /* read the palette file */
int     mapset = 0;
extern  int     overlay3d;          /* 3D overlay flag: 0 = OFF */

extern	char	temp1[256];	/* temporary strings        */
extern	int	initmode;	/* initial video mode       */
extern	int	initfractype;	/* initial type set flag    */
extern	int	initbatch;	/* 1 if batch run (no kbd)  */
extern	int	init3d[20];	/* '3d=nn/nn/nn/...' values */
extern	int	initcorners;		/* initial flag: corners set*/
extern	double	initxmin,initxmax;	/* initial corner values    */
extern	double	initymin,initymax;	/* initial corner values    */
extern	double	initparam[4];		/* initial parameters       */
extern	int	display3d;	/* 3D display flag: 0 = OFF */
extern	int	invert;		/* non-zero if inversion active */
extern	double  inversion[3];   /* radius, xcenter, ycenter */
extern	long	fudge;		/* 2**fudgefactor	*/
extern	int	bitshift;	/* fudgefactor		*/
extern	double	param[4];	/* up to four parameters    */
extern	double	potparam[3];	/* three potential parameters*/
extern	int	fractype;	/* if == 0, use Mandelbrot  */
extern	char	floatflag;	/* floating-point fractals? */
extern	int	maxit;		/* try this many iterations */
extern	char	ifsfilename[80];    /* IFS code file */
extern	char	ifs3dfilename[80];  /* IFS 3D code file */
extern  char	preview;	/* 3D preview mode flag */
extern int decomp[];		/* decomposition parameters */
extern int rflag, rseed;	/* Plasma-Cloud seed values */
extern int fractype;		/* current fractal type */
extern int transparent[];	/* transparency values */
extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern unsigned char olddacbox[256][3]; /* backup copy of the Video-DAC */
extern	long	xmin, xmax, ymin, ymax;	/* screen corner values */
extern	int tab_status;		/* == 0 means no fractal on screen */
				/* == 1 means fractal in progress  */
				/* == 2 means fractal complete     */
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors;				/* maximum colors available */
extern int	row, col;


/* keep this in synch with fractals.c !!! */
#define IFS          26
#define IFS3D        27
#define LORENZ3D     65

/* Define command keys */

#define   PAGE_UP        1073
#define   PAGE_DOWN      1081
#define   LEFT_ARROW     1075
#define   RIGHT_ARROW    1077
#define   UP_ARROW       1072
#define   DOWN_ARROW     1080
#define   LEFT_ARROW_2   1115
#define   RIGHT_ARROW_2  1116
#define   UP_ARROW_2     1141
#define   DOWN_ARROW_2   1145
#define   HOME           1071
#define   END            1079
#define   ENTER          13
#define   ENTER_2        1013
#define   ESC            27
#define   SPACE          32
#define   F1             1059
#define   F2             1060
#define   F3             1061
#define   F4             1062
#define   F5             1063
#define   F6             1064
#define   F7             1065
#define   F8             1066
#define   F9             1067
#define   F10            1068

/* --------------------------------------------------------------------- */

unsigned video_seg;                /* Current video display segment */
int crtrows, crtcols;              /* Lines per page, columns/row */
int isatextmode;                   /* 1 if a text mode, 0 if graphics */

fullscreen_setup()		/* set up for full-screen prompting */
{
   int  video_mode;              /* current video mode */
   unsigned char far *lowptr;    /* Low memory pointer */

   /* setup stuff for video */
#ifdef __TURBOC__
   lowptr = MK_FP(0, 0x449);		/* Set for low memory */
#else
   FP_SEG(lowptr) = 0;                     /* Set for low memory */
   FP_OFF(lowptr) = 0x449;                 /* Get video mode */
#endif
   isatextmode = 1;                        /* assume text */
   if ((video_mode = *lowptr) >= 0 && video_mode < 4) 
      video_seg = 0xB800; /* Define video segment */
   else if (video_mode == 7)               /* If monochrome */
      video_seg = 0xB000;
   else {                                  /* graphics mode */
      video_seg = 0xB800;                  /* might be wrong, but not used */
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
	int numprompts,		/* there are this many prompts (max) */
	char *prompts[],	/* array of prompting pointers */
	double values[]		/* array of (initial) values */
	)
{
char temp2[24][15];		/* string value of answers go here */
int row, col, c, done, i, j, k;

if (numprompts <= 0)		/* ?? nothing to do! */
	return;

fullscreen_setup();		/* set up for full-screen prompting */

movecursor(25,81);		/* kill cursor */

putstring(22,10,7,"  Use the cursor keys to select values to change",0);
putstring(23,10,7,"  Type in any replacement values you wish to use",0);
putstring(24,10,7,"Press the ENTER when finished (or ESCAPE to back out)",0);

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
	putstring(startrow+i, col, 7,   temp2[i], 0);
      
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
	for (i = 0; i < numprompts; i++) 	/* fill in the results */
		values[i] = atof(temp2[i]);

return(done);
}

fullscreen_prompt2(		/* full-screen prompting routine */
	int startrow,		/* start on this row */
	int numprompts,		/* there are this many prompts (max) */
	char *prompts[],	/* array of prompting pointers */
	struct fullscreenvalues values[]		/* array of (initial) values */
	)
{
char temp2[24][15];		/* string value of answers go here */
int row, col, c, done, i, j, k;

if (numprompts <= 0)		/* ?? nothing to do! */
	return;

fullscreen_setup();		/* set up for full-screen prompting */

movecursor(25,81);		/* kill cursor */

putstring(22,10,7,"  Use the cursor keys to select values to change",0);
putstring(23,10,7,"  Type in any replacement values you wish to use",0);
putstring(24,10,7,"Press the ENTER when finished (or ESCAPE to back out)",0);

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
	putstring(startrow+i, col, 7,   temp2[i], 0);
      
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
	for (i = 0; i < numprompts; i++) 	/* fill in the results */
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
fullscreen_choice(int numchoices,         /* How many choices in list  */
                  char choices[1][50],    /* array of choices          */
                  int numcols,            /* how many columns          */
                  int startrow,           /* upper left corner of list */
                  int startcol,	          /* upper left corner of list */
                  int current)            /* start with this item      */
{
   char *s;
   int done;
   int colwidth;                 /* width of a column */
   int c;
   int crow, ccol, i, j, k, k1;
   int attribute;

   fullscreen_setup();			/* set up full-screen values */   
   colwidth = 22;                 /* width of a column */
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
compare(unsigned char *i, unsigned char *j)
{
   return(strcmp(fractalspecific[(int)*i].name,fractalspecific[(int)*j].name));
}

/* --------------------------------------------------------------------- */

get_fracttype(int t)		/* prompt for and select fractal type */
{
   char *s;
   int oldhelpmode;
   int done;
   int numtypes;                 /* length of list    */
   int numcols;                  /* number of columns */
   int colwidth;                 /* width of a column */
   int startrow, startcol;       /* upper left corner of list */
   int c;
   int crow, ccol, i, j, k, k1;
   int attribute;
   
   unsigned char onthelist[256];	/* list of REAL name locations */

restart:

   setvideomode(3,0,0,0);		/* switch to text mode */

   fullscreen_setup();			/* set up full-screen values */   
   
   /* setup context sensitive help */
   oldhelpmode = helpmode;
   helpmode = HELPFRACTALS;

   /* set up details of how type list is displayed on the screen */
   numcols  =  5;                 /* number of columns */
   colwidth = 16;                 /* width of a column */
   startrow =  5;                 /* upper left corner of list */
   startcol =  1;                 /* upper left corner of list */

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
      "(or press the 'h' key for a short description of each fractal type)",
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
      while (! keypressed());
      c = getakey();
      
      /* write type back to normal video */
      putstring(crow,ccol, 7,fractalspecific[onthelist[i]].name,0);
      
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
         continue; 
      }
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
	int oldfractype		/* previous fractal type */
	)
{
int numparams;
int i;

initfractype = newfractype;

if (strcmp(fractalspecific[initfractype].param[3],
	"Imaginary portion of p2") == 0) 
	if (get_formula_name() < 0)	/* scan for the formula name */
		return(-1);
	
 for ( i = 0; i < 4; i++)
         initparam[i] = fractalspecific[initfractype].paramvalue[i];

for (numparams = 0; numparams < 4; numparams++) {
	if (fractalspecific[initfractype].param[numparams][0] == 0) break;
	if (numparams == 0) {
		printf("\n\n Select your options for fractal type %s",
			fractalspecific[initfractype].name[0] != '*' ?
			fractalspecific[initfractype].name           :
			&fractalspecific[initfractype].name[1]       );
 		printf("\n\n Please enter any parameters that apply \n\n");
		}
	}

 if (fullscreen_prompt(10,numparams,
         fractalspecific[initfractype].param, initparam) < 0)
                 return(-1);

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
extern char FormName[];		/* Name of the Formula (if not null) */

get_formula_name()	/* get the fractal formula name */
{
   int numcols;                  /* number of columns */
   int startrow, startcol;       /* upper left corner of list */
   char choices[21][50];
   char fullfilename[200];	/* Full file name */
   int numformulas, i;
   FILE *File;

   FormName[0] = 0;		/* start by declaring failure */

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
     printf("\n\n...Press any key to continue..");
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
         if (numformulas >= 20) break;
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
   numcols  =  3;                 /* number of columns */
   startrow =  5;                 /* upper left corner of list */
   startcol = 10;                 /* upper left corner of list */

   if ((i = fullscreen_choice(numformulas, choices,numcols,startrow,startcol,0)) >= 0)
      strcpy(FormName, choices[i]);
   
   if (RunForm(FormName)) {
       FormName[0] = 0;		/* declare failure */
       buzzer(2);
       printf("\n\n...Press any key to continue..");
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

    memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
    setvideomode(3,0,0,0);                 /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */

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
   "Radius of inversion (-1 for auto calculate)", 
   "X Center inversion  (0  for auto calculate)",
   "Y Center inversion  (0  for auto calculate)",
   ""
   };

   invert = 0;
   inversion[0] = inversion[1] = inversion[2] = 0;
   if(fractalspecific[initfractype].calctype != StandardFractal &&
      fractalspecific[initfractype].calctype != calcmand) 
      return(0);

    memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
    setvideomode(3,0,0,0);                 /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */

   printf("\n\n Please enter inversion parameters that apply.  Note\n");
   printf("  that the inversion option requires a fixed radius and \n");
   printf("  center for zooming to make sense - if you want to zoom,\n");
   printf("  do not use default values, but specify radius and center\n");

   if (fullscreen_prompt(10,3,invert_prompts,inversion) < 0)
      return(-1);
   
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
    memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
    setvideomode(3,0,0,0);                 /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */

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

get_3d_params()		/* prompt for 3D parameters */
{
   int numcols;                 /* number of columns */
   int startrow;                /* upper left corner of list */
   int startcol;                /* upper left corner of list */
   char choices[8][50];
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
   strncpy(choices[k++],"make a surface grid"                   ,50);
   strncpy(choices[k++],"just draw the points"                  ,50);
   strncpy(choices[k++],"connect the dots (wire frame)"         ,50);
   strncpy(choices[k++],"surface fill (colors interpolated)"    ,50);
   strncpy(choices[k++],"surface fill (colors not interpolated)",50);
   strncpy(choices[k++],"solid fill (bars up from \"ground\")"  ,50);
   if(SPHERE)
   {
   strncpy(choices[k++],"light source"                          ,50);
   }
   else
   {
   strncpy(choices[k++],"light source before transformation)"   ,50);
   strncpy(choices[k++],"light source after transformation)"    ,50);
   } 
   numcols  =  1;                 /* number of columns */
   startrow = 12;                 /* upper left corner of list */
   startcol = 23;                 /* upper left corner of list */

   s = "Select Fill Type";
   putstring(startrow-2,(crtcols-strlen(s))/2,7,s,0);
   putstring(22,10,7,"                                                   ",0);
   putstring(23,10,7,"                                                   ",0);
   putstring(24,10,7,"                                                   ",0);
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
   values[0]   = XROT      ;
   values[1]   = YROT      ;
   values[2]   = ZROT      ;
   values[3]   = XSCALE    ;
   values[4]   = YSCALE    ;
   k = 5;
   prompts3d[k]= "Surface Roughness scaling factor in pct"; 
   values[k++] = ROUGH     ;   

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

   if(ILLUMINE)
   {
      prompts3d[k]= "X value light vector";
      values[k++] = XLIGHT    ;

      prompts3d[k]= "Y value light vector";
      values[k++] = YLIGHT    ;

      prompts3d[k]= "Z value light vector";
      values[k++] = ZLIGHT    ;

      prompts3d[k]= "Light Source Smoothing Factor";
      values[k++] = LIGHTAVG  ;
      
      prompts3d[k] = "";
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
   if(ILLUMINE)
   {
      XLIGHT   = values[k++];
      YLIGHT   = values[k++];
      ZLIGHT   = values[k++];
      LIGHTAVG = values[k++];
   }
return(0);
}

check_mapfile()
{
    char temp[81];
    while ((FILLTYPE >= 5) || overlay3d || (mapset == 1) || glassestype)
    {
        setclear();
        movecursor(5,5);
        if (mapset && ((glassestype==0) || (glassestype==3)))
        {
            printf("\n\nA replacement .MAP file, %s, has been",MAP_name);
            printf(" previously selected.\n");
            printf("Press \"CR\" to continue using it.\n");
            printf("Enter \"*\" to use the palette from the image about ");
            printf("to be loaded.\n  ==> ");
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
            movecursor(5,5);
            printf("\n\nEnter name of .MAP file to use, or \n");
            printf("Press \"CR\" or enter \"*\" to use palette from the ");
            printf("image about");
            printf(" to be loaded.\n  ==> ");
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
            printf ("Hit any key to continue");
            getch();
            setclear();
            continue;
        }
        /* File has been opened OK */
        ValidateLuts(dacfile);  /* read the palette file to make sure its ok */
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
      if(fractype==IFS3D || fractype==LORENZ3D)
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
   xadjust         =  uvalues[k++].uval.dval;
   red_crop_left   =  uvalues[k++].uval.dval;
   red_crop_right  =  uvalues[k++].uval.dval;
   blue_crop_left  =  uvalues[k++].uval.dval;
   blue_crop_right =  uvalues[k++].uval.dval;
   red_bright      =  uvalues[k++].uval.dval;
   blue_bright     =  uvalues[k++].uval.dval;
 
   if(glassestype != 3)
      strcpy(funnyglasses_map_name,uvalues[k].uval.sval);
   return(0);
}

/* --------------------------------------------------------------------- */

get_ifs_params()		/* prompt for IFS params */
{
char *s;
int numcols;                  /* number of columns */
int startrow, startcol;       /* upper left corner of list */
char choices[4][50];
char *filename;
float far *initarray;
static char ifstype = '2';
int totrows, totcols;
int i, j, numlines;
FILE *tempfile;

setfortext();			/* switch to text mode */

memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
setvideomode(3,0,0,0);                 /* clear the screen entirely */
memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */


ifsgetfile();			/* read in any "IFS=" file */

ifs3dgetfile();			/* read in any "IFS3D=" file */

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
else if (tab_status != 0) {   /* there's a fractal image active */
   ifstype = 't';
   get_ifs3d_params();
   return;
   }
   strcpy(choices[0],"2D IFS Codes");
   strcpy(choices[1],"3D IFS Codes");
   strcpy(choices[2],"3D Transform Parameters");

   /* set up details of how type list is displayed on the screen */
   numcols  =  1;                 /* number of columns */
   startrow =  5;                 /* upper left corner of list */
   startcol = 30;                 /* upper left corner of list */
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
       return;
       break;
     default:
       return;
       break;
    }
if (ifstype == '3') {
   filename = ifs3dfilename;
   initarray = &initifs3d[0][0];
   totrows = NUMIFS;
   totcols = IFS3DPARM;
   }

for ( ;; ) {

    for (numlines = 0; numlines < totrows; numlines++)    /* find the first zero entry */
        if (initarray[(numlines * totcols) + totcols - 1] <= 0.0001) break;

    memcpy(olddacbox,dacbox,256*3);        /* save the DAC */
    setvideomode(3,0,0,0);                 /* clear the screen entirely */
    memcpy(dacbox,olddacbox,256*3);        /* restore the DAC */

    printf(" Your current IFS Parameters are: \n\n");
    printf("#   a     b     c     d     e     f");
    if (ifstype == '3')
         printf("     g     h     i     j     k     l");
    printf("    prob \n\n");

    for (i = 0; i < numlines; i++) {
        printf("%1d", i+1);
        for (j = 0; j < totcols; j++)
            printf("%6.2f", (float )initarray[(i*totcols)+j]);
        printf("\n");
        }

    printf("\n\n Enter the number of the line you want to edit\n");
    printf("  (or RESTORE to start from another (.IFS) file, or SAVE to\n");
    if(ifstype == '3')
    {
       printf("   save your edits in an (.IFS) file, or TRANSFORM to alter\n");
       printf("   3D transformation values, or <ENTER> to end) ==> ");
    }
    else
       printf("   save your edits in an (.IFS) file, or <ENTER> to end) ==> ");
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

/*  **** the routines below this point are *not* prompting routines ***  */

/* --------------------------------------------------------------------- */

tab_display()			/* display the status of the current image */
{
extern double xxmin, xxmax, yymin, yymax;
int i;

if (tab_status == 0)		/* no active fractal image */
	return (0);		/* (no TAB on the credits screen) */
	
setfortext();

printf("\n\nCurrent Fractal Type is: %-15s  %s\n", 
        fractalspecific[fractype].name[0] == '*' ?
        &fractalspecific[fractype].name[1] :
        fractalspecific[fractype].name,
        tab_status == 1 ?
        "(still being generated)" :
        "(image completed / interrupted)"
        );

if (helpmode == HELPCYCLING)
	printf("%s  %s\n",
		"                                        ",
		"(You are in color-cycling mode)");

printf("\n");

for(i=0;i<4;i++)
	printf("   Param%1d = %12.9f \n",i+1,param[i]);

printf("\nCurrent Screen Corners are: \n\n");
printf("     Xmin = %20.16f \n",xxmin);
printf("     Xmax = %20.16f \n",xxmax);
printf("     Ymin = %20.16f \n",yymin);
printf("     Ymax = %20.16f \n",yymax);

printf("\nCurrent Iteration Maximum = %d\n",maxit);

if (fractype == PLASMA)
	printf("\nCurrent 'rseed=' value is %d\n",rseed);

if(invert) {
	extern double f_radius,f_xcenter,f_ycenter;
	printf("\nCurrent Inversion parameters are: \n");
	printf("   radius = %12.9f \n",f_radius);
	printf("  xcenter = %12.9f \n",f_xcenter);
	printf("  ycenter = %12.9f \n",f_ycenter);
	}

if (floatflag)
	printf("\nFloating-point flag is activated\n");
	
printf("\nPress any key to continue...");
getakey();
setforgraphics();

return(0);
}

/* --------------------------------------------------------------------- */

ifsgetfile()		/* read in IFS parameters */
{
   FILE	 *ifsfile;		/* IFS code file pointer */
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
   FILE	 *ifsfile;		/* IFS code file pointer */
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
               initifs3d[i][j]   = localifs[j];
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
   int i, c;
   double values[3];

   if(colors < 255)
      return(-1);
   setfortext();		/* switch to text mode */
   fullscreen_setup();		/* set up for full-screen prompting */

   for (i = 0; i < 3; i++)
      values[i] = starfield_values[i];
   printf("\n\nPlease enter your Starfield Parameters:\n");
   if (fullscreen_prompt(10,3,starfield_prompts,values) < 0) {
      setforgraphics();         /* back to graphics */
      return(-1);
      }
   setforgraphics();            /* back to graphics */
   for (i = 0; i < 3; i++)
      starfield_values[i] = values[i];

   if (starfield_values[0] <   1.0) starfield_values[0] =   1.0;
   if (starfield_values[0] > 100.0) starfield_values[0] = 100.0;
   if (starfield_values[1] <   1.0) starfield_values[1] =   1.0;
   if (starfield_values[1] > 100.0) starfield_values[1] = 100.0;
   if (starfield_values[2] <   1.0) starfield_values[2] =   1.0;
   if (starfield_values[2] > 100.0) starfield_values[2] = 100.0;

   Distribution = (int)(starfield_values[0]);
   con  = (long)(((starfield_values[1]) / 100.0) * (1L << 16));
   Slope = (int)(starfield_values[2]);
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
setvideomode(3,0,0,0);
helpmessage(goodbyemessage);
exit(0);
}
