/*
	This module consists only of the fractalspecific structure
	and a *slew* of defines needed to get it to compile
*/

/* includes needed for fractalspecific */

#include <stdio.h>
#include <stdlib.h>
#include "fractint.h"
#include "mpmath.h"
#include "helpdefs.h"
#include "fractype.h"
#include "prototyp.h"

/* functions defined elswhere needed for fractalspecific */
/* moved to prototyp.h */

/* parameter descriptions */
/* Note: parameters preceded by + are integer parameters */

/* for Mandelbrots */
static char realz0[] = "Real Perturbation of Z(0)";
static char imagz0[] = "Imaginary Perturbation of Z(0)";

/* for Julias */
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

/* for Newtons */
static char newtdegree[] = "+Polynomial Degree (>= 2)";

/* for MarksMandel/Julia */
static char exponent[]   = "Real part of Exponent";
static char imexponent[] = "Imag part of Exponent";

/* for Complex Newton */
static char realroot[]	 = "Real part of Root";
static char imagroot[]	 = "Imag part of Root";
static char realdegree[] = "Real part of Degree";
static char imagdegree[] = "Imag part of Degree";

/* for Lorenz */
static char timestep[]	   = "Time Step";

/* for formula */
static char p1real[] = "Real portion of p1";
static char p2real[] = "Real portion of p2";
static char p1imag[] = "Imaginary portion of p1";
static char p2imag[] = "Imaginary portion of p2";

/* trig functions */
static char recoeftrg1[] = "Real Coefficient First Function";
static char imcoeftrg1[] = "Imag Coefficient First Function";
static char recoeftrg2[] = "Real Coefficient Second Function";
static char imcoeftrg2[] = "Imag Coefficient Second Function";

/* MCP 7-7-91
static char recoefsqr[] = "Real Coefficient Square Term";
static char imcoefsqr[] = "Imag Coefficient Square Term";
*/

static char recoef2nd[] = "Real Coefficient Second Term";
static char imcoef2nd[] = "Imag Coefficient Second Term";

/* KAM Torus */
static char kamangle[] = "Angle (radians)";
static char kamstep[] =  "Step size";
static char kamstop[] =  "Stop value";
static char pointsperorbit[] = "+Points per orbit";

/* Newtbasin */
static char stripes[] = "Enter non-zero value for stripes";

/* Gingerbreadman */
static char initx[] = "Initial x";
static char inity[] = "Initial y";

/* popcorn */
static char step[] = "Step size";

/* bifurcations */
static char filt[] = "+Filter Cycles";
static char seed[] = "Seed Population";

/* frothy basins */
static char frothattractor[] = "+3 or 6 attractor system";
static char frothshade[] =  "+Enter non-zero value for alternate color shading";

/* symmetrical icon fractals */
static char lambda[] = "Lambda";
static char alpha[]  = "Alpha";
static char beta[]   = "Beta";
static char gamma2[]  = "Gamma";
static char omega[]  = "Omega";
static char symdegree[] = "+Degree of symmetry";

static char shiftval[] = "Function Shift Value";

/* ifs */
static char color_method[] = "+Coloring method (0,1)";

/* orbit fractals */
static char A[] = "a";
static char B[] = "b";
static char D[] = "d";

/* 4D fractals */
static char C[] = "c";
static char C1[] = "c1";
static char CI[] = "ci";
static char CJ[] = "cj";
static char CK[] = "ck";
static char ZJ[] = "zj";
static char ZK[] = "zk";

/* empty string */
static char ES[] = "";
 
/* bailout defines */
#define FTRIGBAILOUT 2500.0
#define LTRIGBAILOUT   64.0
#define FROTHBAILOUT    6.0
#define STDBAILOUT	4.0
#define NOBAILOUT	0.0

struct moreparams far moreparams[] = 
{
    ICON,  omega,symdegree,ES,ES,ES,ES,0,3,0,0,0,0,
    ICON3D,omega,symdegree,ES,ES,ES,ES,0,3,0,0,0,0,
    HYPERCMPLXJFP,ZJ,ZK,   ES,ES,ES,ES,0,0,0,0,0,0,
    QUATJULFP    ,ZJ,ZK,   ES,ES,ES,ES,0,0,0,0,0,0,   
    -1,      NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,0,0
};         


struct fractalspecificstuff far fractalspecific[] =
{
   /*
     fractal name, parameter text strings, parameter values,
     helptext, helpformula, flags,
     xmin  xmax  ymin  ymax int tojulia   tomandel tofloat  symmetry
   |------|-----|-----|-----|--|--------|---------|--------|---------|
     orbit fnct     per_pixel fnct  per_image fnct  calctype fcnt    bailout
   |---------------|---------------|---------------|----------------|-------|
   */

   "mandel", realz0, imagz0,ES,ES,0,0,0,0,
   HT_MANDEL, HF_MANDEL, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, JULIA, NOFRACTAL, MANDELFP, XAXIS_NOPARM,
   JuliaFractal, mandel_per_pixel,MandelSetup, StandardFractal, STDBAILOUT,

   "julia", realparm, imagparm,ES,ES,0.3,0.6,0,0,
   HT_JULIA, HF_JULIA, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL, JULIAFP,  ORIGIN,
   JuliaFractal, julia_per_pixel, JuliaSetup, StandardFractal, STDBAILOUT,

   "*newtbasin", newtdegree,stripes, ES,ES,3,0,0,0,
   HT_NEWTBAS, HF_NEWTBAS, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN,   NOSYM,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "lambda",      realparm, imagparm,ES,ES,0.85,0.6,0,0,
   HT_LAMBDA, HF_LAMBDA, WINFRAC+OKJB,
   -1.5,  2.5, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP,  NOSYM,
   LambdaFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*mandel",    realz0, imagz0,ES,ES,0,0,0,0,
   HT_MANDEL, HF_MANDEL, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,   NOFRACTAL, MANDEL,  XAXIS_NOPARM,
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup, StandardFractal, STDBAILOUT,

   "*newton", newtdegree, ES, ES,ES,3,0,0,0,
   HT_NEWT, HF_NEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTON,	XAXIS,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "*julia",     realparm, imagparm,ES,ES,0.3,0.6,0,0,
   HT_JULIA, HF_JULIA, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP, JULIA,  ORIGIN,
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup, StandardFractal,STDBAILOUT,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)",
                  "+Algorithm (0 = original, 1 = new)",
                  "+Random Seed Value (0 = Random, 1 = Reuse Last)",
                  "+Save as Pot File? (0 = No,     1 = Yes)",
                  2,0,0,0,
   HT_PLASMA, HF_PLASMA, NOZOOM+NOGUESS+NOTRACE+NORESUME+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	   NULL,   StandaloneSetup,	 plasma,	  NOBAILOUT,

   "*mandelfn",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_MANDFN, HF_MANDFN, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDATRIGFP,NOFRACTAL, MANDELTRIG, XYAXIS_NOPARM,
   LambdaTrigfpFractal,othermandelfp_per_pixel,MandelTrigSetup,StandardFractal,LTRIGBAILOUT,

   "*manowar",    realz0, imagz0,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_MANOWAR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, MANOWARJFP, NOFRACTAL, MANOWAR,  XAXIS_NOPARM,
   ManOWarfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "manowar",    realz0, imagz0,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_MANOWAR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, MANOWARJ, NOFRACTAL, MANOWARFP, XAXIS_NOPARM,
   ManOWarFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "test","(testpt Param #1)","(testpt param #2)","(testpt param #3)","(testpt param #4)",0,0,0,0,
   HT_TEST, HF_TEST, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	  NULL, 	    StandaloneSetup, test,    STDBAILOUT,

   "sierpinski",  ES,ES,ES,ES,0,0,0,0,
   HT_SIER, HF_SIER, WINFRAC,
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP,   NOSYM,
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,

   "barnsleym1",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL, BARNSLEYM1FP,  XYAXIS_NOPARM,
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj1",  realparm, imagparm,ES,ES,0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ1, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP,  ORIGIN,
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "barnsleym2",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL, BARNSLEYM2FP,  YAXIS_NOPARM,
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj2",  realparm, imagparm,ES,ES,0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ2, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP,  ORIGIN,
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "sqr(fn)", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SQRFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQRTRIGFP,XAXIS,
   SqrTrigFractal,   long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(fn)", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SQRFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, SQRTRIG,XAXIS,
   SqrTrigfpFractal,   otherjuliafp_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "fn+fn", recoeftrg1, imcoeftrg1,recoeftrg2, imcoeftrg2,1,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIGFP,XAXIS,
   TrigPlusTrigFractal,   long_julia_per_pixel, TrigPlusTriglongSetup,	StandardFractal,LTRIGBAILOUT,

   "mandellambda",realz0, imagz0,ES,ES,0,0,0,0,
   HT_MLAMBDA, HF_MLAMBDA, WINFRAC,
   -3.0,  5.0, -3.0,  3.0, 1, LAMBDA,	 NOFRACTAL, MANDELLAMBDAFP,  XAXIS_NOPARM,
   LambdaFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksmandel", realz0, imagz0, exponent,ES,0,0,1,0,
   HT_MARKS, HF_MARKSMAND, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL, MARKSMANDELFP,  NOSYM,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksjulia", realparm, imagparm, exponent,ES,0.1,0.9,0,0,
   HT_MARKS, HF_MARKSJULIA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL, MARKSJULIAFP,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,STDBAILOUT,

   "unity", ES,ES,ES,ES,0,0,0,0,
   HT_UNITY, HF_UNITY, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, UNITYFP,   XYAXIS,
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,NOBAILOUT,

   "mandel4", realz0, imagz0,ES,ES,0,0,0,0,
   HT_MANDJUL4, HF_MANDEL4, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,	  NOFRACTAL, MANDEL4FP,  XAXIS_NOPARM,
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  STDBAILOUT,

   "julia4", realparm, imagparm,ES,ES,0.6,0.55,0,0,
   HT_MANDJUL4, HF_JULIA4, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4, JULIA4FP, ORIGIN,
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,	 STDBAILOUT,

   "ifs",color_method,ES,ES,ES,0,0,0,0,
   HT_IFS, -1, NOGUESS+NOTRACE+NORESUME+WINFRAC,
   -8.0,  8.0, -1.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,  NOSYM,
   NULL,	  NULL,      StandaloneSetup, ifs,    NOBAILOUT,

   "*ifs3d",color_method,ES,ES,ES,0,0,0,0,
   HT_IFS, -1, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -11.0,  11.0, -11.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,	  NULL,      StandaloneSetup, ifs,    NOBAILOUT,

   "barnsleym3",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL, BARNSLEYM3FP,  XAXIS_NOPARM,
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj3",  realparm, imagparm,ES,ES,0.1,0.36,0,0,
   HT_BARNS, HF_BARNSJ3, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP,  NOSYM,
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "fn(z*z)", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGSQRFP,XYAXIS,
   TrigZsqrdFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*fn(z*z)", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGSQR,XYAXIS,
   TrigZsqrdfpFractal,	 juliafp_per_pixel, JuliafpSetup,  StandardFractal,STDBAILOUT,

   "*bifurcation",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFURCATION, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL, LBIFURCATION, NOSYM,
   BifurcVerhulstTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*fn+fn",recoeftrg1,imcoeftrg1,recoeftrg2,imcoeftrg2,1,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIG,XAXIS,
   TrigPlusTrigfpFractal, otherjuliafp_per_pixel, TrigPlusTrigfpSetup,	StandardFractal,LTRIGBAILOUT,

   "fn*fn", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGXTRIGFP,XAXIS,
   TrigXTrigFractal, long_julia_per_pixel, FnXFnSetup,	StandardFractal,LTRIGBAILOUT,

   "*fn*fn", ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGXTRIG,XAXIS,
   TrigXTrigfpFractal, otherjuliafp_per_pixel, FnXFnSetup,  StandardFractal,LTRIGBAILOUT,

   "sqr(1/fn)",ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SQROVFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQR1OVERTRIGFP,NOSYM,
   Sqr1overTrigFractal, long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(1/fn)",ES,ES,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SQROVFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, SQR1OVERTRIG,NOSYM,
   Sqr1overTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "fn*z+z",recoeftrg1, imcoeftrg1, recoef2nd,imcoef2nd,1,0,1,0,
   HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 1, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZFP,XAXIS,
   ZXTrigPlusZFractal,julia_per_pixel,ZXTrigPlusZSetup,  StandardFractal,LTRIGBAILOUT,

   "*fn*z+z",recoeftrg1, imcoeftrg2, recoef2nd,imcoef2nd,1,0,1,0,
   HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZ,XAXIS,
   ZXTrigPlusZfpFractal,   juliafp_per_pixel, ZXTrigPlusZSetup,  StandardFractal,LTRIGBAILOUT,

   "*kamtorus",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150,
   HT_KAM, HF_KAM, NOGUESS+NOTRACE+WINFRAC,
   -1.0,  1.0, -.75, .75, 0, NOFRACTAL, NOFRACTAL, KAM,   NOSYM,
   kamtorusfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat, NOBAILOUT,

   "kamtorus",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150,
   HT_KAM, HF_KAM, NOGUESS+NOTRACE+WINFRAC,
   -1.0,  1.0, -.75, .75,16, NOFRACTAL, NOFRACTAL, KAMFP, NOSYM,
   kamtoruslongorbit,  NULL, orbit3dlongsetup, orbit2dlong,   NOBAILOUT,

   "*kamtorus3d",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150,
   HT_KAM, HF_KAM, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -3.0,  3.0, -1, 3.5, 0, NOFRACTAL, NOFRACTAL, KAM3D,   NOSYM,
   kamtorusfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "kamtorus3d",kamangle,kamstep,kamstop,pointsperorbit,1.3,.05,1.5,150,
   HT_KAM, HF_KAM, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -3.0,  3.0, -1, 3.5,16, NOFRACTAL, NOFRACTAL, KAM3DFP, NOSYM,
   kamtoruslongorbit,  NULL, orbit3dlongsetup, orbit3dlong,   NOBAILOUT,

   "lambdafn",     realparm, imagparm,ES,ES,1.0,0.4,0,0,
   HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+WINFRAC+OKJB,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, MANDELTRIG, LAMBDATRIGFP,PI_SYM,
   LambdaTrigFractal,long_julia_per_pixel, LambdaTrigSetup,	StandardFractal,LTRIGBAILOUT,

   "manfn+zsqrd",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 16, LJULTRIGPLUSZSQRD,  NOFRACTAL, FPMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredFractal,mandel_per_pixel,MandellongSetup,StandardFractal, STDBAILOUT,

   "julfn+zsqrd",  realparm, imagparm,ES,ES,-0.5,0.5,0,0,
   HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 16, NOFRACTAL, LMANTRIGPLUSZSQRD, FPJULTRIGPLUSZSQRD,	NOSYM,
   TrigPlusZsquaredFractal,julia_per_pixel, JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*manfn+zsqrd", realz0, imagz0,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULTRIGPLUSZSQRD,   NOFRACTAL, LMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal, STDBAILOUT,

   "*julfn+zsqrd", realparm, imagparm,ES,ES,-0.5,0.5,0,0,
   HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANTRIGPLUSZSQRD, LJULTRIGPLUSZSQRD, NOSYM,
   TrigPlusZsquaredfpFractal, juliafp_per_pixel,  JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*lambdafn",  realparm, imagparm,ES,ES,1.0,0.4,0,0,
   HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELTRIGFP, LAMBDATRIG, PI_SYM,
   LambdaTrigfpFractal,otherjuliafp_per_pixel,LambdaTrigSetup,StandardFractal,LTRIGBAILOUT,

   "mandelfn",realz0, imagz0,ES,ES, 0,0,0,0,
   HT_MANDFN, HF_MANDFN, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 16, LAMBDATRIG, NOFRACTAL, MANDELTRIGFP, XYAXIS_NOPARM,
   LambdaTrigFractal,long_mandel_per_pixel,MandelTrigSetup,StandardFractal,LTRIGBAILOUT,

   "manzpower", realz0, imagz0, exponent,imexponent,0,0,2,0,
   HT_PICKMJ, HF_MANZPOWER, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER,	XAXIS_NOIMAG,
   longZpowerFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julzpower", realparm, imagparm, exponent,imexponent,0.3,0.6,2,0,
   HT_PICKMJ, HF_JULZPOWER, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER,	 ORIGIN,
   longZpowerFractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "*manzpower", realz0, imagz0, exponent,imexponent,0,0,2,0,
   HT_PICKMJ, HF_MANZPOWER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULIAZPOWER,   NOFRACTAL, LMANDELZPOWER,  XAXIS_NOIMAG,
   floatZpowerFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julzpower", realparm, imagparm, exponent,imexponent,0.3,0.6,2,0,
   HT_PICKMJ, HF_JULZPOWER, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER,	ORIGIN,
   floatZpowerFractal, otherjuliafp_per_pixel,	JuliafpSetup,StandardFractal,STDBAILOUT,

   "manzzpwr", realz0, imagz0, exponent,ES,0,0,2,0,
   HT_PICKMJ, HF_MANZZPWR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULZTOZPLUSZPWR,   NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM,
   floatZtozPluszpwrFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "julzzpwr", realparm, imagparm, exponent,ES,-0.3,0.3,2,0,
   HT_PICKMJ, HF_JULZZPWR, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL,	NOSYM,
   floatZtozPluszpwrFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "manfn+exp",realz0, imagz0,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 16, LJULTRIGPLUSEXP,    NOFRACTAL,  FPMANTRIGPLUSEXP, XAXIS_NOPARM,
   LongTrigPlusExponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julfn+exp", realparm, imagparm,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+WINFRAC+OKJB,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANTRIGPLUSEXP,FPJULTRIGPLUSEXP, NOSYM,
   LongTrigPlusExponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*manfn+exp", realz0, imagz0,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, FPJULTRIGPLUSEXP, NOFRACTAL, LMANTRIGPLUSEXP,   XAXIS_NOPARM,
   FloatTrigPlusExponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julfn+exp", realparm, imagparm,ES,ES,0,0,0,0,
   HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+WINFRAC+OKJB,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANTRIGPLUSEXP, LJULTRIGPLUSEXP,   NOSYM,
   FloatTrigPlusExponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*popcorn", step, ES, ES,ES,0.05,0,0,0,
   HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORN,  NOPLOT,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,  popcorn,STDBAILOUT,

   "popcorn", step, ES, ES,ES,0.05,0,0,0,
   HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORN,  NOPLOT,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,popcorn,STDBAILOUT,

   "*lorenz",timestep,A,B, C,.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -15,  15, 0, 30, 0, NOFRACTAL, NOFRACTAL, LLORENZ,   NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "lorenz",timestep,A,B, C,.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -15,  15, 0, 30, 16, NOFRACTAL, NOFRACTAL, FPLORENZ,   NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "lorenz3d",timestep,A,B, C,.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 16, NOFRACTAL, NOFRACTAL, FPLORENZ3D,   NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "newton", newtdegree,ES, ES,ES,3,0,0,0,
   HT_NEWT, HF_NEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTON,   XAXIS,
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "newtbasin", newtdegree,stripes, ES,ES,3,0,0,0,
   HT_NEWTBAS, HF_NEWTBAS, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTBASIN,	 NOSYM,
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "complexnewton", realdegree,imagdegree,realroot,imagroot,3,0,1,0,
   HT_NEWTCMPLX, HF_COMPLEXNEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   ComplexNewton, otherjuliafp_per_pixel,  ComplexNewtonSetup, StandardFractal,NOBAILOUT,

   "complexbasin", realdegree,imagdegree,realroot,imagroot,3,0,1,0,
   HT_NEWTCMPLX, HF_COMPLEXNEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   ComplexBasin, otherjuliafp_per_pixel,  ComplexNewtonSetup,  StandardFractal, NOBAILOUT,

   "cmplxmarksmand", realz0, imagz0, realdegree, imagdegree,0,0,1,0,
   HT_MARKS, HF_CMPLXMARKSMAND, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, COMPLEXMARKSJUL, NOFRACTAL, NOFRACTAL,   NOSYM,
   MarksCplxMand, MarksCplxMandperp, MandelfpSetup, StandardFractal, STDBAILOUT,

   "cmplxmarksjul", realparm, imagparm, realdegree, imagdegree,0.3,0.6,1,0,
   HT_MARKS, HF_CMPLXMARKSJUL, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, COMPLEXMARKSMAND, NOFRACTAL,	NOSYM,
   MarksCplxMand, juliafp_per_pixel, JuliafpSetup, StandardFractal, STDBAILOUT,

   "formula", p1real, p1imag, p2real, p2imag, 0,0,0,0,
   HT_FORMULA, -2, WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, FFORMULA, SETUP_SYM,
   Formula, form_per_pixel, intFormulaSetup, StandardFractal, 0,

   "*formula", p1real, p1imag, p2real, p2imag, 0,0,0,0,
   HT_FORMULA, -2, WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, FORMULA, SETUP_SYM,
   Formula, form_per_pixel, fpFormulaSetup, StandardFractal, 0,

   "*sierpinski",  ES,ES,ES,ES,0,0,0,0,
   HT_SIER, HF_SIER, WINFRAC,
   -0.9,  1.7, -0.9,  1.7, 0, NOFRACTAL, NOFRACTAL, SIERPINSKI,   NOSYM,
   SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,StandardFractal,127.0,

   "*lambda", realparm, imagparm,ES,ES,0.85,0.6,0,0,
   HT_LAMBDA, HF_LAMBDA, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA,  NOSYM,
   LambdaFPFractal,   juliafp_per_pixel, JuliafpSetup,	StandardFractal,STDBAILOUT,

   "*barnsleym1", realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ1FP,NOFRACTAL, BARNSLEYM1,  XYAXIS_NOPARM,
   Barnsley1FPFractal, othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj1", realparm, imagparm,ES,ES,0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ1, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1,  ORIGIN,
   Barnsley1FPFractal, otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym2", realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ2FP,NOFRACTAL, BARNSLEYM2,  YAXIS_NOPARM,
   Barnsley2FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj2", realparm, imagparm,ES,ES,0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ2, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2,  ORIGIN,
   Barnsley2FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym3", realz0, imagz0,ES,ES,0,0,0,0,
   HT_BARNS, HF_BARNSM3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3,  XAXIS_NOPARM,
   Barnsley3FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj3", realparm, imagparm,ES,ES,0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ3, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3,  XAXIS,
   Barnsley3FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*mandellambda",realz0, imagz0,ES,ES,0,0,0,0,
   HT_MLAMBDA, HF_MLAMBDA, WINFRAC,
   -3.0,  5.0, -3.0,  3.0, 0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA,  XAXIS_NOPARM,
   LambdaFPFractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "julibrot", ES,ES,ES,ES,0,0,0,0,
   HT_JULIBROT, -1, NOGUESS+NOTRACE+NOROTATE+NORESUME+WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, JULIBROTFP, NOSYM,
   JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal, STDBAILOUT,

   "*lorenz3d",timestep,A,B,C,.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, LLORENZ3D,   NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "rossler3d",timestep,A,B,C,.04,.2,.2,5.7,
   HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -20.0,   40.0, 16, NOFRACTAL, NOFRACTAL, FPROSSLER,	NOSYM,
   rosslerlongorbit, NULL,	   orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "*rossler3d",timestep,A,B,C,.04,.2,.2,5.7,
   HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -20.0,   40.0, 0, NOFRACTAL, NOFRACTAL, LROSSLER,   NOSYM,
   rosslerfloatorbit, NULL,	    orbit3dfloatsetup, orbit3dfloat,	NOBAILOUT,

   "henon",A,B,ES,ES,1.4,.3,0,0,
   HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -1.4,  1.4,	-.5,   .5, 16, NOFRACTAL, NOFRACTAL, FPHENON,	NOSYM,
   henonlongorbit, NULL,	 orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "*henon",A,B,ES,ES,1.4,.3,0,0,
   HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -1.4,  1.4,	-.5,   .5, 0, NOFRACTAL, NOFRACTAL, LHENON,   NOSYM,
   henonfloatorbit, NULL,	  orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "pickover",A,B,C,D,2.24,.43,-.65,-2.43,
   HT_PICK, HF_PICKOVER, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -2.8,  2.8,	-2.0, 2.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   pickoverfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "gingerbreadman",initx,inity,ES,ES,-.1,0,0,0,
   HT_GINGER, HF_GINGER, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -4.5,  8.5,	-4.5, 8.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   gingerbreadfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "diffusion", "+Border size","+Type (0=Central,1=Falling,2=Square Cavity)",ES, ES,10,0,0,0,
   HT_DIFFUS, HF_DIFFUS, NOZOOM+NOGUESS+NOTRACE+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,   NULL,     StandaloneSetup, diffusion,    NOBAILOUT,

   "*unity", ES,ES,ES,ES,0,0,0,0,
   HT_UNITY, HF_UNITY, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, UNITY,   XYAXIS,
   UnityfpFractal, otherjuliafp_per_pixel,StandardSetup,StandardFractal,NOBAILOUT,

   "*spider", realz0, imagz0,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SPIDER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, SPIDER,  XAXIS_NOPARM,
   SpiderfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "spider",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_SPIDER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, SPIDERFP,  XAXIS_NOPARM,
   SpiderFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "tetrate", realz0, imagz0,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_TETRATE, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	XAXIS,
   TetratefpFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "magnet1m", realz0, imagz0,ES,ES,0,0,0,0,
   HT_MAGNET, HF_MAGM1, WINFRAC,
   -4.0, 4.0, -3.0, 3.0, 0, MAGNET1J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet1Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet1j", realparm, imagparm,ES,ES,0,0,0,0,
   HT_MAGNET, HF_MAGJ1, WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET1M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet1Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "magnet2m", realz0, imagz0,ES,ES,0,0,0,0,
   HT_MAGNET, HF_MAGM2, WINFRAC,
   -1.5,3.7, -1.95,1.95,   0, MAGNET2J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet2Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet2j", realparm, imagparm,ES,ES,0,0,0,0,
   HT_MAGNET, HF_MAGJ2, WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET2M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet2Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "bifurcation",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFURCATION, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   1.9,  3.0, 0,  1.34, 1, NOFRACTAL, NOFRACTAL, BIFURCATION, NOSYM,
   LongBifurcVerhulstTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "biflambda",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFLAMBDA, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.0, 4.0, -1.0, 2.0, 1, NOFRACTAL, NOFRACTAL, BIFLAMBDA,   NOSYM,
   LongBifurcLambdaTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*biflambda",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFLAMBDA, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.0, 4.0, -1.0, 2.0, 0, NOFRACTAL, NOFRACTAL, LBIFLAMBDA,  NOSYM,
   BifurcLambdaTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bif+sinpi",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFPLUSSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.275,1.45, 0.0, 2.0, 0, NOFRACTAL, NOFRACTAL, LBIFADSINPI,	 NOSYM,
   BifurcAddTrigPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bif=sinpi",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFEQSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.5, 2.5, -3.5, 3.5, 0, NOFRACTAL, NOFRACTAL, LBIFEQSINPI,	 NOSYM,
   BifurcSetTrigPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*popcornjul", step, ES, ES,ES,0.05,0,0,0,
   HT_POPCORN, HF_POPCJUL, WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORNJUL,  ORIGIN,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "popcornjul", step, ES, ES,ES,0.05,0,0,0,
   HT_POPCORN, HF_POPCJUL, WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORNJUL,  ORIGIN,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "lsystem", "+Order",ES,ES,ES,2,0,0,0,
   HT_LSYS, -3, NOZOOM+NORESUME+NOGUESS+NOTRACE+WINFRAC,
   -1, 1, -1, 1, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   NULL, NULL, StandaloneSetup, Lsystem, NOBAILOUT,

   "*manowarj", realparm, imagparm,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_MANOWARJ, WINFRAC+OKJB,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, MANOWARFP, MANOWARJ,   NOSYM,
   ManOWarfpFractal,juliafp_per_pixel, JuliafpSetup,StandardFractal,STDBAILOUT,

   "manowarj",  realparm, imagparm,ES,ES,0,0,0,0,
   HT_SCOTSKIN, HF_MANOWARJ, WINFRAC+OKJB,
   -2.5,  1.5, -1.5, 1.5, 1, NOFRACTAL, MANOWAR,   MANOWARJFP, NOSYM,
   ManOWarFractal,julia_per_pixel, JulialongSetup,StandardFractal,STDBAILOUT,

   "*fn(z)+fn(pix)", realz0,imagz0,recoeftrg2,imcoeftrg2,0,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXLONG, NOSYM,
   Richard8fpFractal,otherrichard8fp_per_pixel, MandelfpSetup,StandardFractal,LTRIGBAILOUT,

   "fn(z)+fn(pix)",  realz0,imagz0,recoeftrg2,imcoeftrg2,0,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2,
   -2.5,  1.5, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXFP, NOSYM,
   Richard8Fractal,long_richard8_per_pixel, MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "*marksmandelpwr", realz0, imagz0,ES,ES,0,0,0,0,
   HT_MARKS, HF_MARKSMANDPWR, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, MARKSMANDELPWR, XAXIS_NOPARM,
   MarksMandelPwrfpFractal,marks_mandelpwrfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "marksmandelpwr",  realz0, imagz0,ES,ES,0,0,0,0,
   HT_MARKS, HF_MARKSMANDPWR, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL,	 NOFRACTAL, MARKSMANDELPWRFP,  XAXIS_NOPARM,
   MarksMandelPwrFractal,marks_mandelpwr_per_pixel, MandelSetup,StandardFractal,STDBAILOUT,

   "*tim's_error",    realz0, imagz0,ES,ES,0,0,0,0,
   HT_MARKS, HF_TIMSERR, WINFRAC+TRIG1,
   -2.5,  3.0, -2.0,  2.0, 0, NOFRACTAL,	 NOFRACTAL, TIMSERROR,	XAXIS_NOPARM,
   TimsErrorfpFractal,marks_mandelpwrfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "tim's_error",    realz0, imagz0,ES,ES,0,0,0,0,
   HT_MARKS, HF_TIMSERR, WINFRAC+TRIG1,
   -2.5,  3.0, -2.0,  2.0, 1, NOFRACTAL,	 NOFRACTAL, TIMSERRORFP,  XAXIS_NOPARM,
   TimsErrorFractal,marks_mandelpwr_per_pixel, MandelSetup,StandardFractal,STDBAILOUT,

   "bif=sinpi",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFEQSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.5, 2.5, -3.5, 3.5, 1, NOFRACTAL, NOFRACTAL, BIFEQSINPI,	 NOSYM,
   LongBifurcSetTrigPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bif+sinpi",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFPLUSSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.275,1.45, 0.0, 2.0, 1, NOFRACTAL, NOFRACTAL, BIFADSINPI,	 NOSYM,
   LongBifurcAddTrigPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bifstewart",filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFSTEWART, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.7,2.0, -1.1,1.1, 0, NOFRACTAL, NOFRACTAL, LBIFSTEWART, NOSYM,
   BifurcStewartTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bifstewart", filt,seed,ES,ES,1000.0,0.66,0,0,
   HT_BIF, HF_BIFSTEWART, TRIG1+NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.7,2.0, -1.1,1.1, 1, NOFRACTAL, NOFRACTAL, BIFSTEWART, NOSYM,
   LongBifurcStewartTrig, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "hopalong",A,B,C,ES,.4,1,0,0,
   HT_MARTIN, HF_HOPALONG, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -2.0, 3.0, -1.625, 2.625, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	NOSYM,
   hopalong2dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "circle", "magnification",ES,ES,ES,200000,0,0,0,
   HT_CIRCLE, HF_CIRCLE, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,  XYAXIS,
   CirclefpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,NOBAILOUT,

   "martin",A,ES,ES,ES,3.14,0,0,0,
   HT_MARTIN, HF_MARTIN, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -32,  32, -32, 32, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	NOSYM,
   martin2dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

  "lyapunov", "+Order (integer)", "Population Seed", "+Filter Cycles", ES, 0, 0.5, 0, 0,
   HT_LYAPUNOV, HT_LYAPUNOV, WINFRAC,
   -8.0, 8.0, -6.0, 6.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   BifurcLambda, NULL, lya_setup, lyapunov, NOBAILOUT,

   "lorenz3d1",timestep,A,B,C,.02,5,15,1,
   HT_LORENZ, HF_LORENZ3D1, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d1floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "lorenz3d3",timestep,A,B,C,.02,10,28,2.66,
   HT_LORENZ, HF_LORENZ3D3, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d3floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "lorenz3d4",timestep,A,B,C,.02,10,28,2.66,
   HT_LORENZ, HF_LORENZ3D4, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d4floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "lambda(fn||fn)", realparm, imagparm, shiftval, ES,1,0.1,1,0,
   HT_FNORFN, HF_LAMBDAFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANLAMFNFN, FPLAMBDAFNFN, ORIGIN,
   LambdaTrigOrTrigFractal, long_julia_per_pixel, LambdaTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "*lambda(fn||fn)", realparm, imagparm, shiftval,ES,1,0.1,1,0,
   HT_FNORFN, HF_LAMBDAFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANLAMFNFN, LLAMBDAFNFN,ORIGIN,
   LambdaTrigOrTrigfpFractal, otherjuliafp_per_pixel, LambdaTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "julia(fn||fn)", realparm, imagparm, shiftval, ES,0,0,8,0,
   HT_FNORFN, HF_JULIAFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANFNFN, FPJULFNFN,XAXIS,
   JuliaTrigOrTrigFractal, long_julia_per_pixel, JuliaTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "*julia(fn||fn)", realparm, imagparm, shiftval,ES,0,0,8,0,
   HT_FNORFN, HF_JULIAFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANFNFN, LJULFNFN,XAXIS,
   JuliaTrigOrTrigfpFractal, otherjuliafp_per_pixel, JuliaTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "manlam(fn||fn)", realz0, imagz0, shiftval,ES,0,0,10,0,
   HT_FNORFN, HF_MANLAMFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, LLAMBDAFNFN, NOFRACTAL, FPMANLAMFNFN,XAXIS_NOPARM,
   LambdaTrigOrTrigFractal, long_mandel_per_pixel, ManlamTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "*manlam(fn||fn)", realz0, imagz0, shiftval,ES,0,0,10,0,
   HT_FNORFN, HF_MANLAMFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, FPLAMBDAFNFN, NOFRACTAL, LMANLAMFNFN,XAXIS_NOPARM,
   LambdaTrigOrTrigfpFractal, othermandelfp_per_pixel, ManlamTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "mandel(fn||fn)", realz0, imagz0, shiftval,ES,0,0,0.5,0,
   HT_FNORFN, HF_MANDELFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 16, LJULFNFN, NOFRACTAL, FPMANFNFN,XAXIS_NOPARM,
   JuliaTrigOrTrigFractal, long_mandel_per_pixel, MandelTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "*mandel(fn||fn)", realz0, imagz0, shiftval,ES,0,0,0.5,0,
   HT_FNORFN, HF_MANDELFNFN, TRIG2,
   -4.0,  4.0, -3.0,  3.0, 0, FPJULFNFN, NOFRACTAL, LMANFNFN,XAXIS_NOPARM,
   JuliaTrigOrTrigfpFractal, othermandelfp_per_pixel, MandelTrigOrTrigSetup, StandardFractal,LTRIGBAILOUT,

   "bifmay",filt,seed,"Beta >= 2",ES,300.0,0.9,5,0,
   HT_BIF, HF_BIFMAY, NOGUESS+NOTRACE+NOROTATE,
   -3.5, -0.9, -0.5, 3.2, 16, NOFRACTAL, NOFRACTAL, BIFMAY,	 NOSYM,
   LongBifurcMay, NULL, BifurcMaySetup, Bifurcation, NOBAILOUT,

   "*bifmay",filt,seed,"Beta >= 2",ES,300.0,0.9,5,0,
   HT_BIF, HF_BIFMAY, NOGUESS+NOTRACE+NOROTATE,
   -3.5, -0.9, -0.5, 3.2, 0, NOFRACTAL, NOFRACTAL, LBIFMAY,	 NOSYM,
   BifurcMay, NULL, BifurcMaySetup, Bifurcation, NOBAILOUT,

  "halley", "+Order (integer > 1)", "Relaxation coefficient", "Epsilon", ES, 6, 1.0, 0.0001, 0,
   HT_HALLEY, HF_HALLEY, 0,
   -2.0, 2.0, -1.3, 1.3, 0, NOFRACTAL, NOFRACTAL, HALLEY, XYAXIS,
   MPCHalleyFractal, MPCHalley_per_pixel, HalleySetup, StandardFractal, NOBAILOUT,

  "*halley", "+Order (integer > 1)", "Relaxation coefficient", "Epsilon", ES, 6, 1.0, 0.0001, 0,
   HT_HALLEY, HF_HALLEY, 0,
   -2.0, 2.0, -1.3, 1.3, 0, NOFRACTAL, NOFRACTAL, MPHALLEY, XYAXIS,
   HalleyFractal, Halley_per_pixel, HalleySetup, StandardFractal, NOBAILOUT,

   "dynamic","+# of intervals (<0 = connect)","time step (<0 = Euler",A,B,
   50,.1,1,3,
   HT_DYNAM, HF_DYNAM, NOGUESS+NOTRACE+NORESUME+WINFRAC+TRIG1,
   -20.0,  20.0,  -20.0,   20.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   dynamfloat, NULL, dynam2dfloatsetup, dynam2dfloat, NOBAILOUT,

   "quat", "notused", "notused",CJ,CK,0,0,0,0,
   HT_QUAT, HF_QUAT, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, QUATJULFP, NOFRACTAL, NOFRACTAL,  XAXIS,
   QuaternionFPFractal, quaternionfp_per_pixel,MandelfpSetup,StandardFractal,
   LTRIGBAILOUT,

   "quatjul", C1, CI, CJ,CK,-.745,0,.113,.05,
   HT_QUAT, HF_QUATJ, WINFRAC+OKJB+MORE,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, QUATFP, NOFRACTAL,  ORIGIN,
   QuaternionFPFractal, quaternionjulfp_per_pixel,JuliafpSetup,StandardFractal,
   LTRIGBAILOUT,

   "cellular", "Initial String | 0 = Random | -1 = Reuse Last Random",
   "Rule = # of digits (see below) | 0 = Random",
   "Type (see below)",
   "Starting Row Number",
   11.0, 3311100320.0, 41.0, 0,
   HT_CELLULAR, HF_CELLULAR, NOGUESS+NOTRACE+NOZOOM,
   -1.0,  1.0, -1.0,  1.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,  NOSYM,
   NULL,	   NULL,   CellularSetup,	 cellular,	  NOBAILOUT,

   "*julibrot", ES,ES,ES,ES,0, 0, 0, 0,
   HT_JULIBROT, -1, NOGUESS+NOTRACE+NOROTATE+NORESUME+WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, JULIBROT, NOSYM,
   JuliafpFractal, jbfp_per_pixel, JulibrotSetup, Std4dfpFractal, STDBAILOUT,

#ifdef RANDOM_RUN
   "julia_inverse", realparm, imagparm,
   "Max Hits per Pixel", "Random Run Interval", -0.11, 0.6557, 4, 1024,
   HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+WINFRAC+NORESUME,
   -2.0,  2.0, -1.5, 1.5, 24, NOFRACTAL, MANDEL, INVERSEJULIAFP,  NOSYM,
   Linverse_julia_orbit, NULL, orbit3dlongsetup, inverse_julia_per_image, NOBAILOUT,

   "*julia_inverse", realparm, imagparm,
   "Max Hits per Pixel", "Random Run Interval", -0.11, 0.6557, 4, 1024,
   HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+WINFRAC+NORESUME,
   -2.0,  2.0, -1.5, 1.5,  0, NOFRACTAL, MANDEL, INVERSEJULIA,  NOSYM,
   Minverse_julia_orbit, NULL, orbit3dfloatsetup, inverse_julia_per_image, NOBAILOUT,
#else
   "julia_inverse", realparm, imagparm,
   "Max Hits per Pixel", ES, -0.11, 0.6557, 4, 1024,
   HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+WINFRAC+NORESUME,
   -2.0,  2.0, -1.5, 1.5, 24, NOFRACTAL, MANDEL, INVERSEJULIAFP,  NOSYM,
   Linverse_julia_orbit, NULL, orbit3dlongsetup, inverse_julia_per_image, NOBAILOUT,

   "*julia_inverse", realparm, imagparm,
   "Max Hits per Pixel", ES, -0.11, 0.6557, 4, 1024,
   HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+WINFRAC+NORESUME,
   -2.0,  2.0, -1.5, 1.5,  0, NOFRACTAL, MANDEL, INVERSEJULIA,  NOSYM,
   Minverse_julia_orbit, NULL, orbit3dfloatsetup, inverse_julia_per_image, NOBAILOUT,

#endif

   "mandelcloud","+# of intervals (<0 = connect)",ES,ES,ES,
   50,0,0,0,
   HT_MANDELCLOUD, HF_MANDELCLOUD, NOGUESS+NOTRACE+NORESUME+WINFRAC,
   -2.5,  1.5,  -1.5,   1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   mandelcloudfloat, NULL, dynam2dfloatsetup, dynam2dfloat, NOBAILOUT,

   "phoenix",p1real,p2real,"Degree of Z = 0 | >= 2 | <= -3",ES,0.56667,-0.5,0,0,
   HT_PHOENIX, HF_PHOENIX, 0,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, MANDPHOENIX, PHOENIXFP, XAXIS,
   LongPhoenixFractal, long_phoenix_per_pixel, PhoenixSetup, StandardFractal, STDBAILOUT,

   "*phoenix",p1real,p2real,"Degree of Z = 0 | >= 2 | <= -3",ES,0.56667,-0.5,0,0,
   HT_PHOENIX, HF_PHOENIX, 0,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, MANDPHOENIXFP, PHOENIX, XAXIS,
   PhoenixFractal, phoenix_per_pixel, PhoenixSetup, StandardFractal, STDBAILOUT,

   "mandphoenix",realz0,imagz0,"Degree of Z = 0 | >= 2 | <= -3",ES,0.0,0.0,0,0,
   HT_PHOENIX, HF_MANDPHOENIX, 0,
   -2.5, 1.5, -1.5, 1.5, 1, PHOENIX, NOFRACTAL, MANDPHOENIXFP, NOSYM,
   LongPhoenixFractal, long_mandphoenix_per_pixel, MandPhoenixSetup, StandardFractal, STDBAILOUT,

   "*mandphoenix",realz0,imagz0,"Degree of Z = 0 | >= 2 | <= -3",ES,0.0,0.0,0,0,
   HT_PHOENIX, HF_MANDPHOENIX, 0,
   -2.5, 1.5, -1.5, 1.5, 0, PHOENIXFP, NOFRACTAL, MANDPHOENIX, NOSYM,
   PhoenixFractal, mandphoenix_per_pixel, MandPhoenixSetup, StandardFractal, STDBAILOUT,

   "hypercomplex", "notused", "notused",CJ,CK,0,0,0,0,
   HT_HYPERC, HF_HYPERC, WINFRAC+OKJB+TRIG1,
   -2.0,  2.0, -1.5,  1.5, 0, HYPERCMPLXJFP, NOFRACTAL, NOFRACTAL,  XAXIS,
   HyperComplexFPFractal, quaternionfp_per_pixel,MandelfpSetup,StandardFractal,
   LTRIGBAILOUT,

   "hypercomplexj", C1, CI, CJ,CK,-.745,0,.113,.05,
   HT_HYPERC, HF_HYPERCJ, WINFRAC+OKJB+TRIG1+MORE,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, HYPERCMPLXFP, NOFRACTAL,  ORIGIN,
   HyperComplexFPFractal, quaternionjulfp_per_pixel,JuliafpSetup,StandardFractal,
   LTRIGBAILOUT,

  "frothybasin", frothattractor, frothshade, ES, ES, 3, 0, 0, 0,
   HT_FROTH, HF_FROTH, NOTRACE+WINFRAC,
   -3, 3, -2.5, 2, 28, NOFRACTAL, NOFRACTAL, FROTHFP, NOSYM,
   NULL, NULL, froth_setup, calcfroth, FROTHBAILOUT,

  "*frothybasin", frothattractor, frothshade, ES, ES, 3, 0, 0, 0,
   HT_FROTH, HF_FROTH, NOTRACE+WINFRAC,
   -3, 3, -2.5, 2, 0, NOFRACTAL, NOFRACTAL, FROTH, NOSYM,
   NULL, NULL, froth_setup, calcfroth, FROTHBAILOUT,

   "*mandel4",realz0, imagz0,ES,ES,0,0,0,0,
   HT_MANDJUL4, HF_MANDEL4, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, JULIA4FP, NOFRACTAL, MANDEL4,  XAXIS_NOPARM,
   Mandel4fpFractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julia4", realparm, imagparm,ES,ES,0.6,0.55,0,0,
   HT_MANDJUL4, HF_JULIA4, WINFRAC+OKJB,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDEL4FP, JULIA4, ORIGIN,
   Mandel4fpFractal, juliafp_per_pixel, JuliafpSetup,StandardFractal,	 STDBAILOUT,

   "*marksmandel", realz0, imagz0, exponent,ES,0,0,1,0,
   HT_MARKS, HF_MARKSMAND, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, MARKSJULIAFP, NOFRACTAL, MARKSMANDEL,  NOSYM,
   MarksLambdafpFractal,marksmandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*marksjulia", realparm, imagparm, exponent,ES,0.1,0.9,0,0,
   HT_MARKS, HF_MARKSJULIA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MARKSMANDELFP, MARKSJULIA,   ORIGIN,
   MarksLambdafpFractal,juliafp_per_pixel,MarksJuliafpSetup,StandardFractal,STDBAILOUT,

   /* dmf */
   "icons", lambda, alpha,beta,gamma2, -2.34, 2.0, 0.2, 0.1,
   HT_ICON, HF_ICON, NOGUESS+NOTRACE+WINFRAC+INFCALC+MORE,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,  NOFRACTAL, NOSYM,
   iconfloatorbit, NULL, orbit3dfloatsetup,  orbit2dfloat, NOBAILOUT,

   /* dmf */
   "icons3d", lambda, alpha,beta,gamma2, -2.34, 2.0, 0.2, 0.1,
   HT_ICON, HF_ICON, NOGUESS+NOTRACE+WINFRAC+INFCALC+PARMS3D+MORE,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL,  NOFRACTAL, NOSYM,
   iconfloatorbit, NULL, orbit3dfloatsetup,  orbit3dfloat, NOBAILOUT,

/*  the following demonstration drunkard's walk fractal requires
    only the demowalk() routine in FRACTALS.C to be functional
    (the definition of the positional value DEMOWALK in FRACTYPE.H
    is not really required, but is good practice).
*/
/*
   "demowalk", "Average Stepsize (% of image)",
               "Color (0 means rotate colors)",ES,ES,5,0.0,0,0,
   -1, -1, NORESUME+WINFRAC,
   -2.5,  1.5,  -1.5,   1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   NULL,  NULL,  StandaloneSetup,  demowalk,  NOBAILOUT,
*/

/*  the following demonstration Mandelbrot/Julia fractals require
    only the definition of the four positional values DEMOMANDEL,
    DEMOJULIA, DEMOMANDELFP, and DEMOJULIAFP in FRACTYPE.H to be
    functional - and we only need *them* for the integer/floatingpoint
    and Mandelbrot/Julia toggle items in their structure items.
*/
/*
   "demomandel", realz0, imagz0,ES,ES,0,0,0,0,
   -1, -1, WINFRAC,
   -2.5, 1.5, -1.5, 1.5, 1, DEMOJULIA,  NOFRACTAL, DEMOMANDELFP, XAXIS_NOPARM,
   JuliaFractal, mandel_per_pixel, MandellongSetup, StandardFractal, STDBAILOUT,

   "demojulia", realparm, imagparm,ES,ES,0.3,0.6,0,0,
   -1, -1, WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, DEMOMANDEL, DEMOJULIAFP, ORIGIN,
   JuliaFractal, julia_per_pixel, JulialongSetup, StandardFractal, STDBAILOUT,

   "*demomandel", realz0, imagz0,ES,ES,0,0,0,0,
   -1, -1, WINFRAC,
   -2.5, 1.5, -1.5, 1.5, 0, DEMOJULIAFP,  NOFRACTAL, DEMOJULIA, XAXIS_NOPARM,
   JuliafpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal, STDBAILOUT,

   "*demojulia", realparm, imagparm,ES,ES,0.3,0.6,0,0,
   -1, -1, WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 0, NOFRACTAL, DEMOMANDELFP, DEMOMANDEL, ORIGIN,
   JuliafpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal, STDBAILOUT,
*/


   NULL, NULL, NULL, NULL, NULL,0,0,0,0,    /* marks the END of the list */
   -1, -1, 0,
   0,  0, 0,  0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL, NULL, NULL, NULL,0

};

int num_fractal_types = (sizeof(fractalspecific)/
	sizeof(struct fractalspecificstuff)) -1;

