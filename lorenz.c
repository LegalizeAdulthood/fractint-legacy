/* 
   This file contains two 3 dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images. Tim Wegner 
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fractint.h"
#include "fractype.h"

int realtime;
extern int soundflag;
extern int basehertz;
extern int solidguessing;
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
extern int XXdots, YYdots; /* local dots */
extern int	debugflag;			/* for debugging purposes */
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	maxit;				/* try this many iterations */
extern double param[];
extern double xxmin,xxmax,yymin,yymax;         /* corners */
extern char far plasmamessage[];
extern	int	diskvideo;			/* for disk-video klooges */
extern double deltaX, deltaY;
extern int	bitshift;			/* bit shift for fudge */
extern long	fudge;				/* fudge factor (2**n) */
extern int	colors;				/* maximum colors available */

extern int display3d;
static long l_dx,l_dy,l_dz,l_dt,l_a,l_b,l_c,l_d;
static long l_adt,l_bdt,l_cdt,l_xdt,l_ydt;
static long l_initx,l_inity,l_initz;
static long initorbitlong[3];

static double dx,dy,dz,dt,a,b,c,d;
static double adt,bdt,cdt,xdt,ydt;
static double initx,inity,initz;
static double initorbit[3];
extern int inside;

/* these are potential user parameters */
int connect = 1;    /* flag to connect potints with a line */
int waste = 100;    /* waste this many points before plotting */ 
int projection = 2; /* projection plane - default is to plot x-y */

/******************************************************************/
/*   setup functions - put in fractalspecific[fractype].per_image */
/******************************************************************/

orbit3dlongsetup()
{
   connect = 1;
   waste = 100;
   projection = 2;
   if(fractype==LHENON)
      connect=0;
   if(fractype==LROSSLER)
      waste = 500;
   if(fractype==LLORENZ)
      projection = 1;

   fudge = 1L << bitshift;
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
   solidguessing = 0;
   return(1);
}

orbit3dfloatsetup()
{
   connect = 1;
   waste = 100;
   projection = 2;

   if(fractype==FPHENON || fractype==FPPICKOVER || fractype==FPGINGERBREAD)
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
   
   solidguessing = 0;
   return(1);
}

/******************************************************************/
/*   orbit functions - put in fractalspecific[fractype].orbitcalc */
/******************************************************************/

lorenz3dlongorbit(long *l_x, long *l_y, long *l_z)
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

lorenz3dfloatorbit(double *x, double *y, double *z)
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

henonfloatorbit(double *x, double *y, double *z)
{
      double newx,newy;
      newx  = 1 + *y - a*(*x)*(*x);
      newy  = b*(*x);
      *x = newx;
      *y = newy;
      return(0);
}

henonlongorbit(long *l_x, long *l_y, long *l_z)
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
 
rosslerfloatorbit(double *x, double *y, double *z)
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

pickoverfloatorbit(double *x, double *y, double *z)
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
gingerbreadfloatorbit(double *x, double *y, double *z)
{
      double newx;
      newx = 1 - (*y) + fabs(*x);
      *y = *x;
      *x = newx;
      return(0);
}

rosslerlongorbit(long *l_x, long *l_y, long *l_z)
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

/**********************************************************************/
/*   Main fractal engines - put in fractalspecific[fractype].calctype */
/**********************************************************************/

orbit2dfloat() 
{
   double *soundvar;
   double x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   double *p0,*p1,*p2; 

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
   
   while(1)
   {
      if (++count > 1000) 
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
              color = 1;        /* (don't use the background color) */
      }

      if(check_key())
      {
     	 nosnd();
         return(-1);
      }
      if ( x > xxmin && x < xxmax && y > yymin && y < yymax )
      {
         if (soundflag > 0)
           snd((int)(*soundvar*100+basehertz));
         col =          (( x-xxmin) / deltaX);
         row = YYdots - (( y-yymin) / deltaY);
         if(oldcol != -1 && connect)
            draw_line(col,row,oldcol,oldrow,color&(colors-1));
         else            
            (*plot)(col,row,color&(colors-1));
         oldcol = col;
         oldrow = row;    
      }
      else
         oldrow = oldcol = -1;

      fractalspecific[fractype].orbitcalc(p0, p1, p2);
      
   }
   return(0); 
}

orbit2dlong() 
{
   long *soundvar;
   long x,y,z;
   int color,col,row;
   int count;
   long delx,dely,xmin,xmax,ymin,ymax;
   int oldrow, oldcol;
   long *p0,*p1,*p2; 

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
   fudge = 1L<<bitshift;
   delx = deltaX*fudge;
   dely = deltaY*fudge;
   xmin = xxmin*fudge;
   ymin = yymin*fudge;
   xmax = xxmax*fudge;
   ymax = yymax*fudge;

   while(1)
   {
      if (++count > 1000) 
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
              color = 1;        /* (don't use the background color) */
      }
      if(check_key())
      {
     	 nosnd();
         return(-1);
      }
      if ( x > xmin && x < xmax && y > ymin && y < ymax )
      {
         if (soundflag > 0)
         {
            double yy;
            yy = *soundvar;
            yy = yy/fudge;
            snd((int)(yy*100+basehertz));
         }  
         col =          (( x-xmin) / delx);
         row = YYdots - (( y-ymin) / dely);
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
      fractalspecific[fractype].orbitcalc(p0, p1, p2);
   }
   return(0); 
}


orbit3dlongcalc()
{
   unsigned count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;
   long delx,dely;

   double tmpx, tmpy, tmpz;
   long iview[3];         /* perspective viewer's coordinates */

   long tmp;   
   long maxvals[3];
   long minvals[3];
   unsigned long maxct,ct;
   register int col;
   register int row;
   register int color;

   long xmin, xmax, ymin, ymax;

   int i,j,k;

   /* 3D viewing stuff */
   MATRIX doublemat;           /* transformation matrix */
   MATRIX doublemat1;           /* transformation matrix */
   long longmat[4][4];         /* long version of matrix */
   long longmat1[4][4];         /* long version of matrix */
   long orbit[3];	           /* interated function orbit value */
   long viewvect[3];        /* orbit transformed for viewing */
   long viewvect1[3];        /* orbit transformed for viewing */

   oldcol1 = oldrow1 = oldcol = oldrow = -1;
   color = 2;
   if(color >= colors)
      color = 1;   
   fudge = 1L << bitshift;

   orbit[0] = initorbitlong[0];
   orbit[1] = initorbitlong[1];
   orbit[2] = initorbitlong[2];

   delx = deltaX*fudge;
   dely = deltaY*fudge;

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
      setvideomode(3,0,0,0);
      buzzer(2);
      helpmessage(plasmamessage);
      return(-1);
   }

   xmin  = xxmin*fudge;   
   xmax  = xxmax*fudge;
   ymax  = yymax*fudge;
   ymin  = yymin*fudge;

   /* make maxct a function of screen size               */
   maxct = maxit*40L;
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */ 
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
         col =          (( viewvect[0]-xmin) / delx) + xxadjust;
         row = YYdots - (( viewvect[1]-ymin) / dely) + yyadjust;
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
            col =          (( viewvect1[0]-xmin) / delx) + xxadjust1;
            row = YYdots - (( viewvect1[1]-ymin) / dely) + yyadjust1;
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


orbit3dfloatcalc()
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
   MATRIX doublemat;           /* transformation matrix */
   MATRIX doublemat1;           /* transformation matrix */
   double orbit[3];	           /* interated function orbit value */
   double viewvect[3];        /* orbit transformed for viewing */
   double viewvect1[3];        /* orbit transformed for viewing */
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
      setvideomode(3,0,0,0);
      buzzer(2);
      helpmessage(plasmamessage);
      return(-1);
   }

   maxct = maxit*40L;
   ct = 0L;
   while(ct++ < maxct) /* loop until keypress or maxit */ 
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
         col =          (( viewvect[0]-xxmin) / deltaX) + xxadjust;
         row = YYdots - (( viewvect[1]-yymin) / deltaY) + yyadjust;
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
            col =          (( viewvect1[0]-xxmin) / deltaX) + xxadjust1;
            row = YYdots - (( viewvect1[1]-yymin) / deltaY) + yyadjust1;
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
funny_glasses_call(int (*calc)())
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
   if(glassestype && status == 0)
   {
      if(glassestype==3) /* photographer's mode */
      {
         setfortext();
         printf("First image (left eye) is ready - hit any key to see it\n");
         printf("Then hit any key again to create second image\n");
         getch();
         setforgraphics();
         getch();
         /* is there a better way to clear the screen in graphics mode? */
         setvideomode(videoentry.videomodeax,
             videoentry.videomodebx,
             videoentry.videomodecx,
             videoentry.videomodedx);
      }
      whichimage = 2;
      plot_setup();
      plot = standardplot;
      /* is there a better way to clear the graphics screen ? */
      if(status = calc())
         return(status);
      if(glassestype==3) /* photographer's mode */
      {
         setfortext();
         printf("Second image (right eye) is ready - hit any key to see it\n");
         getch();
         setforgraphics();
      }   
   } 
   return(status);
}

/* double version - mainly for testing */
ifs3dfloat()
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
   MATRIX doublemat;           /* transformation matrix */
   MATRIX doublemat1;           /* transformation matrix */
   VECTOR orbitvect;	           /* interated function orbit value */
   VECTOR viewvect;        /* orbit transformed for viewing */
   VECTOR viewvect1;        /* orbit transformed for viewing */

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
      setvideomode(3,0,0,0);
      buzzer(2);
      helpmessage(plasmamessage);
      return(-1);
   }

   orbitvect[0] = 0;
   orbitvect[1] = 0;
   orbitvect[2] = 0;

   maxct = maxit*40L;
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
         col =          (( viewvect[0]-xxmin) / deltaX) + xxadjust;
         row = YYdots - (( viewvect[1]-yymin) / deltaY) + yyadjust;
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
            col =          (( viewvect1[0]-xxmin) / deltaX) + xxadjust1;
            row = YYdots - (( viewvect1[1]-yymin) / deltaY) + yyadjust1;
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

ifs3dlong()
{
   int bitshift;   /* want these local */
   long fudge;
   long delx,dely;

   double tmpx, tmpy, tmpz;
   extern VECTOR view;
   long iview[3];         /* perspective viewer's coordinates */

   long tmp;   
   long maxvals[3];
   long minvals[3];
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
   MATRIX doublemat1;           /* transformation matrix */
   long longmat[4][4];         /* long version of matrix */
   long longmat1[4][4];         /* long version of matrix */
   long orbitvect[3];	           /* interated function orbit value */
   long viewvect[3];        /* orbit transformed for viewing */
   long viewvect1[3];        /* orbit transformed for viewing */

   srand(1);

   bitshift = 16;
   fudge = 1L << bitshift;

   delx = deltaX*fudge;
   dely = deltaY*fudge;

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

   orbitvect[0] = 0;
   orbitvect[1] = 0;
   orbitvect[2] = 0;

   maxct = maxit*40L;
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
         col =          (( viewvect[0]-xmin) / delx) + xxadjust;
         row = YYdots - (( viewvect[1]-ymin) / dely) + yyadjust;
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
            col =          (( viewvect1[0]-xmin) / delx) + xxadjust1;
            row = YYdots - (( viewvect1[1]-ymin) / dely) + yyadjust1;
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

setupmatrix(MATRIX doublemat)
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

orbit3dfloat()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;   
   return(funny_glasses_call(orbit3dfloatcalc));
}
orbit3dlong()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;   
   return(funny_glasses_call(orbit3dlongcalc));
}

ifs3d()
{
   display3d = -1;

   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;   
   if(floatflag)
      return(funny_glasses_call(ifs3dfloat)); /* double version of ifs3d */
   else
      return(funny_glasses_call(ifs3dlong));  /* long version of ifs3d   */
}
