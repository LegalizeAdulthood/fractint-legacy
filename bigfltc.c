/* bigfltc.c -  C version of routines to be eventually put in bigflta.asm */

/* Wesley Loewer's Big Floating Point Numbers. (C) 1994, Wesley B. Loewer */


#include <memory.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "prototyp.h"
#include "bignum.h"
#include "bigflt.h"

/********************************************************************/
/* normalize big float */
bf_t norm_bf(bf_t r)
    {
    int scale;
    BYTE hi_byte;
    S16 far *rexp;

    rexp  = (S16 far *)(r+bflength);

    /* check for overflow */
    hi_byte = r[bflength-1];
    if (hi_byte != 0x00 && hi_byte != 0xFF)
        {
        _fmemmove(r, r+1, bflength-1);
        r[bflength-1] = (BYTE)(hi_byte & 0x80 ? 0xFF : 0x00);
        setS16(rexp,accessS16(rexp)+1);   /* exp */
        }

    /* check for underflow */
    else
        {
        for (scale = 2; scale < bflength && r[bflength-scale] == hi_byte; scale++)
            ; /* do nothing */
        if (scale == bflength && hi_byte == 0) /* zero */
            *rexp = 0;
        else
            {
            scale -= 2;
            if (scale > 0) /* it did underflow */
                {
                _fmemmove(r+scale, r, bflength-scale-1);
                _fmemset(r, 0, scale);
                setS16(rexp,accessS16(rexp)-(S16)scale);    /* exp */
                }
            }
        }

    return r;
    }

/********************************************************************/
/* normalize big float with forced sign */
/* positive = 1, force to be positive   */
/*          = 0, force to be negative   */
void norm_sign_bf(bf_t r, int positive)
    {
    norm_bf(r);
    r[bflength-1] = (BYTE)(positive ? 0x00 : 0xFF);
    }
/******************************************************/
/* adjust n1, n2 for before addition or subtraction   */
/* by forcing exp's to match.                         */
/* returns the value of the adjusted exponents        */
S16 adjust_bf_add(bf_t n1, bf_t n2)
    {
    int scale, fill_byte;
    S16 rexp;
    S16 far *n1exp, far *n2exp;

    /* scale n1 or n2 */
    /* compare exp's */
    n1exp = (S16 far *)(n1+bflength);
    n2exp = (S16 far *)(n2+bflength);
    if (accessS16(n1exp) > accessS16(n2exp))
        { /* scale n2 */
        scale = accessS16(n1exp) - accessS16(n2exp); /* n1exp - n2exp */
        if (scale < bflength)
            {
            fill_byte = is_bf_neg(n2) ? 0xFF : 0x00;
            _fmemmove(n2, n2+scale, bflength-scale);
            _fmemset(n2+bflength-scale, fill_byte, scale);
            }
        else
            clear_bf(n2);
        *n2exp = *n1exp; /* set exp's = */
	rexp = accessS16(n2exp);
        }
    else if (accessS16(n1exp) < accessS16(n2exp))
        { /* scale n1 */
        scale = accessS16(n2exp) - accessS16(n1exp);  /* n2exp - n1exp */
        if (scale < bflength)
            {
            fill_byte = is_bf_neg(n1) ? 0xFF : 0x00;
            _fmemmove(n1, n1+scale, bflength-scale);
            _fmemset(n1+bflength-scale, fill_byte, scale);
            }
        else
            clear_bf(n1);
        *n1exp = *n2exp; /* set exp's = */
	rexp = accessS16(n2exp);
        }
    else
	rexp = accessS16(n1exp);
    return rexp;
    }

/********************************************************************/
/* r = 0 */
bf_t clear_bf(bf_t r)
    {
    _fmemset( r, 0, bflength+2); /* set array to zero */
    return r;
    }

/********************************************************************/
/* r = max positive value */
bf_t max_bf(bf_t r)
    {
    inttobf(r, 1);
    set16(r+bflength, (S16)(LDBL_MAX_EXP/8));
    return r;
    }

/********************************************************************/
/* r = n */
bf_t copy_bf(bf_t r, bf_t n)
    {
    _fmemcpy( r, n, bflength+2);
    return r;
    }

/***************************************************************************/
/* n1 != n2 ?                                                              */
/* RETURNS:                                                                */
/*  if n1 == n2 returns 0                                                  */
/*  if n1 > n2 returns a positive (steps left to go when mismatch occurred) */
/*  if n1 < n2 returns a negative (steps left to go when mismatch occurred) */

int cmp_bf(bf_t n1, bf_t n2)
    {
    int i;
    int sign1, sign2;
    S16 far *n1exp, far *n2exp;

    /* compare signs */
    sign1 = sign_bf(n1);
    sign2 = sign_bf(n2);
    if (sign1 > sign2)
        return bflength;
    else if (sign1 < sign2)
        return -bflength;
    /* signs are the same */

    /* compare exponents, using signed comparisons */
    n1exp = (S16 far *)(n1+bflength);
    n2exp = (S16 far *)(n2+bflength);
    if (accessS16(n1exp) > accessS16(n2exp))
        return sign1*(bflength);
    else if (accessS16(n1exp) < accessS16(n2exp))
        return -sign1*(bflength);

    /* To get to this point, the signs must match */
    /* so unsigned comparison is ok. */
    /* two bytes at a time */
    for (i=bflength-2; i>=0; i-=2)
        {
        if (access16(n1+i) > access16(n2+i))
            return i+2;
        else if (access16(n1+i) < access16(n2+i))
            return -(i+2);
        }
    return 0;
    }

/********************************************************************/
/* r < 0 ?                                      */
/* returns 1 if negative, 0 if positive or zero */
int is_bf_neg(bf_t n)
    {
    return (S8)n[bflength-1] < 0;
    }

/********************************************************************/
/* n != 0 ?                      */
/* RETURNS: if n != 0 returns 1  */
/*          else returns 0       */
int is_bf_not_zero(bf_t n)
    {
    int bnl;
    int retval;

    bnl = bnlength;
    bnlength = bflength;
    retval = is_bn_not_zero(n);
    bnlength = bnl;

    return retval;
    }

/********************************************************************/
/* r = n1 + n2 */
/* SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision */
bf_t unsafe_add_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl;
    S16 far *rexp;

    if (is_bf_zero(n1))
        {
        copy_bf(r, n2);
        return r;
        }
    if (is_bf_zero(n2))
        {
        copy_bf(r, n1);
        return r;
        }

    rexp = (S16 far *)(r+bflength);
    setS16(rexp,adjust_bf_add(n1, n2));

    bnl = bnlength;
    bnlength = bflength;
    add_bn(r, n1, n2);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r += n */
bf_t unsafe_add_a_bf(bf_t r, bf_t n)
    {
    int bnl;

    if (is_bf_zero(r))
        {
        copy_bf(r, n);
        return r;
        }
    if (is_bf_zero(n))
        {
        return r;
        }

    adjust_bf_add(r, n);

    bnl = bnlength;
    bnlength = bflength;
    add_a_bn(r, n);
    bnlength = bnl;

    norm_bf(r);

    return r;
    }

/********************************************************************/
/* r = n1 - n2 */
/* SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision */
bf_t unsafe_sub_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl;
    S16 far *rexp;

    if (is_bf_zero(n1))
        {
        neg_bf(r, n2);
        return r;
        }
    if (is_bf_zero(n2))
        {
        copy_bf(r, n1);
        return r;
        }

    rexp = (S16 far *)(r+bflength);
    setS16(rexp,adjust_bf_add(n1, n2));

    bnl = bnlength;
    bnlength = bflength;
    sub_bn(r, n1, n2);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r -= n */
bf_t unsafe_sub_a_bf(bf_t r, bf_t n)
    {
    int bnl;

    if (is_bf_zero(r))
        {
        neg_bf(r,n);
        return r;
        }
    if (is_bf_zero(n))
        {
        return r;
        }

    adjust_bf_add(r, n);

    bnl = bnlength;
    bnlength = bflength;
    sub_a_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = -n */
bf_t neg_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 far *rexp, far *nexp;

    rexp = (S16 far *)(r+bflength);
    nexp = (S16 far *)(n+bflength);
    *rexp = *nexp;

    bnl = bnlength;
    bnlength = bflength;
    neg_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r *= -1 */
bf_t neg_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    neg_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = 2*n */
bf_t double_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 far *rexp, far *nexp;

    rexp = (S16 far *)(r+bflength);
    nexp = (S16 far *)(n+bflength);
    *rexp = *nexp;

    bnl = bnlength;
    bnlength = bflength;
    double_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r *= 2 */
bf_t double_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    double_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = n/2 */
bf_t half_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 far *rexp, far *nexp;

    rexp = (S16 far *)(r+bflength);
    nexp = (S16 far *)(n+bflength);
    *rexp = *nexp;

    bnl = bnlength;
    bnlength = bflength;
    half_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r /= 2 */
bf_t half_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    half_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/************************************************************************/
/* r = n1 * n2                                                          */
/* Note: r will be a double wide result, 2*bflength                     */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bf_t unsafe_full_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl, dbfl;
    S16 far *rexp, far *n1exp, far *n2exp;

    if (is_bf_zero(n1) || is_bf_zero(n2) )
        {
        bflength <<= 1;
        clear_bf(r);
        bflength >>= 1;
        return r;
        }

    dbfl = 2*bflength; /* double width bflength */
    rexp  = (S16 far *)(r+dbfl); /* note: 2*bflength */
    n1exp = (S16 far *)(n1+bflength);
    n2exp = (S16 far *)(n2+bflength);
    /* add exp's */
    *rexp = (S16)(accessS16(n1exp) + accessS16(n2exp));

    bnl = bnlength;
    bnlength = bflength;
    unsafe_full_mult_bn(r, n1, n2);
    bnlength = bnl;

    /* handle normalizing full mult on individual basis */

    return r;
    }

/************************************************************************/
/* r = n1 * n2 calculating only the top rlength bytes                   */
/* Note: r will be of length rlength                                    */
/*       2*bflength <= rlength < bflength                               */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bf_t unsafe_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int positive;
    int bnl, bfl, rl;
    int rexp;
    S16 far *n1exp, far *n2exp;

    if (is_bf_zero(n1) || is_bf_zero(n2) )
        {
        clear_bf(r);
        return r;
        }

    n1exp = (S16 far *)(n1+bflength);
    n2exp = (S16 far *)(n2+bflength);
    /* add exp's */
    rexp = accessS16(n1exp) + accessS16(n2exp);

    positive = is_bf_neg(n1) == is_bf_neg(n2); /* are they the same sign? */

    bnl = bnlength;
    bnlength = bflength;
    rl = rlength;
    rlength = rbflength;
    unsafe_mult_bn(r, n1, n2);
    bnlength = bnl;
    rlength = rl;

    bfl = bflength;
    bflength = rbflength;
    set16(r+bflength, (S16)(rexp+2)); /* adjust after mult */
    norm_sign_bf(r, positive);
    bflength = bfl;
    _fmemmove(r, r+padding, bflength+2); /* shift back */

    return r;
    }

/************************************************************************/
/* r = n^2                                                              */
/*   because of the symmetry involved, n^2 is much faster than n*n      */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2+l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)*/
/*                                                                      */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bf_t unsafe_full_square_bf(bf_t r, bf_t n)
    {
    int bnl, dbfl;
    S16 far *rexp, far *nexp;

    if (is_bf_zero(n))
        {
        bflength <<= 1;
        clear_bf(r);
        bflength >>= 1;
        return r;
        }

    dbfl = 2*bflength; /* double width bflength */
    rexp  = (S16 far *)(r+dbfl); /* note: 2*bflength */
    nexp = (S16 far *)(n+bflength);
    setS16(rexp, 2 * accessS16(nexp));

    bnl = bnlength;
    bnlength = bflength;
    unsafe_full_square_bn(r, n);
    bnlength = bnl;

    /* handle normalizing full mult on individual basis */

    return r;
    }


/************************************************************************/
/* r = n^2                                                              */
/*   because of the symmetry involved, n^2 is much faster than n*n      */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2+l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)*/
/*                                                                      */
/* Note: r will be of length rlength                                    */
/*       2*bflength >= rlength > bflength                               */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bf_t unsafe_square_bf(bf_t r, bf_t n)
    {
    int bnl, bfl, rl;
    int rexp;
    S16 far *nexp;

    if (is_bf_zero(n))
        {
        clear_bf(r);
        return r;
        }

    nexp = (S16 far *)(n+bflength);
    rexp = (S16)(2 * accessS16(nexp));

    bnl = bnlength;
    bnlength = bflength;
    rl = rlength;
    rlength = rbflength;
    unsafe_square_bn(r, n);
    bnlength = bnl;
    rlength = rl;

    bfl = bflength;
    bflength = rbflength;
    set16(r+bflength, (S16)(rexp+2)); /* adjust after mult */

    norm_sign_bf(r, 1);
    bflength = bfl;
    _fmemmove(r, r+padding, bflength+2); /* shift back */

    return r;
    }

/********************************************************************/
/* r = n * u  where u is an unsigned integer */
/* SIDE-EFFECTS: n can be "de-normalized" and lose precision */
bf_t unsafe_mult_bf_int(bf_t r, bf_t n, U16 u)
    {
    int positive;
    int bnl;
    S16 far *rexp, far *nexp;

    rexp = (S16 far *)(r+bflength);
    nexp = (S16 far *)(n+bflength);
    *rexp = *nexp;

    positive = !is_bf_neg(n);

/*
if u > 0x00FF, then the integer part of the mantissa will overflow the
2 byte (16 bit) integer size.  Therefore, make adjustment before
multiplication is performed.
*/
    if (u > 0x00FF)
        { /* un-normalize n */
        _fmemmove(n, n+1, bflength-1);  /* this sign extends as well */
	setS16(rexp,accessS16(rexp)+1);
        }

    bnl = bnlength;
    bnlength = bflength;
    mult_bn_int(r, n, u);
    bnlength = bnl;

    norm_sign_bf(r, positive);
    return r;
    }

/********************************************************************/
/* r *= u  where u is an unsigned integer */
bf_t mult_a_bf_int(bf_t r, U16 u)
    {
    int positive;
    int bnl;
    S16 far *rexp;

    rexp = (S16 far *)(r+bflength);
    positive = !is_bf_neg(r);

/*
if u > 0x00FF, then the integer part of the mantissa will overflow the
2 byte (16 bit) integer size.  Therefore, make adjustment before
multiplication is performed.
*/
    if (u > 0x00FF)
        { /* un-normalize n */
        _fmemmove(r, r+1, bflength-1);  /* this sign extends as well */
	setS16(rexp,accessS16(rexp)+1);
        }

    bnl = bnlength;
    bnlength = bflength;
    mult_a_bn_int(r, u);
    bnlength = bnl;

    norm_sign_bf(r, positive);
    return r;
    }

/********************************************************************/
/* r = n / u  where u is an unsigned integer */
bf_t unsafe_div_bf_int(bf_t r, bf_t n,  U16 u)
    {
    int bnl;
    S16 far *rexp, far *nexp;

    if (u == 0) /* division by zero */
        {
        max_bf(r);
        if (is_bf_neg(n))
            neg_a_bf(r);
        return r;
        }

    rexp = (S16 far *)(r+bflength);
    nexp = (S16 far *)(n+bflength);
    *rexp = *nexp;

    bnl = bnlength;
    bnlength = bflength;
    unsafe_div_bn_int(r, n, u);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r /= u  where u is an unsigned integer */
bf_t div_a_bf_int(bf_t r, U16 u)
    {
    int bnl;

    if (u == 0) /* division by zero */
        {
        if (is_bf_neg(r))
            {
            max_bf(r);
            neg_a_bf(r);
            }
        else
            {
            max_bf(r);
            }
        return r;
        }

    bnl = bnlength;
    bnlength = bflength;
    div_a_bn_int(r, u);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1<=|m|<b and f = m*b^n                   */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_value(LDBL f, LDBL b, int *exp_ptr)
   {
   int n;
   LDBL af, ff, orig_b;
   LDBL value[15];
   unsigned powertwo;

   if (b <= 0 || f == 0)
      {
      *exp_ptr = 0;
      return 0;
      }

   orig_b = b;
   af = f >= 0 ? f: -f;     /* abs value */
   ff = af > 1 ? af : 1/af;
   n = 0;
   powertwo = 1;
   while (b < ff)
      {
      value[n] = b;
      n++;
      powertwo <<= 1;
      b *= b;
      }

   *exp_ptr = 0;
   for (; n > 0; n--)
      {
      powertwo >>= 1;
      if (value[n-1] < ff)
         {
         ff /= value[n-1];
         *exp_ptr += powertwo;
         }
      }
   if (f < 0)
      ff = -ff;
   if (af < 1)
      {
      ff = orig_b/ff;
      *exp_ptr = -*exp_ptr - 1;
      }

   return ff;
   }

/********************************************************************/
/* calculates and returns the value of f*b^n                        */
/* sort of like ldexp()                                             */
LDBL scale_value( LDBL f, LDBL b , int n )
   {
   LDBL total=1;
   int an;

   if (b == 0 || f == 0)
      return 0;

   if (n == 0)
      return f;

   an = abs(n);

   while (an != 0)
      {
      if (an & 0x0001)
         total *= b;
      b *= b;
      an >>= 1;
      }

   if (n > 0)
      f *= total;
   else /* n < 0 */
      f /= total;
   return f;
   }

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1<=|m|<10 and f = m*10^n                 */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_10(LDBL f, int *exp_ptr)
   {
   return extract_value(f, 10, exp_ptr);
   }

/********************************************************************/
/* calculates and returns the value of f*10^n                       */
/* sort of like ldexp()                                             */
LDBL scale_10( LDBL f, int n )
   {
   return scale_value( f, 10, n );
   }


#ifdef USE_BIGNUM_C_CODE

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1<=|m|<256 and f = m*256^n               */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_256(LDBL f, int *exp_ptr)
   {
   return extract_value(f, 256, exp_ptr);
   }

/********************************************************************/
/* calculates and returns the value of f*256^n                      */
/* sort of like ldexp()                                             */
/*                                                                  */
/* n must be in the range -2^12 <= n < 2^12 (2^12=4096),            */
/* which should not be a problem                                    */
LDBL scale_256( LDBL f, int n )
   {
   return scale_value( f, 256, n );
   }

/*********************************************************************/
/*  b = f                                                            */
/*  Converts a double to a bigfloat                                  */
bf_t floattobf(bf_t r, LDBL f)
    {
    int power;
    int bnl, il;
    if (f == 0)
        {
        clear_bf(r);
        return r;
        }

    /* remove the exp part */
    f = extract_256(f, &power);

    bnl = bnlength;
    bnlength = bflength;
    il = intlength;
    intlength = 2;
    floattobn(r, f);
    bnlength = bnl;
    intlength = il;

    set16(r + bflength, (S16)power); /* exp */

    return r;
    }

/*********************************************************************/
/*  b = f                                                            */
/*  Converts a double to a bigfloat                                  */
bf_t floattobf1(bf_t r, LDBL f)
    {
    char msg[80];
#ifdef USE_LONG_DOUBLE
    sprintf(msg,"%-.22Le",f);
#else
    sprintf(msg,"%-.22le",f);
#endif    
    strtobf(r,msg); 
    return r;
    }

/*********************************************************************/
/*  f = b                                                            */
/*  Converts a bigfloat to a double                                 */
LDBL bftofloat(bf_t n)
    {
    int power;
    int bnl, il;
    LDBL f;

    bnl = bnlength;
    bnlength = bflength;
    il = intlength;
    intlength = 2;
    f = bntofloat(n);
    bnlength = bnl;
    intlength = il;

    power = (S16)access16(n + bflength);
    f = scale_256(f,power);

    return f;
    }

#endif /* USE_BIGNUM_C_CODE */
