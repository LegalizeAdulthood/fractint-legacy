/* */
/* PARSERFP.C  -- Part of FRACTINT fractal drawer. */
/* */
/*   By Chuck Ebbert  CompuServe [76306,1226] */
/*                     internet: 76306.1226@compuserve.com  */
/* */
/* Fast floating-point parser code.  The functions beginning with */
/*    "fStk" are in PARSERA.ASM.  PARSER.C calls this code after */
/*    it has parsed the formula. */
/* */
/*   Converts the function pointers/load pointers/store pointers */
/*       built by parsestr() into an optimized array of function */
/*       pointer/operand pointer pairs. */
/* */
/* ******************************************************************* */
/*                                                                     */
/*  Copyright (C) 1992, 1993 Chuck Ebbert.  All rights reserved.       */
/*                                                                     */
/*    This code may be freely distributed and used in non-commercial   */
/*    programs provided the author is credited either during program   */
/*    execution or in the documentation, and this copyright notice     */
/*    is left intact.  Sale of this code, or its use in any commercial */
/*    product requires permission from the author.  Nominal            */
/*    distribution and handling fees may be charged by shareware and   */
/*    freeware distributors.                                           */
/*                                                                     */
/*       Chuck Ebbert                                                  */
/*       1915 Blust Ln.                                                */
/*       Enola, PA  17025                                              */
/*                                                                     */
/* ******************************************************************* */
/* */
/* Revised 12 July 1993 (for v18.1) by CAE to fix optimizer bug  */
/* */
/* Revised 22 MAR 1993 (for Fractint v18.0)  */
/* */
/* Uncomment the next line to enable debug. */
/*      #define TESTFP 1  */
/* */

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include "mpmath.h"
#include "prototyp.h"

extern union Arg *Arg1, *Arg2;
/* Some of these variables should be renamed for safety */
extern union Arg s[20], far * far *Store, far * far *Load;
extern int StoPtr, LodPtr, OpPtr;
extern int debugflag;
extern unsigned vsp, LastOp;
extern struct ConstArg far *v;
extern int inside; 	    /* "inside" color to use    */
extern int outside;	    /* "outside" color to use   */
extern int potflag;	    /* potential enabled? */
extern char useinitorbit;
extern int InitLodPtr, InitStoPtr, InitOpPtr, LastInitOp;
extern void (far * far *f)(void);

struct fls { /* function, load, store pointers  CAE fp */
   void (near *function)(void);
   union Arg near *operand;
} far *pfls = (struct fls far *)0;

void  StkLod(void);
void  StkClr(void);
void  dStkAdd(void);
void  dStkSub(void);
void  dStkMul(void);
void  dStkDiv(void);
void  StkSto(void);
void  dStkSqr(void);
void  EndInit(void);
void  dStkMod(void);
void  dStkLTE(void);
void  dStkSin(void);
void  dStkCos(void);
void  dStkSinh(void);
void  dStkCosh(void);
void  dStkCosXX(void);
void  dStkTan(void);
void  dStkTanh(void);
void  dStkCoTan(void);
void  dStkCoTanh(void);
void  dStkLog(void);
void  dStkExp(void);
void  dStkPwr(void);
void  dStkLT(void);
void  dStkFlip(void);
void  dStkReal(void);
void  dStkImag(void);
void  dStkConj(void);
void  dStkNeg(void);
void  dStkAbs(void);
void  dStkRecip(void);
void  StkIdent(void);
void  dStkGT(void);
void  dStkGTE(void);
void  dStkNE(void);
void  dStkEQ(void);
void  dStkAND(void);
void  dStkOR(void);

#define fgf(x) pfls[(x)].function
#define opp(x) pfls[(x)].operand
#define NO_OPERAND (void near *)0
#define LastSqr v[4].a
#define MAX_ARGS 100
#define MAX_STACK 8
#define TWO_FREE 6

#ifndef XFRACT

void (near fStkPull2 )(void); /* pull up fpu stack from 2 to 4 */
void (near fStkPush2 )(void); /* push down fpu stack from 8 to 6 */
void (near fStkPush2a )(void); /* push down fpu stack from 6 to 4 */
void (near fStkPush4 )(void); /* push down fpu stack from 8 to 4 */
void (near fStkLodDup )(void); /* lod, dup */
void (near fStkLodSqr )(void); /* lod, sqr, dont save magnitude */
void (near fStkLodSqr2 )(void); /* lod, sqr, save magnitude */
void (near fStkStoDup )(void); /* store, duplicate */
void (near fStkStoSqr )(void); /* store, sqr, save lastsqr */
void (near fStkStoSqr0 )(void); /* store, sqr, dont save lastsqr */
void (near fStkLodDbl )(void); /* load, double */
void (near fStkStoDbl )(void); /* store, double */
void (near fStkReal2 )(void); /* fast ver. of real */
void (near fStkSqr )(void); /* sqr, save magnitude in lastsqr */
void (near fStkSqr0 )(void); /* sqr, no save magnitude */
void (near fStkClr1 )(void); /* clear fpu */
void (near fStkClr2 )(void); /* test stack top, clear fpu */
void (near fStkStoClr1 )(void); /* store, clr1 */
void (near fStkAdd )(void);
void (near fStkSub )(void);
void (near fStkSto )(void);
void (near fStkSto2 )(void); /* fast ver. of sto */
void (near fStkLod )(void);
void (near fStkEndInit )(void);
void (near fStkMod )(void);
void (near fStkMod2 )(void);
void (near fStkLodMod2 )(void);
void (near fStkStoMod2 )(void);
void (near fStkLTE )(void);
void (near fStkLodLTEMul )(void);
void (near fStkLTE2 )(void);
void (near fStkLodLTE )(void);
void (near fStkLodLTE2 )(void);
void (near fStkLodLTEAnd2 )(void);
void (near fStkLT )(void);
void (near fStkLodLTMul )(void);
void (near fStkLT2 )(void);
void (near fStkLodLT )(void);
void (near fStkLodLT2 )(void);
void (near fStkGTE )(void);
void (near fStkLodGTE )(void);
void (near fStkLodGTE2 )(void);
void (near fStkGT )(void);
void (near fStkGT2 )(void);
void (near fStkLodGT )(void);
void (near fStkLodGT2 )(void);
void (near fStkEQ )(void);
void (near fStkLodEQ )(void);
void (near fStkNE )(void);
void (near fStkLodNE )(void);
void (near fStkAND )(void);
void (near fStkANDClr2 )(void);
void (near fStkOR )(void);
void (near fStkSin )(void);
void (near fStkSinh )(void);
void (near fStkCos )(void);
void (near fStkCosXX )(void);
void (near fStkCosh )(void);
void (near fStkTan )(void);
void (near fStkTanh )(void);
void (near fStkCoTan )(void);
void (near fStkCoTanh )(void);
void (near fStkLog )(void);
void (near fStkExp )(void);
void (near fStkPwr )(void);
void (near fStkMul )(void);
void (near fStkDiv )(void);
void (near fStkFlip )(void);
void (near fStkReal )(void);
void (near fStkImag )(void);
void (near fStkRealFlip )(void);
void (near fStkImagFlip )(void);
void (near fStkConj )(void);
void (near fStkNeg )(void);
void (near fStkAbs )(void);
void (near fStkRecip )(void);
void (near fStkLodReal )(void);
void (near fStkLodRealC )(void);
void (near fStkLodImag )(void);
void (near fStkLodRealFlip )(void);
void (near fStkLodRealAbs )(void);
void (near fStkLodRealMul )(void);
void (near fStkLodRealAdd )(void);
void (near fStkLodRealSub )(void);
void (near fStkLodImagFlip )(void);
void (near fStkLodImagAbs )(void);
void (near fStkLodConj )(void);
void (near fStkLodAdd )(void);
void (near fStkLodSub )(void);
void (near fStkLodSubMod )(void);
void (near fStkLodMul )(void);
void (near fStkPLodAdd )(void);
void (near fStkPLodSub )(void);
void (near Img_Setup )(void);


static void (near *prevfptr )(void);
static int stkcnt, prevstkcnt, cvtptrx, prevlodptr, lastsqrused;

static void CvtFptr(void (near * ffptr)(void), int MinStk, int MaxStk,
      int Delta )
/* (MinStk <= 4, MaxStk >= TWO_FREE) */
{
   char testconst = 0;

   if (stkcnt < MinStk ) { /* not enough operands on fpu stack */
#ifdef TESTFP
      stopmsg(0, "Inserted pull." );
#endif
      opp(cvtptrx) = NO_OPERAND;
      fgf(cvtptrx++) = fStkPull2;  /* so adjust the stack, pull operand */
      stkcnt += 2;
   }
   else if (stkcnt > MaxStk ) { /* too many operands */
#ifdef TESTFP
      stopmsg(0, "Inserted push." );
#endif
      opp(cvtptrx) = NO_OPERAND;
      fgf(cvtptrx++) = fStkPush2;  /* push operand to adjust stack */
      stkcnt -= 2;
   }

   /* set the operand pointer here */
   if (ffptr == fStkSto ){ /* this must be before test for load */
      opp(cvtptrx) = (void near *)(Store[StoPtr++]);
   }
   else if (ffptr == fStkLod && debugflag == 322 ){
      /* if disabling optimizer, set load pointer here */
      opp(cvtptrx) = (void near *)(Load[LodPtr++]);
   }
   else {
      opp(cvtptrx) = NO_OPERAND;
   }

   if (debugflag == 322 ){
      goto SkipOptimizer;
   } /* --------------------------  begin optimizer  -------------- */

   /* For the following: */
   /*   * == cvtptrx points to this */
   /*  () == this is about to be added to the array */

   if (ffptr == fStkLod) { /* we are about to add Lod to the array */
      if (prevfptr == fStkLod && Load[LodPtr-1] == Load[LodPtr] ) {
         /* previous non-adjust operator was Lod of same operand */
         /* i.e. found {? lodx ? (*lodx) } */
         if (fgf(--cvtptrx) == fStkPush2 ){ /* prev fn was push */
            /* {? lod *push (lod) } */
            --cvtptrx; /* found {? *lod push (lod) } */
            if (fgf(cvtptrx-1) == fStkPush2){ /* always more ops here */
#ifdef TESTFP
               stopmsg(0, "push *lod push (lod) -> push4 (*loddup)" );
#endif
               fgf(cvtptrx-1) = fStkPush4;
            }
            else { /* prev op not push */
#ifdef TESTFP
               stopmsg(0, "op *lod push (lod) -> op pusha(p=0) (*loddup)" );
#endif
               opp(cvtptrx) = NO_OPERAND; /* use 'alternate' push fn. */
               fgf(cvtptrx++) = fStkPush2a; /* push with TWO_FREE on stack */
               /* operand ptr will be set below */
            }
         }
         else {  /* never {push *lod (lod) } so must be */
#ifdef TESTFP
            stopmsg(0, "op *lod (lod) -> op (*loddup)" );
#endif
         }
         ffptr = fStkLodDup;
      }
      else if (prevfptr == fStkSto2
               && Store[StoPtr-1] == Load[LodPtr] ){
         /* store, load of same value */
         /* only one operand on stack here when prev oper is Sto2 */
         --cvtptrx;
#ifdef TESTFP
         stopmsg(0, "*sto2 (lod) -> (*stodup)" );
#endif
         ffptr = fStkStoDup;
      }
      /* This may cause roundoff problems when later operators */
      /*  use the rounded value that was stored here, while the next */
      /*  operator uses the more accurate internal value. */
      else if (prevfptr == fStkStoClr1 && prevstkcnt == 2
               && Store[StoPtr-1] == Load[LodPtr] ){
         /* store, clear, load same value found */
         /* only one operand was on stack so this is safe */
         --cvtptrx;
#ifdef TESTFP
         stopmsg (0, "*StoClr1 (Lod) -> (*Sto2)" );
#endif
         ffptr = fStkSto2; /* use different Sto fn */
      }
      else {
         /* the really awful hack below gets the first char of the name */
         /*    of the variable being loaded */
         testconst = **(((char * far *)Load[LodPtr] ) - 2 );
         if ( !isalpha(testconst) && Load[LodPtr]->d.y == 0.0 ){
            /* if first character not alpha, the var is a constant */
#ifdef TESTFP
            stopmsg (0, "(*lod) -> (*lodrealc)" );
#endif
            ffptr = fStkLodRealC; /* a real const is being loaded */
         }
      }
      /* set the operand ptr here */
      opp(cvtptrx) = (void near *)(Load[LodPtr++]);
   }
   else if (ffptr == fStkAdd ){
      if (prevfptr == fStkLodDup ){ /* there is never a push before add */
         --cvtptrx; /* found {? *loddup (add) } */
         if (cvtptrx>0 && fgf(cvtptrx-1) == fStkPush2a ){
            /* because {push lod lod } impossible so is {push loddup } */
#ifdef TESTFP
            stopmsg (0, "pusha *loddup (add) -> (*loddbl),stk+=2" );
#endif
            --cvtptrx;
            opp(cvtptrx) = opp(cvtptrx+1); /* fix opptr */
            stkcnt += 2;
         }
         else if (cvtptrx>0 && fgf(cvtptrx-1) == fStkPush4 ){
#ifdef TESTFP
            stopmsg (0, "push4 *loddup (add) -> push2 (*loddbl),stk+=2" );
#endif
            fgf(cvtptrx-1) = fStkPush2;
            stkcnt += 2;  /*  CAE added 12 July 1993 to fix bug  */
         }
         else {
#ifdef TESTFP
            stopmsg (0, "op *loddup (add) -> {op (*loddbl)" );
#endif
         }
         ffptr = fStkLodDbl;
      }
      else if (prevfptr == fStkStoDup ){
#ifdef TESTFP
         stopmsg (0, "stodup (add) -> (stodbl)" );
#endif
         /* there are always exactly 4 on stack here */
         --cvtptrx;
         ffptr = fStkStoDbl;
      }
      else if (prevfptr == fStkLod ){ /* have found {lod (*add) } */
         --cvtptrx;	/* {? *lod (add) } */
         if (fgf(cvtptrx-1) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "*push load (add) -> (*plodadd),stk+=2" );
#endif
            --cvtptrx;
            stkcnt += 2; /* eliminated a push */
            opp(cvtptrx) = opp(cvtptrx+1); /* fix opptrs */
            ffptr = fStkPLodAdd;
         }
         else {
#ifdef TESTFP
            stopmsg (0, "op *lod (add) -> op (*lodadd)" );
#endif
            ffptr = fStkLodAdd;
         }
      }
      else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC ){
         --cvtptrx; /* found {? *lodreal (add) } */
         if (fgf(cvtptrx-1) == fStkPush2 ){
#ifdef TESTFP
         stopmsg (0, "*push lodreal (add) -> (*lodrealadd),stk+=2" );
#endif
            --cvtptrx;
            stkcnt += 2;	/* push eliminated */
            opp(cvtptrx) = opp(cvtptrx+1); /* fix opptrs */
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lodreal (add) -> (*lodrealadd)" );
#endif
         }
         ffptr = fStkLodRealAdd;
      }
   }
   else if (ffptr == fStkSub ){
      if (prevfptr == fStkLod ){
         /* found {lod (*sub) } */
         --cvtptrx; /* {*lod (sub) } */
         /* there is never a sequence (lod push sub ) */
         if (fgf(cvtptrx-1) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "*push lod (sub) -> (*plodsub),stk+=2" );
#endif
            --cvtptrx;
            opp(cvtptrx) = opp(cvtptrx+1); /* fix opptrs */
            stkcnt += 2; /* push was deleted so adj. stkcnt */
            ffptr = fStkPLodSub;
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lod (sub) -> (*lodsub)" );
#endif
            ffptr = fStkLodSub;
         }
      }
      else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC ){
         --cvtptrx; /* {? *lodreal (sub) } */
         if (fgf(cvtptrx-1) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "*push lodreal (sub) -> (*lodrealsub),stk+=2" );
#endif
            --cvtptrx;
            stkcnt += 2;	/* push eliminated */
            opp(cvtptrx) = opp(cvtptrx+1); /* fix opptrs */
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lodreal (sub) -> (*lodrealsub)" );
#endif
         }
         ffptr = fStkLodRealSub;
      }
   }
   else if (ffptr == fStkMul ){
      if (prevfptr == fStkLodDup ){
         /* found {loddup ? (*mul) } */
         if (fgf(--cvtptrx) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "loddup *push (mul) -> (*lodsqr),stk+=2" );
#endif
            stkcnt += 2; /* eliminate this push */
            --cvtptrx;	/* prev is a LodDup */
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*loddup (mul) -> (*lodsqr)" );
#endif
         }
         ffptr = fStkLodSqr;
      }
      else if (prevfptr == fStkStoDup ){ /* no pushes here, 4 on stk. */
#ifdef TESTFP
         stopmsg (0, "stodup (mul) -> (*stosqr0)" );
#endif
         --cvtptrx;
         ffptr = fStkStoSqr0; /* dont save lastsqr here ever */
      }
      else if (prevfptr == fStkLod ){
         --cvtptrx; /* {lod *? (mul) } */
         if (fgf(cvtptrx) == fStkPush2 ){ /* {lod *push (mul) } */
            --cvtptrx; /* {? *lod push (mul) } */
            if(fgf(cvtptrx-1) == fStkPush2 ){
#ifdef TESTFP
               stopmsg (0, "push *lod push (mul) -> push4 (*lodmul)" );
#endif
               fgf(cvtptrx-1) = fStkPush4;
            }
            else {
#ifdef TESTFP
               stopmsg (0, "op *lod push (mul) -> op pusha (*lodmul)" );
#endif
               opp(cvtptrx+1) = opp(cvtptrx); /* fix operand ptr */
               fgf(cvtptrx) = fStkPush2a;
               opp(cvtptrx++) = NO_OPERAND;
            }
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lod (mul) -> (*lodmul)" );
#endif
         }
         ffptr = fStkLodMul;
      }
      else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC ){
         --cvtptrx; /* found {lodreal *? (mul) } */
         if (fgf(cvtptrx) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "lodreal *push2 (mul) -> (*lodrealmul),stk+=2" );
#endif
            --cvtptrx;
            stkcnt += 2;	/* stack ends with TWO_FREE, not 4 */
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lodreal (mul) }-> {(*lodrealmul)" );
#endif
         }
         ffptr = fStkLodRealMul;
      }
      else if (prevfptr == fStkLodLT && fgf(cvtptrx-1) != fStkPull2 ){
         /* this shortcut fails if {Lod LT Pull Mul } found */
#ifdef TESTFP
         stopmsg (0, "LodLT (*Mul) -> (*LodLTMul)" );
#endif
         --cvtptrx; /* never { lod LT Push Mul } here */
         ffptr = fStkLodLTMul;
      }
      else if (prevfptr == fStkLodLTE && fgf(cvtptrx-1) != fStkPull2 ){
#ifdef TESTFP
         stopmsg (0, "LodLTE (*mul) -> (*LodLTEmul)" );
#endif
         --cvtptrx;
         ffptr = fStkLodLTEMul;
      }
   }
   else if (ffptr == fStkClr1 && prevfptr == fStkSto ){
#ifdef TESTFP
         stopmsg (0, "sto (*clr1) -> (*stoclr1)" );
#endif
      --cvtptrx;
      ffptr = fStkStoClr1;
   }
   else if (ffptr == fStkDiv ){
      if (prevfptr == fStkLodRealC && vsp < MAX_ARGS - 1 ){
         /* have found a divide by a real constant */
         /*  and there is space to create a new one */
         /* {lodrealc ? (*div) } */
         --cvtptrx; /* change '/ const' to '* 1/const' */
         if (fgf(cvtptrx) == fStkPush2 ){
#ifdef TESTFP
            stopmsg (0, "lodrealc *push (div) -> (*lodrealmul),stk+=2" );
#endif
            --cvtptrx;
            stkcnt += 2;
         }
         else {
#ifdef TESTFP
            stopmsg (0, "*lodrealc (div) -> {(*lodrealmul)" );
#endif
         }
         v[vsp].s = (void near *)0; /* this constant has no name */
         v[vsp].len = 0;
         v[vsp].a.d.x = 1.0 / Load[LodPtr-1]->d.x;
         v[vsp].a.d.y = 0.0;
         opp(cvtptrx) = (void near *)&v[vsp++].a; /* isn't C fun! */
         ffptr = fStkLodRealMul;
      }
   }
   else if (ffptr == fStkReal ){
      if (prevfptr == fStkLod ){
#ifdef TESTFP
         stopmsg (0, "lod (*real) -> (*lodreal)" );
#endif
         --cvtptrx;
         ffptr = fStkLodReal;
      }
      else if (stkcnt < MAX_STACK ){
#ifdef TESTFP
         stopmsg (0, "(*real) -> (*real2)" );
#endif
         ffptr = fStkReal2;
      }
   }
   else if (ffptr == fStkImag && prevfptr == fStkLod ){
#ifdef TESTFP
         stopmsg (0, "lod (*imag) -> lodimag" );
#endif
      --cvtptrx;
      ffptr = fStkLodImag;
   }
   else if (ffptr == fStkConj && prevfptr == fStkLod ){
#ifdef TESTFP
         stopmsg (0, "lod (*conj) -> (*lodconj)" );
#endif
      --cvtptrx;
      ffptr = fStkLodConj;
   }
   else if (ffptr == fStkMod && stkcnt < MAX_STACK ){
#ifdef TESTFP
      stopmsg (0, "(*mod) -> (*mod2)" );
#endif
      ffptr = fStkMod2; /* use faster version if room on stack */
      if (prevfptr == fStkLod ){
#ifdef TESTFP
         stopmsg (0, "lod (*mod2) -> (*lodmod2)" );
#endif
         --cvtptrx;
         ffptr = fStkLodMod2;
      }
      else if (prevfptr == fStkSto || prevfptr == fStkSto2 ){
#ifdef TESTFP
         stopmsg (0, "sto (*mod2) -> (*stomod2)" );
#endif
         --cvtptrx;
         ffptr = fStkStoMod2;
      }
      else if (prevfptr == fStkLodSub ){
#ifdef TESTFP
         stopmsg (0, "lodsub (*mod2) -> (*lodsubmod)" );
#endif
         --cvtptrx;
         ffptr = fStkLodSubMod;
      }
   }
   else if (ffptr == fStkFlip ){
      if (prevfptr == fStkReal || prevfptr == fStkReal2 ){
#ifdef TESTFP
         stopmsg (0, "real (*flip) -> (*realflip)" );
#endif
         --cvtptrx;
         ffptr = fStkRealFlip;
      }
      else if (prevfptr == fStkImag ){
#ifdef TESTFP
         stopmsg (0, "imag (*flip) -> (*imagflip)" );
#endif
         --cvtptrx;
         ffptr = fStkImagFlip;
      }
      else if (prevfptr == fStkLodReal ){
#ifdef TESTFP
         stopmsg (0, "lodreal (*flip) -> (*lodrealflip)" );
#endif
         --cvtptrx;
         ffptr = fStkLodRealFlip;
      }
      else if (prevfptr == fStkLodImag ){
#ifdef TESTFP
         stopmsg (0, "lodimag (*flip) -> (*lodimagflip)" );
#endif
         --cvtptrx;
         ffptr = fStkLodImagFlip;
      }
   }
   else if (ffptr == fStkAbs ){
      if (prevfptr == fStkLodReal ){
#ifdef TESTFP
         stopmsg (0, "lodreal (*abs) -> (*lodrealabs)" );
#endif
         --cvtptrx;
         ffptr = fStkLodRealAbs;
      }
      else if (prevfptr == fStkLodImag ){
#ifdef TESTFP
         stopmsg (0, "lodimag (*abs) -> (*lodimagabs)" );
#endif
         --cvtptrx;
         ffptr = fStkLodImagAbs;
      }
   }
   else if (ffptr == fStkSqr ){
      if (prevfptr == fStkLod && fgf(cvtptrx-1) != fStkPush2 ){
#ifdef TESTFP
         stopmsg (0, "lod (*sqr) -> (*lodsqr)" );
#endif
         --cvtptrx;
         ffptr = fStkLodSqr; /* assume no need to save lastsqr */
         if (lastsqrused) {
#ifdef TESTFP
            stopmsg (0, "(*lodsqr) -> (*lodsqr2)" );
#endif
            ffptr = fStkLodSqr2; /* lastsqr is being used */
         }
      }
      else if (prevfptr == fStkSto2 ){
#ifdef TESTFP
         stopmsg (0, "sto2 (*sqr) -> (*stosqr0)" );
#endif
         --cvtptrx;
         ffptr = fStkStoSqr0; /* assume no need to save lastsqr */
         if (lastsqrused) {
#ifdef TESTFP
            stopmsg (0, "(*stosqr0) -> (*stosqr)" );
#endif
            ffptr = fStkStoSqr; /* save lastsqr */
         }
      }
      else {
         if (!lastsqrused) {
#ifdef TESTFP
            stopmsg (0, "(*sqr) -> (*sqr0)" );
#endif
            ffptr = fStkSqr0; /* don't save lastsqr */
         }
      }
   }
   else if (ffptr == fStkLTE && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*LTE) -> (*LodLTE)" );
#endif
      --cvtptrx;
      ffptr = fStkLodLTE;
   }
   else if (ffptr == fStkLT && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*LT) -> (*LodLT)" );
#endif
      --cvtptrx;
      ffptr = fStkLodLT;
   }
   else if (ffptr == fStkGT && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*GT) -> (*LodGT)" );
#endif
      --cvtptrx;
      ffptr = fStkLodGT;
   }
   else if (ffptr == fStkGTE && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*GTE) -> (*LodGTE)" );
#endif
      --cvtptrx;
      ffptr = fStkLodGTE;
   }
   else if (ffptr == fStkNE && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*NE) -> (*LodNE)" );
#endif
      --cvtptrx;
      ffptr = fStkLodNE;
   }
   else if (ffptr == fStkEQ && ( prevfptr == fStkLod
         || prevfptr == fStkLodReal || prevfptr == fStkLodRealC ) ){
#ifdef TESTFP
      stopmsg(0, "Lod (*EQ) -> (*LodEQ)" );
#endif
      --cvtptrx;
      ffptr = fStkLodEQ;
   }

SkipOptimizer:  /* -----------  end of optimizer code  ------------ */

   fgf(cvtptrx++) = prevfptr = ffptr;
   prevstkcnt = stkcnt;

   if (Delta == 999 ){
      stkcnt = 0;
   }
   else {
      stkcnt += Delta;
   }
   return;
}

int CvtStk() {	/* convert the array of ptrs */
   extern int fform_per_pixel(void);	/* these fns are in parsera.asm */
   extern int fFormula(void);
   extern int BadFormula(void);
   extern char FormName[];
   void (far *ftst)(void);
   void (near *ntst)(void);

#ifdef TESTFP
   if (debugflag == 322) {
      stopmsg(0, "Skipping optimizer." );
   }
#endif

   for (OpPtr = LodPtr = lastsqrused = 0; OpPtr < LastOp; OpPtr++) {
      ftst = f[OpPtr];
      if (ftst == StkLod && Load[LodPtr++] == &LastSqr ){
         lastsqrused = 1; /* lastsqr is being used in the formula */
      }
      if (  ftst != StkLod  /* these are the supported parser fns */
            && ftst != StkClr
            && ftst != dStkAdd
            && ftst != dStkSub
            && ftst != dStkMul
            && ftst != dStkDiv
            && ftst != StkSto
            && ftst != dStkSqr
            && ftst != EndInit
            && ftst != dStkMod
            && ftst != dStkLTE
            && ftst != dStkSin
            && ftst != dStkCos
            && ftst != dStkSinh
            && ftst != dStkCosh
            && ftst != dStkCosXX
            && ftst != dStkTan
            && ftst != dStkTanh
            && ftst != dStkCoTan
            && ftst != dStkCoTanh
            && ftst != dStkLog
            && ftst != dStkExp
            && ftst != dStkPwr
            && ftst != dStkLT
            && ftst != dStkFlip
            && ftst != dStkReal
            && ftst != dStkImag
            && ftst != dStkConj
            && ftst != dStkNeg
            && ftst != dStkAbs
            && ftst != dStkRecip
            && ftst != StkIdent
            && ftst != dStkGT
            && ftst != dStkGTE
            && ftst != dStkNE
            && ftst != dStkEQ
            && ftst != dStkAND
            && ftst != dStkOR ){
         stopmsg(0, "Fast failure, using old code" );
         return 1; /* Use old code */
      }
   }

#ifdef TESTFP
   if (lastsqrused) {
      stopmsg(0, "LastSqr used" );
   }
   else {
      stopmsg(0, "LastSqr not used" );
   }
#endif

   if (f[LastOp-1] != StkClr ){ /* some formulas don't clear at the end! */
#ifdef TESTFP
      stopmsg (0, "clr added at end" );
#endif
      f[LastOp++] = StkClr;
   }
   prevfptr = (void (near *)(void))0;
   prevstkcnt = 999; /* there was not previous stk cnt */
   stkcnt = cvtptrx = 0;

   for (OpPtr = LodPtr = StoPtr = 0; OpPtr < LastOp; OpPtr++) {
      ftst = f[OpPtr];
      if (ftst == StkLod ) {
#ifdef TESTFP
         stopmsg(0, "lod,0,TWO_FREE,2" );
#endif
         CvtFptr(fStkLod, 0, TWO_FREE, 2 );
         continue;
      }
      if (ftst == StkClr ) {
         if (OpPtr == LastOp - 1 ){
#ifdef TESTFP
            stopmsg(0, "clr2,0,MAX_STACK,999" );
#endif
            CvtFptr(fStkClr2, 0, MAX_STACK, 999 );
         } else {
#ifdef TESTFP
            stopmsg(0, "clr1,0,MAX_STACK,999" );
#endif
            CvtFptr(fStkClr1, 0, MAX_STACK, 999 );
         }
         continue;
      }
      if (ftst == dStkAdd ) {
#ifdef TESTFP
         stopmsg(0, "add,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkAdd, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkSub ) {
#ifdef TESTFP
         stopmsg(0, "sub,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkSub, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkDiv ) {
#ifdef TESTFP
         stopmsg(0, "div,4,TWO_FREE,-2" );
#endif
         CvtFptr(fStkDiv, 4, TWO_FREE, -2 );
         continue;
      }
      if (ftst == dStkMul ) {
#ifdef TESTFP
         stopmsg(0, "mul,4,TWO_FREE,-2" );
#endif
         CvtFptr(fStkMul, 4, TWO_FREE, -2 );
         continue;
      }
      if (ftst == StkSto ) {
#ifdef TESTFP
         stopmsg(0, "sto,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkSto, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkSqr ) {
#ifdef TESTFP
         stopmsg(0, "sqr,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkSqr, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == EndInit) {
#ifdef TESTFP
         stopmsg(0, "endinit,0,MAX_STACK,999" );
#endif
         CvtFptr(fStkEndInit, 0, MAX_STACK, 999 );
         continue;
      }
      if (ftst == dStkMod ) {
#ifdef TESTFP
         stopmsg(0, "mod,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkMod, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkLTE ) {
#ifdef TESTFP
         stopmsg(0, "LTE,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkLTE, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkSin ) {
#ifdef TESTFP
         stopmsg(0, "sin,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkSin, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkCos ) {
#ifdef TESTFP
         stopmsg(0, "cos,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkCos, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkSinh ) {
#ifdef TESTFP
         stopmsg(0, "sinh,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkSinh, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkCosh ) {
#ifdef TESTFP
         stopmsg(0, "cosh,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkCosh, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkCosXX ) {
#ifdef TESTFP
         stopmsg(0, "cosxx,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkCosXX, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkTan ) {
#ifdef TESTFP
         stopmsg(0, "tan,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkTan, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkTanh ) {
#ifdef TESTFP
         stopmsg(0, "tanh,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkTanh, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkCoTan ) {
#ifdef TESTFP
         stopmsg(0, "cotan,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkCoTan, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkCoTanh ) {
#ifdef TESTFP
         stopmsg(0, "cotanh,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkCoTanh, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkExp ) {
#ifdef TESTFP
         stopmsg(0, "exp,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkExp, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkLog ) {
#ifdef TESTFP
         stopmsg(0, "log,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkLog, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == dStkPwr ) {
#ifdef TESTFP
         stopmsg(0, "pwr,4,TWO_FREE,-2" );
#endif
         CvtFptr(fStkPwr, 4, TWO_FREE, -2 );
         continue;
      }
      if (ftst == dStkLT ) {
#ifdef TESTFP
         stopmsg(0, "LT,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkLT, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkFlip ) {
#ifdef TESTFP
         stopmsg(0, "flip,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkFlip, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkReal ) {
#ifdef TESTFP
         stopmsg(0, "real,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkReal, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkImag ) {
#ifdef TESTFP
         stopmsg(0, "imag,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkImag, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkConj ) {
#ifdef TESTFP
         stopmsg(0, "conj,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkConj, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkNeg ) {
#ifdef TESTFP
         stopmsg(0, "neg,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkNeg, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkAbs ) {
#ifdef TESTFP
         stopmsg(0, "abs,2,MAX_STACK,0" );
#endif
         CvtFptr(fStkAbs, 2, MAX_STACK, 0 );
         continue;
      }
      if (ftst == dStkRecip ) {
#ifdef TESTFP
         stopmsg(0, "recip,2,TWO_FREE,0" );
#endif
         CvtFptr(fStkRecip, 2, TWO_FREE, 0 );
         continue;
      }
      if (ftst == StkIdent ) {
#ifdef TESTFP
         stopmsg(0, "ident skipped" );
#endif
         /* don't bother converting this one */
         continue;
      }
      if (ftst == dStkGT ) {
#ifdef TESTFP
         stopmsg(0, "GT,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkGT, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkGTE ) {
#ifdef TESTFP
         stopmsg(0, "GTE,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkGTE, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkEQ ) {
#ifdef TESTFP
         stopmsg(0, "EQ,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkEQ, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkNE ) {
#ifdef TESTFP
         stopmsg(0, "NE,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkNE, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkOR ) {
#ifdef TESTFP
         stopmsg(0, "OR,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkOR, 4, MAX_STACK, -2 );
         continue;
      }
      if (ftst == dStkAND ) {
#ifdef TESTFP
         stopmsg(0, "AND,4,MAX_STACK,-2" );
#endif
         CvtFptr(fStkAND, 4, MAX_STACK, -2 );
         continue;
      }
      stopmsg(0, "Fast failure, using old code." );
      return 1; /* this should never happen but is not fatal now */
   }

   if (debugflag == 322 ){
      /* skip these optimizations too */
      goto skipfinalopt;
   } /* ---------------------------------- final optimizations ---- */

   ntst = fgf(cvtptrx-2 ); /* cvtptrx -> one past last operator (clr2) */
   if (ntst == fStkLT ){
#ifdef TESTFP
      stopmsg (0, "LT Clr2 -> LT2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLT2;
   }
   else if (ntst == fStkLodLT ){
#ifdef TESTFP
      stopmsg (0, "LodLT Clr2 -> LodLT2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLodLT2;
   }
   else if (ntst == fStkLTE ){
#ifdef TESTFP
      stopmsg (0, "LTE Clr2 -> LTE2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLTE2;
   }
   else if (ntst == fStkLodLTE ){
#ifdef TESTFP
      stopmsg (0, "LodLTE Clr2 -> LodLTE2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLodLTE2;
   }
   else if (ntst == fStkGT ){
#ifdef TESTFP
      stopmsg (0, "GT Clr2 -> GT2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkGT2;
   }
   else if (ntst == fStkLodGT ){
#ifdef TESTFP
      stopmsg (0, "LodGT Clr2 -> LodGT2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLodGT2;
   }
   else if (ntst == fStkLodGTE ){
#ifdef TESTFP
      stopmsg (0, "LodGTE Clr2 -> LodGTE2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkLodGTE2;
   }
   else if (fgf(cvtptrx-2 ) == fStkAND ){
#ifdef TESTFP
      stopmsg (0, "AND Clr2 -> ANDClr2" );
#endif
      --cvtptrx;
      fgf(cvtptrx-1) = fStkANDClr2;
      ntst = fgf(cvtptrx-2);
      if (ntst == fStkLodLTE ){
#ifdef TESTFP
         stopmsg (0, "LodLTE ANDClr2 -> LodLTEAnd2" );
#endif
         --cvtptrx;
         fgf(cvtptrx-1) = fStkLodLTEAnd2;
      }
   }

skipfinalopt: /* ---------------- end of final optimizations ----- */

   LastOp = cvtptrx; /* save the new operator count */
   LastSqr.d.y = 0.0; /* do this once per image */

   /* now change the pointers */
   if (FormName[0] != 0 ){ /* but only if parse succeeded */
      curfractalspecific->per_pixel = fform_per_pixel;
      curfractalspecific->orbitcalc = fFormula;
   }
   else {
      curfractalspecific->per_pixel = BadFormula;
      curfractalspecific->orbitcalc = BadFormula;
   }
   Img_Setup(); /* call assembler setup code */
   return 1;
}

#endif  /*  XFRACT  */
