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

 ScottSin(XAXIS) { z = pixel, TEST = (p1+3): z = sin(z) + sqr(z), |z|<TEST }

 ScottSinH(XAXIS) { z = pixel, TEST = (p1+3): z = sinh(z) + sqr(z), |z|<TEST }

 ScottCos(XYAXIS) { z = pixel, TEST = (p1+3): z = cos(z) + sqr(z), |z|<TEST }
 
 ScottCosH(XYAXIS) { z = pixel, TEST = (p1+3): z = cosh(z) + sqr(z), |z|<TEST }

 ScottSZSA(XYAXIS) { z = pixel, TEST = (p1+3): z = sin(z*z), |z|<TEST }
 
 ScottSZSB(XYAXIS) { z = pixel, TEST = (p1+3): z = sin(z)*sin(z), |z|<TEST }

 ScottCZSA(XYAXIS) { z = pixel, TEST = (p1+3): z = cos(z*z), |z|<TEST }
 
 ScottCZSB(XYAXIS) { z = pixel, TEST = (p1+3): z = cos(z)*cos(z), |z|<TEST }

 ScottLTS(XAXIS) { z = pixel, TEST = (p1+3): z = log(z)*sin(z), |z|<TEST }
 
 ScottLTC(XAXIS) { z = pixel, TEST = (p1+3): z = log(z)*cos(z), |z|<TEST }

 ScottLPC(XAXIS) { z = pixel, TEST = (p1+3): z = log(z)+cos(z), |z|<TEST }
 
 ScottLPS { z = pixel, TEST = (p1+3): z = log(z)+sin(z), |z|<TEST }
 
 ScottSIC(XYAXIS) { z = pixel, TEST = (p1+3): z = sqr(1/cos(z)), |z|<TEST }
 
 ScottSIS { z = pixel, TEST = (p1+3): z = sqr(1/sin(z)), |z|<TEST }

 ScottZSZZ(XAXIS) { z = pixel, TEST = (p1+3): z = (z*sin(z))+z, |z|<TEST }

 ScottZCZZ(XYAXIS) { z = pixel, TEST = (p1+3): z = (z*cos(z))+z, |z|<TEST }


 { Formulas from this point on are the contrubutions of Lee H. Skinner }

 { Tetration formula of Ackerman's Generalized Exponential }

 Tetrate(XAXIS) { c = z = pixel: z = c ^ z, |z| <= (P1+3) }

 { from expansion module 2 of Fractal Magic 5.0 (Lee stole it - not me!) }

 Spider(XAXIS) { c = z = pixel: z = z * z + c; c = c / 2 + z, |z| <= 4 }

 { "Dragonland" from EGA Fractal Master.  Default corners should really be }
 {  -2/4/-2.25/2.25  Not symmetrical to x=1, although it is close!         }

 Mandelglass(XAXIS) { z = (0.5, 0.0): z = pixel * z * (1 - z), |z| <= 4 }
