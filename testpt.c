/*

Write your fractal program here. initreal and initimag are the values in
the complex plane; parm1, and parm2 are paramaters to be entered with the
"params=" option (if needed). The function should return the color associated
with initreal and initimag.  FRACTINT will repeatedly call your function with
the values of initreal and initimag ranging over the rectangle defined by the
"corners=" option. Assuming your formula is iterative, "maxit" is the maximum
iteration. If "maxit" is hit, color "inside" should be returned.

Note that this routine could be sped up using external variables/arrays
rather than the current parameter-passing scheme.  The goal, however was
to make it as easy as possible to add fractal types, and this looked like
the easiest way.

This module is part of an overlay, with calcfrac.c.  The routines in it
must not be called by any part of Fractint other than calcfrac.

The sample code below is a straightforward Mandelbrot routine.

*/

extern void gettruecolor(int, int, int*, int*, int*);
extern void puttruecolor(int, int, int, int, int);
extern int  getakey(void);

int teststart()     /* this routine is called just before the fractal starts */
{
   extern int debugflag;
   extern int xdots, ydots, colors;

   /*
        true-color demo: if debugging flag is 500 and in 256-color mode
        write out a couple of truecolor patterns, read them in and
        write them out again in a different location, then wait for
        a keystroke and clear the screen before generating the
        actual fractal
   */
   if ((debugflag == 500) && (xdots >= 640) && (ydots >= 480) && (colors == 256)) {
      int xdot, ydot, red, green, blue;
      for (xdot = 0; xdot <= 255; xdot++) {
         for (ydot = 0; ydot < 384; ydot++) {
            red = xdot;
            if (ydot < 128) {
               green = 2 * ydot;
               blue  = 0;
            }
            if (ydot >= 128 && ydot < 256) {
               green = 0;
               blue  = 2 * (ydot-128);
            }
            if (ydot >= 256) {
               red   = 2 * (ydot - 256);
               green = 2 * (ydot - 256);
               blue  = 2 * (ydot - 256);
            }
            puttruecolor(xdot, ydot, red, green, blue); 
         }
      }
      for (xdot = 0; xdot <= 255; xdot++) {
         for (ydot = 0; ydot < 384; ydot++) {
            gettruecolor(xdot, ydot, &red, &green, &blue);
            puttruecolor(xdot+256, ydot, red, green, blue); 
         }
      }
      getakey();
      for (xdot = 0; xdot <= 255; xdot++) {
         for (ydot = 0; ydot <= 384; ydot++) {
            puttruecolor(xdot, ydot, 0, 0, 0); 
            puttruecolor(xdot+256, ydot, 0, 0, 0); 
         }
      }
   }

   return( 0 );
}

void testend()       /* this routine is called just after the fractal ends */
{
}

/* this routine is called once for every pixel */
/* (note: possibly using the dual-pass / solif-guessing options */

int testpt(double initreal,double initimag,double parm1,double parm2,
long maxit,int inside)
{
   double oldreal, oldimag, newreal, newimag, magnitude;
   long color;
   oldreal=parm1;
   oldimag=parm2;
   magnitude = 0.0;
   color = 0;
   while ((magnitude < 4.0) && (color < maxit)) {
      newreal = oldreal * oldreal - oldimag * oldimag + initreal;
      newimag = 2 * oldreal * oldimag + initimag;
      color++;
      oldreal = newreal;
      oldimag = newimag;
      magnitude = newreal * newreal + newimag * newimag;
   }
   if (color >= maxit) color = inside;
   return((int)color);
}
