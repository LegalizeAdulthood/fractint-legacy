/*
FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
images (well, SOMEBODY had to do it!).  The modules are set up so that
all logic that is independent of any fractal-specific code is in
CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
struscture that ties (we hope!) everything together is in FRACTALP.C.
Original author Tim Wegner, but just about ALL the authors have
contributed SOME code to this routine at one time or another, or
contributed to one of the many massive restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbitcalc. EVERY
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'lnew.x' and
   'lnew.y', for floating point fractals in 'new.x' and 'new.y'.

2. Routines that are called once per pixel to set various variables
   prior to the orbit calculation. These have names like xxx_per_pixel
   and are fairly generic - chances are one is right for your new type.
   They are stored in fractalspecific[fractype].per_pixel.

3. Routines that are called once per screen to set various variables.
   These have names like XxxxSetup, and are stored in
   fractalspecific[fractype].per_image.

4. The main fractal routine. Usually this will be StandardFractal(),
   but if you have written a stand-alone fractal routine independent
   of the StandardFractal mechanisms, your routine name goes here,
   stored in fractalspecific[fractype].calctype.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */


#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif
#include "fractint.h"
#include "mpmath.h"
#include "helpdefs.h"
#include "fractype.h"
#include "prototyp.h"

#define NEWTONDEGREELIMIT  100

extern int using_jiim;
extern _CMPLX  initorbit;
extern _LCMPLX linitorbit;
extern char useinitorbit;
extern double fgLimit;
extern int distest;

#define CMPLXsqr_old(out)	\
   (out).y = (old.x+old.x) * old.y;\
   (out).x = tempsqrx - tempsqry

#define CMPLXpwr(arg1,arg2,out)   (out)= ComplexPower((arg1), (arg2))
#define CMPLXmult1(arg1,arg2,out)    Arg2->d = (arg1); Arg1->d = (arg2);\
	 dStkMul(); Arg1++; Arg2++; (out) = Arg2->d

#define CMPLXmult(arg1,arg2,out)  \
	{\
	   _CMPLX TmP;\
	   TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;\
	   TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;\
	   (out) = TmP;\
	 }
#if 0
#define CMPLXmult(arg1,arg2,out)  {CMPLXmult2(arg1,arg2,&out);}
#endif

void CMPLXmult2(_CMPLX arg1,_CMPLX arg2, _CMPLX *out)
{
    _CMPLX TmP;
    TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;
    TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;
    *out = TmP;
}

#define CMPLXadd(arg1,arg2,out)    \
    (out).x = (arg1).x + (arg2).x; (out).y = (arg1).y + (arg2).y
#define CMPLXsub(arg1,arg2,out)    \
    (out).x = (arg1).x - (arg2).x; (out).y = (arg1).y - (arg2).y
#define CMPLXtimesreal(arg,real,out)   \
    (out).x = (arg).x*(real);\
    (out).y = (arg).y*(real)

#define CMPLXrecip(arg,out)    \
   { double denom; denom = sqr((arg).x) + sqr((arg).y);\
     if(denom==0.0) {(out).x = 1.0e10;(out).y = 1.0e10;}else\
    { (out).x =  (arg).x/denom;\
     (out).y = -(arg).y/denom;}}

extern int xshift, yshift;
extern int biomorph;
extern int forcesymmetry;
extern int symmetry;
_LCMPLX lcoefficient,lold,lnew,lparm, linit,ltmp,ltmp2,lparm2;
long ltempsqrx,ltempsqry;
extern int decomp[];
extern double param[];
extern int potflag;			       /* potential enabled? */
extern double f_radius,f_xcenter,f_ycenter;    /* inversion radius, center */
extern double xxmin,xxmax,yymin,yymax;	       /* corners */
extern int overflow;
extern int integerfractal;	/* TRUE if fractal uses integer math */

int maxcolor;
int root, degree,basin;
double floatmin,floatmax;
double roverd, d1overd, threshold;
_CMPLX tmp2;
extern _CMPLX init,tmp,old,new,saved,jbcfp;
_CMPLX coefficient;
_CMPLX	staticroots[16]; /* roots array for degree 16 or less */
_CMPLX	*roots = staticroots;
struct MPC	*MPCroots;
extern int color, row, col;
extern int invert;
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;
long FgHalf;

_CMPLX one;
_CMPLX pwr;
_CMPLX Coefficient;

extern int	colors; 			/* maximum colors available */
extern int	inside; 			/* "inside" color to use    */
extern int	outside;			/* "outside" color to use   */
extern int	finattract;
extern int	fractype;			/* fractal type */
extern int	debugflag;			/* for debugging purposes */

extern double	param[];		/* parameters */
extern long	far *lx0, far *ly0;		/* X, Y points */
extern long	far *lx1, far *ly1;		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	delmin; 			/* min(max(dx),max(dy) */
extern double	ddelmin;			/* min(max(dx),max(dy) */
extern long	fudge;				/* fudge factor (2**n) */
extern int	bitshift;			/* bit shift for fudge */
int	bitshiftless1;			/* bit shift less 1 */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

#define modulus(z)	 (sqr((z).x)+sqr((z).y))
#define conjugate(pz)	((pz)->y = 0.0 - (pz)->y)
#define distance(z1,z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))
#define pMPsqr(z) (*pMPmul((z),(z)))
#define MPdistance(z1,z2)  (*pMPadd(pMPsqr(*pMPsub((z1).x,(z2).x)),pMPsqr(*pMPsub((z1).y,(z2).y))))

double twopi = PI*2.0;
int c_exp;


/* These are local but I don't want to pass them as parameters */
_CMPLX lambda;
extern double magnitude, rqlim, rqlim2;
_CMPLX parm,parm2;
_CMPLX *floatparm;
_LCMPLX *longparm; /* used here and in jb.c */
extern int (*calctype)();
extern unsigned long lm;		/* magnitude limit (CALCMAND) */

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

double sinx,cosx,sinhx,coshx;
double siny,cosy,sinhy,coshy;
double tmpexp;
double tempsqrx,tempsqry,tempsqrz,tempsqrt;

double foldxinitx,foldyinity,foldxinity,foldyinitx;
long oldxinitx,oldyinity,oldxinity,oldyinitx;
long longtmp;
extern long lmagnitud, llimit, llimit2, l16triglim;
extern periodicitycheck;
extern char floatflag;

/* These are for quaternions */
double qc,qci,qcj,qck;

/* temporary variables for trig use */
long lcosx, lcoshx, lsinx, lsinhx;
long lcosy, lcoshy, lsiny, lsinhy;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/
extern	      int      attractors; /* number of finite attractors   */
extern _CMPLX  attr[];	   /* finite attractor values (f.p) */
extern _LCMPLX lattr[];	  /* finite attractor values (int) */
extern int    attrperiod[]; /* finite attractor period */

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
_CMPLX	T_Cm1;	      /* 3 * (floatparm - 1)		    */
_CMPLX	T_Cm2;	      /* 3 * (floatparm - 2)		    */
_CMPLX	T_Cm1Cm2;     /* (floatparm - 1) * (floatparm - 2) */

void FloatPreCalcMagnet2() /* precalculation for Magnet2 (M & J) for speed */
  {
    T_Cm1.x = floatparm->x - 1.0;   T_Cm1.y = floatparm->y;
    T_Cm2.x = floatparm->x - 2.0;   T_Cm2.y = floatparm->y;
    T_Cm1Cm2.x = (T_Cm1.x * T_Cm2.x) - (T_Cm1.y * T_Cm2.y);
    T_Cm1Cm2.y = (T_Cm1.x * T_Cm2.y) + (T_Cm1.y * T_Cm2.x);
    T_Cm1.x += T_Cm1.x + T_Cm1.x;   T_Cm1.y += T_Cm1.y + T_Cm1.y;
    T_Cm2.x += T_Cm2.x + T_Cm2.x;   T_Cm2.y += T_Cm2.y + T_Cm2.y;
  }


/* -------------------------------------------------------------------- */
/*		Bailout Routines Macros 												*/
/* -------------------------------------------------------------------- */

static int near floatbailout()
{
   if ( ( magnitude = ( tempsqrx=sqr(new.x) )
		    + ( tempsqry=sqr(new.y) ) ) >= rqlim ) return(1);
   old = new;
   return(0);
}

/* longbailout() is equivalent to next */
#define LONGBAILOUT()	\
   ltempsqrx = lsqr(lnew.x); ltempsqry = lsqr(lnew.y);\
   lmagnitud = ltempsqrx + ltempsqry;\
   if (lmagnitud >= llimit || lmagnitud < 0 || labs(lnew.x) > llimit2\
	 || labs(lnew.y) > llimit2 || overflow) \
	       { overflow=0;return(1);}\
   lold = lnew;

#define FLOATTRIGBAILOUT()  \
   if (fabs(old.y) >= rqlim2) return(1);

#define LONGTRIGBAILOUT()  \
   if(labs(lold.y) >= llimit2 || overflow) { overflow=0;return(1);}

#define LONGXYTRIGBAILOUT()  \
   if(labs(lold.x) >= llimit2 || labs(lold.y) >= llimit2 || overflow)\
	{ overflow=0;return(1);}

#define FLOATXYTRIGBAILOUT()  \
   if (fabs(old.x) >= rqlim2 || fabs(old.y) >= rqlim2) return(1);

#define FLOATHTRIGBAILOUT()  \
   if (fabs(old.x) >= rqlim2) return(1);

#define LONGHTRIGBAILOUT()  \
   if(labs(lold.x) >= llimit2 || overflow) { overflow=0;return(1);}

#define TRIG16CHECK(X)	\
      if(labs((X)) > l16triglim || overflow) { overflow=0;return(1);}

#define FLOATEXPBAILOUT()  \
   if (fabs(old.y) >= 1.0e8) return(1);\
   if (fabs(old.x) >= 6.4e2) return(1);

#define LONGEXPBAILOUT()  \
   if (labs(lold.y) >= (1000L<<bitshift)) return(1);\
   if (labs(lold.x) >=	  (8L<<bitshift)) return(1);

#if 0
/* this define uses usual trig instead of fast trig */
#define FPUsincos(px,psinx,pcosx) \
   *(psinx) = sin(*(px));\
   *(pcosx) = cos(*(px));

#define FPUsinhcosh(px,psinhx,pcoshx) \
   *(psinhx) = sinh(*(px));\
   *(pcoshx) = cosh(*(px));
#endif

#define LTRIGARG(X)    \
   if(labs((X)) > l16triglim)\
   {\
      double tmp;\
      tmp = (X);\
      tmp /= fudge;\
      tmp = fmod(tmp,twopi);\
      tmp *= fudge;\
      (X) = tmp;\
   }\

static int near Halleybailout()
{
   if ( fabs(modulus(new)-modulus(old)) < parm2.x)
      return(1);
   old = new;
   return(0);
}

#ifndef XFRACT
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
struct MPC mpcold, mpcnew, mpctmp, mpctmp1;
struct MP mptmpparm2x;

static int near MPCHalleybailout()
{
static struct MP mptmpbailout;
   mptmpbailout = *MPabs(*pMPsub(MPCmod(mpcnew), MPCmod(mpcold)));
   if (pMPcmp(mptmpbailout, mptmpparm2x) < 0)
      return(1);
   mpcold = mpcnew;
   return(0);
}
#endif

/* -------------------------------------------------------------------- */
/*		Fractal (once per iteration) routines			*/
/* -------------------------------------------------------------------- */
static double xt, yt, t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(_CMPLX *base, int exp, _CMPLX *result)
{
    if (exp<0) {
	cpower(base,-exp,result);
	CMPLXrecip(*result,*result);
	return;
    }

    xt = base->x;   yt = base->y;

    if (exp & 1)
    {
       result->x = xt;
       result->y = yt;
    }
    else
    {
       result->x = 1.0;
       result->y = 0.0;
    }

    exp >>= 1;
    while (exp)
    {
	t2 = xt * xt - yt * yt;
	yt = 2 * xt * yt;
	xt = t2;

	if (exp & 1)
	{
	    t2 = xt * result->x - yt * result->y;
	    result->y = result->y * xt + yt * result->x;
	    result->x = t2;
	}
	exp >>= 1;
    }
}
/* long version */
static long lxt, lyt, lt2;

lcpower(_LCMPLX *base, int exp, _LCMPLX *result, int bitshift)
{
    static long maxarg;
    maxarg = 64L<<bitshift;

    overflow = 0;
    lxt = base->x;   lyt = base->y;

    if (exp & 1)
    {
       result->x = lxt;
       result->y = lyt;
    }
    else
    {
       result->x = 1L<<bitshift;
       result->y = 0L;
    }

    exp >>= 1;
    while (exp)
    {
	/*
	if(labs(lxt) >= maxarg || labs(lyt) >= maxarg)
	   return(-1);
	*/
	lt2 = multiply(lxt, lxt, bitshift) - multiply(lyt,lyt,bitshift);
	lyt = multiply(lxt,lyt,bitshiftless1);
	if(overflow)
	   return(overflow);
	lxt = lt2;

	if (exp & 1)
	{
	    lt2 = multiply(lxt,result->x, bitshift) - multiply(lyt,result->y,bitshift);
	    result->y = multiply(result->y,lxt,bitshift) + multiply(lyt,result->x,bitshift);
	    result->x = lt2;
	}
	exp >>= 1;
    }
    if(result->x == 0 && result->y == 0)
       overflow = 1;
    return(overflow);
}
#if 0
z_to_the_z(_CMPLX *z, _CMPLX *out)
{
    static _CMPLX tmp1,tmp2;
    /* raises complex z to the z power */
    int errno_xxx;
    errno_xxx = 0;

    if(fabs(z->x) < DBL_EPSILON) return(-1);

    /* log(x + iy) = 1/2(log(x*x + y*y) + i(arc_tan(y/x)) */
    tmp1.x = .5*log(sqr(z->x)+sqr(z->y));

    /* the fabs in next line added to prevent discontinuity in image */
    tmp1.y = atan(fabs(z->y/z->x));

    /* log(z)*z */
    tmp2.x = tmp1.x * z->x - tmp1.y * z->y;
    tmp2.y = tmp1.x * z->y + tmp1.y * z->x;

    /* z*z = e**(log(z)*z) */
    /* e**(x + iy) =  e**x * (cos(y) + isin(y)) */

    tmpexp = exp(tmp2.x);

    FPUsincos(&tmp2.y,&siny,&cosy);
    out->x = tmpexp*cosy;
    out->y = tmpexp*siny;
    return(errno_xxx);
}
#endif

int complex_div(_CMPLX arg1,_CMPLX arg2,_CMPLX *pz);
int complex_mult(_CMPLX arg1,_CMPLX arg2,_CMPLX *pz);

#ifdef XFRACT /* fractint uses the NewtonFractal2 code in newton.asm */

/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-fudge)) + lsqr(((z).y)))


int NewtonFractal2()
{
    static char start=1;
    if(start)
    {
       start = 0;
    }
    cpower(&old, degree-1, &tmp);
    complex_mult(tmp, old, &new);

    if (DIST1(new) < threshold)
    {
       if(fractype==NEWTBASIN || fractype==MPNEWTBASIN)
       {
	  int tmpcolor;
	  int i;
	  tmpcolor = -1;
	  /* this code determines which degree-th root of root the
	     Newton formula converges to. The roots of a 1 are
	     distributed on a circle of radius 1 about the origin. */
	  for(i=0;i<degree;i++)
	     /* color in alternating shades with iteration according to
		which root of 1 it converged to */
	      if(distance(roots[i],old) < threshold)
	      {
		  if (basin==2) {
	              tmpcolor = 1+(i&7)+((color&1)<<3);
		  } else {
		      tmpcolor = 1+i;
		  }
		  break;
	      }
	   if(tmpcolor == -1)
	      color = maxcolor;
	   else
	      color = tmpcolor;
       }
       return(1);
    }
    new.x = d1overd * new.x + roverd;
    new.y *= d1overd;

    /* Watch for divide underflow */
    if ((t2 = tmp.x * tmp.x + tmp.y * tmp.y) < FLT_MIN)
      return(1);
    else
    {
	t2 = 1.0 / t2;
	old.x = t2 * (new.x * tmp.x + new.y * tmp.y);
	old.y = t2 * (new.y * tmp.x - new.x * tmp.y);
    }
    return(0);
}



#endif /* newton code only used by xfractint */

complex_mult(arg1,arg2,pz)
_CMPLX arg1,arg2,*pz;
{
   pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
   pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
   return(0);
}


complex_div(numerator,denominator,pout)
_CMPLX numerator,denominator,*pout;
{
   double mod;
   if((mod = modulus(denominator)) < FLT_MIN)
      return(1);
   conjugate(&denominator);
   complex_mult(numerator,denominator,pout);
   pout->x = pout->x/mod;
   pout->y = pout->y/mod;
   return(0);
}

#ifndef XFRACT
struct MP mproverd, mpd1overd, mpthreshold,sqrmpthreshold;
struct MP mpt2;
struct MP mpone;
extern struct MPC MPCone;
extern int MPOverflow;
#endif

int MPCNewtonFractal()
{
#ifndef XFRACT
    MPOverflow = 0;
    mpctmp   = MPCpow(mpcold,degree-1);

    mpcnew.x = *pMPsub(*pMPmul(mpctmp.x,mpcold.x),*pMPmul(mpctmp.y,mpcold.y));
    mpcnew.y = *pMPadd(*pMPmul(mpctmp.x,mpcold.y),*pMPmul(mpctmp.y,mpcold.x));
    mpctmp1.x = *pMPsub(mpcnew.x, MPCone.x);
    mpctmp1.y = *pMPsub(mpcnew.y, MPCone.y);
    if(pMPcmp(MPCmod(mpctmp1),mpthreshold)< 0)
    {
      if(fractype==MPNEWTBASIN)
      {
	 int tmpcolor;
	 int i;
	 tmpcolor = -1;
	 for(i=0;i<degree;i++)
	     if(pMPcmp(MPdistance(MPCroots[i],mpcold),mpthreshold) < 0)
	     {
	    if(basin==2)
		   tmpcolor = 1+(i&7) + ((color&1)<<3);
		else
	       tmpcolor = 1+i;
		    break;
	     }
	  if(tmpcolor == -1)
	     color = maxcolor;
	  else
	     color = tmpcolor;
      }
       return(1);
    }

    mpcnew.x = *pMPadd(*pMPmul(mpd1overd,mpcnew.x),mproverd);
    mpcnew.y = *pMPmul(mpcnew.y,mpd1overd);
    mpt2 = MPCmod(mpctmp);
    mpt2 = *pMPdiv(mpone,mpt2);
    mpcold.x = *pMPmul(mpt2,(*pMPadd(*pMPmul(mpcnew.x,mpctmp.x),*pMPmul(mpcnew.y,mpctmp.y))));
    mpcold.y = *pMPmul(mpt2,(*pMPsub(*pMPmul(mpcnew.y,mpctmp.x),*pMPmul(mpcnew.x,mpctmp.y))));
    new.x = *pMP2d(mpcold.x);
    new.y = *pMP2d(mpcold.y);
    return(MPOverflow);
#endif
}


Barnsley1Fractal()
{
#ifndef XFRACT
   /* Barnsley's Mandelbrot type M1 from "Fractals
   Everywhere" by Michael Barnsley, p. 322 */

   /* calculate intermediate products */
   oldxinitx   = multiply(lold.x, longparm->x, bitshift);
   oldyinity   = multiply(lold.y, longparm->y, bitshift);
   oldxinity   = multiply(lold.x, longparm->y, bitshift);
   oldyinitx   = multiply(lold.y, longparm->x, bitshift);
   /* orbit calculation */
   if(lold.x >= 0)
   {
      lnew.x = (oldxinitx - longparm->x - oldyinity);
      lnew.y = (oldyinitx - longparm->y + oldxinity);
   }
   else
   {
      lnew.x = (oldxinitx + longparm->x - oldyinity);
      lnew.y = (oldyinitx + longparm->y + oldxinity);
   }
   return(longbailout());
#endif
}

Barnsley1FPFractal()
{
   /* Barnsley's Mandelbrot type M1 from "Fractals
   Everywhere" by Michael Barnsley, p. 322 */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

   /* calculate intermediate products */
   foldxinitx = old.x * floatparm->x;
   foldyinity = old.y * floatparm->y;
   foldxinity = old.x * floatparm->y;
   foldyinitx = old.y * floatparm->x;
   /* orbit calculation */
   if(old.x >= 0)
   {
      new.x = (foldxinitx - floatparm->x - foldyinity);
      new.y = (foldyinitx - floatparm->y + foldxinity);
   }
   else
   {
      new.x = (foldxinitx + floatparm->x - foldyinity);
      new.y = (foldyinitx + floatparm->y + foldxinity);
   }
   return(floatbailout());
}

Barnsley2Fractal()
{
#ifndef XFRACT
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 331, example 4.2 */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

   /* calculate intermediate products */
   oldxinitx   = multiply(lold.x, longparm->x, bitshift);
   oldyinity   = multiply(lold.y, longparm->y, bitshift);
   oldxinity   = multiply(lold.x, longparm->y, bitshift);
   oldyinitx   = multiply(lold.y, longparm->x, bitshift);

   /* orbit calculation */
   if(oldxinity + oldyinitx >= 0)
   {
      lnew.x = oldxinitx - longparm->x - oldyinity;
      lnew.y = oldyinitx - longparm->y + oldxinity;
   }
   else
   {
      lnew.x = oldxinitx + longparm->x - oldyinity;
      lnew.y = oldyinitx + longparm->y + oldxinity;
   }
   return(longbailout());
#endif
}

Barnsley2FPFractal()
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 331, example 4.2 */

   /* calculate intermediate products */
   foldxinitx = old.x * floatparm->x;
   foldyinity = old.y * floatparm->y;
   foldxinity = old.x * floatparm->y;
   foldyinitx = old.y * floatparm->x;

   /* orbit calculation */
   if(foldxinity + foldyinitx >= 0)
   {
      new.x = foldxinitx - floatparm->x - foldyinity;
      new.y = foldyinitx - floatparm->y + foldxinity;
   }
   else
   {
      new.x = foldxinitx + floatparm->x - foldyinity;
      new.y = foldyinitx + floatparm->y + foldxinity;
   }
   return(floatbailout());
}

JuliaFractal()
{
#ifndef XFRACT
   /* used for C prototype of fast integer math routines for classic
      Mandelbrot and Julia */
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   return(longbailout());
#elif !defined(__386BSD__)
   fprintf(stderr,"JuliaFractal called\n");
   exit(-1);
#endif
}

JuliafpFractal()
{
   /* floating point version of classical Mandelbrot/Julia */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
   new.x = tempsqrx - tempsqry + floatparm->x;
   new.y = 2.0 * old.x * old.y + floatparm->y;
   return(floatbailout());
}

static double f(double x, double y)
{
   return(x - x*y);
}

static double g(double x, double y)
{
   return(-y + x*y);
}

LambdaFPFractal()
{
   /* variation of classical Mandelbrot/Julia */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

   tempsqrx = old.x - tempsqrx + tempsqry;
   tempsqry = -(old.y * old.x);
   tempsqry += tempsqry + old.y;

   new.x = floatparm->x * tempsqrx - floatparm->y * tempsqry;
   new.y = floatparm->x * tempsqry + floatparm->y * tempsqrx;
   return(floatbailout());
}

LambdaFractal()
{
#ifndef XFRACT
   /* variation of classical Mandelbrot/Julia */

   /* in complex math) temp = Z * (1-Z) */
   ltempsqrx = lold.x - ltempsqrx + ltempsqry;
   ltempsqry = lold.y
		 - multiply(lold.y, lold.x, bitshiftless1);
   /* (in complex math) Z = Lambda * Z */
   lnew.x = multiply(longparm->x, ltempsqrx, bitshift)
	- multiply(longparm->y, ltempsqry, bitshift);
   lnew.y = multiply(longparm->x, ltempsqry, bitshift)
	+ multiply(longparm->y, ltempsqrx, bitshift);
   return(longbailout());
#endif
}

SierpinskiFractal()
{
#ifndef XFRACT
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
   lnew.x = (lold.x << 1);		/* new.x = 2 * old.x  */
   lnew.y = (lold.y << 1);		/* new.y = 2 * old.y  */
   if(lold.y > ltmp.y)	/* if old.y > .5 */
      lnew.y = lnew.y - ltmp.x; /* new.y = 2 * old.y - 1 */
   else if(lold.x > ltmp.y)	/* if old.x > .5 */
      lnew.x = lnew.x - ltmp.x; /* new.x = 2 * old.x - 1 */
   /* end barnsley code */
   return(longbailout());
#endif
}

SierpinskiFPFractal()
{
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */

   new.x = old.x + old.x;
   new.y = old.y + old.y;
   if(old.y > .5)
      new.y = new.y - 1;
   else if (old.x > .5)
      new.x = new.x - 1;

   /* end barnsley code */
   return(floatbailout());
}

LambdaexponentFractal()
{
   /* found this in  "Science of Fractal Images" */
   FLOATEXPBAILOUT();
   FPUsincos  (&old.y,&siny,&cosy);

   if (old.x >= rqlim && cosy >= 0.0) return(1);
   tmpexp = exp(old.x);
   tmp.x = tmpexp*cosy;
   tmp.y = tmpexp*siny;

   /*multiply by lamda */
   new.x = floatparm->x*tmp.x - floatparm->y*tmp.y;
   new.y = floatparm->y*tmp.x + floatparm->x*tmp.y;
   old = new;
   return(0);
}

LongLambdaexponentFractal()
{
#ifndef XFRACT
   /* found this in  "Science of Fractal Images" */
   LONGEXPBAILOUT();

   SinCos086  (lold.y, &lsiny,	&lcosy);

   if (lold.x >= llimit && lcosy >= 0L) return(1);
   longtmp = Exp086(lold.x);

   ltmp.x = multiply(longtmp,	   lcosy,   bitshift);
   ltmp.y = multiply(longtmp,	   lsiny,   bitshift);

   lnew.x  = multiply(longparm->x, ltmp.x, bitshift)
	   - multiply(longparm->y, ltmp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltmp.y, bitshift)
	   + multiply(longparm->y, ltmp.x, bitshift);
   lold = lnew;
   return(0);
#endif
}

FloatTrigPlusExponentFractal()
{
   /* another Scientific American biomorph type */
   /* z(n+1) = e**z(n) + trig(z(n)) + C */

   if (fabs(old.x) >= 6.4e2) return(1); /* DOMAIN errors */
   tmpexp = exp(old.x);
   FPUsincos  (&old.y,&siny,&cosy);
   CMPLXtrig0(old,new);

   /*new =   trig(old) + e**old + C  */
   new.x += tmpexp*cosy + floatparm->x;
   new.y += tmpexp*siny + floatparm->y;
   return(floatbailout());
}


LongTrigPlusExponentFractal()
{
#ifndef XFRACT
   /* calculate exp(z) */

   /* domain check for fast transcendental functions */
   TRIG16CHECK(lold.x);
   TRIG16CHECK(lold.y);

   longtmp = Exp086(lold.x);
   SinCos086  (lold.y, &lsiny,	&lcosy);
   LCMPLXtrig0(lold,lnew);
   lnew.x += multiply(longtmp,	  lcosy,   bitshift) + longparm->x;
   lnew.y += multiply(longtmp,	  lsiny,   bitshift) + longparm->y;
   return(longbailout());
#endif
}

MarksLambdaFractal()
{
   /* Mark Peterson's variation of "lambda" function */

   /* Z1 = (C^(exp-1) * Z**2) + C */
   ltmp.x = ltempsqrx - ltempsqry;
   ltmp.y = multiply(lold.x ,lold.y ,bitshiftless1);

   lnew.x = multiply(lcoefficient.x, ltmp.x, bitshift)
	- multiply(lcoefficient.y, ltmp.y, bitshift) + longparm->x;
   lnew.y = multiply(lcoefficient.x, ltmp.y, bitshift)
	+ multiply(lcoefficient.y, ltmp.x, bitshift) + longparm->y;

   return(longbailout());
}

MarksLambdafpFractal()
{
   /* Mark Peterson's variation of "lambda" function */

   /* Z1 = (C^(exp-1) * Z**2) + C */
   tmp.x = tempsqrx - tempsqry;
   tmp.y = old.x * old.y *2;

   new.x = coefficient.x * tmp.x - coefficient.y * tmp.y + floatparm->x;
   new.y = coefficient.x * tmp.y + coefficient.y * tmp.x + floatparm->y;

   return(floatbailout());
}


long XXOne, FgOne, FgTwo;

UnityFractal()
{
#ifndef XFRACT
   /* brought to you by Mark Peterson - you won't find this in any fractal
      books unless they saw it here first - Mark invented it! */
   XXOne = multiply(lold.x, lold.x, bitshift) + multiply(lold.y, lold.y, bitshift);
   if((XXOne > FgTwo) || (labs(XXOne - FgOne) < delmin))
      return(1);
   lold.y = multiply(FgTwo - XXOne, lold.x, bitshift);
   lold.x = multiply(FgTwo - XXOne, lold.y, bitshift);
   lnew=lold;  /* TW added this line */
   return(0);
#endif
}

#define XXOne new.x

UnityfpFractal()
{
   /* brought to you by Mark Peterson - you won't find this in any fractal
      books unless they saw it here first - Mark invented it! */

   XXOne = sqr(old.x) + sqr(old.y);
   if((XXOne > 2.0) || (fabs(XXOne - 1.0) < ddelmin))
      return(1);
   old.y = (2.0 - XXOne)* old.x;
   old.x = (2.0 - XXOne)* old.y;
   new=old;  /* TW added this line */
   return(0);
}

#undef XXOne

Mandel4Fractal()
{
   /* By writing this code, Bert has left behind the excuse "don't
      know what a fractal is, just know how to make'em go fast".
      Bert is hereby declared a bonafide fractal expert! Supposedly
      this routine calculates the Mandelbrot/Julia set based on the
      polynomial z**4 + lambda, but I wouldn't know -- can't follow
      all that integer math speedup stuff - Tim */

   /* first, compute (x + iy)**2 */
   lnew.x  = ltempsqrx - ltempsqry;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1);
   if (longbailout()) return(1);

   /* then, compute ((x + iy)**2)**2 + lambda */
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   return(longbailout());
}

Mandel4fpFractal()
{
   /* first, compute (x + iy)**2 */
   new.x  = tempsqrx - tempsqry;
   new.y = old.x*old.y*2;
   if (floatbailout()) return(1);

   /* then, compute ((x + iy)**2)**2 + lambda */
   new.x  = tempsqrx - tempsqry + floatparm->x;
   new.y =  old.x*old.y*2 + floatparm->y;
   return(floatbailout());
}

floatZtozPluszpwrFractal()
{
   cpower(&old,(int)param[2],&new);
   old = ComplexPower(old,old);
   new.x = new.x + old.x +floatparm->x;
   new.y = new.y + old.y +floatparm->y;
   return(floatbailout());
}

longZpowerFractal()
{
#ifndef XFRACT
   if(lcpower(&lold,c_exp,&lnew,bitshift))
      lnew.x = lnew.y = 8L<<bitshift;
   lnew.x += longparm->x;
   lnew.y += longparm->y;
   return(longbailout());
#endif
}

longCmplxZpowerFractal()
{
#ifndef XFRACT
   _CMPLX x, y;

   x.x = (double)lold.x / fudge;
   x.y = (double)lold.y / fudge;
   y.x = (double)lparm2.x / fudge;
   y.y = (double)lparm2.y / fudge;
   x = ComplexPower(x, y);
   if(fabs(x.x) < fgLimit && fabs(x.y) < fgLimit) {
      lnew.x = (long)(x.x * fudge);
      lnew.y = (long)(x.y * fudge);
   }
   else
      overflow = 1;
   lnew.x += longparm->x;
   lnew.y += longparm->y;
   return(longbailout());
#endif
}

floatZpowerFractal()
{
   cpower(&old,c_exp,&new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

floatCmplxZpowerFractal()
{
   new = ComplexPower(old, parm2);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

Barnsley3Fractal()
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 292, example 4.1 */

   /* calculate intermediate products */
   oldxinitx   = multiply(lold.x, lold.x, bitshift);
   oldyinity   = multiply(lold.y, lold.y, bitshift);
   oldxinity   = multiply(lold.x, lold.y, bitshift);

   /* orbit calculation */
   if(lold.x > 0)
   {
      lnew.x = oldxinitx   - oldyinity - fudge;
      lnew.y = oldxinity << 1;
   }
   else
   {
      lnew.x = oldxinitx - oldyinity - fudge
	   + multiply(longparm->x,lold.x,bitshift);
      lnew.y = oldxinity <<1;

      /* This term added by Tim Wegner to make dependent on the
	 imaginary part of the parameter. (Otherwise Mandelbrot
	 is uninteresting. */
      lnew.y += multiply(longparm->y,lold.x,bitshift);
   }
   return(longbailout());
}

Barnsley3FPFractal()
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 292, example 4.1 */


   /* calculate intermediate products */
   foldxinitx  = old.x * old.x;
   foldyinity  = old.y * old.y;
   foldxinity  = old.x * old.y;

   /* orbit calculation */
   if(old.x > 0)
   {
      new.x = foldxinitx - foldyinity - 1.0;
      new.y = foldxinity * 2;
   }
   else
   {
      new.x = foldxinitx - foldyinity -1.0 + floatparm->x * old.x;
      new.y = foldxinity * 2;

      /* This term added by Tim Wegner to make dependent on the
	 imaginary part of the parameter. (Otherwise Mandelbrot
	 is uninteresting. */
      new.y += floatparm->y * old.x;
   }
   return(floatbailout());
}

TrigPlusZsquaredFractal()
{
#ifndef XFRACT
   /* From Scientific American, July 1989 */
   /* A Biomorph			  */
   /* z(n+1) = trig(z(n))+z(n)**2+C	  */
   LCMPLXtrig0(lold,lnew);
   lnew.x += ltempsqrx - ltempsqry + longparm->x;
   lnew.y += multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   return(longbailout());
#endif
}

TrigPlusZsquaredfpFractal()
{
   /* From Scientific American, July 1989 */
   /* A Biomorph			  */
   /* z(n+1) = trig(z(n))+z(n)**2+C	  */

   CMPLXtrig0(old,new);
   new.x += tempsqrx - tempsqry + floatparm->x;
   new.y += 2.0 * old.x * old.y + floatparm->y;
   return(floatbailout());
}

Richard8fpFractal()
{
   /*  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50} */
   CMPLXtrig0(old,new);
/*   CMPLXtrig1(*floatparm,tmp); */
   new.x += tmp.x;
   new.y += tmp.y;
   return(floatbailout());
}

Richard8Fractal()
{
#ifndef XFRACT
   /*  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50} */
   LCMPLXtrig0(lold,lnew);
/*   LCMPLXtrig1(*longparm,ltmp); */
   lnew.x += ltmp.x;
   lnew.y += ltmp.y;
   return(longbailout());
#endif
}

PopcornFractal()
{
   extern int row;
   tmp = old;
   tmp.x *= 3.0;
   tmp.y *= 3.0;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   tmp.x = sinx/cosx + old.x;
   tmp.y = siny/cosy + old.y;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   new.x = old.x - parm.x*siny;
   new.y = old.y - parm.x*sinx;
   /*
   new.x = old.x - parm.x*sin(old.y+tan(3*old.y));
   new.y = old.y - parm.x*sin(old.x+tan(3*old.x));
   */
   if(plot == noplot)
   {
      plot_orbit(new.x,new.y,1+row%colors);
      old = new;
   }
   else
   /* FLOATBAILOUT(); */
   /* PB The above line was weird, not what it seems to be!  But, bracketing
	 it or always doing it (either of which seem more likely to be what
	 was intended) changes the image for the worse, so I'm not touching it.
	 Same applies to int form in next routine. */
   /* PB later: recoded inline, still leaving it weird */
      tempsqrx = sqr(new.x);
   tempsqry = sqr(new.y);
   if((magnitude = tempsqrx + tempsqry) >= rqlim) return(1);
   old = new;
   return(0);
}

LPopcornFractal()
{
#ifndef XFRACT
   extern int row;
   ltmp = lold;
   ltmp.x *= 3L;
   ltmp.y *= 3L;
   LTRIGARG(ltmp.x);
   LTRIGARG(ltmp.y);
   SinCos086(ltmp.x,&lsinx,&lcosx);
   SinCos086(ltmp.y,&lsiny,&lcosy);
   ltmp.x = divide(lsinx,lcosx,bitshift) + lold.x;
   ltmp.y = divide(lsiny,lcosy,bitshift) + lold.y;
   LTRIGARG(ltmp.x);
   LTRIGARG(ltmp.y);
   SinCos086(ltmp.x,&lsinx,&lcosx);
   SinCos086(ltmp.y,&lsiny,&lcosy);
   lnew.x = lold.x - multiply(lparm.x,lsiny,bitshift);
   lnew.y = lold.y - multiply(lparm.x,lsinx,bitshift);
   if(plot == noplot)
   {
      iplot_orbit(lnew.x,lnew.y,1+row%colors);
      lold = lnew;
   }
   else
      LONGBAILOUT();
   /* PB above still the old way, is weird, see notes in FP popcorn case */
   return(0);
#endif
}

int MarksCplxMand(void)
{
   tmp.x = tempsqrx - tempsqry;
   tmp.y = 2*old.x*old.y;
   FPUcplxmul(&tmp, &Coefficient, &new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

int SpiderfpFractal(void)
{
   /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
   new.x = tempsqrx - tempsqry + tmp.x;
   new.y = 2 * old.x * old.y + tmp.y;
   tmp.x = tmp.x/2 + new.x;
   tmp.y = tmp.y/2 + new.y;
   return(floatbailout());
}

SpiderFractal(void)
{
#ifndef XFRACT
   /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
   lnew.x  = ltempsqrx - ltempsqry + ltmp.x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y;
   ltmp.x = (ltmp.x >> 1) + lnew.x;
   ltmp.y = (ltmp.y >> 1) + lnew.y;
   return(longbailout());
#endif
}

TetratefpFractal()
{
   /* Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) } */
   new = ComplexPower(*floatparm,old);
   return(floatbailout());
}

ZXTrigPlusZFractal()
{
#ifndef XFRACT
   /* z = (p1*z*trig(z))+p2*z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)	     */
   LCMPLXmult(lparm,ltmp,ltmp);      /* ltmp  = p1*trig(old)	      */
   LCMPLXmult(lold,ltmp,ltmp2);      /* ltmp2 = p1*old*trig(old)      */
   LCMPLXmult(lparm2,lold,ltmp);     /* ltmp  = p2*old		      */
   LCMPLXadd(ltmp2,ltmp,lnew);	     /* lnew  = p1*trig(old) + p2*old */
   return(longbailout());
#endif
}

ScottZXTrigPlusZFractal()
{
#ifndef XFRACT
   /* z = (z*trig(z))+z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)       */
   LCMPLXmult(lold,ltmp,lnew);	     /* lnew  = old*trig(old)	*/
   LCMPLXadd(lnew,lold,lnew);	     /* lnew  = trig(old) + old */
   return(longbailout());
#endif
}

SkinnerZXTrigSubZFractal()
{
#ifndef XFRACT
   /* z = (z*trig(z))-z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)       */
   LCMPLXmult(lold,ltmp,lnew);	     /* lnew  = old*trig(old)	*/
   LCMPLXsub(lnew,lold,lnew);	     /* lnew  = trig(old) - old */
   return(longbailout());
#endif
}

ZXTrigPlusZfpFractal()
{
   /* z = (p1*z*trig(z))+p2*z */
   CMPLXtrig0(old,tmp); 	 /* tmp  = trig(old)		 */
   CMPLXmult(parm,tmp,tmp);	 /* tmp  = p1*trig(old) 	 */
   CMPLXmult(old,tmp,tmp2);	 /* tmp2 = p1*old*trig(old)	 */
   CMPLXmult(parm2,old,tmp);	 /* tmp  = p2*old		 */
   CMPLXadd(tmp2,tmp,new);	 /* new  = p1*trig(old) + p2*old */
   return(floatbailout());
}

ScottZXTrigPlusZfpFractal()
{
   /* z = (z*trig(z))+z */
   CMPLXtrig0(old,tmp); 	/* tmp	= trig(old)	  */
   CMPLXmult(old,tmp,new);	 /* new  = old*trig(old)   */
   CMPLXadd(new,old,new);	 /* new  = trig(old) + old */
   return(floatbailout());
}

SkinnerZXTrigSubZfpFractal()
{
   /* z = (z*trig(z))-z */
   CMPLXtrig0(old,tmp); 	/* tmp	= trig(old)	  */
   CMPLXmult(old,tmp,new);	 /* new  = old*trig(old)   */
   CMPLXsub(new,old,new);	 /* new  = trig(old) - old */
   return(floatbailout());
}

Sqr1overTrigFractal()
{
#ifndef XFRACT
   /* z = sqr(1/trig(z)) */
   LCMPLXtrig0(lold,lold);
   LCMPLXrecip(lold,lold);
   LCMPLXsqr(lold,lnew);
   return(longbailout());
#endif
}

Sqr1overTrigfpFractal()
{
   /* z = sqr(1/trig(z)) */
   CMPLXtrig0(old,old);
   CMPLXrecip(old,old);
   CMPLXsqr(old,new);
   return(floatbailout());
}

TrigPlusTrigFractal()
{
#ifndef XFRACT
   /* z = trig(0,z)*p1+trig1(z)*p2 */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXmult(lparm,ltmp,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXmult(lparm2,ltmp2,lold);
   LCMPLXadd(ltmp,lold,lnew);
   return(longbailout());
#endif
}

TrigPlusTrigfpFractal()
{
   /* z = trig0(z)*p1+trig1(z)*p2 */
   CMPLXtrig0(old,tmp);
   CMPLXmult(parm,tmp,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(parm2,old,old);
   CMPLXadd(tmp,old,new);
   return(floatbailout());
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  JCO  5/6/92 */

LambdaTrigOrTrigFractal()
{
#ifndef XFRACT
   /* z = trig0(z)*p1 if mod(old) < p2.x and
          trig1(z)*p1 if mod(old) >= p2.x */
   if ((LCMPLXmod(lold)) < lparm2.x){
     LCMPLXtrig0(lold,ltmp);
     LCMPLXmult(*longparm,ltmp,lnew);}
   else{
     LCMPLXtrig1(lold,ltmp);
     LCMPLXmult(*longparm,ltmp,lnew);}
   return(longbailout());
#endif
}

LambdaTrigOrTrigfpFractal()
{
   /* z = trig0(z)*p1 if mod(old) < p2.x and
          trig1(z)*p1 if mod(old) >= p2.x */
   if (CMPLXmod(old) < parm2.x){
     CMPLXtrig0(old,old);
     FPUcplxmul(floatparm,&old,&new);}
   else{
     CMPLXtrig1(old,old);
     FPUcplxmul(floatparm,&old,&new);}
   return(floatbailout());
}

JuliaTrigOrTrigFractal()
{
#ifndef XFRACT
   /* z = trig0(z)+p1 if mod(old) < p2.x and
          trig1(z)+p1 if mod(old) >= p2.x */
   if (LCMPLXmod(lold) < lparm2.x){
     LCMPLXtrig0(lold,ltmp);
     LCMPLXadd(*longparm,ltmp,lnew);}
   else{
     LCMPLXtrig1(lold,ltmp);
     LCMPLXadd(*longparm,ltmp,lnew);}
   return(longbailout());
#endif
}

JuliaTrigOrTrigfpFractal()
{
   /* z = trig0(z)+p1 if mod(old) < p2.x and
          trig1(z)+p1 if mod(old) >= p2.x */
   if (CMPLXmod(old) < parm2.x){
     CMPLXtrig0(old,old);
     CMPLXadd(*floatparm,old,new);}
   else{
     CMPLXtrig1(old,old);
     CMPLXadd(*floatparm,old,new);}
   return(floatbailout());
}

static int AplusOne, Ap1deg;
static struct MP mpAplusOne, mpAp1deg, mptmpparmy;

int MPCHalleyFractal()
{
#ifndef XFRACT
   /*  X(X^a - 1) = 0, Halley Map */
   /*  a = parm.x,  relaxation coeff. = parm.y,  epsilon = parm2.x  */

int ihal;
struct MPC mpcXtoAlessOne, mpcXtoA;
struct MPC mpcXtoAplusOne; /* a-1, a, a+1 */
struct MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
struct MPC mpcHalnumer2, mpcHaldenom, mpctmp;

   MPOverflow = 0;
   mpcXtoAlessOne.x = mpcold.x;
   mpcXtoAlessOne.y = mpcold.y;
   for(ihal=2; ihal<degree; ihal++) {
     mpctmp.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x,mpcold.x),*pMPmul(mpcXtoAlessOne.y,mpcold.y));
     mpctmp.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x,mpcold.y),*pMPmul(mpcXtoAlessOne.y,mpcold.x));
     mpcXtoAlessOne.x = mpctmp.x;
     mpcXtoAlessOne.y = mpctmp.y;
   }
   mpcXtoA.x = *pMPsub(*pMPmul(mpcXtoAlessOne.x,mpcold.x),*pMPmul(mpcXtoAlessOne.y,mpcold.y));
   mpcXtoA.y = *pMPadd(*pMPmul(mpcXtoAlessOne.x,mpcold.y),*pMPmul(mpcXtoAlessOne.y,mpcold.x));
   mpcXtoAplusOne.x = *pMPsub(*pMPmul(mpcXtoA.x,mpcold.x),*pMPmul(mpcXtoA.y,mpcold.y));
   mpcXtoAplusOne.y = *pMPadd(*pMPmul(mpcXtoA.x,mpcold.y),*pMPmul(mpcXtoA.y,mpcold.x));

   mpcFX.x = *pMPsub(mpcXtoAplusOne.x, mpcold.x);
   mpcFX.y = *pMPsub(mpcXtoAplusOne.y, mpcold.y); /* FX = X^(a+1) - X  = F */

   mpcF2prime.x = *pMPmul(mpAp1deg, mpcXtoAlessOne.x); /* mpAp1deg in setup */
   mpcF2prime.y = *pMPmul(mpAp1deg, mpcXtoAlessOne.y);        /* F" */

   mpcF1prime.x = *pMPsub(*pMPmul(mpAplusOne, mpcXtoA.x), mpone);
   mpcF1prime.y = *pMPmul(mpAplusOne, mpcXtoA.y);                   /*  F'  */

   mpctmp.x = *pMPsub(*pMPmul(mpcF2prime.x,mpcFX.x),*pMPmul(mpcF2prime.y,mpcFX.y));
   mpctmp.y = *pMPadd(*pMPmul(mpcF2prime.x,mpcFX.y),*pMPmul(mpcF2prime.y,mpcFX.x));
   /*  F * F"  */

   mpcHaldenom.x = *pMPadd(mpcF1prime.x, mpcF1prime.x);
   mpcHaldenom.y = *pMPadd(mpcF1prime.y, mpcF1prime.y);      /*  2 * F'  */

   mpcHalnumer1 = MPCdiv(mpctmp, mpcHaldenom);        /*  F"F/2F'  */
   mpctmp.x = *pMPsub(mpcF1prime.x, mpcHalnumer1.x);
   mpctmp.y = *pMPsub(mpcF1prime.y, mpcHalnumer1.y); /*  F' - F"F/2F'  */
   mpcHalnumer2 = MPCdiv(mpcFX, mpctmp);

   mpctmp.x = *pMPmul(mptmpparmy,mpcHalnumer2.x); /* mptmpparmy is */
   mpctmp.y = *pMPmul(mptmpparmy,mpcHalnumer2.y); /* relaxation coef. */
   mpcnew.x = *pMPsub(mpcold.x, mpctmp.x);
   mpcnew.y = *pMPsub(mpcold.y, mpctmp.y);

   new.x = *pMP2d(mpcnew.x);
   new.y = *pMP2d(mpcnew.y);

   return(MPCHalleybailout()||MPOverflow);
#endif
}

HalleyFractal()
{
   /*  X(X^a - 1) = 0, Halley Map */
   /*  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x  */

int ihal;
_CMPLX XtoAlessOne, XtoA, XtoAplusOne; /* a-1, a, a+1 */
_CMPLX FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;

   XtoAlessOne = old;
   for(ihal=2; ihal<degree; ihal++) {
     FPUcplxmul(&old, &XtoAlessOne, &XtoAlessOne);
   }
   FPUcplxmul(&old, &XtoAlessOne, &XtoA);
   FPUcplxmul(&old, &XtoA, &XtoAplusOne);

   CMPLXsub(XtoAplusOne, old, FX);        /* FX = X^(a+1) - X  = F */
   F2prime.x = Ap1deg * XtoAlessOne.x; /* Ap1deg in setup */
   F2prime.y = Ap1deg * XtoAlessOne.y;        /* F" */

   F1prime.x = AplusOne * XtoA.x - 1.0;
   F1prime.y = AplusOne * XtoA.y;                             /*  F'  */

   FPUcplxmul(&F2prime, &FX, &Halnumer1);                  /*  F * F"  */
   Haldenom.x = F1prime.x + F1prime.x;
   Haldenom.y = F1prime.y + F1prime.y;                     /*  2 * F'  */

   FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         /*  F"F/2F'  */
   CMPLXsub(F1prime, Halnumer1, Halnumer2);          /*  F' - F"F/2F'  */
   FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
   new.x = old.x - (parm.y * Halnumer2.x); /* parm.y is relaxation coef. */
   new.y = old.y - (parm.y * Halnumer2.y);

   return(Halleybailout());
}

LongPhoenixFractal()
{
#ifndef XFRACT
/* z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n) */
   ltmp.x = multiply(lold.x, lold.y, bitshift);
   lnew.x = ltempsqrx-ltempsqry+longparm->x+multiply(longparm->y,ltmp2.x,bitshift);
   lnew.y = (ltmp.x + ltmp.x) + multiply(longparm->y,ltmp2.y,bitshift);
   ltmp2 = lold; /* set ltmp2 to Y value */
   return(longbailout());
#endif
}

PhoenixFractal()
{
/* z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n) */
   tmp.x = old.x * old.y;
   new.x = tempsqrx - tempsqry + floatparm->x + (floatparm->y * tmp2.x);
   new.y = (tmp.x + tmp.x) + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

LongPhoenixPlusFractal()
{
#ifndef XFRACT
/* z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n) */
int i;
_LCMPLX loldplus, lnewminus;
   loldplus = lold;
   ltmp = lold;
   for(i=1; i<degree; i++) { /* degree >= 2, degree=degree-1 in setup */
      LCMPLXmult(lold,ltmp,ltmp); /* = old^(degree-1) */
   }
   loldplus.x += longparm->x;
   LCMPLXmult(ltmp, loldplus, lnewminus);
   lnew.x = lnewminus.x + multiply(longparm->y,ltmp2.x,bitshift);
   lnew.y = lnewminus.y + multiply(longparm->y,ltmp2.y,bitshift);
   ltmp2 = lold; /* set ltmp2 to Y value */
   return(longbailout());
#endif
}

PhoenixPlusFractal()
{
/* z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldplus, newminus;
   oldplus = old;
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 2, degree=degree-1 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-1) */
   }
   oldplus.x += floatparm->x;
   FPUcplxmul(&tmp, &oldplus, &newminus);
   new.x = newminus.x + (floatparm->y * tmp2.x);
   new.y = newminus.y + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

LongPhoenixMinusFractal()
{
#ifndef XFRACT
/* z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n) */
int i;
_LCMPLX loldsqr, lnewminus;
   LCMPLXmult(lold,lold,loldsqr);
   ltmp = lold;
   for(i=1; i<degree; i++) { /* degree >= 3, degree=degree-2 in setup */
      LCMPLXmult(lold,ltmp,ltmp); /* = old^(degree-2) */
   }
   loldsqr.x += longparm->x;
   LCMPLXmult(ltmp, loldsqr, lnewminus);
   lnew.x = lnewminus.x + multiply(longparm->y,ltmp2.x,bitshift);
   lnew.y = lnewminus.y + multiply(longparm->y,ltmp2.y,bitshift);
   ltmp2 = lold; /* set ltmp2 to Y value */
   return(longbailout());
#endif
}

PhoenixMinusFractal()
{
/* z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldsqr, newminus;
   FPUcplxmul(&old, &old, &oldsqr);
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 3, degree=degree-2 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-2) */
   }
   oldsqr.x += floatparm->x;
   FPUcplxmul(&tmp, &oldsqr, &newminus);
   new.x = newminus.x + (floatparm->y * tmp2.x);
   new.y = newminus.y + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

ScottTrigPlusTrigFractal()
{
#ifndef XFRACT
   /* z = trig0(z)+trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,lold);
   LCMPLXadd(ltmp,lold,lnew);
   return(longbailout());
#endif
}

ScottTrigPlusTrigfpFractal()
{
   /* z = trig0(z)+trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXadd(tmp,tmp2,new);
   return(floatbailout());
}

SkinnerTrigSubTrigFractal()
{
#ifndef XFRACT
   /* z = trig(0,z)-trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXsub(ltmp,ltmp2,lnew);
   return(longbailout());
#endif
}

SkinnerTrigSubTrigfpFractal()
{
   /* z = trig0(z)-trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXsub(tmp,tmp2,new);
   return(floatbailout());
}


TrigXTrigfpFractal()
{
   /* z = trig0(z)*trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(tmp,old,new);
   return(floatbailout());
}

TrigXTrigFractal()
{
#ifndef XFRACT
   _LCMPLX ltmp2;
   /* z = trig0(z)*trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXmult(ltmp,ltmp2,lnew);
   if(overflow)
      TryFloatFractal(TrigXTrigfpFractal);
   return(longbailout());
#endif
}

 /* call float version of fractal if integer math overflow */
TryFloatFractal(int (*fpFractal)())
{
   overflow=0;
   /* lold had better not be changed! */
   old.x = lold.x; old.x /= fudge;
   old.y = lold.y; old.y /= fudge;
   tempsqrx = sqr(old.x);
   tempsqry = sqr(old.y);
   fpFractal();
   lnew.x = new.x/fudge;
   lnew.y = new.y/fudge;
   return(0);
}

/********************************************************************/
/*  Next six orbit functions are one type - extra functions are     */
/*    special cases written for speed.				    */
/********************************************************************/

TrigPlusSqrFractal() /* generalization of Scott and Skinner types */
{
#ifndef XFRACT
   /* { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT } */
   LCMPLXtrig0(lold,ltmp);     /* ltmp = trig(lold)			   */
   LCMPLXmult(lparm,ltmp,lnew); /* lnew = lparm*trig(lold)		    */
   LCMPLXsqr_old(ltmp); 	/* ltmp = sqr(lold)			    */
   LCMPLXmult(lparm2,ltmp,ltmp);/* ltmp = lparm2*sqr(lold)		    */
   LCMPLXadd(lnew,ltmp,lnew);	/* lnew = lparm*trig(lold)+lparm2*sqr(lold) */
   return(longbailout());
#endif
}

TrigPlusSqrfpFractal() /* generalization of Scott and Skinner types */
{
   /* { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,tmp);     /* tmp = trig(old)			   */
   CMPLXmult(parm,tmp,new); /* new = parm*trig(old)		   */
   CMPLXsqr_old(tmp);	     /* tmp = sqr(old)			    */
   CMPLXmult(parm2,tmp,tmp2); /* tmp = parm2*sqr(old)		     */
   CMPLXadd(new,tmp2,new);    /* new = parm*trig(old)+parm2*sqr(old) */
   return(floatbailout());
}

ScottTrigPlusSqrFractal()
{
#ifndef XFRACT
   /*  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT } */
   LCMPLXtrig0(lold,lnew);    /* lnew = trig(lold)	     */
   LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)	      */
   LCMPLXadd(ltmp,lnew,lnew);  /* lnew = trig(lold)+sqr(lold) */
   return(longbailout());
#endif
}

ScottTrigPlusSqrfpFractal() /* float version */
{
   /* { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old)	  */
   CMPLXsqr_old(tmp);	       /* tmp = sqr(old)	   */
   CMPLXadd(new,tmp,new);      /* new = trig(old)+sqr(old) */
   return(floatbailout());
}

SkinnerTrigSubSqrFractal()
{
#ifndef XFRACT
   /* { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT } 	      */
   LCMPLXtrig0(lold,lnew);    /* lnew = trig(lold)	     */
   LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)	      */
   LCMPLXsub(lnew,ltmp,lnew);  /* lnew = trig(lold)-sqr(lold) */
   return(longbailout());
#endif
}

SkinnerTrigSubSqrfpFractal()
{
   /* { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old) */
   CMPLXsqr_old(tmp);	       /* old = sqr(old)  */
   CMPLXsub(new,tmp,new);      /* new = trig(old)-sqr(old) */
   return(floatbailout());
}

TrigZsqrdfpFractal()
{
   /* { z=pixel: z=trig(z*z), |z|<TEST } */
   CMPLXsqr_old(tmp);
   CMPLXtrig0(tmp,new);
   return(floatbailout());
}

TrigZsqrdFractal() /* this doesn't work very well */
{
#ifndef XFRACT
   /* { z=pixel: z=trig(z*z), |z|<TEST } */
   LCMPLXsqr_old(ltmp);
   LCMPLXtrig0(ltmp,lnew);
   if(overflow)
      TryFloatFractal(TrigZsqrdfpFractal);
   return(longbailout());
#endif
}

SqrTrigFractal()
{
#ifndef XFRACT
   /* { z=pixel: z=sqr(trig(z)), |z|<TEST} */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXsqr(ltmp,lnew);
   return(longbailout());
#endif
}

SqrTrigfpFractal()
{
   /* SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST} */
   CMPLXtrig0(old,tmp);
   CMPLXsqr(tmp,new);
   return(floatbailout());
}


Magnet1Fractal()    /*	  Z = ((Z**2 + C - 1)/(2Z + C - 2))**2	  */
  {		      /*  In "Beauty of Fractals", code by Kev Allen. */
    _CMPLX top, bot, tmp;
    double div;

    top.x = tempsqrx - tempsqry + floatparm->x - 1; /* top = Z**2+C-1 */
    top.y = old.x * old.y;
    top.y = top.y + top.y + floatparm->y;

    bot.x = old.x + old.x + floatparm->x - 2;	    /* bot = 2*Z+C-2  */
    bot.y = old.y + old.y + floatparm->y;

    div = bot.x*bot.x + bot.y*bot.y;		    /* tmp = top/bot  */
    if (div < FLT_MIN) return(1);
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);	    /* Z = tmp**2     */
    new.y = tmp.x * tmp.y;
    new.y += new.y;

    return(floatbailout());
  }

Magnet2Fractal()  /* Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)	) /	 */
		    /*	     (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2  */
  {		    /*	 In "Beauty of Fractals", code by Kev Allen.  */
    _CMPLX top, bot, tmp;
    double div;

    top.x = old.x * (tempsqrx-tempsqry-tempsqry-tempsqry + T_Cm1.x)
	  - old.y * T_Cm1.y + T_Cm1Cm2.x;
    top.y = old.y * (tempsqrx+tempsqrx+tempsqrx-tempsqry + T_Cm1.x)
	  + old.x * T_Cm1.y + T_Cm1Cm2.y;

    bot.x = tempsqrx - tempsqry;
    bot.x = bot.x + bot.x + bot.x
	  + old.x * T_Cm2.x - old.y * T_Cm2.y
	  + T_Cm1Cm2.x + 1.0;
    bot.y = old.x * old.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
	  + old.x * T_Cm2.y + old.y * T_Cm2.x
	  + T_Cm1Cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;		    /* tmp = top/bot  */
    if (div < FLT_MIN) return(1);
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);	    /* Z = tmp**2     */
    new.y = tmp.x * tmp.y;
    new.y += new.y;

    return(floatbailout());
  }

LambdaTrigFractal()
{
#ifndef XFRACT
   LONGXYTRIGBAILOUT();
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
#endif
}

LambdaTrigfpFractal()
{
   FLOATXYTRIGBAILOUT();
   CMPLXtrig0(old,tmp); 	     /* tmp = trig(old) 	  */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

/* bailouts are different for different trig functions */
LambdaTrigFractal1()
{
#ifndef XFRACT
   LONGTRIGBAILOUT(); /* sin,cos */
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
#endif
}

LambdaTrigfpFractal1()
{
   FLOATTRIGBAILOUT(); /* sin,cos */
   CMPLXtrig0(old,tmp); 	     /* tmp = trig(old) 	  */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

LambdaTrigFractal2()
{
#ifndef XFRACT
   LONGHTRIGBAILOUT(); /* sinh,cosh */
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
#endif
}

LambdaTrigfpFractal2()
{
#ifndef XFRACT
   FLOATHTRIGBAILOUT(); /* sinh,cosh */
   CMPLXtrig0(old,tmp); 	     /* tmp = trig(old) 	  */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
#endif
}

ManOWarFractal()
{
#ifndef XFRACT
   /* From Art Matrix via Lee Skinner */
   lnew.x  = ltempsqrx - ltempsqry + ltmp.x + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y + longparm->y;
   ltmp = lold;
   return(longbailout());
#endif
}

ManOWarfpFractal()
{
   /* From Art Matrix via Lee Skinner */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
   new.x = tempsqrx - tempsqry + tmp.x + floatparm->x;
   new.y = 2.0 * old.x * old.y + tmp.y + floatparm->y;
   tmp = old;
   return(floatbailout());
}

/*
   MarksMandelPwr (XAXIS) {
      z = pixel, c = z ^ (z - 1):
	 z = c * sqr(z) + pixel,
      |z| <= 4
   }
*/

MarksMandelPwrfpFractal()
{
   CMPLXtrig0(old,new);
   CMPLXmult(tmp,new,new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

MarksMandelPwrFractal()
{
#ifndef XFRACT
   LCMPLXtrig0(lold,lnew);
   LCMPLXmult(ltmp,lnew,lnew);
   lnew.x += longparm->x;
   lnew.y += longparm->y;
   return(longbailout());
#endif
}

/* I was coding Marksmandelpower and failed to use some temporary
   variables. The result was nice, and since my name is not on any fractal,
   I thought I would immortalize myself with this error!
		Tim Wegner */


TimsErrorfpFractal()
{
   CMPLXtrig0(old,new);
   new.x = new.x * tmp.x - new.y * tmp.y;
   new.y = new.x * tmp.y - new.y * tmp.x;
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}
TimsErrorFractal()
{
#ifndef XFRACT
   LCMPLXtrig0(lold,lnew);
   lnew.x = multiply(lnew.x,ltmp.x,bitshift)-multiply(lnew.y,ltmp.y,bitshift);
   lnew.y = multiply(lnew.x,ltmp.y,bitshift)-multiply(lnew.y,ltmp.x,bitshift);
   lnew.x += longparm->x;
   lnew.y += longparm->y;
   return(longbailout());
#endif
}

CirclefpFractal()
{
   extern int colors;
   extern int color;
   int i;
   i = param[0]*(tempsqrx+tempsqry);
   color = i&(colors-1);
   return(1);
}
/*
CirclelongFractal()
{
   extern int colors;
   extern int color;
   long i;
   i = multiply(lparm.x,(ltempsqrx+ltempsqry),bitshift);
   i = i >> bitshift;
   color = i&(colors-1);
   return(1);
}
*/

/* -------------------------------------------------------------------- */
/*		Initialization (once per pixel) routines						*/
/* -------------------------------------------------------------------- */

#ifdef XFRACT
/* this code translated to asm - lives in newton.asm */
/* transform points with reciprocal function */
void invertz2(_CMPLX *z)
{
   z->x = dx0[col]+dx1[row];
   z->y = dy0[row]+dy1[col];
   z->x -= f_xcenter; z->y -= f_ycenter;  /* Normalize values to center of circle */

   tempsqrx = sqr(z->x) + sqr(z->y);  /* Get old radius */
   if(fabs(tempsqrx) > FLT_MIN)
      tempsqrx = f_radius / tempsqrx;
   else
      tempsqrx = FLT_MAX;   /* a big number, but not TOO big */
   z->x *= tempsqrx;	  z->y *= tempsqrx;	 /* Perform inversion */
   z->x += f_xcenter; z->y += f_ycenter; /* Renormalize */
}
#endif

int long_julia_per_pixel()
{
#ifndef XFRACT
   /* integer julia types */
   /* lambda */
   /* barnsleyj1 */
   /* barnsleyj2 */
   /* sierpinski */
   if(invert)
   {
      /* invert */
      invertz2(&old);

      /* watch out for overflow */
      if(sqr(old.x)+sqr(old.y) >= 127)
      {
	 old.x = 8;  /* value to bail out in one iteration */
	 old.y = 8;
      }

      /* convert to fudged longs */
      lold.x = old.x*fudge;
      lold.y = old.y*fudge;
   }
   else
   {
      lold.x = lx0[col]+lx1[row];
      lold.y = ly0[row]+ly1[col];
   }
   return(0);
#else
   printf("Called long_julia_per_pixel\n");
   exit(0);
#endif
}

int long_richard8_per_pixel()
{
#ifndef XFRACT
    long_mandel_per_pixel();
    LCMPLXtrig1(*longparm,ltmp);
    LCMPLXmult(ltmp,lparm2,ltmp);
    return(1);
#endif
}

int long_mandel_per_pixel()
{
#ifndef XFRACT
   /* integer mandel types */
   /* barnsleym1 */
   /* barnsleym2 */
   linit.x = lx0[col]+lx1[row];

   if(invert)
   {
      /* invert */
      invertz2(&init);

      /* watch out for overflow */
      if(sqr(init.x)+sqr(init.y) >= 127)
      {
	 init.x = 8;  /* value to bail out in one iteration */
	 init.y = 8;
      }

      /* convert to fudged longs */
      linit.x = init.x*fudge;
      linit.y = init.y*fudge;
   }

   if(useinitorbit == 1)
      lold = linitorbit;
   else
      lold = linit;

   lold.x += lparm.x;	 /* initial pertubation of parameters set */
   lold.y += lparm.y;
   return(1); /* 1st iteration has been done */
#else
   printf("Called long_mandel_per_pixel\n");
   exit(0);
#endif
}

int julia_per_pixel()
{
   /* julia */

   if(invert)
   {
      /* invert */
      invertz2(&old);

      /* watch out for overflow */
      if(bitshift <= 24)
	 if (sqr(old.x)+sqr(old.y) >= 127)
	 {
	    old.x = 8;	/* value to bail out in one iteration */
	    old.y = 8;
	 }
      if(bitshift >  24)
	 if (sqr(old.x)+sqr(old.y) >= 4.0)
	 {
	    old.x = 2;	/* value to bail out in one iteration */
	    old.y = 2;
	 }

      /* convert to fudged longs */
      lold.x = old.x*fudge;
      lold.y = old.y*fudge;
   }
   else
   {
      lold.x = lx0[col]+lx1[row];
      lold.y = ly0[row]+ly1[col];
   }

   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
   ltmp = lold;
   return(0);
}

marks_mandelpwr_per_pixel()
{
#ifndef XFRACT
   mandel_per_pixel();
   ltmp = lold;
   ltmp.x -= fudge;
   LCMPLXpwr(lold,ltmp,ltmp);
   return(1);
#endif
}

int mandel_per_pixel()
{
   /* mandel */

   if(invert)
   {
      invertz2(&init);

      /* watch out for overflow */
      if(bitshift <= 24)
	 if (sqr(init.x)+sqr(init.y) >= 127)
	 {
	    init.x = 8;  /* value to bail out in one iteration */
	    init.y = 8;
	 }
      if(bitshift >  24)
	 if (sqr(init.x)+sqr(init.y) >= 4)
	 {
	    init.x = 2;  /* value to bail out in one iteration */
	    init.y = 2;
	 }

      /* convert to fudged longs */
      linit.x = init.x*fudge;
      linit.y = init.y*fudge;
   }
   else
      linit.x = lx0[col]+lx1[row];
   switch (fractype)
     {
	case MANDELLAMBDA:		/* Critical Value 0.5 + 0.0i  */
	    lold.x = FgHalf;
	    lold.y = 0;
	    break;
	default:
	    lold = linit;
	    break;
      }

   /* alter init value */
   if(useinitorbit == 1)
      lold = linitorbit;
   else if(useinitorbit == 2)
      lold = linit;

   if(inside == -60 || inside == -61)
   {
      /* kludge to match "Beauty of Fractals" picture since we start
	 Mandelbrot iteration with init rather than 0 */
      lold.x = lparm.x; /* initial pertubation of parameters set */
      lold.y = lparm.y;
      color = -1;
   }
   else
   {
      lold.x += lparm.x; /* initial pertubation of parameters set */
      lold.y += lparm.y;
   }
   ltmp = linit; /* for spider */
   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
   return(1); /* 1st iteration has been done */
}


int marksmandel_per_pixel()
{
   /* marksmandel */

   if(invert)
   {
      invertz2(&init);

      /* watch out for overflow */
      if(sqr(init.x)+sqr(init.y) >= 127)
      {
	 init.x = 8;  /* value to bail out in one iteration */
	 init.y = 8;
      }

      /* convert to fudged longs */
      linit.x = init.x*fudge;
      linit.y = init.y*fudge;
   }
   else
      linit.x = lx0[col]+lx1[row];

   if(useinitorbit == 1)
      lold = linitorbit;
   else
      lold = linit;

   lold.x += lparm.x;	 /* initial pertubation of parameters set */
   lold.y += lparm.y;

   if(c_exp > 3)
      lcpower(&lold,c_exp-1,&lcoefficient,bitshift);
   else if(c_exp == 3) {
      lcoefficient.x = multiply(lold.x, lold.x, bitshift)
	 - multiply(lold.y, lold.y, bitshift);
      lcoefficient.y = multiply(lold.x, lold.y, bitshiftless1);
   }
   else if(c_exp == 2)
      lcoefficient = lold;
   else if(c_exp < 2) {
      lcoefficient.x = 1L << bitshift;
      lcoefficient.y = 0L;
   }

   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
   return(1); /* 1st iteration has been done */
}

int marksmandelfp_per_pixel()
{
   /* marksmandel */

   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;	 /* initial pertubation of parameters set */
   old.y += parm.y;

   tempsqrx = sqr(old.x);
   tempsqry = sqr(old.y);

   if(c_exp > 3)
      cpower(&old,c_exp-1,&coefficient);
   else if(c_exp == 3) {
      coefficient.x = tempsqrx - tempsqry;
      coefficient.y = old.x * old.y * 2;
   }
   else if(c_exp == 2)
      coefficient = old;
   else if(c_exp < 2) {
      coefficient.x = 1.0;
      coefficient.y = 0.0;
   }

   return(1); /* 1st iteration has been done */
}

marks_mandelpwrfp_per_pixel()
{
   mandelfp_per_pixel();
   tmp = old;
   tmp.x -= 1;
   CMPLXpwr(old,tmp,tmp);
   return(1);
}

int mandelfp_per_pixel()
{
   /* floating point mandelbrot */
   /* mandelfp */

   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];
    switch (fractype)
      {
	case MAGNET2M:
	    FloatPreCalcMagnet2();
	case MAGNET1M:		 /* Crit Val Zero both, but neither   */
	    old.x = old.y = 0.0; /* is of the form f(Z,C) = Z*g(Z)+C  */
	    break;
	case MANDELLAMBDAFP:		/* Critical Value 0.5 + 0.0i  */
	    old.x = 0.5;
	    old.y = 0.0;
	    break;
	default:
	    old = init;
	    break;
      }

   /* alter init value */
   if(useinitorbit == 1)
      old = initorbit;
   else if(useinitorbit == 2)
      old = init;

   if(inside == -60 || inside == -61)
   {
      /* kludge to match "Beauty of Fractals" picture since we start
	 Mandelbrot iteration with init rather than 0 */
      old.x = parm.x; /* initial pertubation of parameters set */
      old.y = parm.y;
      color = -1;
   }
   else
   {
     old.x += parm.x;
     old.y += parm.y;
   }
   tmp = init; /* for spider */
   tempsqrx = sqr(old.x);  /* precalculated value for regular Mandelbrot */
   tempsqry = sqr(old.y);
   return(1); /* 1st iteration has been done */
}

int juliafp_per_pixel()
{
   /* floating point julia */
   /* juliafp */
   if(invert)
      invertz2(&old);
   else
   {
     old.x = dx0[col]+dx1[row];
     old.y = dy0[row]+dy1[col];
   }
   tempsqrx = sqr(old.x);  /* precalculated value for regular Julia */
   tempsqry = sqr(old.y);
   tmp = old;
   return(0);
}

int MPCjulia_per_pixel()
{
#ifndef XFRACT
   /* floating point julia */
   /* juliafp */
   if(invert)
      invertz2(&old);
   else
   {
     old.x = dx0[col]+dx1[row];
     old.y = dy0[row]+dy1[col];
   }
   mpcold.x = *pd2MP(old.x);
   mpcold.y = *pd2MP(old.y);
   return(0);
#endif
}

otherrichard8fp_per_pixel()
{
    othermandelfp_per_pixel();
    CMPLXtrig1(*floatparm,tmp);
    CMPLXmult(tmp,parm2,tmp);
    return(1);
}

int othermandelfp_per_pixel()
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;	 /* initial pertubation of parameters set */
   old.y += parm.y;

   return(1); /* 1st iteration has been done */
}

int MPCHalley_per_pixel()
{
#ifndef XFRACT
   /* MPC halley */
   if(invert)
      invertz2(&init);
   else
     init.x = dx0[col]+dx1[row];

   mpcold.x = *pd2MP(init.x);
   mpcold.y = *pd2MP(init.y);

   return(0);
#endif
}

int Halley_per_pixel()
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   old = init;

   return(0); /* 1st iteration is not done */
}

int otherjuliafp_per_pixel()
{
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dx0[col]+dx1[row];
      old.y = dy0[row]+dy1[col];
   }
   return(0);
}

#if 0
#define Q0 .113
#define Q1 .01
#else
#define Q0 0
#define Q1 0
#endif

int quaternionjulfp_per_pixel()
{
   old.x = dx0[col]+dx1[row];
   old.y = dy0[row]+dy1[col];
   floatparm->x = param[4];
   floatparm->y = param[5];
   qc  = param[0];
   qci = param[1];
   qcj = param[2];
   qck = param[3];
   return(0);
}

int quaternionfp_per_pixel()
{
   old.x = 0;
   old.y = 0;
   floatparm->x = 0;
   floatparm->y = 0;
   qc  = dx0[col]+dx1[row];
   qci = dy0[row]+dy1[col];
   qcj = param[2];
   qck = param[3];
   return(0);
}

int trigmandelfp_per_pixel()
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;	 /* initial pertubation of parameters set */
   old.y += parm.y;
   CMPLXtrig0(old,old);
   return(1); /* 1st iteration has been done */
}

int trigjuliafp_per_pixel()
{
   /* for tetrated types */
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dx0[col]+dx1[row];
      old.y = dy0[row]+dy1[col];
   }
   CMPLXtrig0(old,old);
   return(0);
}

int trigXtrigmandelfp_per_pixel()
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;	 /* initial pertubation of parameters set */
   old.y += parm.y;
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXmult(tmp,tmp2,old);
   return(1); /* 1st iteration has been done */
}

int trigXtrigjuliafp_per_pixel()
{
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dx0[col]+dx1[row];
      old.y = dy0[row]+dy1[col];
   }
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXmult(tmp,tmp2,old);
   return(0);
}

int MarksCplxMandperp(void)
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];
   old.x = init.x + parm.x; /* initial pertubation of parameters set */
   old.y = init.y + parm.y;
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   Coefficient = ComplexPower(init, pwr);
   return(1);
}

int long_phoenix_per_pixel()
{
#ifndef XFRACT
   if(invert)
   {
      /* invert */
      invertz2(&old);

      /* watch out for overflow */
      if(sqr(old.x)+sqr(old.y) >= 127)
      {
	 old.x = 8;  /* value to bail out in one iteration */
	 old.y = 8;
      }

      /* convert to fudged longs */
      lold.x = old.x*fudge;
      lold.y = old.y*fudge;
   }
   else
   {
      lold.x = lx0[col]+lx1[row];
      lold.y = ly0[row]+ly1[col];
   }
   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
   ltmp2.x = 0; /* use ltmp2 as the complex Y value */
   ltmp2.y = 0;
   return(0);
#else
   printf("Called long_phoenix_per_pixel\n");
   exit(0);
#endif
}

int phoenix_per_pixel()
{
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dx0[col]+dx1[row];
      old.y = dy0[row]+dy1[col];
   }
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   tmp2.x = 0; /* use tmp2 as the complex Y value */
   tmp2.y = 0;
   return(0);
}
int long_mandphoenix_per_pixel()
{
#ifndef XFRACT
   linit.x = lx0[col]+lx1[row];

   if(invert)
   {
      /* invert */
      invertz2(&init);

      /* watch out for overflow */
      if(sqr(init.x)+sqr(init.y) >= 127)
      {
	 init.x = 8;  /* value to bail out in one iteration */
	 init.y = 8;
      }

      /* convert to fudged longs */
      linit.x = init.x*fudge;
      linit.y = init.y*fudge;
   }

   if(useinitorbit == 1)
      lold = linitorbit;
   else
      lold = linit;

   lold.x += lparm.x;	 /* initial pertubation of parameters set */
   lold.y += lparm.y;
   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
   ltmp2.x = 0;
   ltmp2.y = 0;
   return(1); /* 1st iteration has been done */
#else
   printf("Called long_mandphoenix_per_pixel\n");
   exit(0);
#endif
}
int mandphoenix_per_pixel()
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col]+dx1[row];

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;	 /* initial pertubation of parameters set */
   old.y += parm.y;
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   tmp2.x = 0;
   tmp2.y = 0;
   return(1); /* 1st iteration has been done */
}

QuaternionFPFractal()
{
   double a0,a1,a2,a3,n0,n1,n2,n3;
   a0 = old.x;
   a1 = old.y;
   a2 = floatparm->x;
   a3 = floatparm->y;

   n0 = a0*a0-a1*a1-a2*a2-a3*a3 + qc;
   n1 = 2*a0*a1 + qci;
   n2 = 2*a0*a2 + qcj;
   n3 = 2*a0*a3 + qck;
   /* Check bailout */
   magnitude = a0*a0+a1*a1+a2*a2+a3*a3;
   if (magnitude>rqlim) {
       return 1;
   }
   old.x = n0;
   old.y = n1;
   floatparm->x = n2;
   floatparm->y = n3;
   return(0);
}

HyperComplex1FPFractal()
{
   double n0,n1,n2,n3;

   n0 = sqr(old.x)-sqr(old.y)-sqr(floatparm->x)+sqr(floatparm->y) + qc;
   n1 = 2*old.x*old.y - 2*floatparm->x*floatparm->y + qci;
   n2 = 2*old.x*floatparm->x - 2*old.y*floatparm->y + qcj;
   n3 = 2*old.x*floatparm->y + 2*old.y*floatparm->x + qck;
   
   old.x = n0;
   old.y = n1;
   floatparm->x = n2;
   floatparm->y = n3;

   /* Check bailout */
   magnitude = sqr(old.x)+sqr(old.y)+sqr(floatparm->x)+sqr(floatparm->y);
   if (magnitude>rqlim) {
       return 1;
   }
   return(0);
}

HyperComplexFPFractal()
{
   double n0,n1,n2,n3;
   _HCMPLX hold, hnew;
   hold.x = old.x;
   hold.y = old.y;
   hold.z = floatparm->x;
   hold.t = floatparm->y;

/*   HComplexSqr(&hold,&hnew); */
   HComplexTrig0(&hold,&hnew);
   
   hnew.x += qc;
   hnew.y += qci;
   hnew.z += qcj;
   hnew.t += qck;
   
   old.x = hnew.x;
   old.y = hnew.y;
   floatparm->x = hnew.z;
   floatparm->y = hnew.t;

   /* Check bailout */
   magnitude = sqr(old.x)+sqr(old.y)+sqr(floatparm->x)+sqr(floatparm->y);
   if (magnitude>rqlim) {
       return 1;
   }
   return(0);
}

/* -------------------------------------------------------------------- */
/*		Setup (once per fractal image) routines 		*/
/* -------------------------------------------------------------------- */

MandelSetup()		/* Mandelbrot Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0
       && bitshift == 29 && potflag == 0
       && biomorph == -1 && inside > -59 && outside >= -1
       && useinitorbit != 1 && using_jiim == 0)
      calctype = calcmand; /* the normal case - use CALCMAND */
   else
   {
      /* special case: use the main processing loop */
      calctype = StandardFractal;
      longparm = &linit;
   }
   return(1);
}

JuliaSetup()		/* Julia Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0
       && bitshift == 29 && potflag == 0
       && biomorph == -1 && inside > -59 && outside >= -1
       && !finattract && using_jiim == 0)
      calctype = calcmand; /* the normal case - use CALCMAND */
   else
   {
      /* special case: use the main processing loop */
      calctype = StandardFractal;
      longparm = &lparm;
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
   }
   return(1);
}

NewtonSetup()		/* Newton/NewtBasin Routines */
{
   int i;
   extern int basin;
   extern int fpu;
   if (debugflag != 1010)
   {
      if(fpu != 0)
      {
	 if(fractype == MPNEWTON)
	    fractype = NEWTON;
	 else if(fractype == MPNEWTBASIN)
	    fractype = NEWTBASIN;
      }
      else
      {
	 if(fractype == NEWTON)
	       fractype = MPNEWTON;
	 else if(fractype == NEWTBASIN)
	       fractype = MPNEWTBASIN;
      }
      curfractalspecific = &fractalspecific[fractype];
   }

   /* set up table of roots of 1 along unit circle */
   degree = (int)parm.x;
   if(degree < 2)
      degree = 3;   /* defaults to 3, but 2 is possible */
   root = 1;

   /* precalculated values */
   roverd	= (double)root / (double)degree;
   d1overd	= (double)(degree - 1) / (double)degree;
   maxcolor	= 0;
   threshold	= .3*PI/degree; /* less than half distance between roots */
#ifndef XFRACT
   if (fractype == MPNEWTON || fractype == MPNEWTBASIN) {
      mproverd	   = *pd2MP(roverd);
      mpd1overd    = *pd2MP(d1overd);
      mpthreshold  = *pd2MP(threshold);
      mpone	   = *pd2MP(1.0);
   }
#endif

   floatmin = FLT_MIN;
   floatmax = FLT_MAX;

   basin = 0;
   if(roots != staticroots) {
      free(roots);
      roots = staticroots;
   }

   if (fractype==NEWTBASIN)
   {
      if(parm.y)
	 basin = 2; /*stripes */
      else
	 basin = 1;
      if(degree > 16)
      {
	 if((roots=(_CMPLX *)malloc(degree*sizeof(_CMPLX)))==NULL)
	 {
	    roots = staticroots;
	    degree = 16;
	 }
      }
      else
	 roots = staticroots;

      /* list of roots to discover where we converged for newtbasin */
      for(i=0;i<degree;i++)
      {
	 roots[i].x = cos(i*twopi/(double)degree);
	 roots[i].y = sin(i*twopi/(double)degree);
      }
   }
#ifndef XFRACT
   else if (fractype==MPNEWTBASIN)
   {
     if(parm.y)
	 basin = 2; /*stripes */
      else
	 basin = 1;

      if(degree > 16)
      {
	 if((MPCroots=(struct MPC *)malloc(degree*sizeof(struct MPC)))==NULL)
	 {
	    MPCroots = (struct MPC *)staticroots;
	    degree = 16;
	 }
      }
      else
	 MPCroots = (struct MPC *)staticroots;

      /* list of roots to discover where we converged for newtbasin */
      for(i=0;i<degree;i++)
      {
	 MPCroots[i].x = *pd2MP(cos(i*twopi/(double)degree));
	 MPCroots[i].y = *pd2MP(sin(i*twopi/(double)degree));
      }
   }
#endif

   param[0] = (double)degree; /* JCO 7/1/92 */
   if (degree%4 == 0)
      symmetry = XYAXIS;
   else
      symmetry = XAXIS;

   calctype=StandardFractal;
#ifndef XFRACT
   if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
      setMPfunctions();
#endif
   return(1);
}


StandaloneSetup()
{
   timer(0,curfractalspecific->calctype);
   return(0);		/* effectively disable solid-guessing */
}

UnitySetup()
{
   periodicitycheck = 0;
   FgOne = (1L << bitshift);
   FgTwo = FgOne + FgOne;
   return(1);
}

MandelfpSetup()
{
   c_exp = param[2];
   pwr.x = param[2] - 1.0;
   pwr.y = param[3];
   floatparm = &init;
   switch (fractype)
   {
   case MARKSMANDELFP:
      if(c_exp < 1)
         c_exp = 1;
      if(!(c_exp & 1))
         symmetry = XYAXIS_NOPARM;    /* odd exponents */
      if(c_exp & 1)
         symmetry = XAXIS_NOPARM;
      break;
   case MANDELFP:
	/*
	   floating point code could probably be altered to handle many of
	   the situations that otherwise are using StandardFractal().
	   calcmandfp() can currently handle invert, any rqlim, potflag
	   zmag, epsilon cross, and all the current outside options
						     Wes Loewer 11/03/91
	*/
	if (debugflag != 90
	    && !distest
	    && decomp[0] == 0
	    && biomorph == -1
	    && (inside >= -1 || inside == -59 || inside == -100)
	    /* uncomment this next line if more outside options are added */
	    /* && outside >= -5 */
	    && useinitorbit != 1
	    && using_jiim == 0)
	{
	   calctype = calcmandfp; /* the normal case - use calcmandfp */
	   calcmandfpasmstart();
	}
	else
	{
	   /* special case: use the main processing loop */
	   calctype = StandardFractal;
	}
	break;
   case FPMANDELZPOWER:
      if((double)c_exp == param[2] && c_exp & 1) /* odd exponents */
         symmetry = XYAXIS_NOPARM;
      if(param[3] != 0)
         symmetry = NOSYM;
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = floatZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
      break;
   case MAGNET1M:
   case MAGNET2M:
      attr[0].x = 1.0;	    /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;	    /* - both MAGNET1 and MAGNET2 */
      attrperiod[0] = 1;
      attractors = 1;
      break;
   case SPIDERFP:
      if(periodicitycheck==1) /* if not user set */
	 periodicitycheck=4;
      break;
   case MANDELEXP:
      symmetry = XAXIS_NOPARM;
      break;
/* Added to account for symmetry in manfn+exp and manfn+zsqrd */
/*     JCO 2/29/92 */
   case FPMANTRIGPLUSEXP:
   case FPMANTRIGPLUSZSQRD:
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
      break;
   case QUATFP:
   case HYPERCMPLXFP:
      floatparm = &tmp;
      attractors = 0;
      periodicitycheck = 0;
      break;
   default:
      break;
   }
   return(1);
}

JuliafpSetup()
{
   c_exp = param[2];
   floatparm = &parm;
   if(fractype==COMPLEXMARKSJUL)
   {
      pwr.x = param[2] - 1.0;
      pwr.y = param[3];
      Coefficient = ComplexPower(*floatparm, pwr);
   }
   switch (fractype)
   {
   case JULIAFP:
	/*
	   floating point code could probably be altered to handle many of
	   the situations that otherwise are using StandardFractal().
	   calcmandfp() can currently handle invert, any rqlim, potflag
	   zmag, epsilon cross, and all the current outside options
						     Wes Loewer 11/03/91
	*/
	if (debugflag != 90
	    && !distest
	    && decomp[0] == 0
	    && biomorph == -1
	    && (inside >= -1 || inside == -59 || inside == -100)
	    /* uncomment this next line if more outside options are added */
	    /* && outside >= -5 */
	    && useinitorbit != 1
	    && !finattract
	    && using_jiim == 0)
	{
	   calctype = calcmandfp; /* the normal case - use calcmandfp */
	   calcmandfpasmstart();
	}
	else
	{
	   /* special case: use the main processing loop */
	   calctype = StandardFractal;
	   get_julia_attractor (0.0, 0.0);   /* another attractor? */
	}
	break;
   case FPJULIAZPOWER:
      if((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2] )
         symmetry = NOSYM;
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = floatZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
      get_julia_attractor (param[0], param[1]);	/* another attractor? */
      break;
   case MAGNET2J:
      FloatPreCalcMagnet2();
   case MAGNET1J:
      attr[0].x = 1.0;	    /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;	    /* - both MAGNET1 and MAGNET2 */
      attrperiod[0] = 1;
      attractors = 1;
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   case LAMBDAFP:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      get_julia_attractor (0.5, 0.0);	/* another attractor? */
      break;
   case LAMBDAEXP:
      if(parm.y == 0.0)
	 symmetry=XAXIS;
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
/* Added to account for symmetry in julfn+exp and julfn+zsqrd */
/*     JCO 2/29/92 */
   case FPJULTRIGPLUSEXP:
   case FPJULTRIGPLUSZSQRD:
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   case HYPERCMPLXJFP:
      if(trigndx[0] != SQR)
         symmetry=NOSYM;
   case QUATJULFP:
      attractors = 0;	/* attractors broken since code checks r,i not j,k */
      periodicitycheck = 0;
      if(param[4] != 0.0 || param[5] != 0)
         symmetry = NOSYM;
      break;
   default:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   }
   return(1);
}

MandellongSetup()
{
   FgHalf = fudge >> 1;
   c_exp = param[2];
   if(fractype==MARKSMANDEL && c_exp < 1)
      c_exp = 1;
   if(fractype==LMANDELZPOWER && c_exp < 1)
      c_exp = 1;
   if((fractype==MARKSMANDEL   && !(c_exp & 1)) ||
       (fractype==LMANDELZPOWER && c_exp & 1))
      symmetry = XYAXIS_NOPARM;    /* odd exponents */
   if((fractype==MARKSMANDEL && (c_exp & 1)) || fractype==LMANDELEXP)
      symmetry = XAXIS_NOPARM;
   if(fractype==SPIDER && periodicitycheck==1)
      periodicitycheck=4;
   longparm = &linit;
   if(fractype==LMANDELZPOWER)
   {
      if(param[3] == 0.0 && debugflag != 6000  && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = longZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
      if(param[3] != 0 || (double)c_exp != param[2] )
         symmetry = NOSYM;
    }
/* Added to account for symmetry in manfn+exp and manfn+zsqrd */
/*     JCO 2/29/92 */
   if((fractype==LMANTRIGPLUSEXP)||(fractype==LMANTRIGPLUSZSQRD))
   {
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
   }
   return(1);
}

JulialongSetup()
{
   c_exp = param[2];
   longparm = &lparm;
   switch (fractype)
   {
   case LJULIAZPOWER:
      if((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2])
         symmetry = NOSYM;
      else if(c_exp < 1)
         c_exp = 1;
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = longZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
      break;
   case LAMBDA:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      get_julia_attractor (0.5, 0.0);	/* another attractor? */
      break;
   case LLAMBDAEXP:
      if(lparm.y == 0)
	 symmetry = XAXIS;
      break;
/* Added to account for symmetry in julfn+exp and julfn+zsqrd */
/*     JCO 2/29/92 */
   case LJULTRIGPLUSEXP:
   case LJULTRIGPLUSZSQRD:
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   default:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   }
   return(1);
}

TrigPlusSqrlongSetup()
{
   curfractalspecific->per_pixel =  julia_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusSqrFractal;
   if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
   {
      if(lparm2.x == fudge)	   /* Scott variant */
	 curfractalspecific->orbitcalc =  ScottTrigPlusSqrFractal;
      else if(lparm2.x == -fudge)  /* Skinner variant */
	 curfractalspecific->orbitcalc =  SkinnerTrigSubSqrFractal;
   }
   return(JulialongSetup());
}

TrigPlusSqrfpSetup()
{
   curfractalspecific->per_pixel =  juliafp_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusSqrfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)	/* Scott variant */
	 curfractalspecific->orbitcalc =  ScottTrigPlusSqrfpFractal;
      else if(parm2.x == -1.0)	/* Skinner variant */
	 curfractalspecific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
   }
   return(JuliafpSetup());
}

TrigPlusTriglongSetup()
{
   FnPlusFnSym();
   if(trigndx[1] == SQR)
      return(TrigPlusSqrlongSetup());
   curfractalspecific->per_pixel =  long_julia_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusTrigFractal;
   if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
   {
      if(lparm2.x == fudge)	   /* Scott variant */
	 curfractalspecific->orbitcalc =  ScottTrigPlusTrigFractal;
      else if(lparm2.x == -fudge)  /* Skinner variant */
	 curfractalspecific->orbitcalc =  SkinnerTrigSubTrigFractal;
   }
   return(JulialongSetup());
}

TrigPlusTrigfpSetup()
{
   FnPlusFnSym();
   if(trigndx[1] == SQR)
      return(TrigPlusSqrfpSetup());
   curfractalspecific->per_pixel =  otherjuliafp_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusTrigfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)	/* Scott variant */
	 curfractalspecific->orbitcalc =  ScottTrigPlusTrigfpFractal;
      else if(parm2.x == -1.0)	/* Skinner variant */
	 curfractalspecific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
   }
   return(JuliafpSetup());
}

FnPlusFnSym() /* set symmetry matrix for fn+fn type */
{
   static char far fnplusfn[7][7] =
   {/* fn2 ->sin     cos    sinh    cosh   exp    log    sqr  */
   /* fn1 */
   /* sin */ {PI_SYM,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cos */ {XAXIS, PI_SYM,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* sinh*/ {XYAXIS,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cosh*/ {XAXIS, XYAXIS,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* exp */ {XAXIS, XYAXIS,XAXIS,  XAXIS, XYAXIS,XAXIS, XAXIS},
   /* log */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XAXIS},
   /* sqr */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XYAXIS}
   };
   if(parm.y == 0.0 && parm2.y == 0.0)
    { if(trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/6/92*/
        symmetry = fnplusfn[trigndx[0]][trigndx[1]];  /* JCO 5/6/92 */
    }                 /* defaults to XAXIS symmetry JCO 5/6/92 */
   else
      symmetry = NOSYM;
   return(0);
}

LambdaTrigOrTrigSetup()
{
/* default symmetry is ORIGIN  JCO 2/29/92 (changed from PI_SYM) */
   longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
   floatparm = &parm;
   if ((trigndx[0] == EXP) || (trigndx[1] == EXP))
      symmetry = NOSYM; /* JCO 1/9/93 */
   if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
      symmetry = XAXIS;
   get_julia_attractor (0.0, 0.0);	/* an attractor? */
   return(1);
}

JuliaTrigOrTrigSetup()
{
/* default symmetry is XAXIS */
   longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
   floatparm = &parm;
   if(parm.y != 0.0)
     symmetry = NOSYM;
   get_julia_attractor (0.0, 0.0);	/* an attractor? */
   return(1);
}

ManlamTrigOrTrigSetup()
{ /* psuedo */
/* default symmetry is XAXIS */
   longparm = &linit; /* added to consolidate code 10/1/92 JCO */
   floatparm = &init;
   if (trigndx[0] == SQR)
      symmetry = NOSYM;
   if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
      symmetry = NOSYM;
   return(1);
}

MandelTrigOrTrigSetup()
{
/* default symmetry is XAXIS_NOPARM */
   longparm = &linit; /* added to consolidate code 10/1/92 JCO */
   floatparm = &init;
   if ((trigndx[0] == 14) || (trigndx[1] == 14)) /* FLIP  JCO 5/28/92 */
      symmetry = NOSYM;
   return(1);
}


ZXTrigPlusZSetup()
{
/*   static char far ZXTrigPlusZSym1[] = */
   /* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
/*	     {XAXIS,XYAXIS,XAXIS,XYAXIS,XAXIS,NOSYM,XYAXIS}; */
/*   static char far ZXTrigPlusZSym2[] = */
   /* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
/*	     {NOSYM,ORIGIN,NOSYM,ORIGIN,NOSYM,NOSYM,ORIGIN}; */

   if(param[1] == 0.0 && param[3] == 0.0)
/*      symmetry = ZXTrigPlusZSym1[trigndx[0]]; */
   switch(trigndx[0])
   {
      case COS:   /* changed to two case statments and made any added */
      case COSH:  /* functions default to XAXIS symmetry. JCO 5/25/92 */
      case SQR:
      case 9:   /* 'real' cos */
         symmetry = XYAXIS;
         break;
      case 14:   /* FLIP  JCO 2/29/92 */
         symmetry = YAXIS;
         break;
      case LOG:
         symmetry = NOSYM;
         break;
      default:
         symmetry = XAXIS;
         break;
      }
   else
/*      symmetry = ZXTrigPlusZSym2[trigndx[0]]; */
   switch(trigndx[0])
   {
      case COS:
      case COSH:
      case SQR:
      case 9:   /* 'real' cos */
         symmetry = ORIGIN;
         break;
      case 14:   /* FLIP  JCO 2/29/92 */
         symmetry = NOSYM;
         break;
      default:
         symmetry = XAXIS;
         break;
      }
   if(curfractalspecific->isinteger)
   {
      curfractalspecific->orbitcalc =  ZXTrigPlusZFractal;
      if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
      {
	 if(lparm2.x == fudge)	   /* Scott variant */
		 curfractalspecific->orbitcalc =  ScottZXTrigPlusZFractal;
	 else if(lparm2.x == -fudge)  /* Skinner variant */
		 curfractalspecific->orbitcalc =  SkinnerZXTrigSubZFractal;
      }
      return(JulialongSetup());
   }
   else
   {
      curfractalspecific->orbitcalc =  ZXTrigPlusZfpFractal;
      if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
      {
	 if(parm2.x == 1.0)	/* Scott variant */
		 curfractalspecific->orbitcalc =  ScottZXTrigPlusZfpFractal;
	 else if(parm2.x == -1.0)	/* Skinner variant */
		 curfractalspecific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
      }
   }
   return(JuliafpSetup());
}

LambdaTrigSetup()
{
   int isinteger;
   if((isinteger = curfractalspecific->isinteger))
      curfractalspecific->orbitcalc =  LambdaTrigFractal;
   else
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
   case 9:   /* 'real' cos, added this and default for additional functions */
      symmetry = PI_SYM;
      if(isinteger)
	 curfractalspecific->orbitcalc =  LambdaTrigFractal1;
      else
	 curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      symmetry = ORIGIN;
      if(isinteger)
	 curfractalspecific->orbitcalc =  LambdaTrigFractal2;
      else
	 curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
      break;
   case SQR:
      symmetry = ORIGIN;
      break;
   case EXP:
      if(isinteger)
	 curfractalspecific->orbitcalc =  LongLambdaexponentFractal;
      else
	 curfractalspecific->orbitcalc =  LambdaexponentFractal;
      symmetry = NOSYM; /* JCO 1/9/93 */
      break;
   case LOG:
      symmetry = NOSYM;
      break;
   default:   /* default for additional functions */
      symmetry = ORIGIN;  /* JCO 5/8/92 */
      break;
   }
   get_julia_attractor (0.0, 0.0);	/* an attractor? */
   if(isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

JuliafnPlusZsqrdSetup()
{
/*   static char far fnpluszsqrd[] = */
   /* fn1 ->  sin   cos    sinh  cosh	sqr    exp   log  */
   /* sin    {NOSYM,ORIGIN,NOSYM,ORIGIN,ORIGIN,NOSYM,NOSYM}; */

/*   symmetry = fnpluszsqrd[trigndx[0]];   JCO  5/8/92 */
   switch(trigndx[0]) /* fix sqr symmetry & add additional functions */
   {
   case COS: /* cosxx */
   case COSH:
   case SQR:
   case 9:   /* 'real' cos */
   case 10:  /* tan */
   case 11:  /* tanh */
   symmetry = ORIGIN;
    /* default is for NOSYM symmetry */
   }
   if(curfractalspecific->isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

SqrTrigSetup()
{
/*   static char far SqrTrigSym[] = */
   /* fn1 ->  sin    cos    sinh   cosh   sqr	 exp   log  */
/*	     {PI_SYM,PI_SYM,XYAXIS,XYAXIS,XYAXIS,XAXIS,XAXIS}; */
/*   symmetry = SqrTrigSym[trigndx[0]];      JCO  5/9/92 */
   switch(trigndx[0]) /* fix sqr symmetry & add additional functions */
   {
   case SIN:
   case COS: /* cosxx */
   case 9:   /* 'real' cos */
   symmetry = PI_SYM;
    /* default is for XAXIS symmetry */
   }
   if(curfractalspecific->isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

FnXFnSetup()
{
   static char far fnxfn[7][7] =
   {/* fn2 ->sin     cos    sinh    cosh  exp    log    sqr */
   /* fn1 */
   /* sin */ {PI_SYM,YAXIS, XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* cos */ {YAXIS, PI_SYM,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* sinh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* cosh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* exp */ {XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, NOSYM, XYAXIS},
   /* log */ {NOSYM, NOSYM, NOSYM, NOSYM, NOSYM, XAXIS, NOSYM},
   /* sqr */ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XYAXIS,NOSYM, XYAXIS},
   };
   /*
   if(trigndx[0]==EXP || trigndx[0]==LOG || trigndx[1]==EXP || trigndx[1]==LOG)
      symmetry = XAXIS;
   else if((trigndx[0]==SIN && trigndx[1]==SIN)||(trigndx[0]==COS && trigndx[1]==COS))
      symmetry = PI_SYM;
   else if((trigndx[0]==SIN && trigndx[1]==COS)||(trigndx[0]==COS && trigndx[1]==SIN))
      symmetry = YAXIS;
   else
      symmetry = XYAXIS;
   */
   if(trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/22/92*/
        symmetry = fnxfn[trigndx[0]][trigndx[1]];  /* JCO 5/22/92 */
                    /* defaults to XAXIS symmetry JCO 5/22/92 */
   else {  /* added to complete the symmetry JCO 5/22/92 */
      if (trigndx[0]==LOG || trigndx[1] ==LOG) symmetry = NOSYM;
      if (trigndx[0]==9 || trigndx[1] ==9) { /* 'real' cos */
         if (trigndx[0]==SIN || trigndx[1] ==SIN) symmetry = PI_SYM;
         if (trigndx[0]==COS || trigndx[1] ==COS) symmetry = PI_SYM;
      }
      if (trigndx[0]==9 && trigndx[1] ==9) symmetry = PI_SYM;
   }
   if(curfractalspecific->isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

MandelTrigSetup()
{
   int isinteger;
   if((isinteger = curfractalspecific->isinteger))
      curfractalspecific->orbitcalc =  LambdaTrigFractal;
   else
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
   symmetry = XYAXIS_NOPARM;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
      if(isinteger)
	 curfractalspecific->orbitcalc =  LambdaTrigFractal1;
      else
	 curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      if(isinteger)
	 curfractalspecific->orbitcalc =  LambdaTrigFractal2;
      else
	 curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
      break;
   case EXP:
      symmetry = XAXIS_NOPARM;
      if(isinteger)
	 curfractalspecific->orbitcalc =  LongLambdaexponentFractal;
      else
	 curfractalspecific->orbitcalc =  LambdaexponentFractal;
      break;
   case LOG:
      symmetry = XAXIS_NOPARM;
      break;
   default:   /* added for additional functions, JCO 5/25/92 */
      symmetry = XYAXIS_NOPARM;
      break;
   }
   if(isinteger)
      return(MandellongSetup());
   else
      return(MandelfpSetup());
}

MarksJuliaSetup()
{
   c_exp = param[2];
   longparm = &lparm;
   lold = *longparm;
   if(c_exp > 2)
      lcpower(&lold,c_exp,&lcoefficient,bitshift);
   else if(c_exp == 2)
   {
      lcoefficient.x = multiply(lold.x,lold.x,bitshift) - multiply(lold.y,lold.y,bitshift);
      lcoefficient.y = multiply(lold.x,lold.y,bitshiftless1);
   }
   else if(c_exp < 2)
      lcoefficient = lold;
   get_julia_attractor (0.0, 0.0);	/* an attractor? */
   return(1);
}

MarksJuliafpSetup()
{
   c_exp = param[2];
   floatparm = &parm;
   old = *floatparm;
   if(c_exp > 2)
      cpower(&old,c_exp,&coefficient);
   else if(c_exp == 2)
   {
      coefficient.x = sqr(old.x) - sqr(old.y);
      coefficient.y = old.x * old.y * 2;
   }
   else if(c_exp < 2)
      coefficient = old;
   get_julia_attractor (0.0, 0.0);	/* an attractor? */
   return(1);
}

SierpinskiSetup()
{
   /* sierpinski */
   periodicitycheck = 0;		/* disable periodicity checks */
   ltmp.x = 1;
   ltmp.x = ltmp.x << bitshift; /* ltmp.x = 1 */
   ltmp.y = ltmp.x >> 1;			/* ltmp.y = .5 */
   return(1);
}

SierpinskiFPSetup()
{
   /* sierpinski */
   periodicitycheck = 0;		/* disable periodicity checks */
   tmp.x = 1;
   tmp.y = 0.5;
   return(1);
}

extern char usr_floatflag;

HalleySetup()
{
   /* Halley */
   periodicitycheck=0;

   if(usr_floatflag)
     fractype = HALLEY; /* float on */
   else
     fractype = MPHALLEY;

   curfractalspecific = &fractalspecific[fractype];

   degree = (int)parm.x;
   if(degree < 2)
      degree = 2;
   param[0] = (double)degree;

/*  precalculated values */
   AplusOne = degree + 1; /* a+1 */
   Ap1deg = AplusOne * degree;

#ifndef XFRACT
   if(fractype == MPHALLEY) {
      setMPfunctions();
      mpAplusOne = *pd2MP((double)AplusOne);
      mpAp1deg = *pd2MP((double)Ap1deg);
      mptmpparmy = *pd2MP(parm.y);
      mptmpparm2x = *pd2MP(parm2.x);
      mpone	   = *pd2MP(1.0);
   }
#endif

   if(degree % 2)
     symmetry = XAXIS;   /* odd */
   else
     symmetry = XYAXIS; /* even */
   return(1);
}

PhoenixSetup()
{
   longparm = &lparm; /* added to consolidate code 10/1/92 JCO */
   floatparm = &parm;
   degree = (int)parm2.x;
   if(degree < 2 && degree > -3) degree = 0;
   param[2] = (double)degree;
   if(degree == 0){
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixFractal;
   }
   if(degree >= 2){
     degree = degree - 1;
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixPlusFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixPlusFractal;
   }
   if(degree <= -3){
     degree = abs(degree) - 2;
     if(degree % 2)
       symmetry = XYAXIS; /* odd */
     else
       symmetry = XAXIS; /* even */
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixMinusFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixMinusFractal;
   }

   return(1);
}

MandPhoenixSetup()
{
   longparm = &linit; /* added to consolidate code 10/1/92 JCO */
   floatparm = &init;
   degree = (int)parm2.x;
   if(degree < 2 && degree > -3) degree = 0;
   param[2] = (double)degree;
   if(degree == 0){
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixFractal;
   }
   if(degree >= 2){
     degree = degree - 1;
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixPlusFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixPlusFractal;
   }
   if(degree <= -3){
     degree = abs(degree) - 2;
     if(usr_floatflag)
       curfractalspecific->orbitcalc =  PhoenixMinusFractal;
     else
       curfractalspecific->orbitcalc =  LongPhoenixMinusFractal;
   }

   return(1);
}

StandardSetup()
{
   if(fractype==UNITYFP)
      periodicitycheck=0;
   return(1);
}

demowalk()
{
    extern double param[];		/* optional user parameters */
    extern int maxit;			/* maximum iterations (steps) */
    extern int rflag, rseed;		/* random number seed */
    extern int xdots, ydots;		/* image coordinates */
    extern int colors;                  /* maximum colors available */
    extern double far *dx0, far *dy0;	/* arrays of pixel coordinates */
    extern double far *dx1, far *dy1;	/* (... for skewed zoom-boxes) */

    float stepsize;			/* average stepsize */
    int xwalk, ywalk;			/* current position */
    int xstep, ystep;			/* current step */
    int steps;				/* number of steps */
    int color;				/* color to draw this step */
    float temp, tempadjust;		/* temporary variables */

if (param[0] != 999) {			/* if 999, do a Mandelbrot instead */

    srand(rseed);			/* seed the random number generator */
    if (!rflag) ++rseed;
    tempadjust = RAND_MAX >> 2;		/* adjustment factor */

    xwalk = xdots / 2;			/* start in the center of the image */
    ywalk = ydots / 2;

    stepsize = min(xdots, ydots) 	/* calculate average stepsize */
               * (param[0]/100.0);	/* as a percentage of the image */

    color = max(0, min(colors, param[1]));  /* set the initial color */  

    for (steps = 0; steps < maxit; steps++) { /* take maxit steps */
        if (keypressed())		/* abort if told to do so */
            return(0);
        temp = rand();			/* calculate the next xstep */
        xstep = ((temp/tempadjust) - 2.0) * stepsize;
        xstep = min(xwalk + xstep, xdots - 1);
        xstep = max(0, xstep);
        temp = rand();			/* calculate the next ystep */
        ystep = ((temp/tempadjust) - 2.0) * stepsize;
        ystep = min(ywalk + ystep, ydots - 1);
        ystep = max(0, ystep);
        if (param[1] == 0.0)		/* rotate the colors? */
            if (++color >= colors)	/* rotate the colors, avoiding */
                color = 1;		/* the background color 0 */
        /* the draw_line function is borrowed from the 3D routines */
        draw_line(xwalk, ywalk,xstep,ystep,color);
        /* or, we could be on a pogo stick and just displaying
           where we landed...
        putcolor(xstep, ystep, color);
        */

        xwalk = xstep;			/* remember where we were */
        ywalk = ystep;
        }
    return(1);                          /* we're done */

} else {				/* a simple Mandelbrot routine */

    /* the following routine determines the X and Y values of
       each pixel coordinate and calculates a simple mandelbrot
       fractal with them - slowly, but surely */
    int ix, iy;
    for (iy = 0; iy < ydots; iy++) {
        for (ix = 0; ix < xdots; ix++) {
            int iter;
            double x, y, newx, newy, tempxx, tempxy, tempyy;
            /* first, obtain the X and Y coordinate values of this pixel */
            x = dx0[ix]+dx1[iy];
            y = dy0[iy]+dy1[ix];
            /* now initialize the temporary values */
            tempxx = tempyy = tempxy = 0.0;
            if (keypressed())		/* abort if told to do so */
                return(0);
            /* the inner iteration loop */
            for (iter = 1; iter < maxit; iter++) {
                /* calculate the X and Y values of Z(iter) */
                newx = tempxx - tempyy + x;
                newy = tempxy + tempxy + y;
                /* calculate the temporary values */
                tempxx = newx * newx;
                tempyy = newy * newy;
                tempxy = newx * newy;
                /* are we done yet? */
                if (tempxx + tempyy > 4.0) break;
                }
            /* color in the pixel */
            putcolor(ix, iy, iter & (colors - 1));
            }
        }
    return(1);                          /* we're done */
    }

}
