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

/* -------------------------------------------------------------------- */
/*	These variables are not specific to any particular fractal	*/
/* -------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include "fractint.h"
#include "mpmath.h"

#ifndef __TURBOC__
#include <malloc.h>
#endif

/* defines should match fractalspecific array indices */

/* first values defined in fractype.h for historical reasons  - from there:
  #define NOFRACTAL      -1
  #define MANDEL          0 
  #define JULIA           1 
  #define NEWTBASIN       2 
  #define LAMBDA          3 
  #define MANDELFP        4 
  #define NEWTON          5 
  #define JULIAFP         6 
  #define PLASMA          7 
*/

#define LAMBDASINE        8 
#define LAMBDACOS         9 
#define LAMBDAEXP         10
#define TEST              11
#define SIERPINSKI        12
#define BARNSLEYM1        13
#define BARNSLEYJ1        14
#define BARNSLEYM2        15
#define BARNSLEYJ2        16
#define MANDELSINE        17
#define MANDELCOS         18
#define MANDELEXP         19
#define MANDELLAMBDA      20
#define MARKSMANDEL       21
#define MARKSJULIA        22
#define UNITY             23
#define MANDEL4           24
#define JULIA4            25
#define IFS               26
#define IFS3D             27
#define BARNSLEYM3        28
#define BARNSLEYJ3        29
#define DEMM              30
#define DEMJ              31
#define BIFURCATION       32
#define MANDELSINH        33
#define LAMBDASINH        34
#define MANDELCOSH        35
#define LAMBDACOSH        36
#define LMANDELSINE       37
#define LLAMBDASINE       38
#define LMANDELCOS        39
#define LLAMBDACOS        40
#define LMANDELSINH       41
#define LLAMBDASINH       42
#define LMANDELCOSH       43
#define LLAMBDACOSH       44
#define LMANSINZSQRD      45
#define LJULSINZSQRD      46
#define FPMANSINZSQRD     47
#define FPJULSINZSQRD     48
#define LMANDELEXP        49
#define LLAMBDAEXP        50
#define LMANDELZPOWER     51
#define LJULIAZPOWER      52
#define FPMANDELZPOWER    53
#define FPJULIAZPOWER     54
#define FPMANZTOZPLUSZPWR 55
#define FPJULZTOZPLUSZPWR 56
#define LMANSINEXP        57
#define LJULSINEXP        58
#define FPMANSINEXP       59
#define FPJULSINEXP       60
#define FPPOPCORN         61
#define LPOPCORN          62
#define FPLORENZ          63
#define LLORENZ           64
#define LORENZ3D          65
#define MPNEWTON          66 
#define MPNEWTBASIN       67 
#define COMPLEXNEWTON     68
#define COMPLEXBASIN      69

/* DEFINITIONS PRIOR TO THIS POINT ARE FROZEN BY THE VERSION 11.0 RELEASE! */

#define COMPLEXMARKSMAND  70
#define COMPLEXMARKSJUL   71
#define FORMULA           72
#define FFORMULA          73
#define SIERPINSKIFP      74
#define LAMBDAFP          75
#define BARNSLEYM1FP      76
#define BARNSLEYJ1FP      77
#define BARNSLEYM2FP      78
#define BARNSLEYJ2FP      79
#define BARNSLEYM3FP      80
#define BARNSLEYJ3FP      81
#define MANDELLAMBDAFP    82
#define JULIBROT          83

/* DEFINITIONS PRIOR TO THIS POINT ARE FROZEN BY THE VERSION 12.0 RELEASE! */


#define NEWTONDEGREELIMIT  100
extern int xshift, yshift;
extern void draw_line(int,int,int,int,int);
long Exp086(long);
double fmod(double,double);
extern int biomorph;
extern int forcesymmetry;
struct lcomplex lcoefficient,lold,lnew,llambda, linit,ltemp;
long ltempsqrx,ltempsqry;
extern int decomp[];
extern double param[];
extern double inversion[];                     /* inversion parameters */
extern double f_radius,f_xcenter,f_ycenter;    /* inversion radius, center */
extern double xxmin,xxmax,yymin,yymax;         /* corners */
extern VECTOR view;
extern int init3d[];
extern int overflow;

int maxcolor;
int root, degree,basin;
double floatmin,floatmax;
double roverd, d1overd, threshold;
long lroverd,ld1overd, lthreshold;
int switchedtofloat;
extern struct complex init,tmp,old,new,saved,ComplexPower();
struct complex  staticroots[16]; /* roots array for degree 16 or less */
struct complex  *roots = staticroots;
struct MPC      *MPCroots;
extern int color, oldcolor, oldmax, oldmax10, row, col, passes;
extern int iterations, invert;
extern double far *dx0, far *dy0;
extern long XXOne, FgOne, FgTwo, LowerLimit;
struct complex one;
struct complex pwr;
struct complex Coefficient;

extern void (*plot)();

extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors;				/* maximum colors available */
extern int	inside;				/* "inside" color to use    */
extern int  bof_pp60_61;        /* "Beauty of Fractals pp. 60/61 flag */ 
extern int  min_index;          /* iteration for min_orbit */
extern double min_orbit;        /* orbit value closest to origin */
extern int	maxit;				/* try this many iterations */
extern int	fractype;			/* fractal type */
extern int	numpasses;			/* 0 = 1 pass, 1 = double pass */
extern int	solidguessing;		/* 1 if solid-guessing */
extern int	debugflag;			/* for debugging purposes */
extern int	timerflag;			/* ditto */
extern	int	diskvideo;			/* for disk-video klooges */

extern double	param[];		/* parameters */
extern double	potparam[];		/* potential parameters */
extern long	far *lx0, far *ly0;		/* X, Y points */
extern long	delx,dely;			/* X, Y increments */
extern long	fudge;				/* fudge factor (2**n) */
extern int	bitshift;			/* bit shift for fudge */
extern char potfile[];          /* potential filename */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1,z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))
#define pMPsqr(z) (pMPmul((z),(z)))
#define MPdistance(z1,z2)  (pMPadd(pMPsqr(pMPsub((z1).x,(z2).x)),pMPsqr(pMPsub((z1).y,(z2).y))))

double twopi = PI*2.0;
static int c_exp;

/* These are local but I don't want to pass them as parameters */
extern struct complex lambda;
extern double deltaX, deltaY;
extern double magnitude, rqlim, rqlim2;
extern int XXdots, YYdots; /* local dots */
struct complex parm;
struct complex *floatparm;
struct lcomplex *longparm;
extern int (*calctype)();
extern double closenuff;
extern int pixelpi; /* value of pi in pixels */
extern FILE *fp_pot;
extern int potflag; /* tells if continuous potential on  */
extern unsigned long lm;		/* magnitude limit (CALCMAND) */

extern int	ixstart, ixstop, iystart, iystop;	/* (for CALCMAND) start, stop here */

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only	*/
/* -------------------------------------------------------------------- */

double sinx,cosx,sinhx,coshx;
double siny,cosy,sinhy,coshy;
double tmpexp;
double tempsqrx,tempsqry;

double foldxinitx,foldyinity,foldxinity,foldyinitx;
long oldxinitx,oldyinity,oldxinity,oldyinitx;
long modulusinit;
long ltmp1,ltmp2,ltmp3;
struct lcomplex ltmp;
extern long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
extern periodicitycheck;
extern char floatflag;

extern int StandardFractal();
extern int NewtonFractal2(); /* Lee Crocker's Newton code */

/* these are in mpmath_c.c */
extern int ComplexNewtonSetup(void);
extern int ComplexNewton(void), ComplexBasin(void), MarksCplxMand(void);
extern int MarksCplxMandperp(void);
complex_mult(struct complex arg1,struct complex arg2,struct complex *pz);
complex_div(struct complex arg1,struct complex arg2,struct complex *pz);

/* these are in (I think) JB.C */
extern int Std4dFractal(), JulibrotSetup(), jb_per_pixel();


/* -------------------------------------------------------------------- */
/*		Stand-alone routines											*/
/* -------------------------------------------------------------------- */

extern char far plasmamessage[];

/* Thanks to Rob Beyer */
floatlorenz() 
{
   int count;
   double dx,dy,dz,x,y,z,dt,a,b,c;
   double adt,bdt,cdt,xdt,ydt;
   int oldrow, oldcol;
   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;   
   oldcol = oldrow = -1;
   x = 1;  /* initial conditions */
   y = 1;
   z = 1;
   
   dt = fractalspecific[fractype].paramvalue[0];
   a  = fractalspecific[fractype].paramvalue[1];
   b  = fractalspecific[fractype].paramvalue[2];
   c  = fractalspecific[fractype].paramvalue[3];
         
   /* precalculations for speed */
   adt = a*dt;
   bdt = b*dt;
   cdt = c*dt;
   
   while(1)
   {
      if (++count > 1000) 
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
              color = 1;        /* (don't use the background color) */
      }
      if(check_key())
         return(-1);
      if ( x > xxmin && x < xxmax && z > yymin && z < yymax )
      {
         col =          (( x-xxmin) / deltaX);
         row = YYdots - (( z-yymin) / deltaY);
         if(oldcol != -1)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
         else            
            (*plot)(col,row,color&(colors-1));
         oldcol = col;
         oldrow = row;    
      }
      else
         oldrow = oldcol = -1;

      /* Calculate the next point */
      xdt = x*dt;
      ydt = y*dt;
      dx = -(adt * x) + (adt * y);
      dy =  (bdt * x) - (ydt) - (z * xdt);
      dz = -(cdt * z) + (x * ydt);

      x += dx;
      y += dy;
      z += dz;
   }
   return(0); 
}

longlorenz() 
{
   int count;
   long delx,dely,xmin,xmax,ymin,ymax;
   long dx,dy,dz,x,y,z,dt,a,b,c;
   long adt,bdt,cdt,xdt,ydt;
   int oldrow, oldcol;
   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;   
   oldcol = oldrow = -1;
   fudge = 1L<<bitshift;
   delx = deltaX*fudge;
   dely = deltaY*fudge;
   xmin = xxmin*fudge;
   ymin = yymin*fudge;
   xmax = xxmax*fudge;
   ymax = yymax*fudge;
   x = fudge;  /* initial conditions */
   y = fudge;
   z = fudge;

   /* for speed am assuming a,b,c are integers (they are not fudged) */
   dt = fractalspecific[fractype].paramvalue[0] * fudge;
   a  = fractalspecific[fractype].paramvalue[1];
   b  = fractalspecific[fractype].paramvalue[2];
   c  = fractalspecific[fractype].paramvalue[3];

   /* precalculations for speed */
   adt = a*dt;
   bdt = b*dt;
   cdt = c*dt;

   while(1)
   {
      if (++count > 1000) 
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
              color = 1;        /* (don't use the background color) */
      }
      if(check_key())
         return(-1);
      if ( x > xmin && x < xmax && z > ymin && z < ymax )
      {
         col =          (( x-xmin) / delx);
         row = YYdots - (( z-ymin) / dely);
         if(oldcol != -1)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
         else            
            (*plot)(col,row,color&(colors-1));
         oldcol = col;
         oldrow = row;    
      }
      else
         oldrow = oldcol = -1;

      /* Calculate the next point */
      
      xdt = multiply(x,dt,bitshift);
      ydt = multiply(y,dt,bitshift);
      dx  = -multiply(adt,x,bitshift) + multiply(adt,y,bitshift);
      dy  =  multiply(bdt,x,bitshift) -ydt -multiply(z,xdt,bitshift);
      dz  = -multiply(cdt,z,bitshift) + multiply(x,ydt,bitshift);

      x += dx;
      y += dy;
      z += dz;
   }
   return(0); 
}

/* this ought to be combined with ifs3d */
extern int lorenz3dlong();

ifs()            /* IFS logic shamelessly converted to integer math */
{
   long	 *lifsptr;
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   long localifs[NUMIFS][7];        /* local IFS values */
   long x,y,newx,newy,r,sum, xmin, xmax, ymin, ymax, tempr;

   int i,j,k;

   srand(1);
   
   if(diskvideo) {		/* this would KILL a disk drive! */
	setvideomode(3,0,0,0);
	buzzer(2);
	helpmessage(plasmamessage);
	return(-1);
	}

   for (i = 0; i < NUMIFS; i++)    /* fill in the local IFS array */
   for (j = 0; j < IFSPARM; j++)
         localifs[i][j] = initifs[i][j] * fudge;

   xmin  = lx0[0];              /* find the screen corners */
   xmax  = lx0[xdots-1];
   ymax  = ly0[0];
   ymin  = ly0[ydots-1];

   tempr = fudge / 32767;        /* find the proper rand() fudge */

   x = 0;
   y = 0;

   /* make maxct a function of screen size               */
   /* 1k times maxit at EGA resolution seems about right */
   maxct = (float)maxit*(1024.0*xdots*ydots)/(640.0*350.0); 
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */ 
   {
      if (ct & 127)           /* reduce the number of keypress checks */
         if( check_key() )    /* keypress bails out */
            return(-1);
      r = rand();        /* generate fudged random number between 0 and 1 */
      r *= tempr;

      /* pick which iterated function to execute, weighted by probability */
      sum = localifs[0][6];
      k = 0;
      while ( sum < r)
      {
         k++;
         sum += localifs[k][6];
         if (localifs[k+1][6] == 0) break;    /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      newx = multiply(localifs[k][0], x, bitshift) +
          multiply(localifs[k][1], y, bitshift) +
          localifs[k][4];
      newy = multiply(localifs[k][2], x, bitshift) +
          multiply(localifs[k][3], y, bitshift) +
          localifs[k][5];
      x = newx;
      y = newy;

      /* plot if inside window */
      if ( x > xmin && x < xmax && y > ymin && y < ymax )
      {
         col =          (( x-xmin) / delx);
         row = YYdots - (( y-ymin) / dely);

         /* color is count of hits on this pixel */
         color = getcolor(col,row)+1;
         if( color < colors ) /* color sticks on last value */
            (*plot)(col,row,color);
      }
   }
   return(0);
}

int ifs3d();

/* START Phil Wilson's Code */

static	int		dem_start( void );
static	void		dem_end( void );
static	int		dem_pt( void );
static	double	MSetDist( void );
static	double	JSetDist( void );

#ifndef __TURBOC__
	void far * cdecl _fmalloc(size_t); /* from MSC's <malloc.h> */
#endif

#define BIG 100000.0
#define XOVERFLOW 100000000000000.0

static	int		mono, outside;
static	double	delta, pixelwidth;
static	double	(*DistEstimate)();
static	struct	complex far *orbit;
static	struct	complex deriv, squared;

DistanceEstimatorMethod()
{
   if ( ! dem_start()) return(-1);
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
            dem_end();
            return(-1);
         }
         color = dem_pt();
         (*plot)(col,row,color);
         if(numpasses && (passes == 0))
             (*plot)(col,row+1,color);
      }
   }
   dem_end();
   return(0);
}

static int dem_start( void )
{
	unsigned long array_size = (maxit+1) * sizeof(*orbit);

#ifdef __TURBOC__
	if (( orbit = (struct complex far *) farmemalloc( array_size )) == NULL)
	{
		printf("\n\n\n\n\n\n\n\n\n\n\n\n");
		printf("Not enough Memory for array with %d orbits\n", maxit);
		printf(" (%ul Bytes Required)", array_size);
		buzzer(2);
		dem_end();
		return(0);
	}
#else	/* Assume MSC, whose _fmalloc seems able to find more memory than */
		/* farmemalloc().  Important for those maxiter=4096 all-nighters  */

	if (( orbit = (struct complex far *) _fmalloc( array_size )) == NULL)
	{
		printf("\n\n\n\n\n\n\n\n\n\n\n\n");
		printf("Not enough Memory for array with %d orbits\n", maxit);
		printf(" (%ul Bytes Required)", array_size);
		buzzer(2);
		dem_end();
		return(0);
	}
#endif	

	delta = (deltaX + deltaY) / 4;	/* half a pixel width */
	pixelwidth = 2*delta;

	if ( colors == 2 ) mono = 1;
	if ( mono ) outside = !inside;

	if ( strncmp( fractalspecific[fractype].name, "demm", 4 ) == 0)
		DistEstimate = MSetDist;
	else if ( strncmp( fractalspecific[fractype].name, "demj", 4 ) == 0)
		DistEstimate = JSetDist;
	else return(0);

	return(1); /* everything's OK, proceed */
}

static void dem_end( void )
{
#ifdef __TURBOC__
	farmemfree((char far *) orbit);
#else
	_ffree((void far *) orbit);
#endif
}

static int dem_pt( void )
{
	int		color;
	double	tempcolor, distance;

	if ((distance = DistEstimate()) < delta)	color = inside;
	else {
		if (mono) color = outside;
		else {
			tempcolor = 1 + (( distance/pixelwidth ));
			color = (int)(tempcolor / sqrt( tempcolor )) % colors;
		}
	}
	return(color);
}

/* 	Distance estimator for points near Mandelbrot set							*/
/* 	Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 	*/

static double MSetDist()
{
	int		iter = 0, i, flag;
	double	x, y, temp, dist;

	x = y = squared.x = squared.y = dist = orbit[0].x = orbit[0].y = 0.0;

	while ((iter <	maxit) && (squared.x + squared.y < BIG)) 
	{
		temp = squared.x - squared.y + init.x;
		iter++;
		orbit[iter].y = y = 2 * x * y + init.y;
		orbit[iter].x = x = temp;
		squared.x = x * x;
		squared.y = y * y;
	}
	if ( squared.x + squared.y > BIG ) 
	{
		deriv.x = deriv.y = 0.0;
		i = flag = 0;
		while ((i < iter) && (!flag)) 
		{
			temp = 2 * (orbit[i].x * deriv.x - orbit[i].y * deriv.y) + 1;
			deriv.y = 2 * (orbit[i].y * deriv.x + orbit[i].x * deriv.y);
			deriv.x = temp;
			if ( max( fabs( deriv.x ), fabs( deriv.y )) > XOVERFLOW ) flag++;
			i++;
		}
		if ( !flag )
			dist = log(squared.x + squared.y) * sqrt(squared.x + squared.y)
								 	/	sqrt(deriv.x * deriv.x 	+ deriv.y * deriv.y);
	}
	return(dist);
}

static double JSetDist()
{
	int		iter = 0, i;
	int		flag;
	double	x, y, temp,	dist;

	x = init.x;
	y = init.y;
	squared.x = x * x;
	squared.y = y * y;
	dist = orbit[0].x = orbit[0].y = 0.0;
	
	while ((iter <	maxit)  && (squared.x + squared.y < BIG) ) {
		temp = squared.x - squared.y + parm.x;
		y = 2 * x * y + parm.y;
		x = temp;
		squared.x = x * x;
		squared.y = y * y;
		iter++;
		orbit[ iter ].x = x;
		orbit[ iter ].y = y;
	}
	if ( squared.x + squared.y > BIG ) {
		deriv.x = deriv.y = 0.0;
		i = 0;
		flag = 0;
		while ((i < iter) && (!flag)) {
			temp = 2 * (orbit[i].x * deriv.x - orbit[i].y * deriv.y) + 1;
			deriv.y = 2 * (orbit[i].y * deriv.x + orbit[i].x * deriv.y);
			deriv.x = temp;
			if ( max( fabs( deriv.x ), fabs( deriv.y )) > XOVERFLOW ) flag++;
			i++;
		}
		if ( !flag )
			dist = log(squared.x + squared.y) * sqrt(squared.x + squared.y) / sqrt(deriv.x * deriv.x
																	+ deriv.y * deriv.y);
	}
	return(dist);
}

#define DEFAULTFILTER 1000	/* "Beauty of Fractals" recommends using 5000 
                               (p.25), but that seems unnecessary. Can 
                               override this value with a nonzero param1 */

#define SEED 0.66		/* starting value for population */

static void verhulst( double rate );

int far *verhulst_array;	
unsigned int filter_cycles;

int Bifurcation( void )
{
   unsigned long array_size;
   int row, column;

   array_size =  (YYdots + 1) * sizeof(int);

	if ( (verhulst_array = (int far *) farmemalloc(array_size)) == NULL)
	{
		printf("Better bail out, not enough heap for array of %d int\'s\n",
					YYdots );
		buzzer(2);
	}

	for (row = 0; row <= YYdots; row++)
		verhulst_array[row] = 0;

	mono = 0;
	if ( colors == 2 ) mono = 1;
	if ( mono )	{
		if ( inside ) {
			outside = 0;
			inside = 1;
		}
		else outside = 1;
	}

	if ((filter_cycles = parm.x) == 0) filter_cycles = DEFAULTFILTER;
   
	init.y = dy0[YYdots - 1]; /* Y value of bottom visible pixel */
	
	for (column = 0; column < XXdots; column++)
   {
		verhulst( dx0[column] );	 /* calculate array once per column */

		for (row = YYdots; row > 0; row--)
      {
         int color;
         color = verhulst_array[ row ];
			if ( color && mono ) color = inside;
			else if ( (!color) && mono ) color = outside;
			verhulst_array[ row ] = 0;
			(*plot)(column,row,color);
      }
      if(check_key()) 
      {
			farmemfree( (char far *)verhulst_array );
			return(-1);
      }
   }
	farmemfree( (char far *)verhulst_array );
   return(0);
}

static void verhulst( double rate )  /* P. F. Verhulst (1845) */
{
	double	OldPopulation, NewPopulation;
	unsigned	int pixel_row, counter;

	OldPopulation = SEED;

	for ( counter = 0; counter < filter_cycles; counter++ )
	{
		NewPopulation = ((1 + rate) * OldPopulation ) -
			( rate * OldPopulation * OldPopulation );
		OldPopulation = NewPopulation;
	}
	for ( counter = 0; counter < maxit; counter++ )
	{
		NewPopulation = ((1 + rate) * OldPopulation ) -
			( rate * OldPopulation * OldPopulation );

		OldPopulation = NewPopulation;

		/* assign population value to Y coordinate in pixels */
		pixel_row = YYdots - (int)((NewPopulation - init.y) / deltaY);

		/* if it's visible on the screen, save it in the column array */
		if ((pixel_row >= 0) && (pixel_row <= YYdots))	
							verhulst_array[ pixel_row ] ++;
	}
}

/* END Phil Wilson's Code */


/* -------------------------------------------------------------------- */
/*		Bailout Routines Macros  												*/
/* -------------------------------------------------------------------- */

#define FLOATBAILOUT()   \
   tempsqrx = sqr(new.x);\
   tempsqry = sqr(new.y);\
   if((magnitude = tempsqrx + tempsqry) >= rqlim) return(1);\
   old = new;

#define LONGBAILOUT()   \
   ltempsqrx = lsqr(lnew.x);\
   ltempsqry = lsqr(lnew.y);\
   lmagnitud = ltempsqrx + ltempsqry;\
   if (lmagnitud >= llimit || lmagnitud < 0 || labs(lnew.x) > llimit2\
         || labs(lnew.y) > llimit2) \
      return(1);\
   lold = lnew;

#define FLOATTRIGBAILOUT()  \
   if (fabs(old.y) >= rqlim2) return(1);

#define LONGTRIGBAILOUT()  \
   if(labs(lold.y) >= llimit2) return(1);

#define FLOATHTRIGBAILOUT()  \
   if (fabs(old.x) >= rqlim2) return(1);

#define LONGHTRIGBAILOUT()  \
   if(labs(lold.x) >= llimit2) return(1);

#define TRIG16CHECK(X)  \
      if(labs((X)) > l16triglim) return(1);

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
    
/* -------------------------------------------------------------------- */
/*		Fractal (once per iteration) routines			*/
/* -------------------------------------------------------------------- */
static double xt, yt, t2;
 
/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
cpower(struct complex *base, int exp, struct complex *result)
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
lcpower(struct lcomplex *base, int exp, struct lcomplex *result, int bitshift)
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
        lyt = multiply(lxt,lyt,bitshift)<<1;
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

z_to_the_z(struct complex *z, struct complex *out)
{
    static struct complex tmp1,tmp2;
    /* raises complex z to the z power */
    extern int errno;
    errno = 0;

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
    return(errno);
}
 
/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-fudge)) + lsqr(((z).y)))

#ifdef NEWTON
 
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
              if( distance(roots[i],old) < threshold)
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

#endif

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
   if((mod = modulus(denominator)) < FLT_MIN)
      return(1);
   conjugate(&denominator);
   complex_mult(numerator,denominator,pout);
   pout->x = pout->x/mod;
   pout->y = pout->y/mod;
   return(0);
}


lcomplex_mult(arg1,arg2,pz,bitshift)
struct lcomplex arg1,arg2,*pz;
int bitshift;
{
   overflow = 0;
   pz->x = multiply(arg1.x,arg2.x,bitshift) - multiply(arg1.y,arg2.y,bitshift);
   pz->y = multiply(arg1.x,arg2.y,bitshift) + multiply(arg1.y,arg2.x,bitshift);
   return(overflow);
}

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
             if( pMPcmp(MPdistance(MPCroots[i],mpcold),mpthreshold) < 0)
             { 
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

JuliaFractal() 
{
   /* used for C prototype of fast integer math routines for classic
      Mandelbrot and Julia */ 
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = (multiply(lold.x, lold.y, bitshift) << 1) + longparm->y;
   LONGBAILOUT();
   return(0);
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
   ltempsqrx = lold.x  - multiply(lold.x, lold.x, bitshift)
                 + multiply(lold.y, lold.y, bitshift);
   ltempsqry = lold.y
                 - (multiply(lold.y, lold.x, bitshift)  << 1);
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
   if( lold.y > ltemp.y)		/* if old.y > .5 */
      lnew.y = lnew.y - ltemp.x;	/* new.y = 2 * old.y - 1 */
   else if(lold.x > ltemp.y)	/* if old.x > .5 */
      lnew.x = lnew.x - ltemp.x;	/* new.x = 2 * old.x - 1 */
   /* end barnsley code */
   LONGBAILOUT();
   return(0);
}

SierpinskiFPFractal() 
{
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */ 

   new.x = 2 * old.x;
   new.y = 2 * old.y;
   if( old.y > .5)
      new.y = 2 * old.y - 1;
   else if (old.x > .5)
      new.x = 2 * old.x - 1;

   /* end barnsley code */
   FLOATBAILOUT();
   return(0);
}

static long lcosx, lcoshx, lsinx, lsinhx;
static long lcosy, lcoshy, lsiny, lsinhy;

LongLambdasineFractal() 
{
   LONGTRIGBAILOUT();

   SinCos086(lold.x, &lsinx, &lcosx);
   SinhCosh086(lold.y, &lsinhy, &lcoshy);

   ltemp.x = multiply(lsinx,        lcoshy,   bitshift);
   ltemp.y = multiply(lcosx,        lsinhy,   bitshift);

   lnew.x  = multiply(longparm->x, ltemp.x, bitshift)
           - multiply(longparm->y, ltemp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltemp.y, bitshift)
           + multiply(longparm->y, ltemp.x, bitshift);
   lold = lnew;
   return(0);
}

LongLambdacosFractal() 
{
   LONGTRIGBAILOUT();

   SinCos086(lold.x, &lsinx, &lcosx);
   SinhCosh086(lold.y, &lsinhy, &lcoshy);

   ltemp.x = multiply(lcosx,        lcoshy,   bitshift);
   ltemp.y = multiply(lsinx,        lsinhy,   bitshift);

   lnew.x  = multiply(longparm->x, ltemp.x, bitshift)
           - multiply(longparm->y, ltemp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltemp.y, bitshift)
           + multiply(longparm->y, ltemp.x, bitshift);
   lold = lnew;
   return(0);
}

LongLambdasinhFractal() 
{
   LONGHTRIGBAILOUT();

   SinCos086  (lold.y, &lsiny,  &lcosy);
   SinhCosh086(lold.x, &lsinhx, &lcoshx);

   ltemp.x = multiply(lcosy,        lsinhx,   bitshift);
   ltemp.y = multiply(lsiny,        lcoshx,   bitshift);

   lnew.x  = multiply(longparm->x, ltemp.x, bitshift)
           - multiply(longparm->y, ltemp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltemp.y, bitshift)
           + multiply(longparm->y, ltemp.x, bitshift);
   lold = lnew;
   return(0);
}

LongLambdacoshFractal() 
{
   LONGHTRIGBAILOUT();

   SinCos086(lold.y, &lsiny, &lcosy);
   SinhCosh086(lold.x, &lsinhx, &lcoshx);

   ltemp.x = multiply(lcosy,        lcoshx,   bitshift);
   ltemp.y = multiply(lsiny,        lsinhx,   bitshift);

   lnew.x  = multiply(longparm->x, ltemp.x, bitshift)
           - multiply(longparm->y, ltemp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltemp.y, bitshift)
           + multiply(longparm->y, ltemp.x, bitshift);
   lold = lnew;
   return(0);
}

LambdasineFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   FLOATTRIGBAILOUT();

   /* calculate sin(z) */
   FPUsincos  (&old.x,&sinx,&cosx);
   FPUsinhcosh(&old.y,&sinhy,&coshy);
   tmp.x = sinx*coshy;
   tmp.y = cosx*sinhy;

   /*multiply by lamda */
   new.x = floatparm->x * tmp.x - floatparm->y * tmp.y;
   new.y = floatparm->y * tmp.x + floatparm->x * tmp.y;
   old = new;
   return(0);
}

LambdacosineFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   FLOATTRIGBAILOUT();

   /* calculate cos(z) */
   FPUsincos  (&old.x,&sinx,&cosx);
   FPUsinhcosh(&old.y,&sinhy,&coshy);
   tmp.x = cosx*coshy;
   tmp.y = sinx*sinhy;

   /*multiply by lamda */
   new.x = floatparm->x*tmp.x - floatparm->y*tmp.y;
   new.y = floatparm->y*tmp.x + floatparm->x*tmp.y;
   old = new;
   return(0);
}

LambdasinhFractal() 
{
   FLOATHTRIGBAILOUT();

   /* calculate sinh(z) */

   FPUsincos  (&old.y,&siny,&cosy);
   FPUsinhcosh(&old.x,&sinhx,&coshx);
   tmp.x = sinhx*cosy;
   tmp.y = coshx*siny;
   
   /*multiply by lamda */
   new.x = floatparm->x*tmp.x - floatparm->y*tmp.y;
   new.y = floatparm->y*tmp.x + floatparm->x*tmp.y;
   old = new;
   return(0);
}

LambdacoshFractal() 
{
   FLOATHTRIGBAILOUT();

   /* calculate cosh(z) */
   FPUsincos  (&old.y,&siny,&cosy);
   FPUsinhcosh(&old.x,&sinhx,&coshx);
   tmp.x = coshx*cosy;
   tmp.y = sinhx*siny;

   /*multiply by lamda */
   new.x = floatparm->x*tmp.x - floatparm->y*tmp.y;
   new.y = floatparm->y*tmp.x + floatparm->x*tmp.y;
   old = new;
   return(0);
}

LambdaexponentFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   /* calculate exp(z) */
   if (fabs(old.y) >= 1.0e8) return(1); /* TLOSS  errors */
   if (fabs(old.x) >= 6.4e2) return(1); /* DOMAIN errors */

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

   /* calculate exp(z) */
   if (labs(lold.y) >= 1000L<<bitshift) return(1); /* TLOSS  errors */
   if (labs(lold.x) >=  8L<<bitshift) return(1); /* DOMAIN errors */

   SinCos086  (lold.y, &lsiny,  &lcosy);

   if (lold.x >= llimit && lcosy >= 0L) return(1);
   ltmp1 = Exp086(lold.x);

   ltemp.x = multiply(ltmp1,       lcosy,   bitshift);
   ltemp.y = multiply(ltmp1,       lsiny,   bitshift);

   lnew.x  = multiply(longparm->x, ltemp.x, bitshift)
           - multiply(longparm->y, ltemp.y, bitshift);
   lnew.y  = multiply(longparm->x, ltemp.y, bitshift)
           + multiply(longparm->y, ltemp.x, bitshift);
   lold = lnew;
   return(0);
}

FloatSinexponentFractal() 
{
   /* another Scientific American biomorph type */
   /* z(n+1) = e**z(n) + sin(z(n)) + C */

   if (fabs(old.x) >= 6.4e2) return(1); /* DOMAIN errors */
   tmpexp = exp(old.x);
   
   FPUsincos  (&old.y,&siny,&cosy);
   FPUsincos  (&old.x,&sinx,&cosx);
   FPUsinhcosh(&old.y,&sinhy,&coshy);

   /*new = e**old      + sin(old)   + C  */     
   new.x = tmpexp*cosy + sinx*coshy + floatparm->x;
   new.y = tmpexp*siny + cosx*sinhy + floatparm->y;

   FLOATBAILOUT();
   return(0);
}


LongSinexponentFractal() 
{
   /* calculate exp(z) */
   
   /* domain check for fast transcendental functions */
   TRIG16CHECK(lold.x);
   TRIG16CHECK(lold.y);
   
   ltmp1 = Exp086(lold.x);
   SinCos086  (lold.x, &lsinx,  &lcosx);
   SinCos086  (lold.y, &lsiny,  &lcosy);
   SinhCosh086(lold.y, &lsinhy, &lcoshy);

   lnew.x = multiply(ltmp1,       lcosy,   bitshift) +
            multiply(lsinx,       lcoshy,  bitshift) + longparm->x;
    
   lnew.y = multiply(ltmp1,       lsiny,   bitshift);
            multiply(lcosx,       lsinhy,  bitshift) + longparm->y;
   lold = lnew;
   LONGBAILOUT();
   return(0);
}

MarksLambdaFractal() 
{
   /* Mark Peterson's variation of "lambda" function */

   /* Z1 = (C^(exp-1) * Z**2) + C */
   ltemp.x = ltempsqrx - ltempsqry;
   ltemp.y = multiply(lold.x ,lold.y ,bitshift)<<1;

   lnew.x = multiply(lcoefficient.x, ltemp.x, bitshift)
        - multiply(lcoefficient.y, ltemp.y, bitshift) + longparm->x;
   lnew.y = multiply(lcoefficient.x, ltemp.y, bitshift)
        + multiply(lcoefficient.y, ltemp.x, bitshift) + longparm->y;

   LONGBAILOUT();
   return(0);
}

UnityFractal() 
{
   /* brought to you by Mark Peterson - you won't find this in any fractal
      books unless they saw it here first - Mark invented it! */

   XXOne = multiply(lold.x, lold.x, bitshift) + multiply(lold.y, lold.y, bitshift);
   if((XXOne > FgTwo) || (labs(XXOne - FgOne) < delx))
      return(1);
   lold.y = multiply(FgTwo - XXOne, lold.x, bitshift);
   lold.x = multiply(FgTwo - XXOne, lold.y, bitshift);
   return(0);
}

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
   lnew.y = (multiply(lold.x, lold.y, bitshift) << 1);
   LONGBAILOUT();

   /* then, compute ((x + iy)**2)**2 + lambda */
   lnew.x  = ltempsqrx - ltempsqry + longparm->x;
   lnew.y = (multiply(lold.x, lold.y, bitshift) << 1) + longparm->y;
   LONGBAILOUT();
   return(0);
}

floatZtozpluszpwrFractal() 
{
   cpower(&old,(int)param[2],&new);
   old = ComplexPower(old,old);
/*
   cpower(&old,5,&new);
   z_to_the_z(&old,&old);
*/
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

SinZsquaredFractal() 
{
   /* From Scientific American, July 1989 */
   /* A Biomorph                          */
   /* z(n+1) = sin(z(n))+z(n)**2+C        */

   SinCos086  (lold.x, &lsinx,  &lcosx);
   SinhCosh086(lold.y, &lsinhy, &lcoshy);

   lnew.x = multiply(lsinx,        lcoshy,   bitshift);
   lnew.y = multiply(lcosx,        lsinhy,   bitshift);

   lnew.x += ltempsqrx - ltempsqry + longparm->x;
   lnew.y += (multiply(lold.x, lold.y, bitshift) << 1) + longparm->y;
   LONGBAILOUT();
   return(0);
}

SinZsquaredfpFractal() 
{
   /* From Scientific American, July 1989 */
   /* A Biomorph                          */
   /* z(n+1) = sin(z(n))+z(n)**2+C        */

   FPUsincos  (&old.x,&sinx,&cosx);
   FPUsinhcosh(&old.y,&sinhy,&coshy);
   new.x = tempsqrx - tempsqry + floatparm->x + sinx*coshy;
   new.y = 2.0 * old.x * old.y + floatparm->y + cosx*sinhy;
   FLOATBAILOUT();
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
   return(0);
}

LPopcornFractal()
{
   static long O5 = (long)(.05*(1L<<16));
   extern int row;
   ltemp = lold;
   ltemp.x *= 3L;
   ltemp.y *= 3L;
   LTRIGARG(ltemp.x);
   LTRIGARG(ltemp.y);
   SinCos086(ltemp.x,&lsinx,&lcosx);
   SinCos086(ltemp.y,&lsiny,&lcosy);
   ltemp.x = divide(lsinx,lcosx,bitshift) + lold.x;
   ltemp.y = divide(lsiny,lcosy,bitshift) + lold.y;
   LTRIGARG(ltemp.x);
   LTRIGARG(ltemp.y);
   SinCos086(ltemp.x,&lsinx,&lcosx);
   SinCos086(ltemp.y,&lsiny,&lcosy);
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
   struct complex temp;
   temp.x = tempsqrx - tempsqry;
   temp.y = 2*old.x*old.y;
/*   FPUcplxmul(&old, &old, &temp); */
   FPUcplxmul(&temp, &Coefficient, &new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   FLOATBAILOUT();
   return(0);
}

NotImplementedFractal() /* should never get HERE!! */
{		
   /*
   This fractal found in the classic "Table of Even Prime Numbers", the
   abridged edition, by Vacuous E. Void. */
   return(1);
}

/* -------------------------------------------------------------------- */
/*		Initialization (once per pixel) routines						*/
/* -------------------------------------------------------------------- */

#if 0
/* this code translated to asm - lives in newton.asm */
/* transform points with reciprocal function */
void invertz1(struct complex *z)
{
   z->x = dx0[col];
   z->y = dy0[row];
   z->x -= f_xcenter; z->y -= f_ycenter;  /* Normalize values to center of circle */

   tempsqrx = sqr(z->x) + sqr(z->y);  /* Get old radius */
   if(fabs(tempsqrx) > FLT_MIN)
      tempsqrx = f_radius / tempsqrx;
   else
      tempsqrx = FLT_MAX;   /* a big number, but not TOO big */
   z->x *= tempsqrx;      z->y *= tempsqrx;      /* Perform inversion */
   z->x += f_xcenter; z->y += f_ycenter; /* Renormalize */
}
#endif

void long_julia_per_pixel()
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
      lold.x = lx0[col];
      lold.y = ly0[row];
   }
}

void long_mandel_per_pixel()
{
   /* integer mandel types */
   /* barnsleym1 */
   /* barnsleym2 */
   linit.x = lx0[col];

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

   lold.x = linit.x + llambda.x;   /* initial pertubation of parameters set */
   lold.y = linit.y + llambda.y;

}
void julia_per_pixel()
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
            old.x = 8;  /* value to bail out in one iteration */
            old.y = 8;
         }
      if(bitshift >  24)
         if (sqr(old.x)+sqr(old.y) >= 4.0)
         {
            old.x = 2;  /* value to bail out in one iteration */
            old.y = 2;
         }
       
      /* convert to fudged longs */
      lold.x = old.x*fudge;
      lold.y = old.y*fudge;
   }
   else
   {
      lold.x = lx0[col];
      lold.y = ly0[row];
   }

   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
}

void mandel_per_pixel()
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
      linit.x = lx0[col];
   if(bof_pp60_61)
   {
      /* kludge to match "Beauty of Fractals" picture since we start
         Mandelbrot iteration with init rather than 0 */
      lold.x = llambda.x; /* initial pertubation of parameters set */
      lold.y = llambda.y;
      color = -1;
   }
   else
   {   
      lold.x = linit.x + llambda.x; /* initial pertubation of parameters set */
      lold.y = linit.y + llambda.y;
   } 
   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
}


void marksmandel_per_pixel()
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
      linit.x = lx0[col];
   
   lold.x = linit.x + llambda.x; /* initial pertubation of parameters set */
   lold.y = linit.y + llambda.y;

   if(c_exp > 3)
      lcpower(&lold,c_exp-1,&lcoefficient,bitshift);
   else if(c_exp == 3) {
      lcoefficient.x = multiply(lold.x, lold.x, bitshift)
         - multiply(lold.y, lold.y, bitshift);
      lcoefficient.y = multiply(lold.x, lold.y, bitshift) << 1;
   }
   else if(c_exp == 2)
      lcoefficient = lold;
   else if(c_exp < 2) {
      lcoefficient.x = 1L << bitshift;
      lcoefficient.y = 0L;
   }

   ltempsqrx = multiply(lold.x, lold.x, bitshift);
   ltempsqry = multiply(lold.y, lold.y, bitshift);
}


void mandelfp_per_pixel()
{
   /* floating point mandelbrot */
   /* mandelfp */

   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col];
   if(bof_pp60_61)
   {
      /* kludge to match "Beauty of Fractals" picture since we start
         Mandelbrot iteration with init rather than 0 */
      old.x = parm.x; /* initial pertubation of parameters set */
      old.y = parm.y;
      color = -1;
   }
   else
   {   
      old.x = init.x + parm.x; /* initial pertubation of parameters set */
      old.y = init.y + parm.y;
   } 

   tempsqrx = sqr(old.x);  /* precalculated value for regular Mandelbrot */
   tempsqry = sqr(old.y);
}

void juliafp_per_pixel()
{
   /* floating point julia */
   /* juliafp */
   if(invert)
      invertz2(&old);
   else
   {
     old.x = dx0[col];
     old.y = dy0[row];
   }   
   tempsqrx = sqr(old.x);  /* precalculated value for regular Julia */
   tempsqry = sqr(old.y);
}
void MPCjulia_per_pixel()
{
   /* floating point julia */
   /* juliafp */
   if(invert)
      invertz2(&old);
   else
   {
     old.x = dx0[col];
     old.y = dy0[row];
   }   
   mpcold.x = pd2MP(old.x);
   mpcold.y = pd2MP(old.y);
}

void othermandelfp_per_pixel()
{
   /* mandelsine */
   /* mandelcos  */
   /* mandelexp  */
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col];
   old.x = init.x + parm.x; /* initial pertubation of parameters set */
   old.y = init.y + parm.y;
}

void otherjuliafp_per_pixel()
{
   /* lambdasine */
   /* lambdacos  */
   /* lambdaexp  */
   /* mandelsine */
   /* mandelcos  */
   /* mandelexp  */
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dx0[col];
      old.y = dy0[row];
   }   
}

int MarksCplxMandperp(void) 
{
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col];
   old.x = init.x + parm.x; /* initial pertubation of parameters set */
   old.y = init.y + parm.y;
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   Coefficient = ComplexPower(init, pwr);
   return(1);
}

/* -------------------------------------------------------------------- */
/*		Setup (once per fractal image) routines			*/
/* -------------------------------------------------------------------- */

MandelSetup()		/* Mandelbrot Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0 
          && forcesymmetry == 999 && biomorph == -1 && bof_pp60_61 == 0)
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
          && forcesymmetry == 999 && biomorph == -1)
      calctype = calcmand; /* the normal case - use CALCMAND */
   else
   {
      /* special case: use the main processing loop */
      calctype = StandardFractal;
      longparm = &llambda;
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
   roverd       = (double)root / (double)degree;
   d1overd      = (double)(degree - 1) / (double)degree;
   maxcolor     = 0;
   threshold    = .3*PI/degree; /* less than half distance between roots */
   if (fractype == MPNEWTON || fractype == MPNEWTBASIN) {
      mproverd     = pd2MP(roverd);
      mpd1overd    = pd2MP(d1overd);
      mpthreshold  = pd2MP(threshold);
      mpone        = pd2MP(1.0);
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
      basin = 1;
      if(degree > 16)
      {
	     if((roots=(struct complex *)malloc(degree*sizeof(struct complex)))==NULL)
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
      setsymmetry(XYAXIS);
   else
      setsymmetry(XAXIS);

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
   periodicitycheck = 0;		/* disable periodicity checks */
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
      setsymmetry(XYAXIS_NOPARM);         

   pwr.x = param[2] - 1.0;
   pwr.y = param[3];
   
   floatparm = &init;
   return(1);
}
 
JuliafpSetup()
{
   c_exp = param[2];
   if(fractype==FPJULIAZPOWER && c_exp < 1)
      c_exp = 1;
   if(fractype==FPJULIAZPOWER && c_exp & 1) /* odd exponents */
      setsymmetry(NOSYM);
   floatparm = &parm;
   if(fractype==COMPLEXMARKSJUL)
   {
      pwr.x = param[2] - 1.0;
      pwr.y = param[3];
      Coefficient = ComplexPower(*floatparm, pwr);
   }
   return(1);
}

MandellongSetup()
{
   c_exp = param[2];
   
   if(fractype==LMANDELZPOWER && c_exp < 1) 
      c_exp = 1;
   if((fractype==MARKSMANDEL   && !(c_exp & 1)) ||
      (fractype==LMANDELZPOWER && c_exp & 1))
      setsymmetry(XYAXIS_NOPARM);    /* odd exponents */      
  longparm = &linit;
   return(1);
}
JulialongSetup()
{
   c_exp = param[2];
   if(fractype==LJULIAZPOWER && c_exp < 1)
      c_exp = 1;
   if(fractype==LJULIAZPOWER && c_exp & 1) /* odd exponents */
      setsymmetry(NOSYM);         
   longparm = &llambda;
   return(1);
}

MarksJuliaSetup()
{
   c_exp = param[2];
   longparm = &llambda;
   lold = *longparm;
   if(c_exp > 2)
      lcpower(&lold,c_exp,&lcoefficient,bitshift);
   else if(c_exp == 2)
   {
      lcoefficient.x = multiply(lold.x,lold.x,bitshift) - multiply(lold.y,lold.y,bitshift);
      lcoefficient.y = multiply(lold.x,lold.y,bitshift) << 1;
   }
   else if(c_exp < 2)
      lcoefficient = lold;
   return(1);
}

SierpinskiSetup()
{
   /* sierpinski */
   periodicitycheck = 0;		/* disable periodicity checks */
   ltemp.x = 1; ltemp.x = ltemp.x << bitshift;	/* ltemp.x = 1 */
   ltemp.y = ltemp.x >> 1;			/* ltemp.y = .5 */
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
   return(1);
}
/* parameter descriptions */
/* for Mandelbrots */
static char realz0[] = "Real Portion of Z(0)";
static char imagz0[] = "Imaginary Portion of Z(0)";

/* for Julias */
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

/* for Newtons */
static char newtdegree[] = "Polynomial Degree ( > 2 )";

/* for MarksMandel/Julia */
static char exponent[] = "Parameter Exponent (  > 0 )";

/* for Complex Newton */
static char realroot[]   = "Real part of Root";
static char imagroot[]   = "Imag part of Root";
static char realdegree[] = "Real part of Degree";
static char imagdegree[] = "Imag part of Degree";

/* for Lorenz */
static char timestep[]     = "Time Step";

/* for formula */
static char p1real[] = "Real portion of p1";
static char p2real[] = "Real portion of p2";
static char p1imag[] = "Imaginary portion of p1";
static char p2imag[] = "Imaginary portion of p2";

/* bailout defines */
#define FTRIGBAILOUT 2500.0
#define LTRIGBAILOUT   64.0
#define STDBAILOUT 	    4.0

struct fractalspecificstuff fractalspecific[] = 
{
   /*
     fractal name and parameters (text strings)
     xmin  xmax  ymin  ymax int tojulia   tomandel symmetry 
   |------|-----|-----|-----|--|--------|---------|--------|
     orbit fnct     per_pixel fnct  per_image fnct  calctype fcnt    bailout
   |---------------|---------------|---------------|----------------|-------|
   */

   "mandel",      realz0, imagz0,"","",0,0,0,0,
   -2.5,  1.5, -1.5,  1.5, 1, JULIA,     NOFRACTAL, MANDELFP, XAXIS_NOPARM, 
   JuliaFractal,  mandel_per_pixel,MandelSetup,    calcmand,        STDBAILOUT,

   "julia",       realparm, imagparm,"","",0.3,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL, JULIAFP,  ORIGIN, 
   JuliaFractal,   julia_per_pixel, JuliaSetup,    calcmand,        STDBAILOUT,

   "*newtbasin",   newtdegree,"", "","",3,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN,   NOSYM, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "lambda",      realparm, imagparm,"","",0.85,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP,  NOSYM, 
   LambdaFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,
   
   "*mandel",    realz0, imagz0,"","",0,0,0,0,
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,   NOFRACTAL, MANDEL,  XAXIS_NOPARM, 
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,
   
   "*newton",      newtdegree,"", "","",3,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTON,   XAXIS, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "*julia",     realparm, imagparm,"","",0.3,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP, JULIA,  ORIGIN, 
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)","","","",2,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,           NULL,   StandaloneSetup,      plasma,          0.0,
   
   "*lambdasine",  realparm, imagparm,"","",1.0,0.4,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELSINE, LLAMBDASINE, PI_SYM, 
   LambdasineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,
   
   "*lambdacos",   realparm, imagparm,"","",1.5,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELCOS, LLAMBDACOS,  PI_SYM, 
   LambdacosineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "*lambdaexp",   realparm, imagparm,"","",0.12,2.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELEXP, LLAMBDAEXP,  XAXIS, 
   LambdaexponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "test",        "(testpt Param #1)","(testpt param #2)","(testpt param #3)", "(testpt param #4)",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, test,    STDBAILOUT,
 
  "sierpinski",  "","","","",0,0,0,0,
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP,   NOSYM, 
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,

  "barnsleym1",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL, BARNSLEYM1FP,  XYAXIS_NOPARM, 
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
 
  "barnsleyj1",  realparm, imagparm,"","",0.6,1.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP,  ORIGIN, 
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "barnsleym2",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL, BARNSLEYM2FP,  YAXIS_NOPARM, 
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
   
   "barnsleyj2",  realparm, imagparm,"","",0.6,1.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP,  ORIGIN, 
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,
   
   "*mandelsine",  realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDASINE,NOFRACTAL, LMANDELSINE, XYAXIS_NOPARM, 
   LambdasineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelcos",   realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDACOS, NOFRACTAL, LMANDELCOS, XYAXIS_NOPARM, 
   LambdacosineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelexp",   realz0, imagz0,"","",0,0,0,0,
   -4.0,  4.0, -3.0,  3.0, 0, LAMBDAEXP, NOFRACTAL, LMANDELEXP,   0, 
   LambdaexponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "mandellambda",realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, LAMBDA,    NOFRACTAL, MANDELLAMBDAFP,  XAXIS_NOPARM, 
   LambdaFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksmandel", realz0, imagz0, exponent,"",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL, NOFRACTAL,  NOSYM,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksjulia", realparm, imagparm, exponent,"",0.1,0.9,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL, NOFRACTAL,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,STDBAILOUT,

   "unity",       "","","","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   XYAXIS, 
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,0.0,
   
   "mandel4",      realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,     NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  STDBAILOUT,

   "julia4",       realparm, imagparm,"","",0.6,0.55,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4, NOFRACTAL, ORIGIN, 
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,    STDBAILOUT,

   "ifs",        "","","","",0,0,0,0,
   -8.0,  8.0, -1.0, 11.0, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, ifs,    0.0,

   "ifs3d",        "","","","",0,0,0,0,
   -11.0,  11.0, -11.0, 11.0, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,      StandaloneSetup, ifs3d,    0.0,

   "barnsleym3",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL, BARNSLEYM3FP,  XAXIS_NOPARM, 
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
   
   "barnsleyj3",  realparm, imagparm,"","",0.1,0.36,0,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP,  XAXIS, 
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "demm",    realz0, imagz0,"","",0,0,0,0,
   -2.5,  1.5, -1.5,  1.5, 0, DEMJ,   NOFRACTAL, NOFRACTAL,   XAXIS_NOPARM, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,STDBAILOUT,

   "demj",     realparm, imagparm,"","",0.3,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, DEMM, NOFRACTAL,    ORIGIN, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,STDBAILOUT,

   "bifurcation",     "", "","","",0,0,0,0,
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,				NULL, 		StandaloneSetup, Bifurcation,STDBAILOUT,

   "*mandelsinh",  realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDASINH,NOFRACTAL, LMANDELSINH, XYAXIS_NOPARM, 
   LambdasinhFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*lambdasinh",  realparm, imagparm,"","",1.0,0.7,0,0,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, MANDELSINH, LLAMBDASINH, PI_SYM, 
   LambdasinhFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelcosh",   realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDACOSH, NOFRACTAL, LMANDELCOSH, XYAXIS_NOPARM, 
   LambdacoshFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,
   
   "*lambdacosh",   realparm, imagparm,"","",1.0,0.7,0,0,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, MANDELCOSH, LLAMBDACOSH, PI_SYM, 
   LambdacoshFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "mandelsine",realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDASINE, NOFRACTAL, MANDELSINE, XYAXIS_NOPARM, 
   LongLambdasineFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdasine",      realparm, imagparm,"","",1.0,0.4,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELSINE, LAMBDASINE,PI_SYM, 
   LongLambdasineFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelcos",realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDACOS, NOFRACTAL, MANDELCOS, XYAXIS_NOPARM, 
   LongLambdacosFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdacos",      realparm, imagparm,"","",1.5,0.6,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELCOS, LAMBDACOS, PI_SYM, 
   LongLambdacosFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelsinh",realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDASINH, NOFRACTAL, MANDELSINH, XYAXIS_NOPARM, 
   LongLambdasinhFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdasinh",      realparm, imagparm,"","",1.0,0.7,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELSINH, LAMBDASINH, ORIGIN, 
   LongLambdasinhFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelcosh",realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDACOSH,    NOFRACTAL, MANDELCOSH,  XYAXIS_NOPARM, 
   LongLambdacoshFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdacosh",      realparm, imagparm,"","",1.0,0.7,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELCOSH, LAMBDACOSH, ORIGIN, 
   LongLambdacoshFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mansinzsqrd",      realz0, imagz0,"","",0,0,0,0,
   -2.5,  1.5, -1.5,  1.5, 16, LJULSINZSQRD,  NOFRACTAL, FPMANSINZSQRD, XAXIS_NOPARM, 
   SinZsquaredFractal,mandel_per_pixel,MandellongSetup,StandardFractal, STDBAILOUT,

   "julsinzsqrd",       realparm, imagparm,"","",-0.5,0.5,0,0,
   -2.0,  2.0, -1.5,  1.5, 16, NOFRACTAL, LMANSINZSQRD, FPJULSINZSQRD,  NOSYM, 
   SinZsquaredFractal,julia_per_pixel, JulialongSetup,StandardFractal, STDBAILOUT,

   "*mansinzsqrd",    realz0, imagz0,"","",0,0,0,0,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULSINZSQRD,   NOFRACTAL, LMANSINZSQRD, XAXIS_NOPARM, 
   SinZsquaredfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal, STDBAILOUT,

   "*julsinzsqrd",     realparm, imagparm,"","",-0.5,0.5,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANSINZSQRD, LJULSINZSQRD,   NOSYM, 
   SinZsquaredfpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal, STDBAILOUT,

   "mandelexp",realz0, imagz0,"","",0,0,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, LLAMBDAEXP,    NOFRACTAL,  MANDELEXP, XAXIS, 
   LongLambdaexponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdaexp",      realparm, imagparm,"","",0.12,2.1,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELEXP, LAMBDAEXP, XAXIS, 
   LongLambdaexponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "manzpower", realz0, imagz0, exponent,"",0,0,2,0,
   -2.0,  2.0, -1.5,  1.5, 1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER,  XAXIS,
   longZpowerFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julzpower", realparm, imagparm, exponent,"",0.3,0.6,2,0,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER,   ORIGIN,
   longZpowerFractal,julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "*manzpower",    realz0, imagz0, exponent,"",0,0,2,0,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULIAZPOWER,   NOFRACTAL, LMANDELZPOWER,  XAXIS_NOPARM, 
   floatZpowerFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julzpower",     realparm, imagparm, exponent,"",0.3,0.6,2,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER,  ORIGIN, 
   floatZpowerFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "manzzpwr",    realz0, imagz0, exponent,"",0,0,2,0,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULZTOZPLUSZPWR,   NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   floatZtozpluszpwrFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "julzzpwr",     realparm, imagparm, exponent,"",-0.3,0.3,2,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL,  NOSYM, 
   floatZtozpluszpwrFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "mansinexp",realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 16, LJULSINEXP,    NOFRACTAL,  FPMANSINEXP, XAXIS_NOPARM, 
   LongSinexponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julsinexp",      realparm, imagparm,"","",0,0,0,0,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANSINEXP,FPJULSINEXP, NOSYM,
   LongSinexponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*mansinexp",   realz0, imagz0,"","",0,0,0,0,
   -8.0,  8.0, -6.0,  6.0, 0, FPJULSINEXP, NOFRACTAL, LMANSINEXP,   XAXIS_NOPARM, 
   FloatSinexponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julsinexp",   realparm, imagparm,"","",0,0,0,0,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANSINEXP, LJULSINEXP,   NOSYM, 
   FloatSinexponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*popcorn", "", "", "","",0,0,0,0,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORN,  NOPLOT, 
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "popcorn", "", "", "","",0,0,0,0,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORN,  NOPLOT, 
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*lorenz",timestep,"a","b", "c",.02,5,15,1,
   -15,  15, 0, 30, 0, NOFRACTAL, NOFRACTAL, LLORENZ,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, floatlorenz,    0.0,

   "lorenz",timestep,"a","b", "c",.02,5,15,1,
   -15,  15, 0, 30, 16, NOFRACTAL, NOFRACTAL, FPLORENZ,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, longlorenz,    0.0,

   "lorenz3d",timestep,"a","b", "c",.02,5,15,1,
   -30.0,  30.0,  -30.0,   30.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,      StandaloneSetup, lorenz3dlong,    0.0,

   "newton",      newtdegree,"", "","",3,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTON,   XAXIS, 
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "newtbasin",      newtdegree,"", "","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTBASIN,   NOSYM, 
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "complexnewton", realdegree, imagdegree, realroot, imagroot,3,0,1,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   ComplexNewton, otherjuliafp_per_pixel,  ComplexNewtonSetup, StandardFractal,0.0,

   "complexbasin", realdegree, imagdegree, realroot, imagroot,3,0,1,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   ComplexBasin, otherjuliafp_per_pixel,  ComplexNewtonSetup,  StandardFractal, 0.0,

   "cmplxmarksmand", realz0, imagz0, realdegree, imagdegree,0,0,1,0,
   -2.0,  2.0, -1.5,  1.5, 0, COMPLEXMARKSJUL, NOFRACTAL, NOFRACTAL,   NOSYM,
   MarksCplxMand, MarksCplxMandperp, MandelfpSetup, StandardFractal, STDBAILOUT,

   "cmplxmarksjul", realparm, imagparm, realdegree, imagdegree,0.3,0.6,1,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, COMPLEXMARKSMAND, NOFRACTAL,   NOSYM,
   MarksCplxMand, juliafp_per_pixel, JuliafpSetup, StandardFractal, STDBAILOUT,

   "formula", p1real, p1imag, p2real, p2imag, 0,0,0,0,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, FFORMULA, SETUP_SYM,
   Formula, form_per_pixel, intFormulaSetup, StandardFractal, 0,

   "*formula", p1real, p1imag, p2real, p2imag, 0,0,0,0,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, FORMULA, SETUP_SYM,
   Formula, form_per_pixel, fpFormulaSetup, StandardFractal, 0,

  "*sierpinski",  "","","","",0,0,0,0,
   -0.9,  1.7, -0.9,  1.7, 0, NOFRACTAL, NOFRACTAL, SIERPINSKI,   NOSYM, 
   SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,StandardFractal,127.0,

   "*lambda",      realparm, imagparm,"","",0.85,0.6,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA,  NOSYM, 
   LambdaFPFractal,   otherjuliafp_per_pixel, JuliafpSetup,  StandardFractal,STDBAILOUT,

  "*barnsleym1",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ1FP,NOFRACTAL, BARNSLEYM1,  XYAXIS_NOPARM, 
   Barnsley1FPFractal, othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

  "*barnsleyj1",  realparm, imagparm,"","",0.6,1.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1,  ORIGIN, 
   Barnsley1FPFractal, otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym2",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ2FP,NOFRACTAL, BARNSLEYM2,  YAXIS_NOPARM, 
   Barnsley2FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj2",  realparm, imagparm,"","",0.6,1.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2,  ORIGIN, 
   Barnsley2FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym3",  realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3,  XAXIS_NOPARM, 
   Barnsley3FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj3",  realparm, imagparm,"","",0.6,1.1,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3,  XAXIS, 
   Barnsley3FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*mandellambda",realz0, imagz0,"","",0,0,0,0,
   -2.0,  2.0, -1.5,  1.5, 0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA,  XAXIS_NOPARM, 
   LambdaFPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "julibrot", "","","","", -.83, -.83, .25, -.25,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal, STDBAILOUT,

   NULL, NULL, NULL, NULL, NULL,0,0,0,0, /* marks the END of the list */
   0,  0, 0,  0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL, NULL, NULL, NULL,0
};
	