/* bigflt.h */
/* Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer */

#ifndef _BIGFLT_H
#define _BIGFLT_H

#ifndef PORT_H
#include "port.h"
#endif

#define CALCULATING_BIG_PI 0  /* change to 1 for generating big_pi[] table */

#if 0
#define bf_t bn_t  /* for clarity in identifying one or the other */
#endif

typedef bn_t bf_t;  /* for clarity in identifying one or the other */

extern int bflength, rbflength, bfshiftfactor, bfdecimals;

extern bf_t bftmp1, bftmp2, bftmp3, bftmp4, bftmp5, bftmp6;  /* rbflength+2 */
extern bf_t bftest1, bftest2, bftest3;                       /* rbflength+2 */
extern bf_t bftmpcpy1, bftmpcpy2;                            /* bflength+2  */
extern bf_t bfxmin, bfxmax, bfymin, bfymax, bfx3rd, bfy3rd;  /* bflength+2 */
extern bf_t bfsxmin,bfsxmax,bfsymin,bfsymax,bfsx3rd,bfsy3rd; /* bflength+2 */
extern bf_t bfxdel, bfydel, bfxdel2, bfydel2;                /* bflength+2 */
extern bf_t bfparms[];                                  /* (bflength+2)*10 */
extern bf_t bf_pi;


extern bf_t bfxdel, bfydel, bfxdel2, bfydel2, bfclosenuff;      /* rbflength+2 */
extern bf_t bftmpsqrx, bftmpsqry;                               /* rbflength+2 */
extern _BFCMPLX bfold, /* bfnew, */ bfparm, bfsaved;             /* bflength+2 */
extern _BFCMPLX bfnew;                                          /* rbflength+2 */



/* bigfltc.c */
bf_t norm_bf(bf_t r);
void norm_sign_bf(bf_t r, int positive);
S16 adjust_bf_add(bf_t n1, bf_t n2);
bf_t clear_bf(bf_t r);
bf_t max_bf(bf_t r);
bf_t copy_bf(bf_t r, bf_t n);
int cmp_bf(bf_t n1, bf_t n2);
int is_bf_neg(bf_t n);
int is_bf_not_zero(bf_t n);
bf_t unsafe_add_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_add_a_bf(bf_t r, bf_t n);
bf_t unsafe_sub_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_sub_a_bf(bf_t r, bf_t n);
bf_t neg_bf(bf_t r, bf_t n);
bf_t neg_a_bf(bf_t r);
bf_t double_bf(bf_t r, bf_t n);
bf_t double_a_bf(bf_t r);
bf_t half_bf(bf_t r, bf_t n);
bf_t half_a_bf(bf_t r);
bf_t unsafe_full_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_full_square_bf(bf_t r, bf_t n);
bf_t unsafe_square_bf(bf_t r, bf_t n);
bf_t unsafe_mult_bf_int(bf_t r, bf_t n, U16 u);
bf_t mult_a_bf_int(bf_t r, U16 u);
bf_t unsafe_div_bf_int(bf_t r, bf_t n,  U16 u);
bf_t div_a_bf_int(bf_t r, U16 u);

/************/
/* bigflt.c */
void bf_hexdump(bf_t r);
bf_t strtobf(bf_t r, char *s);
int strlen_needed_bf();
char *unsafe_bftostr(char *s, int dec, bf_t r);
char *unsafe_bftostr_e(char *s, int dec, bf_t r);
char *unsafe_bftostr_f(char *s, int dec, bf_t r);
bn_t bftobn(bn_t n, bf_t f);
bn_t bntobf(bf_t f, bn_t n);
long bftoint(bf_t f);
bf_t inttobf(bf_t r, long longval);
bf_t floattobf(bf_t r, LDBL f);

int sign_bf(bf_t n);
bf_t abs_bf(bf_t r, bf_t n);
bf_t abs_a_bf(bf_t r);
bf_t unsafe_inv_bf(bf_t r, bf_t n);
bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_sqrt_bf(bf_t r, bf_t n);
bf_t exp_bf(bf_t r, bf_t n);
bf_t unsafe_ln_bf(bf_t r, bf_t n);
bf_t unsafe_sincos_bf(bf_t s, bf_t c, bf_t n);
bf_t unsafe_atan_bf(bf_t r, bf_t n);
bf_t unsafe_atan2_bf(bf_t r, bf_t ny, bf_t nx);

bf_t add_bf(bf_t r, bf_t n1, bf_t n2);
bf_t add_a_bf(bf_t r, bf_t n);
bf_t sub_bf(bf_t r, bf_t n1, bf_t n2);
bf_t sub_a_bf(bf_t r, bf_t n);
bf_t full_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t full_square_bf(bf_t r, bf_t n);
bf_t square_bf(bf_t r, bf_t n);
bf_t mult_bf_int(bf_t r, bf_t n, U16 u);
bf_t div_bf_int(bf_t r, bf_t n,  U16 u);

char *bftostr(char *s, int dec, bf_t r);
char *bftostr_e(char *s, int dec, bf_t r);
char *bftostr_f(char *s, int dec, bf_t r);
LDBL bftofloat(bf_t n);
bf_t inv_bf(bf_t r, bf_t n);
bf_t div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t sqrt_bf(bf_t r, bf_t n);
bf_t ln_bf(bf_t r, bf_t n);
bf_t sincos_bf(bf_t s, bf_t c, bf_t n);
bf_t atan_bf(bf_t r, bf_t n);
bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx);
int is_bf_zero(bf_t n);
int convert_bf(bf_t new, bf_t old, int newbflength, int oldbflength);

/****************************/
/* bigflta.asm or bigfltc.c */
LDBL extract_10(LDBL f, int *exp_ptr);
LDBL scale_10( LDBL f, int n );
LDBL extract_256(LDBL f, int *exp_ptr);
LDBL scale_256( LDBL f, int n );

#define bf10_t bn_t  /* use this for dynamic allocation */

extern bf10_t unsafe_bftobf10(bf10_t s, int dec, bf_t n);
extern bf10_t mult_a_bf10_int(bf10_t s, int dec, U16 n);
extern bf10_t div_a_bf10_int (bf10_t s, int dec, U16 n);
extern char  *bf10tostr_e(char *s, int dec, bf10_t n);
extern char  *bf10tostr_f(char *s, int dec, bf10_t n);
#endif
