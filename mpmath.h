#ifndef MPMATH_H
#define MPMATH_H

#include <math.h>

struct MP {
   int Exp;
        unsigned long Mant;
};

struct MPC {
        struct MP x, y;
};

extern struct MP MPTrigTable[2][4][256], InvHalfPi, HalfPi, InvLn2, Ln2;
extern int MPaccuracy, MPOverflow;

/* Mark Peterson's expanded floating point operators.  Automatically uses
   either the 8086 or 80386 processor type specified in global 'cpu'. If
   the operation results in an overflow (result < 2**(2**14), or division
   by zero) the global 'MPoverflow' is set to one. */

extern int cpu;

struct MP *MPmul(struct MP x, struct MP y);
struct MP *MPdiv(struct MP x, struct MP y);
struct MP *MPadd(struct MP x, struct MP y);
struct MP *MPsub(struct MP x, struct MP y);
struct MP *MPabs(struct MP x);
struct MP *d2MP(double x);  /* Convert double to type MP */
double *MP2d(struct MP m);  /* Convert type MP to double */
struct MP *fg2MP(long x, int fg); /* Convert fudged to type MP */


int MPcmp(struct MP x, struct MP y);
/* return = -1 if x < y
          = 0 if x == y
          = 1 if x > y */

/* Mark Peterson's complex expanded floating point operators */
struct MPC MPCmul(struct MPC x, struct MPC y);
struct MPC MPCdiv(struct MPC x, struct MPC y);
struct MPC MPCadd(struct MPC x, struct MPC y);
struct MPC MPCsub(struct MPC x, struct MPC y);
int MPCcmp(struct MPC x, struct MPC y);
struct MPC MPCpow(struct MPC x, int exp);
struct MPC MPCsqr(struct MPC x);
struct MP MPCmod(struct MPC x);
struct complex MPC2cmplx(struct MPC x);
struct MPC cmplx2MPC(struct complex z);

/* Prototypes for direct calling of processor specific routines */
struct MP *MPmul086(struct MP x, struct MP y);
struct MP *MPdiv086(struct MP x, struct MP y);
struct MP *MPadd086(struct MP x, struct MP y);
struct MP *MPsub086(struct MP x, struct MP y);
int MPcmp086(struct MP x, struct MP y);
struct MP *d2MP086(double x);
double *MP2d086(struct MP m);
struct MP *fg2MP086(long x, int fg);

struct MP *MPmul386(struct MP x, struct MP y);
struct MP *MPdiv386(struct MP x, struct MP y);
struct MP *MPadd386(struct MP x, struct MP y);
struct MP *MPsub386(struct MP x, struct MP y);
int MPcmp386(struct MP x, struct MP y);
struct MP *d2MP386(double x);
double *MP2d386(struct MP m);
struct MP *fg2MP386(long x, int fg);

/* function pointer support added by Tim Wegner 12/07/89 */
extern int        (*pMPcmp)(struct MP x, struct MP y);
extern struct MP  *(*pMPmul)(struct MP x, struct MP y);
extern struct MP  *(*pMPdiv)(struct MP x, struct MP y);
extern struct MP  *(*pMPadd)(struct MP x, struct MP y);
extern struct MP  *(*pMPsub)(struct MP x, struct MP y);
extern struct MP  *(*pd2MP)(double x)                 ;
extern double *   (*pMP2d)(struct MP m)              ;
void setMPfunctions(void);


/* FPU routines */
extern void FPUaptan387(double *y, double *x, double *atan);
extern void FPUcplxmul(struct complex *x, struct complex *y,
                                                          struct complex *z);
extern void FPUcplxdiv(struct complex *x, struct complex *y,
                                                          struct complex *z);
extern void FPUsincos(double *Angle, double *Sin, double *Cos);
extern void FPUsinhcosh(double *Angle, double *Sinh, double *Cosh);
extern void FPUcplxlog(struct complex *x, struct complex *z);
extern void FPUcplxexp387(struct complex *x, struct complex *z);
extern struct complex ComplexPower(struct complex x, struct complex y);

/* Integer Routines */
void SinCos086(long x, long *sinx, long *cosx);
void SinhCosh086(long x, long *sinx, long *cosx);

/*** Formula Declarations ***/
typedef enum { D_MATH, M_MATH, L_MATH } MATH_TYPE;
extern MATH_TYPE MathType;

int ParseStr(char *Str);
int Formula(void);
int form_per_pixel(void);
int fpFormulaSetup(void);
int mpcFormulaSetup(void);
int intFormulaSetup(void);
/* char *FormStr, *FormName;    BDT commented these out */

long
   far RegFg2Float(long x, char FudgeFact),
   far RegFloat2Fg(long x, int Fudge),
   far RegDivFloat(long x, long y),
   far RegSftFloat(long x, char Shift),
   far r16Mul(long x, long y),
   far LogFudged(unsigned long x, int Fudge),
   far LogFloat14(unsigned long x),
   far ExpFloat14(long x);

unsigned long far ExpFudged(long x, int Fudge);

#define fDiv(x, y, z) (void)((*(long*)&z) = RegDivFloat(*(long*)&x, *(long*)&y))
#define fMul16(x, y, z) (void)((*(long*)&z) = r16Mul(*(long*)&x, *(long*)&y))
#define fShift(x, Shift, z) (void)((*(long*)&z) = \
   RegSftFloat(*(long*)&x, Shift))
#define Fg2Float(x, f, z) (void)((*(long*)&z) = RegFg2Float(x, f))
#define Float2Fg(x, f) RegFloat2Fg(*(long*)&x, f)
#define fLog14(x, z) (void)((*(long*)&z) = \
        RegFg2Float(LogFloat14(*(long*)&x), 16))
#define fExp14(x, z) (void)((*(long*)&z) = ExpFloat14(*(long*)&x));
#define fSqrt14(x, z) fLog14(x, z); fShift(z, -1, z); fExp14(z, z)

#ifndef _LCOMPLEX_DEFINED
struct lcomplex {
   long x, y;
};
#define _LCOMPLEX_DEFINED
#endif

union Arg {
   struct complex d;
   struct MPC m;
   struct lcomplex l;
};

extern union Arg *Arg1,*Arg2;

extern void lStkSin(),lStkCos(),lStkSinh(),lStkCosh(),lStkLog(),lStkExp(),lStkSqr();
extern void dStkSin(),dStkCos(),dStkSinh(),dStkCosh(),dStkLog(),dStkExp(),dStkSqr();

#endif
