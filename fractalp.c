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

/* functions defined elswhere needed for fractalspecific */

extern int JuliaFractal();
extern int mandel_per_pixel();
extern int MandelSetup();
extern int julia_per_pixel();
extern int JuliaSetup();
extern int NewtonFractal2();
extern int otherjulafp_per_pixel();
extern int NewtonSetup();
extern int StandardFractal();
extern int LambdaFractal();
extern int JulialongSetup();
extern int JuliafpFractal();
extern int MandelfpSetup();
extern int juliafp_per_pixel();
extern int JuliafpSetup();
extern int StandaloneSetup();
extern int LambdaTrigfpFractal();
extern int othermandelfp_per_pixel();
extern int MandelTrigSetup();
extern int ManOWarfpFractal();
extern int ManOWarFractal();
extern int MandelLongSetup();
extern int otherjuliafp_per_pixel();
extern int mandelfp_per_pixel();
extern int MandellongSetup();
extern int SierpinskiFractal();
extern int long_julia_per_pixel();
extern int SierpinskiSetup();
extern int Barnsley1Fractal();
extern int long_mandel_per_pixel();
extern int Barnsley2Fractal();
extern int SqrTrigFractal();
extern int SqrTrigSetup();
extern int SqrTrigfpFractal();
extern int TrigPlusTrigFractal();
extern int TrigPlusTriglongSetup();
extern int MarksLambdaFractal();
extern int marksmandel_per_pixel();
extern int MarksJuliaSetup();
extern int UnityFractal();
extern int UnitySetup();
extern int Mandel4Fractal();
extern int Barnsley3Fractal();
extern int TrigZsqrdFractal();
extern int TrigZsqrdfpFractal();
extern int TrigPlusTrigfpFractal();
extern int TrigPlusTrigfpSetup();
extern int TrigXTrigFractal();
extern int FnXFnSetup();
extern int TrigXTrigfpFractal();
extern int Sqr1overTrigFractal();
extern int Sqr1overTrigfpFractal();
extern int ZXTrigPlusZFractal();
extern int ZXTrigPlusZfpFractal();
extern int kamtorusfloatorbit();
extern int orbit2dfloat();
extern int kamtoruslongorbit();
extern int orbit2dlong();
extern int ZXTrigPlusZSetup();
extern int LambdaTrigFractal();
extern int LambdaTrigSetup();
extern int TrigPlusZsquaredFractal();
extern int JuliafnPlusZsqrdSetup();
extern int TrigPlusZsquaredfpFractal();
extern int longZpowerFractal();
extern int floatZpowerFractal();
extern int floatZtozPluszpwrFractal();
extern int LongTrigPlusExponentFractal();
extern int FloatTrigPlusExponentFractal();
extern int PopcornFractal();
extern int LPopcornFractal();
extern int MPCNewtonFractal();
extern int MPCjulia_per_pixel();
extern int ComplexNewton();
extern int ComplexNewtonSetup();
extern int ComplexBasin();
extern int MarksCplxMand();
extern int MarksCplxMandperp();
extern int SierpinskiFPFractal();
extern int SierpinskiFPSetup();
extern int LambdaFPFractal();
extern int Barnsley1FPFractal();
extern int Barnsley2FPFractal();
extern int Barnsley3FPFractal();
extern int jb_per_pixel();
extern int JulibrotSetup();
extern int Std4dFractal();
extern int UnityfpFractal();
extern int StandardSetup();
extern int SpiderfpFractal();
extern int SpiderFractal();
extern int TetratefpFractal();
extern int Magnet1Fractal();
extern int Magnet2Fractal();
extern int Richard8fpFractal();
extern int otherrichard8fp_per_pixel();
extern int Richard8Fractal();
extern int long_richard8_per_pixel();
extern int MarksMandelPwrfpFractal();
extern int marks_mandelpwrfp_per_pixel();
extern int MarksMandelPwrFractal();
extern int marks_mandelpwr_per_pixel();
extern int TimsErrorfpFractal();
extern int TimsErrorFractal();
extern int Lsystem();
extern int CirclefpFractal();
extern int lya_setup();
extern int lyapunov();
extern int hopalong2dfloatorbit();
extern int martin2dfloatorbit();
extern int orbit3dfloat();
extern int orbit3dlong();
extern int lorenz3dlongorbit();
extern int orbit3dlongsetup();
extern int lorenz3dfloatorbit();
extern int lorenz3d1floatorbit();
extern int lorenz3d3floatorbit();
extern int lorenz3d4floatorbit();
extern int orbit3dfloatsetup();
extern int rosslerfloatorbit();
extern int rosslerlongorbit();
extern int henonfloatorbit();
extern int henonlongorbit();
extern int pickoverfloatorbit();
extern int gingerbreadfloatorbit();
extern int diffusion();
extern int plasma();
extern int test();
extern int ifs();
extern int Bifurcation(void);
extern int BifurcVerhulst(void);
extern int LongBifurcVerhulst(void);
extern int BifurcLambda(void);
extern int LongBifurcLambda(void);
extern int BifurcAddSinPi(void);
extern int LongBifurcAddSinPi(void);
extern int BifurcSetSinPi(void);
extern int LongBifurcSetSinPi(void);
extern int BifurcStewart(void);
extern int LongBifurcStewart(void);
extern int popcorn(void);

/* parameter descriptions */
/* for Mandelbrots */
static char realz0[] = "Real Perturbation of Z(0)";
static char imagz0[] = "Imaginary Perturbation of Z(0)";

/* for Julias */
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

/* for Newtons */
static char newtdegree[] = "Polynomial Degree (> 2)";

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
static char pointsperorbit[] = "Points per orbit";

/* Newtbasin */
static char stripes[] = "Enter non-zero value for stripes";

/* Gingerbreadman */
static char initx[] = "Initial x";
static char inity[] = "Initial y";

/* popcorn */
static char step[] = "Step size";

/* bifurcations */
static char filt[] = "Filter Cycles";
static char seed[] = "Seed Population";

/* bailout defines */
#define FTRIGBAILOUT 2500.0
#define LTRIGBAILOUT   64.0
#define STDBAILOUT	    4.0
#define NOBAILOUT	0.0

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

   "mandel",      realz0, imagz0,"","",0,0,0,0,
   HT_MANDEL, HF_MANDEL, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, JULIA,	 NOFRACTAL, MANDELFP, XAXIS_NOPARM,
   JuliaFractal,  mandel_per_pixel,MandelSetup,    calcmand,	    STDBAILOUT,

   "julia",       realparm, imagparm,"","",0.3,0.6,0,0,
   HT_JULIA, HF_JULIA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL, JULIAFP,  ORIGIN,
   JuliaFractal,   julia_per_pixel, JuliaSetup,    calcmand,	    STDBAILOUT,

   "*newtbasin",   newtdegree,"", "","",3,0,0,0,
   HT_NEWTBAS, HF_NEWTBAS, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN,   NOSYM,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "lambda",      realparm, imagparm,"","",0.85,0.6,0,0,
   HT_LAMBDA, HF_LAMBDA, WINFRAC,
   -1.5,  2.5, -1.5,  1.5, 1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP,  NOSYM,
   LambdaFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*mandel",    realz0, imagz0,"","",0,0,0,0,
   HT_MANDEL, HF_MANDEL, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, JULIAFP,   NOFRACTAL, MANDEL,  XAXIS_NOPARM,
   JuliafpFractal,mandelfp_per_pixel, MandelfpSetup, calcmandfp, STDBAILOUT,

   "*newton",      newtdegree,stripes, "","",3,0,0,0,
   HT_NEWT, HF_NEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, MPNEWTON,	XAXIS,
   NewtonFractal2, otherjuliafp_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "*julia",     realparm, imagparm,"","",0.3,0.6,0,0,
   HT_JULIA, HF_JULIA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELFP, JULIA,  ORIGIN,
   JuliafpFractal, juliafp_per_pixel,  JuliafpSetup, calcmandfp,STDBAILOUT,

   "plasma",      "Graininess Factor (.1 to 50, default is 2)",
                  "Algorithm (0 = original, 1 = new)",
                  "Random Seed Value (0 = Random, 1 = Reuse Last)",
                  "",2,0,0,0,
   HT_PLASMA, HF_PLASMA, NOZOOM+NOGUESS+NOTRACE+NORESUME+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	   NULL,   StandaloneSetup,	 plasma,	  NOBAILOUT,

   "*mandelfn",  realz0, imagz0,"","",0,0,0,0,
   HT_MANDFN, HF_MANDFN, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, LAMBDATRIGFP,NOFRACTAL, MANDELTRIG, XYAXIS_NOPARM,
   LambdaTrigfpFractal,othermandelfp_per_pixel,MandelTrigSetup,StandardFractal,FTRIGBAILOUT,

   "*manowar",    realz0, imagz0,"","",0,0,0,0,
   HT_SCOTSKIN, HF_MANOWAR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, MANOWARJFP, NOFRACTAL, MANOWAR,  XAXIS_NOPARM,
   ManOWarfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "manowar",    realz0, imagz0,"","",0,0,0,0,
   HT_SCOTSKIN, HF_MANOWAR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, MANOWARJ, NOFRACTAL, MANOWARFP, XAXIS_NOPARM,
   ManOWarFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "test","(testpt Param #1)","(testpt param #2)","(testpt param #3)","(testpt param #4)",0,0,0,0,
   HT_TEST, HF_TEST, 0,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,	  NULL, 	    StandaloneSetup, test,    STDBAILOUT,

   "sierpinski",  "","","","",0,0,0,0,
   HT_SIER, HF_SIER, WINFRAC,
   -0.9,  1.7, -0.9,  1.7, 1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP,   NOSYM,
   SierpinskiFractal,long_julia_per_pixel, SierpinskiSetup,StandardFractal,127.0,

   "barnsleym1",  realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ1,NOFRACTAL, BARNSLEYM1FP,  XYAXIS_NOPARM,
   Barnsley1Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj1",  realparm, imagparm,"","",0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP,  ORIGIN,
   Barnsley1Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "barnsleym2",  realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ2,NOFRACTAL, BARNSLEYM2FP,  YAXIS_NOPARM,
   Barnsley2Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj2",  realparm, imagparm,"","",0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP,  ORIGIN,
   Barnsley2Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "sqr(fn)", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_SQRFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQRTRIGFP,XYAXIS,
   SqrTrigFractal,   long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(fn)", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_SQRFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, SQRTRIG,XYAXIS,
   SqrTrigfpFractal,   otherjuliafp_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "fn+fn", recoeftrg1, imcoeftrg1,recoeftrg2, imcoeftrg2,1,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIGFP,XAXIS,
   TrigPlusTrigFractal,   long_julia_per_pixel, TrigPlusTriglongSetup,	StandardFractal,LTRIGBAILOUT,

   "mandellambda",realz0, imagz0,"","",0,0,0,0,
   HT_MLAMBDA, HF_MLAMBDA, WINFRAC,
   -3.0,  5.0, -3.0,  3.0, 1, LAMBDA,	 NOFRACTAL, MANDELLAMBDAFP,  XAXIS_NOPARM,
   LambdaFractal,mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksmandel", realz0, imagz0, exponent,"",0,0,1,0,
   HT_MARKS, HF_MARKSMAND, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, MARKSJULIA, NOFRACTAL, NOFRACTAL,  NOSYM,
   MarksLambdaFractal,marksmandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "marksjulia", realparm, imagparm, exponent,"",0.1,0.9,0,0,
   HT_MARKS, HF_MARKSJULIA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MARKSMANDEL, NOFRACTAL,   ORIGIN,
   MarksLambdaFractal,julia_per_pixel,MarksJuliaSetup,StandardFractal,STDBAILOUT,

   "unity", "","","","",0,0,0,0,
   HT_UNITY, HF_UNITY, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, UNITYFP,   XYAXIS,
   UnityFractal, long_julia_per_pixel,UnitySetup,StandardFractal,NOBAILOUT,

   "mandel4", realz0, imagz0,"","",0,0,0,0,
   HT_MANDJUL4, HF_MANDEL4, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, JULIA4,	  NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM,
   Mandel4Fractal,  mandel_per_pixel, MandellongSetup, StandardFractal,  STDBAILOUT,

   "julia4", realparm, imagparm,"","",0.6,0.55,0,0,
   HT_MANDJUL4, HF_JULIA4, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, MANDEL4, NOFRACTAL, ORIGIN,
   Mandel4Fractal,   julia_per_pixel, JulialongSetup,StandardFractal,	 STDBAILOUT,

   "ifs",    "","","","",0,0,0,0,
   HT_IFS, -1, NOGUESS+NOTRACE+NORESUME+WINFRAC,
   -8.0,  8.0, -1.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,  NOSYM,
   NULL,	  NULL,      StandaloneSetup, ifs,    NOBAILOUT,

   "*ifs3d", "","","","",0,0,0,0,
   HT_IFS, -1, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -11.0,  11.0, -11.0, 11.0, 16, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL,	  NULL,      StandaloneSetup, ifs,    NOBAILOUT,

   "barnsleym3",  realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, BARNSLEYJ3,NOFRACTAL, BARNSLEYM3FP,  XAXIS_NOPARM,
   Barnsley3Fractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "barnsleyj3",  realparm, imagparm,"","",0.1,0.36,0,0,
   HT_BARNS, HF_BARNSJ3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP,  NOSYM,
   Barnsley3Fractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "fn(z*z)", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGSQRFP,XYAXIS,
   TrigZsqrdFractal,   julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*fn(z*z)", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGSQR,XYAXIS,
   TrigZsqrdfpFractal,	 juliafp_per_pixel, JuliafpSetup,  StandardFractal,STDBAILOUT,

   "*bifurcation",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFURCATION, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   1.9,  3.0, 0,  1.34, 0, NOFRACTAL, NOFRACTAL, LBIFURCATION, NOSYM,
   BifurcVerhulst, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*fn+fn",recoeftrg1,imcoeftrg1,recoeftrg2,imcoeftrg2,1,0,1,0,
   HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIG,XAXIS,
   TrigPlusTrigfpFractal, otherjuliafp_per_pixel, TrigPlusTrigfpSetup,	StandardFractal,LTRIGBAILOUT,

   "fn*fn", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, TRIGXTRIGFP,PI,
   TrigXTrigFractal, long_julia_per_pixel, FnXFnSetup,	StandardFractal,LTRIGBAILOUT,

   "*fn*fn", "","","","",0,0,0,0,
   HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, NOFRACTAL, TRIGXTRIG,PI,
   TrigXTrigfpFractal, otherjuliafp_per_pixel, FnXFnSetup,  StandardFractal,LTRIGBAILOUT,

   "sqr(1/fn)","","","","",0,0,0,0,
   HT_SCOTSKIN, HF_SQROVFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, NOFRACTAL, SQR1OVERTRIGFP,NOSYM,
   Sqr1overTrigFractal, long_julia_per_pixel, SqrTrigSetup,  StandardFractal,LTRIGBAILOUT,

   "*sqr(1/fn)","","","","",0,0,0,0,
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

   "lambdafn",     realparm, imagparm,"","",1.0,0.4,0,0,
   HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, MANDELTRIG, LAMBDATRIGFP,PI_SYM,
   LambdaTrigFractal,long_julia_per_pixel, LambdaTrigSetup,	StandardFractal,LTRIGBAILOUT,

   "manfn+zsqrd",  realz0, imagz0,"","",0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 16, LJULTRIGPLUSZSQRD,  NOFRACTAL, FPMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredFractal,mandel_per_pixel,MandellongSetup,StandardFractal, STDBAILOUT,

   "julfn+zsqrd",  realparm, imagparm,"","",-0.5,0.5,0,0,
   HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 16, NOFRACTAL, LMANTRIGPLUSZSQRD, FPJULTRIGPLUSZSQRD,	NOSYM,
   TrigPlusZsquaredFractal,julia_per_pixel, JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*manfn+zsqrd", realz0, imagz0,"","",0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULTRIGPLUSZSQRD,   NOFRACTAL, LMANTRIGPLUSZSQRD, XAXIS_NOPARM,
   TrigPlusZsquaredfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal, STDBAILOUT,

   "*julfn+zsqrd", realparm, imagparm,"","",-0.5,0.5,0,0,
   HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANTRIGPLUSZSQRD, LJULTRIGPLUSZSQRD, NOSYM,
   TrigPlusZsquaredfpFractal, juliafp_per_pixel,  JuliafnPlusZsqrdSetup,StandardFractal, STDBAILOUT,

   "*lambdafn",  realparm, imagparm,"","",1.0,0.4,0,0,
   HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELTRIGFP, LAMBDATRIG, PI_SYM,
   LambdaTrigfpFractal,otherjuliafp_per_pixel,LambdaTrigSetup,StandardFractal,FTRIGBAILOUT,

   "mandelfn",realz0, imagz0,"","", 0,0,0,0,
   HT_MANDFN, HF_MANDFN, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 16, LAMBDATRIG, NOFRACTAL, MANDELTRIGFP, XYAXIS_NOPARM,
   LambdaTrigFractal,long_mandel_per_pixel,MandelTrigSetup,StandardFractal,LTRIGBAILOUT,

   "manzpower", realz0, imagz0, exponent,imexponent,0,0,2,0,
   HT_PICKMJ, HF_MANZPOWER, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER,	XAXIS,
   longZpowerFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julzpower", realparm, imagparm, exponent,imexponent,0.3,0.6,2,0,
   HT_PICKMJ, HF_JULZPOWER, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER,	 ORIGIN,
   longZpowerFractal,long_julia_per_pixel,JulialongSetup,StandardFractal,STDBAILOUT,

   "*manzpower", realz0, imagz0, exponent,imexponent,0,0,2,0,
   HT_PICKMJ, HF_MANZPOWER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULIAZPOWER,   NOFRACTAL, LMANDELZPOWER,  XAXIS_NOPARM,
   floatZpowerFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julzpower", realparm, imagparm, exponent,imexponent,0.3,0.6,2,0,
   HT_PICKMJ, HF_JULZPOWER, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER,	ORIGIN,
   floatZpowerFractal, otherjuliafp_per_pixel,	JuliafpSetup,StandardFractal,STDBAILOUT,

   "manzzpwr", realz0, imagz0, exponent,"",0,0,2,0,
   HT_PICKMJ, HF_MANZZPWR, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, FPJULZTOZPLUSZPWR,   NOFRACTAL, NOFRACTAL,  XAXIS_NOPARM,
   floatZtozPluszpwrFractal,othermandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "julzzpwr", realparm, imagparm, exponent,"",-0.3,0.3,2,0,
   HT_PICKMJ, HF_JULZZPWR, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL,	NOSYM,
   floatZtozPluszpwrFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "manfn+exp",realz0, imagz0,"","",0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 16, LJULTRIGPLUSEXP,    NOFRACTAL,  FPMANTRIGPLUSEXP, XAXIS_NOPARM,
   LongTrigPlusExponentFractal,long_mandel_per_pixel,MandellongSetup,StandardFractal,STDBAILOUT,

   "julfn+exp", realparm, imagparm,"","",0,0,0,0,
   HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 16, NOFRACTAL, LMANTRIGPLUSEXP,FPJULTRIGPLUSEXP, NOSYM,
   LongTrigPlusExponentFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "*manfn+exp", realz0, imagz0,"","",0,0,0,0,
   HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, FPJULTRIGPLUSEXP, NOFRACTAL, LMANTRIGPLUSEXP,   XAXIS_NOPARM,
   FloatTrigPlusExponentFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*julfn+exp", realparm, imagparm,"","",0,0,0,0,
   HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+WINFRAC,
   -4.0,  4.0, -3.0,  3.0, 0, NOFRACTAL, FPMANTRIGPLUSEXP, LJULTRIGPLUSEXP,   NOSYM,
   FloatTrigPlusExponentFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*popcorn", step, "", "","",0.05,0,0,0,
   HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORN,  NOPLOT,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,  popcorn,STDBAILOUT,

   "popcorn", step, "", "","",0.05,0,0,0,
   HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORN,  NOPLOT,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,popcorn,STDBAILOUT,

   "*lorenz",timestep,"a","b", "c",.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -15,  15, 0, 30, 0, NOFRACTAL, NOFRACTAL, LLORENZ,   NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "lorenz",timestep,"a","b", "c",.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -15,  15, 0, 30, 16, NOFRACTAL, NOFRACTAL, FPLORENZ,   NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "lorenz3d",timestep,"a","b", "c",.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 16, NOFRACTAL, NOFRACTAL, FPLORENZ3D,   NOSYM,
   lorenz3dlongorbit, NULL,	    orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "newton",    newtdegree,"", "","",3,0,0,0,
   HT_NEWT, HF_NEWT, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NEWTON,   XAXIS,
   MPCNewtonFractal, MPCjulia_per_pixel,  NewtonSetup, StandardFractal,NOBAILOUT,

   "newtbasin", newtdegree,stripes, "","",0,0,0,0,
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

   "*sierpinski",  "","","","",0,0,0,0,
   HT_SIER, HF_SIER, WINFRAC,
   -0.9,  1.7, -0.9,  1.7, 0, NOFRACTAL, NOFRACTAL, SIERPINSKI,   NOSYM,
   SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,StandardFractal,127.0,

   "*lambda", realparm, imagparm,"","",0.85,0.6,0,0,
   HT_LAMBDA, HF_LAMBDA, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA,  NOSYM,
   LambdaFPFractal,   juliafp_per_pixel, JuliafpSetup,	StandardFractal,STDBAILOUT,

   "*barnsleym1", realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ1FP,NOFRACTAL, BARNSLEYM1,  XYAXIS_NOPARM,
   Barnsley1FPFractal, othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj1", realparm, imagparm,"","",0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ1, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1,  ORIGIN,
   Barnsley1FPFractal, otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym2", realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ2FP,NOFRACTAL, BARNSLEYM2,  YAXIS_NOPARM,
   Barnsley2FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj2", realparm, imagparm,"","",0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ2, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2,  ORIGIN,
   Barnsley2FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*barnsleym3", realz0, imagz0,"","",0,0,0,0,
   HT_BARNS, HF_BARNSM3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3,  XAXIS_NOPARM,
   Barnsley3FPFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "*barnsleyj3", realparm, imagparm,"","",0.6,1.1,0,0,
   HT_BARNS, HF_BARNSJ3, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3,  XAXIS,
   Barnsley3FPFractal,otherjuliafp_per_pixel,JuliafpSetup,StandardFractal,STDBAILOUT,

   "*mandellambda",realz0, imagz0,"","",0,0,0,0,
   HT_MLAMBDA, HF_MLAMBDA, WINFRAC,
   -3.0,  5.0, -3.0,  3.0, 0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA,  XAXIS_NOPARM,
   LambdaFPFractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "julibrot", "","","","",-.83,-.83,.25,-.25,
   HT_JULIBROT, -1, NOGUESS+NOTRACE+NOROTATE+NORESUME+WINFRAC,
   -2.0, 2.0, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal, NOBAILOUT,

   "*lorenz3d",timestep,"a","b","c",.02,5,15,1,
   HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, LLORENZ3D,   NOSYM,
   lorenz3dfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "rossler3d",timestep,"a","b","c",.04,.2,.2,5.7,
   HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -20.0,   40.0, 16, NOFRACTAL, NOFRACTAL, FPROSSLER,	NOSYM,
   rosslerlongorbit, NULL,	   orbit3dlongsetup, orbit3dlong,    NOBAILOUT,

   "*rossler3d",timestep,"a","b","c",.04,.2,.2,5.7,
   HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -20.0,   40.0, 0, NOFRACTAL, NOFRACTAL, LROSSLER,   NOSYM,
   rosslerfloatorbit, NULL,	    orbit3dfloatsetup, orbit3dfloat,	NOBAILOUT,

   "henon","a","b","","",1.4,.3,0,0,
   HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -1.4,  1.4,	-.5,   .5, 16, NOFRACTAL, NOFRACTAL, FPHENON,	NOSYM,
   henonlongorbit, NULL,	 orbit3dlongsetup, orbit2dlong,    NOBAILOUT,

   "*henon","a","b","","",1.4,.3,0,0,
   HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -1.4,  1.4,	-.5,   .5, 0, NOFRACTAL, NOFRACTAL, LHENON,   NOSYM,
   henonfloatorbit, NULL,	  orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "pickover","a","b","c","d",2.24,.43,-.65,-2.43,
   HT_PICK, HF_PICKOVER, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -2.8,  2.8,	-2.0, 2.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   pickoverfloatorbit, NULL,	     orbit3dfloatsetup, orbit3dfloat,	 NOBAILOUT,

   "gingerbreadman",initx,inity,"","",-.1,0,0,0,
   HT_GINGER, HF_GINGER, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -4.5,  8.5,	-4.5, 8.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   gingerbreadfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,    NOBAILOUT,

   "diffusion", "Border size","","", "",10,0,0,0,
   HT_DIFFUS, HF_DIFFUS, NOZOOM+NOGUESS+NOTRACE+WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	 NOSYM,
   NULL,   NULL,     StandaloneSetup, diffusion,    NOBAILOUT,

   "*unity", "","","","",0,0,0,0,
   HT_UNITY, HF_UNITY, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, UNITY,   XYAXIS,
   UnityfpFractal, otherjuliafp_per_pixel,StandardSetup,StandardFractal,NOBAILOUT,

   "*spider", realz0, imagz0,"","",0,0,0,0,
   HT_SCOTSKIN, HF_SPIDER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, SPIDER,  XAXIS_NOPARM,
   SpiderfpFractal,mandelfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "spider",  realz0, imagz0,"","",0,0,0,0,
   HT_SCOTSKIN, HF_SPIDER, WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL, NOFRACTAL, SPIDERFP,  XAXIS_NOPARM,
   SpiderFractal,mandel_per_pixel, MandellongSetup,StandardFractal,STDBAILOUT,

   "tetrate", realz0, imagz0,"","",0,0,0,0,
   HT_SCOTSKIN, HF_TETRATE, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	XAXIS,
   TetratefpFractal,othermandelfp_per_pixel,MandelfpSetup,StandardFractal,STDBAILOUT,

   "magnet1m", realz0, imagz0,"","",0,0,0,0,
   HT_MAGNET, HF_MAGM1, WINFRAC,
   -4.0, 4.0, -3.0, 3.0, 0, MAGNET1J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet1Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet1j", realparm, imagparm,"","",0,0,0,0,
   HT_MAGNET, HF_MAGJ1, WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET1M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet1Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "magnet2m", realz0, imagz0,"","",0,0,0,0,
   HT_MAGNET, HF_MAGM2, WINFRAC,
   -1.5,3.7, -1.95,1.95,   0, MAGNET2J,NOFRACTAL,NOFRACTAL, XAXIS_NOPARM,
   Magnet2Fractal,mandelfp_per_pixel,MandelfpSetup,StandardFractal,100.0,

   "magnet2j", realparm, imagparm,"","",0,0,0,0,
   HT_MAGNET, HF_MAGJ2, WINFRAC,
   -8.0,  8.0, -6.0,  6.0, 0, NOFRACTAL,MAGNET2M,NOFRACTAL, XAXIS_NOIMAG,
   Magnet2Fractal,juliafp_per_pixel,JuliafpSetup,StandardFractal,100.0,

   "bifurcation",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFURCATION, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   1.9,  3.0, 0,  1.34, 1, NOFRACTAL, NOFRACTAL, BIFURCATION, NOSYM,
   LongBifurcVerhulst, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "biflambda",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFLAMBDA, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.0, 4.0, -1.0, 2.0, 1, NOFRACTAL, NOFRACTAL, BIFLAMBDA,   NOSYM,
   LongBifurcLambda, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*biflambda",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFLAMBDA, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.0, 4.0, -1.0, 2.0, 0, NOFRACTAL, NOFRACTAL, LBIFLAMBDA,  NOSYM,
   BifurcLambda, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bif+sinpi",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFPLUSSINPI, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.275,1.45, 0.0, 2.0, 0, NOFRACTAL, NOFRACTAL, LBIFADSINPI,	 NOSYM,
   BifurcAddSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bif=sinpi",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFEQSINPI, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.5, 2.5, -3.5, 3.5, 0, NOFRACTAL, NOFRACTAL, LBIFEQSINPI,	 NOSYM,
   BifurcSetSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*popcornjul", step, "", "","",0.05,0,0,0,
   HT_POPCORN, HF_POPCJUL, WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 0, NOFRACTAL, NOFRACTAL, LPOPCORNJUL,  ORIGIN,
   PopcornFractal, otherjuliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "popcornjul", step, "", "","",0.05,0,0,0,
   HT_POPCORN, HF_POPCJUL, WINFRAC,
   -3.0,  3.0, -2.2,  2.2, 16, NOFRACTAL, NOFRACTAL, FPPOPCORNJUL,  ORIGIN,
   LPopcornFractal,   long_julia_per_pixel, JulialongSetup,  StandardFractal,STDBAILOUT,

   "lsystem", "Order","","","",2,0,0,0,
   HT_LSYS, -3, NOZOOM+NORESUME+NOGUESS+NOTRACE+WINFRAC,
   -1, 1, -1, 1, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   NULL, NULL, StandaloneSetup, Lsystem, NOBAILOUT,

   "*manowarj", realparm, imagparm,"","",0,0,0,0,
   HT_SCOTSKIN, HF_MANOWARJ, WINFRAC,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, MANOWARFP, MANOWARJ,   NOSYM,
   ManOWarfpFractal,juliafp_per_pixel, JuliafpSetup,StandardFractal,STDBAILOUT,

   "manowarj",  realparm, imagparm,"","",0,0,0,0,
   HT_SCOTSKIN, HF_MANOWARJ, WINFRAC,
   -2.5,  1.5, -1.5, 1.5, 1, NOFRACTAL, MANOWAR,   MANOWARJFP, NOSYM,
   ManOWarFractal,julia_per_pixel, JulialongSetup,StandardFractal,STDBAILOUT,

   "*fn(z)+fn(pix)", realz0,imagz0,recoeftrg2,imcoeftrg2,0,0,1,0,
   -1, HF_FNPLUSFNPIX, TRIG2,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXLONG, NOSYM,
   Richard8fpFractal,otherrichard8fp_per_pixel, MandelfpSetup,StandardFractal,LTRIGBAILOUT,

   "fn(z)+fn(pix)",  realz0,imagz0,recoeftrg2,imcoeftrg2,0,0,1,0,
   -1, HF_FNPLUSFNPIX, TRIG2,
   -2.5,  1.5, -1.5, 1.5, 1, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXFP, NOSYM,
   Richard8Fractal,long_richard8_per_pixel, MandellongSetup,StandardFractal,LTRIGBAILOUT,

   "*marksmandelpwr", realz0, imagz0,"","",0,0,0,0,
   -1, HF_MARKSMANDPWR, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5, 1.5, 0, NOFRACTAL, NOFRACTAL, MARKSMANDELPWR, XAXIS_NOPARM,
   MarksMandelPwrfpFractal,marks_mandelpwrfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "marksmandelpwr",  realz0, imagz0,"","",0,0,0,0,
   -1, HF_MARKSMANDPWR, TRIG1+WINFRAC,
   -2.5,  1.5, -1.5,  1.5, 1, NOFRACTAL,	 NOFRACTAL, MARKSMANDELPWRFP,  XAXIS_NOPARM,
   MarksMandelPwrFractal,marks_mandelpwr_per_pixel, MandelSetup,StandardFractal,STDBAILOUT,

   "*tim's_error",    realz0, imagz0,"","",0,0,0,0,
   -1, HF_TIMSERR, WINFRAC+TRIG1,
   -2.5,  3.0, -2.0,  2.0, 0, NOFRACTAL,	 NOFRACTAL, TIMSERROR,	XAXIS_NOPARM,
   TimsErrorfpFractal,marks_mandelpwrfp_per_pixel, MandelfpSetup,StandardFractal,STDBAILOUT,

   "tim's_error",    realz0, imagz0,"","",0,0,0,0,
   -1, HF_TIMSERR, WINFRAC+TRIG1,
   -2.5,  3.0, -2.0,  2.0, 1, NOFRACTAL,	 NOFRACTAL, TIMSERRORFP,  XAXIS_NOPARM,
   TimsErrorFractal,marks_mandelpwr_per_pixel, MandelSetup,StandardFractal,STDBAILOUT,

   "bif=sinpi",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFEQSINPI, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   -2.5, 2.5, -3.5, 3.5, 16, NOFRACTAL, NOFRACTAL, BIFEQSINPI,	 NOSYM,
   LongBifurcSetSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bif+sinpi",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFPLUSSINPI, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.275,1.45, 0.0, 2.0, 16, NOFRACTAL, NOFRACTAL, BIFADSINPI,	 NOSYM,
   LongBifurcAddSinPi, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "*bifstewart",filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFSTEWART, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.7,2.0, -1.1,1.1, 0, NOFRACTAL, NOFRACTAL, LBIFSTEWART, NOSYM,
   BifurcStewart, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "bifstewart", filt,seed,"","",1000.0,0.66,0,0,
   HT_BIF, HF_BIFSTEWART, NOGUESS+NOTRACE+NOROTATE+WINFRAC,
   0.7,2.0, -1.1,1.1, 16, NOFRACTAL, NOFRACTAL, BIFSTEWART, NOSYM,
   LongBifurcStewart, NULL, StandaloneSetup, Bifurcation, NOBAILOUT,

   "hopalong","a","b","c","",.4,1,0,0,
   HT_MARTIN, HF_HOPALONG, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -2.0, 3.0, -1.625, 2.625, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	NOSYM,
   hopalong2dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

   "circle", "magnification","","","",200000,0,0,0,
   HT_CIRCLE, HF_CIRCLE, WINFRAC,
   -2.0,  2.0, -1.5,  1.5, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,  XYAXIS,
   CirclefpFractal, juliafp_per_pixel,  JuliafpSetup,StandardFractal,STDBAILOUT,

   "martin","a","","","",3.14,0,0,0,
   HT_MARTIN, HF_MARTIN, NOGUESS+NOTRACE+INFCALC+WINFRAC,
   -32,  32, -32, 32, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,	NOSYM,
   martin2dfloatorbit, NULL,	     orbit3dfloatsetup, orbit2dfloat,	 NOBAILOUT,

  "lyapunov", "Order (integer)", "Population Seed", "Filter Cycles", "", 0, 0.5, 0, 0,
   -1, -1, NOGUESS+NOTRACE+WINFRAC,
   2.0, 4.0, 2.5, 4.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   BifurcLambda, NULL, lya_setup, lyapunov, NOBAILOUT,

   "lorenz3d1",timestep,"a","b","c",.02,5,15,1,
   HT_LORENZ, HF_LORENZ3D1, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d1floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "lorenz3d3",timestep,"a","b","c",.02,10,28,2.66,
   HT_LORENZ, HF_LORENZ3D3, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d3floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   "lorenz3d4",timestep,"a","b","c",.02,10,28,2.66,
   HT_LORENZ, HF_LORENZ3D4, NOGUESS+NOTRACE+NORESUME+WINFRAC+PARMS3D,
   -30.0,  30.0,  -30.0,   30.0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
   lorenz3d4floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat, NOBAILOUT,

   NULL, NULL, NULL, NULL, NULL,0,0,0,0,    /* marks the END of the list */
   -1, -1, 0,
   0,  0, 0,  0, 0, NOFRACTAL, NOFRACTAL, NOFRACTAL,   NOSYM,
   NULL, NULL, NULL, NULL,0

};
