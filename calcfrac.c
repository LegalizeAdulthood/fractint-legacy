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

/*******************************************************
** Changes made to support external Newton() routine: **
*******************************************************/

double roverd, d1overd, threshold, floatmin;
int infinity, maxcolor;

int color, oldcolor, oldmax, row, col, passes;
struct complex init,tmp,old,new,saved,roots[10];
int iterations, root, degree, not_done, invert, invertsym;
double far *dx0, far *dy0;

void (*plot)() = putcolor;

/**********************
** Back to old code: **
**********************/

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

extern double	param[];		/* parameters */
extern double	potparam[];		/* potential parameters */
extern long	lx0[], ly0[];		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	fudge;				/* fudge factor (2**n) */
extern int	bitshift;			/* bit shift for fudge */
extern char potfile[];          /* potential filename */

/* COMPLEX stuff */
#define REAL_ONE     1
#define REAL_NEG_ONE 2
#define IMAG_ONE     3
#define IMAG_NEG_ONE 4
#define MAX_COLOR    5
#define INFINITY    14
#define THRESHOLD .2

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1,z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))

struct complex one = {1,0};

/* These are local but I don't want to pass them as parameters */
static    unsigned char top;   /* flag to indicate top of calcfract */ 
static    double xxmin,xxmax,yymin,yymax; /* corners */
static	  struct complex lambda;
static    unsigned char c;
static	  int i;
          double deltaX, deltaY;
static    double magnitude, rqlim;
static    int Xdots, Ydots; /* local dots */
static    double parm1,parm2;
extern    int extraseg;
static    int (*calctype)();
static    double closenuff;
static    int pixelpi; /* value of pi in pixels */
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

calcfract()
{
   top     = 1;
   if(fp_pot)
   {
      fclose(fp_pot);
      fp_pot = NULL;
   }

   ixstart = 0;				/* default values to disable ..	*/
   iystart = 0;				/*   solid guessing		*/
   ixstop = xdots-1;
   iystop = ydots-1;
   guessing = 0;
   
   xxmin  = (double)lx0[      0] / fudge;  
   xxmax  = (double)lx0[xdots-1] / fudge;
   yymax  = (double)ly0[      0] / fudge;
   yymin  = (double)ly0[ydots-1] / fudge;
   deltaX = (double)lx0[      1] / fudge - xxmin;
   deltaY = yymax - (double)ly0[      1] / fudge;

   parm1  = param[0];
   parm2  = param[1];

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

   plot = putcolor;
   Xdots = xdots;
   Ydots = ydots; 

   if(fabs(potparam[0]) > 0.0)
      potflag = 1;
   else
      potflag = 0;   

   lm = 4;				/* CALCMAND magnitude limit */
   lm = lm << bitshift;
/*		Continuous potential override (unused at the moment)
   if (potparam[2] > 4 && potparam[2] < 64)
      lm = potparam[2]*fudge;
*/
   /* set modulus bailout value if 3rd potparam set */
   if(fabs(potparam[2]) > 0.0) 
      rqlim = potparam[2];
   else
      rqlim = 4.0;

   /* ORBIT stuff */
   save_orbit = (int far *)(dx0 + 2*MAXPIXELS);
   show_orbit = 0;
   orbit_ptr = 0;
   if(colors < 16)
       orbit_color = 1;

/* initialization code that gets run for all but the original mandelbrot
   and Julia set functions */

if (fractype != MANDEL && fractype != JULIA
   && fractype != LAMBDA && fractype != PLASMA) 
   {
      /* set up dx0 and dy0 analogs of lx0 and ly0 in Bert's "extra" segment */
      /* put fractal parameters in doubles */
      for(i=0;i<xdots;i++)
         dx0[i] = (double)lx0[i] / fudge;
      for(i=0;i<ydots;i++)
        dy0[i] = (double)ly0[i] / fudge;
   }
   
/* LC 5/9/89: Added support for Inversion of Newtons */

   invert = 0;
   invertsym = 1;
   if ((fractype == NEWTON || fractype == NEWTBASIN) && param[1] != 0.0) 
   {
      invert = 1;
      if (param[2] != 0.0 || param[3] != 0.0) invertsym = 0;
      if (param[2] == 0.0) 
         param[2] = (dx0[ixstart] + dx0[ixstop]) / 2.0;
      if (param[3] == 0.0) 
         param[3] = (dy0[iystart] + dy0[iystop]) / 2.0;
   }

   if (potfile[0] != 0) 				/* potential file? */
   {
      numpasses = 0;				/* disable dual-pass */
	  solidguessing = 0;			/* disable solid-guessing */
   }

   /* set things up for right fractal calculation */
   switch(fractype)
   {
   case PLASMA:
      timer(plasma,0);
      break;
   case MANDEL:
   case JULIA:
      /* call original ASM fractint Mandelbrot or Julia */
      symmetry = 0;           /* first, claim symmetry is true */
      if (diskvideo || potfile[0])      /* no symmetry in disk-video mode */
         symmetry = 1;
      if (fabs(yymax + yymin) > fabs(yymax - yymin)*.01)
         symmetry = 1;        /* non-symmetrical along the Y-axis */
      if (fractype == JULIA && fabs(xxmax + xxmin) > fabs(xxmax - xxmin)*.01)
         symmetry = 1;        /* non-symmetrical along the X-axis */
      if (symmetry == 0)      /* is symmetry still true? */
         Ydots /= 2;          /* well, then, let's cut the load in half! */

      if(solidguessing)
      {
         calctype = calcmand;
         timer(solidguess,0);
      }   
      else
         timer(calcmand,0);
      break;
   case NEWTBASIN:
   case NEWTON:
      /* set up table of roots of 1 along unit circle */
      degree = (int)parm1;
      if(degree  >= 10)
         degree = 10;
      else if(degree < 3)
         degree = 3;

/*******************
** More new code: **
*******************/

	  root = 1;
	  roverd = (double)root / (double)degree;
	  d1overd = (double)(degree - 1) / (double)degree;
	  floatmin = FLT_MIN;
	  threshold = THRESHOLD;
	  infinity = INFINITY;
	  maxcolor = MAX_COLOR;

/***************************
** Back to the old stuff: **
***************************/

      for(i=0;i<degree;i++)
      {
         roots[i].x = cos(i*PI*2.0/(float)degree);
         roots[i].y = sin(i*PI*2.0/(float)degree);
      }
     if ((! diskvideo) 
            && fabs(xxmax + xxmin) < .0001 && fabs(yymax + yymin) < .0001 
            && fractype==NEWTON && degree%4 == 0 && invertsym)
     {
         plot = symplot4 ;
         Xdots /= 2 ;
         Ydots /= 2 ;
         iystop = Ydots -1;
         ixstop = Xdots -1;
     }
     else if ((! diskvideo) && fabs(yymax + yymin) < .0001 && fractype==NEWTON && invertsym)
      {
         plot = symplot2 ;
         Ydots /= 2 ;
         iystop = Ydots -1;
     }
     else
         plot = putcolor;
      if(solidguessing)
      {
         calctype = MainNewton;
         timer(solidguess,0);
      }   
      else
         timer(MainNewton,0);
      break;
   case LAMBDA:
      /* lambda function */
      closenuff = ((delx < dely ? delx : dely) >> 1); /* for periodicity checking */
      closenuff /= fudge;
      if(solidguessing)
      {
         calctype = Lambda;
         timer(solidguess,0);
      }   
      else
         timer(Lambda,0);
      break;
   case MANDELFP:
      /* floating point mandelbrot */
      if ((!potfile[0]) && (! diskvideo) &&
         fabs(yymax + yymin) < fabs(yymax - yymin)*.01)	/* Take advantage of Y-axis */
      {							/* symmetry to halve needed */
         plot = symplot2 ; 		/* calculations.            */
		 Ydots /= 2 ;		        /* Here, the choice of plot */
         iystop = Ydots -1;
      }							/* function is made         */
      else
         plot = putcolor ;
   
      closenuff = ((delx < dely ? delx : dely) >> 1); /* for periodicity checking */
      closenuff /= fudge;
      if(solidguessing)
      {
         calctype = Mandelfp;
         timer(solidguess,0);
      }   
      else
         timer(Mandelfp,0);
      break;
   case JULIAFP:
      /* floating point Julia */
      if ((!potfile[0]) && (! diskvideo) &&
          fabs(yymax + yymin) < fabs(yymax - yymin)*.01 &&
          fabs(xxmax + xxmin) < fabs(xxmax - xxmin)*.01)
      {
         plot = symplot2J;
         Ydots /= 2 ;
         iystop = Ydots -1;
      }				
      else
         plot = putcolor ;
      closenuff = ((delx < dely ? delx : dely) >> 1); /* for periodicity checking */
      closenuff /= fudge;
      if(solidguessing)
      {
         calctype = Juliafp;
         timer(solidguess,0);
      }   
      else
         timer(Juliafp,0);
      break;

   case LAMBDASINE:
   case LAMBDACOS:
   case LAMBDAEXP:

   plot = putcolor;			/* default option: no symmetry */
   Xdots = xdots;
   Ydots = ydots; 

      if (fractype == LAMBDAEXP && (! diskvideo) &&
         fabs(yymax + yymin) < fabs(yymax - yymin)*.01)	/* Take advantage of Y-axis */
      {							/* symmetry to halve needed */
         plot = symplot2 ; 				/* calculations.            */
		 Ydots /= 2 ;
         iystop = Ydots -1;
      }	

      if ((fractype == LAMBDASINE || fractype == LAMBDACOS)) {	/* symmetry may apply here */
        /* calculate value of PI in pixels */
        pixelpi = (PI/fabs(xxmax-xxmin))*xdots;
        if(pixelpi < xdots)
        {
           Xdots = pixelpi;
           ixstop = Xdots-1;
        }   
        if ( (! diskvideo) &&
          fabs(yymax + yymin) < fabs(yymax - yymin)*.01 &&
          fabs(xxmax + xxmin) < fabs(xxmax - xxmin)*.01)
        {
           if(parm2 == 0.0)
           {
              plot = symPIplot4J ;
              Ydots /= 2 ;
              iystop = Ydots -1;
           }
           else
           {
              plot = symPIplot2J;
              Ydots /= 2 ;
              iystop = Ydots -1;
           }
        }				
        else
           plot = symPIplot ;
      }
      if(solidguessing)
      {
         calctype = Lambdasine;
         timer(solidguess,0);
      }   
      else
         timer(Lambdasine,0);
      break;
   case TEST:
      timer(test,0);
      break;
   default:
      return(-1);
      break;
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
   for (row = passes; row < Ydots; row=row+1+numpasses)
   {
      register int col;
      init.y = dy0[row];
      for (col = 0; col < Xdots; col++)       /* look at each point on screen */
      {
         register color;
         init.x = dx0[col];
         if(check_key()) {
            testend();
            return(-1);
            }
         color = testpt(init.x,init.y,parm1,parm2,maxit,inside);
         (*plot)(col,row,color);
         if(numpasses && (passes == 0))
             (*plot)(col,row+1,color);
      }
   }
   testend();
   return(0);
}

/* calculate Julia set of lamda*sin(z) */
/* Reference - The Science of Fractal Images , Barnsley et al, p. 158 */
/* try params=1/.1  params=1/0  params=1/.4 */
Lambdasine()
{

   double oldreal,oldimag,newreal,newimag,tmpreal,tmpimag;
   int iterations;

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
         register int col;
         init.y = dy0[row];

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            register color;
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            init.x = dx0[col];
            if(check_key())
                  return(-1);
            orbit_ptr = 0;        


            /* calculate lambda * sin(z) [ or cos(x) or exp(x) ] */
            /* lamda = parm1,parm2 */
            oldreal=init.x;
            oldimag=init.y;
            iterations = 0;
            while(iterations++ < maxit)
            {   
                if (fractype == LAMBDASINE) {     /* calculate sin(z) */
                   if (fabs(oldimag) >= 50.0) break;
                   tmpreal = sin(oldreal)*cosh(oldimag);
                   tmpimag = cos(oldreal)*sinh(oldimag);
                   }
                if (fractype == LAMBDACOS) {    /* calculate cos(z) */
                   if (fabs(oldimag) >= 50.0) break;
                   tmpreal = cos(oldreal)*cosh(oldimag);
                   tmpimag = sin(oldreal)*sinh(oldimag);
                   }
                if (fractype == LAMBDAEXP) {    /* calculate exp(z) */
                   double tmpexp, tmpcos;
                   if (fabs(oldimag) >= 1.0e8) break; /* TLOSS  errors */
                   if (fabs(oldreal) >= 6.4e2) break; /* DOMAIN errors */
                   tmpcos = cos(oldimag);
                   if (oldreal >= 50.0 && tmpcos >= 0.0) break;
                   tmpexp = exp(oldreal);
                   tmpreal = tmpexp*tmpcos;
                   tmpimag = tmpexp*sin(oldimag);
                   }
            
                /*multiply by lamda */
                newreal = parm1*tmpreal - parm2*tmpimag;
                newimag = parm2*tmpreal + parm1*tmpimag;
                oldreal = newreal;
                oldimag = newimag;
                if(show_orbit)
                   plot_orbit(newreal,newimag);
            }
            if(show_orbit)
               scrub_orbit();
            if(iterations >= maxit && inside >= 0)
               color = inside;
            else
               color = iterations&(colors-1);   
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

Mandelfp()
{
   int caught_a_cycle;
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
         register int col;
         oldcolor = 1;
         oldmax = maxit;
         saved.x = 0;
         saved.y = 0;
         init.y = dy0[row];

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            register color;
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            init.x = dx0[col];
            if(check_key())
                  return(-1);
            orbit_ptr = 0;        
            old.x=parm1;
            old.y=parm2;
            magnitude = 0.0;
            color = -1;
            caught_a_cycle = 1;
            while ((magnitude < rqlim) && (color < maxit))
            {
               new.x = sqr (old.x) - sqr (old.y) + init.x;
               new.y = 2 * old.x * old.y + init.y;
               color++;
               old=new;
               magnitude = sqr (new.x) + sqr (new.y);
               if(show_orbit)
                  plot_orbit(new.x,new.y);
               if (oldcolor >= oldmax-10) 
               {
                  if ((color & 15) == 0) 
                     saved = new;
                  else if ((fabs(saved.x-new.x) +
                        fabs(saved.y-new.y)) < closenuff)
                  {
                     caught_a_cycle = 7;
                     color = maxit;
                  }  
               }
            }
            if (oldcolor < maxit && color >= maxit)
		       oldmax = oldcolor;
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
               /* MUST call potential every pixel!! */
               color = potential(magnitude,color); 
               if(color < 0 || color >= colors) continue; 
            }
 
            if(oldcolor >= maxit && inside >= 0) /* really color, not oldcolor */
               color = inside;
            else
               color = color&(colors-1);    

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
Juliafp()
{
   int caught_a_cycle;
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
         register int col;
         oldcolor = 1;
         oldmax = maxit;
         saved.x = 0;
         saved.y = 0;

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            register color;
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            old.x = dx0[col];
            old.y = dy0[row];
            if(check_key())
               return(-1);
            orbit_ptr = 0;   
            magnitude = 0.0;
            color = 0;
            while ((magnitude < rqlim) && (color < maxit))
            {
               new.x = sqr (old.x) - sqr (old.y) + parm1;
               new.y = 2 * old.x * old.y + parm2;

               /* START CODE SAME AS MANDELFP HERE TO BOTTOM */
               color++;
               old=new;
               magnitude = sqr (new.x) + sqr (new.y);
               if(show_orbit)
                  plot_orbit(new.x,new.y);
               if (oldcolor >= oldmax-10) 
               {
                  if ((color & 15) == 0) 
                     saved = new;
                  else if ((fabs(saved.x-new.x) +
                        fabs(saved.y-new.y)) < closenuff)
                  {
                     caught_a_cycle = 7;
                     color = maxit;
                  }  
               }
            }
            if (oldcolor < maxit && color >= maxit)
		       oldmax = oldcolor;
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
               /* MUST call potential every pixel!! */
               color = potential(magnitude,color); 
               if(color < 0 || color >= colors) continue; 
            }
 
            if(oldcolor >= maxit && inside >= 0) /* really color, not oldcolor */
               color = inside;
            else
               color = color&(colors-1);    

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

MainNewton()
{
   
   {
      Newton(); /* Lee's Newton */
      return(0);
   }
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
         register int col;
         oldcolor = 1;
	     oldmax = maxit;
         saved.x = 0;
         saved.y = 0;
         init.y = dy0[row];

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            register color;
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            init.x = dx0[col];
            if(check_key())
                  return(-1);
            orbit_ptr = 0;        
            old = init;
            color = -1;
            iterations = 0;
            not_done = 1;
            while (not_done)
            {
               iterations++;
               complex_power(old,degree-1,&tmp);
               complex_mult(tmp,old,&new);      /* new = old**5 */
               if(distance(new,one) < threshold || iterations > maxit)
                  not_done = 0; 
               new.x = ((degree-1.0)*new.x+root)/degree;
               new.y = (degree-1.0)*new.y/degree;
         
               /* watch out for divide underflow */
               if(complex_div(new,tmp,&new))
                  old = new;
               else
               {
                  not_done = 0;
                  color = infinity;
                  break;
               }         
               if(show_orbit)
                  plot_orbit(new.x,new.y);
            }
            if(show_orbit)
               scrub_orbit();
            if(fractype==2)
            {
               color = 0;
               /* this code determines which degree-th root of root the        
                  Newton formula converges to. The roots of a 1 are 
                  distributed on a circle of radius 1 about the origin. */
               for(i=0;i<degree;i++)
               {
                  /* color in alternating shades with iteration according to
                     which root of 1 it converged to */
                  if( distance(roots[i],new) < threshold)
                     color = 1+i%8+8*(iterations%2);
               }
               if(!color) 
                    color = maxcolor;
            }else
            {
               if(iterations < maxit)
                  color = iterations&(colors-1);
               else
                  color = inside;
            }
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

extern long multiply(long x, long y, int shift);

Lambda()
{
   int caught_a_cycle;
   long lambdar, lambdai, zr, zi, oldzr, oldzi, savedr, savedi, tempr, tempi;
   long lmagnitud, llimit, lclosenuff;

   lambdar = parm1 * fudge;		/* real portion of Lambda */
   lambdai = parm2 * fudge;		/* imaginary portion of Lambda */
   llimit = rqlim * fudge;		/* stop if magnitude exceeds this */
   lclosenuff = closenuff * fudge;      /* "close enough" value */

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
         register int col;
         oldcolor = maxit;
         savedr = 0;
         savedi = 0;

         /* if numpasses == 1, on first pass skip cols */
         for (col = ixstart; col <= ixstop; col = col + 1 + numpasses - passes)
         {
            register color;
            /* on second pass even row,col pts already done */
            if(passes) /* second pass */
               if(row&1 ==0 && col&1 == 0)
                  continue;
            if(check_key())
               return(-1);
            orbit_ptr = 0;   
            lmagnitud = 0;
            color = 0;
            oldzr = lx0[col];
            oldzi = ly0[row];
            while ((lmagnitud < llimit) && (color < maxit))
            {
		/* (in complex math) temp = Z * (1-Z) */
		tempr = oldzr
			- multiply(oldzr, oldzr, bitshift)
			+ multiply(oldzi, oldzi, bitshift);
		tempi = oldzi
			- (multiply(oldzi, oldzr, bitshift)  << 1);
		/* (in complex math) Z = Lambda * Z */
		zr = 	  multiply(lambdar, tempr, bitshift)
			- multiply(lambdai, tempi, bitshift);
		zi =	  multiply(lambdar, tempi, bitshift)
			+ multiply(lambdai, tempr, bitshift);
		/* (in complex math) get the magnititude */
		lmagnitud = multiply(zr, zr, bitshift)
			+   multiply(zi, zi, bitshift);
		oldzr = zr;  oldzi = zi;
               color++;
               if(show_orbit)
                  iplot_orbit(zr,zi);
               if (color >= oldcolor) 
               {
                  if ((color & 15) == 0) {
			savedr = zr;
			savedi = zi;
			}
                  else {
                     tempr = savedr - zr;  if (tempr < 0) tempr = -tempr;
                     tempi = savedi - zi;  if (tempi < 0) tempi = -tempi;
                     if ((tempr + tempi) < lclosenuff) {
                        caught_a_cycle = 7;
                        color = maxit;
                     }
                  }  
               }
            }
            oldcolor = color+10;
            if (color >= maxit)
               oldcolor = 0;
            if(show_orbit)
               scrub_orbit();
            if (color >= maxit) 
               color = inside;
            else
               caught_a_cycle = color;   
            if (color == 0)color = 1; /* needed to make same as calcmand */
            if(debugflag==98) color = caught_a_cycle;
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

complex_mult(arg1,arg2,pz)
struct complex arg1,arg2,*pz;
{
   pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
   pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
   return(0);
}
complex_div(numerator,denominator,pout)
struct complex numerator,denominator,*pout;
{  
   double mod;
   if((mod = modulus(denominator)) < floatmin)
      return(NULL);
   conjugate(&denominator);
   complex_mult(numerator,denominator,pout);
   pout->x = pout->x/mod;
   pout->y = pout->y/mod;
   return(1);
}
complex_power(z,n,pout)
struct complex z;
int n;
struct complex *pout;
{
   if(n<0)
      return(NULL);
   else if(n==0)
   {
      pout->x      = 1.0;
      pout->y = 0.0;
   }
   else if(n == 1)
   {
      pout->x      = z.x;
      pout->y = z.y;
   }
   else
   {
      struct complex tmp;
      n--;
      complex_mult(z,z,pout);
      while(--n)
      {
         tmp = *pout;
         complex_mult(z,tmp,pout);      /* new = old**2 */
      }
   }
   return(1);
}
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

static int iparm1;				/* iparm1 = parm1 * 16 */
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

void adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   long pseudorandom;
   if(getcolor(x,y) != 0)
      return;

   pseudorandom = ((long)iparm1)*((rand()-16383));
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
   if(check_key())
      return;
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
	printf("\n\nI'm sorry, but Plasma Clouds can currently only be run in a\n");
	printf("4-or-more-color video mode (and color-cycled only on VGA adapters\n");
	printf("[or EGA adapters in their 640x350x16 mode]).  Also, because of the\n");
	printf("random-screen-access algorithm, Plasma Clouds cannot be run using\n");
	printf("an Expanded-Memory or Disk-based 'Video' mode.  Finally, we REALLY\n");
	printf("recommend using a 256-color mode if you have one.\n");
	printf("\n\nEither press a function key (like F1 thru F5) that selects one of\n");
	printf("those modes, or press the 't' key to select a new fractal type.\n");
	return(-1);
	}
   parm1  = param[0];
   iparm1 = (parm1 * 8) ;
   if (parm1 <= 0.0) iparm1 = 16;
   if (parm1 >= 100) iparm1 = 800;
   srand((unsigned)(time(NULL)));
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
   if(keypressed())
   {
     if(keypressed() == 'o' || keypressed() == 'O')
     {
        getakey();
        show_orbit = 1 - show_orbit;
        return(0);
      }   
      else
        return(-1);
   }
return(0);
}

iplot_orbit(ix, iy)
long ix, iy;
{
int i, j;
  if (ix < lx0[0] || ix > lx0[xdots-1] || iy < ly0[ydots-1] || iy > ly0[0]
    || diskvideo)
    return(0);
  i = (ix - lx0[0]) / delx;
  j = ydots - (iy - ly0[ydots-1]) / dely;

  /* save orbit value */
  if( 0 <= i && i < xdots && 0 <= j && j < ydots)
  {
     *(save_orbit + orbit_ptr++) = i; 
     *(save_orbit + orbit_ptr++) = j; 
     putcolor(i,j,getcolor(i,j)^orbit_color);
  }
}

plot_orbit(real,imag)
double real,imag;
{
  long ix,iy;
  if((real < xxmin) || (real > xxmax) || (imag < yymin) || (imag > yymax))
     return(0);
  /* convert to longs */
  ix = real * fudge;
  iy = imag * fudge;
  iplot_orbit(ix,iy);
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
           typelist[fractype],
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

   for(j=0;j<Ydots;j += yblock)
   {
      iystart = j;  
/*      iystop  = min(j+yblock-1,Ydots-1);  */
      iystop  = j+yblock-1;  
      for(i=0;i<Xdots;i += xblock)
      {
         ixstart = i;
/*         ixstop  = min(i+xblock-1,Xdots-1); */
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

/*
   "intpotential" is called by the MANDEL/JULIA routines with scaled long
   integer magnitudes.  The routine just calls "potential".  Bert
*/

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
/******************************************************************/

int potential(double mag,int iterations)
{
   extern char potfile[];     /* name of pot save file */
   extern struct fractal_info save_info;	/*  for saving data */

   static int x,y;            /* keep track of position in image */
   float f_mag,f_tmp,pot;
   int i_pot;
   unsigned int intbuf;
   FILE *t16_create();
   
   extern int maxit;          /* maximum iteration bailout limit */
   extern double potparam[];     /* continuous potential parameters */
   extern char potfile[];     /* pot savename */
   extern unsigned int decoderline[];
   
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
      pot = iterations+2;
      if(pot <= 0.0 || mag < 1.0)
         pot = 0.0;
      else /* pot = log(mag) / pow(2.0, (double)pot); */ 
      {
         f_mag = mag;
         i_pot = pot;
         fLog14(f_mag,f_tmp);
         if(i_pot < 120) /* empirically determined limit of fShift */
            fShift(f_tmp,-i_pot,pot);
         else
            pot = f_tmp/pow(2.0,(double)pot);
      }
      /* following transformation strictly for aesthetic reasons */
      /* meaning of parameters: 
      potparam[0] -- zero potential level - highest color - 
      potparam[1] -- slope multiplier -- higher is steeper
      potparam[2] -- rqlim value if changeable (bailout for modulus) */

      if(pot > 0.0)
      {
         fSqrt14(pot,pot);
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
   return(potential(magnitude, iterations));
}
