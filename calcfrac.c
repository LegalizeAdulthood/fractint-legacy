/*

FRACTALS.C and CALCFRAC.C actually calculate the fractal images (well,
SOMEBODY had to do it!).  The modules are set up so that all logic that
is independent of any fractal-specific code is in CALCFRAC.C, and the
code that IS fractal-specific is in FRACTALS.C. Original author Tim Wegner,
but just about ALL the authors have contributed SOME code to this routine
at one time or another, or contributed to one of the many massive
restructurings.

   -------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <dos.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include "fractint.h"
#include "mpmath.h"
#include "targa_lc.h"

extern unsigned int decoderline[];
extern int overflow;
extern int bailout;
extern char far plasmamessage[];
long lmagnitud, llimit, llimit2, lclosenuff,l16triglim;
struct complex init,tmp,old,new,saved;
extern struct lcomplex lold;
extern double tempsqrx,tempsqry;
extern long ltempsqrx,ltempsqry;
extern int biomorph;
extern struct lcomplex linit;
extern int basin;
extern int cpu;

int color, oldcolor, oldmax, oldmax10, row, col, passes;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
extern double far *dx0, far *dy0;
long XXOne, FgOne, FgTwo, LowerLimit;

extern int LogFlag;
extern char far *LogTable;

void (*plot)() = putcolor;
extern int bound_trace_main();

extern double inversion[];	    /* inversion radius, f_xcenter, f_ycenter */
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors; 			/* maximum colors available */
extern int	inside; 			/* "inside" color to use    */
extern int	outside; 			/* "outside" color to use   */
double min_orbit;		/* orbit value closest to origin */
int    min_index;				/* iteration of min_orbit */
extern int	maxit;				/* try this many iterations */
extern int	fractype;			/* fractal type */
extern int	numpasses;			/* 0 = 1 pass, 1 = double pass */
extern int	solidguessing;			/* 1 if solid-guessing */
extern int	debugflag;			/* for debugging purposes */
extern int	timerflag;			/* ditto */
extern	int	diskvideo;			/* for disk-video klooges */

extern int rflag, rseed;
extern int decomp[];

extern double	param[];		/* parameters */
extern double	potparam[];		/* potential parameters */
extern long	far *lx0, far *ly0;		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	fudge;				/* fudge factor (2**n) */
extern int	bitshift;			/* bit shift for fudge */
extern char potfile[];		/* potential filename */


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

extern	  int extraseg;

/* These are local but I don't want to pass them as parameters */
static	  unsigned char top;   /* flag to indicate top of calcfract */
static	  unsigned char c;
static	  int i;
static	  int kbdcount;

extern double xxmin,xxmax,yymin,yymax; /* corners */
struct complex lambda;
double deltaX, deltaY;
double magnitude, rqlim, rqlim2;
extern int bof_pp60_61;
int XXdots, YYdots; /* local dots */
extern struct complex parm;
int (*calctype)();
double closenuff;
int pixelpi; /* value of pi in pixels */
FILE *fp_pot;
int potflag; /* tells if continuous potential on  */
unsigned long lm;		/* magnitude limit (CALCMAND) */

/* ORBIT variables */
int	show_orbit;			/* flag to turn on and off */
int	orbit_ptr;			/* pointer into save_orbit array */
static int  far *save_orbit;		/* array to save orbit values */
static int orbit_color=15;		/* XOR color */

int	ixstart, ixstop, iystart, iystop;	/* (for CALCMAND) start, stop here */
int	symmetry;			/* symmetry flag for calcmand() */
int	guessing;			/* solid-guessing flag for calcmand() */

extern	int	integerfractal; 	/* TRUE if fractal uses integer math */

/* static vars for solidguess & its subroutines */
static int maxblock,halfblock;
static int guessplot;			/* paint 1st pass row at a time?   */
static int right_guess,bottom_guess;
#define maxyblk 7    /* maxxblk*maxyblk*2 <= 4096, the size of "prefix" */
#define maxxblk 202  /* each maxnblk is oversize by 2 for a "border" */
/* next has a skip bit for each maxblock unit;
   1st pass sets bit  [1]... off only if block's contents guessed;
   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
   bits are numbered [..][y/16+1][x+1]&(1<<(y&15)) */
extern unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
/* size of next puts a limit of 2048 pixels across on solid guessing logic */
extern char dstack[4096];		/* common temp, two out_line calls */
extern int rowcount;			/* out_line parameter		   */

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

extern struct lcomplex lnew,llambda;
int periodicitycheck = 1;

extern double floatmin, floatmax;

extern int StandardFractal();
int boundarytraceflag = 0;

extern int display3d;

calcfract()
{
   int oldnumpasses, oldsolidguessing, oldperiodicitycheck;
   oldnumpasses        = numpasses;	     /* save these */
   oldsolidguessing    = solidguessing;
   oldperiodicitycheck = periodicitycheck;

   display3d = 0;

   orbit_color = 15;
   top	   = 1;
   if(fp_pot)
   {
      fclose(fp_pot);
      fp_pot = NULL;
   }

   if (invert)
   {
      floatmin = FLT_MIN;
      floatmax = FLT_MAX;
   }
   ixstart = 0; 			/* default values to disable .. */
   iystart = 0; 			/*   solid guessing		*/
   ixstop = xdots-1;
   iystop = ydots-1;
   guessing = 0;

   basin = 0;

   if (integerfractal) 
   {
      deltaX = (double)lx0[	 1] / fudge - xxmin;
      deltaY = yymax - (double)ly0[	 1] / fudge;
   } 
   else 
   {
      deltaX = dx0[	 1] - xxmin;
      deltaY = yymax - dy0[	 1];
   }

   parm.x  = param[0];
   parm.y  = param[1];

   if(fabs(potparam[0]) > 0.0 && !boundarytraceflag)
      potflag = 1;
   else
      potflag = 0;

   if (LogFlag)
      ChkLogFlag();

   lm = 4;				/* CALCMAND magnitude limit */
   lm = lm << bitshift;
   /*
      Continuous potential override (unused at the moment)
         if (potparam[2] > 4 && potparam[2] < 64)
            lm = potparam[2]*fudge;
   */
   /* set modulus bailout value if 3rd potparam set */

   if (fabs(potparam[2]) > 0.0)
      rqlim = potparam[2];
   else if (decomp[0] > 0 && decomp[1] > 0)
      rqlim = (double)decomp[1];
   else if(bailout)    /* user input bailout */
      rqlim = bailout;
   else if(biomorph != -1)   /* biomorph benefits from larger bailout */
      rqlim = 100;
   else
      rqlim = fractalspecific[fractype].orbit_bailout;

   /* ORBIT stuff */
   save_orbit = (int far *)(dx0 + 2*MAXPIXELS);
   show_orbit = 0;
   orbit_ptr = 0;
   if(colors < 16)
      orbit_color = 1;

   if (integerfractal)		/* the bailout limit can't be too high here */
      if (rqlim > 127.0) rqlim = 127.0;
   if(inversion[0] != 0.0)
   {
      f_radius	  = inversion[0];
      f_xcenter   = inversion[1];
      f_ycenter   = inversion[2];

      if (inversion[0] < 0.0)  /*  auto calc radius 1/6 screen */
         inversion[0] = f_radius = (min(xxmax - xxmin,yymax - yymin)) / 6.0;

      if (invert < 2)  /* xcenter not already set */
      {
         inversion[1] = f_xcenter = (xxmin + xxmax) / 2.0;
         if (fabs(f_xcenter) < (xxmax-xxmin) / 100)
            inversion[1] = f_xcenter = 0.0;
      }

      if (invert < 3)  /* ycenter not already set */
      {
         inversion[2] = f_ycenter = (yymin + yymax) / 2.0;
         if (fabs(f_ycenter) < (yymax-yymin) / 100)
            inversion[2] = f_ycenter = 0.0;
      }

      invert = 3; /* so values will not be changed if we come back */
   }

   if(fractalspecific[fractype].symmetry != SETUP_SYM)
      setsymmetry(fractalspecific[fractype].symmetry);

   if (potfile[0] != 0 || boundarytraceflag)			/* potential file? */
   {
      numpasses = 0;				/* disable dual-pass */
      solidguessing = 0;			/* disable solid-guessing */
   }

   closenuff = ((delx < dely ? delx : dely) >> abs(periodicitycheck)); /* for periodicity checking */
   closenuff /= fudge;
   rqlim2 = sqrt(rqlim);
   if (integerfractal)		/* for integer routines (lambda) */
   {
      llambda.x = parm.x * fudge;		/* real portion of Lambda */
      llambda.y = parm.y * fudge;		/* imaginary portion of Lambda */
      llimit = rqlim * fudge;		/* stop if magnitude exceeds this */
      if (llimit <= 0) llimit = 0x7fffffff; /* klooge for integer math */
      llimit2 = rqlim2 * fudge; /* stop if magnitude exceeds this */
      lclosenuff = closenuff * fudge;	/* "close enough" value */
      l16triglim = 8L<<16;   /* domain limit of fast trig functions */
   }

   calctype = fractalspecific[fractype].calctype;		/* assume a standard fractal type */

   /* per_image routine is run here */
   if (fractalspecific[fractype].per_image())  /* a stand-alone type? */
   {
      if(fractalspecific[fractype].symmetry == SETUP_SYM)
         setsymmetry(symmetry);
      /* fractal routine (usually StandardFractal) is run here */
      if(boundarytraceflag)
         timer(bound_trace_main,0);
      else if(solidguessing)	     /* use std solid-guessing? */
         timer(solidguess,0);
      else
         timer(calctype,0);
   }

   potfile[0] = 0;
   solidguessing    = oldsolidguessing;
   numpasses	    = oldnumpasses;
   periodicitycheck = oldperiodicitycheck;
   if (! check_key())
      return(0);
   else
      return(1);
}

test()
{
   teststart();
   for (passes=0; passes <= numpasses ; passes++)
      for (row = passes; row < YYdots; row=row+1+numpasses)
      {
         register int col;
         init.y = dy0[row];
         for (col = 0; col < XXdots; col++)       /* look at each point on screen */
         {
            register color;
            init.x = dx0[col];
            if(check_key())
            {
               testend();
               return(-1);
            }
            color = testpt(init.x,init.y,parm.x,parm.y,maxit,inside);
            (*plot)(col,row,color);
            if(numpasses && (passes == 0))
               (*plot)(col,row+1,color);
         }
      }
   testend();
   return(0);
}

/* BTM function to return the color value for a pixel.	*/

static int far *LeftX  = (int far *)NULL;
static int far *RightX = (int far *)NULL;
static int repeats;

int calc_xy(int mx, int my)
{

   color = getcolor(mx,my); /* see if pixel is black */
   if (color!=0)	    /* pixel is NOT black so we must have already */
   {			  /* calculated its color, so lets skip it	*/
      repeats++;	     /* count successive easy ones */
      return(color);
   }
   repeats = 0; 	/* we'll have to work for this one wo reset counter */

   ixstart = ixstop = mx;
   iystart = iystop = my;
   if ((color=(*calctype)())==0)
   {
      color = inside;
      (*plot)(mx,my,color);
   }
   return(color);
} /* calc_xy function of BTM code */


boundary_trace(int C, int R)   /* BTM main function */
{
   enum
   {
      North, East, South, West
   } Dir;
   int modeON, C_first, bcolor, low_row, iters, gcolor;
   low_row = R;
   modeON = 0;
   Dir = South;
   bcolor = color;
   C_first = C;
   iters = 0;
   repeats = 0;

   /* main loop of BTM inside this loop the boundary is traced on screen! */
   do
   {
      if(--kbdcount<=0)
      {
         if(check_key())
            return(-1);
         kbdcount=(cpu==386) ? 80 : 30;
      }
      iters++;		/* count times thru loop */

      if (C > RightX[R]) 
         RightX[R] = C; /* maintain left and right limits */
      else
         if (R==low_row)
            if (C<=C_first) /* works 99.9% of time! */
               break;
      if (C < LeftX[R])  
         LeftX[R]  = C; /* to aid in filling polygon later */
      switch (Dir)
      {
      case North :
         if (R > 0)
            if(calc_xy(C,R-1)==bcolor)
            {
               R--;
               if (C > 0)
                  if(calc_xy(C-1,R)==bcolor)
                  {
                     C--;
                     Dir = West;
                  }
               break;
            }
         Dir = East;
         break;
      case East :
         if (C < XXdots-1)
            if(calc_xy(C+1,R)==bcolor)
            {
               C++;
               if (R > 0)
                  if(calc_xy(C,R-1)==bcolor)
                  {
                     R--;
                     Dir = North;
                  }
               break;
            }
         Dir = South;
         break;
      case South :
         if (R < YYdots-1)
            if(calc_xy(C,R+1)==bcolor)
            {
               R++;
               if (C < XXdots-1)
                  if(calc_xy(C+1,R)==bcolor)
                  {
                     C++;
                     Dir = East;
                  }
               break;
            }
         Dir = West;
         break;
      case West:
         if (C > 0)
            if(calc_xy(C-1,R)==bcolor)
            {
               C--;
               if (R < YYdots-1)
                  if(calc_xy(C,R+1)== bcolor)
                  {
                     R++;
                     Dir = South;
                  }
               break;
            }
         Dir = North;
         break;
      } /* case */
   }
   while (repeats<xdots); /* if the line above fails to stop looping around
                                          the polygon this trick surely will! */
   if (iters<4)
   {
      LeftX[low_row] = 3000; 
      RightX[low_row] = -3000;
      return(0);  /* no need to fill a polygon of 3 points! */
   }

   /* Avoid tracing around whole fractal object */
   if (YYdots==ydots)
      if (LeftX[0]==0)
         if (RightX[0]==XXdots-1)
            if (LeftX[YYdots-1]==0)
               if (RightX[YYdots-1]==XXdots-1)
               {
                  /* clean up in this RARE case or next fills will fail! */
                  for (low_row = 0; low_row <= ydots-1; low_row++)
                  {
                     LeftX[low_row] = 3000;
                     RightX[low_row] = -3000;
                  }
                  return(0);
               }
   /* fill in the traced polygon, simple but it works darn well */
   C = 0;
   for (R = low_row; R<YYdots-1; R++)
      if (RightX[R] != -3000)
      {
         if((kbdcount-=2)<=0)
         {
            if(check_key())
               return(-1);
            kbdcount=(cpu==386) ? 80 : 30;
         }
         if(debugflag==1946)
            C = fillseg1(LeftX[R], RightX[R],R, bcolor);
         else
            C = fillseg(LeftX[R], RightX[R],R, bcolor);
         
         LeftX[R]  =  3000; 
         RightX[R] = -3000; /* reset array element */
      }
      else if (C!=0) /* this is why C = 0 above! */
         return(0);
   return(0);
} /* BTM function */
fillseg1(int LeftX, int RightX, int R,  int bcolor)
{
   register modeON, C;
   int  gcolor;
   modeON = 0;
   for (C = LeftX; C <= RightX; C++)
   {
      gcolor=getcolor(C,R);
      if (modeON!=0 && gcolor==0)
         (*plot)(C,R,bcolor);
      else
      {
         if (gcolor==bcolor) /* TW saved a getcolor here */
            modeON = 1;
         else
            modeON = 0;
      }
   }
   return(C);
}

fillseg(int LeftX, int RightX, int R,  int bcolor)
{
   unsigned char *forwards;
   unsigned char *backwards;
   register modeON, C;
   int  gcolor;
   modeON = 0;
   forwards  = (unsigned char *)decoderline;
   backwards = (unsigned char *)dstack;
   modeON = 0;
   get_line(R,LeftX,RightX,forwards); 
   for (C = LeftX; C <= RightX; C++)
   {
      gcolor=forwards[C-LeftX];
      if (modeON!=0 && gcolor==0)
         forwards[C-LeftX]=bcolor;
      else
      {
         if (gcolor==bcolor) /* TW saved a getcolor here */
            modeON = 1;
         else
            modeON = 0;
      }
   }
   if(plot==putcolor) /* no symmetry! easy! */
      put_line(R,LeftX,RightX,forwards); 
   else if(plot==symplot2) /* X-axis symmetry */
   {
      put_line(R,        LeftX,RightX,forwards); 
      put_line(ydots-R-1,LeftX,RightX,forwards); 
   }
   else if(plot==symplot2J) /* Origin symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,        LeftX,         RightX,       forwards); 
      put_line(ydots-R-1,xdots-RightX-1,xdots-LeftX-1,backwards); 
   }  
   else if(plot==symplot2Y) /* Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,        LeftX,         RightX,       forwards); 
      put_line(R,        xdots-RightX-1,xdots-LeftX-1,backwards); 
   }  
   else if(plot==symplot4) /* X-axis and Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,        LeftX,         RightX,       forwards); 
      put_line(R,        xdots-RightX-1,xdots-LeftX-1,backwards); 
      put_line(ydots-R-1,LeftX,         RightX,       forwards); 
      put_line(ydots-R-1,xdots-RightX-1,xdots-LeftX-1,backwards); 
   }
   else  /* the other symmetry types are on their own! */
   {
      int i;
      for(i=LeftX;i<=RightX;i++)
         (*plot)(i,R,forwards[i-LeftX]);
   }  
   return(C);
}

/* copy a string backwards for symmetry functions */
reverse_string(char *t, char *s, int len)
{
   register i;
   len--;
   for(i=0;i<=len;i++)
      t[i] = s[len-i];
   return(0);
}

bound_trace_main()
{
   int srow, scol;
   long maxrow;
   maxrow = ((long)ydots)*sizeof(int);

   if((LeftX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
      return(-1);
   else if((RightX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
   {
      farmemfree((char far *)LeftX);
      return(-1);
   }

   /* This is set in solidguess() */
   numpasses = 0;	   /* disable dual-pass */
   passes = 0;
   potflag = 0;

   /* note we had to change the loop counters to temp vars! */
   for (srow = 0; srow <= ydots-1; srow++)
   {
      LeftX[srow] = 3000;
      RightX[srow] = -3000;
   }

   /* call calc routine once to get its vars initialized */
   ixstart = ixstop = iystart = iystop = 0;
   guessing = 1;
   (*calctype)();
   /* set guessing=3 for the rest to not reset periodicity control vars */
   guessing = 3;

   for (srow = 0; srow < YYdots; srow++)
   {
      oldcolor = 1;
      oldmax = min(maxit, 250);
      oldmax10 = oldmax - 10;

      for (scol = 0; scol < XXdots; scol++)
      {
         if(--kbdcount<=0)
         {
            if(check_key())
            {
               farmemfree((char far *)LeftX);
               farmemfree((char far *)RightX);
               return(-1);
            }
            kbdcount=(cpu==386) ? 80 : 30;
         }

         /* BTM Hook! */
         color = getcolor(scol,srow);
         /* if pixel is BLACK (0) then we haven't done it yet!
                           		     so first calculate its color and call the routine
                           		     that will try and trace a polygon if one exists */
         if (color==0)
         {
            color = calc_xy(scol, srow);
            boundary_trace(scol, srow); /* go trace boundary! WHOOOOOOO! */
         }
      }
   }
   farmemfree((char far *)LeftX);
   farmemfree((char far *)RightX);
   return(0);
} /* end of hacked Standard Fractal */

StandardFractal()
{
   int caught_a_cycle;
   int savedand, savedincr;		/* for periodicity checking */
   struct lcomplex lsaved;

   if(guessing<2) kbdcount=(cpu==386) ? 80 : 30;

   for (passes=0; passes <= numpasses ; passes++)
   {
      /* if numpasses == 1, on first pass skip rows */
      for (row = iystart; row <= iystop; row = row + 1 + numpasses - passes)
      {
         /* in guessing 3, inherit oldcolor etc */
         if(guessing!=3)
         {
            oldcolor = 1;
            oldmax = min(maxit, 250);
            oldmax10 = oldmax - 10;
         }

         /* really fractal specific, but we'll leave it here */
         if (! integerfractal)
            init.y = dy0[row];
         else
            linit.y = ly0[row];

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            if (! integerfractal)
            {
               saved.x = 0;
               saved.y = 0;
            }
            else
            {
               lsaved.x = 0;
               lsaved.y = 0;
            }
            orbit_ptr = 0;
            color = 0;
            caught_a_cycle = 1;
            savedand = 1;		/* begin checking every other cycle */
            savedincr = 1;		/* start checking the very first time */

            if(bof_pp60_61)
            {
               magnitude = lmagnitud = 0;
               min_orbit = 100000.0;
            }
            overflow=0;       /* reset integer math overflow flag */
            fractalspecific[fractype].per_pixel(); /* initialize the calculations */
            while (++color < maxit)
            {
               /* calculation of one orbit goes here */
               /* input in "old" -- output in "new" */

               if(fractalspecific[fractype].orbitcalc())
                  break;

               if(bof_pp60_61)
               {
                  if(integerfractal)
                  {
                     if(lmagnitud == 0)
                        lmagnitud = multiply(lnew.x,lnew.x,(int)fudge) +
                            multiply(lnew.y,lnew.y,(int)fudge);
                     magnitude = lmagnitud;
                     magnitude = magnitude/fudge;
                  }
                  else if(magnitude == 0.0)
                     magnitude = sqr(new.x) + sqr(new.y);
                  if(magnitude < min_orbit)
                  {
                     min_orbit = magnitude;
                     min_index = color +1;
                  }
               }
               if(show_orbit)
               {
                  if (! integerfractal)
                     plot_orbit(new.x,new.y,-1);
                  else
                     iplot_orbit(lnew.x,lnew.y,-1);
               }

               if (oldcolor >= oldmax10)
                  if (periodicitycheck) 	/* only if this is OK to do */
                  {
                     if ((color & savedand) == 0) /* time to save a new value */
                     {
                        if (! integerfractal)
                           saved = new; 	/* floating pt fractals */
                        else
                           lsaved = lnew;		/* integer fractals */
                        if (--savedincr == 0) /* time to lengthen the periodicity? */
                        {
                           savedand = (savedand << 1) + 1; /* longer periodicity */
                           savedincr = 4;		   /* restart counter */
                        }
                     }
                     else /* check against an old save */
                     {
                        if (! integerfractal)	/* floating-pt periodicity chk */
                        {
                           if(fabs(saved.x-new.x) < closenuff)
                               if(fabs(saved.y-new.y) < closenuff)
                           {
                              caught_a_cycle = 7;
                              color = maxit;
                           }
                        }
                        else  /* integer periodicity check */
                        {
                           if(labs(lsaved.x-lnew.x) < lclosenuff)
                               if(labs(lsaved.y-lnew.y) < lclosenuff)
                           {
                              caught_a_cycle = 7;
                              color = maxit;
                           }
                        }
                     }
                  }
            }
            if(color >= maxit)
            {
               if (oldcolor < maxit)
               {
                  oldmax = oldcolor;
                  oldmax10 = oldmax - 10;
               }
               if(periodicitycheck < 0 && caught_a_cycle==7)
                  color = caught_a_cycle; /* show periodicity */
            }
            if(show_orbit)
               scrub_orbit();
            oldcolor = color;
            if (color == 0)color = 1; /* needed to make same as calcmand */

            if(potflag)
            {
               if(integerfractal)		/* adjust integer fractals */
               {
                  new.x = lnew.x;
                  new.x /= fudge;
                  new.y = lnew.y;
                  new.y /= fudge;
               }
               magnitude = sqr(new.x)+sqr(new.y);

               /* MUST call potential every pixel!! */
               color = potential(magnitude,color);
               if(color < 0 || color >= colors) continue;
            }
            else if (decomp[0] > 0)
               decomposition();
            else if(biomorph != -1)
            {
               if(integerfractal)
               {
                  if(labs(lnew.x) < llimit2 || labs(lnew.y) < llimit2)
                     color = biomorph;
               }
               else if(fabs(new.x) < rqlim2 || fabs(new.y) < rqlim2)
                  color = biomorph;
            }
            if((kbdcount-=color)<=0)
            {
               if(check_key())
                  return(-1);
               kbdcount=(cpu==386) ? 80 : 30;
            }
            if(oldcolor >= maxit) /* really color, not oldcolor */
            {
               if(bof_pp60_61 == 60)
                  color = sqrt(min_orbit)*75;
               else if(bof_pp60_61 == 61)
                  color = min_index;
               else if(inside >= 0)
                  color = inside;
               else if(inside == -1)
                  color = maxit;
            }
            else if(outside >= 0) /* merge escape-time stripes to one color */
               color = outside;

            if(LogFlag)
               color = LogTable[color];
            color = color&(colors-1);
            (*plot)(col,row,color);
            if(guessing>=2) /* solidguess does one pixel at a time */
               return(color);
            if(numpasses && (passes == 0))
            {
               (*plot)(col  ,row+1,color);
               (*plot)(col+1,row  ,color);
               (*plot)(col+1,row+1,color);
            }
         }
      }
   }
   if(guessing) /* solidguess wants to know the color */
      return(color);
   return(0);
}
decomposition()
{
   static double cos45	   = 0.70710678118654750; /* cos 45	degrees */
   static double sin45	   = 0.70710678118654750; /* sin 45	degrees */
   static double cos22_5   = 0.92387953251128670; /* cos 22.5	degrees */
   static double sin22_5   = 0.38268343236508980; /* sin 22.5	degrees */
   static double cos11_25  = 0.98078528040323040; /* cos 11.25	degrees */
   static double sin11_25  = 0.19509032201612820; /* sin 11.25	degrees */
   static double cos5_625  = 0.99518472667219690; /* cos 5.625	degrees */
   static double sin5_625  = 0.09801714032956060; /* sin 5.625	degrees */
   static double tan22_5   = 0.41421356237309500; /* tan 22.5	degrees */
   static double tan11_25  = 0.19891236737965800; /* tan 11.25	degrees */
   static double tan5_625  = 0.09849140335716425; /* tan 5.625	degrees */
   static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
   static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
   static long lcos45	  ; /* cos 45	  degrees */
   static long lsin45	  ; /* sin 45	  degrees */
   static long lcos22_5   ; /* cos 22.5   degrees */
   static long lsin22_5   ; /* sin 22.5   degrees */
   static long lcos11_25  ; /* cos 11.25  degrees */
   static long lsin11_25  ; /* sin 11.25  degrees */
   static long lcos5_625  ; /* cos 5.625  degrees */
   static long lsin5_625  ; /* sin 5.625  degrees */
   static long ltan22_5   ; /* tan 22.5   degrees */
   static long ltan11_25  ; /* tan 11.25  degrees */
   static long ltan5_625  ; /* tan 5.625  degrees */
   static long ltan2_8125 ; /* tan 2.8125 degrees */
   static long ltan1_4063 ; /* tan 1.4063 degrees */
   static char start=1;
   int temp = 0;
   struct lcomplex lalt;
   struct complex alt;
   if(start & integerfractal)
   {
      start = 0;
      lcos45	 = cos45      *fudge;
      lsin45	 = sin45      *fudge;
      lcos22_5	 = cos22_5    *fudge;
      lsin22_5	 = sin22_5    *fudge;
      lcos11_25  = cos11_25   *fudge;
      lsin11_25  = sin11_25   *fudge;
      lcos5_625  = cos5_625   *fudge;
      lsin5_625  = sin5_625   *fudge;
      ltan22_5	 = tan22_5    *fudge;
      ltan11_25  = tan11_25   *fudge;
      ltan5_625  = tan5_625   *fudge;
      ltan2_8125 = tan2_8125  *fudge;
      ltan1_4063 = tan1_4063  *fudge;
   }
   color = 0;
   if (integerfractal) /* the only case */
   {
      if (lnew.y < 0)
      {
         temp = 2;
         lnew.y = -lnew.y;
      }

      if (lnew.x < 0)
      {
         ++temp;
         lnew.x = -lnew.x;
      }

      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (lnew.x < lnew.y)
         {
            ++temp;
            lalt.x = lnew.x; /* just */
            lnew.x = lnew.y; /* swap */
            lnew.y = lalt.x; /* them */
         }

         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (multiply(lnew.x,ltan22_5,bitshift) < lnew.y)
            {
               ++temp;
               lalt = lnew;
               lnew.x = multiply(lalt.x,lcos45,bitshift) +
                   multiply(lalt.y,lsin45,bitshift);
               lnew.y = multiply(lalt.x,lsin45,bitshift) -
                   multiply(lalt.y,lcos45,bitshift);
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (multiply(lnew.x,ltan11_25,bitshift) < lnew.y)
               {
                  ++temp;
                  lalt = lnew;
                  lnew.x = multiply(lalt.x,lcos22_5,bitshift) +
                      multiply(lalt.y,lsin22_5,bitshift);
                  lnew.y = multiply(lalt.x,lsin22_5,bitshift) -
                      multiply(lalt.y,lcos22_5,bitshift);
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (multiply(lnew.x,ltan5_625,bitshift) < lnew.y)
                  {
                     ++temp;
                     lalt = lnew;
                     lnew.x = multiply(lalt.x,lcos11_25,bitshift) +
                         multiply(lalt.y,lsin11_25,bitshift);
                     lnew.y = multiply(lalt.x,lsin11_25,bitshift) -
                         multiply(lalt.y,lcos11_25,bitshift);
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (multiply(lnew.x,ltan2_8125,bitshift) < lnew.y)
                     {
                        ++temp;
                        lalt = lnew;
                        lnew.x = multiply(lalt.x,lcos5_625,bitshift) +
                            multiply(lalt.y,lsin5_625,bitshift);
                        lnew.y = multiply(lalt.x,lsin5_625,bitshift) -
                            multiply(lalt.y,lcos5_625,bitshift);
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if (multiply(lnew.x,ltan1_4063,bitshift) < lnew.y)
                           if ((lnew.x*ltan1_4063 < lnew.y))
                              ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   else /* double case */
   {
      if (new.y < 0)
      {
         temp = 2;
         new.y = -new.y;
      }
      if (new.x < 0)
      {
         ++temp;
         new.x = -new.x;
      }
      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (new.x < new.y)
         {
            ++temp;
            alt.x = new.x; /* just */
            new.x = new.y; /* swap */
            new.y = alt.x; /* them */
         }
         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (new.x*tan22_5 < new.y)
            {
               ++temp;
               alt = new;
               new.x = alt.x*cos45 + alt.y*sin45;
               new.y = alt.x*sin45 - alt.y*cos45;
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (new.x*tan11_25 < new.y)
               {
                  ++temp;
                  alt = new;
                  new.x = alt.x*cos22_5 + alt.y*sin22_5;
                  new.y = alt.x*sin22_5 - alt.y*cos22_5;
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (new.x*tan5_625 < new.y)
                  {
                     ++temp;
                     alt = new;
                     new.x = alt.x*cos11_25 + alt.y*sin11_25;
                     new.y = alt.x*sin11_25 - alt.y*cos11_25;
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (new.x*tan2_8125 < new.y)
                     {
                        ++temp;
                        alt = new;
                        new.x = alt.x*cos5_625 + alt.y*sin5_625;
                        new.y = alt.x*sin5_625 - alt.y*cos5_625;
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if ((new.x*tan1_4063 < new.y))
                           ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   for (i = 1; temp > 0; ++i)
   {
      if (temp & 1)
         color = (1 << i) - 1 - color;
      temp >>= 1;
   }
   if (decomp[0] == 2)
      color &= 1;
   if (colors > decomp[0])
      color++;
}

#if 0
MainNewton()
{
   Newton(); /* Lee's Newton */
   return(0);
}
#endif

/* Symmetry plot for period PI */
void symPIplot( x, y, color)
int x, y, color ;
{
  while(x < xdots)
  {
     putcolor( x, y, color) ;
     x += pixelpi;
  }
}
/* Symmetry plot for period PI plus Origin Symmetry */
void symPIplot2J( x, y, color)
int x, y, color ;
{
  while(x < xdots)
  {
     putcolor( x, y, color) ;
     putcolor( xdots - x - 1, (ydots - y - 1), color) ;
     x += pixelpi;
  }
}
/* Symmetry plot for period PI plus Both Axis Symmetry */
void symPIplot4J( x, y, color)
int x, y, color ;
{
  while(x < xdots>>1)
  {
     putcolor( x	    , y 	   , color) ;
     putcolor( x	    , ydots - y - 1, color) ;
     putcolor( xdots - x - 1, y 	   , color) ;
     putcolor( xdots - x - 1, ydots - y - 1, color) ;
     x += pixelpi;
  }
}

/* Symmetry plot for X Axis Symmetry */
void symplot2( x, y, color)
int x, y, color ;
{
  putcolor( x, y, color) ;
  putcolor( x, (ydots - y - 1), color) ;
}

/* Symmetry plot for Y Axis Symmetry */
void symplot2Y( x, y, color)
int x, y, color ;
{
  putcolor( x, y, color) ;
  putcolor( (xdots - x - 1), y, color) ;
}

/* Symmetry plot for Origin Symmetry */
void symplot2J( x, y, color)
int x, y, color ;
{
  putcolor( x, y, color) ;
  putcolor( xdots - x - 1, (ydots - y - 1), color) ;
}

/* Symmetry plot for Both Axis Symmetry */
void symplot4( x, y, color)
int x, y, color ;
{
  putcolor( x		 , y		, color) ;
  putcolor( x		 , ydots - y - 1, color) ;
  putcolor( xdots - x - 1, y		, color) ;
  putcolor( xdots - x - 1, ydots - y - 1, color) ;
}

/* Symmetry plot for X Axis Symmetry - Newtbasin version */
void symplot2basin( x, y, color)
int x, y, color ;
{
  extern int degree;
  putcolor( x, y, color) ;
  putcolor( x, (ydots - y - 1),( degree+1-color)%degree+1)  ;
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
void symplot4basin( x, y, color)
int x, y, color ;
{
  extern int degree;
  int color1;
  if(color == 0) /* assumed to be "inside" color */
  {
     symplot4( x, y, color);
     return;
  }
  color1 = degree/2+degree+2 - color;
  if(color < degree/2+2)
     color1 = degree/2+2 - color;
  else
	 color1 = degree/2+degree+2 - color;
  putcolor( x		 , y		, color) ;
  putcolor( x		 , ydots - y - 1,  (degree+1 - color)%degree+1) ;
  putcolor( xdots - x - 1, y		,  color1) ;
  putcolor( xdots - x - 1, ydots - y - 1, (degree+1 - color1)%degree+1) ;
}

/* Do nothing plot!!! */
void noplot(int x,int y,int color)
{
}

static int iparmx;				    /* iparmx = parm.x * 16 */
static int shiftvalue;				/* shift based on #colors */

typedef struct palett
{
   unsigned char red;
   unsigned char green;
   unsigned char blue;
} Palettetype;

extern int colors;
extern int xdots,ydots;
extern Palettetype dacbox[256];
extern int cpu, daccount, cyclelimit;
static int plasma_check;			/* to limit kbd checking */

void adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   long pseudorandom;
   if(getcolor(x,y) != 0)
      return;

   pseudorandom = ((long)iparmx)*((rand()-16383));
   pseudorandom = pseudorandom*(abs(xa-xb)+abs(ya-yb));
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = ((getcolor(xa,ya)+getcolor(xb,yb)+1)>>1)+pseudorandom;
   if (pseudorandom <	1) pseudorandom =   1;
   if (pseudorandom >= colors) pseudorandom = colors-1;
   putcolor(x,y,(int)pseudorandom);
}

void subDivide(int x1,int y1,int x2,int y2)
{
   int x,y;
   int v;
   if ((++plasma_check & 0x7f) == 1)
      if(check_key())
      {
	 plasma_check--;
	 return;
      }
   if(x2-x1<2 && y2-y1<2)
      return;
   x = (x1+x2)>>1;
   y = (y1+y2)>>1;
   adjust(x1,y1,x ,y1,x2,y1);
   adjust(x2,y1,x2,y ,x2,y2);
   adjust(x1,y2,x ,y2,x2,y2);
   adjust(x1,y1,x1,y ,x1,y2);
   if(getcolor(x,y) == 0)
   {
      v = (getcolor(x1,y1)+getcolor(x2,y1)+getcolor(x2,y2)+
		  getcolor(x1,y2)+2)>>2;
      putcolor(x,y,v);
   }
   subDivide(x1,y1,x ,y );
   subDivide(x ,y1,x2,y );
   subDivide(x ,y ,x2,y2);
   subDivide(x1,y ,x ,y2);
}

plasma()
{
   plasma_check = 0;

   if(colors < 4 || diskvideo) {
	setvideomode(3,0,0,0);
	buzzer(2);
	helpmessage(plasmamessage);
	return(-1);
	}
   iparmx = param[0] * 8;
   if (parm.x <= 0.0) iparmx = 16;
   if (parm.x >= 100) iparmx = 800;

   if (!rflag) rseed = (int)time(NULL);
      srand(rseed);

   if (colors == 256)			/* set the (256-color) palette */
      set_Plasma_palette();		/* skip this if < 256 colors */

   if (colors > 16)			/* adjust based on # of colors */
      shiftvalue = 18;
   else {
      if (colors > 4)
	 shiftvalue = 22;
      else {
	 if (colors > 2)
	    shiftvalue = 24;
	 else
	    shiftvalue = 25;
	 }
      }

   putcolor(	  0,	  0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,	  0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(	  0,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));

   subDivide(0,0,xdots-1,ydots-1);
   if (! check_key())
	return(0);
   else
	return(1);
}

set_Plasma_palette()
{
   static Palettetype Red    = {63, 0, 0};
   static Palettetype Green  = { 0,63, 0};
   static Palettetype Blue   = { 0, 0,63};
   int i;

	 if( CustomLut() ) return(0);		/* TARGA 3 June 89 j mclain */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
       dacbox[i].red   = (i*Green.red	+ (86-i)*Blue.red  )/85;
       dacbox[i].green = (i*Green.green + (86-i)*Blue.green)/85;
       dacbox[i].blue  = (i*Green.blue	+ (86-i)*Blue.blue )/85;

       dacbox[i+85].red   = (i*Red.red	 + (86-i)*Green.red  )/85;
       dacbox[i+85].green = (i*Red.green + (86-i)*Green.green)/85;
       dacbox[i+85].blue  = (i*Red.blue  + (86-i)*Green.blue )/85;

       dacbox[i+170].red   = (i*Blue.red   + (86-i)*Red.red  )/85;
       dacbox[i+170].green = (i*Blue.green + (86-i)*Red.green)/85;
       dacbox[i+170].blue  = (i*Blue.blue  + (86-i)*Red.blue )/85;
   }
	ValidateLuts( NULL );	/* TARGA 3 June 89  j mclain */
	spindac(0,1);
}

check_key()
{
int key;
  if((key = keypressed()) != 0) {
    if(key != 'o' && key != 'O')
       return(-1);
   getakey();
   show_orbit = 1 - show_orbit;
   }
   return(0);
}


/*

Table entry for fractals.c

   "diffusion",        "Border size","Stop width","", "",30,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,   NULL,     StandaloneSetup, diffusion,    NULL,

		    Hasty author comments

This routine generates a fractal via Diffusion Limited Aggregation.
You start out with a single point in the center of the screen.	A second point
moves around randomly until it comes into contact with the first point, at
which time its location is fixed and the process repeats.

One unfortunate problem is that on a large screen, this process will tend to
take eons.  To speed things up, I restrict the point to a box around the
fractal.  The variable border contains the distance between the edge of the
box and the fractal.  (Changing this can change the characteristics of the
fractal -- try 1.)  The fractal stops when it tries to expand this border
beyond the edge of the screen.	The second paramter, stop, is the value the
border size may decrease by before the fractal stops.

This was inspired by a Scientific American article a couple years back which
includes actual pictures of real physical phenomena that behave like this!

The show orbits function toggles the visiblity of the randomly moving point.

There isn't any particularly natural way to color this.  I just color each
point randomly.

			  -- Adrian Mariano

*/

/* I added this macro to get rid of unresolved reference to "random" - TW */
#define random(x)  (rand()%(x))
#define trigmax 250 /* size of trig table array */

diffusion()
{
   int xmax,ymax,xmin,ymin;     /* Current maximum coordinates */
   int border;   /* Distance between release point and fractal */
   int i;
   float sinarray[trigmax];
   float cosarray[trigmax];
   int angle;
   register int x,y;

   if (diskvideo)
   {
        setvideomode(3,0,0,0);
        buzzer(2);
        helpmessage(plasmamessage);
        return(-1);
   }
   border = param[0];
   for(i=0;i<trigmax;i++)
   {
      sinarray[i] = sin(i*2*PI/trigmax);
      cosarray[i] = cos(i*2*PI/trigmax);
   }
   if (border <= 0)
      border = 10;

   if (!rflag)
      rseed = (int)time(NULL);
   srand(rseed);

   xmax = xdots / 2 + border;  /* Initial box */
   xmin = xdots / 2 - border;
   ymax = ydots / 2 + border;
   ymin = ydots / 2 - border;

   putcolor(xdots / 2, ydots / 2,random(colors-1)+1);  /* Seed point */

   while (1)
   {
      /* Release new point on circle just inside the box */
      angle = random(trigmax);
      x = cosarray[angle]*(xmax-xmin) + xdots;
      y = sinarray[angle]*(ymax-ymin) + ydots;
      x = x >> 1; /* divide by 2 */
      y = y >> 1;

      /* Loop as long as the point (x,y) is surrounded by color 0 */
      /* on all eight sides                                       */
      while((getcolor(x+1,y+1) == 0) && (getcolor(x+1,y  ) == 0) &&
            (getcolor(x+1,y-1) == 0) && (getcolor(x  ,y+1) == 0) &&
            (getcolor(x  ,y-1) == 0) && (getcolor(x-1,y+1) == 0) &&
            (getcolor(x-1,y  ) == 0) && (getcolor(x-1,y-1) == 0))
      {
         /* Erase moving point */
         if (show_orbit)
            putcolor(x,y,0);

         /* Make sure point is inside the box */
         if (x==xmax)
            x--;
         else if (x==xmin)
            x++;
         if (y==ymax)
            y--;
         else if (y==ymin)
            y++;

         /* Take one random step */
         x += random(3)-1;
         y += random(3)-1;

         /* Check keyboard */
         if ((++plasma_check & 0x7f) == 1)
           if(check_key())
           {
              plasma_check--;
              return 1;
           }

         /* Show the moving point */
         if (show_orbit)
            putcolor(x,y,random(colors-1)+1);

      }
      putcolor(x,y,random(colors-1)+1);

      /* Is point to close to the edge? */
      if (((x+border)>xmax) || ((x-border)<xmin) || ((y-border)<ymin) || ((y+border)>ymax))
      {
         /* Increase box size, but not past the edge of the screen */
         if (ymin != 1) 
         {
            ymin--;
            ymax++;
         }
         if (xmin != 1)
         {  
            xmin--;
            xmax++;
         }
         if ((ymin==1) || (xmin==1))
           return 0;
      }
   }
}



iplot_orbit(ix, iy, color)
long ix, iy;
int color;
{
int i, j;
  if (ix < lx0[0] || ix > lx0[xdots-1] || iy < ly0[ydots-1] || iy > ly0[0]
    || diskvideo)
    return(0);
  i = (ix - lx0[0]) / delx;
  j = ydots - (iy - ly0[ydots-1]) / dely;

  /* save orbit value */
  if( 0 <= i && i < xdots && 0 <= j && j < ydots && orbit_ptr < 1000)
  {
     if(color == -1)
     {
	*(save_orbit + orbit_ptr++) = i;
	*(save_orbit + orbit_ptr++) = j;
	putcolor(i,j,getcolor(i,j)^orbit_color);
     }
     else
	putcolor(i,j,color);
  }
}

plot_orbit(real,imag,color)
double real,imag;
int color;
{
  int i, j;
  if((real < xxmin) || (real > xxmax) || (imag < yymin) || (imag > yymax))
     return(0);
  i = (real - xxmin)/deltaX;
  j = ydots - (imag - yymin)/deltaY;

  /* save orbit value */
  if( 0 <= i && i < xdots && 0 <= j && j < ydots && orbit_ptr < 1000)
  {
     if(color == -1)
     {
	*(save_orbit + orbit_ptr++) = i;
	*(save_orbit + orbit_ptr++) = j;
	putcolor(i,j,getcolor(i,j)^orbit_color);
     }
     else
	putcolor(i,j,color);
  }
}
scrub_orbit()
{
   int i,j;
   int last_orbit;
   last_orbit = orbit_ptr;
   orbit_ptr = 0;
   while(orbit_ptr < last_orbit)
   {
     i = *(save_orbit + orbit_ptr++);
     j = *(save_orbit + orbit_ptr++);
     putcolor(i,j,getcolor(i,j)^orbit_color);
   }
}
/* timer function. Assumes #include <time.h> */
timer(int (*fractal)(),int argc,...)
{
   va_list arg_marker;	/* variable arg list */
   int argcount;    /* how many optional args */
   char *timestring;
   time_t ltime;
   FILE *fp;
   int out;
   long t1,t2;
   int args[4];
   int i;

   /* argc = number of optional arguments */
   va_start(arg_marker,argc);

   i=0;
   while(i < argc)
      args[i++] = va_arg(arg_marker,int);

   if(timerflag)
      fp=fopen("bench","a");
   t1 = clock();
   switch(argc)
   {
   case 0:
      out = (*fractal)();
      break;
   case 1:
      out = (*fractal)(args[0]);
      break;
   case 2:
      out = (*fractal)(args[0],args[1]);
      break;
   case 3:
      out = (*fractal)(args[0],args[1],args[2]);
      break;
   case 4:
      out = (*fractal)(args[0],args[1],args[2],args[3]);
      break;
   default:
      return(-1);
      break;
   }
   t2 = clock();
   if(timerflag)
   {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */

      /* at the moment only decode timer takes a parameter */
      if(argc)
	 fprintf(fp,"decode ");
      fprintf(fp,"%s type=%s resolution = %dx%d maxiter=%d",
	   timestring,
	   fractalspecific[fractype].name,
	   xdots,
	   ydots,
	   maxit);
      fprintf(fp," time=%8.4f secs\n",
	   ((float)(t2-t1))/CLK_TCK);
      if(fp != NULL)
	  fclose(fp);
   }
   return(out);
}

/*
   I, Timothy Wegner, invented this solidguessing idea and implemented it in
   more or less the overall framework you see here.  I am adding this note
   now in a possibly vain attempt to secure my place in history, because
   Pieter Branderhorst has totally rewritten this routine, incorporating
   a *MUCH* more sophisticated algorithm.  His revised code is not only
   faster, but is also more accurate. Harrumph!
*/

solidguess()
{
   int oldnumpasses;
   int i,x,y,xlim,ylim,blocksize;
   unsigned int *pfxp0,*pfxp1;
   unsigned int u;

   /* save numpasses - this logic effectively bypasses it */
   oldnumpasses = numpasses;
   numpasses = 0;

   guessplot=(plot!=putcolor && plot!=symplot2 && plot!=symplot2J);
   /* check if guessing at bottom & right edges is ok */
   bottom_guess = (plot==putcolor || plot==symplot2);
   right_guess	= (bottom_guess   || plot==symplot2J);

   /* guessing 1 means reset oldcolor & kbdcount, and calc max stuff */
   /*	       2       reset oldcolor, do just one pixel */
   /*	       3       reset nothing,  do just one pixel */
   /* in all cases, calc is to return -1 if kbhit detected, color otherwise */

   /* blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200 */
   blocksize=4;
   i=300;
   while(i<=ydots)
   {
      blocksize+=blocksize;
      i+=i;
   }
   /* increase blocksize if prefix array not big enough */
   while(blocksize*(maxxblk-2)<XXdots || blocksize*(maxyblk-2)*16<YYdots)
      blocksize+=blocksize;
   maxblock=blocksize;
   pfxp1=&prefix[1][0][0];
   for(i=maxxblk*maxyblk;--i>=0;)
      *(pfxp1++)=0; /* init skip flags */

   /* first pass, calc every blocksize**2 pixel, quarter result & paint it */
   guessing=1; /* just for the first dot */
   iystart=iystop=0;
   for(x=0; x<XXdots; x+=maxblock)
   { /* calc top row */
      ixstart=ixstop=x;
      if((*calctype)()==-1)
         goto exit_solidguess;
   }
   for(y=0; y<YYdots; y+=blocksize)
   {
      if(y+blocksize<YYdots)
      { /* calc the row below */
         iystart=iystop=y+blocksize;
         guessing=3;
         oldcolor = 1;             /*NEW*/
         oldmax = min(maxit, 250); /*NEW*/
         oldmax10 = oldmax - 10;   /*NEW*/
         for(x=0; x<XXdots; x+=maxblock)
         {
            ixstart=ixstop=x;
            if((*calctype)()==-1)
               goto exit_solidguess;
         }
      }
      guessing=2;
      if(guessrow(1,y,blocksize)!=0)
         goto exit_solidguess;
   }

   /* calculate skip flags for skippable blocks */
   i=maxblock-1;
   xlim=(XXdots+i)/maxblock+1;
   ylim=((YYdots+i)/maxblock+15)/16+1;
   if(right_guess==0) /* no right edge guessing, zap border */
      for(y=0;y<=ylim;++y)
         prefix[1][y][xlim]=-1;
   if(bottom_guess==0) /* no bottom edge guessing, zap border */
   {
      i=(YYdots+maxblock-1)/maxblock+1;
      y=i/16+1;
      i=1<<(i&15);
      for(x=0;x<=xlim;++x)
         prefix[1][y][x]|=i;
   }
   /* set each bit in prefix[0] to OR of it & surrounding 8 in prefix[1] */
   for(y=0;++y<ylim;)
   {
      pfxp0=&prefix[0][y][0];
      pfxp1=&prefix[1][y][0];
      for(x=0;++x<xlim;)
      {
         ++pfxp1;
         u=*(pfxp1-1)|*pfxp1|*(pfxp1+1);
         *(++pfxp0)=u|(u>>1)|(u<<1)
            |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1)))>>15)
               |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1)))<<15);
      }
   }

   /* remaining pass(es), halve blocksize & quarter each blocksize**2 */
   guessing=2;
   while((blocksize=blocksize>>1)>=2)
   {
      for(y=0; y<YYdots; y+=blocksize)
         if(guessrow(0,y,blocksize)!=0)
            goto exit_solidguess;
   }

   exit_solidguess:
   numpasses = oldnumpasses;
   guessing  = 0;
}

#define calcadot(c,x,y) { ixstart=ixstop=x;iystart=iystop=y;\
      if((c=(*calctype)())==-1) return -1; }

int guessrow(int firstpass,int y,int blocksize)
{
   int x,i,j,color;
   int xplushalf,xplusblock;
   int ylessblock,ylesshalf,yplushalf,yplusblock;
   int	   c21,c31,c41; 	/* cxy is the color of pixel at (x,y) */
   int c12,c22,c32,c42; 	/* where c22 is the topleft corner of */
   int c13,c23,c33;		    /* the block being handled in current */
   int	   c24,    c44; 	/* iteration			      */
   int guessed23,guessed32,guessed33,guessed12,guessed13;
   int orig23,orig32,orig33;
   int prev11,fix21,fix31;
   unsigned int *pfxptr,pfxmask;

   halfblock=blocksize>>1;
   i=y/maxblock;
   pfxptr=&prefix[firstpass][(i>>4)+1][0];
   pfxmask=1<<(i&15);
   ylesshalf=y-halfblock;
   ylessblock=y-blocksize; /* constants, for speed */
   yplushalf=y+halfblock;
   yplusblock=y+blocksize;
   prev11=-1;
   c24=c12=c13=c22=getcolor(0,y);
   c31=c21=getcolor(0,(y>0)?ylesshalf:0);
   if(yplusblock<YYdots)
      c24=getcolor(0,yplusblock);
   else if(bottom_guess==0)
      c24=-1;
   guessed12=guessed13=0;

   for(x=0; x<XXdots;)	/* increment done at end, or when doing continue */
   {
      if((x&(maxblock-1))==0)  /* time for skip flag stuff */
      {
         ++pfxptr;
         if(firstpass==0 && (*pfxptr&pfxmask)==0)  /* check for fast skip */
         {
            /* next useful in testing to make skips visible */
            /*
               if(halfblock==1)
               {
                  (*plot)(x+1,y,0); (*plot)(x,y+1,0); (*plot)(x+1,y+1,0);
                  }
             */
            x+=maxblock;
            prev11=c31=c21=c24=c12=c13=c22;
            guessed12=guessed13=0;
            continue;
         }
      }

      if(firstpass)  /* 1st pass, paint topleft corner */
         plotblock(0,x,y,c22);
      /* setup variables */
      xplusblock=(xplushalf=x+halfblock)+halfblock;
      if(xplushalf>=XXdots)
      {
         if(right_guess==0)
            c31=-1;
      }
      else if(y>0)
         c31=getcolor(xplushalf,ylesshalf);
      if(xplusblock<XXdots)
      {
         if(yplusblock<YYdots)
            c44=getcolor(xplusblock,yplusblock);
         c41=getcolor(xplusblock,(y>0)?ylesshalf:0);
         c42=getcolor(xplusblock,y);
      }
      else if(right_guess==0)
         c41=c42=c44=-1;
      if(yplusblock>=YYdots)
         c44=(bottom_guess)?c42:-1;

      /* guess or calc the remaining 3 quarters of current block */
      guessed23=guessed32=guessed33=1;
      c23=c32=c33=c22;
      if(yplushalf>=YYdots)
      {
         if(bottom_guess==0)
            c23=c33=-1;
         guessed23=guessed33=-1;
      }
      if(xplushalf>=XXdots)
      {
         if(right_guess==0)
            c32=c33=-1;
         guessed32=guessed33=-1;
      }
      while(1) /* go around till none of 23,32,33 change anymore */
      {
         if(guessed33>0
             && (c33!=c44 || c33!=c42 || c33!=c24 || c33!=c32 || c33!=c23))
         {
            calcadot(c33,xplushalf,yplushalf);
            guessed33=0;
         }
         if(guessed32>0
             && (c32!=c33 || c32!=c42 || c32!=c31 || c32!=c21
             || c32!=c41 || c32!=c23))
         {
            calcadot(c32,xplushalf,y);
            guessed32=0;
            continue;
         }
         if(guessed23>0
             && (c23!=c33 || c23!=c24 || c23!=c13 || c23!=c12 || c23!=c32))
         {
            calcadot(c23,x,yplushalf);
            guessed23=0;
            continue;
         }
         break;
      }

      if(firstpass) /* note whether any of block's contents were calculated */
         if(guessed23==0 || guessed32==0 || guessed33==0)
            *pfxptr|=pfxmask;

      if(halfblock>1) /* not last pass, check if something to display */
         if(firstpass)	/* display guessed corners, fill in block */
         {
            if(guessplot)
            {
               if(guessed23>0)
                  (*plot)(x,yplushalf,c23);
               if(guessed32>0)
                  (*plot)(xplushalf,y,c32);
               if(guessed33>0)
                  (*plot)(xplushalf,yplushalf,c33);
            }
            plotblock(1,x,yplushalf,c23);
            plotblock(0,xplushalf,y,c32);
            plotblock(1,xplushalf,yplushalf,c33);
         }
         else  /* repaint changed blocks */
         {
            if(c23!=c22)
               plotblock(-1,x,yplushalf,c23);
            if(c32!=c22)
               plotblock(-1,xplushalf,y,c32);
            if(c33!=c22)
               plotblock(-1,xplushalf,yplushalf,c33);
         }

      /* check if some calcs in this block mean earlier guesses need fixing */
      fix21=((c22!=c12 || c22!=c32)
          && c21==c22 && c21==c31 && c21==prev11
          && y>0
          && (x==0 || c21==getcolor(x-halfblock,ylessblock))
          && (xplushalf>=XXdots || c21==getcolor(xplushalf,ylessblock))
          && c21==getcolor(x,ylessblock));
      fix31=(c22!=c32
          && c31==c22 && c31==c42 && c31==c21 && c31==c41
          && y>0 && xplushalf<XXdots
          && c31==getcolor(xplushalf,ylessblock)
          && (xplusblock>=XXdots || c31==getcolor(xplusblock,ylessblock))
          && c31==getcolor(x,ylessblock));
      prev11=c31; /* for next time around */
      if(fix21)
      {
         calcadot(c21,x,ylesshalf);
         if(halfblock>1 && c21!=c22)
            plotblock(-1,x,ylesshalf,c21);
      }
      if(fix31)
      {
         calcadot(c31,xplushalf,ylesshalf);
         if(halfblock>1 && c31!=c22)
            plotblock(-1,xplushalf,ylesshalf,c31);
      }
      if(c23!=c22)
      {
         if(guessed12)
         {
            calcadot(c12,x-halfblock,y);
            if(halfblock>1 && c12!=c22)
               plotblock(-1,x-halfblock,y,c12);
         }
         if(guessed13)
         {
            calcadot(c13,x-halfblock,yplushalf);
            if(halfblock>1 && c13!=c22)
               plotblock(-1,x-halfblock,yplushalf,c13);
         }
      }

      c22=c42;
      c24=c44;
      c13=c33;
      c31=c21=c41;
      c12=c32;
      guessed12=guessed32;
      guessed13=guessed33;
      x+=blocksize;
   } /* end x loop */

   if(firstpass==0 || guessplot) return 0;

/* paint rows the fast way */
   for(i=0;i<halfblock;++i)
   {
      if((rowcount=y+i)<YYdots)
         out_line(&dstack[0],xdots);
      if((rowcount=y+i+halfblock)<YYdots)
         out_line(&dstack[2048],xdots);
   }
   if(plot!=putcolor)  /* symmetry, just vertical & origin the fast way */
   {
      if(plot==symplot2J) /* origin sym, reverse lines */
         for(i=XXdots/2;--i>=0;)
         {
            color=dstack[i];
            dstack[i]=dstack[j=xdots-1-i];
            dstack[j]=color;
            j+=2048;
            color=dstack[i+2048];
            dstack[i+2048]=dstack[j];
            dstack[j]=color;
         }
      for(i=0;i<halfblock;++i)
      {
         if((rowcount=ydots-1-y-i)>=YYdots)
            out_line(&dstack[0],xdots);
         if((rowcount=ydots-1-y-i-halfblock)>=YYdots)
            out_line(&dstack[2048],xdots);
      }
   }
   return 0;
}

plotblock(int buildrow,int x,int y,int color)
{
   int i,xlim,ylim;
   if((xlim=x+halfblock)>XXdots)
      xlim=XXdots;
   if(buildrow>=0 && guessplot==0) /* save it for later out_line */
   {
      if(buildrow==0)
         for(i=x;i<xlim;++i)
            dstack[i]=color;
      else
         for(i=x;i<xlim;++i)
            dstack[i+2048]=color;
      return(0);
   }
   /* paint it */
   if((ylim=y+halfblock)>YYdots)
   {
      if(y>=YYdots)
         return(0);
      ylim=YYdots;
   }
   for(i=x;++i<xlim;)
      (*plot)(i,y,color); /* skip 1st dot on 1st row */
   while(++y<ylim)
      for(i=x;i<xlim;++i)
         (*plot)(i,y,color);
}

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia	  */
/* Reference: Science of Fractal Images p. 190. 		  */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.	  */
/*								  */
/* The purpose of this function is to calculate a color value	  */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.			  */
/*								  */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential	  */
/* surface. Returns color.  - Tim Wegner 6/25/89		  */
/*								  */
/*		       -- Change history --			  */
/*								  */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*								  */
/******************************************************************/

int potential(double mag, int iterations)
{
   extern char potfile[];     /* name of pot save file */
   extern struct fractal_info save_info;	/*  for saving data */

   static int x,y;	      /* keep track of position in image */
   float f_mag,f_tmp,pot;
   double d_tmp;
   int i_pot;
   unsigned int intbuf;
   FILE *t16_create();

   extern int maxit;	      /* maximum iteration bailout limit */
   extern double potparam[];	 /* continuous potential parameters */
   extern char potfile[];     /* pot savename */
   extern char floatflag;

   if(top)
   {
      top = 0;
      x   = 0;
      y   = 0;
      /* make sure file is closed */
      if(fp_pot)
	 fclose(fp_pot);
      if(potfile[0])
	 fp_pot = t16_create(potfile, xdots, ydots,
	     sizeof(struct fractal_info), &save_info);
   }
   if(iterations < maxit)
   {
      i_pot = pot = iterations+2;
      if(i_pot <= 0 || mag <= 1.0)
	 pot = 0.0;
      else /* pot = log(mag) / pow(2.0, (double)pot); */
      {
	 i_pot = pot;
	 if(i_pot < 120 && !floatflag) /* empirically determined limit of fShift */
	 {
	    f_mag = mag;
	    fLog14(f_mag,f_tmp); /* this SHOULD be non-negative */
	    fShift(f_tmp,-i_pot,pot);
	 }
	 else
	 {
	    d_tmp = log(mag)/(double)pow(2.0,(double)pot);
	    if(d_tmp > FLT_MIN) /* prevent float type underflow */
	       pot = d_tmp;
	    else
	       pot = 0.0;
	 }
      }
      /* following transformation strictly for aesthetic reasons */
      /* meaning of parameters:
      potparam[0] -- zero potential level - highest color -
      potparam[1] -- slope multiplier -- higher is steeper
      potparam[2] -- rqlim value if changeable (bailout for modulus) */

      if(pot > 0.0)
      {
	 if(floatflag)
	    pot = (float)sqrt((double)pot);
	 else
	 {
	    fSqrt14(pot,f_tmp);
	    pot = f_tmp;
	 }
	 pot = potparam[0] - pot*potparam[1] - 1.0;
      }
      else
	 pot = potparam[0] - 1.0;
   }
   else if(inside > 0)
      pot = inside; /* negative inside turns off inside replacement */
   else
      pot = potparam[0];

   if(pot > colors)
      pot = colors -1;
   if(pot < 1.0)
      pot = 1.0; /* avoid color 0 */

    intbuf = pot*(1<<8); /* shift 8 bits */
    decoderline[x] = intbuf;
    if(++x == xdots)
    {
       /* new row */
       if(fp_pot)
	  t16_putline(fp_pot, xdots, decoderline);
       x = 0;
       if(++y == ydots)
       {
	  /* we're done */
	  t16_flush(fp_pot);
	  fclose(fp_pot);
	  fp_pot = NULL;
	  potfile[0] = 0;
       }
    }
    return((int)pot);
}

/*
   "intpotential" is called by the MANDEL/JULIA routines with scaled long
   integer magnitudes.	The routine just calls "potential".  Bert
*/

int intpotential(unsigned long longmagnitude, int iterations)
{				 /* this version called from calcmand() */
   double magnititude;

   magnitude = ((double)longmagnitude) / fudge;
   magnitude = 256*magnitude;
   return(potential(magnitude, iterations));
}
setsymmetry(int sym)	/* set up proper symmetrical plot functions */
{
   extern int forcesymmetry;
   plot = putcolor;	/* assume no symmetry */
   XXdots = xdots;
   YYdots = ydots;
   iystop = YYdots -1;
   ixstop = XXdots -1;
   symmetry = 1;
   if(sym == NOPLOT && forcesymmetry == 999)
   {
      plot = noplot;
      return(0);
   }
   /* NOTE: either diskvideo or 16-bit potential disables symmetry */
   /* also ant decomp= option and any inversion not about the origin */
   if ((!potfile[0]) && (!diskvideo) && (!invert || inversion[2] == 0.0)
   && decomp[0] == 0)
   {
      if(sym != XAXIS && sym != XAXIS_NOPARM && inversion[1] != 0.0 && forcesymmetry == 999)
	 return(0);
      else if(forcesymmetry != 999)
	 sym = forcesymmetry;
      switch(sym)	/* symmetry switch */
      {
      case XAXIS_NOPARM:			/* X-axis Symmetry  (no params)*/
	 if (parm.x != 0.0 || parm.y != 0.0)
	    break;
      case XAXIS:			/* X-axis Symmetry */
	 if (fabs(yymax + yymin) >= fabs(yymax - yymin)*.01 )
	    if(forcesymmetry==999)
	       break;
	 symmetry = 0;
	 if(basin)
	    plot = symplot2basin;
	 else
	    plot = symplot2;
	 YYdots /= 2;
	 iystop = YYdots -1;
	 break;
      case YAXIS_NOPARM:			/* Y-axis Symmetry (No Parms)*/
	 if (parm.x != 0.0 || parm.y != 0.0)
	    break;
      case YAXIS:			/* Y-axis Symmetry */
	 if (fabs(xxmax + xxmin) >= fabs(xxmax - xxmin)*.01)
	    if(forcesymmetry==999)
	       break;
	 plot = symplot2Y;
	 XXdots /= 2;
	 ixstop = XXdots -1;
	 break;
      case XYAXIS_NOPARM:			/* X-axis AND Y-axis Symmetry (no parms)*/
	 if(parm.x != 0.0 || parm.y != 0.0)
	    break;
      case XYAXIS:			/* X-axis AND Y-axis Symmetry */
	 if(forcesymmetry!=999)
	 {
	    if(basin)
	       plot = symplot4basin;
	    else
	       plot = symplot4 ;
	    XXdots /= 2 ;
	    YYdots /= 2 ;
	    iystop = YYdots -1;
	    ixstop = XXdots -1;
	    break;
	 }
	 if ((fabs(yymax + yymin) < fabs(yymax - yymin)*.01 &&
	     fabs(xxmax + xxmin) < fabs(xxmax - xxmin)*.01))
	 {
	    if(basin)
	       plot = symplot4basin;
	    else
	       plot = symplot4 ;
	    XXdots /= 2 ;
	    YYdots /= 2 ;
	    iystop = YYdots -1;
	    ixstop = XXdots -1;
	 }
	 else if (fabs(yymax + yymin) < fabs(yymax - yymin)*.01)
	 {
	    if(basin)
	       plot = symplot2basin;
	    else
	       plot = symplot2 ;
	    YYdots /= 2 ;
	    iystop = YYdots -1;
	 }
	 break;
      case ORIGIN_NOPARM:			/* Origin Symmetry (no parms)*/
	 if (parm.x != 0.0 || parm.y != 0.0)
	    break;
      case ORIGIN:			/* Origin Symmetry */
	 if (fabs(yymax + yymin) >= fabs(yymax - yymin)*.01 ||
	     fabs(xxmax + xxmin) >= fabs(xxmax - xxmin)*.01)
	    if(forcesymmetry==999)
	       break;
	 symmetry = 0;
	 plot = symplot2J;
	 YYdots /= 2 ;
	 iystop = YYdots -1;
	 break;
      case PI_SYM_NOPARM:
	 if (parm.x != 0.0 || parm.y != 0.0)
	    break;
      case PI_SYM:			/* PI symmetry */
	 if(invert && forcesymmetry == 999)
	 {
	    if (fabs(yymax + yymin) >= fabs(yymax - yymin)*.01 ||
		fabs(xxmax + xxmin) >= fabs(xxmax - xxmin)*.01)
	       break;
	    symmetry = 0;
	    plot = symplot2J;
	    YYdots /= 2 ;
	    iystop = YYdots -1;
	    break;
	 }
	 pixelpi = (PI/fabs(xxmax-xxmin))*xdots; /* PI in pixels */
	 if(pixelpi < xdots)
	 {
	    XXdots = pixelpi;
	    ixstop = XXdots-1;
	 }

	 if (fabs(yymax + yymin) < fabs(yymax - yymin)*.01 &&
	     fabs(xxmax + xxmin) < fabs(xxmax - xxmin)*.01)
	 {
	    if(parm.y == 0.0)
	    {
	       plot = symPIplot4J ;
	       YYdots /= 2 ;
	       iystop = YYdots -1;
	    }
	    else
	    {
	       plot = symPIplot2J;
	       YYdots /= 2 ;
	       iystop = YYdots -1;
	    }
	 }
	 else if(!invert || forcesymmetry != 999)
	    plot = symPIplot ;
	 break;
      default:			/* no symmetry */
	 break;
      }
   }
}
