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
   value. These have names live "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbit_calc. EVERY 
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'newx' and 
   'newy', for floating point fractals in 'new.x' and 'new.y'.
   
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
#include <math.h>
#include <limits.h>
#include "fractint.h"
#include "fmath.h"

/* defines should match fractalspecific array indices */

/* first values defined in fractype.h for historical reasons */

#define LAMBDASINE   8 
#define LAMBDACOS    9 
#define LAMBDAEXP    10
#define TEST         11
#define SIERPINSKI   12
#define BARNSLEYM1   13
#define BARNSLEYJ1   14
#define BARNSLEYM2   15
#define BARNSLEYJ2   16
#define MANDELSINE   17
#define MANDELCOS    18
#define MANDELEXP    19
#define MANDELLAMBDA 20
#define MARKSMANDEL  21
#define MARKSJULIA   22
#define UNITY        23
#define MANDEL4      24
#define JULIA4       25
#define IFS          26
#define IFS3D        27
#define BARNSLEYM3   28
#define BARNSLEYJ3   29
#define DEMM         30
#define DEMJ         31
#define BIFURCATION  32

struct lcomplex lcoefficient,lold;

extern int decomp[];
extern double param[];
extern double inversion[];                     /* inversion parameters */
extern double f_radius,f_xcenter,f_ycenter;    /* inversion radius, center */
extern double xxmin,xxmax,yymin,yymax;         /* corners */
extern VECTOR view;
extern int init3d[];

int infinity, maxcolor;
int root, degree,basin;
double roverd, d1overd, threshold, floatmin, floatmax;
extern struct complex init,tmp,old,new,saved;
struct complex roots[10];
long tempx, tempy;

extern int color, oldcolor, oldmax, oldmax10, row, col, passes;
extern int iterations, invert;
extern double far *dx0, far *dy0;
extern long XXOne, FgOne, FgTwo, LowerLimit;
struct complex one;

extern void (*plot)();
extern void symplot2(int, int, int);
extern void symplot4(int, int, int);

extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors;				/* maximum colors available */
extern int	inside;				/* "inside" color to use    */
extern int	maxit;				/* try this many iterations */
extern int	fractype;			/* fractal type */
extern int	numpasses;			/* 0 = 1 pass, 1 = double pass */
extern int	solidguessing;		/* 1 if solid-guessing */
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


static int c_exp;

/* These are local but I don't want to pass them as parameters */
extern struct complex lambda;
extern double deltaX, deltaY;
extern double magnitude, rqlim;
extern int XXdots, YYdots; /* local dots */
extern double parm1,parm2;
extern double *floatparmx,*floatparmy;
extern long *longparmx,*longparmy;
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

double tmpexp, tmpcos, tempsqrx, tempsqry;

long oldxinitx,oldyinity,oldxinity,oldyinitx,initx,inity;
extern long newx, newy;
long modulusinit;
extern long lambdax, lambday;
extern long savedx, savedy;
long temp2x, temp2y;
extern long lmagnitud, llimit, llimit2, lclosenuff;
extern periodicitycheck;
extern char floatflag;

extern int StandardFractal();
extern int NewtonFractal2(); /* Lee Crocker's Newton code */

/* -------------------------------------------------------------------- */
/*		Stand-alone routines											*/
/* -------------------------------------------------------------------- */

extern char far plasmamessage[];

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


ifs3d()
{
   if(floatflag)
      return(ifs3dfloat()); /* double version of ifs3d */
   else
      return(ifs3dlong());  /* long version of ifs3d   */
}

/* double version - mainly for testing */
ifs3dfloat()
{
   double tmp;   
   VECTOR minifs;
   VECTOR maxifs;
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   double newx,newy,newz,r,sum;
    
   int i,k;

   /* 3D viewing stuff */
   MATRIX doublemat;           /* transformation matrix */
   VECTOR ifsvect;	           /* interated function orbit value */
   VECTOR viewvect;        /* orbit transformed for viewing */
   /*
   printf("xrot %d yrot %d zrot %d \nxshift %d yshift %d perspective %d\n",
             XROT,YROT,ZROT,XSHIFT,YSHIFT,ZVIEWER);
   */
   for(i=0;i<3;i++)
   {
      minifs[i] =  100000.0; /* impossible value */
      maxifs[i] = -100000.0;
   }
   
   /* build transformation matrix */
   identity (doublemat);

   /* apply rotations - uses the same rotation variables as line3d.c */
   xrot ((double)XROT / 57.29577,doublemat);
   yrot ((double)YROT / 57.29577,doublemat);
   zrot ((double)ZROT / 57.29577,doublemat);
   /*
   scale((double)XSCALE/100.0,(double)YSCALE/100.0,(double)ROUGH/100.0,doublemat);
   */
   if(!ZVIEWER)
      trans((double)XSHIFT*(xxmax-xxmin)/100.0,(double)YSHIFT*(yymax-yymin)/100.0,0.0,doublemat);

   if(diskvideo)		/* this would KILL a disk drive! */
   {
	  setvideomode(3,0,0,0);
	  buzzer(2);
	  helpmessage(plasmamessage);
	  return(-1);
   }

   ifsvect[0] = 0;
   ifsvect[1] = 0;
   ifsvect[2] = 0;
   
   /* make maxct a function of screen size               */
   /* 1k times maxit at EGA resolution seems about right */
   maxct = (double)maxit*(1024.0*xdots*ydots)/(640.0*350.0); 
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */ 
   {
      if (ct & 127)           /* reduce the number of keypress checks */
         if( check_key() )    /* keypress bails out */
            return(-1);
      r = rand();        /* generate fudged random number between 0 and 1 */
      r /= 32767;

      /* pick which iterated function to execute, weighted by probability */
      sum = initifs3d[0][12];
      k = 0;
      while ( sum < r)
      {
         k++;
         sum += initifs3d[k][12];
         if (initifs3d[k+1][12] == 0) break;    /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      newx = initifs3d[k][0] * ifsvect[0] +
             initifs3d[k][1] * ifsvect[1] +
             initifs3d[k][2] * ifsvect[2] + initifs3d[k][9];
      newy = initifs3d[k][3] * ifsvect[0] +
             initifs3d[k][4] * ifsvect[1] +
             initifs3d[k][5] * ifsvect[2] + initifs3d[k][10];
      newz = initifs3d[k][6] * ifsvect[0] +
             initifs3d[k][7] * ifsvect[1] +
             initifs3d[k][8] * ifsvect[2] + initifs3d[k][11];
            
      ifsvect[0] = newx;
      ifsvect[1] = newy;
      ifsvect[2] = newz;

      /* 3D VIEWING TRANSFORM */
      vmult(ifsvect,doublemat,viewvect);

      /* find minz and maxz */
      if(ZVIEWER && ct <= 100) /* waste this many points to find minz and maxz */
      {
         for(i=0;i<3;i++)
            if ((tmp = viewvect[i]) < minifs[i])
               minifs[i] = tmp;
            else if (tmp > maxifs[i])
               maxifs[i] = tmp;
         if(ct == 100)
         {
            /* set of view vector */
            for(i=0;i<2;i++)
               view[i] = (maxifs[i]+minifs[i])/2.0;
      
            /* z value of user's eye - should be more negative than extreme 
               negative part of image */
            view[2] = (minifs[2]-maxifs[2])*(double)ZVIEWER/100.0;

            /* translate back exactly amount so maximum values are non-positive */
            trans((double)XSHIFT*(xxmax-xxmin)/100.0,(double)YSHIFT*(yymax-yymin)/100.0,-maxifs[2],doublemat); 
/*
            printf("minx %lf maxx %lf\n",minifs[0],maxifs[0]);
            printf("miny %lf maxy %lf\n",minifs[1],maxifs[1]);
            printf("minz %lf maxz %lf\n",minifs[2],maxifs[2]);
            printf("view %lf %lf %lf\n",view[0],view[1],view[2]); */
         }
      }      
      
      /* apply perspective if requested */
      if(ZVIEWER)
         perspective(viewvect);

      /* plot if inside window */
      if ( viewvect[0] > xxmin && viewvect[0] < xxmax 
        && viewvect[1] > yymin && viewvect[1] < yymax && ct > 100 )
      {
         col =          (( viewvect[0]-xxmin) / deltaX);
         row = YYdots - (( viewvect[1]-yymin) / deltaY);
         /* color is count of hits on this pixel */
         color = getcolor(col,row)+1;
         if( color < colors ) /* color sticks on last value */
            (*plot)(col,row,color);
      }
   } /* end while */
   return(0);
}
ifs3dlong()
{
   int bitshift;   /* want these local */
   long fudge;
   long delx,dely;
      
   double tmpx, tmpy, tmpz;
   extern VECTOR view;
   long iview[3];         /* perspective viewer's coordinates */      
   long tmp;   
   long maxifs[3];
   long minifs[3];
   extern int init3d[];
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   long localifs[NUMIFS][IFS3DPARM];        /* local IFS values */
   long newx,newy,newz,r,sum, xmin, xmax, ymin, ymax, tempr;
    
   int i,j,k;

   /* 3D viewing stuff */
   MATRIX doublemat;           /* transformation matrix */
   long longmat[4][4];         /* long version of matrix */
   long ifsvect[3];	           /* interated function orbit value */
   long viewvect[3];        /* orbit transformed for viewing */

   /*
   printf("xrot %d yrot %d zrot %d \nxshift %d yshift %d perspective %d\n",
             XROT,YROT,ZROT,XSHIFT,YSHIFT,ZVIEWER);
   */
   bitshift = 16;
   fudge = 1L << bitshift;

   delx = deltaX*fudge;
   dely = deltaY*fudge;
      
   for(i=0;i<3;i++)
   {
      minifs[i] =  1L << 30;
      maxifs[i] = -minifs[i];
   }
   
   /* build transformation matrix */
   identity (doublemat);

   /* apply rotations - uses the same rotation variables as line3d.c */
   xrot ((double)XROT / 57.29577,doublemat);
   yrot ((double)YROT / 57.29577,doublemat);
   zrot ((double)ZROT / 57.29577,doublemat);

   /* apply scale */
   scale((double)XSCALE/100.0,(double)YSCALE/100.0,(double)ROUGH/100.0,doublemat);

   if(!ZVIEWER)
      trans((double)XSHIFT*(xxmax-xxmin)/100.0,(double)YSHIFT*(yymax-yymin)/100.0,0.0,doublemat);

   /* copy xform matrix to long for for fixed point math */
   for (i = 0; i < 4; i++)
   for (j = 0; j < 4; j++)
      longmat[i][j] = doublemat[i][j] * fudge;

   if(diskvideo)		/* this would KILL a disk drive! */
   {
	  setvideomode(3,0,0,0);
	  buzzer(2);
	  helpmessage(plasmamessage);
	  return(-1);
   }

   for (i = 0; i < NUMIFS; i++)    /* fill in the local IFS array */
   for (j = 0; j < IFS3DPARM; j++)
         localifs[i][j] = initifs3d[i][j] * fudge;

   xmin  = xxmin*fudge;   
   xmax  = xxmax*fudge;
   ymax  = yymax*fudge;
   ymin  = yymin*fudge;

   tempr = fudge / 32767;        /* find the proper rand() fudge */

   ifsvect[0] = 0;
   ifsvect[1] = 0;
   ifsvect[2] = 0;
   
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
      sum = localifs[0][12];
      k = 0;
      while ( sum < r)
      {
         k++;
         sum += localifs[k][12];
         if (localifs[k+1][12] == 0) break;    /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      newx = multiply(localifs[k][0], ifsvect[0], bitshift) +
             multiply(localifs[k][1], ifsvect[1], bitshift) +
             multiply(localifs[k][2], ifsvect[2], bitshift) + localifs[k][9];
      newy = multiply(localifs[k][3], ifsvect[0], bitshift) +
             multiply(localifs[k][4], ifsvect[1], bitshift) +
             multiply(localifs[k][5], ifsvect[2], bitshift) + localifs[k][10];
      newz = multiply(localifs[k][6], ifsvect[0], bitshift) +
             multiply(localifs[k][7], ifsvect[1], bitshift) +
             multiply(localifs[k][8], ifsvect[2], bitshift) + localifs[k][11];
            
      ifsvect[0] = newx;
      ifsvect[1] = newy;
      ifsvect[2] = newz;


      /* 3D VIEWING TRANSFORM */
      longvmult(ifsvect,longmat,viewvect, bitshift);

      /* find minz and maxz */
      if( ZVIEWER && ct <= 100) /* waste this many points to find minz and maxz */
      {
         for(i=0;i<3;i++)
            if ((tmp = viewvect[i]) < minifs[i])
               minifs[i] = tmp;
            else if (tmp > maxifs[i])
               maxifs[i] = tmp;
         if(ct == 100)
         {
            /* set of view vector */
            for(i=0;i<2;i++)
               iview[i] = (maxifs[i]+minifs[i])/2;
      
            /* z value of user's eye - should be more negative than extreme 
               negative part of image */
            iview[2] = (long)((minifs[2]-maxifs[2])*(double)ZVIEWER/100.0);

            /*
            printf("min %ld max %ld iview[2] %d\n",minifs[2],maxifs[2],iview[2]);
            getch();
            */ 

            /* translate back exactly amount so maximum values are non-positive */
            tmpx = ((double)XSHIFT*(xmax-xmin))/(100.0*fudge);
            tmpy = ((double)YSHIFT*(ymax-ymin))/(100.0*fudge);
            tmpz = -((double)maxifs[2]);
            tmpz *= 1.1;
            tmpz /= fudge; 
            trans(tmpx,tmpy,tmpz,doublemat); 
            
            for(i=0;i<3;i++)
            {
               view[i] = iview[i];
               view[i] /= fudge;
            } 
   
            /* copy xform matrix to long for for fixed point math */
            for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
               longmat[i][j] = doublemat[i][j] * fudge;
         }
      }      
      
      /* apply perspective if requested */
      if(ZVIEWER && ct > 100)
      {
         if(debugflag==22 || ZVIEWER < 100) /* use float for small persp */
         {
            /* use float perspective calc */
            VECTOR tmpv;
            for(i=0;i<3;i++)
            {
               tmpv[i] = viewvect[i];
               tmpv[i] /= fudge;
            } 
            perspective(tmpv);
            for(i=0;i<3;i++)
               viewvect[i] = tmpv[i]*fudge;
         }
         else
            longpersp(viewvect,iview,bitshift);
      }
            
      /* plot if inside window */
      if ( viewvect[0] > xmin && viewvect[0] < xmax 
        && viewvect[1] > ymin && viewvect[1] < ymax && ct > 100 )
      {
         col =          (( viewvect[0]-xmin) / delx);
         row = YYdots - (( viewvect[1]-ymin) / dely);

         /* color is count of hits on this pixel */
         color = getcolor(col,row)+1;
         if( color < colors ) /* color sticks on last value */
            (*plot)(col,row,color);
      }
   }
   return(0);
}

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
		temp = squared.x - squared.y + parm1;
		y = 2 * x * y + parm2;
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

	if ((filter_cycles = parm1) == 0) filter_cycles = DEFAULTFILTER;
   
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
static long lxt, lyt, lt2;
lcpower(struct lcomplex *base, int exp, struct lcomplex *result, int fudge)
{
    lxt = base->x;   lyt = base->y;
 
    if (exp & 1) 
    {  
       result->x = lxt;     
       result->y = lyt;     
    }
    else         
    {  
       result->x = 1L<<fudge;    
       result->y = 0L;    
    }
 
    exp >>= 1;
    while (exp) 
    {
        lt2 = multiply(lxt, lxt, fudge) - multiply(lyt,lyt,fudge);
        lyt = multiply(lxt,lyt,fudge)<<1;
        lxt = lt2;
 
        if (exp & 1) 
        {
            lt2 = multiply(lxt,result->x, fudge) - multiply(lyt,result->y,fudge);
            result->y = multiply(result->y,lxt,fudge) + multiply(lyt,result->x,fudge);
            result->x = lt2;
        }
        exp >>= 1;
    }
}
 
/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
 
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

    if (DIST1(new) < THRESHOLD) 
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
                   tmpcolor = 1+(i&7)+((color&1)<<3);
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

JuliaFractal() 
{
   /* used for C prototype of fast integer math routines for classic
      Mandelbrot and Julia */ 
   newx  = tempx - tempy + *longparmx;
   newy = (multiply(lold.x, lold.y, bitshift) << 1) + *longparmy;
   tempx = multiply(newx, newx, bitshift);
   tempy = multiply(newy, newy, bitshift);
   lmagnitud = tempx + tempy;
   if (lmagnitud >= llimit || lmagnitud < 0 || labs(newx) > llimit2 
      || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   return(0);
}

Barnsley1Fractal() 
{
   /* Barnsley's Mandelbrot type M1 from "Fractals
   Everywhere" by Michael Barnsley, p. 322 */ 

   /* calculate intermediate products */
   oldxinitx   = multiply(lold.x, *longparmx, bitshift);
   oldyinity   = multiply(lold.y, *longparmy, bitshift);
   oldxinity   = multiply(lold.x, *longparmy, bitshift);
   oldyinitx   = multiply(lold.y, *longparmx, bitshift);
   /* orbit calculation */                  
   if(lold.x >= 0) 
   {
      newx = (oldxinitx - *longparmx - oldyinity);               
      newy = (oldyinitx - *longparmy + oldxinity);
   }
   else 
   {
      newx = (oldxinitx + *longparmx - oldyinity);               
      newy = (oldyinitx + *longparmy + oldxinity);               
   }
   lmagnitud = multiply(newx, newx, bitshift)
              +multiply(newy, newy, bitshift);
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   return(0);
}

Barnsley2Fractal() 
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 331, example 4.2 */
    
   /* calculate intermediate products */
   oldxinitx   = multiply(lold.x, *longparmx, bitshift);
   oldyinity   = multiply(lold.y, *longparmy, bitshift);
   oldxinity   = multiply(lold.x, *longparmy, bitshift);
   oldyinitx   = multiply(lold.y, *longparmx, bitshift);

   /* orbit calculation */                  
   if(oldxinity + oldyinitx >= 0) 
   {
      newx = oldxinitx - *longparmx - oldyinity;               
      newy = oldyinitx - *longparmy + oldxinity;               
   }
   else 
   {
      newx = oldxinitx + *longparmx - oldyinity;               
      newy = oldyinitx + *longparmy + oldxinity;               
   }
   lmagnitud = multiply(newx, newx, bitshift)
              +multiply(newy, newy, bitshift);
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   return(0);
}

JuliafpFractal() 
{
   /* floating point version of classical Mandelbrot/Julia */
   new.x = tempsqrx - tempsqry + *floatparmx;
   new.y = 2.0 * old.x * old.y + *floatparmy;
   tempsqrx = sqr(new.x);
   tempsqry = sqr(new.y);
   if((tempsqrx + tempsqry) >= rqlim) return(1);
   old = new;
   return(0);
}

LambdaFractal() 
{
   /* variation of classical Mandelbrot/Julia */
   
   /* in complex math) temp = Z * (1-Z) */
   tempx = lold.x  - multiply(lold.x, lold.x, bitshift)
                 + multiply(lold.y, lold.y, bitshift);
   tempy = lold.y
                 - (multiply(lold.y, lold.x, bitshift)  << 1);
   /* (in complex math) Z = Lambda * Z */
   newx = multiply(*longparmx, tempx, bitshift)
        - multiply(*longparmy, tempy, bitshift);
   newy = multiply(*longparmx, tempy, bitshift)
        + multiply(*longparmy, tempx, bitshift);
   /* (in complex math) get the magnititude */
   lmagnitud = multiply(newx, newx, bitshift)
             + multiply(newy, newy, bitshift);
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   return(0);
}

SierpinskiFractal() 
{
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */ 
   newx = (lold.x << 1);		/* new.x = 2 * old.x  */
   newy = (lold.y << 1);		/* new.y = 2 * old.y  */
   if( lold.y > tempy)		/* if old.y > .5 */
      newy = newy - tempx;	/* new.y = 2 * old.y - 1 */
   else if(lold.x > tempy)	/* if old.x > .5 */
      newx = newx - tempx;	/* new.x = 2 * old.x - 1 */
   /* end barnsley code */
   /* (in complex math) get the magnititude */
   lmagnitud = multiply(newx, newx, bitshift)
           +   multiply(newy, newy, bitshift);
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   return(0);
}

LambdasineFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   /* calculate sin(z) */
   if (fabs(old.y) >= 50.0) return(1);
   tmp.x = sin(old.x)*cosh(old.y);
   tmp.y = cos(old.x)*sinh(old.y);
 
   /*multiply by lamda */
   new.x = *floatparmx*tmp.x - *floatparmy*tmp.y;
   new.y = *floatparmy*tmp.x + *floatparmx*tmp.y;
   old = new;
   return(0);
}

LambdacosineFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   /* calculate cos(z) */
   if (fabs(old.y) >= 50.0) return(1);
   tmp.x = cos(old.x)*cosh(old.y);
   tmp.y = sin(old.x)*sinh(old.y);

   /*multiply by lamda */
   new.x = *floatparmx*tmp.x - *floatparmy*tmp.y;
   new.y = *floatparmy*tmp.x + *floatparmx*tmp.y;
   old = new;
   return(0);
}

LambdaexponentFractal() 
{
   /* found this in  "Science of Fractal Images" */ 

   /* calculate exp(z) */
   if (fabs(old.y) >= 1.0e8) return(1); /* TLOSS  errors */
   if (fabs(old.x) >= 6.4e2) return(1); /* DOMAIN errors */
   tmpcos = cos(old.y);
   if (old.x >= 50.0 && tmpcos >= 0.0) return(1);
   tmpexp = exp(old.x);
   tmp.x = tmpexp*tmpcos;
   tmp.y = tmpexp*sin(old.y);

   /*multiply by lamda */
   new.x = *floatparmx*tmp.x - *floatparmy*tmp.y;
   new.y = *floatparmy*tmp.x + *floatparmx*tmp.y;
   old = new;
   return(0);
}

MarksLambdaFractal() 
{
   /* Mark Peterson's variation of "lambda" function */

   /* Z1 = (Lambda * Z**2) + Lambda */
   temp2x = tempx - tempy;
   temp2y = multiply(lold.x ,lold.y ,bitshift)<<1;

   newx = multiply(lcoefficient.x, temp2x, bitshift)
        - multiply(lcoefficient.y, temp2y, bitshift) + *longparmx;
   newy = multiply(lcoefficient.x, temp2y, bitshift)
        + multiply(lcoefficient.y, temp2x, bitshift) + *longparmy;

   /* (in complex math) get the magnititude */
   tempx = multiply(newx, newx, bitshift);
   tempy = multiply(newy, newy, bitshift);
   lmagnitud = tempx + tempy;
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx;
   lold.y = newy;
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
   newx  = tempx - tempy;
   newy = (multiply(lold.x, lold.y, bitshift) << 1);
   tempx = multiply(newx, newx, bitshift);
   tempy = multiply(newy, newy, bitshift);
   lmagnitud = tempx + tempy;
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
   /* then, compute ((x + iy)**2)**2 + lambda */
   newx  = tempx - tempy + *longparmx;
   newy = (multiply(lold.x, lold.y, bitshift) << 1) + *longparmy;
   tempx = multiply(newx, newx, bitshift);
   tempy = multiply(newy, newy, bitshift);
   lmagnitud = tempx + tempy;
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
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
      newx = oldxinitx   - oldyinity - fudge;
      newy = oldxinity << 1;               
   }
   else 
   {
      newx = oldxinitx - oldyinity - fudge
           + multiply(*longparmx,lold.x,bitshift);
      newy = oldxinity <<1;

      /* This term added by Tim Wegner to make dependent on the 
         imaginary part of the parameter. (Otherwise Mandelbrot 
         is uninteresting. */
      newy += multiply(*longparmy,lold.x,bitshift); 
   }
   lmagnitud = multiply(newx, newx, bitshift)
              +multiply(newy, newy, bitshift);
   if (lmagnitud >= llimit || lmagnitud < 0 ||
      labs(newx) > llimit2 || labs(newy) > llimit2) return(1);
   lold.x = newx; lold.y = newy;
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
/*		Initialization (once per pixel) routines		*/
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
   initx = lx0[col];

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
      initx = init.x*fudge;
      inity = init.y*fudge;
   }

   lold.x = init.x + lambdax;   /* initial pertubation of parameters set */
   lold.y = init.y + lambday;
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

   tempx = multiply(lold.x, lold.x, bitshift);
   tempy = multiply(lold.y, lold.y, bitshift);
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
      initx = init.x*fudge;
      inity = init.y*fudge;
   }
   else
      initx = lx0[col];
   
   lold.x = initx + lambdax; /* initial pertubation of parameters set */
   lold.y = inity + lambday;
   tempx = multiply(lold.x, lold.x, bitshift);
   tempy = multiply(lold.y, lold.y, bitshift);
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
      initx = init.x*fudge;
      inity = init.y*fudge;
   }
   else
      initx = lx0[col];
   
   lold.x = initx + lambdax; /* initial pertubation of parameters set */
   lold.y = inity + lambday;

   if(c_exp > 2)
      lcpower(&lold,c_exp,&lcoefficient,bitshift);
   else if(c_exp == 2)
   {
      lcoefficient.x = multiply(lold.x,lold.x,bitshift) - multiply(lold.y,lold.y,bitshift);
      lcoefficient.y = multiply(lold.x,lold.y,bitshift);
   }
   else if(c_exp < 2)
      lcoefficient = lold;

   lold.x += lambdax; /* perturb initial value */
   lold.y += lambday;
   tempx = multiply(lold.x, lold.x, bitshift);
   tempy = multiply(lold.y, lold.y, bitshift);
}


void mandelfp_per_pixel()
{
   /* floating point mandelbrot */
   /* mandelfp */

   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col];
   old.x = init.x + parm1; /* initial pertubation of parameters set */
   old.y = init.y + parm2;

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

void othermandelfp_per_pixel()
{
   /* mandelsine */
   /* mandelcos  */
   /* mandelexp  */
   if(invert)
      invertz2(&init);
   else
      init.x = dx0[col];
   old.x = init.x + parm1; /* initial pertubation of parameters set */
   old.y = init.y + parm2;
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

/* -------------------------------------------------------------------- */
/*		Setup (once per fractal image) routines			*/
/* -------------------------------------------------------------------- */

MandelSetup()		/* Mandelbrot Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0)
      calctype = calcmand; /* the normal case - use CALCMAND */
   else
   {
      /* special case: use the main processing loop */
      calctype = StandardFractal;
      longparmx = &initx;
      longparmy = &inity;
   }
   return(1);
}
JuliaSetup()		/* Julia Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0)
      calctype = calcmand; /* the normal case - use CALCMAND */
   else
   {
      /* special case: use the main processing loop */
      calctype = StandardFractal;
      longparmx = &lambdax;
      longparmy = &lambday;
   }
   return(1);
}

NewtonSetup()		/* Newton/NewtBasin Routines */
{
   int i;
   extern int basin;      

   /* set up table of roots of 1 along unit circle */
   degree = (int)parm1;
   if(degree  >= 10)
      degree = 10;
   else if(degree < 3)
      degree = 3;
   root = 1;
   
   /* precalculated values */
   roverd = (double)root / (double)degree;
   d1overd = (double)(degree - 1) / (double)degree;
   threshold = THRESHOLD;
   infinity = INFINITY;
   maxcolor = MAX_COLOR;
   floatmin = FLT_MIN;
   floatmax = FLT_MAX;

   /* list of roots to discover where we converged for newtbasin */
   for(i=0;i<degree;i++)
   {
      roots[i].x = cos(i*PI*2.0/(float)degree);
      roots[i].y = sin(i*PI*2.0/(float)degree);
   }

   if (fractype==NEWTBASIN)
      basin = 1;
   else
      basin = 0;   
   if (fractype==NEWTON && degree%4 == 0)
      setsymmetry(XYAXIS);
   else if (fractype==NEWTON)
      setsymmetry(XAXIS);
   else
      plot = putcolor;

   calctype=StandardFractal;
   if(debugflag == 90)
      fractalspecific[fractype].orbitcalc = NewtonFractal;      
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
   floatparmx = &init.x;
   floatparmy = &init.y;
   return(1);
}
JuliafpSetup()
{
   floatparmx = &parm1;
   floatparmy = &parm2;
   return(1);
}
MandellongSetup()
{
   if(fractype == MARKSMANDEL)
   {
      c_exp = param[2];
      if(c_exp & 1) /* odd exponents */
         setsymmetry(XYAXIS);         
   }   
   longparmx = &initx;
   longparmy = &inity;
   return(1);
}
JulialongSetup()
{
   longparmx = &lambdax;
   longparmy = &lambday;
   return(1);
}
MarksJuliaSetup()
{
   c_exp = param[2];
   longparmx = &lambdax;
   longparmy = &lambday;
   lold.x = *longparmx;
   lold.y = *longparmy;
   if(c_exp > 2)
      lcpower(&lold,c_exp,&lcoefficient,bitshift);
   else if(c_exp == 2)
   {
      lcoefficient.x = multiply(lold.x,lold.x,bitshift) - multiply(lold.y,lold.y,bitshift);
      lcoefficient.y = multiply(lold.x,lold.y,bitshift);
   }
   else if(c_exp < 2)
      lcoefficient = lold;
   return(1);
}

SierpinskiSetup()
{
   /* sierpinski */
   periodicitycheck = 0;		/* disable periodicity checks */
   tempx = 1; tempx = tempx << bitshift;	/* tempx = 1 */
   tempy = tempx >> 1;			/* tempy = .5 */
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
static char newtdegree[] = "Polynomial Degree (3 to 10)";

/* for MarksMandel/Julia */
static char exponent[] = "Parameter Exponent (  > 0 )";

struct fractalspecificstuff fractalspecific[] = 
{
   /*
     fractal name and parameters (text strings)
     xmin  xmax  ymin  ymax int tojulia   tomandel symmetry 
   |------|-----|-----|-----|--|--------|---------|--------|
     orbit fnct     per_pixel fnct  per_image fnct  calctype fcnt    bailout
   |---------------|---------------|---------------|----------------|-------|
   */

   "mandel",      realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 1, JULIA,     NOFRACTAL,  XAXIS_NOPARM, 
   JuliaFractal,  mandel_per_pixel,MandelSetup,    calcmand,        4.0,

   "julia",       realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL,      ORIGIN, 
   JuliaFractal,   julia_per_pixel, JuliaSetup,    calcmand,        4.0,

   "newtbasin",   newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "lambda",      realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, NOSYM, 
   LambdaFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,4.0,
   
   "mandelfp",    realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,   NOFRACTAL,   XAXIS_NOPARM, 
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,4.0,
   
   "newton",      newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,   XAXIS, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "juliafp",     realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP,    ORIGIN, 
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,4.0,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)","","","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,           NULL,   StandaloneSetup,      plasma,          0.0,
   
   "lambdasine",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELSINE,  PI_SYM, 
   LambdasineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,50.0,
   
   "lambdacos",   realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELCOS,   PI_SYM, 
   LambdacosineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,50.0,

   "lambdaexp",   realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELEXP,   XAXIS, 
   LambdaexponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,50.0,

   "test",        "(testpt Param #1)","(testpt param #2)","(testpt param #3)", "(testpt param #4)",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, test,    4.0,
 
  "sierpinski",  "","","","",
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL,   NOSYM, 
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,
 
  "barnsleym1",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL,  XYAXIS_NOPARM, 
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,4.0,
 
  "barnsleyj1",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1,  ORIGIN, 
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,4.0,

   "barnsleym2",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL,  YAXIS_NOPARM, 
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,4.0,
   
   "barnsleyj2",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2,  ORIGIN, 
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,4.0,
   
   "mandelsine",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 0, LAMBDASINE,NOFRACTAL,  XYAXIS_NOPARM, 
   LambdasineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,50.0,

   "mandelcos",   realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 0, LAMBDACOS, NOFRACTAL,  XYAXIS_NOPARM, 
   LambdacosineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,50.0,

   "mandelexp",   realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 0, LAMBDAEXP, NOFRACTAL,   0, 
   LambdaexponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,50.0,

   "mandellambda",realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, LAMBDA,    NOFRACTAL,  XAXIS_NOPARM, 
   LambdaFractal,long_julia_per_pixel,MandellongSetup,StandardFractal,4.0,

   "marksmandel", realz0, imagz0, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL,  XAXIS,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,4.0,

   "marksjulia", realparm, imagparm, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,4.0,

   "unity",       "","","","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL,   XYAXIS, 
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,0.0,
   
   "mandel4",      realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,     NOFRACTAL,  XAXIS_NOPARM, 
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  4.0,

   "julia4",       realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4,      ORIGIN, 
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,    4.0,

   "ifs",        "","","", "",
   -8.0,  8.0, -1.0, 11.0, 1, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, ifs,    0.0,

   "ifs3d",        "","","", "",
   -11.0,  11.0, -11.0, 11.0, 1, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,      StandaloneSetup, ifs3d,    0.0,

   "barnsleym3",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL,  XAXIS_NOPARM, 
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,4.0,
   
   "barnsleyj3",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3,  XAXIS, 
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,4.0,

   "demm",    realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 0, DEMJ,   NOFRACTAL,   XAXIS_NOPARM, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,4.0,

   "demj",     realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, DEMM,    ORIGIN, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,4.0,

   "bifurcation",     "", "","","",
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL,    NOSYM, 
   NULL,				NULL, 		StandaloneSetup, Bifurcation,4.0,

   NULL, NULL, NULL, NULL, NULL,	/* marks the END of the list */
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL, NULL, NULL, NULL,0.0
};
