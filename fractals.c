/*
FRACTALS.C and CALCFRAC.C actually calculate the fractal images (well,
SOMEBODY had to do it!).  The modules are set up so that all logic that
is independent of any fractal-specific code is in CALCFRAC.C, and the
code that IS fractal-specific is in FRACTALS.C. Original author Tim Wegner,
but just about ALL the authors have contributed SOME code to this routine
at one time or another, or contributed to one of the many massive
restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbit_calc. EVERY
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
#include "fractint.h"
#include "mpmath.h"

#ifndef __TURBOC__
#include <malloc.h>
#endif

#include "fractype.h"

#define NEWTONDEGREELIMIT  100

extern struct complex  initorbit;
extern struct lcomplex linitorbit;
extern char useinitorbit;
 
/* -------------------------------------------------------------------- */
/*   The following #defines allow the complex transcendental functions	*/
/*   in parser.c to be used here thus avoiding duplicated code. 	*/
/* -------------------------------------------------------------------- */

union Arg argfirst,argsecond;

unsigned char trigndx[] = {SIN,SQR,SINH,COSH};
void (*ltrig0)() = lStkSin;
void (*ltrig1)() = lStkSqr;
void (*ltrig2)() = lStkSinh;
void (*ltrig3)() = lStkCosh;

void (*dtrig0)() = dStkSin;
void (*dtrig1)() = dStkSqr;
void (*dtrig2)() = dStkSinh;
void (*dtrig3)() = dStkCosh;

struct trig_funct_lst trigfn[] =
/* changing the order of these alters meaning of *.fra file */
{
   {"sin", lStkSin, dStkSin },
   {"cos", lStkCos, dStkCos },
   {"sinh",lStkSinh,dStkSinh},
   {"cosh",lStkCosh,dStkCosh},
   {"exp", lStkExp, dStkExp },
   {"log", lStkLog, dStkLog },
   {"sqr", lStkSqr, dStkSqr }
};

/* switch obsolete fractal types to new generalizations */
backwardscompat()
{
    extern	int	initfractype;
    extern	int	distest;
    extern struct fractal_info read_info;
    switch(initfractype)
    {
    case LAMBDASINE:
       initfractype = LAMBDATRIGFP;
       trigndx[0] = SIN;
       break;
    case LAMBDACOS    :
       initfractype = LAMBDATRIGFP;
       trigndx[0] = COS;
       break;
    case LAMBDAEXP    :
       initfractype = LAMBDATRIGFP;
       trigndx[0] = EXP;
       break;
    case MANDELSINE   :
       initfractype = MANDELTRIGFP;
       trigndx[0] = SIN;
       break;
    case MANDELCOS    :
       initfractype = MANDELTRIGFP;
       trigndx[0] = COS;
       break;
    case MANDELEXP    :
       initfractype = MANDELTRIGFP;
       trigndx[0] = EXP;
       break;
    case MANDELSINH   :
       initfractype = MANDELTRIGFP;
       trigndx[0] = SINH;
       break;
    case LAMBDASINH   :
       initfractype = LAMBDATRIGFP;
       trigndx[0] = SINH;
       break;
    case MANDELCOSH   :
       initfractype = MANDELTRIGFP;
       trigndx[0] = COSH;
       break;
    case LAMBDACOSH   :
       initfractype = LAMBDATRIGFP;
       trigndx[0] = COSH;
       break;
    case LMANDELSINE  :
       initfractype = MANDELTRIG;
       trigndx[0] = SIN;
       break;
    case LLAMBDASINE  :
       initfractype = LAMBDATRIG;
       trigndx[0] = SIN;
       break;
    case LMANDELCOS   :
       initfractype = MANDELTRIG;
       trigndx[0] = COS;
       break;
    case LLAMBDACOS   :
       initfractype = LAMBDATRIG;
       trigndx[0] = COS;
       break;
    case LMANDELSINH  :
       initfractype = MANDELTRIG;
       trigndx[0] = SINH;
       break;
    case LLAMBDASINH  :
       initfractype = LAMBDATRIG;
       trigndx[0] = SINH;
       break;
    case LMANDELCOSH  :
       initfractype = MANDELTRIG;
       trigndx[0] = COSH;
       break;
    case LLAMBDACOSH  :
       initfractype = LAMBDATRIG;
       trigndx[0] = COSH;
       break;
    case LMANDELEXP   :
       initfractype = MANDELTRIG;
       trigndx[0] = EXP;
       break;
    case LLAMBDAEXP   :
       initfractype = LAMBDATRIG;
       trigndx[0] = EXP;
       break;
    case DEMM	      :
       initfractype = MANDELFP;
       distest = (read_info.ydots - 1) * 2;
       break;
    case DEMJ	      :
       initfractype = JULIAFP;
       distest = (read_info.ydots - 1) * 2;
       break;
    case MANDELLAMBDA :
       useinitorbit = 2;
       break;       
   }
   return(0);
}

/* set array of trig function indices according to "function=" command */
set_trig_array(int k, char *name)
{
   char trigname[6];  
   int i, lstlen;
   char *slash;
   strncpy(trigname,name,5);
   trigname[5] = 0; /* safety first */
   
   if ((slash = strchr(trigname,'/')))
      *slash = 0;
   
   strlwr(trigname);
   lstlen = sizeof(trigfn)/sizeof(struct trig_funct_lst);

   for(i=0;i<lstlen;i++)
   {
      if(strcmp(trigname,trigfn[i].name)==0)
      {
	 trigndx[k] = i;
	 set_trig_pointers(k);
	 break;
      }
   }
   return(0);
}
set_trig_pointers(int which)
{
  /* set trig variable functions to avoid array lookup time */
   int i;
   switch(which)
   {
   case 0:
      ltrig0 = trigfn[trigndx[0]].lfunct;
      dtrig0 = trigfn[trigndx[0]].dfunct;
      break;
   case 1:
      ltrig1 = trigfn[trigndx[1]].lfunct;
      dtrig1 = trigfn[trigndx[1]].dfunct;
      break;
   case 2:
      ltrig2 = trigfn[trigndx[2]].lfunct;
      dtrig2 = trigfn[trigndx[2]].dfunct;
      break;
   case 3:
      ltrig3 = trigfn[trigndx[3]].lfunct;
      dtrig3 = trigfn[trigndx[3]].dfunct;
      break;
   default: /* do 'em all */
      for(i=0;i<4;i++)
	 set_trig_pointers(i);
      break;
   }
   return(0);
}
typedef  struct complex CMPLX;
typedef  struct lcomplex LCMPLX;

#define CMPLXmod(z)	  (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)	((z).y =  -((z).y))
#define LCMPLXmod(z)	   (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)	((z).y =  -((z).y))


#define LCMPLXtrig0(arg,out) Arg1->l = (arg); ltrig0(); (out)=Arg1->l
#define LCMPLXtrig1(arg,out) Arg1->l = (arg); ltrig1(); (out)=Arg1->l
#define LCMPLXtrig2(arg,out) Arg1->l = (arg); ltrig2(); (out)=Arg1->l
#define LCMPLXtrig3(arg,out) Arg1->l = (arg); ltrig3(); (out)=Arg1->l

#define  CMPLXtrig0(arg,out) Arg1->d = (arg); dtrig0(); (out)=Arg1->d
#define  CMPLXtrig1(arg,out) Arg1->d = (arg); dtrig1(); (out)=Arg1->d
#define  CMPLXtrig2(arg,out) Arg1->d = (arg); dtrig2(); (out)=Arg1->d
#define  CMPLXtrig3(arg,out) Arg1->d = (arg); dtrig3(); (out)=Arg1->d


#define LCMPLXsin(arg,out)   Arg1->l = (arg); lStkSin();  (out) = Arg1->l
#define LCMPLXcos(arg,out)   Arg1->l = (arg); lStkCos();  (out) = Arg1->l
#define LCMPLXsinh(arg,out)  Arg1->l = (arg); lStkSinh(); (out) = Arg1->l
#define LCMPLXcosh(arg,out)  Arg1->l = (arg); lStkCosh(); (out) = Arg1->l
#define LCMPLXlog(arg,out)   Arg1->l = (arg); lStkLog();  (out) = Arg1->l
#define LCMPLXexp(arg,out)   Arg1->l = (arg); lStkExp();  (out) = Arg1->l
/*
#define LCMPLXsqr(arg,out)   Arg1->l = (arg); lStkSqr();  (out) = Arg1->l
*/
#define LCMPLXsqr(arg,out)   \
   (out).x = lsqr((arg).x) - lsqr((arg).y);\
   (out).y = multiply((arg).x, (arg).y, bitshiftless1)
#define LCMPLXsqr_old(out)	 \
   (out).x = ltempsqrx - ltempsqry;\
   (out).y = multiply(lold.x, lold.y, bitshiftless1)

#define LCMPLXpwr(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
	 lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l
#define LCMPLXmult(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
	 lStkMul(); Arg1++; Arg2++; (out) = Arg2->l
#define LCMPLXadd(arg1,arg2,out)    \
    (out).x = (arg1).x + (arg2).x; (out).y = (arg1).y + (arg2).y
#define LCMPLXsub(arg1,arg2,out)    \
    (out).x = (arg1).x - (arg2).x; (out).y = (arg1).y - (arg2).y

#define LCMPLXtimesreal(arg,real,out)	\
    (out).x = multiply((arg).x,(real),bitshift);\
    (out).y = multiply((arg).y,(real),bitshift)

#define LCMPLXrecip(arg,out)	\
{ long denom; denom = lsqr((arg).x) + lsqr((arg).y);\
if(denom==0L) overflow=1; else {(out).x = divide((arg).x,denom,bitshift);\
(out).y = -divide((arg).y,denom,bitshift);}}

#define CMPLXsin(arg,out)    Arg1->d = (arg); dStkSin();  (out) = Arg1->d
#define CMPLXcos(arg,out)    Arg1->d = (arg); dStkCos();  (out) = Arg1->d
#define CMPLXsinh(arg,out)   Arg1->d = (arg); dStkSinh(); (out) = Arg1->d
#define CMPLXcosh(arg,out)   Arg1->d = (arg); dStkCosh(); (out) = Arg1->d
#define CMPLXlog(arg,out)    Arg1->d = (arg); dStkLog();  (out) = Arg1->d
#define CMPLXexp(arg,out)    FPUcplxexp(&(arg), &(out))
/*
#define CMPLXsqr(arg,out)    Arg1->d = (arg); dStkSqr();  (out) = Arg1->d
*/
#define CMPLXsqr(arg,out)    \
   (out).x = sqr((arg).x) - sqr((arg).y);\
   (out).y = ((arg).x+(arg).x) * (arg).y
#define CMPLXsqr_old(out)	\
   (out).x = tempsqrx - tempsqry;\
   (out).y = (old.x+old.x) * old.y

#define CMPLXpwr(arg1,arg2,out)   (out)= ComplexPower((arg1), (arg2))
#define CMPLXmult1(arg1,arg2,out)    Arg2->d = (arg1); Arg1->d = (arg2);\
	 dStkMul(); Arg1++; Arg2++; (out) = Arg2->d
#define CMPLXmult(arg1,arg2,out)  \
	{\
	   CMPLX TmP;\
	   TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;\
	   TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;\
	   (out) = TmP;\
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
long Exp086(long);
double fmod(double,double);
extern int biomorph;
extern int forcesymmetry;
extern int symmetry;
LCMPLX lcoefficient,lold,lnew,lparm, linit,ltmp,ltmp2,lparm2;
long ltempsqrx,ltempsqry;
extern int decomp[];
extern double param[];
extern double f_radius,f_xcenter,f_ycenter;    /* inversion radius, center */
extern double xxmin,xxmax,yymin,yymax;	       /* corners */
extern VECTOR view;
extern int overflow;
extern int orbit_ptr;		/* pointer into save_orbit array */
extern int integerfractal;	/* TRUE if fractal uses integer math */
extern int kbdcount;

int maxcolor;
int root, degree,basin;
double floatmin,floatmax;
double roverd, d1overd, threshold;
CMPLX tmp2;
extern CMPLX init,tmp,old,new,saved,ComplexPower();
CMPLX	staticroots[16]; /* roots array for degree 16 or less */
CMPLX	*roots = staticroots;
struct MPC	*MPCroots;
extern int color, oldcolor, oldmax, oldmax10, row, col;
extern int invert;
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;
extern long XXOne, FgOne, FgTwo;
long FgHalf;

CMPLX one;
CMPLX pwr;
CMPLX Coefficient;

extern int StandardPixel();
extern void (*plot)();

extern int  alloc_resume(int,int);	/* save/resume stuff */
extern int  start_resume();
extern void end_resume();
extern int  put_resume(int len, ...);
extern int  get_resume(int len, ...);
extern int  calc_status, resuming;

extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors; 			/* maximum colors available */
extern int	inside; 			/* "inside" color to use    */
extern int	finattract;
extern int  min_index;		/* iteration for min_orbit */
extern double min_orbit;	/* orbit value closest to origin */
extern int	maxit;				/* try this many iterations */
extern int	fractype;			/* fractal type */
extern int	debugflag;			/* for debugging purposes */

extern double	param[];		/* parameters */
extern long	far *lx0, far *ly0;		/* X, Y points */
extern long	far *lx1, far *ly1;		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	delmin; 			/* min(max(dx),max(dy) */
extern long	ddelmin;			/* min(max(dx),max(dy) */
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
#define pMPsqr(z) (pMPmul((z),(z)))
#define MPdistance(z1,z2)  (pMPadd(pMPsqr(pMPsub((z1).x,(z2).x)),pMPsqr(pMPsub((z1).y,(z2).y))))

double twopi = PI*2.0;
static int c_exp;

/* These are local but I don't want to pass them as parameters */
extern CMPLX lambda;
extern double deltaX, deltaY;
extern double magnitude, rqlim, rqlim2;
CMPLX parm,parm2;
CMPLX *floatparm;
LCMPLX *longparm;
extern int (*calctype)();
extern FILE *fp_pot;
extern int potflag; /* tells if continuous potential on  */
extern unsigned long lm;		/* magnitude limit (CALCMAND) */

extern int ixstart, ixstop, iystart, iystop; /* start, stop here */

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

double sinx,cosx,sinhx,coshx;
double siny,cosy,sinhy,coshy;
double tmpexp;
double tempsqrx,tempsqry;

double foldxinitx,foldyinity,foldxinity,foldyinitx;
long oldxinitx,oldyinity,oldxinity,oldyinitx;
long longtmp;
extern long lmagnitud, llimit, llimit2, l16triglim;
extern periodicitycheck;
extern char floatflag;

extern int StandardFractal();
extern int NewtonFractal2(); /* Lee Crocker's Newton code */

/* these are in mpmath_c.c */
extern int ComplexNewtonSetup(void);
extern int ComplexNewton(void), ComplexBasin(void), MarksCplxMand(void);
extern int MarksCplxMandperp(void);

/* these are in (I think) JB.C */
extern int Std4dFractal(), JulibrotSetup(), jb_per_pixel();

/* temporary variables for trig use */
long lcosx, lcoshx, lsinx, lsinhx;
long lcosy, lcoshy, lsiny, lsinhy;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/
extern	      int      attractors; /* number of finite attractors   */
extern CMPLX  attr[];	   /* finite attractor values (f.p) */
extern LCMPLX lattr[];	  /* finite attractor values (int) */

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
CMPLX	T_Cm1;	      /* 3 * (floatparm - 1)		    */
CMPLX	T_Cm2;	      /* 3 * (floatparm - 2)		    */
CMPLX	T_Cm1Cm2;     /* (floatparm - 1) * (floatparm - 2) */

FloatPreCalcMagnet2() /* precalculation for Magnet2 (M & J) for speed */
  {
    T_Cm1.x = floatparm->x - 1.0;   T_Cm1.y = floatparm->y;
    T_Cm2.x = floatparm->x - 2.0;   T_Cm2.y = floatparm->y;
    T_Cm1Cm2.x = (T_Cm1.x * T_Cm2.x) - (T_Cm1.y * T_Cm2.y);
    T_Cm1Cm2.y = (T_Cm1.x * T_Cm2.y) + (T_Cm1.y * T_Cm2.x);
    T_Cm1.x += T_Cm1.x + T_Cm1.x;   T_Cm1.y += T_Cm1.y + T_Cm1.y;
    T_Cm2.x += T_Cm2.x + T_Cm2.x;   T_Cm2.y += T_Cm2.y + T_Cm2.y;
  }

/* -------------------------------------------------------------------- */
/*		Stand-alone routines											*/
/* -------------------------------------------------------------------- */

extern char far plasmamessage[];
extern int orbit2dfloat();
extern int orbit2dlong();
extern int kamtorusfloatorbit();
extern int kamtoruslongorbit();

/* functions defined elswhere needed for fractalspecific */
extern int orbit3dfloat();
extern int orbit3dlong();
extern int lorenz3dlongorbit();
extern int orbit3dlongsetup();
extern int lorenz3dfloatorbit();
extern int orbit3dfloatsetup();
extern int rosslerfloatorbit();
extern int rosslerlongorbit();
extern int henonfloatorbit();
extern int henonlongorbit();
extern int pickoverfloatorbit();
extern int gingerbreadfloatorbit();
extern int diffusion();
extern int ifs();
extern int ifs3d();

/***************************************************************/
/* The following code now forms a generalised Fractal Engine   */
/* for Bifurcation fractal typeS.  By rights it now belongs in */
/* CALCFRACT.C, but it's easier for me to leave it here !      */

/* Original code by Phil Wilson, hacked around by Kev Allen.   */

/* Besides generalisation, enhancements include Periodicity    */
/* Checking during the plotting phase (AND halfway through the */
/* filter cycle, if possible, to halve calc times), quicker    */
/* floating-point calculations for the standard Verhulst type, */
/* and new bifurcation types (integer bifurcation, f.p & int   */
/* biflambda - the real equivalent of complex Lambda sets -    */
/* and f.p renditions of bifurcations of r*sin(Pi*p), which    */
/* spurred Mitchel Feigenbaum on to discover his Number).      */

/* To add further types, extend the fractalspecific[] array in */
/* usual way, with Bifurcation as the engine, and the name of  */
/* the routine that calculates the next bifucation generation  */
/* as the "orbitcalc" routine in the fractalspecific[] entry.  */

/* Bifurcation "orbitcalc" routines get called once per screen */
/* pixel column.  They should calculate the next generation    */
/* from the doubles Rate & Population (or the longs lRate &    */
/* lPopulation if they use integer math), placing the result   */
/* back in Population (or lPopulation).  They should return 0  */
/* if all is ok, or any non-zero value if calculation bailout  */
/* is desirable (eg in case of errors, or the series tending   */
/* to infinity).		Have fun !		       */
/***************************************************************/

#define BIG 100000.0
#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
			       (p.25), but that seems unnecessary. Can
			       override this value with a nonzero param1 */

#define SEED 0.66		/* starting value for population */

static void verhulst (void);

int far *verhulst_array;
unsigned int filter_cycles;

unsigned int	half_time_check;
	 long	lPopulation, lRate;
	 double Population,  Rate;
static	 int	mono, outside_x;

int Bifurcation(void)
{
   unsigned long array_size;
   int row, column;
   column = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&column,0);
      end_resume();
   }

   array_size =  (iystop + 2) * sizeof(int);
   if ((verhulst_array = (int far *) farmemalloc(array_size)) == NULL)
   {
      setfortext();
      printf("\n\n\n\n\nInsufficient free memory for calculation.");
      printf("\n\n\nHit any key to continue.\n");
      buzzer(2);
      getakey();
      setforgraphics();
      return(-1);
   }

   for (row = 0; row <= iystop+1; row++)
      verhulst_array[row] = 0;

   mono = 0;
   if (colors == 2)
      mono = 1;
   if (mono)
   {
      if (inside)
      {
	 outside_x = 0;
	 inside = 1;
      }
      else
	 outside_x = 1;
   }

   if ((filter_cycles = parm.x) == 0)
      filter_cycles = DEFAULTFILTER;
   half_time_check = FALSE;
   if (periodicitycheck && maxit < filter_cycles)
   {
      filter_cycles = (filter_cycles - maxit + 1) / 2;
      half_time_check = TRUE;
   }

   if (integerfractal)
      linit.y = ly0[iystop];   /* Y-value of	*/
   else
      init.y = dy0[iystop];   /* bottom pixels */

   while (column <= ixstop)
   {
      if(check_key())
      {
	 farmemfree((char far *)verhulst_array);
	 alloc_resume(10,1);
	 put_resume(sizeof(int),&column,0);
	 return(-1);
      }
      if (integerfractal)
	 lRate = lx0[column];
      else
	 Rate = dx0[column];
      verhulst();	 /* calculate array once per column */

      for (row = iystop+1; row > 0; row--)
      {
	 int color;
	 color = verhulst_array[row];
	 if(color && mono)
	    color = inside;
	 else if((!color) && mono)
	    color = outside_x;
	 verhulst_array[row] = 0;
	 (*plot)(column,row,color);
      }
      column++;
   }
   farmemfree((char far *)verhulst_array);
   return(0);
}

static void verhulst()		/* P. F. Verhulst (1845) */
{
   unsigned int pixel_row, counter, errors;

   if (integerfractal)
      lPopulation = SEED * fudge;
   else
      Population = SEED;

   errors = overflow = FALSE;

   for (counter=0 ; counter < filter_cycles ; counter++)
   {
      errors = (*fractalspecific[fractype].orbitcalc)();
      if (errors)
	 return;
   }

   if (half_time_check) /* check for periodicity at half-time */
   {
      Bif_Period_Init();
      for (counter=0 ; counter < maxit ; counter++)
      {
	 errors = (*fractalspecific[fractype].orbitcalc)();
	 if (errors) return;
	 if (periodicitycheck && Bif_Periodic(counter)) break;
      }
      if (counter >= maxit)   /* if not periodic, go the distance */
      {
	 for (counter=0 ; counter < filter_cycles ; counter++)
	 {
	    errors = (*fractalspecific[fractype].orbitcalc)();
	    if (errors) return;
	 }
      }
   }

   if (periodicitycheck) Bif_Period_Init();

   for (counter=0 ; counter < maxit ; counter++)
   {
      errors = (*fractalspecific[fractype].orbitcalc)();
      if (errors) return;

      /* assign population value to Y coordinate in pixels */
      if (integerfractal)
	 pixel_row = iystop + 1 - (lPopulation - linit.y) / dely;
      else
	 pixel_row = iystop + 1 - (int)((Population - init.y) / deltaY);

      /* if it's visible on the screen, save it in the column array */
      if ((pixel_row >= 0) && (pixel_row <= iystop + 1))
	 verhulst_array[ pixel_row ] ++;

      if (periodicitycheck && Bif_Periodic(counter))
      {
	 if ((pixel_row >= 0) && (pixel_row <= iystop + 1))
	    verhulst_array[ pixel_row ] --;
	 break;
      }
   }
}
static	long	lBif_closenuf, lBif_savedpop;	/* poss future use  */
static	double	 Bif_closenuf,	Bif_savedpop;
static	int	 Bif_savedinc,	Bif_savedand;

Bif_Period_Init()
{
   Bif_savedinc = 1;
   Bif_savedand = 1;
   if (integerfractal)
   {
      lBif_savedpop = -1;
      lBif_closenuf = dely / 8;
   }
   else
   {
      Bif_savedpop = -1.0;
      Bif_closenuf = deltaY / 8.0;
   }
}

Bif_Periodic (time)	/* Bifurcation Population Periodicity Check */
int time;		/* Returns : 1 if periodicity found, else 0 */
{
   if ((time & Bif_savedand) == 0)	/* time to save a new value */
   {
      if (integerfractal) lBif_savedpop = lPopulation;
      else		     Bif_savedpop =  Population;
      if (--Bif_savedinc == 0)
      {
	 Bif_savedand = (Bif_savedand << 1) + 1;
	 Bif_savedinc = 4;
      }
   }
   else 			/* check against an old save */
   {
      if (integerfractal)
      {
	 if (labs(lBif_savedpop-lPopulation) <= lBif_closenuf)
	    return(1);
      }
      else
      {
	 if (fabs(Bif_savedpop-Population) <= Bif_closenuf)
	    return(1);
      }
   }
   return(0);
}

/* Here Endeth the Generalised Bifurcation Fractal Engine   */

/* END Phil Wilson's Code (modified slightly by Kev Allen et. al. !) */

/* -------------------------------------------------------------------- */
/*		Bailout Routines Macros 												*/
/* -------------------------------------------------------------------- */

#define FLOATBAILOUT()	 \
   tempsqrx = sqr(new.x);\
   tempsqry = sqr(new.y);\
   if((magnitude = tempsqrx + tempsqry) >= rqlim) return(1);\
   old = new;

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
   if (labs(lold.y) >= 1000L<<bitshift) return(1);\
   if (labs(lold.x) >=	8L<<bitshift) return(1);

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

/**********************************************************************/
/*								                                      */
/* The following are Bifurcation "orbitcalc" routines...              */
/*								                                      */
/**********************************************************************/

BifurcVerhulst()
  {
    Population += Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }

LongBifurcVerhulst()
  {
    ltmp.y = lPopulation - multiply(lPopulation,lPopulation,bitshift);
    lPopulation += multiply(lRate,ltmp.y,bitshift);
    return (overflow);
  }

BifurcLambda()
  {
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }

LongBifurcLambda()
  {
    ltmp.y = lPopulation - multiply(lPopulation,lPopulation,bitshift);
    lPopulation = multiply(lRate,ltmp.y,bitshift);
    return (overflow);
  }

BifurcAddSinPi()
  {
    Population += Rate * sin(PI*Population);
    return (fabs(Population) > BIG);
  }

BifurcSetSinPi()
  {
    Population = Rate * sin(PI*Population);
    return (fabs(Population) > BIG);
  }

/* -------------------------------------------------------------------- */
/*		Fractal (once per iteration) routines			*/
/* -------------------------------------------------------------------- */
static double xt, yt, t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
cpower(CMPLX *base, int exp, CMPLX *result)
{
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
static long lxt, lyt, lt1, lt2;
lcpower(LCMPLX *base, int exp, LCMPLX *result, int bitshift)
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

z_to_the_z(CMPLX *z, CMPLX *out)
{
    static CMPLX tmp1,tmp2;
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

/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-fudge)) + lsqr(((z).y)))

#ifdef NEWTON
complex_mult(CMPLX arg1,CMPLX arg2,CMPLX *pz);
complex_div(CMPLX arg1,CMPLX arg2,CMPLX *pz);

int NewtonFractal()
{
    static char start=1;
    if(start)
    {
       printf("c version");
       start = 0;
    }
    cpower(&old, degree-1, &tmp);
    complex_mult(tmp, old, &new);

    if (DIST1(new) < threshold)
    {
       if(fractype==NEWTBASIN)
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
	       /*    tmpcolor = 1+(i&7)+((color&1)<<3); */
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


complex_mult(arg1,arg2,pz)
CMPLX arg1,arg2,*pz;
{
   pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
   pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
   return(0);
}

complex_div(numerator,denominator,pout)
CMPLX numerator,denominator,*pout;
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


lcomplex_mult(arg1,arg2,pz,bitshift)
LCMPLX arg1,arg2,*pz;
int bitshift;
{
   overflow = 0;
   pz->x = multiply(arg1.x,arg2.x,bitshift) - multiply(arg1.y,arg2.y,bitshift);
   pz->y = multiply(arg1.x,arg2.y,bitshift) + multiply(arg1.y,arg2.x,bitshift);
   return(overflow);
}

#endif

#define MPCmod(m) (pMPadd(pMPmul((m).x, (m).x), pMPmul((m).y, (m).y)))
struct MPC mpcold, mpcnew, mpctmp, mpctmp1;
struct MP mproverd, mpd1overd, mpthreshold,sqrmpthreshold;
struct MP mpt2;
struct MP mpone;
extern struct MPC MPCone;
extern int MPOverflow;
int MPCNewtonFractal()
{
    MPOverflow = 0;
    mpctmp   = MPCpow(mpcold,degree-1);

    mpcnew.x = pMPsub(pMPmul(mpctmp.x,mpcold.x),pMPmul(mpctmp.y,mpcold.y));
    mpcnew.y = pMPadd(pMPmul(mpctmp.x,mpcold.y),pMPmul(mpctmp.y,mpcold.x));

    mpctmp1.x = pMPsub(mpcnew.x, MPCone.x);
    mpctmp1.y = pMPsub(mpcnew.y, MPCone.y);

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

    mpcnew.x = pMPadd(pMPmul(mpd1overd,mpcnew.x),mproverd);
    mpcnew.y = pMPmul(mpcnew.y,mpd1overd);

    mpt2 = MPCmod(mpctmp);
    mpt2 = pMPdiv(mpone,mpt2);

    mpcold.x = pMPmul(mpt2,(pMPadd(pMPmul(mpcnew.x,mpctmp.x),pMPmul(mpcnew.y,mpctmp.y))));
    mpcold.y = pMPmul(mpt2,(pMPsub(pMPmul(mpcnew.y,mpctmp.x),pMPmul(mpcnew.x,mpctmp.y))));

    new.x = *pMP2d(mpcold.x);
    new.y = *pMP2d(mpcold.y);
    return(MPOverflow);
}


Barnsley1Fractal()
{
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
   LONGBAILOUT();
   return(0);
}

Barnsley1FPFractal()
{
   /* Barnsley's Mandelbrot type M1 from "Fractals
   Everywhere" by Michael Barnsley, p. 322 */

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
   FLOATBAILOUT();
   return(0);
}

Barnsley2Fractal()
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 331, example 4.2 */

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
   LONGBAILOUT();
   return(0);
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
   FLOATBAILOUT();
   return(0);
}

JuliaFractal()
{
   /* used for C prototype of fast integer math routines for classic
      Mandelbrot and Julia */
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   LONGBAILOUT();
   return(0);
}

JuliafpFractal()
{
   /* floating point version of classical Mandelbrot/Julia */
   new.x = tempsqrx - tempsqry + floatparm->x;
   new.y = 2.0 * old.x * old.y + floatparm->y;
   FLOATBAILOUT();
   return(0);
}

LambdaFPFractal()
{
   /* variation of classical Mandelbrot/Julia */

   tempsqrx = old.x - old.x * old.x + old.y * old.y;
   tempsqry = old.y - old.y * old.x * 2;

   new.x = floatparm->x * tempsqrx - floatparm->y * tempsqry;
   new.y = floatparm->x * tempsqry + floatparm->y * tempsqrx;
   FLOATBAILOUT();
   return(0);
}

LambdaFractal()
{
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
   LONGBAILOUT();
   return(0);
}

SierpinskiFractal()
{
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
   lnew.x = (lold.x << 1);		/* new.x = 2 * old.x  */
   lnew.y = (lold.y << 1);		/* new.y = 2 * old.y  */
   if(lold.y > ltmp.y)	/* if old.y > .5 */
      lnew.y = lnew.y - ltmp.x; /* new.y = 2 * old.y - 1 */
   else if(lold.x > ltmp.y)	/* if old.x > .5 */
      lnew.x = lnew.x - ltmp.x; /* new.x = 2 * old.x - 1 */
   /* end barnsley code */
   LONGBAILOUT();
   return(0);
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
   FLOATBAILOUT();
   return(0);
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
   FLOATBAILOUT();
   return(0);
}


LongTrigPlusExponentFractal()
{
   /* calculate exp(z) */

   /* domain check for fast transcendental functions */
   TRIG16CHECK(lold.x);
   TRIG16CHECK(lold.y);

   longtmp = Exp086(lold.x);
   SinCos086  (lold.y, &lsiny,	&lcosy);
   LCMPLXtrig0(lold,lnew);
   lnew.x += multiply(longtmp,	  lcosy,   bitshift) + longparm->x;
   lnew.y += multiply(longtmp,	  lsiny,   bitshift) + longparm->y;
   LONGBAILOUT();
   return(0);
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

   LONGBAILOUT();
   return(0);
}


UnityFractal()
{
   /* brought to you by Mark Peterson - you won't find this in any fractal
      books unless they saw it here first - Mark invented it! */
   XXOne = multiply(lold.x, lold.x, bitshift) + multiply(lold.y, lold.y, bitshift);
   if((XXOne > FgTwo) || (labs(XXOne - FgOne) < delmin))
      return(1);
   lold.y = multiply(FgTwo - XXOne, lold.x, bitshift);
   lold.x = multiply(FgTwo - XXOne, lold.y, bitshift);
   lnew=lold;  /* TW added this line */
   return(0);
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
   LONGBAILOUT();

   /* then, compute ((x + iy)**2)**2 + lambda */
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   LONGBAILOUT();
   return(0);
}

floatZtozPluszpwrFractal()
{
   cpower(&old,(int)param[2],&new);
   old = ComplexPower(old,old);
   new.x = new.x + old.x +floatparm->x;
   new.y = new.y + old.y +floatparm->y;
   FLOATBAILOUT();
   return(0);
}

longZpowerFractal()
{
   if(lcpower(&lold,c_exp,&lnew,bitshift))
      lnew.x = lnew.y = 8L<<bitshift;
   lnew.x += longparm->x;
   lnew.y += longparm->y;
   LONGBAILOUT();
   return(0);
}
floatZpowerFractal()
{
   cpower(&old,c_exp,&new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   FLOATBAILOUT();
   return(0);
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
   LONGBAILOUT();
   return(0);
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
   FLOATBAILOUT();
   return(0);
}

TrigPlusZsquaredFractal()
{
   /* From Scientific American, July 1989 */
   /* A Biomorph			  */
   /* z(n+1) = trig(z(n))+z(n)**2+C	  */
   LCMPLXtrig0(lold,lnew);
   lnew.x += ltempsqrx - ltempsqry + longparm->x;
   lnew.y += multiply(lold.x, lold.y, bitshiftless1) + longparm->y;
   LONGBAILOUT();
   return(0);
}

TrigPlusZsquaredfpFractal()
{
   /* From Scientific American, July 1989 */
   /* A Biomorph			  */
   /* z(n+1) = trig(z(n))+z(n)**2+C	  */

   CMPLXtrig0(old,new);
   new.x += tempsqrx - tempsqry + floatparm->x;
   new.y += 2.0 * old.x * old.y + floatparm->y;
   FLOATBAILOUT();
   return(0);
}

/* main engine for orbit-popcorn, subset of std engine */
popcorn()
{
   int start_row;
   start_row = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&start_row,0);
      end_resume();
   }
   kbdcount=(cpu==386) ? 80 : 30;
   plot = noplot;
   tempsqrx = ltempsqrx = 0; /* PB added this to cover weird BAILOUTs */
   for (row = start_row; row <= iystop; row++)
   {
      oldcolor = 1;
      oldmax = min(maxit, 250);
      oldmax10 = oldmax - 10;
      for (col = 0; col <= ixstop; col++)
      {
	 if (StandardPixel() == -1) /* interrupted */
	 {
	    alloc_resume(10,1);
	    put_resume(sizeof(int),&row,0);
	    return(-1);
	 }
      }
   }
   calc_status = 4;
   return(0);
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
   new.x = old.x - .05*siny;
   new.y = old.y - .05*sinx;
   /*
   new.x = old.x - .05*sin(old.y+tan(3*old.y));
   new.y = old.y - .05*sin(old.x+tan(3*old.x));
   */
   if(plot == noplot)
   {
      plot_orbit(new.x,new.y,1+row%colors);
      old = new;
   }
   else
      FLOATBAILOUT();
   /* PB The above line is weird, not what it seems to be!  But, bracketing
	 it or always doing it (either of which seem more likely to be what
	 was intended) changes the image for the worse, so I'm not touching it.
	 Same applies to int form in next routine. */
   return(0);
}

LPopcornFractal()
{
   static long O5 = (long)(.05*(1L<<16));
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
   lnew.x = lold.x - multiply(O5,lsiny,bitshift);
   lnew.y = lold.y - multiply(O5,lsinx,bitshift);
   if(plot == noplot)
   {
      iplot_orbit(lnew.x,lnew.y,1+row%colors);
      lold = lnew;
   }
   else
      LONGBAILOUT();
   return(0);
}

int MarksCplxMand(void)
{
   tmp.x = tempsqrx - tempsqry;
   tmp.y = 2*old.x*old.y;
   FPUcplxmul(&tmp, &Coefficient, &new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   FLOATBAILOUT();
   return(0);
}

int SpiderfpFractal(void)
{
   /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
   new.x = tempsqrx - tempsqry + tmp.x;
   new.y = 2 * old.x * old.y + tmp.y;
   tmp.x = tmp.x/2 + new.x;
   tmp.y = tmp.y/2 + new.y;
   FLOATBAILOUT();
   return(0);
}

SpiderFractal(void)
{
   /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
   lnew.x  = ltempsqrx - ltempsqry + ltmp.x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y;
   ltmp.x = (ltmp.x >> 1) + lnew.x;
   ltmp.y = (ltmp.y >> 1) + lnew.y;
   LONGBAILOUT();
   return(0);
}

TetratefpFractal()
{
   /* Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) } */
   new = ComplexPower(*floatparm,old);
   FLOATBAILOUT();
   return(0);
}

ZXTrigPlusZFractal()
{
   /* z = (p1*z*trig(z))+p2*z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)	     */
   LCMPLXmult(lparm,ltmp,ltmp);      /* ltmp  = p1*trig(old)	      */
   LCMPLXmult(lold,ltmp,ltmp2);      /* ltmp2 = p1*old*trig(old)      */
   LCMPLXmult(lparm2,lold,ltmp);     /* ltmp  = p2*old		      */
   LCMPLXadd(ltmp2,ltmp,lnew);	     /* lnew  = p1*trig(old) + p2*old */
   LONGBAILOUT();
   return(0);
}

ScottZXTrigPlusZFractal()
{
   /* z = (z*trig(z))+z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)       */
   LCMPLXmult(lold,ltmp,lnew);	     /* lnew  = old*trig(old)	*/
   LCMPLXadd(lnew,lold,lnew);	     /* lnew  = trig(old) + old */
   LONGBAILOUT();
   return(0);
}

SkinnerZXTrigSubZFractal()
{
   /* z = (z*trig(z))-z */
   LCMPLXtrig0(lold,ltmp);	    /* ltmp  = trig(old)       */
   LCMPLXmult(lold,ltmp,lnew);	     /* lnew  = old*trig(old)	*/
   LCMPLXsub(lnew,lold,lnew);	     /* lnew  = trig(old) - old */
   LONGBAILOUT();
   return(0);
}

ZXTrigPlusZfpFractal()
{
   /* z = (p1*z*trig(z))+p2*z */
   CMPLXtrig0(old,tmp); 	 /* tmp  = trig(old)		 */
   CMPLXmult(parm,tmp,tmp);	 /* tmp  = p1*trig(old) 	 */
   CMPLXmult(old,tmp,tmp2);	 /* tmp2 = p1*old*trig(old)	 */
   CMPLXmult(parm2,old,tmp);	 /* tmp  = p2*old		 */
   CMPLXadd(tmp2,tmp,new);	 /* new  = p1*trig(old) + p2*old */
   FLOATBAILOUT();
   return(0);
}

ScottZXTrigPlusZfpFractal()
{
   /* z = (z*trig(z))+z */
   CMPLXtrig0(old,tmp); 	/* tmp	= trig(old)	  */
   CMPLXmult(old,tmp,new);	 /* new  = old*trig(old)   */
   CMPLXadd(new,old,new);	 /* new  = trig(old) + old */
   FLOATBAILOUT();
   return(0);
}

SkinnerZXTrigSubZfpFractal()
{
   /* z = (z*trig(z))-z */
   CMPLXtrig0(old,tmp); 	/* tmp	= trig(old)	  */
   CMPLXmult(old,tmp,new);	 /* new  = old*trig(old)   */
   CMPLXsub(new,old,new);	 /* new  = trig(old) - old */
   FLOATBAILOUT();
   return(0);
}

Sqr1overTrigFractal()
{
   /* z = sqr(1/trig(z)) */
   LCMPLXtrig0(lold,lold);
   LCMPLXrecip(lold,lold);
   LCMPLXsqr(lold,lnew);
   LONGBAILOUT();
   return(0);
}

Sqr1overTrigfpFractal()
{
   /* z = sqr(1/trig(z)) */
   CMPLXtrig0(old,old);
   CMPLXrecip(old,old);
   CMPLXsqr(old,new);
   FLOATBAILOUT();
   return(0);
}

TrigPlusTrigFractal()
{
   /* z = trig(0,z)*p1+trig1(z)*p2 */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXmult(lparm,ltmp,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXmult(lparm2,ltmp2,lold);
   LCMPLXadd(ltmp,lold,lnew);
   LONGBAILOUT();
   return(0);
}

TrigPlusTrigfpFractal()
{
   /* z = trig0(z)*p1+trig1(z)*p2 */
   CMPLXtrig0(old,tmp);
   CMPLXmult(parm,tmp,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(parm2,old,old);
   CMPLXadd(tmp,old,new);
   FLOATBAILOUT();
   return(0);
}


ScottTrigPlusTrigFractal()
{
   /* z = trig0(z)+trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,lold);
   LCMPLXadd(ltmp,lold,lnew);
   LONGBAILOUT();
   return(0);
}

ScottTrigPlusTrigfpFractal()
{
   /* z = trig0(z)+trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXadd(tmp,tmp2,new);
   FLOATBAILOUT();
   return(0);
}

SkinnerTrigSubTrigFractal()
{
   /* z = trig(0,z)-trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXsub(ltmp,ltmp2,lnew);
   LONGBAILOUT();
   return(0);
}

SkinnerTrigSubTrigfpFractal()
{
   /* z = trig0(z)-trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXsub(tmp,tmp2,new);
   FLOATBAILOUT();
   return(0);
}


TrigXTrigfpFractal()
{
   /* z = trig0(z)*trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(tmp,old,new);
   FLOATBAILOUT();
   return(0);
}

TrigXTrigFractal()
{
   LCMPLX ltmp2;
   /* z = trig0(z)*trig1(z) */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXtrig1(lold,ltmp2);
   LCMPLXmult(ltmp,ltmp2,lnew);
   if(overflow)
      TryFloatFractal(TrigXTrigfpFractal);
   LONGBAILOUT();
   return(0);
}

 /* call float version of fractal if integer math overflow */
TryFloatFractal(void (*fpFractal)())
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
   /* { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT } */
   LCMPLXtrig0(lold,ltmp);     /* ltmp = trig(lold)			   */
   LCMPLXmult(lparm,ltmp,lnew); /* lnew = lparm*trig(lold)		    */
   LCMPLXsqr_old(ltmp); 	/* ltmp = sqr(lold)			    */
   LCMPLXmult(lparm2,ltmp,ltmp);/* ltmp = lparm2*sqr(lold)		    */
   LCMPLXadd(lnew,ltmp,lnew);	/* lnew = lparm*trig(lold)+lparm2*sqr(lold) */
   LONGBAILOUT();
   return(0);
}

TrigPlusSqrfpFractal() /* generalization of Scott and Skinner types */
{
   /* { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,tmp);     /* tmp = trig(old)			   */
   CMPLXmult(parm,tmp,new); /* new = parm*trig(old)		   */
   CMPLXsqr_old(tmp);	     /* tmp = sqr(old)			    */
   CMPLXmult(parm2,tmp,tmp2); /* tmp = parm2*sqr(old)		     */
   CMPLXadd(new,tmp2,new);    /* new = parm*trig(old)+parm2*sqr(old) */
   FLOATBAILOUT();
   return(0);
}

ScottTrigPlusSqrFractal()
{
   /*  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT } */
   LCMPLXtrig0(lold,lnew);    /* lnew = trig(lold)	     */
   LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)	      */
   LCMPLXadd(ltmp,lnew,lnew);  /* lnew = trig(lold)+sqr(lold) */
   LONGBAILOUT();
   return(0);
}

ScottTrigPlusSqrfpFractal() /* float version */
{
   /* { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old)	  */
   CMPLXsqr_old(tmp);	       /* tmp = sqr(old)	   */
   CMPLXadd(new,tmp,new);      /* new = trig(old)+sqr(old) */
   FLOATBAILOUT();
   return(0);
}

SkinnerTrigSubSqrFractal()
{
   /* { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT } 	      */
   LCMPLXtrig0(lold,lnew);    /* lnew = trig(lold)	     */
   LCMPLXsqr_old(ltmp);        /* lold = sqr(lold)	      */
   LCMPLXsub(lnew,ltmp,lnew);  /* lnew = trig(lold)-sqr(lold) */
   LONGBAILOUT();
   return(0);
}

SkinnerTrigSubSqrfpFractal()
{
   /* { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old) */
   CMPLXsqr_old(tmp);	       /* old = sqr(old)  */
   CMPLXsub(new,tmp,new);      /* new = trig(old)-sqr(old) */
   FLOATBAILOUT();
   return(0);
}

TrigZsqrdfpFractal()
{
   /* { z=pixel: z=trig(z*z), |z|<TEST } */
   CMPLXsqr_old(tmp);
   CMPLXtrig0(tmp,new);
   FLOATBAILOUT();
   return(0);
}

TrigZsqrdFractal() /* this doesn't work very well */
{
   /* { z=pixel: z=trig(z*z), |z|<TEST } */
   LCMPLXsqr_old(ltmp);
   LCMPLXtrig0(ltmp,lnew);
   if(overflow)
      TryFloatFractal(TrigZsqrdfpFractal);
   LONGBAILOUT();
   return(0);
}

SqrTrigFractal()
{
   /* { z=pixel: z=sqr(trig(z)), |z|<TEST} */
   LCMPLXtrig0(lold,ltmp);
   LCMPLXsqr(ltmp,lnew);
   LONGBAILOUT();
   return(0);
}

SqrTrigfpFractal()
{
   /* SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST} */
   CMPLXtrig0(old,tmp);
   CMPLXsqr(tmp,new);
   FLOATBAILOUT();
   return(0);
}


Magnet1Fractal()    /*	  Z = ((Z**2 + C - 1)/(2Z + C - 2))**2	  */
  {		      /*  In "Beauty of Fractals", code by Kev Allen. */
    CMPLX top, bot, tmp;
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

    FLOATBAILOUT();
    return(0);
  }

Magnet2Fractal()  /* Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)	) /	 */
		    /*	     (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2  */
  {		    /*	 In "Beauty of Fractals", code by Kev Allen.  */
    CMPLX top, bot, tmp;
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

    FLOATBAILOUT();
    return(0);
  }

LambdaTrigFractal()
{
   LONGXYTRIGBAILOUT();
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
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
   LONGTRIGBAILOUT(); /* sin,cos */
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
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
   LONGHTRIGBAILOUT(); /* sinh,cosh */
   LCMPLXtrig0(lold,ltmp);	     /* ltmp = trig(lold)	    */
   LCMPLXmult(*longparm,ltmp,lnew);   /* lnew = longparm*trig(lold)  */
   lold = lnew;
   return(0);
}

LambdaTrigfpFractal2()
{
   FLOATHTRIGBAILOUT(); /* sinh,cosh */
   CMPLXtrig0(old,tmp); 	     /* tmp = trig(old) 	  */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

ManOWarFractal()
{
   /* From Art Matrix via Lee Skinner */
   lnew.x  = ltempsqrx - ltempsqry + ltmp.x + longparm->x;
   lnew.y = multiply(lold.x, lold.y, bitshiftless1) + ltmp.y + longparm->y;
   ltmp = lold;
   LONGBAILOUT();
   return(0);
}

ManOWarfpFractal()
{
   /* From Art Matrix via Lee Skinner */
   new.x = tempsqrx - tempsqry + tmp.x + floatparm->x;
   new.y = 2.0 * old.x * old.y + tmp.y + floatparm->y;
   tmp = old;
   FLOATBAILOUT();
   return(0);
}


/* -------------------------------------------------------------------- */
/*		Initialization (once per pixel) routines						*/
/* -------------------------------------------------------------------- */

#if 0
/* this code translated to asm - lives in newton.asm */
/* transform points with reciprocal function */
void invertz1(CMPLX *z)
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
}

int long_mandel_per_pixel()
{
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
   return(0);
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
   return(0);
}

int MPCjulia_per_pixel()
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
   mpcold.x = pd2MP(old.x);
   mpcold.y = pd2MP(old.y);
   return(0);
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

/* -------------------------------------------------------------------- */
/*		Setup (once per fractal image) routines 		*/
/* -------------------------------------------------------------------- */

MandelSetup()		/* Mandelbrot Routine */
{
   extern int outside; /* there is a static int outside decl elsewhere */
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0
       && forcesymmetry == 999 && biomorph == -1 && inside != -60
       && inside != -61 && outside == -1 && useinitorbit != 1)
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
   extern int outside; /* there is a static int outside decl elsewhere */
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0
       && forcesymmetry == 999 && biomorph == -1 && inside != -60
       && inside != -61 && outside == -1 && !finattract)
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
   if(fpu != 0 && debugflag != 1010)
   {
      if(fractype == MPNEWTON)
	 fractype = NEWTON;
      else if(fractype == MPNEWTBASIN)
	 fractype = NEWTBASIN;
   }

   if(fpu == 0 && debugflag != 1010)
   {
      if(fractype == NEWTON)
	    fractype = MPNEWTON;
      else if(fractype == NEWTBASIN)
	    fractype = MPNEWTBASIN;
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
   if (fractype == MPNEWTON || fractype == MPNEWTBASIN) {
      mproverd	   = pd2MP(roverd);
      mpd1overd    = pd2MP(d1overd);
      mpthreshold  = pd2MP(threshold);
      mpone	   = pd2MP(1.0);
   }

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
	 if((roots=(CMPLX *)malloc(degree*sizeof(CMPLX)))==NULL)
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
	 roots[i].x = cos(i*PI*2.0/(double)degree);
	 roots[i].y = sin(i*PI*2.0/(double)degree);
      }
   }
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
	 MPCroots[i].x = pd2MP(cos(i*PI*2.0/(double)degree));
	 MPCroots[i].y = pd2MP(sin(i*PI*2.0/(double)degree));
      }
   }

   if (degree%4 == 0)
      symmetry = XYAXIS;
   else
      symmetry = XAXIS;

   calctype=StandardFractal;
   if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
      setMPfunctions();
   return(1);
}


StandaloneSetup()
{
   timer(fractalspecific[fractype].calctype,0);
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
   if(fractype==FPMANDELZPOWER && c_exp < 1)
      c_exp = 1;
   if(fractype==FPMANDELZPOWER && c_exp & 1) /* odd exponents */
      symmetry = XYAXIS_NOPARM;

   pwr.x = param[2] - 1.0;
   pwr.y = param[3];

   floatparm = &init;
   switch (fractype)
   {
   case MAGNET1M:
   case MAGNET2M:
      attr[0].x = 1.0;	    /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;	    /* - both MAGNET1 and MAGNET2 */
      attractors = 1;
      break;
   case SPIDERFP:
      if(periodicitycheck==1) /* if not user set */
	 periodicitycheck=4;
      break;
   case MANDELEXP:
      symmetry = XAXIS_NOPARM;
      break;
   default:
      break;
   }
   return(1);
}

JuliafpSetup()
{
   c_exp = param[2];
   if(fractype==FPJULIAZPOWER && c_exp < 1)
      c_exp = 1;
   if(fractype==FPJULIAZPOWER && c_exp & 1) /* odd exponents */
      symmetry = NOSYM;
   floatparm = &parm;
   if(fractype==COMPLEXMARKSJUL)
   {
      pwr.x = param[2] - 1.0;
      pwr.y = param[3];
      Coefficient = ComplexPower(*floatparm, pwr);
   }
   switch (fractype)
   {
   case MAGNET2J:
      FloatPreCalcMagnet2();
   case MAGNET1J:
      attr[0].x = 1.0;	    /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;	    /* - both MAGNET1 and MAGNET2 */
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
   return(1);
}

JulialongSetup()
{
   c_exp = param[2];
   if(fractype==LJULIAZPOWER && c_exp < 1)
      c_exp = 1;
   if(fractype==LJULIAZPOWER && c_exp & 1) /* odd exponents */
      symmetry = NOSYM;
   longparm = &lparm;
   switch (fractype)
   {
   case LAMBDA:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      get_julia_attractor (0.5, 0.0);	/* another attractor? */
      break;
   case LLAMBDAEXP:
      if(lparm.y == 0)
	 symmetry = XAXIS;
      break;
   default:
      get_julia_attractor (0.0, 0.0);	/* another attractor? */
      break;
   }
   return(1);
}

TrigPlusSqrlongSetup()
{
   fractalspecific[fractype].per_pixel =  julia_per_pixel;
   fractalspecific[fractype].orbitcalc =  TrigPlusSqrFractal;
   if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
   {
      if(lparm2.x == fudge)	   /* Scott variant */
	 fractalspecific[fractype].orbitcalc =	ScottTrigPlusSqrFractal;
      else if(lparm2.x == -fudge)  /* Skinner variant */
	 fractalspecific[fractype].orbitcalc =	SkinnerTrigSubSqrFractal;
   }
   return(JulialongSetup());
}

TrigPlusSqrfpSetup()
{
   fractalspecific[fractype].per_pixel =  juliafp_per_pixel;
   fractalspecific[fractype].orbitcalc =  TrigPlusSqrfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)	/* Scott variant */
	 fractalspecific[fractype].orbitcalc =	ScottTrigPlusSqrfpFractal;
      else if(parm2.x == -1.0)	/* Skinner variant */
	 fractalspecific[fractype].orbitcalc =	SkinnerTrigSubSqrfpFractal;
   }
   return(JuliafpSetup());
}

TrigPlusTriglongSetup()
{
   FnPlusFnSym();
   if(trigndx[1] == SQR)
      return(TrigPlusSqrlongSetup());
   fractalspecific[fractype].per_pixel =  long_julia_per_pixel;
   fractalspecific[fractype].orbitcalc =  TrigPlusTrigFractal;
   if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
   {
      if(lparm2.x == fudge)	   /* Scott variant */
	 fractalspecific[fractype].orbitcalc =	ScottTrigPlusTrigFractal;
      else if(lparm2.x == -fudge)  /* Skinner variant */
	 fractalspecific[fractype].orbitcalc =	SkinnerTrigSubTrigFractal;
   }
   return(JulialongSetup());
}

TrigPlusTrigfpSetup()
{
   FnPlusFnSym();
   if(trigndx[1] == SQR)
      return(TrigPlusSqrfpSetup());
   fractalspecific[fractype].per_pixel =  otherjuliafp_per_pixel;
   fractalspecific[fractype].orbitcalc =  TrigPlusTrigfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)	/* Scott variant */
	 fractalspecific[fractype].orbitcalc =	ScottTrigPlusTrigfpFractal;
      else if(parm2.x == -1.0)	/* Skinner variant */
	 fractalspecific[fractype].orbitcalc =	SkinnerTrigSubTrigfpFractal;
   }
   return(JuliafpSetup());
}

FnPlusFnSym() /* set symmetry matrix for fn+fn type */
{
   static char far fnplusfn[7][7] =  
   {/* fn2 ->sin     cos    sinh    cosh   sqr    exp    log  */    
   /* fn1 */
   /* sin */ {PI_SYM,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cos */ {XAXIS, PI_SYM,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* sinh*/ {XYAXIS,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cosh*/ {XAXIS, XYAXIS,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* sqr */ {XAXIS, XYAXIS,XAXIS,  XAXIS, XYAXIS,XAXIS, XAXIS},
   /* exp */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XAXIS},
   /* log */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XYAXIS}
   };
   if(parm.y == 0.0 && parm2.y == 0.0)
      symmetry = fnplusfn[trigndx[0]][trigndx[1]];
   else
      symmetry = NOSYM;
   return(0);
}

ZXTrigPlusZSetup()
{
   static char far ZXTrigPlusZSym1[] = 
   /* fn1 ->  sin   cos    sinh  cosh   sqr    exp   log  */    
             {XAXIS,XYAXIS,XAXIS,XYAXIS,XYAXIS,XAXIS,XAXIS};
   static char far ZXTrigPlusZSym2[] = 
   /* fn1 ->  sin   cos    sinh  cosh   sqr    exp   log  */    
             {NOSYM,ORIGIN,NOSYM,ORIGIN,XYAXIS,NOSYM,NOSYM};
   if(param[1] == 0.0 && param[3] == 0.0)
      symmetry = ZXTrigPlusZSym1[trigndx[0]];
   else
      symmetry = ZXTrigPlusZSym2[trigndx[0]];

   if(fractalspecific[fractype].isinteger)
   {
      fractalspecific[fractype].orbitcalc =  ZXTrigPlusZFractal;
      if(lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != 90)
      {
         if(lparm2.x == fudge)	   /* Scott variant */
		 fractalspecific[fractype].orbitcalc =	ScottZXTrigPlusZFractal;
         else if(lparm2.x == -fudge)  /* Skinner variant */
		 fractalspecific[fractype].orbitcalc =	SkinnerZXTrigSubZFractal;
      }
      return(JulialongSetup());
   }
   else
   {
      fractalspecific[fractype].orbitcalc =  ZXTrigPlusZfpFractal;
      if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
      {
         if(parm2.x == 1.0)	/* Scott variant */
		 fractalspecific[fractype].orbitcalc =	ScottZXTrigPlusZfpFractal;
         else if(parm2.x == -1.0)	/* Skinner variant */
		 fractalspecific[fractype].orbitcalc =	SkinnerZXTrigSubZfpFractal;
      }
   }
   return(JuliafpSetup());
}

LambdaTrigSetup()
{
   int isinteger;
   if(isinteger = fractalspecific[fractype].isinteger)
      fractalspecific[fractype].orbitcalc =  LambdaTrigFractal;
   else
      fractalspecific[fractype].orbitcalc =  LambdaTrigfpFractal;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
      symmetry = PI_SYM;
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LambdaTrigFractal1;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      symmetry = ORIGIN;
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LambdaTrigFractal2;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaTrigfpFractal2;
      break;
   case SQR:
      symmetry = ORIGIN;
      break;
   case EXP:
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LongLambdaexponentFractal;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaexponentFractal;
      symmetry = XAXIS;
      break;
   case LOG:
      symmetry = NOSYM;
      break;
   }
   if(isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

JuliafnPlusZsqrdSetup()
{
   static char far fnpluszsqrd[] = 
   /* fn1 ->  sin   cos    sinh  cosh   sqr    exp   log  */    
   /* sin */ {NOSYM,ORIGIN,NOSYM,ORIGIN,ORIGIN,NOSYM,NOSYM};

   symmetry = fnpluszsqrd[trigndx[0]];
   if(fractalspecific[fractype].isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

SqrTrigSetup()
{
   static char far SqrTrigSym[] = 
   /* fn1 ->  sin    cos    sinh   cosh   sqr    exp   log  */    
             {PI_SYM,PI_SYM,XYAXIS,XYAXIS,XYAXIS,XAXIS,XAXIS};
   symmetry = SqrTrigSym[trigndx[0]];
   if(fractalspecific[fractype].isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

FnXFnSetup()
{
   static char far fnxfn[7][7] = 
   {/* fn2 ->sin     cos    sinh    cosh   sqr    exp    log  */    
   /* fn1 */
   /* sin */ {PI_SYM,YAXIS, XYAXIS,XYAXIS,XYAXIS,XAXIS, XAXIS},
   /* cos */ {YAXIS, PI_SYM,XYAXIS,XYAXIS,XYAXIS,XAXIS, XAXIS},
   /* sinh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, XAXIS},
   /* cosh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, XAXIS},
   /* sqr */ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, XAXIS},
   /* exp */ {XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* log */ {XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
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
   symmetry = fnxfn[trigndx[0]][trigndx[1]];
   if(fractalspecific[fractype].isinteger)
      return(JulialongSetup());
   else
      return(JuliafpSetup());
}

MandelTrigSetup()
{
   int isinteger;
   if(isinteger = fractalspecific[fractype].isinteger)
      fractalspecific[fractype].orbitcalc =  LambdaTrigFractal;
   else
      fractalspecific[fractype].orbitcalc =  LambdaTrigfpFractal;
   symmetry = XYAXIS_NOPARM;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LambdaTrigFractal1;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LambdaTrigFractal2;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaTrigfpFractal2;
      break;
   case EXP:
      symmetry = XAXIS_NOPARM;
      if(isinteger)
	 fractalspecific[fractype].orbitcalc =	LongLambdaexponentFractal;
      else
	 fractalspecific[fractype].orbitcalc =	LambdaexponentFractal;
      break;
   case LOG:
      symmetry = XAXIS_NOPARM;
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


StandardSetup()
{
   if(fractype==UNITYFP)
      periodicitycheck=0;
   return(1);
}



/* parameter descriptions */
/* for Mandelbrots */
static char realz0[] = "Real Perturbation of Z(0)";
static char imagz0[] = "Imaginary Perturbation of Z(0)";

/* for Julias */
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

/* for Newtons */
static char newtdegree[] = "Polynomial Degree (> 2)";

/* for MarksMandel/Julia */
static char exponent[] = "Parameter Exponent ( > 0)";

/* for Complex Newton */
static char realroot[]	 = "Real part of Root";
static char imagroot[]	 = "Imag part of Root";
static char realdegree[] = "Real part of Degree";
static char imagdegree[] = "Imag part of Degree";

/* for Lorenz */
static char timestep[]	   = "Time Step";

/* for formula */
static char p1real[] = "Real portion of p1";
static char p2real[] = "Real portion of p2";
static char p1imag[] = "Imaginary portion of p1";
static char p2imag[] = "Imaginary portion of p2";

/* trig functions */
static char recoeftrg1[] = "Real Coefficient First Function";
static char imcoeftrg1[] = "Imag Coefficient First Function";
static char recoeftrg2[] = "Real Coefficient Second Function";
static char imcoeftrg2[] = "Imag Coefficient Second Function";
static char recoefsqr[] = "Real Coefficient Square Term";
static char imcoefsqr[] = "Imag Coefficient Square Term";
static char recoef2nd[] = "Real Coefficient Second Term";
static char imcoef2nd[] = "Imag Coefficient Second Term";

/* KAM Torus */
static char kamangle[] = "Angle (radians)";
static char kamstep[] =  "Step size";
static char kamstop[] =  "Stop value";
static char pointsperorbit[] = "Points per orbit";

/* Newtbasin */
static char stripes[] = "Enter non-zero value for stripes";

/* bailout defines */
#define FTRIGBAILOUT 2500.0
#define LTRIGBAILOUT   64.0
#define STDBAILOUT	    4.0
#define NOBAILOUT	0.0

struct fractalspecificstuff far fractalspecific[] =
{
   /*
     fractal name, parameter text strings, parameter values, flags,
     xmin  xmax  ymin  ymax int tojulia   tomandel tofloat  symmetry
   |------|-----|-----|-----|--|--------|---------|--------|---------|
     orbit fnct     per_pixel fnct  per_image fnct  calctype fcnt    bailout
   |---------------|---------------|---------------|----------------|-------|
   */

   "mandel",      realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 1, JULIA,	 NOFRACTAL, MANDELFP, XAXIS_NOPARM,
   JuliaFractal,  mandel_per_pixel,MandelSetup,    calcmand,	    STDBAILOUT,

   "julia",       realparm, imagparm,"","",0.3,0.6,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL, JULIAFP,  ORIGIN,
   JuliaFractal,   julia_per_pixel, JuliaSetup,    calcmand,	    STDBAILOUT,

   "*newtbasin",   newtdegree,"", "","",3,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN,   NOSYM,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "lambda",      realparm, imagparm,"","",0.85,0.6,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP,  NOSYM,
   LambdaFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*mandel",    realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,	 NOFRACTAL, MANDEL,  XAXIS_NOPARM,
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*newton",      newtdegree,stripes, "","",3,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTON,	XAXIS,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "*julia",     realparm, imagparm,"","",0.3,0.6,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP, JULIA,  ORIGIN,
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)","","","",2,0,0,0,
   NOZOOM+NOGUESS+NOTRACE+NORESUME,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	   NULL,   StandaloneSetup,	 plasma,	  NOBAILOUT,

   "*mandelfn",  realz0, imagz0,"","",0,0,0,0,TRIG1,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDATRIGFP,NOFRACTAL, MANDELTRIG, XYAXIS_NOPARM,
   LambdaTrigfpFractal,othermandelfp_per_pixel,MandelTrigSetup,StandardFractal,FTRIGBAILOUT,

   "*manowar",    realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MANOWAR,  XAXIS_NOPARM,
   ManOWarfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "manowar",    realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, MANOWARFP,  XAXIS_NOPARM,
   ManOWarFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "test",        "(testpt Param #1)","(testpt param #2)","(testpt param #3)", "(testpt param #4)",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	  NULL, 	    StandaloneSetup, test,    STDBAILOUT,

  "sierpinski",  "","","","",0,0,0,0, 0,
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP,   NOSYM,
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,

  "barnsleym1",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL, BARNSLEYM1FP,  XYAXIS_NOPARM,
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

  "barnsleyj1",  realparm, imagparm,"","",0.6,1.1,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP,  ORIGIN,
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "barnsleym2",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL, BARNSLEYM2FP,  YAXIS_NOPARM,
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj2",  realparm, imagparm,"","",0.6,1.1,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP,  ORIGIN,
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "sqr(fn)", "","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQRTRIGFP,XYAXIS,
   SqrTrigFractal,   long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(fn)", "","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, SQRTRIG,XYAXIS,
   SqrTrigfpFractal,   otherjuliafp_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "fn+fn", recoeftrg1, imcoeftrg1,recoeftrg2, imcoeftrg2,1,0,1,0,TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIGFP,XAXIS,
   TrigPlusTrigFractal,   long_julia_per_pixel, TrigPlusTriglongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandellambda",realz0, imagz0,"","",0,0,0,0, 0,
   -3.0,  5.0, -3.0,  3.0, 1, LAMBDA,	 NOFRACTAL, MANDELLAMBDAFP,  XAXIS_NOPARM,
   LambdaFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksmandel", realz0, imagz0, exponent,"",0,0,1,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL, NOFRACTAL,  NOSYM,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksjulia", realparm, imagparm, exponent,"",0.1,0.9,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL, NOFRACTAL,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,STDBAILOUT,

   "unity",       "","","","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, UNITYFP,   XYAXIS,
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,NOBAILOUT,

   "mandel4",      realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,	  NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM,
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  STDBAILOUT,

   "julia4",       realparm, imagparm,"","",0.6,0.55,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4, NOFRACTAL, ORIGIN,
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,	 STDBAILOUT,

   "ifs",        "","","","",0,0,0,0, NOGUESS+NOTRACE+NORESUME,
   -8.0,  8.0, -1.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,  NOSYM,
   NULL,	  NULL, 	    StandaloneSetup, ifs,    NOBAILOUT,

   "ifs3d",        "","","","",0,0,0,0, NOGUESS+NOTRACE+NORESUME,
   -11.0,  11.0, -11.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,	  NULL,      StandaloneSetup, ifs3d,	NOBAILOUT,

   "barnsleym3",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL, BARNSLEYM3FP,  XAXIS_NOPARM,
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj3",  realparm, imagparm,"","",0.1,0.36,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP,  XAXIS,
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "fn(z*z)", "","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGSQRFP,XYAXIS,
   TrigZsqrdFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*fn(z*z)", "","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGSQR,XYAXIS,
   TrigZsqrdfpFractal,	 juliafp_per_pixel, JuliafpSetup,  StandardFractal,STDBAILOUT,

   "*bifurcation",     "", "","","",0,0,0,0, NOGUESS+NOTRACE+NOROTATE,
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL, LBIFURCATION, NOSYM,
   BifurcVerhulst, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*fn+fn", recoeftrg1, imcoeftrg1,recoeftrg2, imcoeftrg2,1,0,1,0,TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIG,XAXIS,
   TrigPlusTrigfpFractal, otherjuliafp_per_pixel, TrigPlusTrigfpSetup,  StandardFractal,LTRIGBAILOUT,

   "fn*fn", "","","","",0,0,0,0,TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGXTRIGFP,PI,
   TrigXTrigFractal, long_julia_per_pixel, FnXFnSetup,  StandardFractal,LTRIGBAILOUT,

   "*fn*fn", "","","","",0,0,0,0,TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGXTRIG,PI,
   TrigXTrigfpFractal, otherjuliafp_per_pixel, FnXFnSetup,  StandardFractal,LTRIGBAILOUT,

   "sqr(1/fn)","","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQR1OVERTRIGFP,NOSYM,
   Sqr1overTrigFractal, long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(1/fn)","","","","",0,0,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, SQR1OVERTRIG,NOSYM,
   Sqr1overTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "fn*z+z",recoeftrg1, imcoeftrg1, recoef2nd,imcoef2nd,1,0,1,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 1, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZFP,XAXIS,
   ZXTrigPlusZFractal,julia_per_pixel,ZXTrigPlusZSetup,  StandardFractal,LTRIGBAILOUT,

   "*fn*z+z",recoeftrg1, imcoeftrg2, recoef2nd,imcoef2nd,1,0,1,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZ,XAXIS,
   ZXTrigPlusZfpFractal,   juliafp_per_pixel, ZXTrigPlusZSetup,  StandardFractal,LTRIGBAILOUT,

   "*kamtorus",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150, NOGUESS+NOTRACE+INFCALC,
   -1.0,  1.0, -.75, .75, 0, NOFRACTAL, NOFRACTAL, KAM, NOSYM,
   kamtorusfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "kamtorus",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150, NOGUESS+NOTRACE+INFCALC,
   -1.0,  1.0, -.75, .75,16, NOFRACTAL, NOFRACTAL, KAMFP,	NOSYM,
   kamtoruslongorbit, NULL,	     orbit3dlongsetup, orbit2dlong,	 NOBAILOUT,

   "*kamtorus3d",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150, NOGUESS+NOTRACE+INFCALC,
   -3.0,  3.0, -1, 3.5, 0, NOFRACTAL, NOFRACTAL, KAM3D,       NOSYM,
   kamtorusfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "kamtorus3d",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150, NOGUESS+NOTRACE+INFCALC,
   -3.0,  3.0, -1, 3.5,16, NOFRACTAL, NOFRACTAL, KAM3DFP,     NOSYM,
   kamtoruslongorbit, NULL,	     orbit3dlongsetup, orbit3dlong,	 NOBAILOUT,

   "lambdafn",      realparm, imagparm,"","",1.0,0.4,0,0,TRIG1,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, MANDELTRIG, LAMBDATRIGFP,PI_SYM,
   LambdaTrigFractal,long_julia_per_pixel, LambdaTrigSetup,	StandardFractal,LTRIGBAILOUT,

   "manfn+zsqrd",      realz0, imagz0,"","",0,0,0,0, TRIG1,
   -2.5,  1.5, -1.5,  1.5, 16, LJULTRIGPLUSZSQRD,  NOFRACTAL, FPMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredFractal,mandel_per_pixel,MandellongSetup,StandardFractal, STDBAILOUT,

   "julfn+zsqrd",       realparm, imagparm,"","",-0.5,0.5,0,0, TRIG1,
   -2.0,  2.0, -1.5,  1.5, 16, NOFRACTAL, LMANTRIGPLUSZSQRD, FPJULTRIGPLUSZSQRD,	NOSYM,
   TrigPlusZsquaredFractal,julia_per_pixel, JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*manfn+zsqrd",    realz0, imagz0,"","",0,0,0,0, TRIG1,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULTRIGPLUSZSQRD,   NOFRACTAL, LMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal, STDBAILOUT,

   "*julfn+zsqrd",     realparm, imagparm,"","",-0.5,0.5,0,0, TRIG1,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANTRIGPLUSZSQRD, LJULTRIGPLUSZSQRD,	NOSYM,
   TrigPlusZsquaredfpFractal, juliafp_per_pixel,  JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*lambdafn",  realparm, imagparm,"","",1.0,0.4,0,0,TRIG1,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELTRIGFP, LAMBDATRIG, PI_SYM,
   LambdaTrigfpFractal,otherjuliafp_per_pixel,LambdaTrigSetup,StandardFractal,FTRIGBAILOUT,

   "mandelfn",realz0, imagz0,"","",0,0,0,0,TRIG1,
   -8.0,  8.0, -6.0,  6.0, 16, LAMBDATRIG, NOFRACTAL, MANDELTRIGFP, XYAXIS_NOPARM,
   LambdaTrigFractal,long_mandel_per_pixel,MandelTrigSetup,StandardFractal,LTRIGBAILOUT,

   "manzpower", realz0, imagz0, exponent,"",0,0,2,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER,	XAXIS,
   longZpowerFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julzpower", realparm, imagparm, exponent,"",0.3,0.6,2,0, 0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER,	 ORIGIN,
   longZpowerFractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "*manzpower",    realz0, imagz0, exponent,"",0,0,2,0, 0,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULIAZPOWER,   NOFRACTAL, LMANDELZPOWER,  XAXIS_NOPARM,
   floatZpowerFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julzpower",     realparm, imagparm, exponent,"",0.3,0.6,2,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER,	ORIGIN,
   floatZpowerFractal, otherjuliafp_per_pixel,	JuliafpSetup,StandardFractal,STDBAILOUT,

   "manzzpwr",    realz0, imagz0, exponent,"",0,0,2,0, 0,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULZTOZPLUSZPWR,   NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM,
   floatZtozPluszpwrFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "julzzpwr",     realparm, imagparm, exponent,"",-0.3,0.3,2,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL,	NOSYM,
   floatZtozPluszpwrFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "manfn+exp",realz0, imagz0,"","",0,0,0,0, TRIG1,
   -8.0,  8.0, -6.0,  6.0, 16, LJULTRIGPLUSEXP,    NOFRACTAL,  FPMANTRIGPLUSEXP, XAXIS_NOPARM,
   LongTrigPlusExponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julfn+exp",      realparm, imagparm,"","",0,0,0,0, TRIG1,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANTRIGPLUSEXP,FPJULTRIGPLUSEXP, NOSYM,
   LongTrigPlusExponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*manfn+exp",   realz0, imagz0,"","",0,0,0,0, TRIG1,
   -8.0,  8.0, -6.0,  6.0, 0, FPJULTRIGPLUSEXP, NOFRACTAL, LMANTRIGPLUSEXP,   XAXIS_NOPARM,
   FloatTrigPlusExponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julfn+exp",   realparm, imagparm,"","",0,0,0,0, TRIG1,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANTRIGPLUSEXP, LJULTRIGPLUSEXP,   NOSYM,
   FloatTrigPlusExponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*popcorn", "", "", "","",0,0,0,0, NOGUESS+NOTRACE,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORN,  NOPLOT,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,  popcorn,STDBAILOUT,

   "popcorn", "", "", "","",0,0,0,0, NOGUESS+NOTRACE,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORN,  NOPLOT,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,popcorn,STDBAILOUT,

   "*lorenz",timestep,"a","b", "c",.02,5,15,1, NOGUESS+NOTRACE+INFCALC,
   -15,  15, 0, 30, 0, NOFRACTAL, NOFRACTAL, LLORENZ,	NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "lorenz",timestep,"a","b", "c",.02,5,15,1, NOGUESS+NOTRACE+INFCALC,
   -15,  15, 0, 30, 16, NOFRACTAL, NOFRACTAL, FPLORENZ,   NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "lorenz3d",timestep,"a","b", "c",.02,5,15,1, NOGUESS+NOTRACE+NORESUME,
   -30.0,  30.0,  -30.0,   30.0, 16, NOFRACTAL, NOFRACTAL, FPLORENZ3D,	 NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "newton",      newtdegree,"", "","",3,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTON,   XAXIS,
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "newtbasin",      newtdegree,stripes, "","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTBASIN,	 NOSYM,
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "complexnewton", realdegree, imagdegree, realroot, imagroot,3,0,1,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   ComplexNewton, otherjuliafp_per_pixel,  ComplexNewtonSetup, StandardFractal,NOBAILOUT,

   "complexbasin", realdegree, imagdegree, realroot, imagroot,3,0,1,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   ComplexBasin, otherjuliafp_per_pixel,  ComplexNewtonSetup,  StandardFractal, NOBAILOUT,

   "cmplxmarksmand", realz0, imagz0, realdegree, imagdegree,0,0,1,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, COMPLEXMARKSJUL, NOFRACTAL, NOFRACTAL,   NOSYM,
   MarksCplxMand, MarksCplxMandperp, MandelfpSetup, StandardFractal, STDBAILOUT,

   "cmplxmarksjul", realparm, imagparm, realdegree, imagdegree,0.3,0.6,1,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, COMPLEXMARKSMAND, NOFRACTAL,	NOSYM,
   MarksCplxMand, juliafp_per_pixel, JuliafpSetup, StandardFractal, STDBAILOUT,

   "formula", p1real, p1imag, p2real, p2imag, 0,0,0,0, 0,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, FFORMULA, SETUP_SYM,
   Formula, form_per_pixel, intFormulaSetup, StandardFractal, 0,

   "*formula", p1real, p1imag, p2real, p2imag, 0,0,0,0, 0,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, FORMULA, SETUP_SYM,
   Formula, form_per_pixel, fpFormulaSetup, StandardFractal, 0,

  "*sierpinski",  "","","","",0,0,0,0, 0,
   -0.9,  1.7, -0.9,  1.7, 0, NOFRACTAL, NOFRACTAL, SIERPINSKI,   NOSYM,
   SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,StandardFractal,127.0,

   "*lambda",      realparm, imagparm,"","",0.85,0.6,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA,  NOSYM,
   LambdaFPFractal,   juliafp_per_pixel, JuliafpSetup,  StandardFractal,STDBAILOUT,

  "*barnsleym1",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ1FP,NOFRACTAL, BARNSLEYM1,  XYAXIS_NOPARM,
   Barnsley1FPFractal, othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

  "*barnsleyj1",  realparm, imagparm,"","",0.6,1.1,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1,  ORIGIN,
   Barnsley1FPFractal, otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym2",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ2FP,NOFRACTAL, BARNSLEYM2,  YAXIS_NOPARM,
   Barnsley2FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj2",  realparm, imagparm,"","",0.6,1.1,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2,  ORIGIN,
   Barnsley2FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym3",  realz0, imagz0,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3,  XAXIS_NOPARM,
   Barnsley3FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj3",  realparm, imagparm,"","",0.6,1.1,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3,  XAXIS,
   Barnsley3FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*mandellambda",realz0, imagz0,"","",0,0,0,0, 0,
   -3.0,  5.0, -3.0,  3.0, 0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA,  XAXIS_NOPARM,
   LambdaFPFractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "julibrot", "","","","", -.83, -.83, .25, -.25, NOGUESS+NOTRACE+NOROTATE+NORESUME,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal, STDBAILOUT,

   "*lorenz3d",timestep,"a","b", "c",.02,5,15,1, NOGUESS+NOTRACE+NORESUME,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, LLORENZ3D,   NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "rossler3d",timestep,"a","b", "c",.04, .2, .2, 5.7, NOGUESS+NOTRACE+NORESUME,
   -30.0,  30.0,  -20.0,   40.0, 16, NOFRACTAL, NOFRACTAL, FPROSSLER,	NOSYM,
   rosslerlongorbit, NULL,	   orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "*rossler3d",timestep,"a","b", "c",.04, .2, .2, 5.7, NOGUESS+NOTRACE+NORESUME,
   -30.0,  30.0,  -20.0,   40.0, 0, NOFRACTAL, NOFRACTAL, LROSSLER,   NOSYM,
   rosslerfloatorbit, NULL,	    orbit3dfloatsetup, orbit3dfloat,	NOBAILOUT,

   "henon","a","b","","",1.4,.3,0,0, NOGUESS+NOTRACE+INFCALC,
   -1.4,  1.4,	-.5,   .5, 16, NOFRACTAL, NOFRACTAL, FPHENON,	NOSYM,
   henonlongorbit, NULL,	 orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "*henon","a","b","","",1.4,.3,0,0, NOGUESS+NOTRACE+INFCALC,
   -1.4,  1.4,	-.5,   .5, 0, NOFRACTAL, NOFRACTAL, LHENON,   NOSYM,
   henonfloatorbit, NULL,	  orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "pickover","a","b","c","d",2.24,.43,-.65, -2.43, NOGUESS+NOTRACE+NORESUME,
   -2.8,  2.8,	-2.0, 2.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   pickoverfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "gingerbreadman","","","","",0,0,0,0, NOGUESS+NOTRACE+INFCALC,
   -4.5,  8.5,	-4.5, 8.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   gingerbreadfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "diffusion",        "Border size","","", "",10,0,0,0,
   NOZOOM+NOGUESS+NOTRACE+NORESUME,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,   NULL,     StandaloneSetup, diffusion,    NOBAILOUT,

   "*unity",       "","","","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, UNITY,   XYAXIS,
   UnityfpFractal, otherjuliafp_per_pixel,StandardSetup,StandardFractal,NOBAILOUT,

   "*spider",    realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, SPIDERFP+1,  XAXIS_NOPARM,
   SpiderfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "spider",    realz0, imagz0,"","",0,0,0,0, 0,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, SPIDERFP,  XAXIS_NOPARM,
   SpiderFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "tetrate",  realparm, imagparm,"","",0,0,0,0, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	XAXIS,
   TetratefpFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "magnet1m", realz0, imagz0,"","",0,0,0,0,0,
   -4.0, 4.0, -3.0, 3.0, 0, MAGNET1J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet1Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet1j", realz0, imagz0,"","",0,0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET1M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet1Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "magnet2m", realz0, imagz0,"","",0,0,0,0,0,
   -1.5,3.7, -1.95,1.95,   0, MAGNET2J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet2Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet2j", realz0, imagz0,"","",0,0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET2M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet2Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "bifurcation",     "", "","","",0,0,0,0,NOGUESS+NOTRACE+NOROTATE,
   1.9,  3.0, 0,  1.34, 1, NOFRACTAL, NOFRACTAL, BIFURCATION, NOSYM,
   LongBifurcVerhulst, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "biflambda",     "", "","","",0,0,0,0,NOGUESS+NOTRACE+NOROTATE,
   -2.0, 4.0, -1.0, 2.0, 1, NOFRACTAL, NOFRACTAL, BIFLAMBDA,   NOSYM,
   LongBifurcLambda, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*biflambda",     "", "","","",0,0,0,0,NOGUESS+NOTRACE+NOROTATE,
   -2.0, 4.0, -1.0, 2.0, 0, NOFRACTAL, NOFRACTAL, LBIFLAMBDA,  NOSYM,
   BifurcLambda, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bif+sinpi",    "", "","","",0,0,0,0,NOGUESS+NOTRACE+NOROTATE,
   0.275,1.45, 0.0, 2.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   BifurcAddSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bif=sinpi",     "", "","","",0,0,0,0,NOGUESS+NOTRACE+NOROTATE,
   -2.5, 2.5, -3.5, 3.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   BifurcSetSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*popcornjul", "", "", "","",0,0,0,0, 0,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORNJUL,  ORIGIN,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "popcornjul", "", "", "","",0,0,0,0, 0,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORNJUL,  ORIGIN,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,


   NULL, NULL, NULL, NULL, NULL,0,0,0,0, 0, /* marks the END of the list */
   0,  0, 0,  0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL, NULL, NULL, NULL,0
};
