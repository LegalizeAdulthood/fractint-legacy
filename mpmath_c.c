/* MPMath_c.c (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
     All rights reserved.

   Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     128 Hamden Ave., F
     Waterbury, CT 06704
     (203) 754-1162
*/

#include "mpmath.h"

int MPaccuracy = 32;

struct MP MPsub(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd(x, y));
}

/* added by TW */
struct MP MPsub086(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd086(x, y));
}

/* added by TW */
struct MP MPsub386(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd386(x, y));
}

struct MP MPabs(struct MP x) {
   x.Exp &= 0x7fff;
   return(x);
}

struct MPC MPCsqr(struct MPC x) {
   struct MPC z;

	z.x = pMPsub(pMPmul(x.x, x.x), pMPmul(x.y, x.y));
	z.y = pMPmul(x.x, x.y);
	z.y.Exp++;
   return(z);
}

struct MP MPCmod(struct MPC x) {
	return(pMPadd(pMPmul(x.x, x.x), pMPmul(x.y, x.y)));
}

struct MPC MPCmul(struct MPC x, struct MPC y) {
   struct MPC z, t;

	z.x = pMPsub(pMPmul(x.x, y.x), pMPmul(x.y, y.y));
	z.y = pMPadd(pMPmul(x.x, y.y), pMPmul(x.y, y.x));
   return(z);
}

struct MPC MPCdiv(struct MPC x, struct MPC y) {
   struct MP mod;

   mod = MPCmod(y);
	y.y.Exp ^= 0x8000;
	y.x = pMPdiv(y.x, mod);
	y.y = pMPdiv(y.y, mod);
   return(MPCmul(x, y));
}

struct MPC MPCadd(struct MPC x, struct MPC y) {
   struct MPC z;

	z.x = pMPadd(x.x, y.x);
	z.y = pMPadd(x.y, y.y);
   return(z);
}

struct MPC MPCsub(struct MPC x, struct MPC y) {
   struct MPC z;

	z.x = pMPsub(x.x, y.x);
	z.y = pMPsub(x.y, y.y);
   return(z);
}

struct MPC MPCone = { 0x3fff, 0x80000000l, 0, 0l };

struct MPC MPCpow(struct MPC x, int exp) {
   struct MPC z;
   struct MPC zz;

   if(exp & 1)
      z = x;
   else
      z = MPCone;
   exp >>= 1;
   while(exp) {
		zz.x = pMPsub(MPmul(x.x, x.x), pMPmul(x.y, x.y));
		zz.y = pMPmul(x.x, x.y);
		zz.y.Exp++;
      x = zz;
      if(exp & 1) {
			zz.x = pMPsub(pMPmul(z.x, x.x), pMPmul(z.y, x.y));
			zz.y = pMPadd(pMPmul(z.x, x.y), pMPmul(z.y, x.x));
         z = zz;
      }
      exp >>= 1;
   }
   return(z);
}

int MPCcmp(struct MPC x, struct MPC y) {
   struct MPC z;

	if(pMPcmp(x.x, y.x) || pMPcmp(x.y, y.y)) {
		z.x = MPCmod(x);
		z.y = MPCmod(y);
		return(pMPcmp(z.x, z.y));
   }
   else
      return(0);
}

struct complex MPC2cmplx(struct MPC x) {
   struct complex z;

	z.x = *pMP2d(x.x);
	z.y = *pMP2d(x.y);
   return(z);
}

struct MPC cmplx2MPC(struct complex z) {
   struct MPC x;

	x.x = pd2MP(z.x);
	x.y = pd2MP(z.y);
   return(x);
}

/* function pointer versions added by Tim Wegner 12/07/89 */
int        (*ppMPcmp)() = MPcmp086;
int        (*pMPcmp)(struct MP x, struct MP y) = MPcmp086;
struct MP  (*pMPmul)(struct MP x, struct MP y) = MPmul086;
struct MP  (*pMPdiv)(struct MP x, struct MP y) = MPdiv086;
struct MP  (*pMPadd)(struct MP x, struct MP y) = MPadd086;
struct MP  (*pMPsub)(struct MP x, struct MP y) = MPsub086;
struct MP  (*pd2MP)(double x)                  = d2MP086 ;
double *   (*pMP2d)(struct MP m)               = MP2d086 ;
struct MP  (*pfg2MP)(long x, int fg)           = fg2MP086;

void setMPfunctions(void) {
   if(cpu == 386)
   {
      pMPmul = MPmul386;
      pMPdiv = MPdiv386;
      pMPadd = MPadd386;
      pMPsub = MPsub386;
      pMPcmp = MPcmp386;
      pd2MP  = d2MP386 ;
      pMP2d  = MP2d386 ;
      pfg2MP = fg2MP386;
   }
   else
   {
      pMPmul = MPmul086;
      pMPdiv = MPdiv086;
      pMPadd = MPadd086;
      pMPsub = MPsub086;
      pMPcmp = MPcmp086;
      pd2MP  = d2MP086 ;
      pMP2d  = MP2d086 ;
      pfg2MP = fg2MP086;
   }
}

#define sqr(x) ((x) * (x))

extern int debugflag, fpu;

struct complex ComplexPower(struct complex x, struct complex y) {
   struct complex z, cLog, t;
   double dist, e2x, siny, cosy;

   FPUcplxlog(&x, &cLog);
   FPUcplxmul(&cLog, &y, &t);

   if(fpu == 387)
      FPUcplxexp387(&t, &z);
   else {
      e2x = exp(t.x);
      FPUsincos(&t.y, &siny, &cosy);
      z.x = e2x * cosy;
      z.y = e2x * siny;
   }
   return(z);
}


/***** FRACTINT specific routines and variables *****/

#ifndef TESTING_MATH

#include "fractint.h"

extern double param[];
extern struct complex old, new, init;
extern double threshold, roverd, d1overd, dx0[], dy0[];
extern int periodicitycheck, row, col, debugflag;


#include <float.h>
#include <stdlib.h>

extern void (*plot)(int x, int y, int Color);
extern int  xdots, ydots;     /* coordinates of dots on the screen  */
extern int  colors;           /* maximum colors available */
extern int  maxit;

char far *LogTable = (char far *)0;
extern int LogFlag;
float fColors;

void ChkLogFlag(void) {
   float l, f, c = (float)256.0, m;
   unsigned n, x;

   if(!LogFlag) {
      if(LogTable) {
         farmemfree(LogTable);
         LogTable = (char far *)0;
      }
   }
   else {
      if(!LogTable) {
         if(LogTable = farmemalloc((long)maxit + 1)) {
            Fg2Float((long)maxit, 0, m);
            fLog14(m, m);
            fDiv(c, m, c);
            for(n = 1; n < maxit; n++) {
               Fg2Float((long)n, 0, f);
               fLog14(f, l);
               fMul16(l, c, l);
               LogTable[n] = Float2Fg(l, 0) + 1;
            }
            LogTable[0] = 0;
         }
         else
            buzzer(2);
      }
   }
}

long far ExpFloat14(long x) {
   static float fLogTwo = (float)0.6931472;
   int f;
   long Ans;

   f = 23 - (int)RegFloat2Fg(RegDivFloat(x, *(long*)&fLogTwo), 0);
   Ans = ExpFudged(RegFloat2Fg(x, 16), f);
   return(RegFg2Float(Ans, (char)f));
}

extern struct complex tmp;
extern int color, colors;
double TwoPi;
struct complex temp, t2, BaseLog;
struct complex cdegree = { 3.0, 0.0 },
               croot   = { 1.0, 0.0 };

int ComplexNewtonSetup(void) {
   threshold = .001;
   periodicitycheck = 0;
   if(param[0] != 0.0 || param[1] != 0.0 || param[2] != 0.0 || 
      param[3] != 0.0) {
      croot.x = param[2];
      croot.y = param[3];
      cdegree.x = param[0];
      cdegree.y = param[1];
      FPUcplxlog(&croot, &BaseLog);
      TwoPi = asin(1.0) * 4;
   }
   return(1);
}

int ComplexNewton(void) {
   struct complex cd1;
   double dist, mod, Angle, e2x;

   /* new = ((cdegree-1) * old**cdegree) + croot
            ----------------------------------
                 cdegree * old**(cdegree-1)         */

   cd1.x = cdegree.x - 1.0;
   cd1.y = cdegree.y;

   temp = ComplexPower(old, cd1);
   FPUcplxmul(&temp, &old, &new);

   tmp.x = new.x - croot.x;
   tmp.y = new.y - croot.y;
   if((sqr(tmp.x) + sqr(tmp.y)) < threshold)
      return(1);

   FPUcplxmul(&new, &cd1, &tmp);
   tmp.x += croot.x;
   tmp.y += croot.y;

   FPUcplxmul(&temp, &cdegree, &t2);
   FPUcplxdiv(&tmp, &t2, &old);
   return(0);
}

int ComplexBasin(void) {
   struct complex cd1;
   double dist, mod, Angle, e2x;

   /* new = ((cdegree-1) * old**cdegree) + croot
            ----------------------------------
                 cdegree * old**(cdegree-1)         */

   cd1.x = cdegree.x - 1.0;
   cd1.y = cdegree.y;

   temp = ComplexPower(old, cd1);
   FPUcplxmul(&temp, &old, &new);

   tmp.x = new.x - croot.x;
   tmp.y = new.y - croot.y;
   if((sqr(tmp.x) + sqr(tmp.y)) < threshold) {
      if(fabs(old.y) < .01)
         old.y = 0.0;
      FPUcplxlog(&old, &temp);
      FPUcplxmul(&temp, &cdegree, &tmp);
      mod = tmp.y/TwoPi;
      color = (int)mod;
      if(fabs(mod - color) > 0.5) {
         if(mod < 0.0)
            color--;
         else
            color++;
      }
      color += 2;
      if(color < 0)
         color += 128;
      return(1);
   }

   FPUcplxmul(&new, &cd1, &tmp);
   tmp.x += croot.x;
   tmp.y += croot.y;

   FPUcplxmul(&temp, &cdegree, &t2);
   FPUcplxdiv(&tmp, &t2, &old);
   return(0);
}

void fullscreen_prompt(int startrow, int numprompts, char *prompts[], 
   double values[]);
int get_fracttype(int t);

static int Distribution = 30, Offset = 0, Slope = 25, f;
static long con;

int Starfield(void) {
   int c;

   plasma();
   Distribution = (int)param[1];
   con = (long)(param[2] / 100 * (1L << 16));
   Slope = (int)param[3];
   for(row = 0; row < ydots; row++) {
      for(col = 0; col < xdots; col++) {
         if(check_key())
            return(-1);
         c = getcolor(col, row);
         putcolor(col, row, GausianNumber(c, colors));
      }
   }
   return(0);
}

#endif

