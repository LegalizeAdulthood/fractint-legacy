#ifndef MPMATH_H
#define MPMATH_H

#include <math.h>

struct MP {
   int Exp;
	unsigned long Mant;
};

struct MPC {
	struct MP r, i;
};

extern struct MP MPTrigTable[2][4][256], InvHalfPi, HalfPi, InvLn2, Ln2;
extern int MPaccuracy, MPOverflow;

/* Mark Peterson's expanded floating point operators.  Automatically uses
   either the 8086 or 80386 processor type specified in global 'cpu'. If
   the operation results in an overflow (result < 2**(2**14), or division
   by zero) the global 'MPoverflow' is set to one. */

extern int cpu;

struct MP MPmul(struct MP x, struct MP y);
struct MP MPdiv(struct MP x, struct MP y);
struct MP MPadd(struct MP x, struct MP y);
struct MP MPsub(struct MP x, struct MP y);
struct MP MPabs(struct MP x);
struct MP d2MP(double x);  /* Convert double to type MP */
double *MP2d(struct MP m);  /* Convert type MP to double */
struct MP fg2MP(long x, int fg); /* Convert fudged to type MP */


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
struct MP MPmul086(struct MP x, struct MP y);
struct MP MPdiv086(struct MP x, struct MP y);
struct MP MPadd086(struct MP x, struct MP y);
struct MP MPsub086(struct MP x, struct MP y);
int MPcmp086(struct MP x, struct MP y);
struct MP d2MP086(double x);
double *MP2d086(struct MP m);
struct MP fg2MP086(long x, int fg); 

struct MP MPmul386(struct MP x, struct MP y);
struct MP MPdiv386(struct MP x, struct MP y);
struct MP MPadd386(struct MP x, struct MP y);
struct MP MPsub386(struct MP x, struct MP y);
int MPcmp386(struct MP x, struct MP y);
struct MP d2MP386(double x);
double *MP2d386(struct MP m);
struct MP fg2MP386(long x, int fg); 

/* function pointer support added by Tim Wegner 12/07/89 */
extern int        (*pMPcmp)(struct MP x, struct MP y);
extern struct MP  (*pMPmul)(struct MP x, struct MP y);
extern struct MP  (*pMPdiv)(struct MP x, struct MP y);
extern struct MP  (*pMPadd)(struct MP x, struct MP y);
extern struct MP  (*pMPsub)(struct MP x, struct MP y);
extern struct MP  (*pd2MP)(double x)                 ;
extern double *   (*pMP2d)(struct MP m)              ;


/* FPU routines */
extern void FPUaptan387(double *y, double *x, double *atan);
extern void FPUcplxmul(struct complex *x, struct complex *y,
							  struct complex *z);
extern void FPUcplxdiv(struct complex *x, struct complex *y,
							  struct complex *z);
extern void FPUsincos(double *Angle, double *Sin, double *Cos);
extern void FPUsinhcosh(double *Angle, double *Sinh, double *Cosh);
extern void FPUcplxlog(struct complex *x, struct complex *z);


#endif
