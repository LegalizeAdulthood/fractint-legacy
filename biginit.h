/* biginit.h */
/* Used for fractint only. */
/* Many of these are redundant from big.h  */
/* but the fractint specific ones are not. */

#define MATHBITS      32
#define MATHBYTES     (MATHBITS/8)
#define NUMVARS       30            /* room for this many on stack */
#define CURRENTREZ    1
#define MAXREZ        0


/* globals */
int bnstep, bnlength, intlength, rlength, padding, shiftfactor, decimals;
int bflength, rbflength, bfshiftfactor, bfdecimals;

/* used internally by bignum.c routines */
bn_t bntmp1, bntmp2, bntmp3, bntmp4, bntmp5, bntmp6; /* rlength  */
bn_t bntmpcpy1, bntmpcpy2;                           /* bnlength */

/* used by other routines */
bn_t bnxmin, bnxmax, bnymin, bnymax, bnx3rd, bny3rd;        /* bnlength */
bn_t bnxdel, bnydel, bnxdel2, bnydel2, bnclosenuff;         /* bnlength */
bn_t bntmpsqrx, bntmpsqry, bntmp;                           /* rlength  */
_BNCMPLX bnold, /* bnnew, */ bnparm, bnsaved;               /* bnlength */
_BNCMPLX bnnew;                                              /* rlength */
bn_t bn_pi;                                           /* TAKES NO SPACE */

bf_t bftmp1, bftmp2, bftmp3, bftmp4, bftmp5, bftmp6;     /* rbflength+2 */
bf_t bftmpcpy1, bftmpcpy2;                               /* rbflength+2 */
bf_t bfxdel, bfydel, bfxdel2, bfydel2, bfclosenuff;      /* rbflength+2 */
bf_t bftmpsqrx, bftmpsqry;                               /* rbflength+2 */
_BFCMPLX /* bfold,  bfnew, */ bfparm, bfsaved;            /* bflength+2 */
_BFCMPLX bfold,  bfnew;                                  /* rbflength+2 */
bf_t bf_pi;                                           /* TAKES NO SPACE */
bf_t big_pi;                                              /* bflength+2 */

/* for testing only */

/* used by other routines */
bf_t bfxmin, bfxmax, bfymin, bfymax, bfx3rd, bfy3rd;      /* bflength+2 */
bf_t bfsxmin, bfsxmax, bfsymin, bfsymax, bfsx3rd, bfsy3rd;/* bflength+2 */
bf_t bfparms[10];                                    /* (bflength+2)*10 */
bf_t bftmp;
bf_t bf10tmp;                                              /* dec+4 */
