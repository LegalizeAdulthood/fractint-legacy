/*
   This file contains two 3 dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images. Tim Wegner
*/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "fractint.h"
#include "fractype.h"
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

/* Routines in this module	*/

int  orbit3dlongsetup();
int  orbit3dfloatsetup();
int  lorenz3dlongorbit(long *, long *, long *);
int  lorenz3dfloatorbit(double *, double *, double *);
int  henonfloatorbit(double *, double *, double *);
int  henonlongorbit(long *, long *, long *);
int  rosslerfloatorbit(double *, double *, double *);
int  pickoverfloatorbit(double *, double *, double *);
int  gingerbreadfloatorbit(double *, double *, double *);
int  rosslerlongorbit(long *, long *, long *);
int  kamtorusfloatorbit(double *, double *, double *);
int  kamtoruslongorbit(long *, long *, long *);
int  orbit2dfloat(void);
int  orbit2dlong(void);
int  orbit3dlongcalc(void);
int  orbit3dfloatcalc(void);
int  funny_glasses_call(int (*calc)());
int  ifs(void);
int  orbit3dfloat(void);
int  orbit3dlong(void);
int  ifs3d(void);

static int  ifs3dlong(void);
static int  ifs3dfloat(void);
static double determinant(double mat[3][3]);
static int  solve3x3(double mat[3][3],double vec[3],double ans[3]);
static int  setup_convert_to_screen(struct affine *);
static void setupmatrix(MATRIX);

static int realtime;
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
extern void (*plot)();
extern void (*standardplot)();
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
static int t;
static long l_dx,l_dy,l_dz,l_dt,l_a,l_b,l_c,l_d;
static long l_adt,l_bdt,l_cdt,l_xdt,l_ydt;
static long l_initx,l_inity,l_initz;
static long initorbitlong[3];

static double dx,dy,dz,dt,a,b,c,d;
static double adt,bdt,cdt,xdt,ydt;
static double initx,inity,initz;
static double initorbit[3];
extern int inside;

extern int  alloc_resume(int,int);
extern int  start_resume();
extern void end_resume();
extern int  put_resume(int len, ...);
extern int  get_resume(int len, ...);
extern int  calc_status, resuming;
extern int  diskisactive;
extern int  resave_flag;
extern char savename[];

/* these are potential user parameters */
int connect = 1;    /* flag to connect points with a line */
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

static int setup_convert_to_screen(struct affine *scrn_cnvt)
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

/******************************************************************/
/*   setup functions - put in fractalspecific[fractype].per_image */
/******************************************************************/

double orbit;
long   l_orbit;

extern double sinx,cosx;
long l_sinx,l_cosx;

int orbit3dlongsetup()
{
   connect = 1;
   waste = 100;
   projection = 2;
   if(fractype==LHENON || fractype==KAM || fractype==KAM3D)
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
	    || fractype == KAMFP || fractype == KAM3DFP)
      connect=0;
   if(fractype==FPROSSLER)
      waste = 500;
   if(fractype==FPLORENZ)
      projection = 1; /* plot x and z */

   initorbit[0] = 1;  /* initial conditions */
   initorbit[1] = 1;
   initorbit[2] = 1;
   if(fractype==FPGINGERBREAD)
   {
      initorbit[0] = -.1;  /* initial conditions */
      initorbit[1] = 0;
   }
   if(fractype==FPHENON || fractype==FPPICKOVER)
   {
      a =  param[0];
      b =  param[1];
      c =  param[2];
      d =  param[3];
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

int lorenz3dfloatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;
      dx  = -adt*(*x) + adt*(*y);
      dy  =  bdt*(*x) - ydt - (*z)*xdt;
      dz  = -cdt*(*z) + (*x)*ydt;

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


/**********************************************************************/
/*   Main fractal engines - put in fractalspecific[fractype].calctype */
/**********************************************************************/

int orbit2dfloat()
{
   double *soundvar;
   double x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   double *p0,*p1,*p2;
   struct affine cvt;

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
	     return(-1);
      }

      if (++count > 1000)
      {        /* time to switch colors? */
	 count = 0;
	 if (++color >= colors)   /* another color to switch to? */
	      color = 1;	/* (don't use the background color) */
      }

      col = cvt.a*x + cvt.b*y + cvt.e;
      row = cvt.c*x + cvt.d*y + cvt.f;
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
	 if (soundflag > 0)
	   snd((int)(*soundvar*100+basehertz));

	 if(oldcol != -1 && connect)
	    draw_line(col,row,oldcol,oldrow,color&(colors-1));
	 else
	    (*plot)(col,row,color&(colors-1));
	 oldcol = col;
	 oldrow = row;
      }
      else
	 oldrow = oldcol = -1;

      if(fractalspecific[fractype].orbitcalc(p0, p1, p2))
	 break;
   }
   return(0);
}

int orbit2dlong()
{
   long a,b,c,d,e,f;
   long *soundvar;
   long x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   long *p0,*p1,*p2;
   struct affine cvt;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);
   a = cvt.a*fudge; b = cvt.b*fudge; c = cvt.c*fudge;
   d = cvt.d*fudge; e = cvt.e*fudge; f = cvt.f*fudge;

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
	 return(-1);
      }
      if (++count > 1000)
      {        /* time to switch colors? */
	 count = 0;
	 if (++color >= colors)   /* another color to switch to? */
	      color = 1;	/* (don't use the background color) */
      }

      col = (multiply(a,x,bitshift) + multiply(b,y,bitshift) + e) >> bitshift;
      row = (multiply(c,x,bitshift) + multiply(d,y,bitshift) + f) >> bitshift;
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
	 else
	    (*plot)(col,row,color&(colors-1));
	 oldcol = col;
	 oldrow = row;
      }
      else
	 oldrow = oldcol = -1;

      /* Calculate the next point */
      if(fractalspecific[fractype].orbitcalc(p0, p1, p2))
	 break;
   }
   return(0);
}

int orbit3dlongcalc()
{
   long a,b,c,d,e,f;
   unsigned count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;

   double tmpx, tmpy, tmpz;
   long iview[3];	  /* perspective viewer's coordinates */

   long tmp;
   long maxvals[3];
   long minvals[3];
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   int i,j,k;

   /* 3D viewing stuff */
   MATRIX doublemat;	       /* transformation matrix */
   MATRIX doublemat1;		/* transformation matrix */
   long longmat[4][4];	       /* long version of matrix */
   long longmat1[4][4]; 	/* long version of matrix */
   long orbit[3];		   /* interated function orbit value */
   long viewvect[3];	    /* orbit transformed for viewing */
   long viewvect1[3];	     /* orbit transformed for viewing */

   struct affine cvt;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);
   a = cvt.a*fudge; b = cvt.b*fudge; c = cvt.c*fudge;
   d = cvt.d*fudge; e = cvt.e*fudge; f = cvt.f*fudge;

   oldcol1 = oldrow1 = oldcol = oldrow = -1;
   color = 2;
   if(color >= colors)
      color = 1;

   orbit[0] = initorbitlong[0];
   orbit[1] = initorbitlong[1];
   orbit[2] = initorbitlong[2];

   for(i=0;i<3;i++)
   {
      minvals[i] =  1L << 30;
      maxvals[i] = -minvals[i];
   }

   setupmatrix(doublemat);
   if(realtime)
      setupmatrix(doublemat1);

   /* copy xform matrix to long for for fixed point math */
   for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
      {
	 longmat[i][j] = doublemat[i][j] * fudge;
	 if(realtime)
	    longmat1[i][j] = doublemat1[i][j] * fudge;
      }
   if(diskvideo)		/* this would KILL a disk drive! */
   {
      notdiskmsg();
      return(-1);
   }

   /* make maxct a function of screen size		 */
   maxct = maxit*40L;
   count = ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
	 count = 0;
	 if (++color >= colors)   /* another color to switch to? */
	    color = 1;	      /* (don't use the background color) */
      }
      if(check_key())
      {
	 nosnd();
	 return(-1);
      }

      fractalspecific[fractype].orbitcalc(&orbit[0],&orbit[1],&orbit[2]);

      /* 3D VIEWING TRANSFORM */
      longvmult(orbit,longmat,viewvect, bitshift);
      if(realtime)
	 longvmult(orbit,longmat1,viewvect1, bitshift);

      if(ct <= waste) /* waste this many points to find minz and maxz */
      {
	 /* find minz and maxz */
	 for(i=0;i<3;i++)
	    if ((tmp = viewvect[i]) < minvals[i])
	       minvals[i] = tmp;
	    else if (tmp > maxvals[i])
	       maxvals[i] = tmp;
	 if(ct == waste)
	 {
	    iview[0] = iview[1] = 0L; /* center viewer on origin */

	    /* z value of user's eye - should be more negative than extreme
			   negative part of image */
	    iview[2] = (long)((minvals[2]-maxvals[2])*(double)ZVIEWER/100.0);

	    /* center image on origin */
	    tmpx = (-minvals[0]-maxvals[0])/(2.0*fudge); /* center x */
	    tmpy = (-minvals[1]-maxvals[1])/(2.0*fudge); /* center y */

	    /* apply perspective shift */
	    tmpx += ((double)xshift*(xxmax-xxmin))/(xdots)  ;
	    tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	    tmpz = -((double)maxvals[2]);
	    tmpz /= fudge;
	    trans(tmpx,tmpy,tmpz,doublemat);

	    if(realtime)
	    {
	       /* center image on origin */
	       tmpx = (-minvals[0]-maxvals[0])/(2.0*fudge); /* center x */
	       tmpy = (-minvals[1]-maxvals[1])/(2.0*fudge); /* center y */

	       tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	       tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	       tmpz = -((double)maxvals[2]);
	       tmpz /= fudge;
	       trans(tmpx,tmpy,tmpz,doublemat1);
	    }
	    for(i=0;i<3;i++)
	    {
	       view[i] = iview[i];
	       view[i] /= fudge;
	    }

	    /* copy xform matrix to long for for fixed point math */
	    for (i = 0; i < 4; i++)
	       for (j = 0; j < 4; j++)
	       {
		  longmat[i][j] = doublemat[i][j] * fudge;
		  if(realtime)
		     longmat1[i][j] = doublemat1[i][j] * fudge;
	       }
	 }
      }

      if(ct > waste)
      {
	 /* apply perspective if requested */
	 if(ZVIEWER)
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
	       if(realtime)
	       {
		  for(i=0;i<3;i++)
		  {
		     tmpv[i] = viewvect1[i];
		     tmpv[i] /= fudge;
		  }
		  perspective(tmpv);
		  for(i=0;i<3;i++)
		     viewvect1[i] = tmpv[i]*fudge;
	       }
	    }
	    else
	    {
	       longpersp(viewvect,iview,bitshift);
	       if(realtime)
		  longpersp(viewvect1,iview,bitshift);
	    }
	 }
	 if(realtime)
	    whichimage=1;
	 /* plot if inside window */
	 col = (multiply(a,viewvect[0],bitshift)+
		multiply(b,viewvect[1],bitshift)+e) >> bitshift;
	 row = (multiply(c,viewvect[0],bitshift)+
		multiply(d,viewvect[1],bitshift)+f) >> bitshift;
	 col += xxadjust;
	 row += yyadjust;
	 if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	 {
	    if (soundflag > 0)
	    {
	       double yy;
	       yy = viewvect[soundflag-1];
	       yy = yy/fudge;
	       snd((int)(yy*100+basehertz));
	    }
	    if(oldcol != -1 && connect)
	       draw_line(col,row,oldcol,oldrow,color&(colors-1));
	    else
	       (*plot)(col,row,color&(colors-1));
	    oldcol = col;
	    oldrow = row;
	 }
	 else
	    oldrow = oldcol = -1;
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    col = (multiply(a,viewvect1[0],bitshift)+
		   multiply(b,viewvect1[1],bitshift)+e) >> bitshift;
	    row = (multiply(c,viewvect1[0],bitshift)+
		   multiply(d,viewvect1[1],bitshift)+f) >> bitshift;
	    col += xxadjust1;
	    row += yyadjust1;

	    if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	    {
	       if(oldcol1 != -1 && connect)
		  draw_line(col,row,oldcol1,oldrow1,color&(colors-1));
	       else
		  (*plot)(col,row,color&(colors-1));
	       oldcol1 = col;
	       oldrow1 = row;
	    }
	    else
	       oldrow1 = oldcol1 = -1;
	 }
      }
   }
   return(0);
}


int orbit3dfloatcalc()
{
   unsigned count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;

   double tmpx, tmpy, tmpz;
   extern VECTOR view;
   double tmp;
   double maxvals[3];
   double minvals[3];
   extern int init3d[];
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;


   int i,j,k;

   /* 3D viewing stuff */
   MATRIX doublemat;	       /* transformation matrix */
   MATRIX doublemat1;		/* transformation matrix */
   double orbit[3];		   /* interated function orbit value */
   double viewvect[3];	      /* orbit transformed for viewing */
   double viewvect1[3];        /* orbit transformed for viewing */
   struct affine cvt;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   oldcol = oldrow = -1;
   oldcol1 = oldrow1 = -1;
   color = 2;
   if(color >= colors)
      color = 1;
   orbit[0] = initorbit[0];
   orbit[1] = initorbit[1];
   orbit[2] = initorbit[2];

   for(i=0;i<3;i++)
   {
      minvals[i] =  100000.0; /* impossible value */
      maxvals[i] = -100000.0;
   }

   setupmatrix(doublemat);
   if(realtime)
      setupmatrix(doublemat1);

   if(diskvideo)		/* this would KILL a disk drive! */
   {
      notdiskmsg();
      return(-1);
   }

   maxct = maxit*40L;
   count = ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
	 count = 0;
	 if (++color >= colors)   /* another color to switch to? */
	    color = 1;	      /* (don't use the background color) */
      }

      if(check_key())
      {
	 nosnd();
	 return(-1);
      }

      fractalspecific[fractype].orbitcalc(&orbit[0],&orbit[1],&orbit[2]);

      /* 3D VIEWING TRANSFORM */
      vmult(orbit,doublemat,viewvect );
      if(realtime)
	 vmult(orbit,doublemat1,viewvect1);

      if(ct <= waste) /* waste this many points to find minz and maxz */
      {
	 /* find minz and maxz */
	 for(i=0;i<3;i++)
	    if ((tmp = viewvect[i]) < minvals[i])
	       minvals[i] = tmp;
	    else if (tmp > maxvals[i])
	       maxvals[i] = tmp;
	 if(ct == waste)
	 {
	    view[0] = view[1] = 0; /* center on origin */
	    /* z value of user's eye - should be more negative than extreme
			   negative part of image */
	    view[2] = (minvals[2]-maxvals[2])*(double)ZVIEWER/100.0;

	    /* center image on origin */
	    tmpx = (-minvals[0]-maxvals[0])/(2.0); /* center x */
	    tmpy = (-minvals[1]-maxvals[1])/(2.0); /* center y */

	    /* apply perspective shift */
	    tmpx += ((double)xshift*(xxmax-xxmin))/(xdots)  ;
	    tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	    tmpz = -((double)maxvals[2]);
	    trans(tmpx,tmpy,tmpz,doublemat);

	    if(realtime)
	    {
	       /* center image on origin */
	       tmpx = (-minvals[0]-maxvals[0])/(2.0); /* center x */
	       tmpy = (-minvals[1]-maxvals[1])/(2.0); /* center y */

	       tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	       tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	       tmpz = -((double)maxvals[2]);
	       trans(tmpx,tmpy,tmpz,doublemat1);
	    }
	 }
      }

      if(ct > waste)
      {
	 /* apply perspective if requested */
	 if(ZVIEWER)
	 {
	    perspective(viewvect);
	    if(realtime)
	       perspective(viewvect1);
	 }
	 if(realtime)
	    whichimage=1;
	 /* plot if inside window */
	 col = cvt.a*viewvect[0] + cvt.b*viewvect[1] + cvt.e + xxadjust;
	 row = cvt.c*viewvect[0] + cvt.d*viewvect[1] + cvt.f + yyadjust;
	 if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	 {
	    if (soundflag > 0)
	       snd((int)(viewvect[soundflag-1]*100+basehertz));
	    if(oldcol != -1 && connect)
	       draw_line(col,row,oldcol,oldrow,color&(colors-1));
	    else
	       (*plot)(col,row,color&(colors-1));
	    oldcol = col;
	    oldrow = row;
	 }
	 else
	    oldrow = oldcol = -1;
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    col = cvt.a*viewvect1[0] + cvt.b*viewvect1[1] + cvt.e + xxadjust1;
	    row = cvt.c*viewvect1[0] + cvt.d*viewvect1[1] + cvt.f + yyadjust1;
	    if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	    {
	       if(oldcol1 != -1 && connect)
		  draw_line(col,row,oldcol1,oldrow1,color&(colors-1));
	       else
		  (*plot)(col,row,color&(colors-1));
	       oldcol1 = col;
	       oldrow1 = row;
	    }
	    else
	       oldrow1 = oldcol1 = -1;
	 }
      }
   }
   return(0);
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
	       if (resave_flag == 2)
		  resave_flag = 0;
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
   double tmp;
   double tmpx;
   double tmpy;
   double tmpz;
   VECTOR minvals;
   VECTOR maxvals;
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   double newx,newy,newz,r,sum;

   int i,k;

   /* 3D viewing stuff */
   MATRIX doublemat;	       /* transformation matrix */
   MATRIX doublemat1;		/* transformation matrix */
   VECTOR orbitvect;		   /* interated function orbit value */
   VECTOR viewvect;	   /* orbit transformed for viewing */
   VECTOR viewvect1;	    /* orbit transformed for viewing */
   struct affine cvt;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   srand(1);
   for(i=0;i<3;i++)
   {
      minvals[i] =  100000.0; /* impossible value */
      maxvals[i] = -100000.0;
   }

   setupmatrix(doublemat);
   if(realtime)
      setupmatrix(doublemat1);

   if(diskvideo)		/* this would KILL a disk drive! */
   {
      notdiskmsg();
      return(-1);
   }

   orbitvect[0] = 0;
   orbitvect[1] = 0;
   orbitvect[2] = 0;

   maxct = maxit*40L;
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      if (ct & 127)	      /* reduce the number of keypress checks */
	 if( check_key() )    /* keypress bails out */
	    return(-1);
      r = rand();	 /* generate fudged random number between 0 and 1 */
      r /= 32767;

      /* pick which iterated function to execute, weighted by probability */
      sum = initifs3d[0][12];
      k = 0;
      while ( sum < r)
      {
	 k++;
	 sum += initifs3d[k][12];
	 if (initifs3d[k+1][12] == 0) break;	/* for safety */
      }

      /* calculate image of last point under selected iterated function */
      newx = initifs3d[k][0] * orbitvect[0] +
	  initifs3d[k][1] * orbitvect[1] +
	  initifs3d[k][2] * orbitvect[2] + initifs3d[k][9];
      newy = initifs3d[k][3] * orbitvect[0] +
	  initifs3d[k][4] * orbitvect[1] +
	  initifs3d[k][5] * orbitvect[2] + initifs3d[k][10];
      newz = initifs3d[k][6] * orbitvect[0] +
	  initifs3d[k][7] * orbitvect[1] +
	  initifs3d[k][8] * orbitvect[2] + initifs3d[k][11];

      orbitvect[0] = newx;
      orbitvect[1] = newy;
      orbitvect[2] = newz;

      /* 3D VIEWING TRANSFORM */
      vmult(orbitvect,doublemat,viewvect);
      if(realtime)
	 vmult(orbitvect,doublemat1,viewvect1);

      /* find minz and maxz */
      if(ct <= waste) /* waste this many points to find minz and maxz */
      {
	 for(i=0;i<3;i++)
	    if ((tmp = viewvect[i]) < minvals[i])
	       minvals[i] = tmp;
	    else if (tmp > maxvals[i])
	       maxvals[i] = tmp;
	 if(ct == waste)
	 {
	    /* center viewer position on object */
	    view[0] = view[1] = 0; /* center on origin */

	    /* z value of user's eye - should be more negative than extreme
			   negative part of image */
	    view[2] = (minvals[2]-maxvals[2])*(double)ZVIEWER/100.0;

	    /* center image on origin */
	    tmpx = (-minvals[0]-maxvals[0])/(2.0); /* center x */
	    tmpy = (-minvals[1]-maxvals[1])/(2.0); /* center y */

	    /* apply perspective shift */
	    tmpx += ((double)xshift*(xxmax-xxmin))/(xdots)  ;
	    tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	    tmpz = -((double)maxvals[2]);
	    trans(tmpx,tmpy,tmpz,doublemat);

	    if(realtime)
	    {
	       /* center image on origin */
	       tmpx = (-minvals[0]-maxvals[0])/(2.0); /* center x */
	       tmpy = (-minvals[1]-maxvals[1])/(2.0); /* center y */

	       tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	       tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	       tmpz = -((double)maxvals[2]);
	       trans(tmpx,tmpy,tmpz,doublemat1);
	    }
	 }
      }


      if(ct > waste)
      {
	 /* apply perspective if requested */
	 if(ZVIEWER)
	 {
	    perspective(viewvect);
	    if(realtime)
	       perspective(viewvect1);
	 }
	 if(realtime)
	    whichimage=1;
	 /* plot if inside window */
	 col = cvt.a*viewvect[0] + cvt.b*viewvect[1] + cvt.e + xxadjust;
	 row = cvt.c*viewvect[0] + cvt.d*viewvect[1] + cvt.f + yyadjust;
	 if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	 {
	    color = getcolor(col,row)+1;
	    if( color < colors ) /* color sticks on last value */
	       (*plot)(col,row,color);
	 }
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    col = cvt.a*viewvect1[0] + cvt.b*viewvect1[1] + cvt.e + xxadjust1;
	    row = cvt.c*viewvect1[0] + cvt.d*viewvect1[1] + cvt.f + yyadjust1;
	    if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	    {
	       color = getcolor(col,row)+1;
	       if( color < colors ) /* color sticks on last value */
		  (*plot)(col,row,color);
	    }
	 }
      }
   } /* end while */
   return(0);
}

int ifs()	/* IFS logic shamelessly converted to integer math */
{
   long a,b,c,d,e,f;
   long  *lifsptr;
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   long localifs[NUMIFS][7];	    /* local IFS values */
   long x,y,newx,newy,r,sum, tempr;

   int i,j,k;
   struct affine cvt;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);
   a = cvt.a*fudge; b = cvt.b*fudge; c = cvt.c*fudge;
   d = cvt.d*fudge; e = cvt.e*fudge; f = cvt.f*fudge;

   srand(1);

   if(diskvideo) {		/* this would KILL a disk drive! */
	notdiskmsg();
	return(-1);
	}

   for (i = 0; i < NUMIFS; i++)    /* fill in the local IFS array */
   for (j = 0; j < IFSPARM; j++)
	 localifs[i][j] = initifs[i][j] * fudge;

   tempr = fudge / 32767;	 /* find the proper rand() fudge */

   /* make maxct a function of screen size		 */
   /* 1k times maxit at EGA resolution seems about right */
   maxct = (float)maxit*(1024.0*xdots*ydots)/(640.0*350.0);
   ct = 0L;
   x = y = 0;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      if (ct & 127)	      /* reduce the number of keypress checks */
	 if( check_key() )    /* keypress bails out */
	    return(-1);
      r = rand();	 /* generate fudged random number between 0 and 1 */
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
      col = (multiply(a,x,bitshift) + multiply(b,y,bitshift) + e) >> bitshift;
      row = (multiply(c,x,bitshift) + multiply(d,y,bitshift) + f) >> bitshift;
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
	 /* color is count of hits on this pixel */
	 color = getcolor(col,row)+1;
	 if( color < colors ) /* color sticks on last value */
	    (*plot)(col,row,color);
      }
   }
   return(0);
}

static int ifs3dlong()
{
   long a,b,c,d,e,f;
   double tmpx, tmpy, tmpz;
   extern VECTOR view;
   long iview[3];	  /* perspective viewer's coordinates */
   long tmp;
   long maxvals[3];
   long minvals[3];
   extern int init3d[];
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   long localifs[NUMIFS][IFS3DPARM];	    /* local IFS values */
   long newx,newy,newz,r,sum, tempr;

   int i,j,k;

   /* 3D viewing stuff */
   MATRIX doublemat;	       /* transformation matrix */
   MATRIX doublemat1;		/* transformation matrix */
   long longmat[4][4];	       /* long version of matrix */
   long longmat1[4][4]; 	/* long version of matrix */
   long orbitvect[3];		   /* interated function orbit value */
   long viewvect[3];	    /* orbit transformed for viewing */
   long viewvect1[3];	     /* orbit transformed for viewing */
   struct affine cvt;

   srand(1);

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);
   a = cvt.a*fudge; b = cvt.b*fudge; c = cvt.c*fudge;
   d = cvt.d*fudge; e = cvt.e*fudge; f = cvt.f*fudge;

   for(i=0;i<3;i++)
   {
      minvals[i] =  1L << 30;
      maxvals[i] = -minvals[i];
   }

   setupmatrix(doublemat);
   if(realtime)
      setupmatrix(doublemat1);

   /* copy xform matrix to long for for fixed point math */
   for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
      {
	 longmat[i][j] = doublemat[i][j] * fudge;
	 if(realtime)
	    longmat1[i][j] = doublemat1[i][j] * fudge;
      }
   if(diskvideo)		/* this would KILL a disk drive! */
   {
      notdiskmsg();
      return(-1);
   }

   for (i = 0; i < NUMIFS; i++)    /* fill in the local IFS array */
      for (j = 0; j < IFS3DPARM; j++)
	 localifs[i][j] = initifs3d[i][j] * fudge;

   tempr = fudge / 32767;	 /* find the proper rand() fudge */

   orbitvect[0] = 0;
   orbitvect[1] = 0;
   orbitvect[2] = 0;

   maxct = maxit*40L;
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */
   {
      if (ct & 127)	      /* reduce the number of keypress checks */
	 if( check_key() )    /* keypress bails out */
	    return(-1);
      r = rand();	 /* generate fudged random number between 0 and 1 */
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
      newx = multiply(localifs[k][0], orbitvect[0], bitshift) +
	  multiply(localifs[k][1], orbitvect[1], bitshift) +
	  multiply(localifs[k][2], orbitvect[2], bitshift) + localifs[k][9];
      newy = multiply(localifs[k][3], orbitvect[0], bitshift) +
	  multiply(localifs[k][4], orbitvect[1], bitshift) +
	  multiply(localifs[k][5], orbitvect[2], bitshift) + localifs[k][10];
      newz = multiply(localifs[k][6], orbitvect[0], bitshift) +
	  multiply(localifs[k][7], orbitvect[1], bitshift) +
	  multiply(localifs[k][8], orbitvect[2], bitshift) + localifs[k][11];

      orbitvect[0] = newx;
      orbitvect[1] = newy;
      orbitvect[2] = newz;

      /* 3D VIEWING TRANSFORM */
      longvmult(orbitvect,longmat,viewvect, bitshift);
      if(realtime)
	 longvmult(orbitvect,longmat1,viewvect1, bitshift);

      if(ct <= waste) /* waste this many points to find minz and maxz */
      {
	 /* find minz and maxz */
	 for(i=0;i<3;i++)
	    if ((tmp = viewvect[i]) < minvals[i])
	       minvals[i] = tmp;
	    else if (tmp > maxvals[i])
	       maxvals[i] = tmp;
	 if(ct == waste)
	 {
	    iview[0] = iview[1] = 0L; /* center viewer on origin */

	    /* z value of user's eye - should be more negative than extreme
			   negative part of image */
	    iview[2] = (long)((minvals[2]-maxvals[2])*(double)ZVIEWER/100.0);

	    /* center image on origin */
	    tmpx = (-minvals[0]-maxvals[0])/(2.0*fudge); /* center x */
	    tmpy = (-minvals[1]-maxvals[1])/(2.0*fudge); /* center y */

	    /* apply perspective shift */
	    tmpx += ((double)xshift*(xxmax-xxmin))/(xdots)  ;
	    tmpy += ((double)yshift*(yymax-yymin))/(ydots);
	    tmpz = -((double)maxvals[2]);
	    tmpz /= fudge;
	    trans(tmpx,tmpy,tmpz,doublemat);

	    if(realtime)
	    {
	       /* center image on origin */
	       tmpx = (-minvals[0]-maxvals[0])/(2.0*fudge); /* center x */
	       tmpy = (-minvals[1]-maxvals[1])/(2.0*fudge); /* center y */

	       tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
	       tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
	       tmpz = -((double)maxvals[2]);
	       tmpz /= fudge;
	       trans(tmpx,tmpy,tmpz,doublemat1);
	    }

	    for(i=0;i<3;i++)
	    {
	       view[i] = iview[i];
	       view[i] /= fudge;
	    }

	    /* copy xform matrix to long for for fixed point math */
	    for (i = 0; i < 4; i++)
	       for (j = 0; j < 4; j++)
	       {
		  longmat[i][j] = doublemat[i][j] * fudge;
		  if(realtime)
		     longmat1[i][j] = doublemat1[i][j] * fudge;
	       }
	 }
      }
      if(ct > waste)
      {
	 /* apply perspective if requested */
	 if(ZVIEWER && ct > waste)
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
	       if(realtime)
	       {
		  for(i=0;i<3;i++)
		  {
		     tmpv[i] = viewvect1[i];
		     tmpv[i] /= fudge;
		  }
		  perspective(tmpv);
		  for(i=0;i<3;i++)
		     viewvect1[i] = tmpv[i]*fudge;
	       }
	    }
	    else
	    {
	       longpersp(viewvect,iview,bitshift);
	       if(realtime)
		  longpersp(viewvect1,iview,bitshift);
	    }
	 }
	 if(realtime)
	 whichimage=1;
	 /* plot if inside window */
	 col = (multiply(a,viewvect[0],bitshift)+
		multiply(b,viewvect[1],bitshift)+e) >> bitshift;
	 row = (multiply(c,viewvect[0],bitshift)+
		multiply(d,viewvect[1],bitshift)+f) >> bitshift;
	 col += xxadjust;
	 row += yyadjust;

	 if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	 {
	    color = getcolor(col,row)+1;
	    if( color < colors ) /* color sticks on last value */
	       (*plot)(col,row,color);
	 }
	 if(realtime)
	 {
	    whichimage=2;
	    /* plot if inside window */
	    col = (multiply(a,viewvect1[0],bitshift)+
		   multiply(b,viewvect1[1],bitshift)+e) >> bitshift;
	    row = (multiply(c,viewvect1[0],bitshift)+
		   multiply(d,viewvect1[1],bitshift)+f) >> bitshift;
	    col += xxadjust1;
	    row += yyadjust1;
	    if ( 0 <= col && col < xdots && 0 <= row && row < ydots)
	    {
	       color = getcolor(col,row)+1;
	       if( color < colors ) /* color sticks on last value */
		  (*plot)(col,row,color);
	    }
	 }
      }
   }
   return(0);
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

