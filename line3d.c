/* This file contains a 3D replacement for the out_line function called
   by the decoder. The purpose is to apply various 3D transformations before 
   displaying points. Called once line of the original GIF file. 
   
   Original Author Tim Wegner, with much help from the usual crew of speed
   demons.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <dos.h>
#include "fractint.h"

int clipcolor(int,int,int);   
int interpcolor(int,int,int);
extern int (*plot)();   
extern int xdots, ydots, colors;
extern int debugflag;
extern void draw_line (int X1, int Y1, int X2, int Y2, int color);
extern int extraseg;
extern unsigned height;
extern int rowcount; /* in general.asm */
extern int init3d[];			/* 3D arguments (FRACTINT.C) */
extern FILE *fp_pot;			/* potential file pointer */

static float far *sinarray;		/* all sines go here */
static float PISLICE;			/* cheap, one-time calc of PI/SLICE */
#define SLICE  4096			/* chop up the pie this many times */
#define SLICE4 1024			/* one fourth of the pie */

static double rXrscale;            /* precalculation factor */

static int persp;       /* flag for indicating perspective transformations */

/* this array remembers the previous line */
struct point
{
   int x;
   int y;
   int color;
} far *lastrow;
static struct point p1,p2,p3;
struct f_point
{
   float x;
   float y;
   float color;
} far *f_lastrow;

/* array of min and max x values used in triangle fill */
struct minmax 
{
    int minx;
    int maxx;
} far *minmax_x;
     
VECTOR view;    /* position of observer for perspective */

line3d(unsigned char pixels[], unsigned linelen)
{

   /* these values come from FRACTINT.C */

   /* These variables must preserve their values across calls */
   static float theta,theta1,theta2;  /* current,start,stop latitude */
   static float phi,phi1,phi2;        /* current start,stop longitude */
   static double aspect;              /* aspect ration */
   static int   deltatheta,deltaphi;  /* increment of latitude, longitude */
   static double rscale;              /* surface roughness factor */
   static long xcenter,ycenter;       /* circle center */
   static double sclx,scly,sclz;      /* scale factors */
   static double R;                   /* radius values */
   static double Rfactor;             /* for intermediate calculation */
   static MATRIX m;                   /* transformation matric */
   static long im[4][4];		      /* "" */
   static long iview[4];		      /* for perspective views */
   static double zcutoff;             /* perspective backside cutoff value */  
   
   double r;                          /* sphere radius */
   double xval,yval,zval;             /* rotation values */
   double costheta,sintheta;          /* precalculated sin/cos of latitude */
   double cosphi,sinphi;              /* precalculated sin/cos of longitude */
   int tmp_phi,tmp_theta;             /* temporary latitude/longitude */

   struct point cur;                  /* current pixels */
   struct point old;                  /* old pixels */
   static struct point bad;           /* out of range value */

   struct f_point f_cur;
   struct f_point f_old;
   static struct f_point f_bad;       /* out of range value */

   static float far *fpixels;         /* float persion of pixels array */
    
   VECTOR v;                          /* double vector */
   VECTOR v1,v2;
   VECTOR cross;
   VECTOR crossavg;
   static VECTOR light_direction;
   
   long iv[4];			              /* long equivalent of v */
   long iv0[4];			              /* long equivalent of v */
   int color;                         /* current decoded color */
   int col;                           /* current column (original GIF) */ 
   int showbox;                       /* flag for showing box */
   /* corners of transformed xdotx by ydots x colors box */ 
   double xmin, ymin, zmin, xmax, ymax, zmax; 
   int i,j;

   plot = clipcolor;  /* the usual FRACTINT plot function with clipping */
   
   /* 
      This IF clause is executed ONCE per image. All precalculations are 
      done here, with out any special concern about speed. DANGER - 
      communication with the rest of the program is generally via static
      or global variables.
   */   
   if(rowcount == 0)
   {
      long check_extra;       
      /* lastrow stores the previous row of the original GIF image for
         the purpose of filling in gaps with triangle procedure */
      if(extraseg)
      {
#ifdef __TURBOC__
         lastrow = MK_FP(extraseg,0);
#else         
         FP_SEG(lastrow)=extraseg;
         FP_OFF(lastrow)=0;
#endif      
      }else
         return(-1);

      /* stick this array a safe distance from end */
      /* this array used in triangle-fill */
      minmax_x = (struct minmax far *)(lastrow+MAXPIXELS);

      /* have used up 12300 of extra seg now */
      check_extra = sizeof(*lastrow)*(MAXPIXELS);
      /* printf("check extra = %ld\n",check_extra); */
      
      /* now stuff the sine and cosine arrays still farther out */
      sinarray = (float far *)(minmax_x+MAXPIXELS);

      /* have used up 16396 of extra seg now */
      check_extra = check_extra + sizeof(*sinarray)*SLICE4;
      /* printf("check extra = %ld\n",check_extra); */

      f_lastrow = (struct f_point far *)(sinarray+SLICE4);

      /* have used up 40996 of extra seg now */
      check_extra = check_extra + sizeof(*f_lastrow)*(MAXPIXELS+1);
      /* printf("check extra = %ld\n",check_extra); */

      fpixels = (float far *)(f_lastrow+MAXPIXELS);
      
      check_extra = check_extra + sizeof(float)*linelen;
      /* printf("check extra = %ld\n",check_extra); */

      /* (fill in the sine array with  <invalid> 2.0's) */
      if (SPHERE) 
      {
         for (i = 0; i < SLICE4; i++) 
            sinarray[i] = 2.0;
         PISLICE = 2 * PI / SLICE;	/* calculate this just once */
      }

      /* get scale factors */
      sclx =   XSCALE/100.0;
      scly =   YSCALE/100.0;
      sclz = - ROUGH/100.0;

      if(SPHERE==FALSE)  /* skip this slow stuff in sphere case */
      {
         /* start with identity */
         identity (m);

         /* translate so origin is in center of box */
         trans ( (double)xdots/(-2.0),(double)ydots/(-2.0),
                 (double)colors/(-2.0),m);
     
         /* apply scale factors */
         scale(sclx,scly,sclz,m);  

         /* rotation values - converting from degrees to radians */
         xval = XROT / 57.29577;
         yval = YROT / 57.29577;  
         zval = ZROT / 57.29577;
         xrot (xval,m);
         yrot (yval,m);
         zrot (zval,m);
         

         /* Find values of translation that make all x,y,z negative */
         /* m current matrix */
         /* 0 means don't show box */
         /* returns minimum and maximum values of x,y,z in fractal */
         corners(m,0,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
      }   

      /* perspective 3D vector - iview[2] == 0 means no perspective */

      /* set perspective flag */
      persp = 0;
      if (ZVIEWER != 0) persp = 1;
 
      /* set of view vector */
      iview[0] = xdots >> 1;  
      iview[1] = ydots >> 1;
      
      /* z value of user's eye - should be more negative than extreme 
         negative part of image */
      if(SPHERE) /* sphere case */
         iview[2] = -(long)((double)ydots*(double)ZVIEWER/100.0);
      else          /* non-sphere case */
         iview[2] = (long)((zmin-zmax)*(double)ZVIEWER/100.0);

      view[0] = iview[0];  view[1] = iview[1]; view[2] = iview[2];
      iview[0] = iview[0] << 16;
      iview[1] = iview[1] << 16;
      iview[2] = iview[2] << 16; 

      if(SPHERE==FALSE) /* sphere skips this */
      {
         /* translate back exactly amount so maximum values are non-positive */
         trans(((double)xdots-xmax-xmin)/2,((double)ydots-ymax-ymin)/2,-zmax,m);
         trans((double)XSHIFT,(double)(-YSHIFT),0.0,m);
         /* matrix m now contains ALL those transforms composed together !!!! */
         /* convert m to long integers shifted 16 bits */
         for (i = 0; i < 4; i++)
         for (j = 0; j < 4; j++)
		    im[i][j] = m[i][j] * 65536.0;
         /* this shows box before plotting fractal - very handy - should let
         user do this */
         showbox = 0;
         if(debugflag==20)
            showbox=1;   
         if(showbox)
         {
            /* 1 means show box - xmin etc. do nothing here */
            corners(m,1,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
         }
      }
      else /* sphere stuff goes here */
      {
         
         /* Sphere is on side - north pole on right. Top is -90 degrees
            latitude; bottom 90 degrees */
      
         /* units tenths of a degree */
         
         /* Map X to this LATITUDE range */
         theta1 = THETA1*(1.0*SLICE/360);
         theta2 = THETA2*(1.0*SLICE/360);

         /* Map Y to this LONGITUDE range */
         phi1   = PHI1*(1.0*SLICE/360);
         phi2   = PHI2*(1.0*SLICE/360);

         theta = (int)theta1;
         phi   = (int)phi1;
         deltatheta = (float)(theta2 - theta1)/linelen;         
         deltaphi   = (float)(phi2   - phi1  )/height;         

         xcenter = xdots/2 + XSHIFT;
         ycenter = ydots/2 - YSHIFT;

         /* affects how rough planet terrain is */
         rscale= .3*ROUGH/100.0;

         /* radius of planet */
         R = (double)(ydots)/2;
         
         /* precalculate factor */
         rXrscale = R*rscale;
         
         /* aspect ratio calculation - assume screen is 4 x 3 */
         aspect = (double)xdots*.75/(double)ydots; 
         sclx = scly = RADIUS/100.0;

         /* adjust x scale factor for aspect */
         sclx *= aspect;
         
         /* precalculation factor used in sphere calc */
         Rfactor = rscale*R/(double)colors;
         
         if(persp) /* precalculate fudge factor */
         {
            double radius;
            double zview;
            double angle;
 
            xcenter      = xcenter << 16;
            ycenter      = ycenter << 16;
 
            Rfactor *= 65536.0;
            R       *= 65536.0;
           
            /* calculate z cutoff factor */
            /* attempt to prevent out-of-view surfaces from being written */
            zview = -(long)((double)ydots*(double)ZVIEWER/100.0);
            radius = (double)(ydots)/2;
            angle = atan(-radius/(zview+radius));
            zcutoff = -radius - sin(angle)*radius;
            zcutoff *= 1.1; /* for safety */
            zcutoff *= 65536;
         }
      }
      /* Both Sphere and Normal 3D */
      light_direction[0] = XLIGHT;
      light_direction[1] = YLIGHT;
      light_direction[2] = ZLIGHT;
      
      if(FILLTYPE==6 && !SPHERE) /* transform light direction */
      {
         v[0] = 0.0;
         v[1] = 0.0;
         v[2] = 0.0;
         vmult(v,m,v);
         vmult(light_direction,m,light_direction);
         light_direction[0] -= v[0]; 
         light_direction[1] -= v[1]; 
         light_direction[2] -= v[2]; 
      }
      normalize_vector(light_direction);
      
      /* bad has values caught by clipping */
      f_bad.x     = bad.x     = xdots;
      f_bad.y     = bad.y     = ydots;
      f_bad.color = bad.color = colors;
      for(i=0;i<linelen;i++)
      {
         lastrow[i]   = bad;
         f_lastrow[i] = f_bad;
      }
      if(fp_pot)
         fclose(fp_pot);
      fp_pot=fopen("tmppot","rb");
   }
   
   col = 0;

   /* make sure these pixel coordinates are out of range */
   old   = bad;
   f_old = f_bad;

   set_pixel_buff(pixels,fpixels,linelen);
   
   /* PROCESS ROW LOOP BEGINS HERE */
   while(col < linelen)
   {
      /* printf("row %d col %d\r",rowcount,col); */
      cur.color   = pixels[col];
      f_cur.color = fpixels[col];
      if (cur.color > 0 && cur.color < WATERLINE) 
          f_cur.color = cur.color = WATERLINE;	/* "lake" */

      if(SPHERE) /* sphere case */
      {
         register i, j;
         /* avoid side effects by using temporary variables */
         /* should be able to get rid of these */
         tmp_phi   = phi;
         tmp_theta = theta;

         /* get angles in range */
         if (tmp_phi >= 0)
            tmp_phi = tmp_phi & (SLICE-1);
         else
            tmp_phi = SLICE - ((-tmp_phi) & (SLICE-1));
         if (tmp_theta >= 0)
            tmp_theta = tmp_theta & (SLICE-1);
         else
            tmp_theta = SLICE - ((-tmp_theta) & (SLICE-1));

         /* lookup sines and cosines */ /* Hoo, Boy, is THIS ugly!! - Bert */
         i = tmp_phi & (SLICE4-1);
         j = SLICE4 - i;
         if (sinarray[i] > 1.5) 
         {
            sinarray[i] = sin(PISLICE*i);
            sinarray[j] = sin(PISLICE*j);
         }
         if (tmp_phi < SLICE4) 
         {
            sinphi   =  sinarray[i];
            cosphi   =  sinarray[j];
         } 
         else 
         {
            if (tmp_phi < (SLICE4+SLICE4)) 
            {
               sinphi   =  sinarray[j];
               cosphi   = -sinarray[i];
            } 
            else 
            {
               if (tmp_phi < (SLICE-SLICE4)) 
               {
                  sinphi   = -sinarray[i];
                  cosphi   = -sinarray[j];
               } 
               else 
               {
                  sinphi   = -sinarray[j];
                  cosphi   =  sinarray[i];
               }
            }
         }

         i = tmp_theta & (SLICE4-1);
         j = SLICE4 - i;
         if (sinarray[i] > 1.5) 
         {
            sinarray[i] = sin(PISLICE*i);
            sinarray[j] = sin(PISLICE*j);
         }
         if (tmp_theta < SLICE4) 
         {
            sintheta   =  sinarray[i];
            costheta   =  sinarray[j];
         } 
         else 
         {
            if (tmp_theta < (SLICE4+SLICE4)) 
            {
               sintheta   =  sinarray[j];
               costheta   = -sinarray[i];
            } 
            else 
            {
               if (tmp_theta < (SLICE-SLICE4)) 
               {
                  sintheta   = -sinarray[i];
                  costheta   = -sinarray[j];
               } 
               else 
               {
                  sintheta   = -sinarray[j];
                  costheta   =  sinarray[i];
               }
            }
         }

         /* if naked use of goto's offend you don't read next two lines! */
         if(sinphi < 0) 
         {
            cur   = bad;
            f_cur = f_bad;
            goto loopbottom;   /* another goto ! */
         }
         if(debugflag = 26) /* do math lib calc instead */
         {
            double dtheta,dphi;
            dtheta = theta*PISLICE;
            dphi   = phi*PISLICE;

            sintheta = sin(dtheta);
            costheta = cos(dtheta);
            sinphi   = sin(dphi  );
            cosphi   = cos(dphi  );
         }
         /* KEEP THIS FOR DOCS - original formula --
         if(rscale < 0.0)
            r = 1.0+((double)cur.color/(double)colors)*rscale;
         else
            r = 1.0-rscale+((double)cur.color/(double)colors)*rscale;
         R = (double)ydots/2;
         r = r*R;
         cur.x = xdots/2 + sclx*r*sintheta*aspect + xup ;
         cur.y = ydots/2 + scly*r*costheta*cosphi - yup ;
         */

         if(rscale < 0.0)
            r = R + Rfactor*(double)f_cur.color;
         else if(rscale > 0.0)
            r = R -rXrscale + Rfactor*(double)f_cur.color;
         else
            r = R;
         if(persp)
         {
            /* NOTE: fudge was pre-calculated above in r and R */
            iv[2] = -R - r*costheta*sinphi; /* (almost) guarantee negative */
            if(iv[2] > zcutoff)
            {
               cur.x = xdots;
               cur.y = ydots;
               goto loopbottom;   /* another goto ! */
            }
            iv[0] = xcenter + sintheta*sclx*r;          /* x */
            iv[1] = ycenter + costheta*cosphi*scly*r;   /* y */
            
            if(FILLTYPE >= 5) /* calculate illumination normal before persp */
            {
               double r0;
               int xcenter0,ycenter0;
            
               r0       = r/65536;
               xcenter0 =  xcenter >> 16;
               ycenter0 =  xcenter >> 16;
                
               f_cur.x     = xcenter0 + sintheta*sclx*r0;
               f_cur.y     = ycenter0 + costheta*cosphi*scly*r0;
               f_cur.color = -r0*costheta*sinphi;
            }
             
            ipersp(iv,iview);
            cur.x = (iv[0]+32768L) >> 16;
            cur.y = (iv[1]+32768L) >> 16;
         }
         else
         {
            cur.x = f_cur.x = xcenter + sintheta*sclx*r;
            cur.y = f_cur.y = ycenter + costheta*cosphi*scly*r;
            if(FILLTYPE >= 5)
                f_cur.color = -r*costheta*sinphi;
         }
      }
      else /* non-sphere 3D */
      {
         if(debugflag == 22)
         {
            /* slow float version for comparison */
            v[0] = col;
            v[1] = rowcount;
            v[2] = f_cur.color;
            vmult(v,m,v);
            f_cur.x   = v[0];
            f_cur.y   = v[1];
            f_cur.color = v[2];
            if(persp)
               perspective(v);
            cur.x = v[0];
            cur.y = v[1];
         }
         else
         {
             if(FILLTYPE >= 5) /* flag to save vector before perspective */
                iv0[0] = 1;    /* in ivmult calculation */
             else
                iv0[0] = 0;
             
             /* use 32-bit multiply math to snap this out */
             iv[0] = col;  iv[1] = rowcount;  iv[2] = f_cur.color*65536.0;
             iv[0] = iv[0] << 16;  iv[1] = iv[1] << 16; /*iv[2] = iv[2] << 16;*/
             ivmult(iv,im,iv0,iv,iview);
             cur.x = (iv[0]+32768L) >> 16;
             cur.y = (iv[1]+32768L) >> 16;
    
             if(FILLTYPE >= 5)
             {
                f_cur.x      = iv0[0];
                f_cur.x     /= 65536.0;
                f_cur.y      = iv0[1];
                f_cur.y     /= 65536.0;
                f_cur.color  = iv0[2];
                f_cur.color /= 65536.0;
             }
         }
      }
      
      switch(FILLTYPE)
      {
      case 0:   
         (*plot)(cur.x,cur.y,cur.color);
         break;
      case 1:             /* connect-a-dot */
         if (old.x < xdots) 
            draw_line(old.x,old.y,cur.x,cur.y,cur.color); 
         old.x = cur.x;
         old.y = cur.y;
         break;
      case 2: /* with interpolation */
      case 3: /* no interpolation */
         /* 
            "triangle fill" - consider four points: current point,
            previous point same row, point opposite current point in
            previous row, point after current point in previous row. 
            The object is to fill all points inside the two triangles.
              
                           lastrow[col].x/y___ lastrow[col+1]
                           /   1              /
                         /     1            /
                       /       1          /
            oldrow/col _____ trow/col   /
         */   
         if(rowcount && col)    /* skip first row and first column */
         {  
            if(col < xdots-1)
               putatriangle(lastrow[col+1],lastrow[col],cur,cur.color);
            putatriangle(old,           lastrow[col],cur,cur.color);

            /* make sure corner colors not overwritten */
            (*plot)(cur.x,cur.y,cur.color);
            (*plot)(old.x,old.y,old.color);
            (*plot)(lastrow[col  ].x,lastrow[col  ].y,lastrow[col  ].color);
            if(col < xdots-1)
               (*plot)(lastrow[col+1].x,lastrow[col+1].y,lastrow[col+1].color);

         }
         break;
      case 4: /* "solid fill" */ 
         if (SPHERE) 
         {
            if (persp) 
            {
                  old.x = xcenter>>16;
                  old.y = ycenter>>16;
            } else 
            {
                  old.x = xcenter;
                  old.y = ycenter;
            }
         } else 
         {
            iv[0] = col;  iv[1] = rowcount;  iv[2] = 0;
            iv[0] = iv[0] << 16;  iv[1] = iv[1] << 16; iv[2] = iv[2] << 16;
            ivmult(iv,im,iv0,iv,iview);
            old.x = (iv[0]+32768L) >> 16;
            old.y = (iv[1]+32768L) >> 16;
         }
         if (old.x < 0) 
            old.x = 0;
         if (old.x >= xdots) 
            old.x = xdots-1;
         if (old.y < 0) 
            old.y = 0;
         if (old.y >= ydots) 
            old.y = ydots-1;
         draw_line(old.x,old.y,cur.x,cur.y,cur.color);
         break;
      case 5:
      case 6: 
         /* light-source modulated fill */
         if(rowcount && col)    /* skip first row and first column */
         {  
            if(FILLTYPE==5)
            {
               v1[0] = 1;
               v1[1] = 0;
               v1[2] = f_cur.color - f_old.color;
    
               v2[0] = 0;
               v2[1] = 1;
               v2[2] = f_lastrow[col].color - f_cur.color;
            }
            else if(FILLTYPE==6)
            {
               v1[0] = f_cur.x     - f_lastrow[col-1].x;    
               v1[1] = f_cur.y     - f_lastrow[col-1].y;    
               v1[2] = f_cur.color - f_lastrow[col-1].color;
    
               v2[0] = f_lastrow[col].x     - f_cur.x;
               v2[1] = f_lastrow[col].y     - f_cur.y;
               v2[2] = f_lastrow[col].color - f_cur.color;
            } 
            
            cross_product (v1, v2, cross);
      
            /* normalize cross - and check if non-zero */
            if(normalize_vector(cross)) 
            {
               /* this shouldn't happen */
               /*
               printf("normal vector error\r");
               showfpoint(f_cur);
               showfpoint(f_lastrow[col]);
               showfpoint(f_lastrow[col-1]);
               getch();
               */
               cur.color = f_cur.color = colors;
            }   
            else
            {
               /* line-wise averaging scheme */
               if(LIGHTAVG>0)
               {
                  if(col==1)
                  {
                     /* initialize array of old normal vectors */
                     crossavg[0] = cross[0];
                     crossavg[1] = cross[1];
                     crossavg[2] = cross[2];
                  }
                  crossavg[0] = (crossavg[0]*LIGHTAVG+cross[0])/(LIGHTAVG+1);
                  crossavg[1] = (crossavg[1]*LIGHTAVG+cross[1])/(LIGHTAVG+1);
                  crossavg[2] = (crossavg[2]*LIGHTAVG+cross[2])/(LIGHTAVG+1);
                  cross[0] = crossavg[0];
                  cross[1] = crossavg[1];
                  cross[2] = crossavg[2];
                  normalize_vector(cross);
               } 
               /* dot product of unit vectors is cos of angle between */
               /* we will use this value to shade surface */
               cur.color = 1+(colors-2)*(1.0-dot_product(cross,light_direction));
            }
            if(cur.color < 0) 
               cur.color = colors-1;
            if(col < 2 || rowcount < 2) /* don't have valid colors yet */
               break;
            if(col < xdots-1)
               putatriangle(lastrow[col+1],lastrow[col],cur,cur.color);
            putatriangle(old,lastrow[col],cur,cur.color);
            
            /* make sure corner colors not overwritten */
            (*plot)(cur.x,cur.y,cur.color);
            (*plot)(old.x,old.y,old.color);
            (*plot)(lastrow[col  ].x,lastrow[col  ].y,lastrow[col  ].color);
            if(col < xdots-1)
               (*plot)(lastrow[col+1].x,lastrow[col+1].y,lastrow[col+1].color);
         }
	     break;
      }
      loopbottom:
      if (FILLTYPE >= 2 && FILLTYPE != 4)
      {
        /* for triangle fill purposes */
        old = lastrow[col] = cur;
        
        if(FILLTYPE >= 5)
        {	    
           /* for illumination model purposes */
           f_old = f_lastrow[col] = f_cur;
	 }
      }
      col++;
      if(SPHERE) /* incremental latitude calculation */ 
         theta += deltatheta;
   }
   if(++rowcount >= ydots)
   {
      /* done - make sure pot file is closed */
      if(fp_pot)
      {
         fclose(fp_pot);
         fp_pot = NULL;
      }
   }

   if(SPHERE) /* incremental longitude calculation */
   {
      phi += deltaphi;
      theta = theta1;
   }
   return(0); /* decoder needs to know all is well !!! */
}

/* Bresenham's algorithm for drawing line */
void draw_line (int X1, int Y1, int X2, int Y2, int color)
{                                 /* uses Bresenham algorithm to draw a line */
  int dX, dY;                                           /* vector components */
  int row, col,
      final,                                   /* final row or column number */
      G,                               /* used to test for new row or column */
      inc1,                 /* G increment when row or column doesn't change */
      inc2;                        /* G increment when row or column changes */
  char pos_slope;
  extern int xdots,ydots;
  extern int (*plot)();    

  dX = X2 - X1;                                    /* find vector components */
  dY = Y2 - Y1;
  pos_slope = (dX > 0);                                /* is slope positive? */
  if (dY < 0)
    pos_slope = !pos_slope;
  if (abs (dX) > abs (dY))                              /* shallow line case */
  {
    if (dX > 0)                     /* determine start point and last column */
    {
      col = X1;
      row = Y1;
      final = X2;
    }
    else
    {
      col = X2;
      row = Y2;
      final = X1;
    }
    inc1 = 2 * abs (dY);               /* determine increments and initial G */
    G = inc1 - abs (dX);
    inc2 = 2 * (abs (dY) - abs (dX));
    if (pos_slope)
      while (col <= final)      /* step through columns checking for new row */
      {
        (*plot) (col, row, color);
        col++;
        if (G >= 0)                              /* it's time to change rows */
        {
          row++;             /* positive slope so increment through the rows */
          G += inc2;
        }
        else                                         /* stay at the same row */
          G += inc1;
      }
    else
      while (col <= final)      /* step through columns checking for new row */
      {
        (*plot) (col, row, color);
        col++;
        if (G > 0)                               /* it's time to change rows */
        {
          row--;             /* negative slope so decrement through the rows */
          G += inc2;
        }
        else                                         /* stay at the same row */
          G += inc1;
      }
  }  /* if |dX| > |dY| */
  else                                                    /* steep line case */
  {
    if (dY > 0)                        /* determine start point and last row */
    {
      col = X1;
      row = Y1;
      final = Y2;
    }
    else
    {
      col = X2;
      row = Y2;
      final = Y1;
    }
    inc1 = 2 * abs (dX);               /* determine increments and initial G */
    G = inc1 - abs (dY);
    inc2 = 2 * (abs (dX) - abs (dY));
    if (pos_slope)
      while (row <= final)      /* step through rows checking for new column */
      {
        (*plot) (col, row, color);
        row++;
        if (G >= 0)                           /* it's time to change columns */
        {
          col++;          /* positive slope so increment through the columns */
          G += inc2;
        }
        else                                      /* stay at the same column */
          G += inc1;
      }
    else
      while (row <= final)      /* step through rows checking for new column */
      {
        (*plot) (col, row, color);
        row++;
        if (G > 0)                            /* it's time to change columns */
        {
          col--;          /* negative slope so decrement through the columns */
          G += inc2;
        }
        else                                      /* stay at the same column */
          G += inc1;
      }
  }
}  /* draw_line */


/* vector version of line draw */
void vdraw_line(v1,v2,color)
double *v1,*v2;
int color;
{
   int x1,y1,x2,y2;
   x1 = (int)v1[0];
   y1 = (int)v1[1];
   x2 = (int)v2[0];
   y2 = (int)v2[1];
   draw_line(x1,y1,x2,y2,color);
}

/* 
   This bizarre function has two purposes. The main purpose is to determine
   the extreme values of a transformed box containing a fractal for
   centering purposes. As a side effect it can also plot the transformed
   box as an aid in setting reasonable rotation parameters 
*/
corners(m,show,pxmin,pymin,pzmin,pxmax,pymax,pzmax)
MATRIX m;
int show; /* turns on box-showing feature */
double *pxmin,*pymin,*pzmin,*pxmax,*pymax,*pzmax;
{
   extern int xdots,ydots,colors;
   VECTOR b1,b2,b3,b4,t1,t2,t3,t4;
   
   *pxmin = *pymin = *pzmin = INT_MAX;
   *pxmax = *pymax = *pzmax = INT_MIN;

   /* define corners of box fractal is in in x,y,z plane */
   /* "b" stands for "bottom" - these points are the corners of the screen
       in the x-y plane. The "t"'s stand for Top - they are the top of
       the cube where 255 color points hit. */   
   b1[0] = 0;
   b1[1] = 0;
   b1[2] = 0;

   b2[0] = xdots-1;
   b2[1] = 0;
   b2[2] = 0;

   b3[0] = xdots-1;
   b3[1] = ydots-1;
   b3[2] = 0;

   b4[0] = 0;
   b4[1] = ydots-1;
   b4[2] = 0;

   t1[0] = 0;
   t1[1] = 0;
   t1[2] = colors-1;

   t2[0] = xdots-1;
   t2[1] = 0;
   t2[2] = colors-1;

   t3[0] = xdots-1;
   t3[1] = ydots-1;
   t3[2] = colors-1;

   t4[0] = 0;
   t4[1] = ydots-1;
   t4[2] = colors-1;

   /* transform points */
   vmult(b1,m,b1);   
   vmult(b2,m,b2);   
   vmult(b3,m,b3);   
   vmult(b4,m,b4);   
                      
   vmult(t1,m,t1);   
   vmult(t2,m,t2);   
   vmult(t3,m,t3);   
   vmult(t4,m,t4);
   
   /* find minimum x */
   if(b1[0] <= *pxmin) *pxmin = b1[0];
   if(b2[0] <= *pxmin) *pxmin = b2[0];
   if(b3[0] <= *pxmin) *pxmin = b3[0];
   if(b4[0] <= *pxmin) *pxmin = b4[0];

   if(t1[0] <= *pxmin) *pxmin = t1[0];
   if(t2[0] <= *pxmin) *pxmin = t2[0];
   if(t3[0] <= *pxmin) *pxmin = t3[0];
   if(t4[0] <= *pxmin) *pxmin = t4[0];

   /* find minimum y */
   if(b1[1] <= *pymin) *pymin = b1[1];
   if(b2[1] <= *pymin) *pymin = b2[1];
   if(b3[1] <= *pymin) *pymin = b3[1];
   if(b4[1] <= *pymin) *pymin = b4[1];

   if(t1[1] <= *pymin) *pymin = t1[1];
   if(t2[1] <= *pymin) *pymin = t2[1];
   if(t3[1] <= *pymin) *pymin = t3[1];
   if(t4[1] <= *pymin) *pymin = t4[1];

   /* find minimum z */
   if(b1[2] <= *pzmin) *pzmin = b1[2];
   if(b2[2] <= *pzmin) *pzmin = b2[2];
   if(b3[2] <= *pzmin) *pzmin = b3[2];
   if(b4[2] <= *pzmin) *pzmin = b4[2];

   if(t1[2] <= *pzmin) *pzmin = t1[2];
   if(t2[2] <= *pzmin) *pzmin = t2[2];
   if(t3[2] <= *pzmin) *pzmin = t3[2];
   if(t4[2] <= *pzmin) *pzmin = t4[2];

   /* find maximum x */
   if(b1[0] >= *pxmax) *pxmax = b1[0];
   if(b2[0] >= *pxmax) *pxmax = b2[0];
   if(b3[0] >= *pxmax) *pxmax = b3[0];
   if(b4[0] >= *pxmax) *pxmax = b4[0];

   if(t1[0] >= *pxmax) *pxmax = t1[0];
   if(t2[0] >= *pxmax) *pxmax = t2[0];
   if(t3[0] >= *pxmax) *pxmax = t3[0];
   if(t4[0] >= *pxmax) *pxmax = t4[0];

   /* find maximum y */
   if(b1[1] >= *pymax) *pymax = b1[1];
   if(b2[1] >= *pymax) *pymax = b2[1];
   if(b3[1] >= *pymax) *pymax = b3[1];
   if(b4[1] >= *pymax) *pymax = b4[1];

   if(t1[1] >= *pymax) *pymax = t1[1];
   if(t2[1] >= *pymax) *pymax = t2[1];
   if(t3[1] >= *pymax) *pymax = t3[1];
   if(t4[1] >= *pymax) *pymax = t4[1];

   /* find maximum z */
   if(b1[2] >= *pzmax) *pzmax = b1[2];
   if(b2[2] >= *pzmax) *pzmax = b2[2];
   if(b3[2] >= *pzmax) *pzmax = b3[2];
   if(b4[2] >= *pzmax) *pzmax = b4[2];

   if(t1[2] >= *pzmax) *pzmax = t1[2];
   if(t2[2] >= *pzmax) *pzmax = t2[2];
   if(t3[2] >= *pzmax) *pzmax = t3[2];
   if(t4[2] >= *pzmax) *pzmax = t4[2];

   if(show)
   {
      if(persp)
      {
         /* bottom */
         perspective(b1); 
         perspective(b2); 
         perspective(b3); 
         perspective(b4); 
                          
         perspective(t1); 
         perspective(t2); 
         perspective(t3); 
         perspective(t4);
      } 
      /* draw box connecting transformed points. NOTE COLORS */   
   
      /* bottom */
      vdraw_line (b1,b2,2);
      vdraw_line (b2,b3,2);
      vdraw_line (b3,b4,2);
      vdraw_line (b4,b1,2);
      
      /* top */
      vdraw_line (t1,t2,3);
      vdraw_line (t2,t3,3);
      vdraw_line (t3,t4,3);
      vdraw_line (t4,t1,3);
      
      /* sides */
      vdraw_line (b1,t1,4); /* these pixels written first - want in BACK */
      vdraw_line (b2,t2,5);
      vdraw_line (b3,t3,6); /* these pixels written last - want in FRONT */
      vdraw_line (b4,t4,7);
   }
}

/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
int putminmax(int x,int y,int color)
{
    if(x < minmax_x[y].minx) minmax_x[y].minx = max(0,x);
    if(x > minmax_x[y].maxx) minmax_x[y].maxx = min(xdots-1,x);
    return(0);
}

/* 
   This routine fills in a triangle. Extreme left and right values for
   each row are calculated by calling the line function for the sides.
   Then rows are filled in with horizontal lines 
*/
putatriangle(pt1,pt2,pt3,color)
struct point pt1,pt2,pt3;
int color;
{
   extern struct point p1,p2,p3;
   int miny,maxy,minx,maxx;
   int y;

   p1 = pt1;
   p2 = pt2;
   p3 = pt3;

   /* clip if out of range */
   if( p1.x < 0 || p1.x >= xdots) return(-1);
   if( p2.x < 0 || p2.x >= xdots) return(-1);
   if( p3.x < 0 || p3.x >= xdots) return(-1);
   if( p1.y < 0 || p1.y >= ydots) return(-1);
   if( p2.y < 0 || p2.y >= ydots) return(-1);
   if( p3.y < 0 || p3.y >= ydots) return(-1);
   
   /* find min max y */
   miny = ydots;
   maxy = 0;
   if(p1.y < miny) miny = p1.y;
   if(p2.y < miny) miny = p2.y;
   if(p3.y < miny) miny = p3.y;
   
   if(p1.y > maxy) maxy = p1.y;
   if(p2.y > maxy) maxy = p2.y;
   if(p3.y > maxy) maxy = p3.y;

   if(maxy - miny <= 1)
   {
      
      /* find min max x */
      minx = xdots;
      maxx = 0;
      if(p1.x < minx) minx = p1.x;
      if(p2.x < minx) minx = p2.x;
      if(p3.x < minx) minx = p3.x;
      
      if(p1.x > maxx) maxx = p1.x;
      if(p2.x > maxx) maxx = p2.x;
      if(p3.x > maxx) maxx = p3.x;
      if(maxx - minx <= 1) /* bail out if too close together */
         return(-1);
   }
   for(y=miny;y<=maxy;y++)
   {
      minmax_x[y].minx = INT_MAX;
      minmax_x[y].maxx = INT_MIN;
   }      

   /* set plot to "fake" plot function */
   plot = putminmax;

   /* build table of extreme x's of triangle */
   draw_line(p1.x,p1.y,p2.x,p2.y,0);
   draw_line(p2.x,p2.y,p3.x,p3.y,0);
   draw_line(p3.x,p3.y,p1.x,p1.y,0);

   /* fill in triangle */
   if(FILLTYPE != 3)
      plot = interpcolor;
   else
      plot = putcolor;   
   for(y=miny;y<=maxy;y++)
      draw_line(minmax_x[y].minx,y,minmax_x[y].maxx,y,color);
   plot = clipcolor;
   return(0); /* zero means ok */
}
clipcolor(x,y,color)
{
   if(0 <= x    && x < xdots   && 
      0 <= y    && y < ydots   && 
      0 < color && color < colors)
   {   
      putcolor(x,y,color);
      return(0);
   } 
   else   
      return(-1);
}

/* A substitute for plotcolor that interpolates the colors according
   to the x and y values in three points p1,p2,p3 which are static in
   this routine */
int interpcolor(x,y,color)
{
   int d1,d2,d3;


      /* this distance formula is not the usual one - put it has the virtue
         that it uses ONLY additions (almost) and it DOES go to zero as the
         points get close. */

      d1 = abs(p1.x-x)+abs(p1.y-y);
      d2 = abs(p2.x-x)+abs(p2.y-y);
      d3 = abs(p3.x-x)+abs(p3.y-y);
         
      /* calculate a weighted average of colors */   
      color = ((d2+d3)*p1.color + (d1+d3)*p2.color + (d1+d2)*p3.color)
                /(d2+d3+d1+d3+d1+d2);
   if(0 <= x    && x < xdots   && 
      0 <= y    && y < ydots   && 
      0 < color && color < colors)
   {
      putcolor(x,y,color);
      return(0);
   } 
   else   
      return(-1);
}

int set_pixel_buff(unsigned char *pixels,float far *fpixels,unsigned linelen)
{
   unsigned int *intbuf;
   int i;
   extern int filetype; /* 0=GIF (8 bit), 1=TIW (16 bit) */
   switch(filetype)
   {
   case 2:       /* TARGA type 10 (RGB 16 bit) */
   case 1:       /* TIW 16 bit */
      intbuf = (unsigned int *)pixels;
      for(i=0;i<linelen;i++)
         pixels[i] = fpixels[i] = ((float)intbuf[i])/(1<<8);
      break;
   case 0:       /* GIF 8 bit */
      for(i=0;i<linelen;i++)
         *(fpixels+i) = *(pixels+i);
      break;
   default:
      return(-1);
      break; 
   }
   return(0);
}
/*
showfpoint(struct f_point f_pt)
{
   printf("%f %f %f\n",f_pt.x,f_pt.y,f_pt.color);
}
*/
