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
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <dos.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include "fractint.h"
#include "fmath.h"
#include "targa_lc.h"

extern int bailout;
extern char far plasmamessage[];
long lmagnitud, llimit, llimit2, lclosenuff,l16triglim;
struct complex init,tmp,old,new,saved;
extern double tempsqrx,tempsqry;
extern long ltempsqrx,ltempsqry;
extern int biomorph;
extern struct lcomplex linit;
extern int basin;

int color, oldcolor, oldmax, oldmax10, row, col, passes;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
double far *dx0, far *dy0;
long XXOne, FgOne, FgTwo, LowerLimit;

extern int LogFlag;
extern char far *LogTable;

void (*plot)() = putcolor;

extern double inversion[];	    /* inversion radius, f_xcenter, f_ycenter */
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors;				/* maximum colors available */
extern int	inside;				/* "inside" color to use    */
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
extern long	lx0[], ly0[];		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	fudge;				/* fudge factor (2**n) */
extern int	bitshift;			/* bit shift for fudge */
extern char potfile[];          /* potential filename */


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

extern    int extraseg;

/* These are local but I don't want to pass them as parameters */
static    unsigned char top;   /* flag to indicate top of calcfract */ 
static    unsigned char c;
static	  int i;

double xxmin,xxmax,yymin,yymax; /* corners */
struct complex lambda;
double deltaX, deltaY;
double magnitude, rqlim, rqlim2;
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
int	symmetry;			/* symmetry flag for calcmand()	*/
int	guessing;			/* solid-guessing flag for calcmand() */

static	int	integerfractal;		/* TRUE if fractal uses integer math */

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only	*/
/* -------------------------------------------------------------------- */

extern struct lcomplex lnew,llambda;
int periodicitycheck;

extern double floatmin, floatmax;

int StandardFractal();


calcfract()
{
   orbit_color = 15;
   top     = 1;
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
   ixstart = 0;				/* default values to disable ..	*/
   iystart = 0;				/*   solid guessing		*/
   ixstop = xdots-1;
   iystop = ydots-1;
   guessing = 0;

   basin = 0;
   
   xxmin  = (double)lx0[      0] / fudge;  
   xxmax  = (double)lx0[xdots-1] / fudge;
   yymax  = (double)ly0[      0] / fudge;
   yymin  = (double)ly0[ydots-1] / fudge;
   deltaX = (double)lx0[      1] / fudge - xxmin;
   deltaY = yymax - (double)ly0[      1] / fudge;

   parm.x  = param[0];
   parm.y  = param[1];

   if(extraseg)
   {
#ifdef __TURBOC__
      dx0 = MK_FP(extraseg,0);
#else
      FP_SEG(dx0)=extraseg;
      FP_OFF(dx0)=0;
#endif
   }else
      return(-1);
   dy0 = dx0 + MAXPIXELS;

   if(fabs(potparam[0]) > 0.0)
      potflag = 1;
   else
      potflag = 0;   

   if (LogFlag)
      ChkLogFlag();

   lm = 4;				/* CALCMAND magnitude limit */
   lm = lm << bitshift;
/*		Continuous potential override (unused at the moment)
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

   integerfractal = fractalspecific[fractype].isinteger;
   if ((!integerfractal) || invert)
   {
      /* set up dx0 and dy0 analogs of lx0 and ly0 in Bert's "extra" segment */
      /* put fractal parameters in doubles */
      for(i=0;i<xdots;i++)
         dx0[i] = (double)lx0[i] / fudge;
      for(i=0;i<ydots;i++)
        dy0[i] = (double)ly0[i] / fudge;
   }

   if (integerfractal)	 	/* the bailout limit can't be too high here */
      if (rqlim > 127.0) rqlim = 127.0;
   if(inversion[0] != 0.0)
   {
      f_radius    = inversion[0];
      f_xcenter   = inversion[1];
      f_ycenter   = inversion[2];
      
      if (inversion[0] < 0.0)  /*  auto calc radius 1/6 screen */
         inversion[0] = f_radius = (min(xxmax - xxmin,yymax - yymin)) / 6.0;
      
      if (invert < 2) { /* xcenter not already set */
         inversion[1] = f_xcenter = (xxmin + xxmax) / 2.0;
         if (fabs(f_xcenter) < (xxmax-xxmin) / 100)
	         inversion[1] = f_xcenter = 0.0;
         }

      if (invert < 3) { /* ycenter not already set */
         inversion[2] = f_ycenter = (yymin + yymax) / 2.0;
         if (fabs(f_ycenter) < (yymax-yymin) / 100)
	         inversion[2] = f_ycenter = 0.0;
         }

      invert = 3; /* so values will not be changed if we come back */
   }

   setsymmetry(fractalspecific[fractype].symmetry);

   if (potfile[0] != 0) 			/* potential file? */
   {
      numpasses = 0;				/* disable dual-pass */
	  solidguessing = 0;			/* disable solid-guessing */
   }
   
   closenuff = ((delx < dely ? delx : dely) >> 1); /* for periodicity checking */
   closenuff /= fudge;
   rqlim2 = sqrt(rqlim);
   if (integerfractal) 		/* for integer routines (lambda) */
   {
      llambda.x = parm.x * fudge;		/* real portion of Lambda */
      llambda.y = parm.y * fudge;		/* imaginary portion of Lambda */
      llimit = rqlim * fudge;		/* stop if magnitude exceeds this */
      if (llimit <= 0) llimit = 0x7fffffff; /* klooge for integer math */
      llimit2 = rqlim2 * fudge;	/* stop if magnitude exceeds this */
      lclosenuff = closenuff * fudge;   /* "close enough" value */
      l16triglim = 8L<<16;   /* domain limit of fast trig functions */
   }

   calctype = fractalspecific[fractype].calctype;		/* assume a standard fractal type */

   periodicitycheck = 1;		/* turn on periodicity checking */
   
   /* per_image routine is run here */   
   if (fractalspecific[fractype].per_image())  /* a stand-alone type? */
   {
      /* fractal routine (usually StandardFractal) is run here */
      if(solidguessing) 	/* use std solid-guessing? */
         timer(solidguess,0);
      else
         timer(calctype,0);
   }

   potfile[0] = NULL;
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

StandardFractal()
{
   int caught_a_cycle;
   int savedand, savedincr;		/* for periodicity checking */
   int quad;
   struct lcomplex lsaved;

   /* note: numpasses must == 1 in solidguessing mode */
   /* This is set in solidguess() */   
   for (passes=0; passes <= numpasses ; passes++)
   {
      /* guessing == 1 means first pass only */
      if(guessing == 1 && passes == 1)
         break;
         
      /* guessing == 2 means second pass only */   
      if(guessing == 2 && passes == 0)
         continue;
         
      /* if numpasses == 1, on first pass skip rows */
      for (row = iystart; row <= iystop; row = row + 1 + numpasses - passes)
      {
         oldcolor = 1;
         oldmax = min(maxit, 250);
         oldmax10 = oldmax - 10;
         
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
            if(check_key())
                  return(-1);
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
            
            fractalspecific[fractype].per_pixel(); /* initialize the calculations */
            while (++color < maxit)
            {
               /* calculation of one orbit goes here */
               /* input in "old" -- output in "new" */

               if(fractalspecific[fractype].orbitcalc()) break;
               if(show_orbit) 
               {
                  if (! integerfractal)
                     plot_orbit(new.x,new.y,-1);
                  else
                     iplot_orbit(lnew.x,lnew.y,-1);
               }

               if (oldcolor >= oldmax10)
                  if (periodicitycheck)		/* only if this is OK to do */ 
                  {
                     if ((color & savedand) == 0) /* time to save a new value */
                     {
                        if (! integerfractal)
                           saved = new;		/* floating pt fractals */
                        else 
                           lsaved = lnew;		/* integer fractals */
                        if (--savedincr == 0) /* time to lengthen the periodicity? */
                        { 
                           savedand = (savedand << 1) + 1; /* longer periodicity */
                           savedincr = 4;                  /* restart counter */
                        }
                     }
                     else /* check against an old save */
                     {
                        if (! integerfractal)	/* floating-pt periodicity chk */
                        { 
                           if((fabs(saved.x-new.x)+fabs(saved.y-new.y)) 
                                 < closenuff)
                           {
                              caught_a_cycle = 7;
                              color = maxit;
                           }  
                        }
                        else  /* integer periodicity check */
                        {
                           if(labs(lsaved.x-lnew.x)+labs(lsaved.y-lnew.y) 
                                 < lclosenuff) 
                           {
                              caught_a_cycle = 7;
                              color = maxit;
                           }
                        }
                     }
                  }
            }
            if (oldcolor < maxit && color >= maxit) 
            {
		       oldmax = oldcolor;
			   oldmax10 = oldmax - 10;
			}
            if(show_orbit)
               scrub_orbit();
            oldcolor = color;
            if(color < maxit)
            {
               caught_a_cycle = color;   
               if (color == 0)color = 1; /* needed to make same as calcmand */
               if(debugflag==98) color = caught_a_cycle;
            }

            if(potflag) 
            {
               if(integerfractal) 		/* adjust integer fractals */
               { 
                  new.x = lnew.x; new.x /= fudge;
                  new.y = lnew.y; new.y /= fudge;
               }
               magnitude = sqr(new.x)+sqr(new.y);

               /* printf("rqlim %e magnit %e new.x %e new.y %e\n",rqlim,magnitude,new.x,new.y); */

               /* MUST call potential every pixel!! */
               color = potential(magnitude,color); 
               if(color < 0 || color >= colors) continue; 
            }
            else if (decomp[0] > 0)
            {
               int temp = 0;
               struct lcomplex lalt;
               struct complex alt;
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
                        if (lnew.x * 0.414213562373 < lnew.y) /* tan(22.5) */
                        {
                           ++temp;
                           lalt = lnew;
                           lnew.x = lalt.x * 0.707106781187   /* cos(45) */
                                  + lalt.y * 0.707106781187; /* sin(45) */
                           lnew.y = lalt.x * 0.707106781187   /* sin(45) */
                                  - lalt.y * 0.707106781187; /* cos(45) */
                        }
   
                        if (decomp[0] >= 32)
                        {
                           temp <<= 1;
                           if (lnew.x * 0.19891236738 < lnew.y) /* tan(11.25) */
                           {
                              ++temp;
                              lalt = lnew;
                              lnew.x = lalt.x * 0.923879532511   /* cos(22.5) */
                                     + lalt.y * 0.382683432365;  /* sin(22.5) */
                              lnew.y = lalt.x * 0.382683432365   /* sin(22.5) */
                                     - lalt.y * 0.923879532511;  /* cos(22.5) */
                           }
   
                           if (decomp[0] >= 64)
                           {
                              temp <<= 1;
                              if (lnew.x * 0.0984914033572 < lnew.y)
                              {
                                 ++temp;
                                 lalt = lnew;
                                 lnew.x = lalt.x * 0.980785280403
                                        + lalt.y * 0.195090322016;
                                 lnew.y = lalt.x * 0.195090322016
                                        - lalt.y * 0.980785280403;
                              }
   
                              if (decomp[0] >= 128)
                              {
                                 temp <<= 1;
                                 if (lnew.x * 0.0491268497695 < lnew.y)
                                 {
                                    ++temp;
                                    lalt = lnew;
                                    lnew.x = lalt.x * 0.998795456205
                                           + lalt.y * 0.0490676743274;
                                    lnew.y = lalt.x * 0.0490676743274
                                           - lalt.y * 0.998795456205;
                                 }
   
                                 if (decomp[0] == 256)
                                 {
                                    temp <<= 1;
                                    if ((lnew.x * 0.0245486221089 < lnew.y))
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
                        if (new.x * 0.414213562373 < new.y) /* tan(22.5) */
                        {
                           ++temp;
                           alt = new;
                           new.x = alt.x * 0.707106781187   /* cos(45) */
                                 + alt.y * 0.707106781187;  /* sin(45) */
                           new.y = alt.x * 0.707106781187   /* sin(45) */
                                 - alt.y * 0.707106781187;  /* cos(45) */
                        }
   
                        if (decomp[0] >= 32)
                        {
                           temp <<= 1;
                           if (new.x * 0.19891236738 < new.y) /* tan(11.25) */
                           {
                              ++temp;
                              alt = new;
                              new.x = alt.x * 0.923879532511   /* cos(22.5) */
                                    + alt.y * 0.382683432365;  /* sin(22.5) */
                              new.y = alt.x * 0.382683432365   /* sin(22.5) */
                                    - alt.y * 0.923879532511;  /* cos(22.5) */
                           }
   
                           if (decomp[0] >= 64)
                           {
                              temp <<= 1;
                              if (new.x * 0.0984914033572 < new.y)
                              {
                                 ++temp;
                                 alt = new;
                                 new.x = alt.x * 0.980785280403
                                       + alt.y * 0.195090322016;
                                 new.y = alt.x * 0.195090322016
                                       - alt.y * 0.980785280403;
                              }
   
                              if (decomp[0] >= 128)
                              {
                                 temp <<= 1;
                                 if (new.x * 0.0491268497695 < new.y)
                                 {
                                    ++temp;
                                    alt = new;
                                    new.x = alt.x * 0.998795456205
                                          + alt.y * 0.0490676743274;
                                    new.y = alt.x * 0.0490676743274
                                          - alt.y * 0.998795456205;
                                 }
   
                                 if (decomp[0] == 256)
                                 {
                                    temp <<= 1;
                                    if ((new.x * 0.0245486221089 < new.y))
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
               if (decomp[0] == 2)        color &= 1;
               if (colors > decomp[0])    color++;
            }
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
            
            if(oldcolor >= maxit && inside >= 0) /* really color, not oldcolor */
               color = inside;
            if(LogFlag)
               color = LogTable[color];

            (*plot)(col,row,color);
            if(numpasses && (passes == 0))
            {
                (*plot)(col  ,row+1,color);
                (*plot)(col+1,row  ,color);
                (*plot)(col+1,row+1,color);
            }
         }
      }
   }
   return(0);
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
     putcolor( x            , y            , color) ;
     putcolor( x            , ydots - y - 1, color) ;
     putcolor( xdots - x - 1, y            , color) ;
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
  putcolor( x            , y            , color) ;
  putcolor( x            , ydots - y - 1, color) ;
  putcolor( xdots - x - 1, y            , color) ;
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
  putcolor( x            , y            , color) ;
  putcolor( x            , ydots - y - 1,  (degree+1 - color)%degree+1) ;
  putcolor( xdots - x - 1, y            ,  color1) ;
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
   if (pseudorandom <   1) pseudorandom =   1;
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

   putcolor(      0,      0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,      0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(      0,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));

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

	 if( CustomLut() ) return;		/* TARGA 3 June 89 j mclain */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
       dacbox[i].red   = (i*Green.red   + (86-i)*Blue.red  )/85;
       dacbox[i].green = (i*Green.green + (86-i)*Blue.green)/85;
       dacbox[i].blue  = (i*Green.blue  + (86-i)*Blue.blue )/85;

       dacbox[i+85].red   = (i*Red.red   + (86-i)*Green.red  )/85;
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
  long ix,iy;
  if((real < xxmin) || (real > xxmax) || (imag < yymin) || (imag > yymax))
     return(0);
  /* convert to longs */
  ix = real * fudge;
  iy = imag * fudge;
  iplot_orbit(ix,iy,color);
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

/* these variables are used to tune algorithm */
int block_size;      /* multiple of grid used for block */
                     /* smaller blocks catch more blocks but also have */
                     /* more overhead. */     
int xblock,yblock;   /* pixel dimension of solid blocks */
int xres,yres;       /* pixel dimension of solid-guessing grid */  

solidguess()
{
   int oldnumpasses;
   int i,j;

   /* save numpasses - this logic effectively bypasses it */ 
   oldnumpasses = numpasses; 
   numpasses = 1;

   /* initial tuning values */
   block_size = 1;      /* look for block size == grid size */

   /* set up grid and block */
   xres = 2;
   yres = 2;

   xblock = xres*block_size;
   yblock = yres*block_size;

   guessing = 1;			/* solid-guessing - first pass */

   (*calctype)();    		/* generate the overall grid */
	 if (check_key()) {		/* TARGA 2 June 89 j mclain */
    	numpasses = oldnumpasses;
     	return(-1);
  	}													 

   guessing=2;  			/* solid guessing - second pass */

   for(j=0;j<YYdots;j += yblock)
   {
      iystart = j;  
/*      iystop  = min(j+yblock-1,YYdots-1);  */
      iystop  = j+yblock-1;  
      for(i=0;i<XXdots;i += xblock)
      {
         ixstart = i;
/*         ixstop  = min(i+xblock-1,XXdots-1); */
         ixstop  = i+xblock-1;
         if(checkblock())
         {
            (*calctype)();
            if (check_key()) 
            {
               numpasses = oldnumpasses;
               return(-1);
            }
         }
      }
   }
   numpasses = oldnumpasses;
   guessing  = 0;
}

checkblock()
{
   /* check all the grid points in this block */
   /* to see if the same color */
   int same;
   int thecolor;
   int i,j; 
   same=1;
   for(j=iystart;j<=iystop+1;j += yres) {
      if (j >= ydots) continue;
      for(i=ixstart;i<=ixstop+1;i += xres)
      {
         if (i >= xdots) continue;
         if(j == iystart && i == ixstart)
            thecolor=getcolor(i,j);
         else
         {
            if(getcolor(i,j) != thecolor)
               return(1); /* darn! all colors not the same! */
         }   
      }
   }
   return(0); /* yup, all colors the same! Guess its a solid block! */
}      

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia      */
/* Reference: Science of Fractal Images p. 190.                   */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.          */
/*                                                                */
/* The purpose of this function is to calculate a color value     */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.                          */
/*                                                                */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential    */
/* surface. Returns color.  - Tim Wegner 6/25/89                  */
/*                                                                */
/*                     -- Change history --                       */
/*                                                                */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*                                                                */
/******************************************************************/

int potential(double mag, int iterations)
{
   extern char potfile[];     /* name of pot save file */
   extern struct fractal_info save_info;	/*  for saving data */

   static int x,y;            /* keep track of position in image */
   float f_mag,f_tmp,pot;
   double d_tmp;
   int i_pot;
   unsigned int intbuf;
   FILE *t16_create();
   
   extern int maxit;          /* maximum iteration bailout limit */
   extern double potparam[];     /* continuous potential parameters */
   extern char potfile[];     /* pot savename */
   extern unsigned int decoderline[];
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
          potfile[0] = NULL;
       }
    }
    return((int)pot);
}      

/*
   "intpotential" is called by the MANDEL/JULIA routines with scaled long
   integer magnitudes.  The routine just calls "potential".  Bert
*/

int intpotential(unsigned long longmagnitude, int iterations)
{				 /* this version called from calcmand() */
   double magnititude;

   magnitude = ((double)longmagnitude) / fudge;
   magnitude = 256*magnitude;
   return(potential(magnitude, iterations));
}
setsymmetry(int sym)    /* set up proper symmetrical plot functions */
{
   extern int forcesymmetry;
   plot = putcolor;     /* assume no symmetry */
   XXdots = xdots;
   YYdots = ydots;
   iystop = YYdots -1;
   ixstop = XXdots -1;
   symmetry = 1;
   if(sym == NOPLOT && forcesymmetry == 999)
   {
      plot = noplot;
      return;
   }   
   /* NOTE: either diskvideo or 16-bit potential disables symmetry */
   /* also ant decomp= option and any inversion not about the origin */
   if ((!potfile[0]) && (!diskvideo) && (!invert || inversion[2] == 0.0)
   && decomp[0] == 0) 
   {
      if(sym != XAXIS && sym != XAXIS_NOPARM && inversion[1] != 0.0 && forcesymmetry == 999)
         return;
      else if(forcesymmetry != 999)
         sym = forcesymmetry;
      switch(sym) 	/* symmetry switch */
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
