#include "fractint.h"

extern void (*plot)(int, int, int);
int fullscreen_prompt(int startrow, int numprompts, char *prompts[],
   double values[]);
void setfortext(void);
void setforgraphics(void);

extern int row, col, xdots, ydots, bitshift, fractype;
extern int ixstart, ixstop, iystart, iystop;
extern double param[], xxmin, xxmax, yymin, yymax;
extern long delx, dely, ltempsqrx, ltempsqry, far *lx0, far *ly0;
extern struct lcomplex lold, lnew, *longparm;
extern llimit2;
static int bbase;
static long xpixel, ypixel;
static long initjx, initjy, initmx, initmy, initz, djx, djy, dmx, dmy;
static long jx, jy, mx, my, dx, dy, dz, initzx, initzy, xoffset, yoffset;
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
   eyes = (long)(2.5 * (1L << 16)), 
   depth = (long)(8.0 * (1L << 16)), 
   brratio = (long)(0.8 * (1L << 16));

int JulibrotSetup(void) {
	int r;
   double d[NUM_VAR];
   static char *v[NUM_VAR] = {
      "Julia from x",            /* d[0] */
      "Julia to x",              /* d[1] */
      "Julia from y",            /* d[2] */
      "Julia to y",              /* d[3] */
      "Mandelbrot from x",       /* d[4] */
      "Mandelbrot to x",         /* d[5] */
      "Mandelbrot from y",       /* d[6] */
      "Mandelbrot to y",         /* d[7] */
      "Number of z pixels",      /* d[8] */
      "Penetration level",       /* d[9] */
      "Location of z origin",    /* d[10] */
      "Depth of z",              /* d[11] */
      "Screen height",           /* d[12] */
      "Screen width",            /* d[13] */
      "Distance to Screen",      /* d[14] */
      "Distance between eyes",   /* d[15] */
      "Blue:Red ratio",          /* d[16] */
   };

   fg = (double)(1L << bitshift);
   fg16 = (double)(1L << 16);

	jxmax = (long)((d[0] = xxmax) * fg);
	jxmin = (long)((d[1] = xxmin) * fg);
	jymax = (long)((d[2] = yymax) * fg);
	jymin = (long)((d[3] = yymin) * fg);
	mxmax = (long)((d[4] = param[0]) * fg);
	mxmin = (long)((d[5] = param[1]) * fg);
	mymax = (long)((d[6] = param[2]) * fg);
	mymin = (long)((d[7] = param[3]) * fg);
   d[8] = (double)zdots;
   d[9] = (double)shell;
   d[10] = (double)origin / fg16;
   d[11] = (double)depth / fg16;
   d[12] = (double)height / fg16;
   d[13] = (double)width / fg16;
   d[14] = (double)dist / fg16;
   d[15] = (double)eyes / fg16;
   d[16] = (double)brratio / fg16;

   setfortext();
   if((r = fullscreen_prompt(4, NUM_VAR, v, d)) >= 0) {
      jxmin = (long)((xxmax = d[0]) * fg);
      jxmax = (long)((xxmin = d[1]) * fg);
      xoffset = (jxmax + jxmin) / 2;       /* Calculate average */
      jymin = (long)((yymax = d[2]) * fg);
      jymax = (long)((yymin = d[3]) * fg);
      yoffset = (jymax + jymin) / 2;       /* Calculate average */
      mxmin = (long)((param[0] = d[4]) * fg);
      mxmax = (long)((param[1] = d[5]) * fg);
      mymin = (long)((param[2] = d[6]) * fg);
      mymax = (long)((param[3] = d[7]) * fg);
      zdots = (long)(d[8]);
      shell = (long)(d[9]);
      origin = (long)(d[10] * fg16);
      depth = (long)(d[11] * fg16);
      height = (long)(d[12] * fg16);
      width = (long)(d[13] * fg16);
      dist = (long)(d[14] * fg16);
      eyes = (long)(d[15] * fg16);
      brratio = (long)(d[16] * fg16);
      dmx = (mxmax - mxmin) / zdots;
      dmy = (mymax - mymin) / zdots;
      longparm = &jbc;

      x_per_inch = (long)((d[1] - d[0]) / d[13] * fg);
      y_per_inch = (long)((d[3] - d[2]) / d[12] * fg);
		inch_per_xdot = (long)(d[13] / xdots * fg16);
		inch_per_ydot = (long)(d[12] / ydots * fg16);
      initz = origin - (depth / 2);
      LeftEye.x = -(RightEye.x = eyes / 2);
      LeftEye.y = RightEye.y = 0l;
      LeftEye.zx = RightEye.zx = dist;
      LeftEye.zy = RightEye.zy = dist;
      bbase = (int)(128.0 * d[16]);
   }

   setforgraphics();
   return(r == 1);
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

static int n, zpixel, blank, plotted, color;

int zline(long x, long y) {
   xpixel = x;
   ypixel = y;
	mx = mxmin;
	my = mymin;
   if((row + col) & 1)
      Per = &LeftEye;
   else
      Per = &RightEye;
   fractalspecific[fractype].per_pixel();
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
         if(fractalspecific[fractype].orbitcalc())
            break;
      }
      if(n == shell) {
         color = (int)(128l * zpixel / zdots);
         if((row + col) & 1)
            (*plot)(col, row, 127 - color);
         else {
				color = (int)(multiply((long)color << 16, brratio, 16) >> 16);
            (*plot)(col, row, 127 + bbase - color);
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
