/* bignum.h */
/* Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer */

#ifndef _BIGNUM_H
#define _BIGNUM_H

/*
use 0 to indicate no bf_math
    1 to indicate either type
    BIGNUM for explicitly big integer
    BIGFLT for explicitly big float
*/

#ifndef PORT_H
#include "port.h"
#endif

typedef unsigned char far * bn_t;  /* use this for dynamic allocation */

extern int fpu;
extern int cpu;
extern int tim;

#define MATHBITS      32
#define MATHBYTES     (MATHBITS/8)
#define NUMVARS       30            /* room for this many on stack */
#define CURRENTREZ    1
#define MAXREZ        0

struct BFComplex
{
   bn_t x;
   bn_t y;
};
typedef struct BFComplex   _BFCMPLX;

struct BNComplex
{
   bn_t x;
   bn_t y;
};
typedef struct BNComplex   _BNCMPLX;

#ifdef BIG_ENDIAN /* Big Endian: &variable == &(most significant byte), moterola style */
/* prototypes */
U32 access32(BYTE far *addr);
U16 access16(BYTE far *addr);
S16 accessS16(S16 far *addr);
U32 set32(BYTE far *addr, U32 val);
U16 set16(BYTE far *addr, U16 val);
S16 setS16(S16 far *addr, S16 val);

#else /* Little Endian: &variable == &(least significant byte), intel style */
/* equivalent defines */
#define access32(addr)   (*(U32 far *)(addr))
#define access16(addr)   (*(U16 far *)(addr))
#define accessS16(addr)   (*(S16 far *)(addr))
#define set32(addr, val) (*(U32 far *)(addr) = (U32)(val))
#define set16(addr, val) (*(U16 far *)(addr) = (U16)(val))
#define setS16(addr, val) (*(S16 far *)(addr) = (S16)(val))
#endif



/* globals */
extern int bnstep, bnlength, intlength, rlength, padding, shiftfactor, decimals;
extern int bflength, rbflength, bfpadding, bfdecimals;

extern int bf_math;

extern bn_t bn_pi;
extern bn_t bnxmin, bnxmax, bnymin, bnymax, bnx3rd, bny3rd;      /* bnlength */
extern bn_t bnsxmin, bnsxmax, bnsymin, bnsymax, bnsx3rd, bnsy3rd;/* bnlength */
extern bn_t bnxdel, bnydel, bnxdel2, bnydel2, bnclosenuff;       /* bnlength */
extern bn_t bntmpsqrx, bntmpsqry, bntmp;                         /* rlength  */
extern bn_t bftmpsqrx, bftmpsqry, bftmp;                         /* rlength  */
extern _BFCMPLX bfold, bfnew, bfparm;                            /* bnlength */
extern _BNCMPLX bnold, bnnew, bnparm, bnsaved;                   /* bnlength */
extern bn_t bnparms[];                                           /* bnlength*10 */


/* functions defined in bignuma.asm or bignumc.c */
extern bn_t clear_bn(bn_t r);
extern bn_t max_bn(bn_t r);
extern bn_t copy_bn(bn_t r, bn_t n);
extern int cmp_bn(bn_t n1, bn_t n2);
extern int is_bn_neg(bn_t n);
extern int is_bn_not_zero(bn_t n);
extern bn_t add_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t add_a_bn(bn_t r, bn_t n);
extern bn_t sub_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t sub_a_bn(bn_t r, bn_t n);
extern bn_t neg_bn(bn_t r, bn_t n);
extern bn_t neg_a_bn(bn_t r);
extern bn_t double_bn(bn_t r, bn_t n);
extern bn_t double_a_bn(bn_t r);
extern bn_t half_bn(bn_t r, bn_t n);
extern bn_t half_a_bn(bn_t r);
extern bn_t unsafe_full_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t unsafe_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t unsafe_full_square_bn(bn_t r, bn_t n);
extern bn_t unsafe_square_bn(bn_t r, bn_t n);
extern bn_t mult_bn_int(bn_t r, bn_t n, U16 u);
extern bn_t mult_a_bn_int(bn_t r, U16 u);
extern bn_t unsafe_div_bn_int(bn_t r, bn_t n, U16 u);
extern bn_t div_a_bn_int(bn_t r, U16 u);


/* functions defined in bignum.c */
extern void bn_hexdump(bn_t r);
extern bn_t strtobn(bn_t r, char *s);
extern char *unsafe_bntostr(char *s, int dec, bn_t r);
extern bn_t inttobn(bn_t r, long longval);
extern long bntoint(bn_t n);
extern bn_t floattobn(bn_t r, LDBL f);

extern int  sign_bn(bn_t n);
extern bn_t abs_bn(bn_t r, bn_t n);
extern bn_t abs_a_bn(bn_t r);
extern bn_t unsafe_inv_bn(bn_t r, bn_t n);
extern bn_t unsafe_div_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t sqrt_bn(bn_t r, bn_t n);
extern bn_t exp_bn(bn_t r, bn_t n);
extern bn_t unsafe_ln_bn(bn_t r, bn_t n);
extern bn_t unsafe_sincos_bn(bn_t s, bn_t c, bn_t n);
extern bn_t unsafe_atan_bn(bn_t r, bn_t n);
extern bn_t unsafe_atan2_bn(bn_t r, bn_t ny, bn_t nx);
extern int convert_bn(bn_t new,bn_t old,int newbnlength,int newintlength,int oldbnlength,int oldintlength);

    /* "safe" versions */
extern bn_t full_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t full_square_bn(bn_t r, bn_t n);
extern bn_t square_bn(bn_t r, bn_t n);
extern bn_t div_bn_int(bn_t r, bn_t n, U16 u);
extern char *bntostr(char *s, int dec, bn_t r);
extern LDBL bntofloat(bn_t n);
extern bn_t inv_bn(bn_t r, bn_t n);
extern bn_t div_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t ln_bn(bn_t r, bn_t n);
extern bn_t sincos_bn(bn_t s, bn_t c, bn_t n);
extern bn_t atan_bn(bn_t r, bn_t n);
extern bn_t atan2_bn(bn_t r, bn_t ny, bn_t nx);

    /* misc */
extern int is_bn_zero(bn_t n);

#endif
