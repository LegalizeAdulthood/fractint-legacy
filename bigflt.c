/* bigflt.c - C routines for big floating point numbers */

/*
Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer

Big Floating Point Number Format:

A big floating point number consists of a signed big integer of length
bflength and of intlength 2 (see bignum.c for details) followed by a
signed 16 bit exponent.  The value of the big floating point number is:
    value = mantissa * 256^exponent
    where the absolute value of the mantissa is in the range 1<=m<256.

Notice that this differs from the IEEE format where
    value = mantissa * 2^exponent
    where the absolute value of the mantissa is in the range 1<=m<2.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "prototyp.h"

#ifndef _BIGNUM_H
#include "bignum.h"
#endif

#ifndef _BIGFLT_H
#include "bigflt.h"
#endif

#include "biginit.h"

#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795

#if (_MSC_VER >= 700)
#pragma code_seg ("bigflt1_text")     /* place following in an overlay */
#endif

/********************************************************************/
/* bf_hexdump() - for debugging, dumps to stdout                    */

void bf_hexdump(bf_t r)
    {
    int i;

    for (i=0; i<bflength; i++)
        printf("%02X ", *(r+i));
    printf(" e %04hX ", (S16)access16(r+bflength));
    printf("\n");
    return;
    }

/**********************************************************************/
/* strtobf() - converts a string into a bigfloat                       */
/*   r - pointer to a bigfloat                                        */
/*   s - string in the floating point format [+-][dig].[dig]e[+-][dig]*/
/*   note: the string may not be empty or have extra space.           */
/*         It may use scientific notation.                            */
/* USES: bftmp1                                                       */

bf_t strtobf(bf_t r, char *s)
    {
    BYTE onesbyte;
    int signflag=0;
    char *l, *d, *e; /* pointer to s, ".", "[eE]" */
    int powerten=0, keeplooping;

    clear_bf(r);

    if (s[0] == '+')    /* for + sign */
        {
        s++;
        }
    else if (s[0] == '-')    /* for neg sign */
        {
        signflag = 1;
        s++;
        }

    d = strchr(s, '.');
    e = strchr(s, 'e');
    if (e == NULL)
        e = strchr(s, 'E');
    if (e != NULL)
        {
        powerten = atoi(e+1);    /* read in the e (x10^) part */
        l = e - 1; /* just before e */
        }
    else
        l = s + strlen(s) - 1;  /* last digit */

    if (d != NULL) /* is there a decimal point? */
        {
        while (*l >= '0' && *l <= '9') /* while a digit */
            {
            onesbyte = (BYTE)(*(l--) - '0');
            inttobf(bftmp1,onesbyte);
            unsafe_add_a_bf(r, bftmp1);
            div_a_bf_int(r, 10);
            }

        if (*(l--) == '.') /* the digit was found */
            {
            keeplooping = *l >= '0' && *l <= '9' && l>=s;
            while (keeplooping) /* while a digit */
                {
                onesbyte = (BYTE)(*(l--) - '0');
                inttobf(bftmp1,onesbyte);
                unsafe_add_a_bf(r, bftmp1);
                keeplooping = *l >= '0' && *l <= '9' && l>=s;
                if (keeplooping)
                    {
                    div_a_bf_int(r, 10);
                    powerten++;    /* increase the power of ten */
                    }
                }
            }
        }
    else
        {
        keeplooping = *l >= '0' && *l <= '9' && l>=s;
        while (keeplooping) /* while a digit */
            {
            onesbyte = (BYTE)(*(l--) - '0');
            inttobf(bftmp1,onesbyte);
            unsafe_add_a_bf(r, bftmp1);
            keeplooping = *l >= '0' && *l <= '9' && l>=s;
            if (keeplooping)
                {
                div_a_bf_int(r, 10);
                powerten++;    /* increase the power of ten */
                }
            }
        }

    if (powerten > 0)
        {
        for (; powerten>0; powerten--)
            {
            mult_a_bf_int(r, 10);
            }
        }
    else if (powerten < 0)
        {
        for (; powerten<0; powerten++)
            {
            div_a_bf_int(r, 10);
            }
        }
    if (signflag)
        neg_a_bf(r);

    return r;
    }

/********************************************************************/
/* strlen_needed() - returns string length needed to hold bigfloat */

int strlen_needed_bf()
   {
   int length;

   /* first space for integer part */
    length = 1;
    length += decimals;  /* decimal part */
    length += 2;         /* decimal point and sign */
    length += 2;         /* e and sign */
    length += 4;         /* exponent */
    length += 4;         /* null and a little extra for safety */
    return(length);
    }

/********************************************************************/
/* bftostr() - converts a bigfloat into a scientific notation string */
/*   s - string, must be large enough to hold the number.           */
/* dec - decimal places, 0 for max                                  */
/*   r - bigfloat                                                   */
/*   will convert to a floating point notation                      */
/*   SIDE-EFFECT: the bigfloat, r, is destroyed.                    */
/*                Copy it first if necessary.                       */
/* USES: bftmp1 - bftmp2                                            */
/********************************************************************/

char *unsafe_bftostr(char *s, int dec, bf_t r)
    {
    LDBL value;
    int power;
    int saved;
    bf10_t bf10tmp;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    saved = save_stack();
    bf10tmp = alloc_stack(dec+4); /* probably ought to be allocated at init */
    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    power = (S16)access16(bf10tmp+dec+2); /* where the exponent is stored */
    if (power > -4 && power < 6) /* tinker with this */
        bf10tostr_f(s, dec, bf10tmp);
    else
        bf10tostr_e(s, dec, bf10tmp);
    restore_stack(saved);
    return s;
    }


/********************************************************************/
/* the e version puts it in scientific notation, (like printf's %e) */
char *unsafe_bftostr_e(char *s, int dec, bf_t r)
    {
    LDBL value;
    int saved;
    bf10_t bf10tmp;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    saved = save_stack();
    bf10tmp = alloc_stack(dec+4); /* probably ought to be allocated at init */
    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    bf10tostr_e(s, dec, bf10tmp);
    restore_stack(saved);
    return s;
    }
    
/********************************************************************/
/* the f version puts it in decimal notation, (like printf's %f) */
char *unsafe_bftostr_f(char *s, int dec, bf_t r)
    {
    LDBL value;
    int saved;
    bf10_t bf10tmp;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    saved = save_stack();
    bf10tmp = alloc_stack(dec+4); /* probably ought to be allocated at init */
    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    bf10tostr_f(s, dec, bf10tmp);
    restore_stack(saved);
    return s;
    }

#if (_MSC_VER >= 700)
#pragma code_seg ( )       /* back to normal segment */
#endif

/*********************************************************************/
/*  bn = floor(bf)                                                   */
/*  Converts a bigfloat to a bignumber (integer)                     */
/*  bflength must be at least bnlength+2                             */
bn_t bftobn(bn_t n, bf_t f)
    {
    int fexp;
    int movebytes;
    BYTE hibyte;

    fexp = (S16)access16(f+bflength);
    if (fexp >= intlength)
        { /* if it's too big, use max value */
        max_bn(n);
        if (is_bf_neg(f))
            neg_a_bn(n);
        return n;
        }

    if (-fexp > bnlength - intlength) /* too small, return zero */
        {
        clear_bn(n);
        return n;
        }

    /* already checked for over/underflow, this should be ok */
    movebytes = bnlength - intlength + fexp + 1;
    _fmemcpy(n, f+bflength-movebytes-1, movebytes);
    hibyte = *(f+bflength-1);
    _fmemset(n+movebytes, hibyte, bnlength-movebytes); /* sign extends */
    return n;
    }

/*********************************************************************/
/*  bf = bn                                                          */
/*  Converts a bignumber (integer) to a bigfloat                     */
/*  bflength must be at least bnlength+2                             */
bf_t bntobf(bf_t f, bn_t n)
    {
    _fmemcpy(f+bflength-bnlength-1, n, bnlength);
    _fmemset(f, 0, bflength - bnlength - 1);
    *(f+bflength-1) = (BYTE)(is_bn_neg(n) ? 0xFF : 0x00); /* sign extend */
    set16(f+bflength, (S16)(intlength - 1)); /* exp */
    norm_bf(f);
    return f;
    }

/*********************************************************************/
/*  b = l                                                            */
/*  Converts a long to a bigfloat                                   */
bf_t inttobf(bf_t r, long longval)
    {
    clear_bf(r);
    set32(r+bflength-4, (S32)longval);
    set16(r+bflength, (S16)2);
    norm_bf(r);
    return r;
    }

/*********************************************************************/
/*  l = floor(b), floor rounds down                                  */
/*  Converts a bigfloat to a long                                    */
/*  note: a bf value of 2.999... will be return a value of 2, not 3  */
long bftoint(bf_t f)
    {
    int fexp;
    long longval;

    fexp = (S16)access16(f+bflength);
    if (fexp > 2)
        {
        longval = 0x7FFFFFFFL;
        if (is_bf_neg(f))
            longval = -longval;
        return longval;
        }
    longval = *(S32 far *)(f+bflength-4);
    longval >>= 8*(2-fexp);
    return longval;
    }

/********************************************************************/
/* sign(r)                                                          */
int sign_bf(bf_t n)
    {
    return is_bf_neg(n) ? -1 : is_bf_not_zero(n) ? 1 : 0;
    }

/********************************************************************/
/* r = |n|                                                          */
bf_t abs_bf(bf_t r, bf_t n)
    {
    copy_bf(r,n); 
    if (is_bf_neg(r))
       {
       neg_a_bf(r);
       }
    return r;
    }

/********************************************************************/
/* r = |r|                                                          */
bf_t abs_a_bf(bf_t r)
    {
    if (is_bf_neg(r))
        neg_a_bf(r);
    return r;
    }

/********************************************************************/
/* r = 1/n                                                          */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|/256^exp    Make copy first if necessary.   */
bf_t unsafe_inv_bf(bf_t r, bf_t n)
    {
    int signflag=0, i;
    int fexp, rexp;
    LDBL f;
    bf_t orig_r, orig_n, orig_bftmp1;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength,
         orig_bfshiftfactor;

    /* use Newton's recursive method for zeroing in on 1/n : r=r(2-rn) */

    if (is_bf_neg(n))
        { /* will be a lot easier to deal with just positives */
        signflag = 1;
        neg_a_bf(n);
        }

    fexp = (S16)access16(n+bflength);
    set16(n+bflength, (S16)0); /* put within LDBL range */

    f = bftofloat(n);
    if (f == 0) /* division by zero */
        {
        max_bf(r);
        return r;
        }
    f = 1/f; /* approximate inverse */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_bfshiftfactor = bfshiftfactor;
    orig_r             = r;
    orig_n             = n;
    orig_bftmp1        = bftmp1;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    bftmp1 = orig_bftmp1 + orig_bflength - bflength;

    floattobf(r, f); /* start with approximate inverse */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        bftmp1 = orig_bftmp1 + orig_bflength - bflength;

        unsafe_mult_bf(bftmp1, r, n); /* bftmp1=rn */
        inttobf(bftmp2, 1); /* will be used as 1.0 */
        if (cmp_bf(bftmp1, bftmp2) == 0)
            break;

        inttobf(bftmp2, 2); /* will be used as 2.0 */
        unsafe_sub_a_bf(bftmp2, bftmp1); /* bftmp2=2-rn */
        unsafe_mult_bf(bftmp1, r, bftmp2); /* bftmp1=r(2-rn) */
        copy_bf(r, bftmp1); /* r = bftmp1 */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    bfshiftfactor = orig_bfshiftfactor;
    r             = orig_r;
    n             = orig_n;
    bftmp1        = orig_bftmp1;

    if (signflag)
        {
        neg_a_bf(r);
        }
    rexp = (S16)access16(r+bflength);
    rexp -= fexp;
    set16(r+bflength, (S16)rexp); /* adjust result exponent */
    return r;
    }

/********************************************************************/
/* r = n1/n2                                                        */
/*      r - result of length bflength                               */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n1, n2 end up as |n1|/256^x, |n2|/256^x                     */
/*      Make copies first if necessary.                             */
bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int aexp, bexp, rexp;
    LDBL a, b;

    /* first, check for valid data */

    aexp = (S16)access16(n1+bflength);
    set16(n1+bflength, (S16)0); /* put within LDBL range */

    a = bftofloat(n1);
    if (a == 0) /* division into zero */
        {
        clear_bf(r); /* return 0 */
        return r;
        }

    bexp = (S16)access16(n2+bflength);
    set16(n2+bflength, (S16)0); /* put within LDBL range */

    b = bftofloat(n2);
    if (b == 0) /* division by zero */
        {
        max_bf(r);
        return r;
        }

    unsafe_inv_bf(r, n2);
    unsafe_mult_bf(bftmp1, n1, r);
    copy_bf(r, bftmp1); /* r = bftmp1 */

    rexp = (S16)access16(r+bflength);
    rexp += aexp - bexp;
    set16(r+bflength, (S16)rexp); /* adjust result exponent */

    return r;
    }

/********************************************************************/
/* sqrt(r)                                                          */
/* uses bftmp1 - bftmp3 - global temp bigfloats                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bf_t unsafe_sqrt_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0;
    LDBL f;
    bf_t orig_r, orig_n;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength,
         orig_bfshiftfactor;

/* use Newton's recursive method for zeroing in on sqrt(n): r=.5(r+n/r) */

    if (is_bf_neg(n))
        { /* sqrt of a neg, return 0 */
        clear_bf(r);
        return r;
        }

    f = bftofloat(n);
    if (f == 0) /* division by zero will occur */
        {
        clear_bf(r); /* sqrt(0) = 0 */
        return r;
        }
    f = sqrtl(f); /* approximate square root */
    /* no need to check overflow */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_bfshiftfactor = bfshiftfactor;
    orig_r             = r;
    orig_n             = n;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;

    floattobf(r, f); /* start with approximate sqrt */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;

        unsafe_div_bf(bftmp3, n, r);
        unsafe_add_a_bf(r, bftmp3);
        half_a_bf(r);
        if ((comp=abs(cmp_bf(r, bftmp3))) <= 2 ) /* if match or almost match */
            {
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match before? */
                { /* yes, this must be the third time */
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    bfshiftfactor = orig_bfshiftfactor;
    r             = orig_r;
    n             = orig_n;

    return r;
    }

/********************************************************************/
/* exp(r)                                                           */
/* uses bftmp1, bftmp2, bftmp3 - global temp bigfloats             */
bf_t exp_bf(bf_t r, bf_t n)
    {
    U16 fact=1;
    S16 far * testexp, far * rexp;

    testexp = (S16 far *)(bftmp2+bflength);
    rexp = (S16 far *)(r+bflength);

    if (is_bf_zero(n))
        {
        inttobf(r, 1);
        return r;
        }

/* use Taylor Series (very slow convergence) */
    inttobf(r, 1); /* start with r=1.0 */
    copy_bf(bftmp2, r);
    for (;;)
        {
        copy_bf(bftmp1, n);
        unsafe_mult_bf(bftmp3, bftmp2, bftmp1);
        unsafe_div_bf_int(bftmp2, bftmp3, fact);
        if (*testexp < *rexp-(bflength-2))
            break; /* too small to register */
        unsafe_add_a_bf(r, bftmp2);
        fact++;
        }

    return r;
    }

/********************************************************************/
/* ln(r)                                                            */
/* uses bftmp1 - bftmp6 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bf_t unsafe_ln_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0;
    LDBL f;
    bf_t orig_r, orig_n, orig_bftmp5;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength,
         orig_bfshiftfactor;

/* use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1 */

    if (is_bf_neg(n) || is_bf_zero(n))
        { /* error, return largest neg value */
        max_bf(r);
        neg_a_bf(r);
        return r;
        }

    f = bftofloat(n);
    f = logl(f); /* approximate ln(x) */
    /* no need to check overflow */
    /* appears to be ok, do ln */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_bfshiftfactor = bfshiftfactor;
    orig_r             = r;
    orig_n             = n;
    orig_bftmp5        = bftmp5;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    bftmp5 = orig_bftmp5 + orig_bflength - bflength;

    floattobf(r, f); /* start with approximate ln */
    neg_a_bf(r); /* -r */
    copy_bf(bftmp5, r); /* -r */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        bftmp5 = orig_bftmp5 + orig_bflength - bflength;

        exp_bf(bftmp6, r);     /* exp(-r) */
        unsafe_mult_bf(bftmp2, bftmp6, n);  /* n*exp(-r) */
        inttobf(bftmp4, 1);
        unsafe_sub_a_bf(bftmp2, bftmp4);   /* n*exp(-r) - 1 */
        unsafe_sub_a_bf(r, bftmp2);        /* -r - (n*exp(-r) - 1) */
        if ((comp=abs(cmp_bf(r, bftmp5))) <= 2 ) /* if match or almost match */
            {
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match twice before? */
                { /* yes, this must be the third time */
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
        copy_bf(bftmp5, r); /* -r */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    bfshiftfactor = orig_bfshiftfactor;
    r             = orig_r;
    n             = orig_n;
    bftmp5        = orig_bftmp5;

    neg_a_bf(r); /* -(-r) */
    return r;
    }

/********************************************************************/
/* sincos_bf(r)                                                     */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n| mod (pi/4)                                 */
bf_t unsafe_sincos_bf(bf_t s, bf_t c, bf_t n)
    {
    U16 fact=2;
    int k=0, i, halves;
    int signcos=0, signsin=0, switch_sincos=0;
    int sin_done=0, cos_done=0;
    S16 far * testexp, far * cexp, far * sexp;

    testexp = (S16 far *)(bftmp1+bflength);
    cexp = (S16 far *)(c+bflength);
    sexp = (S16 far *)(s+bflength);

#if !CALCULATING_BIG_PI
    /* assure range 0 <= x < pi/4 */

    if (is_bf_zero(n))
        {
        clear_bf(s);    /* sin(0) = 0 */
        inttobf(c, 1);  /* cos(0) = 1 */
        return s;
        }

    if (is_bf_neg(n))
        {
        signsin = !signsin; /* sin(-x) = -sin(x), odd; cos(-x) = cos(x), even */
        neg_a_bf(n);
        }
    /* n >= 0 */

    double_bf(bftmp1, bf_pi); /* 2*pi */
    /* this could be done with remainders, but it would probably be slower */
    while (cmp_bf(n, bftmp1) >= 0) /* while n >= 2*pi */
        {
        copy_bf(bftmp2, bftmp1);
        unsafe_sub_a_bf(n, bftmp2);
        }
    /* 0 <= n < 2*pi */

    copy_bf(bftmp1, bf_pi); /* pi */
    if (cmp_bf(n, bftmp1) >= 0) /* if n >= pi */
        {
        unsafe_sub_a_bf(n, bftmp1);
        signsin = !signsin;
        signcos = !signcos;
        }
    /* 0 <= n < pi */

    half_bf(bftmp1, bf_pi); /* pi/2 */
    if (cmp_bf(n, bftmp1) > 0) /* if n > pi/2 */
        {
        copy_bf(bftmp2, bf_pi);
        unsafe_sub_bf(n, bftmp2, n);
        signcos = !signcos;
        }
    /* 0 <= n < pi/2 */

    half_bf(bftmp1, bf_pi); /* pi/2 */
    half_a_bf(bftmp1);      /* pi/4 */
    if (cmp_bf(n, bftmp1) > 0) /* if n > pi/4 */
        {
        copy_bf(bftmp2, n);
        half_bf(bftmp1, bf_pi); /* pi/2 */
        unsafe_sub_bf(n, bftmp1, bftmp2);  /* pi/2 - n */
        switch_sincos = !switch_sincos;
        }
    /* 0 <= n < pi/4 */

    /* this looks redundant, but n could now be zero when it wasn't before */
    if (is_bf_zero(n))
        {
        clear_bf(s);    /* sin(0) = 0 */
        inttobf(c, 1);  /* cos(0) = 1 */
        return s;
        }


/* at this point, the double angle trig identities could be used as many  */
/* times as desired to reduce the range to pi/8, pi/16, etc...  Each time */
/* the range is cut in half, the number of iterations required is reduced */
/* by "quite a bit."  It's just a matter of testing to see what gives the */
/* optimal results.                                                       */
    /* halves = bflength / 10; */ /* this is experimental */
    halves = 1;
    for (i = 0; i < halves; i++)
        half_a_bf(n);
#endif

/* use Taylor Series (very slow convergence) */
    copy_bf(s, n); /* start with s=n */
    inttobf(c, 1); /* start with c=1 */
    copy_bf(bftmp1, n); /* the current x^n/n! */
    do
        {
        /* even terms for cosine */
        copy_bf(bftmp2, bftmp1);
        unsafe_mult_bf(bftmp1, bftmp2, n);
        div_a_bf_int(bftmp1, fact++);
        if (!cos_done)
            {
            cos_done = (*testexp < *cexp-(bflength-2)); /* too small to register */
            if (!cos_done)
                {
                if (k) /* alternate between adding and subtracting */
                    unsafe_add_a_bf(c, bftmp1);
                else
                    unsafe_sub_a_bf(c, bftmp1);
                }
            }

        /* odd terms for sine */
        copy_bf(bftmp2, bftmp1);
        unsafe_mult_bf(bftmp1, bftmp2, n);
        div_a_bf_int(bftmp1, fact++);
        if (!sin_done)
            {
            sin_done = (*testexp < *sexp-(bflength-2)); /* too small to register */
            if (!sin_done)
                {
                if (k) /* alternate between adding and subtracting */
                    unsafe_add_a_bf(s, bftmp1);
                else
                    unsafe_sub_a_bf(s, bftmp1);
                }
            }
        k = !k; /* toggle */
#if CALCULATING_BIG_PI
        printf("."); /* lets you know it's doing something */
#endif

        } while (!cos_done || !sin_done);

#if !CALCULATING_BIG_PI
        /* now need to undo what was done by cutting angles in half */
        for (i = 0; i < halves; i++)
            {
            unsafe_mult_bf(bftmp2, s, c); /* no need for safe mult */
            double_bf(s, bftmp2); /* sin(2x) = 2*sin(x)*cos(x) */
            unsafe_square_bf(bftmp2,c);
            double_a_bf(bftmp2);
            inttobf(bftmp1, 1);
            unsafe_sub_bf(c, bftmp2, bftmp1); /* cos(2x) = 2*cos(x)*cos(x) - 1 */
            }

        if (switch_sincos)
	    {
            copy_bf(bftmp1, s);
            copy_bf(s, c);
            copy_bf(c, bftmp1);
	    }
	if (signsin)
            neg_a_bf(s);
	if (signcos)
            neg_a_bf(c);
#endif

    return s; /* return sine I guess */
    }

/********************************************************************/
/* atan(r)                                                          */
/* uses bftmp1 - bftmp5 - global temp bigfloats                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n| or 1/|n|                                   */
bf_t unsafe_atan_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0, signflag=0;
    LDBL f;
    bf_t orig_r, orig_n, orig_bf_pi, orig_bftmp3;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength,
         orig_bfshiftfactor;
    int large_arg;

/* use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r)) */

    if (is_bf_neg(n))
        {
        signflag = 1;
        neg_a_bf(n);
        }

/* If n is very large, atanl() won't give enough decimal places to be a */
/* good enough initial guess for Newton's Method.  If it is larger than */
/* say, 1, atan(n) = pi/2 - acot(n) = pi/2 - atan(1/n).                 */

    f = bftofloat(n);
    large_arg = f > 1.0;
    if (large_arg)
        {
        unsafe_inv_bf(bftmp3, n);
        copy_bf(n, bftmp3);
        f = bftofloat(n);
        }

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_bfshiftfactor = bfshiftfactor;
    orig_bf_pi         = bf_pi;
    orig_r             = r;
    orig_n             = n;
    orig_bftmp3        = bftmp3;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    bf_pi = orig_bf_pi + orig_bflength - bflength;
    bftmp3 = orig_bftmp3 + orig_bflength - bflength;

    f = atanl(f); /* approximate arctangent */
    /* no need to check overflow */

    floattobf(r, f); /* start with approximate atan */
    copy_bf(bftmp3, r);


#if CALCULATING_BIG_PI
    setvideomode(3,0,0,0); /* put in text mode */
#endif

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        bf_pi = orig_bf_pi + orig_bflength - bflength;
        bftmp3 = orig_bftmp3 + orig_bflength - bflength;

#if CALCULATING_BIG_PI
        printf("\natan() loop #%i, bflength=%i\nsincos() loops\n", i, bflength);
#endif
        unsafe_sincos_bf(bftmp4, bftmp5, bftmp3);   /* sin(r), cos(r) */
        copy_bf(bftmp3, r); /* restore bftmp3 from sincos_bf() */
        copy_bf(bftmp1, bftmp5);
        unsafe_mult_bf(bftmp2, n, bftmp1);     /* n*cos(r) */
        unsafe_sub_a_bf(bftmp4, bftmp2); /* sin(r) - n*cos(r) */
        unsafe_mult_bf(bftmp1, bftmp5, bftmp4); /* cos(r) * (sin(r) - n*cos(r)) */
        copy_bf(bftmp3, r);
        unsafe_sub_a_bf(r, bftmp1); /* r - cos(r) * (sin(r) - n*cos(r)) */
        if (bflength == orig_bflength && (comp=abs(cmp_bf(r, bftmp3))) <= 2 ) /* if match or almost match */
            {
#if CALCULATING_BIG_PI
            printf("\natan() loop comp=%i\n", comp);
#endif
            if (comp == 0)  /* perfect match */
                break;
            if (almost_match == 2) /* did they almost match before? */
                { /* yes, this must be the third time */
                break;
                }
            else /* this is the first or second time they almost matched */
                almost_match++;
            }
#if CALCULATING_BIG_PI
        if (bflength == orig_bflength && comp > 2)
            printf("\natan() loop comp=%i\n", comp);
#endif

        copy_bf(bftmp3, r); /* make a copy for later comparison */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    bfshiftfactor = orig_bfshiftfactor;
    bf_pi         = orig_bf_pi;
    r             = orig_r;
    n             = orig_n;
    bftmp3        = orig_bftmp3;

    if (large_arg)
        {
        half_bf(bftmp3, bf_pi);  /* pi/2 */
        sub_a_bf(bftmp3, r);     /* pi/2 - atan(1/n) */
        copy_bf(r, bftmp3);
        }

    if (signflag)
        neg_a_bf(r);
    return r;
    }

/********************************************************************/
/* atan2(r,ny,nx)                                                     */
/* uses bftmp1 - bftmp6 - global temp bigfloats                     */
bf_t unsafe_atan2_bf(bf_t r, bf_t ny, bf_t nx)
   {
   int signx, signy;

   signx = sign_bf(nx);
   signy = sign_bf(ny);

   if (signy == 0)
      {
      if (signx < 0)
         copy_bf(r, bf_pi); /* negative x axis, 180 deg */
      else    /* signx >= 0    positive x axis, 0 */
         clear_bf(r);
      return(r);
      }
   if (signx == 0)
      {
      copy_bf(r, bf_pi); /* y axis */
      half_a_bf(r);      /* +90 deg */
      if (signy < 0)
         neg_a_bf(r);    /* -90 deg */
      return(r);
      }

   if (signy < 0)
      neg_a_bf(ny);
   if (signx < 0)
      neg_a_bf(nx);
   unsafe_div_bf(bftmp6,ny,nx);
   unsafe_atan_bf(r, bftmp6);
   if (signx < 0)
      sub_bf(r,bf_pi,r);
   if(signy < 0)
      neg_a_bf(r);
   return(r);
   }

/**********************************************************************/
/* The rest of the functions are "safe" versions of the routines that */
/* have side effects which alter the parameters.                      */
/* Most bf routines change values of parameters, not just the sign.   */
/**********************************************************************/

/**********************************************************************/
bf_t add_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_add_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t add_a_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_add_a_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t sub_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_sub_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t sub_a_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_sub_a_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
/* mult and div only change sign */
bf_t full_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_full_mult_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_mult_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t full_square_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_full_square_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t square_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_square_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t mult_bf_int(bf_t r, bf_t n, U16 u)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_mult_bf_int(r, bftmpcpy1, u);
    return r;
    }

/**********************************************************************/
bf_t div_bf_int(bf_t r, bf_t n,  U16 u)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_div_bf_int(r, bftmpcpy1, u);
    return r;
    }

#if (_MSC_VER >= 700)
#pragma code_seg ("bigflt1_text")     /* place following in an overlay */
#endif

/**********************************************************************/
char *bftostr(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr(s, dec, bftmpcpy1);
    return s;
    }

/**********************************************************************/
char *bftostr_e(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr_e(s, dec, bftmpcpy1);
    return s;
    }

/**********************************************************************/
char *bftostr_f(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr_f(s, dec, bftmpcpy1);
    return s;
    }

#if (_MSC_VER >= 700)
#pragma code_seg ( )       /* back to normal segment */
#endif

/**********************************************************************/
bf_t inv_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_inv_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t div_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_div_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t sqrt_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_sqrt_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t ln_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_ln_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t sincos_bf(bf_t s, bf_t c, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    return unsafe_sincos_bf(s, c, bftmpcpy1);
    }

/**********************************************************************/
bf_t atan_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_atan_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx)
   {
    copy_bf(bftmpcpy1, ny);
    copy_bf(bftmpcpy2, nx);
    unsafe_atan2_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
int is_bf_zero(bf_t n)
    {
    return !is_bf_not_zero(n);
    }

/************************************************************************/
/* convert_bf  -- convert bigfloat numbers from old to new lengths      */
int convert_bf(bf_t new, bf_t old, int newbflength, int oldbflength)
   {
   int savebflength;

   /* save lengths so not dependent on external environment */
   savebflength  = bflength;
   bflength      = newbflength;
   clear_bf(new);
   bflength      = savebflength;

   if(newbflength > oldbflength)
      _fmemcpy(new+newbflength-oldbflength,old,oldbflength+2);
   else
      _fmemcpy(new,old+oldbflength-newbflength,newbflength+2);
   return(0);
   }

/* big10flt.c - C routines for base 10 big floating point numbers */

/**********************************************************
(Just when you thought it was safe to go back in the water.)
Just when you thought you seen every type of format possible,
16 bit integer, 32 bit integer, double, long double, mpmath,
bn_t, bf_t, I now give you bf10_t (big float base 10)!

Why, because this is the only way (I can think of) to properly do a
bftostr() without rounding errors.  With out this, then
   -1.9999999999( > LDBL_DIG of 9's)9999999123456789...
will round to -2.0.  The good news is that we only need to do two
mathematical operations: multiplication and division by integers

bf10_t format: (notice the position of the MSB and LSB)

MSB                                         LSB
  _  _  _  _  _  _  _  _  _  _  _  _ _ _ _ _
n <><------------- dec --------------><> <->
  1 byte pad            1 byte rounding   2 byte exponent.

  total length = dec + 4

***********************************************************/

/**********************************************************************/
/* unsafe_bftobf10() - converts a bigfloat into a bigfloat10          */
/*   n - pointer to a bigfloat                                        */
/*   r - result array of BYTE big enough to hold the bf10_t number    */
/* dec - number of decimals, not including the one extra for rounding */
/*  SIDE-EFFECTS: n is changed to |n|.  Make copy of n if necessary.  */

bf10_t unsafe_bftobf10(bf10_t r, int dec, bf_t n)
   {
   int d;
   int power256;
   int p;
   int bnl;
   bf_t onesbyte;
   bf10_t power10;

   if (is_bf_zero(n))
      { /* in scientific notation, the leading digit can't be zero */
      r[1] = (BYTE)0; /* unless the number is zero */
      return r;
      }

   onesbyte = n + bflength - 1;           /* really it's n+bflength-2 */
   power256 = (S16)access16(n + bflength) + 1; /* so adjust power256 by 1  */

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = r + dec + 1;

   if (is_bf_neg(n))
      {
      neg_a_bf(n);
      r[0] = 1; /* sign flag */
      }
   else
      r[0] = 0;

   p = -1;  /* multiply by 10 right away */
   bnl = bnlength;
   bnlength = bflength;
   for (d=1; d<=dec; d++)
      {
      /* pretend it's a bn_t instead of a bf_t */
      /* this leaves n un-normalized, which is what we want here  */
      mult_a_bn_int(n, 10);

      r[d] = *onesbyte;
      if (d == 1 && r[d] == 0)
         {
         d = 0; /* back up a digit */
         p--; /* and decrease by a factor of 10 */
         }
      *onesbyte = 0;
      }
   bnlength = bnl;
   set16(power10, p); /* save power of ten */

   /* the digits are all read in, now scale it by 256^power256 */
   if (power256 > 0)
      for (d=0; d<power256; d++)
         mult_a_bf10_int(r, dec, 256);

   else if (power256 < 0)
      for (d=0; d>power256; d--)
         div_a_bf10_int(r, dec, 256);

   /* else power256 is zero, don't do anything */

   /* round the last digit */
   if (r[dec] >= 5)
      {
      d = dec-1;
      while (d > 0) /* stop before you get to the sign flag */
         {
         r[d]++;  /* round up */
         if (r[d] < 10)
            {
            d = -1; /* flag for below */
            break; /* finished rounding */
            }
         r[d] = 0;
         d--;
         }
      if (d == 0) /* rounding went back to the first digit and it overflowed */
         {
         r[1] = 0;
         _fmemmove(r+2, r+1, dec-1);
         r[1] = 1;
         p = (S16)access16(power10);
         set16(power10, p+1);
         }
      }
   r[dec] = 0; /* truncate the rounded digit */

   return r;
   }


/**********************************************************************/
/* mult_a_bf10_int()                                                  */
/* r *= n                                                             */
/* dec - number of decimals, including the one extra for rounding */

bf10_t mult_a_bf10_int(bf10_t r, int dec, U16 n)
   {
   int signflag;
   int d, p;
   unsigned value, overflow;
   bf10_t power10;

   if (r[1] == 0 || n == 0)
      {
      r[1] = 0;
      return r;
      }

   power10 = r + dec + 1;
   p = (S16)access16(power10);

   signflag = r[0];  /* r[0] to be used as a padding */
   overflow = 0;
   for (d = dec; d>0; d--)
      {
      value = r[d] * n + overflow;
      r[d] = (BYTE)(value % 10);
      overflow = value / 10;
      }
   while (overflow)
      {
      p++;
      _fmemmove(r+2, r+1, dec-1);
      r[1] = (BYTE)(overflow % 10);
      overflow = overflow / 10;
      }
   set16(power10, p); /* save power of ten */
   r[0] = (BYTE)signflag; /* restore sign flag */
   return r;
   }

/**********************************************************************/
/* div_a_bf10_int()                                                   */
/* r /= n                                                             */
/* dec - number of decimals, including the one extra for rounding */

bf10_t div_a_bf10_int (bf10_t r, int dec, U16 n)
   {
   int src, dest, p;
   unsigned value, remainder;
   bf10_t power10;

   if (r[1] == 0 || n == 0)
      {
      r[1] = 0;
      return r;
      }

   power10 = r + dec + 1;
   p = (S16)access16(power10);

   remainder = 0;
   for (src=dest=1; src<=dec; dest++, src++)
      {
      value = 10*remainder + r[src];
      r[dest] = (BYTE)(value / n);
      remainder = value % n;
      if (dest == 1 && r[dest] == 0)
         {
         dest = 0; /* back up a digit */
         p--;      /* and decrease by a factor of 10 */
         }
      }
   for (; dest<=dec; dest++)
      {
      value = 10*remainder;
      r[dest] = (BYTE)(value / n);
      remainder = value % n;
      if (dest == 1 && r[dest] == 0)
         {
         dest = 0; /* back up a digit */
         p--;      /* and decrease by a factor of 10 */
         }
      }

   set16(power10, p); /* save power of ten */
   return r;
   }


/*************************************************************************/
/* bf10tostr_e()                                                         */
/* Takes a bf10 number and converts it to an ascii string, sci. notation */
/* dec - number of decimals, not including the one extra for rounding    */

char *bf10tostr_e(char *s, int dec, bf10_t n)
   {
   int d, p;
   bf10_t power10;

   if (n[1] == 0)
      {
      strcpy(s, "0.0");
      return s;
      }

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = n + dec + 1;
   p = (S16)access16(power10);

   /* if p is negative, it is not necessary to show all the decimal places */
   if (p < 0 && dec > 8) /* 8 sounds like a reasonable value */
      {
      dec = dec + p;
      if (dec < 8) /* let's keep at least a few */
         dec = 8;
      }

   if (n[0] == 1) /* sign flag */
      *(s++) = '-';
   *(s++) = (char)(n[1] + '0');
   *(s++) = '.';
   for (d=2; d<=dec; d++)
      {
      *(s++) = (char)(n[d] + '0');
      }
   /* clean up trailing 0's */
   while (*(s-1) == '0')
      s--;
   if (*(s-1) == '.') /* put at least one 0 after the decimal */
      *(s++) = '0';
   sprintf(s, "e%d", p);
   return s;
   }

/****************************************************************************/
/* bf10tostr_f()                                                            */
/* Takes a bf10 number and converts it to an ascii string, decimal notation */

char *bf10tostr_f(char *s, int dec, bf10_t n)
   {
   int d, p;
   bf10_t power10;

   if (n[1] == 0)
      {
      strcpy(s, "0.0");
      return s;
      }

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = n + dec + 1;
   p = (S16)access16(power10);

   /* if p is negative, it is not necessary to show all the decimal places */
   if (p < 0 && dec > 8) /* 8 sounds like a reasonable value */
      {
      dec = dec + p;
      if (dec < 8) /* let's keep at least a few */
         dec = 8;
      }

   if (n[0] == 1) /* sign flag */
      *(s++) = '-';
   if (p >= 0)
      {
      for (d=1; d<=p+1; d++)
         *(s++) = (char)(n[d] + '0');
      *(s++) = '.';
      for (; d<=dec; d++)
         *(s++) = (char)(n[d] + '0');
      }
   else
      {
      *(s++) = '0';
      *(s++) = '.';
      for (d=0; d>p+1; d--)
         *(s++) = '0';
      for (d=1; d<=dec; d++)
         *(s++) = (char)(n[d] + '0');
      }

   /* clean up trailing 0's */
   while (*(s-1) == '0')
      s--;
   if (*(s-1) == '.') /* put at least one 0 after the decimal */
      *(s++) = '0';
   *s = '\0'; /* terminating nul */
   return s;
   }
