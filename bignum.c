/* bignum.c - C routines for bignumbers */

/*
Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer

The bignumber format is simply a signed integer of variable length.  The
bytes are stored in reverse order (least significant byte first, most
significant byte last).  The sign bit is the highest bit of the most
significant byte.  Negatives are stored in 2's complement form.  The byte
length of the bignumbers must be a multiple of 4 for 386+ operations, and
a multiple of 2 for 8086/286 and non 80x86 machines.

Some of the arithmetic operations coded here may alter some of the
operands used.  Therefore, take note of the SIDE-EFFECTS listed with each
procedure.  If the value of an operand needs to be retained, just use
copy_bn() first.  This was done for speed sake to avoid unnecessary
copying.  If space is at such a premium that copying it would be
difficult, some of the operations only change the sign of the value.  In
this case, the original could be optained by calling neg_a_bn().

Most of the bignumber routines operate on true integers.  Some of the
procedures, however, are designed specifically for fixed decimal point
operations.  The results and how the results are interpreted depend on
where the implied decimal point is located.  The routines that depend on
where the decimal is located are:  strtobn(), bntostr(), bntoint(), inttobn(),
bntofloat(), floattobn(), inv_bn(), div_bn().  The number of bytes
designated for the integer part may be 1, 2, or 4.


BIGNUMBER FORMAT:
The following is a discription of the bignumber format and associated
variables.  The number is stored in reverse order (Least Significant Byte,
LSB, stored first in memory, Most Significant Byte, MSB, stored last).
Each '_' below represents a block of memory used for arithmetic (1 block =
4 bytes on 386+, 2 bytes on 286-).  All lengths are given in bytes.

LSB                                MSB
  _  _  _  _  _  _  _  _  _  _  _  _
n <----------- bnlength ----------->
                    intlength  ---> <---

  bnlength  = the length in bytes of the bignumber
  intlength = the number of bytes used to represent the integer part of
              the bignumber.  Possible values are 1, 2, or 4.  This
              determines the largest number that can be represented by
              the bignumber.
                intlength = 1, max value = 127.99...
                intlength = 2, max value = 32,767.99...
                intlength = 4, max value = 2,147,483,647.99...


FULL DOUBLE PRECISION MULTIPLICATION:
( full_mult_bn(), full_square_bn() )

The product of two bignumbers, n1 and n2, will be a result, r, which is
a double wide bignumber.  The integer part will also be twice as wide,
thereby eliminating the possiblity of overflowing the number.

LSB                                                                    MSB
  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
r <--------------------------- 2*bnlength ----------------------------->
                                                     2*intlength  --->  <---

If this double wide bignumber, r, needs to be converted to a normal,
single width bignumber, this is easily done with pointer arithmetic.  The
converted value starts at r+shiftfactor (where shiftfactor =
bnlength-intlength) and continues for bnlength bytes.  The lower order
bytes and the upper integer part of the double wide number can then be
ignored.

LSB                                                                    MSB
  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
r <--------------------------- 2*bnlength ----------------------------->
                                                     2*intlength  --->  <---
                                 LSB                                  MSB
	           r+shiftfactor   <----------  bnlength  ------------>
                                                       intlength  ---> <---


PARTIAL PRECISION MULTIPLICATION:
( mult_bn(), square_bn() )

In most cases, full double precision multiplication is not necessary.  The
lower order bytes are usually thrown away anyway.  The non-"full"
multiplication routines only calculate rlength bytes in the result.  The
value of rlength must be in the range: 2*bnlength <= rlength < bnlength.
The amount by which rlength exceeds bnlength accounts for the extra bytes
that must be multiplied so that the first bnlength bytes are correct.
These extra bytes are refered to in the code as the "padding," that is:
rlength=bnlength+padding.

All three of the values, bnlength, rlength, and therefore padding, must be
multiples of the size of memory blocks being used for arithmetic (2 on
8086/286 and 4 on 386+).  Typically, the padding is 2*blocksize.  In the
case where bnlength=blocksize, padding can only be blocksize to keep
rlength from being too big.

The product of two bignumbers, n1 and n2, will then be a result, r, which
is of length rlength.  The integer part will be twice as wide, thereby
eliminating the possiblity of overflowing the number.

             LSB                                      MSB
               _  _  _  _  _  _  _  _  _  _  _  _  _  _
            r  <---- rlength = bnlength+padding ------>
                                    2*intlength  --->  <---

If r needs to be converted to a normal, single width bignumber, this is
easily done with pointer arithmetic.  The converted value starts at
r+shiftfactor (where shiftfactor = padding-intlength) and continues for
bnlength bytes.  The lower order bytes and the upper integer part of the
double wide number can then be ignored.

             LSB                                      MSB
               _  _  _  _  _  _  _  _  _  _  _  _  _  _
            r  <---- rlength = bnlength+padding ------>
                                    2*intlength  --->  <---
                   LSB                                  MSB
      r+shiftfactor  <----------  bnlength  --------->
                                       intlength ---> <---
*/

/************************************************************************/
/* There are three parts to the bignum library:                         */
/*                                                                      */
/* 1) bignum.c - initialization, general routines, routines that would  */
/*    not be speeded up much with assembler.                            */
/*                                                                      */
/* 2) bignuma.asm - hand coded assembler routines.                      */
/*                                                                      */
/* 3) bignumc.c - portable C versions of routines in bignuma.asm        */
/*                                                                      */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "prototyp.h"
#include "bignum.h"
#ifdef sign
#undef sign
#endif

/* globals */
extern int bnstep, bnlength, intlength, rlength, padding, shiftfactor, decimals;
extern int bflength, rbflength, bfshiftfactor, bfdecimals;

/* used internally by bignum.c routines */
extern bn_t bntmp1, bntmp2, bntmp3, bntmp4, bntmp5, bntmp6; /* rlength  */
extern bn_t bntmpcpy1, bntmpcpy2;                           /* bnlength */

/* used by other routines */
extern  bn_t bn_pi;
extern  bn_t bnxmin, bnxmax, bnymin, bnymax, bnx3rd, bny3rd;        /* bnlength */
extern  bn_t bnxdel, bnydel, bnxdel2, bnydel2, bnclosenuff;         /* bnlength */
extern  bn_t bntmpsqrx, bntmpsqry, bntmp;                           /* rlength  */
extern  _BNCMPLX bnold, bnnew, bnparm, bnsaved;                     /* bnlength */

#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795

/*************************************************************************
* The original bignumber code was written specifically for a Little Endian
* system (80x86).  The following is not particularly efficient, but was
* simple to incorporate.  If speed with a Big Endian machine is critical,
* the bignumber format could be reversed.
**************************************************************************/
#ifdef BIG_ENDIAN /* Big Endian: &variable == &(most significant byte), moterola style */
U32 access32(BYTE far *addr)
    {
    return addr[0] | ((U32)addr[1]<<8) | ((U32)addr[2]<<16) | ((U32)addr[3]<<24);
    }

U16 access16(BYTE far *addr)
    {
    return (U16)addr[0] | ((U16)addr[1]<<8);
    }

S16 accessS16(S16 far *addr)
    {
    return (S16)((BYTE *)addr)[0] | ((S16)((BYTE *)addr)[1]<<8);
    }

U32 set32(BYTE far *addr, U32 val)
    {
    addr[0] = val&0xff;
    addr[1] = (val>>8)&0xff;
    addr[2] = (val>>16)&0xff;
    addr[3] = (val>>24)&0xff;
    return val;
    }

U16 set16(BYTE far *addr, U16 val)
    {
    addr[0] = val&0xff;
    addr[1] = (val>>8)&0xff;
    return val;
    }

S16 setS16(S16 far *addr, S16 val)
    {
    ((BYTE *)addr)[0] = val&0xff;
    ((BYTE *)addr)[1] = (val>>8)&0xff;
    return val;
    }
#endif

#if (_MSC_VER >= 700)
#pragma code_seg ("bignum1_text")     /* place following in an overlay */
#endif

/************************************************************************/
/* convert_bn  -- convert bignum numbers from old to new lengths        */
int convert_bn(bn_t new, bn_t old, int newbnlength, int newintlength,
                                   int oldbnlength, int oldintlength)
   {
   int savebnlength, saveintlength;

   /* save lengths so not dependent on external environment */
   saveintlength = intlength;
   savebnlength  = bnlength;

   intlength     = newintlength;
   bnlength      = newbnlength;
   clear_bn(new);

   if(newbnlength - newintlength > oldbnlength - oldintlength)
      {

      /* This will keep the integer part from overflowing past the array. */
      bnlength = oldbnlength - oldintlength + min(oldintlength, newintlength);

      _fmemcpy(new+newbnlength-newintlength-oldbnlength+oldintlength,
               old,bnlength);
      }
   else
      {
      bnlength = newbnlength - newintlength + min(oldintlength, newintlength);
      _fmemcpy(new,old+oldbnlength-oldintlength-newbnlength+newintlength,
               bnlength);
      }
   intlength = saveintlength;
   bnlength  = savebnlength;
   return(0);
   }

/********************************************************************/
/* bn_hexdump() - for debugging, dumps to stdout                    */

void bn_hexdump(bn_t r)
    {
    int i;

    for (i=0; i<bnlength; i++)
        printf("%02X ", r[i]);
    printf("\n");
    return;
    }

/**********************************************************************/
/* strtobn() - converts a string into a bignumer                       */
/*   r - pointer to a bignumber                                       */
/*   s - string in the floating point format [-][digits].[digits]     */
/*   note: the string may not be empty or have extra space and may    */
/*   not use scientific notation (2.3e4).                             */

bn_t strtobn(bn_t r, char *s)
    {
    unsigned l;
    bn_t onesbyte;
    int signflag=0;
    long longval;

    clear_bn(r);
    onesbyte = r + bnlength - intlength;

    if (s[0] == '+')    /* for + sign */
        {
        s++;
        }
    else if (s[0] == '-')    /* for neg sign */
        {
        signflag = 1;
        s++;
        }

    if (strchr(s, '.') != NULL) /* is there a decimal point? */
        {
        l = strlen(s) - 1;      /* start with the last digit */
        while (s[l] >= '0' && s[l] <= '9') /* while a digit */
            {
            *onesbyte = (BYTE)(s[l--] - '0');
            div_a_bn_int(r, 10);
            }

        if (s[l] == '.')
            {
            longval = atol(s);
            switch (intlength)
                { /* only 1, 2, or 4 are allowed */
                case 1:
                    *onesbyte = (BYTE)longval;
                    break;
                case 2:
                    set16(onesbyte, (U16)longval);
                    break;
                case 4:
                    set32(onesbyte, longval);
                    break;
                }
            }
        }
    else
        {
        longval = atol(s);
        switch (intlength)
            { /* only 1, 2, or 4 are allowed */
            case 1:
                *onesbyte = (BYTE)longval;
                break;
            case 2:
                set16(onesbyte, (U16)longval);
                break;
            case 4:
                set32(onesbyte, longval);
                break;
            }
        }


    if (signflag)
        neg_a_bn(r);

    return r;
    }

/********************************************************************/
/* strlen_needed() - returns string length needed to hold bignumber */

int strlen_needed()
   {
   int length;

   /* first space for integer part */
   switch(intlength)
      {
      case 1:
         length = 3;  /* max 127 */
         break;
      case 2:
         length = 5;  /* max 32767 */
         break;
      case 4:
         length = 10; /* max 2147483647 */
         break;
      }
    length += decimals;  /* decimal part */
    length += 2;         /* decimal point and sign */
    length += 4;         /* null and a little extra for safety */
    return(length);
    }
         
/********************************************************************/
/* bntostr() - converts a bignumber into a string                    */
/*   s - string, must be large enough to hold the number.           */
/*   r - bignumber                                                  */
/*   will covert to a floating point notation                       */
/*   SIDE-EFFECT: the bignumber, r, is destroyed.                   */
/*                Copy it first if necessary.                       */

char *unsafe_bntostr(char *s, int dec, bn_t r)
    {
    int l=0, d;
    bn_t onesbyte;
    long longval;

    if (dec == 0)
        dec = decimals;
    onesbyte = r + bnlength - intlength;

    if (is_bn_neg(r))
        {
        neg_a_bn(r);
        *(s++) = '-';
        }
    switch (intlength)
        { /* only 1, 2, or 4 are allowed */
        case 1:
            longval = *onesbyte;
            break;
        case 2:
            longval = access16(onesbyte);
            break;
        case 4:
            longval = access32(onesbyte);
            break;
        }
    ltoa(longval, s, 10);
    l = strlen(s);
    s[l++] = '.';
    for (d=0; d < dec; d++)
        {
        *onesbyte = 0;  /* clear out highest byte */
        mult_a_bn_int(r, 10);
        if (is_bn_zero(r))
            break;
        s[l++] = (BYTE)(*onesbyte + '0');
        }
    s[l] = '\0'; /* don't forget nul char */

    return s;
    }

#if (_MSC_VER >= 700)
#pragma code_seg ( )         /* back to normal segment */
#endif

/*********************************************************************/
/*  b = l                                                            */
/*  Converts a long to a bignumber                                   */
bn_t inttobn(bn_t r, long longval)
    {
    bn_t onesbyte;

    clear_bn(r);
    onesbyte = r + bnlength - intlength;
    switch (intlength)
        { /* only 1, 2, or 4 are allowed */
        case 1:
            *onesbyte = (BYTE)longval;
            break;
        case 2:
            set16(onesbyte, (U16)longval);
            break;
        case 4:
            set32(onesbyte, longval);
            break;
        }
    return r;
    }

/*********************************************************************/
/*  l = floor(b), floor rounds down                                  */
/*  Converts the integer part a bignumber to a long                  */
long bntoint(bn_t n)
    {
    bn_t onesbyte;
    long longval;

    onesbyte = n + bnlength - intlength;
    switch (intlength)
        { /* only 1, 2, or 4 are allowed */
        case 1:
            longval = *onesbyte;
            break;
        case 2:
            longval = access16(onesbyte);
            break;
        case 4:
            longval = access32(onesbyte);
            break;
        }
    return longval;
    }

/*********************************************************************/
/*  b = f                                                            */
/*  Converts a double to a bignumber                                 */
bn_t floattobn(bn_t r, LDBL f)
    {
#ifndef USE_BIGNUM_C_CODE
    floattobf(bftmp1, f);
    return bftobn(r, bftmp1);
    
#else
    bn_t onesbyte;
    int i;
    int signflag=0;
 
    clear_bn(r);
    onesbyte = r + bnlength - intlength;
 
    if (f < 0)
        {
        signflag = 1;
        f = -f;
        }
 
    switch (intlength)
        { /* only 1, 2, or 4 are allowed */
        case 1:
            *onesbyte = (BYTE)f;
            break;
        case 2:
            set16(onesbyte, (U16)f);
            break;
        case 4:
            set32(onesbyte, (U32)f);
            break;
        }
 
    f -= (long)f; /* keep only the decimal part */
    for (i = bnlength - intlength - 1; i >= 0 && f != 0.0; i--)
        {
        f *= 256;
        r[i] = (BYTE)f;  /* keep use the integer part */
        f -= (BYTE)f; /* now throw away the integer part */
        }
 
    if (signflag)
        neg_a_bn(r);
 
    return r;
#endif /* USE_BIGNUM_C_CODE */
    }

/********************************************************************/
/* sign(r)                                                          */
int sign_bn(bn_t n)
    {
    return is_bn_neg(n) ? -1 : is_bn_not_zero(n) ? 1 : 0;
    }

/********************************************************************/
/* r = |n|                                                          */
bn_t abs_bn(bn_t r, bn_t n)
    {
    copy_bn(r,n);    
    if (is_bn_neg(r))
        neg_a_bn(r);
    return r;
    }

/********************************************************************/
/* r = |r|                                                          */
bn_t abs_a_bn(bn_t r)
    {
    if (is_bn_neg(r))
        neg_a_bn(r);
    return r;
    }

/********************************************************************/
/* r = 1/n                                                          */
/* uses bntmp1 - bntmp3 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|    Make copy first if necessary.           */
bn_t unsafe_inv_bn(bn_t r, bn_t n)
    {
    int signflag=0, i;
    long maxval;
    LDBL f;

    /* use Newton's recursive method for zeroing in on 1/n : r=r(2-rn) */

    if (is_bn_neg(n))
        { /* will be a lot easier to deal with just positives */
        signflag = 1;
        neg_a_bn(n);
        }

    f = bntofloat(n);
    if (f == 0) /* division by zero */
        {
        max_bn(r);
        return r;
        }
    f = 1/f; /* approximate inverse */
    maxval = (1L << ((intlength<<3)-1)) - 1;
    if (f > maxval) /* check for overflow */
        {
        max_bn(r);
        return r;
        }
    else if (f <= -maxval)
        {
        max_bn(r);
        neg_a_bn(r);
        return r;
        }

    floattobn(r, f); /* start with approximate inverse */
    clear_bn(bntmp2); /* will be used as 1.0 and 2.0 */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        unsafe_mult_bn(bntmp1, r, n); /* bntmp1=rn */
        bntmp2[bnlength-intlength] = 1; /* bntmp2 = 1.0 */
        if ( !cmp_bn(bntmp2, bntmp1+shiftfactor) ) /* if not different */
            break;  /* they must be the same */
        bntmp2[bnlength-intlength] = 2; /* bntmp2 = 2.0 */
        sub_bn(bntmp3, bntmp2, bntmp1+shiftfactor); /* bntmp3=2-rn */
        unsafe_mult_bn(bntmp1, r, bntmp3); /* bntmp1=r(2-rn) */
        copy_bn(r, bntmp1+shiftfactor); /* r = bntmp1 */
        }

    if (signflag)
        {
        neg_a_bn(r);
        }
    return r;
    }

/********************************************************************/
/* r = n1/n2                                                        */
/*      r - result of length bnlength                               */
/* uses bntmp1 - bntmp3 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n1, n2 end up as |n1|, |n2|                                 */
/*      Make copies first if necessary.                             */
bn_t unsafe_div_bn(bn_t r, bn_t n1, bn_t n2)
    {
    int scale1, scale2, scale, sign=0, i;
    long maxval;
    LDBL a, b, f;

    /* first, check for valid data */
    a = bntofloat(n1);
    if (a == 0) /* division into zero */
        {
        clear_bn(r); /* return 0 */
        return r;
        }
    b = bntofloat(n2);
    if (b == 0) /* division by zero */
        {
        max_bn(r);
        return r;
        }
    f = a/b; /* approximate quotient */
    maxval = (1L << ((intlength<<3)-1)) - 1;
    if (f > maxval) /* check for overflow */
        {
        max_bn(r);
        return r;
        }
    else if (f <= -maxval)
        {
        max_bn(r);
        neg_a_bn(r);
        return r;
        }
    /* appears to be ok, do division */

    if (is_bn_neg(n1))
        {
        neg_a_bn(n1);
        sign = !sign;
        }
    if (is_bn_neg(n2))
        {
        neg_a_bn(n2);
        sign = !sign;
        }

    /* scale n1 and n2 so: |n| >= 1/256 */
    /* scale = (int)(log(1/fabs(a))/LOG_256) = LOG_256(1/|a|) */
    i = bnlength-1;
    while (i >= 0 && n1[i] == 0)
        i--;
    scale1 = bnlength - i - 2;
    if (scale1 < 0)
       scale1 = 0;
    i = bnlength-1;
    while (i >= 0 && n2[i] == 0)
        i--;
    scale2 = bnlength - i - 2;
    if (scale2 < 0)
       scale2 = 0;

    /* important!, use memmove(), not memcpy() */
    _fmemmove(n1+scale1, n1, bnlength-scale1); /* shift bytes over */
    _fmemset(n1, 0, scale1);  /* zero out the rest */
    _fmemmove(n2+scale2, n2, bnlength-scale2); /* shift bytes over */
    _fmemset(n2, 0, scale2);  /* zero out the rest */

    unsafe_inv_bn(r, n2);
    unsafe_mult_bn(bntmp1, n1, r);
    copy_bn(r, bntmp1+shiftfactor); /* r = bntmp1 */

    if (scale1 != scale2)
        {
        /* Rescale r back to what it should be.  Overflow has already been checked */
        if (scale1 > scale2) /* answer is too big, adjust it */
            {
            scale = scale1-scale2;
            _fmemmove(r, r+scale, bnlength-scale); /* shift bytes over */
            _fmemset(r+bnlength-scale, 0, scale);  /* zero out the rest */
            }
        else if (scale1 < scale2) /* answer is too small, adjust it */
            {
            scale = scale2-scale1;
            _fmemmove(r+scale, r, bnlength-scale); /* shift bytes over */
            _fmemset(r, 0, scale);                 /* zero out the rest */
            }
        /* else scale1 == scale2 */

        }

    if (sign)
        neg_a_bn(r);

    return r;
    }

/********************************************************************/
/* sqrt(r)                                                          */
/* uses bntmp1 - bntmp4 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bn_t sqrt_bn(bn_t r, bn_t n)
    {
    int i, comp, almost_match=0;
    LDBL f;

/* use Newton's recursive method for zeroing in on sqrt(n): r=.5(r+n/r) */

    if (is_bn_neg(n))
        { /* sqrt of a neg, return 0 */
        clear_bn(r);
        return r;
        }

    f = bntofloat(n);
    if (f == 0) /* division by zero will occur */
        {
        clear_bn(r); /* sqrt(0) = 0 */
        return r;
        }
    f = sqrtl(f); /* approximate square root */
    /* no need to check overflow */

    floattobn(r, f); /* start with approximate sqrt */
    copy_bn(bntmp4, r);

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        unsafe_div_bn(bntmp4, n, r);
        add_a_bn(r, bntmp4);
        half_a_bn(r);
        if ((comp=abs(cmp_bn(r, bntmp4))) <= 2 ) /* if match or almost match */
            {
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match before? */
                { /* yes, this must be the third time */
                /* average last 4 bytes */
                *(U32 far *)r = (*(U32 far *)r + *(U32 far *)bntmp4) / 2;
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
        }

    return r;
    }

/********************************************************************/
/* exp(r)                                                           */
/* uses bntmp1, bntmp2, bntmp3 - global temp bignumbers             */
bn_t exp_bn(bn_t r, bn_t n)
    {
    U16 fact=1;

    if (is_bn_zero(n))
        {
        inttobn(r, 1);
        return r;
        }

/* use Taylor Series (very slow convergence) */
    inttobn(r, 1); /* start with r=1.0 */
    copy_bn(bntmp2, r);
    for (;;)
        {
        /* copy n, if n is negative, mult_bn() alters n */
        unsafe_mult_bn(bntmp3, bntmp2, copy_bn(bntmp1, n));
        copy_bn(bntmp2, bntmp3+shiftfactor);
        div_a_bn_int(bntmp2, fact);
        if (!is_bn_not_zero(bntmp2))
            break; /* too small to register */
        add_a_bn(r, bntmp2);
        fact++;
        }
    return r;
    }

/********************************************************************/
/* ln(r)                                                            */
/* uses bntmp1 - bntmp6 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bn_t unsafe_ln_bn(bn_t r, bn_t n)
    {
    int i, comp, almost_match=0;
    long maxval;
    LDBL f;

/* use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1 */

    if (is_bn_neg(n) || is_bn_zero(n))
        { /* error, return largest neg value */
        max_bn(r);
        neg_a_bn(r);
        return r;
        }

    f = bntofloat(n);
    f = logl(f); /* approximate ln(x) */
    maxval = (1L << ((intlength<<3)-1)) - 1;
    if (f > maxval) /* check for overflow */
        {
        max_bn(r);
        return r;
        }
    else if (f <= -maxval)
        {
        max_bn(r);
        neg_a_bn(r);
        return r;
        }
    /* appears to be ok, do ln */

    floattobn(r, f); /* start with approximate ln */
    neg_a_bn(r); /* -r */
    copy_bn(bntmp5, r); /* -r */
    inttobn(bntmp4, 1);

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        exp_bn(bntmp6, r);     /* exp(-r) */
        unsafe_mult_bn(bntmp2, bntmp6, n);  /* n*exp(-r) */
        sub_a_bn(bntmp2+shiftfactor, bntmp4);   /* n*exp(-r) - 1 */
        sub_a_bn(r, bntmp2+shiftfactor);        /* -r - (n*exp(-r) - 1) */

        if ((comp=abs(cmp_bn(r, bntmp5))) <= 2 ) /* if match or almost match */
            {
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match twice before? */
                { /* yes, this must be the third time */
                /* average last 4 bytes */
                *(U32 far *)r = (*(U32 far *)r + *(U32 far *)bntmp5) / 2;
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
        copy_bn(bntmp5, r); /* -r */
        }
    neg_a_bn(r); /* -(-r) */
    return r;
    }

/********************************************************************/
/* sincos_bn(r)                                                     */
/* uses bntmp1 - bntmp2 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n| mod (pi/4)                                 */
bn_t unsafe_sincos_bn(bn_t s, bn_t c, bn_t n)
    {
    U16 fact=2;
    int k=0, i, halves;
    int signcos=0, signsin=0, switch_sincos=0;

    /* assure range 0 <= x < pi/4 */

    if (is_bn_zero(n))
        {
        clear_bn(s);    /* sin(0) = 0 */
        inttobn(c, 1);  /* cos(0) = 1 */
        return s;
        }

    if (is_bn_neg(n))
        {
        signsin = !signsin; /* sin(-x) = -sin(x), odd; cos(-x) = cos(x), even */
        neg_a_bn(n);
        }
    /* n >= 0 */

    double_bn(bntmp1, bn_pi); /* 2*pi */
    /* this could be done with remainders, but it would probably be slower */
    while (cmp_bn(n, bntmp1) >= 0) /* while n >= 2*pi */
        sub_a_bn(n, bntmp1);
    /* 0 <= n < 2*pi */

    copy_bn(bntmp1, bn_pi); /* pi */
    if (cmp_bn(n, bntmp1) >= 0) /* if n >= pi */
        {
        sub_a_bn(n, bntmp1);
        signsin = !signsin;
        signcos = !signcos;
        }
    /* 0 <= n < pi */

    half_bn(bntmp1, bn_pi); /* pi/2 */
    if (cmp_bn(n, bntmp1) > 0) /* if n > pi/2 */
        {
        sub_bn(n, bn_pi, n);   /* pi - n */
        signcos = !signcos;
        }
    /* 0 <= n < pi/2 */

    half_bn(bntmp1, bn_pi); /* pi/2 */
    half_a_bn(bntmp1);      /* pi/4 */
    if (cmp_bn(n, bntmp1) > 0) /* if n > pi/4 */
        {
        half_bn(bntmp1, bn_pi); /* pi/2 */
        sub_bn(n, bntmp1, n);  /* pi/2 - n */
        switch_sincos = !switch_sincos;
        }
    /* 0 <= n < pi/4 */

    /* this looks redundant, but n could now be zero when it wasn't before */
    if (is_bn_zero(n))
        {
        clear_bn(s);    /* sin(0) = 0 */
        inttobn(c, 1);  /* cos(0) = 1 */
        return s;
        }


/* at this point, the double angle trig identities could be used as many  */
/* times as desired to reduce the range to pi/8, pi/16, etc...  Each time */
/* the range is cut in half, the number of iterations required is reduced */
/* by "quite a bit."  It's just a matter of testing to see what gives the */
/* optimal results.                                                       */
   /* halves = bnlength / 10; */ /* this is experimental */
   halves = 1;
   for (i = 0; i < halves; i++)
       half_a_bn(n);

/* use Taylor Series (very slow convergence) */
    copy_bn(s, n); /* start with s=n */
    inttobn(c, 1); /* start with c=1 */
    copy_bn(bntmp1, n); /* the current x^n/n! */

    for (;;)
        {
        /* even terms for cosine */
        unsafe_mult_bn(bntmp2, bntmp1, n);
        copy_bn(bntmp1, bntmp2+shiftfactor);
        div_a_bn_int(bntmp1, fact++);
        if (!is_bn_not_zero(bntmp1))
            break; /* too small to register */
        if (k) /* alternate between adding and subtracting */
            add_a_bn(c, bntmp1);
        else
            sub_a_bn(c, bntmp1);

        /* odd terms for sine */
        unsafe_mult_bn(bntmp2, bntmp1, n);
        copy_bn(bntmp1, bntmp2+shiftfactor);
        div_a_bn_int(bntmp1, fact++);
        if (!is_bn_not_zero(bntmp1))
            break; /* too small to register */
        if (k) /* alternate between adding and subtracting */
            add_a_bn(s, bntmp1);
        else
            sub_a_bn(s, bntmp1);
        k = !k; /* toggle */
        }

     /* now need to undo what was done by cutting angles in half */
     inttobn(bntmp1, 1);
     for (i = 0; i < halves; i++)
         {
         unsafe_mult_bn(bntmp2, s, c); /* no need for safe mult */
         double_bn(s, bntmp2+shiftfactor); /* sin(2x) = 2*sin(x)*cos(x) */
         unsafe_square_bn(bntmp2,c);
         double_a_bn(bntmp2+shiftfactor);
         sub_bn(c, bntmp2+shiftfactor, bntmp1); /* cos(2x) = 2*cos(x)*cos(x) - 1 */
         }

    if (switch_sincos)
         {
         copy_bn(bntmp1, s);
         copy_bn(s, c);
         copy_bn(c, bntmp1);
         }
     if (signsin)
         neg_a_bn(s);
     if (signcos)
         neg_a_bn(c);
     return s; /* return sine I guess */
    }

/********************************************************************/
/* atan(r)                                                          */
/* uses bntmp1 - bntmp5 - global temp bignumbers                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bn_t unsafe_atan_bn(bn_t r, bn_t n)
    {
    int i, comp, almost_match=0, signflag=0;
    LDBL f;

/* use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r)) */

    if (is_bn_neg(n))
        {
        signflag = 1;
        neg_a_bn(n);
        }

    f = bntofloat(n);
    f = atanl(f); /* approximate arctangent */
    /* no need to check overflow */

    floattobn(r, f); /* start with approximate atan */
    copy_bn(bntmp3, r);

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        unsafe_sincos_bn(bntmp4, bntmp5, bntmp3);   /* sin(r), cos(r) */
        copy_bn(bntmp3, r); /* restore bntmp3 from sincos_bn() */
        copy_bn(bntmp1, bntmp5);
        unsafe_mult_bn(bntmp2, n, bntmp1);     /* n*cos(r) */
        sub_a_bn(bntmp4, bntmp2+shiftfactor); /* sin(r) - n*cos(r) */
        unsafe_mult_bn(bntmp1, bntmp5, bntmp4); /* cos(r) * (sin(r) - n*cos(r)) */
        sub_a_bn(r, bntmp1+shiftfactor); /* r - cos(r) * (sin(r) - n*cos(r)) */

        if ((comp=abs(cmp_bn(r, bntmp3))) <= 2 ) /* if match or almost match */
            {
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match before? */
                { /* yes, this must be the third time */
                /* average last 4 bytes */
                *(U32 far *)r = (*(U32 far *)r + *(U32 far *)bntmp3) / 2;
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
        copy_bn(bntmp3, r); /* make a copy for later comparison */
        }

    if (signflag)
        neg_a_bn(r);
    return r;
    }

/********************************************************************/
/* atan2(r,ny,nx)                                                     */
/* uses bntmp1 - bntmp6 - global temp bigfloats                     */
bn_t unsafe_atan2_bn(bn_t r, bn_t ny, bn_t nx)
   {
   int signx, signy;

   signx = sign_bn(nx);
   signy = sign_bn(ny);

   if (signy == 0)
      {
      if (signx < 0)
         copy_bn(r, bn_pi); /* negative x axis, 180 deg */
      else    /* signx >= 0    positive x axis, 0 */
         clear_bn(r);
      return(r);
      }
   if (signx == 0)
      {
      copy_bn(r, bn_pi); /* y axis */
      half_a_bn(r);      /* +90 deg */
      if (signy < 0)
         neg_a_bn(r);    /* -90 deg */
      return(r);
      }

   if (signy < 0)
      neg_a_bn(ny);
   if (signx < 0)
      neg_a_bn(nx);
   unsafe_div_bn(bntmp6,ny,nx);
   unsafe_atan_bn(r, bntmp6);
   if (signx < 0)
      sub_bn(r,bn_pi,r);
   if(signy < 0)
      neg_a_bn(r);
   return(r);
   }


/**********************************************************************/
/* The rest of the functions are "safe" versions of the routines that */
/* have side effects which alter the parameters                       */
/**********************************************************************/

/**********************************************************************/
bn_t full_mult_bn(bn_t r, bn_t n1, bn_t n2)
    {
    int sign1, sign2;

    sign1 = is_bn_neg(n1);
    sign2 = is_bn_neg(n2);
    unsafe_full_mult_bn(r, n1, n2);
    if (sign1)
        neg_a_bn(n1);
    if (sign2)
        neg_a_bn(n2);
    return r;
    }

/**********************************************************************/
bn_t mult_bn(bn_t r, bn_t n1, bn_t n2)
    {
    int sign1, sign2;

    /* TW ENDFIX */   
    sign1 = is_bn_neg(n1);
    sign2 = is_bn_neg(n2);
    unsafe_mult_bn(r, n1, n2);
    if (sign1)
        neg_a_bn(n1);
    if (sign2)
        neg_a_bn(n2);
    return r;
    }

/**********************************************************************/
bn_t full_square_bn(bn_t r, bn_t n)
    {
    int sign;

    sign = is_bn_neg(n);
    unsafe_full_square_bn(r, n);
    if (sign)
        neg_a_bn(n);
    return r;
    }

/**********************************************************************/
bn_t square_bn(bn_t r, bn_t n)
    {
    int sign;

    sign = is_bn_neg(n);
    unsafe_square_bn(r, n);
    if (sign)
        neg_a_bn(n);
    return r;
    }

/**********************************************************************/
bn_t div_bn_int(bn_t r, bn_t n, U16 u)
    {
    int sign;

    sign = is_bn_neg(n);
    unsafe_div_bn_int(r, n, u);
    if (sign)
        neg_a_bn(n);
    return r;
    }

#if (_MSC_VER >= 700)
#pragma code_seg ("bignum1_text")     /* place following in an overlay */
#endif

/**********************************************************************/
char *bntostr(char *s, int dec, bn_t r)
    {
    return unsafe_bntostr(s, dec, copy_bn(bntmpcpy2, r));
    }

#if (_MSC_VER >= 700)
#pragma code_seg ( )        /* back to normal segment */
#endif

/**********************************************************************/
bn_t inv_bn(bn_t r, bn_t n)
    {
    int sign;

    sign = is_bn_neg(n);
    unsafe_inv_bn(r, n);
    if (sign)
        neg_a_bn(n);
    return r;
    }

/**********************************************************************/
bn_t div_bn(bn_t r, bn_t n1, bn_t n2)
    {
    copy_bn(bntmpcpy1, n1);
    copy_bn(bntmpcpy2, n2);
    return unsafe_div_bn(r, bntmpcpy1, bntmpcpy2);
    }

/**********************************************************************/
bn_t ln_bn(bn_t r, bn_t n)
    {
#if 0
    int sign;

    sign = is_bn_neg(n);
    unsafe_ln_bn(r, n);
    if (sign)
        neg_a_bn(n);
#endif
    copy_bn(bntmpcpy1, n); /* allows r and n to overlap memory */
    unsafe_ln_bn(r, bntmpcpy1);
    return r;
    }

/**********************************************************************/
bn_t sincos_bn(bn_t s, bn_t c, bn_t n)
    {
    return unsafe_sincos_bn(s, c, copy_bn(bntmpcpy1, n));
    }

/**********************************************************************/
bn_t atan_bn(bn_t r, bn_t n)
    {
    int sign;

    sign = is_bn_neg(n);
    unsafe_atan_bn(r, n);
    if (sign)
        neg_a_bn(n);
    return r;
    }

/**********************************************************************/
bn_t atan2_bn(bn_t r, bn_t ny, bn_t nx)
   {
    copy_bn(bntmpcpy1, ny);
    copy_bn(bntmpcpy2, nx);
    unsafe_atan2_bn(r, bntmpcpy1, bntmpcpy2);
    return r;
    }


/**********************************************************************/
/* Tim's miscellaneous stuff follows                                  */

/**********************************************************************/
int is_bn_zero(bn_t n)
    {
    return !is_bn_not_zero(n);
    }