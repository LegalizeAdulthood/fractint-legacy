/* This file contains a 3D replacement for the out_line function called
   by the decoder. The purpose is to apply various 3D transformations before 
   displaying points. Called once per line of the original GIF file.
   
   Original Author Tim Wegner, with extensive help from Marc Reinig.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <dos.h>
#include "fractint.h"

int clipcolor(int,int,int);   
int clipcolor2(int,int,int);   
int interpcolor(int,int,int);
int T_clipcolor(int,int,int);   
int T_interpcolor(int,int,int);
int (*fillplot)();   
int (*normalplot)();   
int (*standardplot)();

char preview = 0;
char showbox = 0;
static int localpreviewfactor;
int previewfactor = 20;
int xadjust = 0;
int yadjust = 0;
int xxadjust;
int yyadjust;
int xshift;
int yshift; 
extern int overflow;
extern int filetype;
extern char floatflag;
extern int (*plot)();   
extern int xdots, ydots, colors;
extern int debugflag;
extern void draw_line (int X1, int Y1, int X2, int Y2, int color);
extern int extraseg;
extern unsigned height;
extern int rowcount;            /* in general.asm */
extern int init3d[];			/* 3D arguments (FRACTINT.C) */
extern FILE *fp_pot;			/* potential file pointer */
extern long far *lx0;
extern int transparent[2];		/* transparency min/max */

FILE    *dacfile;
char    MAP_name[140];
extern          findpath();
extern          ValidateLuts();  /* read the palette file */
extern  int     mapset; /* Flag indicating a new MAP was selected */

static int zcoord = 256;
static double aspect;              /* aspect ratio */

static float far *sinthetaarray;	/* all sine   thetas go here  */
static float far *costhetaarray;	/* all cosine thetas go here */

static double rXrscale;            /* precalculation factor */

static int persp;       /* flag for indicating perspective transformations */
int bad_value = -10000; /* set bad values to this */
int bad_check = -1000;  /* check values against this to determine if good */

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
VECTOR cross;
VECTOR tmpcross;
line3d(unsigned char pixels[], unsigned linelen)
{

   /* these values come from FRACTINT.C */

   /* These variables must preserve their values across calls */
   static float   deltaphi;  /* increment of latitude, longitude */
   static float phi;
   static double rscale;              /* surface roughness factor */
   static long xcenter,ycenter;       /* circle center */
   static double sclx,scly,sclz;      /* scale factors */
   static double R;                   /* radius values */
   static double Rfactor;             /* for intermediate calculation */
   static MATRIX m;                   /* transformation matrix */
   static MATRIX lightm;              /* m w/no trans, keeps obj. on screen */


   static LMATRIX lm;                 /* "" */
   static LVECTOR lview;		      /* for perspective views */
   static double zcutoff;             /* perspective backside cutoff value */  
   static float twocosdeltaphi;
   static float cosphi,sinphi;              /* precalculated sin/cos of longitude */
   static float oldcosphi1,oldsinphi1;
   static float oldcosphi2,oldsinphi2;
   double r;                          /* sphere radius */
   double xval,yval,zval;             /* rotation values */
   float costheta,sintheta;          /* precalculated sin/cos of latitude */
   float twocosdeltatheta;
   
   int col;                           /* current column (original GIF) */ 
   struct point cur;                  /* current pixels */
   struct point old;                  /* old pixels */
   struct point oldlast;              /* old pixels */

   static struct point bad;           /* out of range value */

   struct f_point f_cur;
   struct f_point f_old;
   static struct f_point f_bad;       /* out of range value */

   static float far *fpixels;         /* float persion of pixels array */
    
   VECTOR v;                          /* double vector */
   VECTOR v1,v2;
   VECTOR crossavg;
   char crossnotinit;                 /* flag for crossavg init indication */
   
   static VECTOR light_direction;
   double v_length;
   VECTOR origin, direct;
   LVECTOR lv;			              /* long equivalent of v */
   LVECTOR lv0;			              /* long equivalent of v */
   int color;                         /* current decoded color */
   /* corners of transformed xdotx by ydots x colors box */ 
   double xmin, ymin, zmin, xmax, ymax, zmax; 
   int i,j;
   int lastdot;
   
   if(transparent[0] || transparent[1])
      plot = normalplot = T_clipcolor;  /*  Use the transparent plot function */
   else if(colors >= 16)
      plot = normalplot = clipcolor;    /* the usual FRACTINT plot function with clipping */
   else
      plot = normalplot = clipcolor2;
   
     
   /* 
      This IF clause is executed ONCE per image. All precalculations are 
      done here, with out any special concern about speed. DANGER - 
      communication with the rest of the program is generally via static
      or global variables.
   */   
   if(rowcount == 0)
   {
      long check_extra;       
      float theta,theta1,theta2;  	/* current,start,stop latitude */
      float phi1,phi2;        		/* current start,stop longitude */
      float   deltatheta;  		/* increment of latitude */
      /* plot_setup();*/
      /* lastrow stores the previous row of the original GIF image for
         the purpose of filling in gaps with triangle procedure */

      if(colors<16)
         zcoord = 256;
      else
         zcoord = colors;   
      crossavg[0] = 0;
	  crossavg[1] = 0;
      crossavg[2] = 0;
      minmax_x = (struct minmax far *)lx0;

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
      check_extra = sizeof(*lastrow)*(MAXPIXELS);

      /* now stuff the sine and cosine arrays still farther out */
      sinthetaarray = (float far *)(lastrow+MAXPIXELS);
      check_extra = check_extra + sizeof(float)*MAXPIXELS;

      costhetaarray = (float far *)(sinthetaarray+MAXPIXELS);
      check_extra = check_extra + sizeof(float)*MAXPIXELS;

      f_lastrow = (struct f_point far *)(costhetaarray+MAXPIXELS);
      check_extra = check_extra + sizeof(*f_lastrow)*(MAXPIXELS);

      fpixels = (float far *)(f_lastrow+MAXPIXELS);
      check_extra = check_extra + sizeof(float)*MAXPIXELS;


      /* get scale factors */
      sclx =   XSCALE/100.0;
      scly =   YSCALE/100.0;
      sclz = - ROUGH/100.0;

      /* aspect ratio calculation - assume screen is 4 x 3 */
      aspect = (double)xdots*.75/(double)ydots; 

      if(SPHERE==FALSE)  /* skip this slow stuff in sphere case */
      {
         /* 
            What is done here is to create a single matrix, m, which has 
            scale, rotation, and shift all combined. This allows us to use 
            a single matrix to transform any point. Additionally, we create 
            two perspective vectors.
            
            Start with a unit matrix. Add scale and rotation. Then calculate 
            the perspective vectors. Finally add enough translation to center 
            the final image plus whatever shift the user has set.
         */
         
         /* start with identity */
         identity (m);
         identity (lightm);

         /* translate so origin is in center of box, so that when we rotate 
            it, we do so through the center */
         trans ( (double)xdots/(-2.0),(double)ydots/(-2.0),
                 (double)zcoord/(-2.0),m);
         trans ( (double)xdots/(-2.0),(double)ydots/(-2.0),
                 (double)zcoord/(-2.0),lightm);

         /* apply scale factors */
         scale(sclx,scly,sclz,m);  
         scale(sclx,scly,sclz,lightm);

         /* rotation values - converting from degrees to radians */
         xval = XROT / 57.29577;
         yval = YROT / 57.29577;  
         zval = ZROT / 57.29577;

         xrot (xval,m);         xrot (xval,lightm);
         yrot (yval,m);         yrot (yval,lightm);
         zrot (zval,m);         zrot (zval,lightm);
         

         /* Find values of translation that make all x,y,z negative */
         /* m current matrix */
         /* 0 means don't show box */
         /* returns minimum and maximum values of x,y,z in fractal */
         corners(m,0,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
      }   

      /* perspective 3D vector - lview[2] == 0 means no perspective */

      /* set perspective flag */
      persp = 0;
      if (ZVIEWER != 0)
      {
         persp = 1;
         if(ZVIEWER < 80) /* force float */
             floatflag |= 2; /* turn on second bit */
      }   
 
      /* set up view vector, and put viewer in center of screen */
      lview[0] = xdots >> 1;  
      lview[1] = ydots >> 1;
      
      /* z value of user's eye - should be more negative than extreme 
         negative part of image */
      if(SPHERE) /* sphere case */
         lview[2] = -(long)((double)ydots*(double)ZVIEWER/100.0);
      else          /* non-sphere case */
         lview[2] = (long)((zmin-zmax)*(double)ZVIEWER/100.0);

      view[0] = lview[0];  view[1] = lview[1]; view[2] = lview[2];
      lview[0] = lview[0] << 16;
      lview[1] = lview[1] << 16;
      lview[2] = lview[2] << 16; 

      if(SPHERE==FALSE) /* sphere skips this */
      {
         /* translate back exactly amount we translated earlier plus enough 
            to center image so maximum values are non-positive */
         trans(((double)xdots-xmax-xmin)/2,((double)ydots-ymax-ymin)/2,-zmax,m);

/* M */ /* Keep the box centered and on screen regardless of shifts */
         trans(((double)xdots-xmax-xmin)/2,((double)ydots-ymax-ymin)/2,
            -zmax,lightm);

         trans((double)(xshift),(double)(-yshift),0.0,m);

         /* matrix m now contains ALL those transforms composed together !!!! */
         /* convert m to long integers shifted 16 bits */
         for (i = 0; i < 4; i++)
         for (j = 0; j < 4; j++)
		    lm[i][j] = m[i][j] * 65536.0;

      }
      else /* sphere stuff goes here */
      {
         float z;
         /* Sphere is on side - north pole on right. Top is -90 degrees
            latitude; bottom 90 degrees */
      
         /* Map X to this LATITUDE range */
         theta1 = THETA1*PI/180.0;
         theta2 = THETA2*PI/180.0;

         /* Map Y to this LONGITUDE range */
         phi1   = PHI1*PI/180.0;
         phi2   = PHI2*PI/180.0;

         theta = theta1;
         phi   = phi1;
         
         /*
            Thanks to Hugh Bray for the following idea: when calculating
            a table of evenly spaced sines or cosines, only a few initial
            values need be calculated, and the remaining values can be 
            gotten from a derivative of the sine/cosine angle sum formula
            at the cost of one multiplication and one addition per value!

            This idea is applied once here to get a complete table for 
            latitude, and near the bottom of this routine to incrementally
            calculate longitude.
            
            Precalculate 2*cos(deltaangle), sin(start) and sin(start+delta).
            Then apply recursively:
              sin(angle+2*delta) = sin(angle+delta) * 2cosdelta - sin(angle)

            Similarly for cosine. Neat!
          */

         deltatheta = (float)(theta2 - theta1)/(float)linelen;         

         /* initial sin,cos theta */
         sinthetaarray[0] = sin((double)theta);
         costhetaarray[0] = cos((double)theta);
         sinthetaarray[1] = sin((double)(theta + deltatheta));
         costhetaarray[1] = cos((double)(theta + deltatheta));
      
         /* sin,cos delta theta */
         twocosdeltatheta = 2.0*cos((double)deltatheta);

         /* build table of other sin,cos with trig identity */
         for(i=2;i<linelen;i++)
         {
            sinthetaarray[i] = sinthetaarray[i-1]*twocosdeltatheta-
                               sinthetaarray[i-2];
            costhetaarray[i] = costhetaarray[i-1]*twocosdeltatheta-
                               costhetaarray[i-2];
         }         

         /* now phi - these calculated as we go - get started here */
         deltaphi   = (float)(phi2   - phi1  )/(float)height;

         /* initial sin,cos phi */

         sinphi = oldsinphi1 = sin((double)phi1);
         cosphi = oldcosphi1 = cos((double)phi1);
         oldsinphi2 = sin((double)(phi1+deltaphi));
         oldcosphi2 = cos((double)(phi1+deltaphi));
      
         /* sin,cos delta phi */
         twocosdeltaphi = 2*cos((double)deltaphi);

         xcenter = xdots/2 + xshift;
         ycenter = ydots/2 - yshift;

         /* affects how rough planet terrain is */
         rscale= .3*ROUGH/100.0;

         /* radius of planet */
         R = (double)(ydots)/2;

         /* precalculate factor */
         rXrscale = R*rscale;
         
         sclx = scly = RADIUS/100.0;

         /* adjust x scale factor for aspect */
         sclx *= aspect;
         
         /* precalculation factor used in sphere calc */
         Rfactor = rscale*R/(double)zcoord;
         
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
      /* set fill plot function */
      if(FILLTYPE != 3)
      {
         fillplot = interpcolor;
         if(transparent[0] || transparent[1]) /*  If transparent colors are set */
            fillplot = T_interpcolor;  /*  Use the transparent plot function  */
      }
      else
      {
         fillplot = clipcolor;

         if(transparent[0] || transparent[1]) /*  If transparent colors are set */
            fillplot = T_clipcolor;  /*  Use the transparent plot function  */
      }

        
      /* Both Sphere and Normal 3D */
      direct[0] = light_direction[0] = XLIGHT;
      direct[1] = light_direction[1] = -YLIGHT;
      direct[2] = light_direction[2] = ZLIGHT;

    /* Needed because sclz = -ROUGH/100 and light_direction is transformed
       in FILLTYPE 6 but not in 5.
    */
      if (FILLTYPE == 5 && !SPHERE)
         direct[2] = light_direction[2] = -ZLIGHT;
      else {

/* MRR Needed to keep light_source in SPHERE consistent with planar
       but don't know exactly why this is needed, but it is. I hate that!
*/
       if (SPHERE)
       {
            light_direction[0] = -XLIGHT;
            direct[2] = -ZLIGHT;
        }
    }

      if(FILLTYPE==6) /* transform light direction */
      {
         /* Think of light direction  as a vector with tail at (0,0,0) and
            head at (light_direction). We apply the transformation to
            BOTH head and tail and take the difference */

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

        if(preview && FILLTYPE >= 5 && showbox)
        {
            normalize_vector(direct);

            /* Set origin[] to be the center of the untransformed xy plane at z=0 */
            /* and torigin[] to be the same */
            origin[0] = (double)(xdots)/2.0;
            origin[1] = (double)(ydots)/2.0;
            origin[2] = 0.0;

            /*  Set length of light direction vector to be convienient for viewing */
            v_length = min ((xdots/2.0), min ((ydots/2.0), (float)(zcoord)));

            /* Set direct[] to point from origin[] in direction of untransformed
            light_direction (direct[]). */

            direct[0] = origin[0] + direct[0] * v_length;
            direct[1] = origin[1] + direct[1] * v_length;
            direct[2] = origin[2] + direct[2] * v_length;

            /* Draw light source vector and box containing it, draw_light_box
            will transform them if necessary. */
            draw_light_box (origin,direct,lightm);
            /* draw box around original field of view to help visualize effect of rotations */
            /* 1 means show box - xmin etc. do nothing here */
            corners(m,1,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
        }

      /* bad has values caught by clipping */
      f_bad.x     = bad.x     = bad_value;
      f_bad.y     = bad.y     = bad_value;
      f_bad.color = bad.color = bad_value;
      for(i=0;i<linelen;i++)
      {
         lastrow[i]   = bad;
         f_lastrow[i] = f_bad;
      }
      if(fp_pot)
         fclose(fp_pot);
      fp_pot=fopen("tmppot","rb");
   } /* end of once-per-image intializations */ 
   crossnotinit = 1;
   col = 0;

   /* make sure these pixel coordinates are out of range */
   old   = bad;
   f_old = f_bad;

   /* copies pixels buffer to float type fpixels buffer for fill purposes */
   set_pixel_buff(pixels,fpixels,linelen);

   /*
   This section of code allows the operation of a preview mode when the
   preview flag is set. Enabled, it allows the drawing of only the first 
   line of the source image, then every 10th line, until and including the 
   last line. For the undrawn lines, only necessary calculations are 
   made. As a bonus, in non-sphere mode a box is drawn to help visualize
   the effects of 3D transformations. Thanks to Marc Reinig for this idea
   and code -- BTW, Marc did NOT put the goto in, but WE did, to avoid
   copying code here, and to avoid a HUGE "if-then" construct. Besides,
   we have ALREADY sinned, so why not sin some more? */

   localpreviewfactor = ydots/previewfactor;
     
   /* Insure last line is drawn in preview and filltype -1  */
   if ((preview || FILLTYPE == -1) && (rowcount != ydots-1))
      if (rowcount % localpreviewfactor)
         if ( !(((FILLTYPE == 5) || (FILLTYPE == 6)) && (rowcount == 1)))
            goto reallythebottom; /* skip over most of the line3d calcs */
   lastdot = min(xdots-1, linelen-1);
   
   /* PROCESS ROW LOOP BEGINS HERE */
   while(col < linelen)
   {
      if (FILLTYPE == -1)
      {
         if (col != lastdot) /* if this is not the last col */
            if (col % localpreviewfactor) /* if not the 1st or mod factor col*/
               goto loopbottom;
      }

      cur.color   = pixels[col];
      f_cur.color = fpixels[col];
      if (cur.color > 0 && cur.color < WATERLINE) 
          f_cur.color = cur.color = WATERLINE;	/* "lake" */

      if(SPHERE) /* sphere case */
      {
         sintheta = sinthetaarray[col];
         costheta = costhetaarray[col];
         
         /* if naked use of goto's offend you don't read next two lines! */
         if(sinphi < 0) 
         {
            cur   = bad;
            f_cur = f_bad;
            goto loopbottom;   /* another goto ! */
         }
         
         /* KEEP THIS FOR DOCS - original formula --
         if(rscale < 0.0)
            r = 1.0+((double)cur.color/(double)zcoord)*rscale;
         else
            r = 1.0-rscale+((double)cur.color/(double)zcoord)*rscale;
         R = (double)ydots/2;
         r = r*R;
         cur.x = xdots/2 + sclx*r*sintheta*aspect + xup ;
         cur.y = ydots/2 + scly*r*costheta*cosphi - yup ;
         */

         if(rscale < 0.0)
            r = R + Rfactor*(double)f_cur.color*costheta;
         else if(rscale > 0.0)
            r = R -rXrscale + Rfactor*(double)f_cur.color*costheta;
         else
            r = R;
         if(persp)
         {
            /* NOTE: fudge was pre-calculated above in r and R */
            lv[2] = -R - r*costheta*sinphi; /* (almost) guarantee negative */
            if(lv[2] > zcutoff)
            {
               cur = bad;
               f_cur = f_bad;
               goto loopbottom;   /* another goto ! */
            }
            lv[0] = xcenter + sintheta*sclx*r;          /* x */
            lv[1] = ycenter + costheta*cosphi*scly*r;   /* y */
            
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
            if(!floatflag)
            {               
               if(longpersp(lv,lview,16) == -1)
               {
                  cur = bad;
                  f_cur = f_bad;
                  goto loopbottom;   /* another goto ! */
               }
               cur.x = ((lv[0]+32768L) >> 16) + xxadjust;
               cur.y = ((lv[1]+32768L) >> 16) + yyadjust;
            }
            if(floatflag||overflow)
            {
               VECTOR v;
               long fudge;
               fudge = 1L<<16;
               v[0] = lv[0];
               v[1] = lv[1];
               v[2] = lv[2];
               v[0] /= fudge;
               v[1] /= fudge;
               v[2] /= fudge;
               perspective(v);
               cur.x = v[0]+.5 + xxadjust;
               cur.y = v[1]+.5 + yyadjust;
            }
         }
         else
         {
            cur.x = f_cur.x = xcenter + sintheta*sclx*r + xxadjust;
            cur.y = f_cur.y = ycenter + costheta*cosphi*scly*r + yyadjust;
            if(FILLTYPE >= 5)
                f_cur.color = -r*costheta*sinphi;
         }
      }
      else /* non-sphere 3D */
      {
         if(!floatflag)
         {
             if(FILLTYPE >= 5) /* flag to save vector before perspective */
                lv0[0] = 1;    /* in longvmultpersp calculation */
             else
                lv0[0] = 0;
             
             /* use 32-bit multiply math to snap this out */
             lv[0] = col;      lv[0] = lv[0] << 16; 
             lv[1] = rowcount; lv[1] = lv[1] << 16; 
             if(filetype) /* don't truncate fractional part */
                 lv[2] = f_cur.color*65536.0;
             else         /* there IS no fractaional part here! */
             {
                lv[2] = f_cur.color;
                lv[2] = lv[2] << 16;  
             }
             
             if(longvmultpersp(lv,lm,lv0,lv,lview,16) == -1)
             {
                cur   = bad;
                f_cur = f_bad;
                goto loopbottom;
             }

             cur.x = ((lv[0]+32768L) >> 16) + xxadjust;
             cur.y = ((lv[1]+32768L) >> 16) + yyadjust;
             if(FILLTYPE >= 5)
             {
                f_cur.x      = lv0[0];
                f_cur.x     /= 65536.0;
                f_cur.y      = lv0[1];
                f_cur.y     /= 65536.0;
                f_cur.color  = lv0[2];
                f_cur.color /= 65536.0;
             }
         }
         if(floatflag||overflow) /* do in float if integer math overflowed */
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
            cur.x = v[0] + xxadjust + .5;
            cur.y = v[1] + yyadjust + .5;
         }
      }
      
      switch(FILLTYPE)
      {
      case -1:
        if (col &&
            old.x > bad_check &&
            old.x < (xdots - bad_check))
            draw_line (old.x, old.y, cur.x, cur.y, cur.color);
        if (rowcount &&
            lastrow[col].x > bad_check &&
            lastrow[col].y > bad_check &&
            lastrow[col].x < (xdots - bad_check) &&
            lastrow[col].y < (ydots - bad_check))
            draw_line (lastrow[col].x,lastrow[col].y,cur.x, cur.y,cur.color);
        break;
       case 0:   
         (*plot)(cur.x,cur.y,cur.color);
         break;
       case 1:             /* connect-a-dot */
         if ((old.x < xdots) && (col) &&
                old.x > bad_check &&
                old.y > bad_check) /* Don't draw from old to cur on col 0 */
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
            if(col == 1)
               putatriangle(lastrow[col],oldlast,old,old.color);
              
            if(col < lastdot)
               putatriangle(lastrow[col+1],lastrow[col],cur,cur.color);
            putatriangle(old,           lastrow[col],cur,cur.color);

            /* make sure corner colors not overwritten */
            (*plot)(cur.x,cur.y,cur.color);
            (*plot)(old.x,old.y,old.color);
            (*plot)(lastrow[col  ].x,lastrow[col  ].y,lastrow[col  ].color);
            if(col < lastdot)
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
         } 
         else 
         {
            lv[0] = col;  lv[1] = rowcount;  lv[2] = 0;

            /* apply fudge bit shift for integer math */             
            lv[0] = lv[0] << 16;  lv[1] = lv[1] << 16;
            /* Since 0, unnecessary lv[2] = lv[2] << 16;*/

            if(longvmultpersp(lv,lm,lv0,lv,lview,16))
            {
               cur   = bad;
               f_cur = f_bad;
               goto loopbottom;   /* another goto ! */
            }

            /*  Round and fudge back to original  */
            old.x = (lv[0]+32768L) >> 16;
            old.y = (lv[1]+32768L) >> 16;
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
            if(f_cur.color < bad_check || f_old.color < bad_check ||
            f_lastrow[col].color < bad_check)
               break;
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
               if(f_lastrow[col].color < bad_check)
                  break;
               v1[0] = f_cur.x     - f_old.x;    
               v1[1] = f_cur.y     - f_old.y;    
               v1[2] = f_cur.color - f_old.color;
    
               v2[0] = f_lastrow[col].x     - f_cur.x;
               v2[1] = f_lastrow[col].y     - f_cur.y;
               v2[2] = f_lastrow[col].color - f_cur.color;
            } 
            
            cross_product (v1, v2, cross);
                
            /* normalize cross - and check if non-zero */
            if(normalize_vector(cross)) 
               cur.color = f_cur.color = bad.color;
            else
            {
               /* line-wise averaging scheme */
               if(LIGHTAVG>0)
               {
                  if(crossnotinit)
                  {
                     /* initialize array of old normal vectors */
                     crossavg[0] = cross[0];
                     crossavg[1] = cross[1];
                     crossavg[2] = cross[2];
                     crossnotinit = 0;
                  }
                  tmpcross[0] = (crossavg[0]*LIGHTAVG+cross[0])/(LIGHTAVG+1);
                  tmpcross[1] = (crossavg[1]*LIGHTAVG+cross[1])/(LIGHTAVG+1);
                  tmpcross[2] = (crossavg[2]*LIGHTAVG+cross[2])/(LIGHTAVG+1);
                     
                  cross[0] = tmpcross[0];
                  cross[1] = tmpcross[1];
                  cross[2] = tmpcross[2];
                  if(normalize_vector(cross)) 
                  {
                     /* this shouldn't happen */
                     if(debugflag > 0)
                     {
                     printf("normal vector error #2\r");
                     showfpoint(f_cur);
                     showfpoint(f_lastrow[col]);
                     showfpoint(f_lastrow[col-1]);
                     getch();
                     }
                     cur.color = f_cur.color = colors;
                  }
                  crossavg[0] = tmpcross[0];
                  crossavg[1] = tmpcross[1];
                  crossavg[2] = tmpcross[2];
               } 
               /* dot product of unit vectors is cos of angle between */
               /* we will use this value to shade surface */
               cur.color = 1+(colors-2)*(1.0-dot_product(cross,light_direction));
            }
            /* if colors out of range, set them to min or max color index
               but avoid background index. This makes colors "opaque" so
               SOMETHING plots */             
            if(cur.color < 0)                  /* prevent transparent colors */
               cur.color = 1;                  /* avoid background */
            if(cur.color >= colors)
               cur.color = colors-1;

            /* why "col < 2"? So we have sufficient geometry for the fill
               algorithm, which needs previous point in same row to have
               already been calculated (variable old) */   
            if(col < 2 || rowcount < 2) /* don't have valid colors yet */
               break;
            if(col < lastdot)
               putatriangle(lastrow[col+1],lastrow[col],cur,cur.color);
            putatriangle(old,lastrow[col],cur,cur.color);

            /* make sure corner colors not overwritten */
            (*plot)(cur.x,cur.y,cur.color);
            (*plot)(old.x,old.y,old.color);
            (*plot)(lastrow[col  ].x,lastrow[col  ].y,lastrow[col  ].color);
            if(col < lastdot)
               (*plot)(lastrow[col+1].x,lastrow[col+1].y,lastrow[col+1].color);
         }
	     break;
      }  /*  End of CASE statement for fill type  */

      loopbottom:

      if (FILLTYPE == -1 || (FILLTYPE >= 2 && FILLTYPE != 4))
      {
        /* for triangle and grid fill purposes */
        oldlast = lastrow[col];
        old = lastrow[col] = cur;
        
        if(FILLTYPE >= 5)
        {	    
           /* for illumination model purposes */
           f_old = f_lastrow[col] = f_cur;
        }
      }
      col++;
   }  /*  End of while statement for plotting line  */

	/*  Done with the row (line). Now, increment rowcount and see if
            you are  done with image  */
   reallythebottom:
   if(++rowcount >= ydots)
   {
      /* end of image mopup stuff */
      /* done - make sure pot file is closed */
      if(fp_pot)
      {
         fclose(fp_pot);
         fp_pot = NULL;
      }
      if(preview && !SPHERE && showbox)
      {
         /* draw box to help visualize effect of rotations */
         /* 1 means show box - xmin etc. do nothing here */
         corners(m,1,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
      }
      
      /* reset floatflag */
      floatflag &= 1; /* strip second bit */
   }
   /* stuff that HAS to be done, even in preview mode, goes here */
   if(SPHERE)
   {
      /* incremental sin/cos phi calc */
      if(rowcount == 1)
      {
         sinphi = oldsinphi2;
         cosphi = oldcosphi2;
      }else
      {
         sinphi = twocosdeltaphi*oldsinphi2 - oldsinphi1;
         cosphi = twocosdeltaphi*oldcosphi2 - oldcosphi1;
         oldsinphi1 = oldsinphi2;
         oldsinphi2 = sinphi;
	     oldcosphi1 = oldcosphi2;
         oldcosphi2 = cosphi;
      }
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
   extern int xdots,ydots,zcoord;
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
   t1[2] = zcoord-1;

   t2[0] = xdots-1;
   t2[1] = 0;
   t2[2] = zcoord-1;

   t3[0] = xdots-1;
   t3[1] = ydots-1;
   t3[2] = zcoord-1;

   t4[0] = 0;
   t4[1] = ydots-1;
   t4[2] = zcoord-1;

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
   
/* M */ /* Keep the box surrounding the fractal */
      b1[0] = b1[0] + xxadjust;        t1[0] = t1[0] + xxadjust;
      b2[0] = b2[0] + xxadjust;        t2[0] = t2[0] + xxadjust;
      b3[0] = b3[0] + xxadjust;        t3[0] = t3[0] + xxadjust;
      b4[0] = b4[0] + xxadjust;        t4[0] = t4[0] + xxadjust;

      b1[1] = b1[1] + yyadjust;        t1[1] = t1[1] + yyadjust;
      b2[1] = b2[1] + yyadjust;        t2[1] = t2[1] + yyadjust;
      b3[1] = b3[1] + yyadjust;        t3[1] = t3[1] + yyadjust;
      b4[1] = b4[1] + yyadjust;        t4[1] = t4[1] + yyadjust;


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

/* This function draws a vector from origin[] to direct[] and a box
   around it. The vector and box are transformed or not depending on
   FILLTYPE.

   Will consolidate this and corners if the feature is used. Also, will
   be adding hidded line capability to this and/or corners.
*/

draw_light_box(origin,direct,m)
    double *origin, *direct;
    MATRIX m;

{
    VECTOR b1,b2,b3,b4,t1,t2,t3,t4;
    int i;
    double temp;
    /* Initialize the arrays */
    for (i=0;i<=2;i++)
        t1[i] = b1[i] = origin[i];

    /*   "x"                 "y"                         "z"    */
    b2[0] = b1[0];      b2[1] = direct[1];          b2[2] = b1[2];
    b3[0] = direct[0];  b3[1] = b2[1];              b3[2] = b1[2];
    b4[0] = b3[0];      b4[1] = b1[1];              b4[2] = b1[2];

                                                    t1[2] = direct[2];
    t2[0] = t1[0];      t2[1] = direct[1];          t2[2] = t1[2];
    t3[0] = direct[0];  t3[1] = t2[1];              t3[2] = t1[2];
    t4[0] = t3[0];      t4[1] = t1[1];              t4[2] = t1[2];

    /* transform the corners if necessary */
    if (FILLTYPE == 6)
    {
        /* Transform the coordinates */
         /* bottom */            /* top */
        vmult(b1,m,b1);        vmult(t1,m,t1);
        vmult(b2,m,b2);        vmult(t2,m,t2);
        vmult(b3,m,b3);        vmult(t3,m,t3);
        vmult(b4,m,b4);        vmult(t4,m,t4);
    }

    if(persp) /* Adjust for perspective if set */
    {
        perspective(b1);        perspective(t1);
        perspective(b2);        perspective(t2);
        perspective(b3);        perspective(t3);
        perspective(b4);        perspective(t4);
    }

    /* Adjust for aspect and shift back to origin (b1[]) */
    temp = b1[0] * aspect;
    temp = temp - b1[0];

    b1[0] = b1[0] * aspect - temp;  t1[0] = t1[0] * aspect - temp;
    b2[0] = b2[0] * aspect - temp;  t2[0] = t2[0] * aspect - temp;
    b3[0] = b3[0] * aspect - temp;  t3[0] = t3[0] * aspect - temp;
    b4[0] = b4[0] * aspect - temp;  t4[0] = t4[0] * aspect - temp;

    /* draw box connecting transformed points. NOTE COLORS */
    /* bottom */                /* top */
    vdraw_line (b1,b2,2);       vdraw_line (t1,t2,3);
    vdraw_line (b2,b3,2);       vdraw_line (t2,t3,3);
    vdraw_line (b3,b4,2);       vdraw_line (t3,t4,3);
    vdraw_line (b4,b1,2);       vdraw_line (t4,t1,3);
      
    /* sides */
    vdraw_line (b1,t1,4); /* these pixels written first - want in BACK */
    vdraw_line (b2,t2,5);
    vdraw_line (b3,t3,6); /* these pixels written last - want in FRONT */
    vdraw_line (b4,t4,7);

    /* draw light source vector */
    vdraw_line (b1,t3,8);
}

/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
int putminmax(int x,int y,int color)
{
    if(y < 0 || y >= ydots) return(-1); 
    if(x < minmax_x[y].minx) minmax_x[y].minx = x;
    if(x > minmax_x[y].maxx) minmax_x[y].maxx = x;
    return(0);
}

/* 
   This routine fills in a triangle. Extreme left and right values for
   each row are calculated by calling the line function for the sides.
   Then rows are filled in with horizontal lines 
*/
#define MAXOFFSCREEN  2 /* allow two of three points to be off screen */
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

   /* are these all good points? */
   if(-abs(p1.x) <= bad_check) return(-1);   
   if(-abs(p1.y) <= bad_check) return(-1);   
   if(-abs(p2.x) <= bad_check) return(-1);   
   if(-abs(p2.y) <= bad_check) return(-1);   
   if(-abs(p3.x) <= bad_check) return(-1);   
   if(-abs(p3.y) <= bad_check) return(-1);   

   /* Too many points off the screen? */
   if(offscreen(&p1) + offscreen(&p2) + offscreen(&p3) > MAXOFFSCREEN)
      return(-1);
      
   /* bail out if points are equal */
   if (pt1.x == pt2.x)
      if (pt2.x == pt3.x)
         if (pt1.y == pt2.y)
            if (pt2.y == pt3.y)
                return(-1);


    
   /* find min max y */
   miny = INT_MAX;
   maxy = INT_MIN;
   if(p1.y < miny) miny = p1.y;
   if(p2.y < miny) miny = p2.y;
   if(p3.y < miny) miny = p3.y;
   
   if(p1.y > maxy) maxy = p1.y;
   if(p2.y > maxy) maxy = p2.y;
   if(p3.y > maxy) maxy = p3.y;

   if(maxy < 0 || miny >= ydots) /* bail out if totally off screen */
      return(-1);   

   if(maxy - miny <= 1)
   {
      /* find min max x */
      minx = INT_MAX;
      maxx = INT_MIN;
      if(p1.x < minx) minx = p1.x;
      if(p2.x < minx) minx = p2.x;
      if(p3.x < minx) minx = p3.x;
      
      if(p1.x > maxx) maxx = p1.x;
      if(p2.x > maxx) maxx = p2.x;
      if(p3.x > maxx) maxx = p3.x;
      if(maxx - minx <= 1) /* bail out if too close together */
         return(-1);
      if(maxx < 0 || minx >= xdots) /* bail out if totally off screen */
         return(-1);   
   }
   
   /* only worried about values on screen */
   miny = max(0,miny);
   maxy = min(ydots-1,maxy);
   
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

   plot = fillplot;
   for(y=miny;y<=maxy;y++)
      draw_line(minmax_x[y].minx,y,minmax_x[y].maxx,y,color);
   plot = normalplot;

   return(0); /* zero means ok */
}

offscreen(struct point *pt)
{
   if(pt->x >= 0)
      if(pt->x < xdots)
        if(pt->y >= 0)
          if(pt->y < ydots)
             return(0); /* point is ok */
   return(1); /* point is off the screen */          
}

clipcolor(int x,int y,int color)
{
   if(0 <= x    && x < xdots   && 
      0 <= y    && y < ydots   && 
      0 <= color && color < colors)
   {   
      standardplot(x,y,color);
      return(0);
   } 
   else   
      return(-1);
}

clipcolor2(int x,int y,int color)
{
   if(0 <= x    && x < xdots   && 
      0 <= y    && y < ydots   && 
      0 <= color)
   {   
      standardplot(x,y,color&1);
      return(0);
   } 
   else   
      return(-1);
}

T_clipcolor(int x,int y,int color)
/*	This function is the same as clipcolor but checks for color being
	in transparent range. Intended to be called only if transparency
    has been enabled.
*/


{
   if(0 <= x    && x < xdots         && /*  is the point on screen?  */ 
      0 <= y    && y < ydots         && /*  Yes?  */
      0 <= color && color < colors   && /*  Colors in valid range?  */
      /*  Lets make sure its not a transparent color  */
      (transparent[0] > color || color > transparent[1]))
   {   
      standardplot(x,y,color); /* I guess we can plot then  */
      return(0);  /*  Done  */
   } 
   else   
      return(-1);  /*  Sorry, can't plot that  */
}

/* A substitute for plotcolor that interpolates the colors according
   to the x and y values of three points (p1,p2,p3) which are static in
   this routine */

int interpcolor(int x,int y,int color)
{
   int d1,d2,d3;

   /* this distance formula is not the usual one - put it has the virtue
      that it uses ONLY additions (almost) and it DOES go to zero as the
      points get close. */

   d1 = abs(p1.x-x)+abs(p1.y-y);
   d2 = abs(p2.x-x)+abs(p2.y-y);
   d3 = abs(p3.x-x)+abs(p3.y-y);
      
   /* calculate a weighted average of colors - 
      long casts prevent integer overflow */
   color = ((long)(d2+d3)*(long)p1.color + 
            (long)(d1+d3)*(long)p2.color + 
            (long)(d1+d2)*(long)p3.color) /(d2+d3+d1+d3+d1+d2);

   if(0 <= x    && x < xdots   && 
      0 <= y    && y < ydots   && 
      0 <= color && color < colors)
   {
      standardplot(x,y,color);
      return(0);
   } 
   else   
      return(-1);
}

int T_interpcolor(int x,int y,int color)

/* A substitute for interpcolor that interpolates the colors according
   to the x and y values of three points (p1,p2,p3) which are static in
   this routine AND additionally checks for transparent colors  */

{
   int d1,d2,d3;

   /* this distance formula is not the usual one - put it has the virtue
      that it uses ONLY additions (almost) and it DOES go to zero as the
      points get close. */

   d1 = abs(p1.x-x)+abs(p1.y-y);
   d2 = abs(p2.x-x)+abs(p2.y-y);
   d3 = abs(p3.x-x)+abs(p3.y-y);
      
   /* calculate a weighted average of colors - 
      long casts prevent integer overflow */
   color = ((long)(d2+d3)*(long)p1.color + 
            (long)(d1+d3)*(long)p2.color + 
            (long)(d1+d2)*(long)p3.color) /(d2+d3+d1+d3+d1+d2);

/*  Checking for on-screen x,y might be moved to top of routine if
    it would speed things significantly for values of perspective <100  */

   if(0 <= x    && x < xdots         && /*  is the point on screen?  */ 
      0 <= y    && y < ydots         && /*  Yes?  */
      0 <= color && color < colors   && /*  Colors in valid range?  */
      /*  Lets make sure its not a transparent color  */
      (transparent[0] > color || color > transparent[1]))
   {   
      standardplot(x,y,color); /* I guess we can plot then  */
      return(0);  /*  Done  */
   } 
   else   
      return(-1);  /*  Sorry, can't plot that  */
}

int set_pixel_buff(unsigned char *pixels,float far *fpixels,unsigned linelen)
{
   unsigned int *intbuf;
   int i;
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


showfpoint(struct f_point pt)
{
   printf("%f %f %f\n",pt.x,pt.y,pt.color);
}

/* cross product  - useful because cross is perpendicular to v and w */
int chk_cross_product (int col, int row,VECTOR v, VECTOR w, VECTOR cross, VECTOR crossavg)
{
   static start = 1;
   static FILE *fp;

   if(start)
   {
      fp = fopen("blob","w");
      start = 0;
   }

   fprintf(fp,"row %+4d col %+4d v1 %+8.3e %+8.3e %+8.3e v2 %+8.3e %+8.3e %+8.3e cross %+8.3e %+8.3e %+8.3e crossavg %+8.3e %+8.3e %+8.3e \n",
        row,col,v[0],v[1],v[2],w[0],w[1],w[2],cross[0],cross[1],cross[2],crossavg[0],crossavg[1],crossavg[2]);
   return(0);
}
slowout_line(char buffer[], int linelen)
{
   int i;
   for(i=0;i<linelen;i++)
      putcolor(i,rowcount,buffer[i]);
   rowcount++;
   return(0);   
}

