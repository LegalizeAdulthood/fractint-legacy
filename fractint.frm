 comment = {
 FRACTINT.DOC has instructions for adding new formulas to this file.
 Note that there are several hard-coded restrictions in the formula
 interpreter:

 1) The fractal name through the open curly bracket must be on a single line.
 2) There is a current hard-coded limit of 30 formulas per formula file, only
    because of restrictions in the prompting routines.
 3) Formulas must currently be less than 200 characters long.
 3) Comments, like this one, are set up using dummy formulas with no
    formula name or the special name "comment".  There can be as many
    of these "comment" fractals as desired, they can be interspersed with
    the real formulas, and they have no length restriction.

 The formulas in the beginning of this file are from Mark Peterson, who
 built this fractal interpreter feature.  The rest are grouped by contributor.
 (Scott Taylor sent many but they are no longer here - they've been
 incorporated as hard-coded types.  Lee Skinner also sent many which have
 now been hard-coded.)
 }

   Mandelbrot(XAXIS) = {
      z = Pixel:  z = sqr(z) + pixel, |z| <= 4
   }

   Dragon (ORIGIN) {
      z = Pixel:
	 z = sqr(z) + (-0.74543, 0.2),
      |z| <= 4
   }

   Daisy (ORIGIN) = { z = pixel:  z = z*z + (0.11031, -0.67037), |z| <= 4 }

   InvMandel {
      c = z = 1 / pixel:
	 z = sqr(z) + c;
      |z| <= 4
   }

   MarksMandelPwr {
      z = pixel, c = z ^ (z - 1):
	 z = c * sqr(z) + pixel,
      |z| <= 4
   }

   DeltaLog(XAXIS) {
      z = pixel, c = log(pixel):
	 z = sqr(z) + c,
      |z| <= 4
   }

   Newton4(XYAXIS) {
      z = pixel; Root = 1:
	 z3 = z*z*z;
	 z4 = z3 * z;
	 z = (3 * z4 + Root) / (4 * z3),
		.004 <= |z4 - Root|
   }


 comment = {
	The following are from Chris Green:
	These fractals all use Newton's or Halley's formula for approximation
	of a function.	In all of these fractals, p1 is the "relaxation
	coefficient". A value of 1 gives the conventional newton or halley
	iteration. Values <1 will generally produce less chaos than values >1.
	1-1.5 is probably a good range to try.	P2 is the imaginary component
	of the relaxation coefficient, and should be zero but maybe a small
	non-zero value will produce something interesting. Who knows?

	These MUST be run with floating point in order to produce the correct
	pictures.  I Haven't explored these sets very much because I lack a
	floating point coprocessor.

	For more information on Halley maps, see "Computers, Pattern, Chaos,
	and Beauty" by Pickover.

	Halley :  Halley's formula applied to x^7-x=0. Setting P1 to 1 will
		create the picture on page 277 of Pickover's book.

	HalleySin : Halley's formula applied to sin(x)=0. Setting P1 to 0.1
		will create the picture from page 281 of Pickover.

	NewtonSinExp : Newton's formula applied to sin(x)+exp(x)-1=0.

	CGNewton3 : A different variation on newton iteration. The initial
		guess is fixed at (1,1), but the equation solved is different
		at each pixel ( x^3-pixel=0 is solved). Try P1=1.8.

	CGNewtonSinExp : Newton's formula applied to sin(x)+exp(x)-pixel=0.

	HyperMandel :  A four dimensional version of the mandelbrot set. Use P1
		to select which two-dimensional plane of the four dimensional
		set you wish to examine.

 }

  halley (XYAXIS) {
	z=pixel:
	z5=z*z*z*z*z;
	z6=z*z5;
	z7=z*z6;
	z=z-p1*((z7-z)/ ((7.0*z6-1)-(42.0*z5)*(z7-z)/(14.0*z6-2))),
	0.0001 <= |z7-z|
	}

  CGhalley (XYAXIS) {
	z=(1,1):
	z5=z*z*z*z*z;
	z6=z*z5;
	z7=z*z6;
	z=z-p1*((z7-z-pixel)/ ((7.0*z6-1)-(42.0*z5)*(z7-z-pixel)/(14.0*z6-2))),
	0.0001 <= |z7-z-pixel|
	}

   halleySin (XYAXIS) {
	z=pixel:
	s=sin(z);
	c=cos(z);
	z=z-p1*(s/(c-(s*s)/(c+c))),
	0.0001 <= |s|
   }

   NewtonSinExp (XAXIS) {
	z=pixel:
	z1=exp(z);
	z2=sin(z)+z1-1;
	z=z-p1*z2/(cos(z)+z1), .0001 < |z2|
   }

   CGNewton3 {
		z=(1,1):
		z2=z*z;
		z3=z*z2;
		z=z-p1*(z3-pixel)/(3.0*z2),
		0.0001 < |z3-pixel|
	}

    HyperMandel {
	a=(0,0);
	b=(0,0):
	z=z+1;
	anew=sqr(a)-sqr(b)+pixel;
	b=2.0*a*b+p1;
	a=anew,
	|a|+|b| <= 4
    }


 comment = { The following are from Lee Skinner }

 comment = { Mandelbrot form 1 of the Tetration formula }

 MTet (XAXIS) {
   z = pixel:
       z = (pixel ^ z) + pixel,
	   |z| <= (P1 + 3)
 }

 comment = { Mandelbrot form 2 of the Tetration formula }

 AltMTet (XAXIS) {
   z = 0:
       z = (pixel ^ z) + pixel,
	   |z| <= (P1 + 3)
 }

 comment = { Julia form 1 of the Tetration formula }

 JTet (XAXIS) {
   z = pixel:
       z = (pixel ^ z) + P1,
	   |z| <= (P2 + 3)
 }

 comment = { Julia form 2 of the Tetration formula }

 AltJTet (XAXIS) {
   z = P1:
       z = (pixel ^ z) + P1,
	   |z| <= (P2 + 3)
 }

 Cubic {
	  p = pixel,
       test = p1 + 3,
	 t3 = 3*p,
	 t2 = p*p,
	  a = (t2 + 1)/t3,
	  b = 2*a*a*a + (t2 - 2)/t3,
	aa3 = a*a*3,
	  z = 0 - a :
	      z = z*z*z - aa3*z + b,  |z| < test }


 { The following resulted from a FRACTINT bug. Version 13 incorrectly
   calculated Spider (see above). We fixed the bug, and reverse-engineered
   what it was doing to Spider - so here is the old "spider" }

 Wineglass(XAXIS) {
    c = z = pixel:
    z = z * z + c, c = (1+imag(c)) * real(c) / 2 + z, |z| <= 4 }


  { The following were sent by JM Colard-Richard}

  Richard1(XYAXIS) {c = z = pixel: z=(z*z*sin(z*z)+z*z)+pixel, |z|<=50}
  Richard2 {c = z = pixel: z=1/(sin(z*z+pixel*pixel)),|z|<=50}
  Richard3 {c = z = pixel: z=(1/sinh(z)*sinh(z))+pixel,|z|<=50}
  Richard4 {c = z = pixel: z=(1/(z*z*cos(z*z)+z*z))+pixel,|z|<=50}
  Richard5 {c = z = pixel: z=sin(z*sinh(z))+pixel,|z|<=50}
  Richard6 {c = z = pixel: z=sin(sinh(z))+pixel,|z|<=50}
  Richard7 {c = z = pixel: z=log(z)*pixel,|z|<=50}
  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}


