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

/* DEFINITIONS PRIOR TO THIS POINT ARE FROZEN BY THE VERSION 11.0 RELEASE! */

#define NEWTONDEGREELIMIT  100
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
extern struct complex init,tmp,old,new,saved;
struct complex  staticroots[16]; /* roots array for degree 16 or less */
struct complex  *roots = staticroots;
struct MPC      *MPCroots;
extern int color, oldcolor, oldmax, oldmax10, row, col, passes;
extern int iterations, invert;
extern double far *dx0, far *dy0;
extern long XXOne, FgOne, FgTwo, LowerLimit;
struct complex one;

extern void (*plot)();

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
#define MPdistance(z1,z2)  (pMPadd(pMPsqr(pMPsub((z1).r,(z2).r)),pMPsqr(pMPsub((z1).i,(z2).i))))

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
extern int ComplexNewton(void), ComplexBasin(void);
complex_mult(struct complex arg1,struct complex arg2,struct complex *pz);
complex_div(struct complex arg1,struct complex arg2,struct complex *pz);

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
   if(param[0] > 0 && param[0] <= .05)
     dt = param[0];
   else
     dt = .02; /* time step */
   if(param[1])
      a = param[1];
   else
      a = 5;
   if(param[2])
      b = param[2];
   else
      b = 15;
   if(param[3])
      c = param[3];
   else
      c = 1;
         
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
   if(param[0] > 0 && param[0] <= .05)
     dt = param[0]*fudge;
   else
     dt = .02*fudge; /* time step */
   if(param[1])
      a = param[1];
   else
      a = 5;
   if(param[2])
      b = param[2];
   else
      b = 15;
   if(param[3])
      c = param[3];
   else
      c = 1;
   
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
lorenz3dlong()
{
   unsigned count;
   long dx,dy,dz,x,y,z,dt,a,b,c;
   long adt,bdt,cdt,xdt,ydt;
   int oldcol,oldrow;
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
   long newx,newy,newz,r,sum, xmin, xmax, ymin, ymax;
    
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
   oldcol = oldrow = -1;
   color = 2;
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

   x = fudge;  /* initial conditions */
   y = fudge;
   z = fudge;
   if(param[0] > 0 && param[0] <= .05)
     dt = param[0]*fudge;
   else
     dt = .02*fudge; /* time step */
   if(param[1])
      a = param[1];
   else
      a = 5;
   if(param[2])
      b = param[2];
   else
      b = 15;
   if(param[3])
      c = param[3];
   else
      c = 1;
   
   /* precalculations for speed */
   adt = a*dt;
   bdt = b*dt;
   cdt = c*dt;

   ifsvect[0] = x;
   ifsvect[1] = y;
   ifsvect[2] = z;
   
   /* make maxct a function of screen size               */
   /* 1k times maxit at EGA resolution seems about right */
   maxct = (float)maxit*(1024.0*xdots*ydots)/(640.0*350.0); 
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */ 
   {
      if (++count > 1000) 
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
              color = 1;        /* (don't use the background color) */
      }
      if(check_key())
         return(-1);

      x = ifsvect[0];
      y = ifsvect[1];
      z = ifsvect[2];

      xdt = multiply(x,dt,bitshift);
      ydt = multiply(y,dt,bitshift);
      dx  = -multiply(adt,x,bitshift) + multiply(adt,y,bitshift);
      dy  =  multiply(bdt,x,bitshift) -ydt -multiply(z,xdt,bitshift);
      dz  = -multiply(cdt,z,bitshift) + multiply(x,ydt,bitshift);

      x += dx;
      y += dy;
      z += dz;

      ifsvect[0] = x;
      ifsvect[1] = y;
      ifsvect[2] = z;


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
         if(oldcol != -1)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
         else            
            (*plot)(col,row,color&(colors-1));
         oldcol = col;
         oldrow = row;    
      }
      else
         oldrow = oldcol = -1;
   }
   return(0);
}

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
   if((tempsqrx + tempsqry) >= rqlim) return(1);\
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
    
    /* the labs in next line added to prevent discontinuity in image */
    tmp1.y = atan(labs(z->y/z->x));

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

#define MPCmod(x) (pMPadd(pMPmul((x).r, (x).r), pMPmul((x).i, (x).i)))
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

    mpcnew.r = pMPsub(pMPmul(mpctmp.r,mpcold.r),pMPmul(mpctmp.i,mpcold.i));
    mpcnew.i = pMPadd(pMPmul(mpctmp.r,mpcold.i),pMPmul(mpctmp.i,mpcold.r));

    mpctmp1.r = pMPsub(mpcnew.r, MPCone.r);
    mpctmp1.i = pMPsub(mpcnew.i, MPCone.i);

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

    mpcnew.r = pMPadd(pMPmul(mpd1overd,mpcnew.r),mproverd);
    mpcnew.i = pMPmul(mpcnew.i,mpd1overd);

    mpt2 = MPCmod(mpctmp);
    mpt2 = pMPdiv(mpone,mpt2);
    
    mpcold.r = pMPmul(mpt2,(pMPadd(pMPmul(mpcnew.r,mpctmp.r),pMPmul(mpcnew.i,mpctmp.i))));
    mpcold.i = pMPmul(mpt2,(pMPsub(pMPmul(mpcnew.i,mpctmp.r),pMPmul(mpcnew.r,mpctmp.i))));

    new.x = *pMP2d(mpcold.r);
    new.y = *pMP2d(mpcold.i);
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

JuliafpFractal() 
{
   /* floating point version of classical Mandelbrot/Julia */
   new.x = tempsqrx - tempsqry + floatparm->x;
   new.y = 2.0 * old.x * old.y + floatparm->y;
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
   cpower(&old,5,&new);
   z_to_the_z(&old,&old);
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
   
   lold.x = linit.x + llambda.x; /* initial pertubation of parameters set */
   lold.y = linit.y + llambda.y;
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
   old.x = init.x + parm.x; /* initial pertubation of parameters set */
   old.y = init.y + parm.y;

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
   mpcold.r = pd2MP(old.x);
   mpcold.i = pd2MP(old.y);
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

/* -------------------------------------------------------------------- */
/*		Setup (once per fractal image) routines			*/
/* -------------------------------------------------------------------- */

MandelSetup()		/* Mandelbrot Routine */
{
   if (debugflag != 90 && ! invert && decomp[0] == 0 && rqlim <= 4.0 
          && forcesymmetry == 999 && biomorph == -1)
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
         MPCroots[i].r = pd2MP(cos(i*PI*2.0/(double)degree));
         MPCroots[i].i = pd2MP(sin(i*PI*2.0/(double)degree));
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

   "mandel",      realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 1, JULIA,     NOFRACTAL, MANDELFP, XAXIS_NOPARM, 
   JuliaFractal,  mandel_per_pixel,MandelSetup,    calcmand,        STDBAILOUT,

   "julia",       realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL, JULIAFP,  ORIGIN, 
   JuliaFractal,   julia_per_pixel, JuliaSetup,    calcmand,        STDBAILOUT,

   "*newtbasin",   newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN,   NOSYM, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "lambda",      realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, NOFRACTAL,  NOSYM, 
   LambdaFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,
   
   "*mandel",    realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,   NOFRACTAL, MANDEL,  XAXIS_NOPARM, 
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,
   
   "*newton",      newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTON,   XAXIS, 
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "*julia",     realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP, JULIA,  ORIGIN, 
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)","","","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,           NULL,   StandaloneSetup,      plasma,          0.0,
   
   "*lambdasine",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELSINE, LLAMBDASINE, PI_SYM, 
   LambdasineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,
   
   "*lambdacos",   realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELCOS, LLAMBDACOS,  PI_SYM, 
   LambdacosineFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "*lambdaexp",   realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELEXP, LLAMBDAEXP,  XAXIS, 
   LambdaexponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "test",        "(testpt Param #1)","(testpt param #2)","(testpt param #3)", "(testpt param #4)",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, test,    STDBAILOUT,
 
  "sierpinski",  "","","","",
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,
 
  "barnsleym1",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL, NOFRACTAL,  XYAXIS_NOPARM, 
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
 
  "barnsleyj1",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1, NOFRACTAL,  ORIGIN, 
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "barnsleym2",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL, NOFRACTAL,  YAXIS_NOPARM, 
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
   
   "barnsleyj2",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2, NOFRACTAL,  ORIGIN, 
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,
   
   "*mandelsine",  realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDASINE,NOFRACTAL, LMANDELSINE, XYAXIS_NOPARM, 
   LambdasineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelcos",   realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDACOS, NOFRACTAL, LMANDELCOS, XYAXIS_NOPARM, 
   LambdacosineFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelexp",   realz0, imagz0,"","",
   -4.0,  4.0, -3.0,  3.0, 0, LAMBDAEXP, NOFRACTAL, LMANDELEXP,   0, 
   LambdaexponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "mandellambda",realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, LAMBDA,    NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   LambdaFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksmandel", realz0, imagz0, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL, NOFRACTAL,  NOSYM,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksjulia", realparm, imagparm, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL, NOFRACTAL,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,STDBAILOUT,

   "unity",       "","","","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   XYAXIS, 
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,0.0,
   
   "mandel4",      realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,     NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  STDBAILOUT,

   "julia4",       realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4, NOFRACTAL, ORIGIN, 
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,    STDBAILOUT,

   "ifs",        "","","", "",
   -8.0,  8.0, -1.0, 11.0, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, ifs,    0.0,

   "ifs3d",        "","","", "",
   -11.0,  11.0, -11.0, 11.0, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,      StandaloneSetup, ifs3d,    0.0,

   "barnsleym3",  realz0, imagz0,"","",
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,
   
   "barnsleyj3",  realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3, NOFRACTAL,  XAXIS, 
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "demm",    realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 0, DEMJ,   NOFRACTAL, NOFRACTAL,   XAXIS_NOPARM, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,STDBAILOUT,

   "demj",     realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, DEMM, NOFRACTAL,    ORIGIN, 
   NULL,				NULL, 		StandaloneSetup, DistanceEstimatorMethod,STDBAILOUT,

   "bifurcation",     "", "","","",
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,				NULL, 		StandaloneSetup, Bifurcation,STDBAILOUT,

   "*mandelsinh",  realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDASINH,NOFRACTAL, LMANDELSINH, XYAXIS_NOPARM, 
   LambdasinhFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,

   "*lambdasinh",  realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, MANDELSINH, LLAMBDASINH, PI_SYM, 
   LambdasinhFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "*mandelcosh",   realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDACOSH, NOFRACTAL, LMANDELCOSH, XYAXIS_NOPARM, 
   LambdacoshFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,FTRIGBAILOUT,
   
   "*lambdacosh",   realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, MANDELCOSH, LLAMBDACOSH, PI_SYM, 
   LambdacoshFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,FTRIGBAILOUT,

   "mandelsine",realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDASINE, NOFRACTAL, MANDELSINE, XYAXIS_NOPARM, 
   LongLambdasineFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdasine",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELSINE, LAMBDASINE,PI_SYM, 
   LongLambdasineFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelcos",realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDACOS, NOFRACTAL, MANDELCOS, XYAXIS_NOPARM, 
   LongLambdacosFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdacos",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELCOS, LAMBDACOS, PI_SYM, 
   LongLambdacosFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelsinh",realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDASINH, NOFRACTAL, MANDELSINH, XYAXIS_NOPARM, 
   LongLambdasinhFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdasinh",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELSINH, LAMBDASINH, ORIGIN, 
   LongLambdasinhFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mandelcosh",realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 16, LLAMBDACOSH,    NOFRACTAL, MANDELCOSH,  XYAXIS_NOPARM, 
   LongLambdacoshFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdacosh",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELCOSH, LAMBDACOSH, ORIGIN, 
   LongLambdacoshFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "mansinzsqrd",      realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 16, LJULSINZSQRD,  NOFRACTAL, FPMANSINZSQRD, XAXIS_NOPARM, 
   SinZsquaredFractal,mandel_per_pixel,MandellongSetup,StandardFractal, STDBAILOUT,

   "julsinzsqrd",       realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 16, NOFRACTAL, LMANSINZSQRD, FPJULSINZSQRD,  NOSYM, 
   SinZsquaredFractal,julia_per_pixel, JulialongSetup,StandardFractal, STDBAILOUT,

   "*mansinzsqrd",    realz0, imagz0,"","",
   -2.5,  1.5, -1.5,  1.5, 0, FPJULSINZSQRD,   NOFRACTAL, LMANSINZSQRD, XAXIS_NOPARM, 
   SinZsquaredfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal, STDBAILOUT,

   "*julsinzsqrd",     realparm, imagparm,"","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANSINZSQRD, LJULSINZSQRD,   NOSYM, 
   SinZsquaredfpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal, STDBAILOUT,

   "mandelexp",realz0, imagz0,"","",
   -4.0,  4.0, -3.0,  3.0, 16, LLAMBDAEXP,    NOFRACTAL,  MANDELEXP, XAXIS, 
   LongLambdaexponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "lambdaexp",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANDELEXP, LAMBDAEXP, XAXIS, 
   LongLambdaexponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,LTRIGBAILOUT,

   "manzpower", realz0, imagz0, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER,  XAXIS,
   longZpowerFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julzpower", realparm, imagparm, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER,   ORIGIN,
   longZpowerFractal,julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "*manzpower",    realz0, imagz0, exponent,"",
   -2.5,  1.5, -1.5,  1.5, 0, FPJULIAZPOWER,   NOFRACTAL, LMANDELZPOWER,  XAXIS_NOPARM, 
   floatZpowerFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julzpower",     realparm, imagparm, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER,  ORIGIN, 
   floatZpowerFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "manzzpwr",    realz0, imagz0, exponent,"",
   -2.5,  1.5, -1.5,  1.5, 0, FPJULZTOZPLUSZPWR,   NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM, 
   floatZtozpluszpwrFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "julzzpwr",     realparm, imagparm, exponent,"",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL,  NOSYM, 
   floatZtozpluszpwrFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "mansinexp",realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 16, LJULSINEXP,    NOFRACTAL,  FPMANSINEXP, XAXIS_NOPARM, 
   LongSinexponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julsinexp",      realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANSINEXP,FPJULSINEXP, NOSYM,
   LongSinexponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*mansinexp",   realz0, imagz0,"","",
   -8.0,  8.0, -6.0,  6.0, 0, FPJULSINEXP, NOFRACTAL, LMANSINEXP,   XAXIS_NOPARM, 
   FloatSinexponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julsinexp",   realparm, imagparm,"","",
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANSINEXP, LJULSINEXP,   NOSYM, 
   FloatSinexponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*popcorn", "", "", "","",
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORN,  NOPLOT, 
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "popcorn", "", "", "","",
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORN,  NOPLOT, 
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*lorenz","Time Step","a","b", "c",
   -15,  15, 0, 30, 0, NOFRACTAL, NOFRACTAL, LLORENZ,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, floatlorenz,    0.0,

   "lorenz","Time Step","a","b", "c",
   -15,  15, 0, 30, 16, NOFRACTAL, NOFRACTAL, FPLORENZ,   NOSYM, 
   NULL,          NULL,             StandaloneSetup, longlorenz,    0.0,

   "lorenz3d","Time Step","a","b", "c",
   -30.0,  30.0,  -30.0,   30.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL,          NULL,      StandaloneSetup, lorenz3dlong,    0.0,

   "newton",      newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTON,   XAXIS, 
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "newtbasin",      newtdegree,"", "","",
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTBASIN,   NOSYM, 
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,0.0,

   "complexnewton", realdegree, imagdegree, realroot, imagroot,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   ComplexNewton, otherjuliafp_per_pixel,  ComplexNewtonSetup, StandardFractal,0.0,

   "complexbasin", realdegree, imagdegree, realroot, imagroot,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   ComplexBasin, otherjuliafp_per_pixel,  ComplexNewtonSetup,  StandardFractal, 0.0,

   NULL, NULL, NULL, NULL, NULL,	/* marks the END of the list */
   0,  0, 0,  0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM, 
   NULL, NULL, NULL, NULL,0
};
