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

The sample code below is a straightforward Mandelbrot routine.

*/

extern int colors;
testpt(initreal,initimag,parm1,parm2,maxit,inside)
double initreal,initimag,parm1,parm2;
int maxit,inside;
{
double oldreal, oldimag, newreal, newimag, magnitude;
register color;
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
return(color);
}
