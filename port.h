/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
**
** XFRACT file "SHARED.H" merged into PORT.H on 3/14/92 by --CWM--
*/

#ifndef PORT_H          /* If this is defined, this file has been       */
#define PORT_H          /* included already in this module.             */

/* If endian.h is not present, it can be handled in the code below, */
/* but if you have this file, it can make it more fool proof. */
/* include <endian.h> */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321  /* to show byte order (taken from gcc) */
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#define MSDOS 1



#ifdef XFRACT           /* XFRACT forces unix configuration! --CWM-- */

 /* CAE added ltoa, overwrite fix for HP-UX v9 26Jan95  */
#ifdef _HPUX_SOURCE
#define ltoa fr_ltoa
#define overwrite fr_overwrite
#endif

#ifdef MSDOS
#undef MSDOS
#endif

#ifdef __MSDOS__
#undef __MSDOS__
#endif

#ifndef unix
#define unix
#endif

#endif  /* XFRACT  */

#ifdef __TURBOC__
   #define __cdecl cdecl

#ifndef __DOS_H
      /*  dos.h is needed for MK_FP  */
      #include <dos.h>
#endif
#endif

#ifdef MSDOS            /* Microsoft C 5.1 for OS/2 and MSDOS */
                        /* NOTE: this is always true on DOS!  */
                        /*       (MSDOS is defined above)  */

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifdef _MSC_VER         /* MSC assert does nothing under MSDOS */
#ifdef assert
#undef assert
#define assert(X)
#endif
#endif

        typedef unsigned char  U8;
        typedef signed char    S8;
        typedef unsigned short U16;
        typedef signed short   S16;
        typedef unsigned long  U32;
        typedef signed long    S32;
        typedef unsigned char  BYTE;
        typedef unsigned char  CHAR;
        typedef void          *VOIDPTR;
        typedef void far      *VOIDFARPTR;
        typedef void const    *VOIDCONSTPTR;

        #define CONST          const
        #define PRINTER        "/dev/prn"
        #define LOBYTEFIRST    1
        #define SLASHC         '\\'
        #define SLASH          "\\"
        #define SLASHSLASH     "\\\\"
        #define SLASHDOT       "\\."
        #define DOTSLASH       ".\\"
        #define DOTDOTSLASH    "..\\"
        #define READMODE        "rb"    /* Correct DOS text-mode        */
        #define WRITEMODE       "wb"    /* file open "feature".         */

        #define write1(ptr,len,n,stream) fwrite(ptr,len,n,stream)
        #define write2(ptr,len,n,stream) fwrite(ptr,len,n,stream)
        #define rand15() rand()

#else                   /* Have to nest because #elif is not portable */
#ifdef AMIGA            /* Lattice C 3.02 for Amiga */
        typedef UBYTE          U8;
        typedef BYTE           S8;
        typedef UWORD          U16;
        typedef WORD           S16;
        typedef unsigned int   U32;
        typedef int            S32;
        typedef UBYTE          BYTE;
        typedef UBYTE          CHAR;

        typedef void          *VOIDPTR;
        typedef void          *VOIDFARPTR;
        typedef void          *VOIDCONSTPTR;

        #define CONST
        #define PRINTER        "PRT:"
        #define LOBYTEFIRST    0
        #define SLASHC         '/'
        #define SLASH          "/"
        #define SLASHSLASH     "//"
        #define SLASHDOT       "/."
        #define DOTSLASH       "./"
        #define DOTDOTSLASH    "../"
        #define READMODE        "rb"
        #define WRITEMODE       "wb"

        #define write1(ptr,len,n,stream) (fputc(*(ptr),stream),1)
        #define write2(ptr,len,n,stream) (fputc((*(ptr))&255,stream),fputc((*(ptr))>>8,stream),1)
        #define rand15() (rand()&0x7FFF)

#define BYTE_ORDER BIG_ENDIAN
#define USE_BIGNUM_C_CODE
#define BIG_ANSI_C

#else
#ifdef unix                     /* Unix machine */
        typedef unsigned char  U8;
        typedef char           S8;
        typedef unsigned short U16;
        typedef short          S16;
        typedef unsigned long  U32;
        typedef long           S32;
        typedef unsigned char  BYTE;
        typedef char           CHAR;
#define __cdecl

#ifdef BADVOID
        typedef char          *VOIDPTR;
        typedef char          *VOIDFARPTR;
        typedef char          *VOIDCONSTPTR;
#else
        typedef void          *VOIDPTR;
        typedef void          *VOIDFARPTR;
        typedef void          *VOIDCONSTPTR;
#endif

#ifndef BYTE_ORDER
/* change for little endians that don't have this defined elsewhere (endian.h) */
#ifdef linux
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN /* the usual case */
#endif
#endif

#ifndef USE_BIGNUM_C_CODE
#define USE_BIGNUM_C_CODE
#endif
#ifndef BIG_ANSI_C
#define BIG_ANSI_C
#endif

#       define CONST
#       define PRINTER        "/dev/lp"
#       define SLASHC         '/'
#       define SLASH          "/"
#       define SLASHSLASH     "//"
#       define SLASHDOT       "/."
#       define DOTSLASH       "./"
#       define DOTDOTSLASH    "../"
#       define READMODE "r"
#       define WRITEMODE        "w"

#       define write1(ptr,len,n,stream) (fputc(*(ptr),stream),1)
#       define write2(ptr,len,n,stream) (fputc((*(ptr))&255,stream),fputc((*(ptr))>>8,stream),1)
#       define rand15() (rand()&0x7FFF)

#       include "unix.h"


#endif
#endif
#endif

/* Uses big_access32(), big_set32(),... functions instead of macros. */
/* Some little endian machines may require this as well. */
#if BYTE_ORDER == BIG_ENDIAN
#define ACCESS_BY_BYTE
#endif


#ifdef LOBYTEFIRST
#define GET16(c,i)              (i) = *((U16*)(&(c)))
#else
#define GET16(c,i)              (i) = (*(unsigned char *)&(c))+\
                                ((*((unsigned char*)&(c)+1))<<8)
#endif


/* The following should work regardless of machine type */
#include <float.h>

/* Some compiler libraries don't correctly handle long double.*/
/* If you want to force the use of doubles, or                */
/* if the compiler supports long doubles, but does not allow  */
/*   scanf("%Lf", &longdoublevar);                            */
/* to read in a long double, then uncomment this next line    */
/* #define DO_NOT_USE_LONG_DOUBLE */
/* #define USE_BIGNUM_C_CODE */  /* ASM code requires using long double */


/* HP-UX support long doubles and allows them to be read in with  */
/*   scanf(), but does not support the functions sinl, cosl, fabsl, etc.  */
/* CAE added this 26Jan95 so it would compile (altered by Wes to new macro) */
#ifdef _HPUX_SOURCE
#define DO_NOT_USE_LONG_DOUBLE
#endif

#ifndef DO_NOT_USE_LONG_DOUBLE
#ifdef LDBL_DIG
/* this is what we're hoping for */
#define USE_LONG_DOUBLE
        typedef long double LDBL;
#else
#define DO_NOT_USE_LONG_DOUBLE
#endif /* #ifdef LDBL_DIG */
#endif /* #ifndef DO_NOT_USE_LONG_DOUBLE */


#ifdef DO_NOT_USE_LONG_DOUBLE

#ifdef USE_LONG_DOUBLE
#undef USE_LONG_DOUBLE
#endif

/* long double isn't supported */
/* impliment LDBL as double */
        typedef double          LDBL;

/* just in case */
#undef LDBL_DIG
#undef LDBL_EPSILON
#undef LDBL_MANT_DIG
#undef LDBL_MAX
#undef LDBL_MAX_10_EXP
#undef LDBL_MAX_EXP
#undef LDBL_MIN
#undef LDBL_MIN_10_EXP
#undef LDBL_MIN_EXP
#undef LDBL_RADIX
#undef LDBL_ROUNDS

#define LDBL_DIG        DBL_DIG        /* # of decimal digits of precision */
#define LDBL_EPSILON    DBL_EPSILON    /* smallest such that 1.0+LDBL_EPSILON != 1.0 */
#define LDBL_MANT_DIG   DBL_MANT_DIG   /* # of bits in mantissa */
#define LDBL_MAX        DBL_MAX        /* max value */
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP /* max decimal exponent */
#define LDBL_MAX_EXP    DBL_MAX_EXP    /* max binary exponent */
#define LDBL_MIN        DBL_MIN        /* min positive value */
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP /* min decimal exponent */
#define LDBL_MIN_EXP    DBL_MIN_EXP    /* min binary exponent */
#define LDBL_RADIX      DBL_RADIX      /* exponent radix */
#define LDBL_ROUNDS     DBL_ROUNDS     /* addition rounding: near */

#define sqrtl           sqrt
#define logl            log
#define log10l          log10
#define atanl           atan
#define fabsl           fabs
#define sinl            sin
#define cosl            cos
#endif

#endif  /* PORT_H */
