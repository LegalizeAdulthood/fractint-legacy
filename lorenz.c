/*
   This file contains two 3 dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images. Tim Wegner
*/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include "mpmath.h"
#include "fractint.h"
#include "fractype.h"
#include "prototyp.h"

#define RANDOM(x)  (rand()%(x))

/* BAD_PIXEL is used to cutoff orbits that are diverging. It might be better
to test the actual floating point prbit values, but this seems safe for now.
A higher value cannot be used - to test, turn off math coprocessor and
use +2.24 for type ICONS. If BAD_PIXEL is set to 20000, this will abort
Fractint with a math error. Note that this approach precludes zooming in very
far to an orbit type. */

#define BAD_PIXEL  10000L    /* pixels can't get this big */

struct affine
{
   /* weird order so a,b,e and c,d,f are vectors */
   double a;
   double b;
   double e;
   double c;
   double d;
   double f;
};
struct l_affine
{
   /* weird order so a,b,e and c,d,f are vectors */
   long a;
   long b;
   long e;
   long c;
   long d;
   long f;
};
struct long3dvtinf /* data used by 3d view transform subroutine */
{
   long ct;		/* iteration counter */
   long orbit[3];	/* interated function orbit value */
   long iview[3];	/* perspective viewer's coordinates */
   long viewvect[3];	/* orbit transformed for viewing */
   long viewvect1[3];	/* orbit transformed for viewing */
   long maxvals[3];
   long minvals[3];
   MATRIX doublemat;	/* transformation matrix */
   MATRIX doublemat1;	/* transformation matrix */
   long longmat[4][4];	/* long version of matrix */
   long longmat1[4][4]; /* long version of matrix */
   int row,col; 	/* results */
   int row1,col1;
   struct l_affine cvt;
};
struct float3dvtinf /* data used by 3d view transform subroutine */
{
   long ct;		/* iteration counter */
   double orbit[3];		   /* interated function orbit value */
   double viewvect[3];	      /* orbit transformed for viewing */
   double viewvect1[3];        /* orbit transformed for viewing */
   double maxvals[3];
   double minvals[3];
   MATRIX doublemat;	/* transformation matrix */
   MATRIX doublemat1;	/* transformation matrix */
   int row,col; 	/* results */
   int row1,col1;
   struct affine cvt;
};

/* Routines in this module	*/

static int  ifs3dlong(void);
static int  ifs3dfloat(void);
static double determinant(double mat[3][3]);
static int  solve3x3(double mat[3][3],double vec[3],double ans[3]);
extern int  setup_convert_to_screen(struct affine *);
static int  l_setup_convert_to_screen(struct l_affine *);
static void setupmatrix(MATRIX);
static int  long3dviewtransf(struct long3dvtinf *inf);
static int  float3dviewtransf(struct float3dvtinf *inf);
static FILE *open_orbitsave();
static void _fastcall plothist(int x, int y, int color);
extern char far insufficient_ifs_mem[];
extern int numaffine;
static int realtime;
extern int orbitsave;
static char orbitsavename[]    = {"orbits.raw"};
static char orbitsave_format[] = {"%g %g %g 15\n"};
extern int active_system;
extern int overflow;
extern int soundflag;
extern int basehertz;
extern int fractype;
extern int glassestype;
extern int whichimage;
extern int init3d[];
extern char floatflag;
extern VECTOR view;
extern int xxadjust, yyadjust;
extern int xxadjust1, yyadjust1;
extern int xshift,yshift;
extern int xshift1,yshift1;
extern int	debugflag;			/* for debugging purposes */
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	maxit;				/* try this many iterations */
extern double param[];
extern double	xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* selected screen corners  */
extern	int	diskvideo;			/* for disk-video klooges */
extern int	bitshift;			/* bit shift for fudge */
extern long	fudge;				/* fudge factor (2**n) */
extern int	colors; 			/* maximum colors available */
extern int display3d;
extern double dxsize,dysize,delxx,delxx2,delyy,delyy2;

extern float far *ifs_defn;
extern int ifs_type;

static int t;
static long l_dx,l_dy,l_dz,l_dt,l_a,l_b,l_c,l_d;
static long l_adt,l_bdt,l_cdt,l_xdt,l_ydt;
static long l_initx,l_inity,l_initz;
static long initorbitlong[3];

static double dx,dy,dz,dt,a,b,c,d;
static double adt,bdt,cdt,xdt,ydt,zdt;
static double initx,inity,initz;
static double initorbit[3];
extern int inside;
extern int outside;
extern int orbit_delay;

extern void (_fastcall * standardplot)(int,int,int);
extern int  calc_status, resuming;
extern int  diskisactive;
extern char savename[];

/*
 * The following declarations used for Inverse Julia.  MVS
 */

extern int    Init_Queue      (unsigned long);
extern void   Free_Queue      (void);
extern void   ClearQueue      (void);
extern int    QueueEmpty      (void);
extern int    QueueFull       (void);
extern int    QueueFullAlmost (void);
extern int    PushLong        (long ,  long);
extern int    PushFloat       (float,  float);
extern int    EnQueueLong     (long ,  long);
extern int    EnQueueFloat    (float,  float);
extern LCMPLX PopLong         (void);
extern _CMPLX PopFloat        (void);
extern LCMPLX DeQueueLong     (void);
extern _CMPLX DeQueueFloat    (void);
extern LCMPLX ComplexSqrtLong (long ,  long);
extern _CMPLX ComplexSqrtFloat(double, double);
static char far NoQueue[] =
  "Not enough memory: switching to random walk.\n";

static int    maxhits;
int    run_length;
enum   {breadth_first, depth_first, random_walk, random_run} major_method;
enum   {left_first, right_first}                             minor_method;
struct affine cvt;
struct l_affine lcvt;

extern _CMPLX  old,  new;
extern LCMPLX lold, lnew;

double Cx, Cy;
long   CxLong, CyLong;

/*
 * end of Inverse Julia declarations;
 */

/* these are potential user parameters */
int connect = 1;    /* flag to connect points with a line */
int euler = 0;	    /* use implicit euler approximation for dynamic system */
int waste = 100;    /* waste this many points before plotting */
int projection = 2; /* projection plane - default is to plot x-y */

/******************************************************************/
/*		   zoom box conversion functions		  */
/******************************************************************/

static double determinant(mat) /* determinant of 3x3 matrix */
double mat[3][3];
{
   /* calculate determinant of 3x3 matrix */
   return(mat[0][0]*mat[1][1]*mat[2][2] +
	  mat[0][2]*mat[1][0]*mat[2][1] +
	  mat[0][1]*mat[1][2]*mat[2][0] -
	  mat[2][0]*mat[1][1]*mat[0][2] -
	  mat[1][0]*mat[0][1]*mat[2][2] -
	  mat[0][0]*mat[1][2]*mat[2][1]);

}

static int solve3x3(mat,vec,ans) /* solve 3x3 inhomogeneous linear equations */
double mat[3][3], vec[3], ans[3];
{
   /* solve 3x3 linear equation [mat][ans] = [vec] */
   double denom;
   double tmp[3][3];
   int i;
   denom = determinant(mat);
   if(fabs(denom) < DBL_EPSILON) /* test if can solve */
     return(-1);
   memcpy(tmp,mat,sizeof(double)*9);
   for(i=0;i<3;i++)
   {
      tmp[0][i] = vec[0];
      tmp[1][i] = vec[1];
      tmp[2][i] = vec[2];
      ans[i]  =  determinant(tmp)/denom;
      tmp[0][i] = mat[0][i];
      tmp[1][i] = mat[1][i];
      tmp[2][i] = mat[2][i];
    }
    return(0);
}


/* Conversion of complex plane to screen coordinates for rotating zoom box.
   Assume there is an affine transformation mapping complex zoom parallelogram
   to rectangular screen. We know this map must map parallelogram corners to
   screen corners, so we have following equations:

      a*xxmin+b*yymax+e == 0	    (upper left)
      c*xxmin+d*yymax+f == 0

      a*xx3rd+b*yy3rd+e == 0	    (lower left)
      c*xx3rd+d*yy3rd+f == ydots-1

      a*xxmax+b*yymin+e == xdots-1  (lower right)
      c*xxmax+d*yymin+f == ydots-1

      First we must solve for a,b,c,d,e,f - (which we do once per image),
      then we just apply the transformation to each orbit value.
*/

int setup_convert_to_screen(struct affine *scrn_cnvt)
{
   /* we do this twice - rather than having six equations with six unknowns,
      everything partitions to two sets of three equations with three
      unknowns. Nice, because who wants to calculate a 6x6 determinant??
    */
   double mat[3][3];
   double vec[3];
   /*
      first these equations - solve for a,b,e
      a*xxmin+b*yymax+e == 0	    (upper left)
      a*xx3rd+b*yy3rd+e == 0	    (lower left)
      a*xxmax+b*yymin+e == xdots-1  (lower right)
   */
   mat[0][0] = xxmin;
   mat[0][1] = yymax;
   mat[0][2] = 1.0;
   mat[1][0] = xx3rd;
   mat[1][1] = yy3rd;
   mat[1][2] = 1.0;
   mat[2][0] = xxmax;
   mat[2][1] = yymin;
   mat[2][2] = 1.0;
   vec[0]    = 0.0;
   vec[1]    = 0.0;
   vec[2]    = (double)(xdots-1);

   if(solve3x3(mat,vec, &(scrn_cnvt->a)))
      return(-1);
   /*
      now solve these:
      c*xxmin+d*yymax+f == 0
      c*xx3rd+d*yy3rd+f == ydots-1
      c*xxmax+d*yymin+f == ydots-1
      (mat[][] has not changed - only vec[])
   */
   vec[0]    = 0.0;
   vec[1]    = (double)(ydots-1);
   vec[2]    = (double)(ydots-1);

   if(solve3x3(mat,vec, &scrn_cnvt->c))
      return(-1);
   return(0);
}

static int l_setup_convert_to_screen(struct l_affine *l_cvt)
{
   struct affine cvt;

   /* MCP 7-7-91, This function should return a something! */
   if(setup_convert_to_screen(&cvt))
      return(-1);
   l_cvt->a = cvt.a*fudge; l_cvt->b = cvt.b*fudge; l_cvt->c = cvt.c*fudge;
   l_cvt->d = cvt.d*fudge; l_cvt->e = cvt.e*fudge; l_cvt->f = cvt.f*fudge;

   /* MCP 7-7-91 */
   return(0);
}


/******************************************************************/
/*   setup functions - put in fractalspecific[fractype].per_image */
/******************************************************************/

static double orbit;
static long   l_orbit;

extern double sinx,cosx;
static long l_sinx,l_cosx;

int orbit3dlongsetup()
{
   connect = 1;
   waste = 100;
   projection = 2;
   if (fractype==LHENON || fractype==KAM || fractype==KAM3D || 
       fractype==INVERSEJULIA)
      connect=0;
   if(fractype==LROSSLER)
      waste = 500;
   if(fractype==LLORENZ)
      projection = 1;

   initorbitlong[0] = fudge;  /* initial conditions */
   initorbitlong[1] = fudge;
   initorbitlong[2] = fudge;

   if(fractype==LHENON)
   {
      l_a =  param[0]*fudge;
      l_b =  param[1]*fudge;
      l_c =  param[2]*fudge;
      l_d =  param[3]*fudge;
   }
   else if(fractype==KAM || fractype==KAM3D)
   {
      a   = param[0];		/* angle */
      if(param[1] <= 0.0)
	 param[1] = .01;
      l_b =  param[1]*fudge;	/* stepsize */
      l_c =  param[2]*fudge;	/* stop */
      t = l_d =  param[3];     /* points per orbit */

      l_sinx = sin(a)*fudge;
      l_cosx = cos(a)*fudge;
      l_orbit = 0;
      initorbitlong[0] = initorbitlong[1] = initorbitlong[2] = 0;
   }
   else if (fractype == INVERSEJULIA)
   {
      LCMPLX Sqrt;

      CxLong = param[0] * fudge;
      CyLong = param[1] * fudge;

      maxhits    = (int) param[2];
      run_length = (int) param[3];
      if (maxhits == 0)
	  maxhits = 1;
      else if (maxhits >= colors)
	  maxhits = colors - 1;

      setup_convert_to_screen(&cvt);
      /* Note: using bitshift of 21 for affine, 24 otherwise */

      lcvt.a = cvt.a * (1L << 21);
      lcvt.b = cvt.b * (1L << 21);
      lcvt.c = cvt.c * (1L << 21);
      lcvt.d = cvt.d * (1L << 21);
      lcvt.e = cvt.e * (1L << 21);
      lcvt.f = cvt.f * (1L << 21);

      Sqrt = ComplexSqrtLong(fudge - 4 * CxLong, -4 * CyLong);

      switch (major_method) {
	 case breadth_first:
	    if (Init_Queue((long)32*1024) == 0)
	    { /* can't get queue memory: fall back to random walk */
	       stopmsg(20, NoQueue);
	       major_method = random_walk;
	       goto lrwalk;
	    }
	    EnQueueLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
	    EnQueueLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
	    break;
	 case depth_first:
	    if (Init_Queue((long)32*1024) == 0)
	    { /* can't get queue memory: fall back to random walk */
	       stopmsg(20, NoQueue);
	       major_method = random_walk;
	       goto lrwalk;
	    }
	    switch (minor_method) {
	       case left_first:
		  PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
		  PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
		  break;
	       case right_first:
		  PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
		  PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
		  break;
	    }
	    break;
	 case random_walk:
lrwalk:
	    lnew.x = initorbitlong[0] = fudge + Sqrt.x / 2;
	    lnew.y = initorbitlong[1] =         Sqrt.y / 2;
	    break;
	 case random_run:
	    lnew.x = initorbitlong[0] = fudge + Sqrt.x / 2;
	    lnew.y = initorbitlong[1] =         Sqrt.y / 2;
	    break;
      }
   }
   else
   {
      l_dt = param[0]*fudge;
      l_a =  param[1]*fudge;
      l_b =  param[2]*fudge;
      l_c =  param[3]*fudge;
   }

   /* precalculations for speed */
   l_adt = multiply(l_a,l_dt,bitshift);
   l_bdt = multiply(l_b,l_dt,bitshift);
   l_cdt = multiply(l_c,l_dt,bitshift);
   return(1);
}

int orbit3dfloatsetup()
{
   connect = 1;
   waste = 100;
   projection = 2;

   if(fractype==FPHENON || fractype==FPPICKOVER || fractype==FPGINGERBREAD
	    || fractype == KAMFP || fractype == KAM3DFP
	    || fractype == FPHOPALONG || fractype == INVERSEJULIAFP)
      connect=0;
   if(fractype==FPLORENZ3D1 || fractype==FPLORENZ3D3 ||
      fractype==FPLORENZ3D4)
      waste = 750;
   if(fractype==FPROSSLER)
      waste = 500;
   if(fractype==FPLORENZ)
      projection = 1; /* plot x and z */

   initorbit[0] = 1;  /* initial conditions */
   initorbit[1] = 1;
   initorbit[2] = 1;
   if(fractype==FPGINGERBREAD)
   {
      initorbit[0] = param[0];	/* initial conditions */
      initorbit[1] = param[1];
   }

   if(fractype==ICON || fractype==ICON3D)        /* DMF */
   {
      initorbit[0] = 0.01;  /* initial conditions */
      initorbit[1] = 0.003;
      connect = 0;
      waste = 2000;
   }

   if(fractype==FPHENON || fractype==FPPICKOVER)
   {
      a =  param[0];
      b =  param[1];
      c =  param[2];
      d =  param[3];
   }
   else if(fractype==ICON || fractype==ICON3D)        /* DMF */
   {
      initorbit[0] = 0.01;  /* initial conditions */
      initorbit[1] = 0.003;
      connect = 0;
      waste = 2000;
      /* Initialize parameters */
      a  =   param[0];
      b  =   param[1];
      c  =   param[2];
      d  =   param[3];
   }
   else if(fractype==KAMFP || fractype==KAM3DFP)
   {
      a = param[0];	      /* angle */
      if(param[1] <= 0.0)
	 param[1] = .01;
      b =  param[1];	/* stepsize */
      c =  param[2];	/* stop */
      t = l_d =  param[3];     /* points per orbit */
      sinx = sin(a);
      cosx = cos(a);
      orbit = 0;
      initorbit[0] = initorbit[1] = initorbit[2] = 0;
   }
   else if(fractype==FPHOPALONG || fractype==FPMARTIN)
   {
      initorbit[0] = 0;  /* initial conditions */
      initorbit[1] = 0;
      initorbit[2] = 0;
      connect = 0;
      a =  param[0];
      b =  param[1];
      c =  param[2];
      d =  param[3];
   }
   else if (fractype == INVERSEJULIAFP)
   {
      _CMPLX Sqrt;

      Cx = param[0];
      Cy = param[1];

      maxhits    = (int) param[2];
      run_length = (int) param[3];
      if (maxhits == 0)
	  maxhits = 1;
      else if (maxhits >= colors)
	  maxhits = colors - 1;

      setup_convert_to_screen(&cvt);

      /* find fixed points: guaranteed to be in the set */
      Sqrt = ComplexSqrtFloat(1 - 4 * Cx, -4 * Cy);
      switch ((int) major_method) {
	 case breadth_first:
	    if (Init_Queue((long)32*1024) == 0)
	    { /* can't get queue memory: fall back to random walk */
	       stopmsg(20, NoQueue);
	       major_method = random_walk;
	       goto rwalk;
	    }
	    EnQueueFloat((1 + Sqrt.x) / 2,  Sqrt.y / 2);
	    EnQueueFloat((1 - Sqrt.x) / 2, -Sqrt.y / 2);
	    break;
	 case depth_first:			/* depth first (choose direction) */
	    if (Init_Queue((long)32*1024) == 0)
	    { /* can't get queue memory: fall back to random walk */
	       stopmsg(20, NoQueue);
	       major_method = random_walk;
	       goto rwalk;
	    }
	    switch (minor_method) {
	       case left_first:
		  PushFloat((1 + Sqrt.x) / 2,  Sqrt.y / 2);
		  PushFloat((1 - Sqrt.x) / 2, -Sqrt.y / 2);
		  break;
	       case right_first:
		  PushFloat((1 - Sqrt.x) / 2, -Sqrt.y / 2);
		  PushFloat((1 + Sqrt.x) / 2,  Sqrt.y / 2);
		  break;
	    }
	    break;
	 case random_walk:
rwalk:
	    new.x = initorbit[0] = 1 + Sqrt.x / 2;
	    new.y = initorbit[1] = Sqrt.y / 2;
	    break;
	 case random_run:	/* random run, choose intervals */
	    major_method = random_run;
	    new.x = initorbit[0] = 1 + Sqrt.x / 2;
	    new.y = initorbit[1] = Sqrt.y / 2;
	    break;
      }
   }
   else
   {
      dt = param[0];
      a =  param[1];
      b =  param[2];
      c =  param[3];

   }

   /* precalculations for speed */
   adt = a*dt;
   bdt = b*dt;
   cdt = c*dt;

   return(1);
}

/******************************************************************/
/*   orbit functions - put in fractalspecific[fractype].orbitcalc */
/******************************************************************/

/* Julia sets by inverse iterations added by Juan J. Buhler 4/3/92 */
/* Integrated with Lorenz by Tim Wegner 7/20/92 */
/* Add Modified Inverse Iteration Method, 11/92 by Michael Snyder  */

int
Minverse_julia_orbit()
{
   static int   random_dir = 0, random_len = 0;
   int    newrow, newcol;
   int    color,  leftright;

   /*
    * First, compute new point
    */
   switch (major_method) {
      case breadth_first:
	 if (QueueEmpty())
	    return -1;
	 new = DeQueueFloat();
	 break;
      case depth_first:
	 if (QueueEmpty())
	    return -1;
	 new = PopFloat();
	 break;
      case random_walk:
#if 0
	 new = ComplexSqrtFloat(new.x - Cx, new.y - Cy);
	 if (RANDOM(2))
	 {
	    new.x = -new.x;
	    new.y = -new.y;
	 }
#endif
	 break;
      case random_run:
#if 0
	 new = ComplexSqrtFloat(new.x - Cx, new.y - Cy);
	 if (random_len == 0)
	 {
	    random_len = RANDOM(run_length);
	    random_dir = RANDOM(3);
	 }
	 switch (random_dir) {
	    case 0:	/* left */
	       break;
	    case 1:	/* right */
	       new.x = -new.x;
	       new.y = -new.y;
	       break;
	    case 2:	/* random direction */
	       if (RANDOM(2))
	       {
		  new.x = -new.x;
		  new.y = -new.y;
	       }
	       break;
	 }
#endif
	 break;
   }

   /*
    * Next, find its pixel position
    */
   newcol = cvt.a * new.x + cvt.b * new.y + cvt.e;
   newrow = cvt.c * new.x + cvt.d * new.y + cvt.f;

   /*
    * Now find the next point(s), and flip a coin to choose one.
    */

   new       = ComplexSqrtFloat(new.x - Cx, new.y - Cy);
   leftright = (RANDOM(2)) ? 1 : -1;

   if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
   {
      /*
       * MIIM must skip points that are off the screen boundary,
       * since it cannot read their color.
       */
      switch (major_method) {
	 case breadth_first:
	    EnQueueFloat(leftright * new.x, leftright * new.y);
	    return 1;
	 case depth_first:
	    PushFloat   (leftright * new.x, leftright * new.y);
	    return 1;
	 case random_run:
	 case random_walk:
	    break;
      }
   }

   /*
    * Read the pixel's color:
    * For MIIM, if color >= maxhits, discard the point
    *           else put the point's children onto the queue
    */
   color  = getcolor(newcol, newrow);
   switch (major_method) {
      case breadth_first:
	 if (color < maxhits)
	 {
	    putcolor(newcol, newrow, color+1);
/*	    new = ComplexSqrtFloat(new.x - Cx, new.y - Cy); */
	    EnQueueFloat( new.x,  new.y);
	    EnQueueFloat(-new.x, -new.y);
	 }
	 break;
      case depth_first:
	 if (color < maxhits)
	 {
	    putcolor(newcol, newrow, color+1);
/*	    new = ComplexSqrtFloat(new.x - Cx, new.y - Cy); */
	    if (minor_method == left_first)
	    {
	       if (QueueFullAlmost())
		  PushFloat(-new.x, -new.y);
	       else
	       {
		  PushFloat( new.x,  new.y);
		  PushFloat(-new.x, -new.y);
	       }
	    }
	    else
	    {
	       if (QueueFullAlmost())
		  PushFloat( new.x,  new.y);
	       else
	       {
		  PushFloat(-new.x, -new.y);
		  PushFloat( new.x,  new.y);
	       }
	    }
	 }
	 break;
      case random_run:
	 if (random_len-- == 0)
	 {
	    random_len = RANDOM(run_length);
	    random_dir = RANDOM(3);
	 }
	 switch (random_dir) {
	    case 0:	/* left */
	       break;
	    case 1:	/* right */
	       new.x = -new.x;
	       new.y = -new.y;
	       break;
	    case 2:	/* random direction */
	       new.x = leftright * new.x;
	       new.y = leftright * new.y;
	       break;
	 }
	 if (color < colors-1)
	    putcolor(newcol, newrow, color+1);
	 break;
      case random_walk:
	 if (color < colors-1)
	    putcolor(newcol, newrow, color+1);
	 new.x = leftright * new.x;
	 new.y = leftright * new.y;
	 break;
   }
   return 1;

}

int
Linverse_julia_orbit()
{
   static int   random_dir = 0, random_len = 0;
   int    newrow, newcol;
   int    color;

   /*
    * First, compute new point
    */
   switch (major_method) {
      case breadth_first:
	 if (QueueEmpty())
	    return -1;
	 lnew = DeQueueLong();
	 break;
      case depth_first:
	 if (QueueEmpty())
	    return -1;
	 lnew = PopLong();
	 break;
      case random_walk:
	 lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	 if (RANDOM(2))
	 {
	    lnew.x = -lnew.x;
	    lnew.y = -lnew.y;
	 }
	 break;
      case random_run:
	 lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	 if (random_len == 0)
	 {
	    random_len = RANDOM(run_length);
	    random_dir = RANDOM(3);
	 }
	 switch (random_dir) {
	    case 0:	/* left */
	       break;
	    case 1:	/* right */
	       lnew.x = -lnew.x;
	       lnew.y = -lnew.y;
	       break;
	    case 2:	/* random direction */
	       if (RANDOM(2))
	       {
		  lnew.x = -lnew.x;
		  lnew.y = -lnew.y;
	       }
	       break;
	 }
   }

   /*
    * Next, find its pixel position
    *
    * Note: had to use a bitshift of 21 for this operation because
    * otherwise the values of lcvt were truncated.  Used bitshift
    * of 24 otherwise, for increased precision.
    */
   newcol = (multiply(lcvt.a, lnew.x >> (bitshift - 21), 21) +
	     multiply(lcvt.b, lnew.y >> (bitshift - 21), 21) + lcvt.e) >> 21;
   newrow = (multiply(lcvt.c, lnew.x >> (bitshift - 21), 21) +
	     multiply(lcvt.d, lnew.y >> (bitshift - 21), 21) + lcvt.f) >> 21;

   if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
   {
      /*
       * MIIM must skip points that are off the screen boundary,
       * since it cannot read their color.
       */
      if (RANDOM(2))
	 color =  1;
      else
	 color = -1;
      switch (major_method) {
	 case breadth_first:
	    lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	    EnQueueLong(color * lnew.x, color * lnew.y);
	    break;
	 case depth_first:
	    lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	    PushLong(color * lnew.x, color * lnew.y);
	    break;
	 case random_run:
	    random_len--;
	 case random_walk:
	    break;
      }
      return 1;
   }

   /*
    * Read the pixel's color:
    * For MIIM, if color >= maxhits, discard the point
    *           else put the point's children onto the queue
    */
   color  = getcolor(newcol, newrow);
   switch (major_method) {
      case breadth_first:
	 if (color < maxhits)
	 {
	    putcolor(newcol, newrow, color+1);
	    lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	    EnQueueLong( lnew.x,  lnew.y);
	    EnQueueLong(-lnew.x, -lnew.y);
	 }
	 break;
      case depth_first:
	 if (color < maxhits)
	 {
	    putcolor(newcol, newrow, color+1);
	    lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
	    if (minor_method == left_first)
	    {
	       if (QueueFullAlmost())
		  PushLong(-lnew.x, -lnew.y);
	       else
	       {
		  PushLong( lnew.x,  lnew.y);
		  PushLong(-lnew.x, -lnew.y);
	       }
	    }
	    else
	    {
	       if (QueueFullAlmost())
		  PushLong( lnew.x,  lnew.y);
	       else
	       {
		  PushLong(-lnew.x, -lnew.y);
		  PushLong( lnew.x,  lnew.y);
	       }
	    }
	 }
	 break;
      case random_run:
	 random_len--;
	 /* fall through */
      case random_walk:
	 if (color < colors-1)
	    putcolor(newcol, newrow, color+1);
	 break;
   }
   return 1;
}

#if 0
int inverse_julia_orbit(double *x, double *y, double *z)
{
   double r, xo, yo;
   int waste;
   if(*z >= 1.0) /* this assumes intial value is 1.0!!! */
   {
      waste = 20; /* skip these points at first */
      *z = 0;
   }
   else
      waste = 1;
   while(waste--)
   {
      xo = *x - param[0]; 
      yo = *y - param[1];
      r  = sqrt(xo*xo + yo*yo);
      *x  = sqrt((r + xo)/2);
      if (yo < 0)
         *x = - *x;
      *y = sqrt((r - xo)/2);
      if(RANDOM(10) > 4)
      {
		  *x = -(*x);
		  *y = -(*y);
      }
   }
   return(0);
}
#endif

int lorenz3dlongorbit(long *l_x, long *l_y, long *l_z)
{
      l_xdt = multiply(*l_x,l_dt,bitshift);
      l_ydt = multiply(*l_y,l_dt,bitshift);
      l_dx  = -multiply(l_adt,*l_x,bitshift) + multiply(l_adt,*l_y,bitshift);
      l_dy  =  multiply(l_bdt,*l_x,bitshift) -l_ydt -multiply(*l_z,l_xdt,bitshift);
      l_dz  = -multiply(l_cdt,*l_z,bitshift) + multiply(*l_x,l_ydt,bitshift);

      *l_x += l_dx;
      *l_y += l_dy;
      *l_z += l_dz;
      return(0);
}

int lorenz3d1floatorbit(double *x, double *y, double *z)
{
      double norm;

      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 1-lobe Lorenz */
      norm = sqrt((*x)*(*x)+(*y)*(*y));
      dx   = (-adt-dt)*(*x) + (adt-bdt)*(*y) + (dt-adt)*norm + ydt*(*z);
      dy   = (bdt-adt)*(*x) - (adt+dt)*(*y) + (bdt+adt)*norm - xdt*(*z) -
	     norm*zdt;
      dz   = (ydt/2) - cdt*(*z);

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3dfloatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 2-lobe Lorenz (the original) */
      dx  = -adt*(*x) + adt*(*y);
      dy  =  bdt*(*x) - ydt - (*z)*xdt;
      dz  = -cdt*(*z) + (*x)*ydt;

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3d3floatorbit(double *x, double *y, double *z)
{
      double norm;

      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 3-lobe Lorenz */
      norm = sqrt((*x)*(*x)+(*y)*(*y));
      dx   = (-(adt+dt)*(*x) + (adt-bdt+zdt)*(*y)) / 3 +
	     ((dt-adt)*((*x)*(*x)-(*y)*(*y)) +
	     2*(bdt+adt-zdt)*(*x)*(*y))/(3*norm);
      dy   = ((bdt-adt-zdt)*(*x) - (adt+dt)*(*y)) / 3 +
	     (2*(adt-dt)*(*x)*(*y) +
	     (bdt+adt-zdt)*((*x)*(*x)-(*y)*(*y)))/(3*norm);
      dz   = (3*xdt*(*x)*(*y)-ydt*(*y)*(*y))/2 - cdt*(*z);

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3d4floatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 4-lobe Lorenz */
      dx   = (-adt*(*x)*(*x)*(*x) + (2*adt+bdt-zdt)*(*x)*(*x)*(*y) +
	     (adt-2*dt)*(*x)*(*y)*(*y) + (zdt-bdt)*(*y)*(*y)*(*y)) /
	     (2 * ((*x)*(*x)+(*y)*(*y)));
      dy   = ((bdt-zdt)*(*x)*(*x)*(*x) + (adt-2*dt)*(*x)*(*x)*(*y) +
	     (-2*adt-bdt+zdt)*(*x)*(*y)*(*y) - adt*(*y)*(*y)*(*y)) /
	     (2 * ((*x)*(*x)+(*y)*(*y)));
      dz   = (2*xdt*(*x)*(*x)*(*y) - 2*xdt*(*y)*(*y)*(*y) - cdt*(*z));

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int henonfloatorbit(double *x, double *y, double *z)
{
      double newx,newy;
      newx  = 1 + *y - a*(*x)*(*x);
      newy  = b*(*x);
      *x = newx;
      *y = newy;
      return(0);
}

int henonlongorbit(long *l_x, long *l_y, long *l_z)
{
      long newx,newy;
      newx = multiply(*l_x,*l_x,bitshift);
      newx = multiply(newx,l_a,bitshift);
      newx  = fudge + *l_y - newx;
      newy  = multiply(l_b,*l_x,bitshift);
      *l_x = newx;
      *l_y = newy;
      return(0);
}

int rosslerfloatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;

      dx = -ydt - (*z)*dt;
      dy = xdt + (*y)*adt;
      dz = bdt + (*z)*xdt - (*z)*cdt;

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int pickoverfloatorbit(double *x, double *y, double *z)
{
      double newx,newy,newz;
      newx = sin(a*(*y)) - (*z)*cos(b*(*x));
      newy = (*z)*sin(c*(*x)) - cos(d*(*y));
      newz = sin(*x);
      *x = newx;
      *y = newy;
      *z = newz;
      return(0);
}
/* page 149 "Science of Fractal Images" */
int gingerbreadfloatorbit(double *x, double *y, double *z)
{
      double newx;
      newx = 1 - (*y) + fabs(*x);
      *y = *x;
      *x = newx;
      return(0);
}

int rosslerlongorbit(long *l_x, long *l_y, long *l_z)
{
      l_xdt = multiply(*l_x,l_dt,bitshift);
      l_ydt = multiply(*l_y,l_dt,bitshift);

      l_dx  = -l_ydt - multiply(*l_z,l_dt,bitshift);
      l_dy  =  l_xdt + multiply(*l_y,l_adt,bitshift);
      l_dz  =  l_bdt + multiply(*l_z,l_xdt,bitshift)
		     - multiply(*l_z,l_cdt,bitshift);

      *l_x += l_dx;
      *l_y += l_dy;
      *l_z += l_dz;

      return(0);
}

/* OSTEP  = Orbit Step (and inner orbit value) */
/* NTURNS = Outside Orbit */
/* TURN2  = Points per orbit */
/* a	  = Angle */


int kamtorusfloatorbit(double *r, double *s, double *z)
{
   double srr;
   if(t++ >= l_d)
   {
      orbit += b;
      (*r) = (*s) = orbit/3;
      t = 0;
      *z = orbit;
      if(orbit > c)
	 return(1);
   }
   srr = (*s)-(*r)*(*r);
   (*s)=(*r)*sinx+srr*cosx;
   (*r)=(*r)*cosx-srr*sinx;
   return(0);
}

int kamtoruslongorbit(long *r, long *s, long *z)
{
   long srr;
   if(t++ >= l_d)
   {
      l_orbit += l_b;
      (*r) = (*s) = l_orbit/3;
      t = 0;
      *z = l_orbit;
      if(l_orbit > l_c)
	 return(1);
   }
   srr = (*s)-multiply((*r),(*r),bitshift);
   (*s)=multiply((*r),l_sinx,bitshift)+multiply(srr,l_cosx,bitshift);
   (*r)=multiply((*r),l_cosx,bitshift)-multiply(srr,l_sinx,bitshift);
   return(0);
}
/*#define sign(x) ((x)>0?1:((x)<0?(-1):0))*/
#define sign(x) ((x)>=0?1:-1)
int hopalong2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   tmp = *y - sign(*x)*sqrt(fabs(b*(*x)-c));
   *y = a - *x;
   *x = tmp;
   return(0);
}

int martin2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   tmp = *y - sin(*x);
   *y = a - *x;
   *x = tmp;
   return(0);
}

int mandelcloudfloat(double *x, double *y, double *z)
{
    double newx,newy,x2,y2;
    x2 = (*x)*(*x);
    y2 = (*y)*(*y);
    if (x2+y2>2) return 1;
    newx = x2-y2+a;
    newy = 2*(*x)*(*y)+b;
    *x = newx;
    *y = newy;
    return(0);
}

int dynamfloat(double *x, double *y, double *z)
{
      _CMPLX cp,tmp;
      double newx,newy;
      cp.x = b* *x;
      cp.y = 0;
      CMPLXtrig0(cp, tmp);
      newy = *y + dt*sin(*x + a*tmp.x);
      if (euler) {
	  *y = newy;
      }

      cp.x = b* *y;
      cp.y = 0;
      CMPLXtrig0(cp, tmp);
      newx = *x - dt*sin(*y + a*tmp.x);
      *x = newx;
      *y = newy;
      return(0);
}

/* dmf */
#undef  LAMBDA
#define LAMBDA  param[0]
#define ALPHA   param[1]
#define BETA    param[2]
#define GAMMA   param[3]
#define OMEGA   param[4]
#define DEGREE  param[5]

int iconfloatorbit(double *x, double *y, double *z)
{

    double oldx, oldy, zzbar, zreal, zimag, za, zb, zn, p;
    unsigned char i;

    oldx = *x;
    oldy = *y;

    zzbar = oldx * oldx + oldy * oldy;
    zreal = oldx;
    zimag = oldy;

    for(i=1; i <= DEGREE-2; i++) {
        za = zreal * oldx - zimag * oldy;
        zb = zimag * oldx + zreal * oldy;
        zreal = za;
        zimag = zb;
    }
    zn = oldx * zreal - oldy * zimag;
    p = LAMBDA + ALPHA * zzbar + BETA * zn;
    *x = p * oldx + GAMMA * zreal - OMEGA * oldy;
    *y = p * oldy - GAMMA * zimag + OMEGA * oldx;

/*    if (fractype==ICON3D) */
    *z = zzbar; 
    return(0);
}
#ifdef LAMBDA  /* Tim says this will make me a "good citizen" */
#undef LAMBDA  /* Yeah, but you were bad, Dan - LAMBDA was already */
#undef ALPHA   /* defined! <grin!> TW */
#undef BETA
#undef GAMMA
#endif
/**********************************************************************/
/*   Main fractal engines - put in fractalspecific[fractype].calctype */
/**********************************************************************/

int inverse_julia_per_image()
{
   int color = 0;

   if (resuming)            /* can't resume */
      return -1;

   while (color >= 0)       /* generate points */
   {
      if (check_key())
      {
	 Free_Queue();
	 return -1;
      }
      color = curfractalspecific->orbitcalc();
      old = new;
   }
   Free_Queue();
   return 0;
}

int orbit2dfloat()
{
   FILE *fp;
   double *soundvar;
   double x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   double *p0,*p1,*p2;
   struct affine cvt;
   int ret,start;
   start = 1;
   
   fp = open_orbitsave();
   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   /* set up projection scheme */
   if(projection==0)
   {
      p0 = &z;
      p1 = &x;
      p2 = &y;
   }
   else if(projection==1)
   {
      p0 = &x;
      p1 = &z;
      p2 = &y;
   }
   else if(projection==2)
   {
      p0 = &x;
      p1 = &y;
      p2 = &z;
   }
   if(soundflag==1)
      soundvar = &x;
   else if(soundflag==2)
      soundvar = &y;
   else if(soundflag==3)
      soundvar = &z;

   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;
   x = initorbit[0];
   y = initorbit[1];
   z = initorbit[2];

   if (resuming)
   {
      start_resume();
      get_resume(sizeof(count),&count,sizeof(color),&color,
          sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
          sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,
          sizeof(t),&t,sizeof(orbit),&orbit,
          0);
      end_resume();
   }

   ret = 0;
   while(1)
   {
      if(check_key())
      {
         nosnd();
         alloc_resume(100,1);
         put_resume(sizeof(count),&count,sizeof(color),&color,
             sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
             sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,
             sizeof(t),&t,sizeof(orbit),&orbit,
             0);
         ret = -1;
         break;
      }

      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
	    color = 1;  /* (don't use the background color) */
      }

      col = cvt.a*x + cvt.b*y + cvt.e;
      row = cvt.c*x + cvt.d*y + cvt.f;
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
         if (soundflag > 0)
            snd((int)(*soundvar*100+basehertz));
/* dmf */
         if(fractype!=ICON)
         {
         if(oldcol != -1 && connect)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
            else
            (*plot)(col,row,color&(colors-1));
         } else {
            /* should this be using plothist()? */
            color = getcolor(col,row)+1;
            if( color < colors ) /* color sticks on last value */
               (*plot)(col,row,color);

         }

         oldcol = col;
         oldrow = row;
         start = 0;
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
         return(ret);
      else   
         oldrow = oldcol = -1;

      if(curfractalspecific->orbitcalc(p0, p1, p2))
         break;
      if(fp)
         fprintf(fp,orbitsave_format,*p0,*p1,0.0);
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int orbit2dlong()
{
   FILE *fp;
   long *soundvar;
   long x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   long *p0,*p1,*p2;
   struct l_affine cvt;
   int ret,start;
   start = 1;
   fp = open_orbitsave();

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&cvt);

   /* set up projection scheme */
   if(projection==0)
   {
      p0 = &z;
      p1 = &x;
      p2 = &y;
   }
   else if(projection==1)
   {
      p0 = &x;
      p1 = &z;
      p2 = &y;
   }
   else if(projection==2)
   {
      p0 = &x;
      p1 = &y;
      p2 = &z;
   }
   if(soundflag==1)
      soundvar = &x;
   else if(soundflag==2)
      soundvar = &y;
   else if(soundflag==3)
      soundvar = &z;
   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;
   x = initorbitlong[0];
   y = initorbitlong[1];
   z = initorbitlong[2];

   if (resuming)
   {
      start_resume();
      get_resume(sizeof(count),&count,sizeof(color),&color,
          sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
          sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,
          sizeof(t),&t,sizeof(l_orbit),&l_orbit,
          0);
      end_resume();
   }

   ret = 0;
   while(1)
   {
      if(check_key())
      {
         nosnd();
         alloc_resume(100,1);
         put_resume(sizeof(count),&count,sizeof(color),&color,
	     sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
             sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,
             sizeof(t),&t,sizeof(l_orbit),&l_orbit,
             0);
         ret = -1;
         break;
      }
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;  /* (don't use the background color) */
      }

      col = (multiply(cvt.a,x,bitshift) + multiply(cvt.b,y,bitshift) + cvt.e) >> bitshift;
      row = (multiply(cvt.c,x,bitshift) + multiply(cvt.d,y,bitshift) + cvt.f) >> bitshift;
      if(overflow)
      {
         overflow = 0;
         return(ret);
      }
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
         if (soundflag > 0)
         {
            double yy;
            yy = *soundvar;
            yy = yy/fudge;
            snd((int)(yy*100+basehertz));
         }
         if(oldcol != -1 && connect)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
         else if(!start)
            (*plot)(col,row,color&(colors-1));
         oldcol = col;
         oldrow = row;
         start = 0;
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
         return(ret);
      else   
	 oldrow = oldcol = -1;

      /* Calculate the next point */
      if(curfractalspecific->orbitcalc(p0, p1, p2))
         break;
      if(fp)
         fprintf(fp,orbitsave_format,(double)*p0/fudge,(double)*p1/fudge,0.0);
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int orbit3dlongcalc()
{
   FILE *fp;
   unsigned count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;
   struct long3dvtinf inf;
   unsigned long maxct;
   int color;
   int ret;

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&inf.cvt);

   oldcol1 = oldrow1 = oldcol = oldrow = -1;
   color = 2;
   if(color >= colors)
      color = 1;

   inf.orbit[0] = initorbitlong[0];
   inf.orbit[1] = initorbitlong[1];
   inf.orbit[2] = initorbitlong[2];

   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   fp = open_orbitsave();

   /* make maxct a function of screen size               */
   maxct = maxit*40L;
   count = inf.ct = 0L;
   ret = 0;
   while(inf.ct++ < maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;        /* (don't use the background color) */
      }
      if(check_key())
      {
         nosnd();
         ret = -1;
         break;
      }

      curfractalspecific->orbitcalc(&inf.orbit[0],&inf.orbit[1],&inf.orbit[2]);
      if(fp)
         fprintf(fp,orbitsave_format,(double)inf.orbit[0]/fudge,(double)inf.orbit[1]/fudge,(double)inf.orbit[2]/fudge);
      if (long3dviewtransf(&inf))
      {
         /* plot if inside window */
         if (inf.col >= 0)
         {
            if(realtime)
               whichimage=1;
            if (soundflag > 0)
            {
               double yy;
               yy = inf.viewvect[soundflag-1];
               yy = yy/fudge;
               snd((int)(yy*100+basehertz));
            }
            if(oldcol != -1 && connect)
               draw_line(inf.col,inf.row,oldcol,oldrow,color&(colors-1));
            else
               (*plot)(inf.col,inf.row,color&(colors-1));
         }
         else if (inf.col == -2)
            return(ret);
         oldcol = inf.col;
         oldrow = inf.row;
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
               if(oldcol1 != -1 && connect)
                  draw_line(inf.col1,inf.row1,oldcol1,oldrow1,color&(colors-1));
               else
                  (*plot)(inf.col1,inf.row1,color&(colors-1));
            }
            else if (inf.col1 == -2)
               return(ret);
            oldcol1 = inf.col1;
            oldrow1 = inf.row1;
         }
      }
   }
   if(fp)
      fclose(fp);
   return(ret);
}


int orbit3dfloatcalc()
{
   FILE *fp;
   unsigned count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;
   extern int init3d[];
   unsigned long maxct;
   int color;
   int ret;
   struct float3dvtinf inf;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&inf.cvt);

   oldcol = oldrow = -1;
   oldcol1 = oldrow1 = -1;
   color = 2;
   if(color >= colors)
      color = 1;
   inf.orbit[0] = initorbit[0];
   inf.orbit[1] = initorbit[1];
   inf.orbit[2] = initorbit[2];

   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   fp = open_orbitsave();

   maxct = maxit*40L;
   count = inf.ct = 0L;
   ret = 0;
   while(inf.ct++ < maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;        /* (don't use the background color) */
      }

      if(check_key())
      {
         nosnd();
         ret = -1;
         break;
      }

      curfractalspecific->orbitcalc(&inf.orbit[0],&inf.orbit[1],&inf.orbit[2]);
      if(fp)
         fprintf(fp,orbitsave_format,inf.orbit[0],inf.orbit[1],inf.orbit[2]);
      if (float3dviewtransf(&inf))
      {
         /* plot if inside window */
         if (inf.col >= 0)
         {
	    if(realtime)
               whichimage=1;
            if (soundflag > 0)
               snd((int)(inf.viewvect[soundflag-1]*100+basehertz));
            if(oldcol != -1 && connect)
               draw_line(inf.col,inf.row,oldcol,oldrow,color&(colors-1));
            else
               (*plot)(inf.col,inf.row,color&(colors-1));
         }
         else if (inf.col == -2)
            return(ret);
         oldcol = inf.col;
         oldrow = inf.row;
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
               if(oldcol1 != -1 && connect)
                  draw_line(inf.col1,inf.row1,oldcol1,oldrow1,color&(colors-1));
               else
                  (*plot)(inf.col1,inf.row1,color&(colors-1));
            }
            else if (inf.col1 == -2)
               return(ret);
            oldcol1 = inf.col1;
            oldrow1 = inf.row1;
         }
      }
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int dynam2dfloatsetup()
{
   connect = 0;
   euler = 0;
   d = param[0]; /* number of intervals */
   if (d<0) {
      d = -d;
      connect = 1;
   } 
   else if (d==0) {
      d = 1;
   }
   if (fractype==DYNAMICFP) {
       a = param[2]; /* parameter */
       b = param[3]; /* parameter */
       dt = param[1]; /* step size */
       if (dt<0) {
	  dt = -dt;
	  euler = 1;
       }
       if (dt==0) dt = 0.01;
   }
   if (outside == -5) {
       plot = plothist;
   }
   return(1);
}

/*
 * This is the routine called to perform a time-discrete dynamical
 * system image.
 * The starting positions are taken by stepping across the image in steps
 * of parameter1 pixels.  maxit differential equation steps are taken, with
 * a step size of parameter2.
 */
int dynam2dfloat()
{
   FILE *fp;
   double *soundvar;
   double x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   double *p0,*p1;
   struct affine cvt;
   int ret;
   int xstep, ystep; /* The starting position step number */
   double xpixel, ypixel; /* Our pixel position on the screen */

   fp = open_orbitsave();
   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   p0 = &x;
   p1 = &y;


   if(soundflag==1)
      soundvar = &x;
   else if(soundflag==2)
      soundvar = &y;
   else if(soundflag==3)
      soundvar = &z;

   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;

   xstep = -1;
   ystep = 0;

   if (resuming) {
       start_resume();
       get_resume(sizeof(count),&count, sizeof(color),&color,
		 sizeof(oldrow),&oldrow, sizeof(oldcol),&oldcol,
		 sizeof(x),&x, sizeof(y), &y, sizeof(xstep), &xstep,
		 sizeof(ystep), &ystep, 0);
       end_resume();
   }

   ret = 0;
   while(1)
   {
      if(check_key())
      {
	     nosnd();
	     alloc_resume(100,1);
	     put_resume(sizeof(count),&count, sizeof(color),&color,
		     sizeof(oldrow),&oldrow, sizeof(oldcol),&oldcol,
		     sizeof(x),&x, sizeof(y), &y, sizeof(xstep), &xstep,
		     sizeof(ystep), &ystep, 0);
	     ret = -1;
	     break;
      }

      xstep ++;
      if (xstep>=d) {
	  xstep = 0;
	  ystep ++;
	  if (ystep>d) {
	      nosnd();
	      ret = -1;
	      break;
	  }
      }

      xpixel = dxsize*(xstep+.5)/d;
      ypixel = dysize*(ystep+.5)/d;
      x = (xxmin+delxx*xpixel) + (delxx2*ypixel);
      y = (yymax-delyy*ypixel) + (-delyy2*xpixel);
      if (fractype==MANDELCLOUD) {
	  a = x;
	  b = y;
      }
      oldcol = -1;

      if (++color >= colors)   /* another color to switch to? */
	  color = 1;	/* (don't use the background color) */

      for (count=0;count<maxit;count++) {

	  col = cvt.a*x + cvt.b*y + cvt.e;
	  row = cvt.c*x + cvt.d*y + cvt.f;
	  if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
	  {
	     if (soundflag > 0)
	       snd((int)(*soundvar*100+basehertz));

	     if (count>=orbit_delay) {
		 if(oldcol != -1 && connect)
		    draw_line(col,row,oldcol,oldrow,color&(colors-1));
		 else if(count > 0 || fractype != MANDELCLOUD)
		    (*plot)(col,row,color&(colors-1));
	     }
	     oldcol = col;
	     oldrow = row;
	  }
	  else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
            return(ret);
	  else
	     oldrow = oldcol = -1;

	  if(curfractalspecific->orbitcalc(p0, p1, NULL))
	     break;
	  if(fp)
	      fprintf(fp,orbitsave_format,*p0,*p1,0.0);
	}
   }
   if(fp)
      fclose(fp);
   return(ret);
}

/* this function's only purpose is to manage funnyglasses related */
/* stuff so the code is not duplicated for ifs3d() and lorenz3d() */
int funny_glasses_call(int (*calc)())
{
   int status;
   status = 0;
   if(glassestype)
      whichimage = 1;
   else
      whichimage = 0;
   plot_setup();
   plot = standardplot;
   status = calc();
   if(realtime && glassestype != 3)
   {
      realtime = 0;
      return(status);
   }
   if(glassestype && status == 0 && display3d)
   {
      if(glassestype==3) /* photographer's mode */
	 if(active_system == 0) { /* dos version */
	    int i;
static char far firstready[]={"\
First image (left eye) is ready.  Hit any key to see it,\n\
then hit <s> to save, hit any other key to create second image."};
	    stopmsg(16,firstready);
	    while ((i = getakey()) == 's' || i == 'S') {
	       diskisactive = 1;
	       savetodisk(savename);
	       diskisactive = 0;
	       }
	    /* is there a better way to clear the screen in graphics mode? */
	    setvideomode(videoentry.videomodeax,
		videoentry.videomodebx,
		videoentry.videomodecx,
		videoentry.videomodedx);
	 }
	 else { 		  /* Windows version */
static char far firstready2[]={"First (Left Eye) image is complete"};
	    stopmsg(0,firstready2);
	    clear_screen();
	    }
      whichimage = 2;
      plot_setup();
      plot = standardplot;
      /* is there a better way to clear the graphics screen ? */
      if(status = calc())
	 return(status);
      if(glassestype==3) /* photographer's mode */
	 if(active_system == 0) { /* dos version */
static char far secondready[]={"Second image (right eye) is ready"};
	    stopmsg(16,secondready);
	 }
   }
   return(status);
}

/* double version - mainly for testing */
static int ifs3dfloat()
{
   int color_method;
   FILE *fp;
   unsigned long maxct;
   int color;

   double newx,newy,newz,r,sum;

   int k;
   int ret;

   struct float3dvtinf inf;

   float far *ffptr;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&inf.cvt);
   srand(1);
   color_method = param[0];
   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   inf.orbit[0] = 0;
   inf.orbit[1] = 0;
   inf.orbit[2] = 0;

   fp = open_orbitsave();

   maxct = maxit*40L;
   inf.ct = 0L;
   ret = 0;
   while(inf.ct++ < maxct) /* loop until keypress or maxit */
   {
      if( check_key() )  /* keypress bails out */
      {
	 ret = -1;
	 break;
      }
      r = rand15();	 /* generate fudged random number between 0 and 1 */
      r /= 32767;

      /* pick which iterated function to execute, weighted by probability */
      sum = ifs_defn[12]; /* [0][12] */
      k = 0;
      while ( sum < r)
      {
	 k++;
	 sum += ifs_defn[k*IFS3DPARM+12];
	 if (ifs_defn[(k+1)*IFS3DPARM+12] == 0) break; /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      ffptr = ifs_defn + k*IFS3DPARM; /* point to first parm in row */
      newx = *ffptr * inf.orbit[0] +
	     *(ffptr+1) * inf.orbit[1] +
	     *(ffptr+2) * inf.orbit[2] + *(ffptr+9);
      newy = *(ffptr+3) * inf.orbit[0] +
	     *(ffptr+4) * inf.orbit[1] +
	     *(ffptr+5) * inf.orbit[2] + *(ffptr+10);
      newz = *(ffptr+6) * inf.orbit[0] +
	     *(ffptr+7) * inf.orbit[1] +
	     *(ffptr+8) * inf.orbit[2] + *(ffptr+11);

      inf.orbit[0] = newx;
      inf.orbit[1] = newy;
      inf.orbit[2] = newz;
      if(fp)
	  fprintf(fp,orbitsave_format,newx,newy,newz);
      if (float3dviewtransf(&inf))
      {
	 /* plot if inside window */
	 if (inf.col >= 0)
	 {
	    if(realtime)
	       whichimage=1;
            if(color_method)
               color = (k&(colors-1))+1;
            else
	    color = getcolor(inf.col,inf.row)+1;
	    if( color < colors ) /* color sticks on last value */
	       (*plot)(inf.col,inf.row,color);
	 }
         else if (inf.col == -2)
            return(ret);
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    if (inf.col1 >= 0)
	    {
              if(color_method)
                 color = (k&(colors-1))+1;
              else
	        color = getcolor(inf.col1,inf.row1)+1;
	        if( color < colors ) /* color sticks on last value */
		  (*plot)(inf.col1,inf.row1,color);
	    }
	    else if (inf.col1 == -2)
               return(ret);
	 }
      }
   } /* end while */
   if(fp)
      fclose(fp);
   return(ret);
}

int ifs()			/* front-end for ifs2d and ifs3d */
{
   if (ifs_defn == NULL && ifsload() < 0)
      return(-1);
   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();
   return((ifs_type == 0) ? ifs2d() : ifs3d());
}


/* IFS logic shamelessly converted to integer math */
int ifs2d()
{
   int color_method;
   FILE *fp;
   unsigned long maxct,ct;
   int col;
   int row;
   int color;
   int ret;
   long far *localifs;
   long far *lfptr;
   long x,y,newx,newy,r,sum, tempr;

   int i,j,k;
   struct l_affine cvt;
   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&cvt);

   srand(1);
   color_method = param[0];
   if((localifs=(long far *)farmemalloc((long)numaffine*IFSPARM*sizeof(long)))==NULL)
   {
      stopmsg(0,insufficient_ifs_mem);
      return(-1);
   }

   for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
      for (j = 0; j < IFSPARM; j++)
	 localifs[i*IFSPARM+j] = ifs_defn[i*IFSPARM+j] * fudge;

   tempr = fudge / 32767;	 /* find the proper rand() fudge */

   fp = open_orbitsave();

   /* make maxct a function of screen size		 */
   /* 1k times maxit at EGA resolution seems about right */
   maxct = (float)maxit*(1024.0*xdots*ydots)/(640.0*350.0);
   ct = 0L;
   x = y = 0;
   ret = 0;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      if( check_key() )  /* keypress bails out */
      {
	 ret = -1;
	 break;
      }
      r = rand15();	 /* generate fudged random number between 0 and 1 */
      r *= tempr;

      /* pick which iterated function to execute, weighted by probability */
      sum = localifs[6];  /* [0][6] */
      k = 0;
      while ( sum < r && k < numaffine-1) /* fixed bug of error if sum < 1 */
	 sum += localifs[++k*IFSPARM+6];
      /* calculate image of last point under selected iterated function */
      lfptr = localifs + k*IFSPARM; /* point to first parm in row */
      newx = multiply(lfptr[0],x,bitshift) + 
             multiply(lfptr[1],y,bitshift) + lfptr[4];
      newy = multiply(lfptr[2],x,bitshift) + 
             multiply(lfptr[3],y,bitshift) + lfptr[5];
      x = newx;
      y = newy;
      if(fp)
	 fprintf(fp,orbitsave_format,(double)newx/fudge,(double)newy/fudge,0.0);

      /* plot if inside window */
      col = (multiply(cvt.a,x,bitshift) + multiply(cvt.b,y,bitshift) + cvt.e) >> bitshift;
      row = (multiply(cvt.c,x,bitshift) + multiply(cvt.d,y,bitshift) + cvt.f) >> bitshift;
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
	 /* color is count of hits on this pixel */
         if(color_method)
            color = (k&(colors-1))+1;
         else
	 color = getcolor(col,row)+1;
	 if( color < colors ) /* color sticks on last value */
	    (*plot)(col,row,color);
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
            return(ret);
   }
   if(fp)
      fclose(fp);
   farmemfree(localifs);
   return(ret);
}

static int ifs3dlong()
{
   int color_method;   
   FILE *fp;
   extern int init3d[];
   unsigned long maxct;
   int color;
   int ret;

   long far *localifs;
   long far *lfptr;
   long newx,newy,newz,r,sum, tempr;

   int i,j,k;

   struct long3dvtinf inf;
   srand(1);
   color_method = param[0];
   if((localifs=(long far *)farmemalloc((long)numaffine*IFS3DPARM*sizeof(long)))==NULL)
   {
      stopmsg(0,insufficient_ifs_mem);
      return(-1);
   }

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&inf.cvt);

   for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
      for (j = 0; j < IFS3DPARM; j++)
	 localifs[i*IFS3DPARM+j] = ifs_defn[i*IFS3DPARM+j] * fudge;

   tempr = fudge / 32767;	 /* find the proper rand() fudge */

   inf.orbit[0] = 0;
   inf.orbit[1] = 0;
   inf.orbit[2] = 0;

   fp = open_orbitsave();

   maxct = maxit*40L;
   inf.ct = 0L;
   ret = 0;
   while(inf.ct++ < maxct) /* loop until keypress or maxit */
   {
      if( check_key() )  /* keypress bails out */
      {
	 ret = -1;
	 break;
      }
      r = rand15();	 /* generate fudged random number between 0 and 1 */
      r *= tempr;

      /* pick which iterated function to execute, weighted by probability */
      sum = localifs[12];  /* [0][12] */
      k = 0;
      while ( sum < r && ++k < numaffine*IFS3DPARM)
	 sum += localifs[k*IFS3DPARM+12];

      /* calculate image of last point under selected iterated function */
      lfptr = localifs + k*IFS3DPARM; /* point to first parm in row */

      /* calculate image of last point under selected iterated function */
      newx = multiply(lfptr[0], inf.orbit[0], bitshift) +
             multiply(lfptr[1], inf.orbit[1], bitshift) +
	     multiply(lfptr[2], inf.orbit[2], bitshift) + lfptr[9];
      newy = multiply(lfptr[3], inf.orbit[0], bitshift) +
	     multiply(lfptr[4], inf.orbit[1], bitshift) +
	     multiply(lfptr[5], inf.orbit[2], bitshift) + lfptr[10];
      newz = multiply(lfptr[6], inf.orbit[0], bitshift) +
	     multiply(lfptr[7], inf.orbit[1], bitshift) +
	     multiply(lfptr[8], inf.orbit[2], bitshift) + lfptr[11];

      inf.orbit[0] = newx;
      inf.orbit[1] = newy;
      inf.orbit[2] = newz;
      if(fp)
	 fprintf(fp,orbitsave_format,(double)newx/fudge,(double)newy/fudge,(double)newz/fudge);

      if (long3dviewtransf(&inf))
      {
	 if((long)abs(inf.row) + (long)abs(inf.col) > BAD_PIXEL) /* sanity check */
            return(ret);
	 /* plot if inside window */
	 if (inf.col >= 0)
	 {
	    if(realtime)
	       whichimage=1;
            if(color_method)
               color = (k&(colors-1))+1;
            else
	    color = getcolor(inf.col,inf.row)+1;
	    if( color < colors ) /* color sticks on last value */
	       (*plot)(inf.col,inf.row,color);
	 }
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    if (inf.col1 >= 0)
	    {
               if(color_method)
                  color = (k&(colors-1))+1;
               else
	       color = getcolor(inf.col1,inf.row1)+1;
	       if( color < colors ) /* color sticks on last value */
		  (*plot)(inf.col1,inf.row1,color);
	    }
	 }
      }
   }
   if(fp)
      fclose(fp);
   farmemfree(localifs);
   return(ret);
}

static void setupmatrix(MATRIX doublemat)
{
   /* build transformation matrix */
   identity (doublemat);

   /* apply rotations - uses the same rotation variables as line3d.c */
   xrot ((double)XROT / 57.29577,doublemat);
   yrot ((double)YROT / 57.29577,doublemat);
   zrot ((double)ZROT / 57.29577,doublemat);

   /* apply scale */
/*   scale((double)XSCALE/100.0,(double)YSCALE/100.0,(double)ROUGH/100.0,doublemat);*/

}

int orbit3dfloat()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   return(funny_glasses_call(orbit3dfloatcalc));
}

int orbit3dlong()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   return(funny_glasses_call(orbit3dlongcalc));
}

int ifs3d()
{
   display3d = -1;

   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   if(floatflag)
      return(funny_glasses_call(ifs3dfloat)); /* double version of ifs3d */
   else
      return(funny_glasses_call(ifs3dlong));  /* long version of ifs3d	 */
}



static int long3dviewtransf(struct long3dvtinf *inf)
{
   int i,j;
   double tmpx, tmpy, tmpz;
   long tmp;

   if (inf->ct == 1)	/* initialize on first call */
   {
      for(i=0;i<3;i++)
      {
	 inf->minvals[i] =  1L << 30;
	 inf->maxvals[i] = -inf->minvals[i];
      }
      setupmatrix(inf->doublemat);
      if(realtime)
	 setupmatrix(inf->doublemat1);
      /* copy xform matrix to long for for fixed point math */
      for (i = 0; i < 4; i++)
	 for (j = 0; j < 4; j++)
	 {
	    inf->longmat[i][j] = inf->doublemat[i][j] * fudge;
	    if(realtime)
	       inf->longmat1[i][j] = inf->doublemat1[i][j] * fudge;
	 }
   }

   /* 3D VIEWING TRANSFORM */
   longvmult(inf->orbit,inf->longmat,inf->viewvect,bitshift);
   if(realtime)
      longvmult(inf->orbit,inf->longmat1,inf->viewvect1,bitshift);

   if(inf->ct <= waste) /* waste this many points to find minz and maxz */
   {
      /* find minz and maxz */
      for(i=0;i<3;i++)
	 if ((tmp = inf->viewvect[i]) < inf->minvals[i])
	    inf->minvals[i] = tmp;
	 else if (tmp > inf->maxvals[i])
	    inf->maxvals[i] = tmp;

      if(inf->ct == waste) /* time to work it out */
      {
	 inf->iview[0] = inf->iview[1] = 0L; /* center viewer on origin */

	 /* z value of user's eye - should be more negative than extreme
			negative part of image */
	 inf->iview[2] = (long)((inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0);

	 /* center image on origin */
	 tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
	 tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

	 /* apply perspective shift */
	 tmpx += ((double)xshift*(xxmax-xxmin))/(xdots);
	 tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	 tmpz = -((double)inf->maxvals[2]) / fudge;
	 trans(tmpx,tmpy,tmpz,inf->doublemat);

	 if(realtime)
	 {
	    /* center image on origin */
	    tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
	    tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

	    tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	    tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	    tmpz = -((double)inf->maxvals[2]) / fudge;
	    trans(tmpx,tmpy,tmpz,inf->doublemat1);
	 }
	 for(i=0;i<3;i++)
	    view[i] = (double)inf->iview[i] / fudge;

	 /* copy xform matrix to long for for fixed point math */
	 for (i = 0; i < 4; i++)
	    for (j = 0; j < 4; j++)
	    {
	       inf->longmat[i][j] = inf->doublemat[i][j] * fudge;
	       if(realtime)
		  inf->longmat1[i][j] = inf->doublemat1[i][j] * fudge;
	    }
      }
      return(0);
   }

   /* inf->ct > waste */
   /* apply perspective if requested */
   if(ZVIEWER)
   {
      if(debugflag==22 || ZVIEWER < 100) /* use float for small persp */
      {
	 /* use float perspective calc */
	 VECTOR tmpv;
	 for(i=0;i<3;i++)
	    tmpv[i] = (double)inf->viewvect[i] / fudge;
	 perspective(tmpv);
	 for(i=0;i<3;i++)
	    inf->viewvect[i] = tmpv[i]*fudge;
	 if(realtime)
	 {
	    for(i=0;i<3;i++)
	       tmpv[i] = (double)inf->viewvect1[i] / fudge;
	    perspective(tmpv);
	    for(i=0;i<3;i++)
	       inf->viewvect1[i] = tmpv[i]*fudge;
	 }
      }
      else
      {
	 longpersp(inf->viewvect,inf->iview,bitshift);
	 if(realtime)
	    longpersp(inf->viewvect1,inf->iview,bitshift);
      }
   }

   /* work out the screen positions */
   inf->row = ((multiply(inf->cvt.c,inf->viewvect[0],bitshift) +
		multiply(inf->cvt.d,inf->viewvect[1],bitshift) + inf->cvt.f)
		>> bitshift)
	      + yyadjust;
   inf->col = ((multiply(inf->cvt.a,inf->viewvect[0],bitshift) +
		multiply(inf->cvt.b,inf->viewvect[1],bitshift) + inf->cvt.e)
		>> bitshift)
	      + xxadjust;
   if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
   {
      if((long)abs(inf->col)+(long)abs(inf->row) > BAD_PIXEL)
        inf->col= inf->row = -2;
      else
        inf->col= inf->row = -1;
   }    
   if(realtime)
   {
      inf->row1 = ((multiply(inf->cvt.c,inf->viewvect1[0],bitshift) +
		    multiply(inf->cvt.d,inf->viewvect1[1],bitshift) +
		    inf->cvt.f) >> bitshift)
		  + yyadjust1;
      inf->col1 = ((multiply(inf->cvt.a,inf->viewvect1[0],bitshift) +
		    multiply(inf->cvt.b,inf->viewvect1[1],bitshift) +
		    inf->cvt.e) >> bitshift)
		  + xxadjust1;
      if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
      {
         if((long)abs(inf->col1)+(long)abs(inf->row1) > BAD_PIXEL)
           inf->col1= inf->row1 = -2;
         else
           inf->col1= inf->row1 = -1;
      }    
   }
   return(1);
}

static int float3dviewtransf(struct float3dvtinf *inf)
{
   int i;
   double tmpx, tmpy, tmpz;
   double tmp;

   if (inf->ct == 1)	/* initialize on first call */
   {
      for(i=0;i<3;i++)
      {
	 inf->minvals[i] =  100000.0; /* impossible value */
	 inf->maxvals[i] = -100000.0;
      }
      setupmatrix(inf->doublemat);
      if(realtime)
	 setupmatrix(inf->doublemat1);
   }

   /* 3D VIEWING TRANSFORM */
   vmult(inf->orbit,inf->doublemat,inf->viewvect );
   if(realtime)
      vmult(inf->orbit,inf->doublemat1,inf->viewvect1);

   if(inf->ct <= waste) /* waste this many points to find minz and maxz */
   {
      /* find minz and maxz */
      for(i=0;i<3;i++)
	 if ((tmp = inf->viewvect[i]) < inf->minvals[i])
	    inf->minvals[i] = tmp;
	 else if (tmp > inf->maxvals[i])
	    inf->maxvals[i] = tmp;
      if(inf->ct == waste) /* time to work it out */
      {
	 view[0] = view[1] = 0; /* center on origin */
	 /* z value of user's eye - should be more negative than extreme
			   negative part of image */
	 view[2] = (inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0;

	 /* center image on origin */
	 tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
	 tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

	 /* apply perspective shift */
	 tmpx += ((double)xshift*(xxmax-xxmin))/(xdots);
	 tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	 tmpz = -(inf->maxvals[2]);
	 trans(tmpx,tmpy,tmpz,inf->doublemat);

	 if(realtime)
	 {
	    /* center image on origin */
	    tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
	    tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

	    tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	    tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	    tmpz = -(inf->maxvals[2]);
	    trans(tmpx,tmpy,tmpz,inf->doublemat1);
	    }
	 }
      return(0);
      }

   /* inf->ct > waste */
   /* apply perspective if requested */
   if(ZVIEWER)
   {
      perspective(inf->viewvect);
      if(realtime)
	 perspective(inf->viewvect1);
   }
   inf->row = inf->cvt.c*inf->viewvect[0] + inf->cvt.d*inf->viewvect[1]
	    + inf->cvt.f + yyadjust;
   inf->col = inf->cvt.a*inf->viewvect[0] + inf->cvt.b*inf->viewvect[1]
	    + inf->cvt.e + xxadjust;
   if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
   {
      if((long)abs(inf->col)+(long)abs(inf->row) > BAD_PIXEL)
        inf->col= inf->row = -2;
      else
        inf->col= inf->row = -1;
   }    
   if(realtime)
   {
      inf->row1 = inf->cvt.c*inf->viewvect1[0] + inf->cvt.d*inf->viewvect1[1]
		+ inf->cvt.f + yyadjust1;
      inf->col1 = inf->cvt.a*inf->viewvect1[0] + inf->cvt.b*inf->viewvect1[1]
		+ inf->cvt.e + xxadjust1;
      if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
      {
         if((long)abs(inf->col1)+(long)abs(inf->row1) > BAD_PIXEL)
           inf->col1= inf->row1 = -2;
         else
           inf->col1= inf->row1 = -1;
      }    
   }
   return(1);
}

static FILE *open_orbitsave()
{
   FILE *fp;
   if (orbitsave && (fp = fopen(orbitsavename,"w")))
   {
      fprintf(fp,"pointlist x y z color\n");
      return fp;
   }
   return NULL;
}

/* Plot a histogram by incrementing the pixel each time it it touched */
static void _fastcall plothist(x, y, color)
int x,y,color;
{
    color = getcolor(x,y)+1;
    if (color<colors) 
	putcolor(x,y,color);
}




