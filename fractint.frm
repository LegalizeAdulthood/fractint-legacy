 comment = {
 FRACTINT.DOC has instructions for adding new formulas to this file.
 Note that there are several hard-coded restrictions in the formula
 interpreter:
 
 1) The fractal name through the open curly bracket must be on a single line.
 2) There is a current hard-coded limit of 20 formulas per formula file, only
    because of restrictions in the prompting routines.
 3) Formulas must currently be less than 200 characters long. 
 3) Comments, like this one, are set up using dummy formulas with no 
    formula name or the special name "comment".  There can be as many
    of these "comment" fractals as desired, they can be interspersed with
    the real formulas, and they have no length restriction.

 The formulas in the beginning of this file are from Mark Peterson, who
 built this fractal interpreter feature.  Those at the end are the
 contribution of Scott Taylor [72401,410].  Keep 'em coming, Scott!
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

   MandelSine (XYAXIS) { z = Pixel:  z = sin(z) * pixel, |z| <= 50 }
   MandelCosine(XYAXIS) { z = pixel:  z = cos(z) * pixel, |z| <= 50 }
   MandelHypSine(XYAXIS) { z = pixel:  z = sinh(z) * pixel, |z| <= 50 }
   MandelHypCosine(XYAXIS) { z = pixel: z = cosh(z) * pixel, |z| <= 50 }

   LambdaLog(XAXIS) { 
      z = pixel, c = log(pixel):  
         z = c * sqr(z) + pixel, 
      |z| <= 4 
   }

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


 { Formulas from this point on were contributed by Scott Taylor [72401,410].  }
 { Parameter 1 controls the depth - larger numbers give more detail & colors. }
 { My favorite range is between 20-30, but smaller numbers are faster.        }

   ScottSin { z = pixel: z = sin(z) + sqr(z), |z| < (p1+3) }

   ScottSinH { z = pixel: z = sinh(z) + sqr(z), |z| < (p1+3) }

   ScottCos(XYAXIS) { z = pixel: z = cos(z) + sqr(z), |z| < (p1+3) }
   
   ScottCosH(XYAXIS) { z = pixel: z = cosh(z) + sqr(z), |z| < (p1+3) }

   ScottSZSA { z = pixel: z = sin(z*z), |z| < (p1+3) }
   
   ScottSZSB { z = pixel: z = sin(z)*sin(z), |z| < (p1+3) }

   ScottCZSA { z = pixel: z = cos(z*z), |z| < (p1+3) }
   
   ScottCZSB { z = pixel: z = cos(z)*cos(z), |z| < (p1+3) }

