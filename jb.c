#include <stdlib.h>
#include "fractint.h"
#include "mpmath.h"
#include "helpdefs.h"

extern  int fullscreen_prompt(char *hdg,int numprompts,char * far *prompts,
               struct fullscreenvalues values[],int options,int fkeymask,
               char far *extrainfo);
void stackscreen(void);
void unstackscreen(void);

extern int row, col, xdots, ydots, bitshift, fractype;
extern int ixstart, ixstop, iystart, iystop, colors, helpmode;
extern double param[], xxmin, xxmax, yymin, yymax;
extern long delx, dely, ltempsqrx, ltempsqry, far *lx0, far *ly0;
extern struct lcomplex lold, lnew, *longparm;
extern llimit2;
static int bbase;
static long xpixel, ypixel;
static long initz, djx, djy, dmx, dmy;
static long jx, jy, mx, my, xoffset, yoffset;
static long jxmin, jxmax, jymin, jymax, mxmin, mxmax, mymin, mymax;
static long x_per_inch, y_per_inch, inch_per_xdot, inch_per_ydot;
struct Perspective {
   long x, y, zx, zy;
};

struct Perspective LeftEye, RightEye, *Per;
struct lcomplex jbc;

#define NUM_VAR 17

static double fg, fg16;
static long
        zdots = 128L,
        shell = 30L,
        origin = (long)(8.0 * (1L << 16)),
   height = (long)(7.0 * (1L << 16)),
        width = (long)(10.0 * (1L << 16)),
   dist = (long)(24.0 * (1L << 16)),
   eyes = (long)(0.0 * (1L << 16)),
   depth = (long)(8.0 * (1L << 16)),
   brratio = (long)(0.0 * (1L << 16));

int JulibrotSetup(void) {
        int r;
        int oldhelpmode;
   struct fullscreenvalues d[NUM_VAR];
   static char StereoFile[] = "glasses1.map";
   static char GreyFile[] = "altern.map";
   char *mapname;
   static char *v[NUM_VAR] = {
      "Julia from x",                              /* d[0] */
      "Julia to x",                                /* d[1] */
      "Julia from y",                              /* d[2] */
      "Julia to y",                                /* d[3] */
      "Mandelbrot from x",                         /* d[4] */
      "Mandelbrot to x",                           /* d[5] */
      "Mandelbrot from y",                         /* d[6] */
      "Mandelbrot to y",                           /* d[7] */
      "Number of z pixels",                        /* d[8] */
      "Penetration level",                         /* d[9] */
      "Location of z origin",                      /* d[10] */
      "Depth of z",                                /* d[11] */
      "Screen height",                             /* d[12] */
      "Screen width",                              /* d[13] */
      "Distance to Screen",                        /* d[14] */
      "Distance between eyes (0 for Greyscale)",   /* d[15] */
      "Blue:Red Ratio (0 for Greyscale)",          /* d[16] */
   };

   if(colors < 255) {
      static char far msg[]=
          {"Sorry, but Julibrots require a 256-color video mode"};
      stopmsg(0,msg);
      return(0);
   }

   fg = (double)(1L << bitshift);
   fg16 = (double)(1L << 16);

   for (r = 0; r < NUM_VAR; ++r)
      d[r].type = 'f';

        jxmax = (long)((d[0].uval.dval = xxmax) * fg);
        jxmin = (long)((d[1].uval.dval = xxmin) * fg);
        jymax = (long)((d[2].uval.dval = yymax) * fg);
        jymin = (long)((d[3].uval.dval = yymin) * fg);
        mxmax = (long)((d[4].uval.dval = param[0]) * fg);
        mxmin = (long)((d[5].uval.dval = param[1]) * fg);
        mymax = (long)((d[6].uval.dval = param[2]) * fg);
        mymin = (long)((d[7].uval.dval = param[3]) * fg);
   d[8].uval.dval = (double)zdots;
   d[9].uval.dval = (double)shell;
   d[10].uval.dval = (double)origin / fg16;
   d[11].uval.dval = (double)depth / fg16;
   d[12].uval.dval = (double)height / fg16;
   d[13].uval.dval = (double)width / fg16;
   d[14].uval.dval = (double)dist / fg16;
   d[15].uval.dval = (double)eyes / fg16;
   d[16].uval.dval = (double)brratio / fg16;

   stackscreen();
   oldhelpmode = helpmode;
   helpmode = HT_JULIBROT;
   if((r = fullscreen_prompt("Julibrot Parameters",
                              NUM_VAR, v, d, 0, 0, 0)) >= 0) {
      jxmin = (long)((xxmax = d[0].uval.dval) * fg);
      jxmax = (long)((xxmin = d[1].uval.dval) * fg);
      xoffset = (jxmax + jxmin) / 2;       /* Calculate average */
      jymin = (long)((yymax = d[2].uval.dval) * fg);
      jymax = (long)((yymin = d[3].uval.dval) * fg);
      yoffset = (jymax + jymin) / 2;       /* Calculate average */
      mxmin = (long)((param[0] = d[4].uval.dval) * fg);
      mxmax = (long)((param[1] = d[5].uval.dval) * fg);
      mymin = (long)((param[2] = d[6].uval.dval) * fg);
      mymax = (long)((param[3] = d[7].uval.dval) * fg);
      zdots = (long)(d[8].uval.dval);
      shell = (long)(d[9].uval.dval);
      origin = (long)(d[10].uval.dval * fg16);
      depth = (long)(d[11].uval.dval * fg16);
      height = (long)(d[12].uval.dval * fg16);
      width = (long)(d[13].uval.dval * fg16);
      dist = (long)(d[14].uval.dval * fg16);
      eyes = (long)(d[15].uval.dval * fg16);
      brratio = (long)(d[16].uval.dval * fg16);
      dmx = (mxmax - mxmin) / zdots;
      dmy = (mymax - mymin) / zdots;
      longparm = &jbc;

      x_per_inch = (long)((d[1].uval.dval - d[0].uval.dval) / d[13].uval.dval * fg);
      y_per_inch = (long)((d[3].uval.dval - d[2].uval.dval) / d[12].uval.dval * fg);
                inch_per_xdot = (long)(d[13].uval.dval / xdots * fg16);
                inch_per_ydot = (long)(d[12].uval.dval / ydots * fg16);
      initz = origin - (depth / 2);
      LeftEye.x = -(RightEye.x = eyes / 2);
      LeftEye.y = RightEye.y = 0l;
      LeftEye.zx = RightEye.zx = dist;
      LeftEye.zy = RightEye.zy = dist;
      bbase = (int)(128.0 * d[16].uval.dval);
   }

   helpmode = oldhelpmode;
   unstackscreen();

   if(d[16].uval.dval == 0.0)
      mapname = GreyFile;
   else
      mapname = StereoFile;
   if (ValidateLuts(mapname) != 0)
      return(0);
   spindac(0,1);                 /* load it, but don't spin */

   return(r >= 0);
}

int jb_per_pixel(void) {
        jx = multiply(Per->x - xpixel, initz, 16);
   jx = divide(jx, dist, 16) - xpixel;
   jx = multiply(jx << (bitshift - 16), x_per_inch, bitshift);
   jx += xoffset;
        djx = divide(depth, dist, 16);
        djx = multiply(djx, Per->x - xpixel, 16) << (bitshift - 16);
        djx = multiply(djx, x_per_inch, bitshift) / zdots;

        jy = multiply(Per->y - ypixel, initz, 16);
        jy = divide(jy, dist, 16) - ypixel;
        jy = multiply(jy << (bitshift - 16), y_per_inch, bitshift);
   jy += yoffset;
        djy = divide(depth, dist, 16);
        djy = multiply(djy, Per->y - ypixel, 16) << (bitshift - 16);
        djy = multiply(djy, y_per_inch, bitshift) / zdots;

   return(1);
}

static int n, zpixel, plotted, color;

int zline(long x, long y) {
   xpixel = x;
   ypixel = y;
        mx = mxmin;
        my = mymin;
   if((row + col) & 1)
      Per = &LeftEye;
   else
      Per = &RightEye;
   curfractalspecific->per_pixel();
   for(zpixel = 0; zpixel < zdots; zpixel++) {
      lold.x = jx;
      lold.y = jy;
      jbc.x = mx;
      jbc.y = my;
      if(check_key())
         return(-1);
      ltempsqrx = multiply(lold.x, lold.x, bitshift);
      ltempsqry = multiply(lold.y, lold.y, bitshift);
      for(n = 0; n < shell; n++) {
         if(curfractalspecific->orbitcalc())
            break;
      }
      if(n == shell) {
         if(brratio) {
            color = (int)(128l * zpixel / zdots);
            if((row + col) & 1)
               (*plot)(col, row, 127 - color);
            else {
                                color = (int)(multiply((long)color << 16, brratio, 16) >> 16);
               (*plot)(col, row, 127 + bbase - color);
            }
         }
         else {
            color = (int)(254l * zpixel / zdots);
            (*plot)(col, row, color + 1);
         }
         plotted = 1;
         break;
      }
      mx += dmx;
      my += dmy;
      jx += djx;
      jy += djy;
   }
   return(0);
}

int Std4dFractal(void) {
   long x, y;
   int xdot, ydot;

   for(y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydot) {
                plotted = 0;
                x = -(width >> 1);
                for(xdot = 0; xdot < xdots; xdot++, x += inch_per_xdot) {
         col = xdot;
         row = ydot;
         if(zline(x, y) < 0)
            return(-1);
         col = xdots - col - 1;
         row = ydots - row - 1;
         if(zline(-x, -y) < 0)
            return(-1);
      }
      if(!plotted) break;
   }
   return(0);
}
