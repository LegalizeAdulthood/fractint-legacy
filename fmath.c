/* FMath.c (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
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

#include "fmath.h"
#include <stdlib.h>

#define d2Fudge(x) (long)(x * (1 << 13))
#define Fudge2d(x) ((double)x / (1 << 13))
#define Fudge (1 << 13)

unsigned int Fudged13Pi = d2Fudge(3.14159), Fudged13One = d2Fudge(1.0);

long far RegSubFloat(long x, long y) {
   if(y) {
      y ^= 0x80000000L;
      return(RegAddFloat(x, y));
   }
   else
      return(x);
}

int far sin13(long x) {
	x = ((x + Fudged13Pi) % (Fudged13Pi << 1));
	if(x > 0)
		x -= Fudged13Pi;
	else
		x += Fudged13Pi;
   return(FastSine((int)x));
}

int far cos13(long x) {
	x = ((x + Fudged13Pi) % (Fudged13Pi << 1));
	if(x > 0)
		x -= Fudged13Pi;
	else
		x += Fudged13Pi;
	return(FastCosine((int)x));
}

long far cosh13(long x) {
	if(labs(x) > (long)Fudged13Pi) {
      x = cosh13(x / 2) >> 6;
      return((x * x) - Fudged13One);
   }
   else
      return(FastHypCosine((int)x));
}

long far sinh13(long x) {
   long HypSine, HypCosine;

   if(labs(x) > (long)Fudged13Pi) {
      HypSine = sinh13(x / 2) / (1 << 6);
      HypCosine = cosh13(x / 2) >> 6;
      return(HypSine * HypCosine);
   }
   else
      return(FastHypSine((int)x));
}

long far ExpFloat14(long x) {
	static float fLogTwo = (float)0.6931472;
	int f;
   long Ans;

	f = 23 - (int)RegFloat2Fg(RegDivFloat(x, *(long*)&fLogTwo), 0);
	Ans = ExpFudged(RegFloat2Fg(x, 16), f);
	return(RegFg2Float(Ans, (char)f));
}

void fSqrZ(struct fComplex *x, struct fComplex *z) {
   struct fComplex t;

   fSqr(x->x, t.x);
   fSqr(x->y, t.y);
   fAdd(t.x, t.y, z->mod);
   fSub(t.x, t.y, z->x);
   fMul(x->x, x->y, z->y);
   fShift(z->y, 1, z->y);
}

void fMod(struct fComplex *x) {
   float m;

	fSqr(x->x, x->mod);
   fSqr(x->y, m);
   fAdd(m, x->mod, x->mod);
}

void fInvZ(struct fComplex *x, struct fComplex *z) {
   float m;

   fDiv(x->x, x->mod, z->x);
   fDiv(x->y, x->mod, z->y);
   ((char*)&(z->y))[3] ^= 0x80;
	fMod(z);
}

void fMulZ(struct fComplex *x, struct fComplex *y, struct fComplex *z) {
   struct fComplex t;

   fMul(x->x, y->x, t.x);
   fMul(x->y, y->y, t.y);
   fAdd(t.x, t.y, z->mod);
   fSub(t.x, t.y, t.x);
   fMul(x->x, y->y, t.y);
   fMul(x->y, y->x, z->y);
   fAdd(t.y, z->y, z->y);
   *(long*)&z->x = *(long*)&t.x;
}

void fDivZ(struct fComplex *x, struct fComplex *y, struct fComplex *z) {
   fInvZ(y, y);
   fMulZ(x, y, z);
}

void fSinZ(struct fComplex *x, struct fComplex *z) {
   struct fComplex tx, ty;

	fSin12(x->x, tx.x);
   fCosh12(x->y, tx.y);
   fCos12(x->x, ty.x);
   fSinh12(x->y, ty.y);
   fMul(tx.x, tx.y, z->x);
   fMul(ty.x, ty.y, z->y);
}

void fCosZ(struct fComplex *x, struct fComplex *z) {
   struct fComplex tx, ty;

   fCos12(x->x, tx.x);
   fCosh12(x->y, tx.y);
   fSin12(x->x, ty.y);
   fSinh12(x->y, ty.y);
   fMul(tx.x, tx.y, z->x);
   fMul(ty.x, ty.y, z->y);
   ((char*)&(z->y))[3] ^= 0x80;
}

void fSinhZ(struct fComplex *x, struct fComplex *z) {
   struct fComplex tx, ty;

   fSinh12(x->x, tx.x);
   fCos12(x->y, tx.y);
   fCosh12(x->x, ty.x);
   fSin12(x->y, ty.y);
   fMul(tx.x, tx.y, z->x);
   fMul(ty.x, ty.y, z->y);
}

void fTanZ(struct fComplex *x, struct fComplex *z) {
   float s, c, sh, ch, m;

   fShift(x->x, 1, z->x);
   fShift(x->y, 1, z->y);
	fSin12(z->x, s);
	fCos12(z->x, c);
	fSinh12(z->y, sh);
	fSinh12(z->y, ch);
   fAdd(c, ch, m);
   fDiv(s, m, z->x);
   fDiv(sh, m, z->y);
}

void fTanhZ(struct fComplex *x, struct fComplex *z) {
   float s, c, sh, ch, m;

   fShift(x->x, 1, z->x);
   fShift(x->y, 1, z->y);
	fSin12(z->y, s);
	fCos12(z->y, c);
	fSinh12(z->x, sh);
	fSinh12(z->x, ch);
   fAdd(c, ch, m);
   fDiv(s, m, z->y);
   fDiv(sh, m, z->x);
}

