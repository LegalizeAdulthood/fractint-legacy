/* bignum.h */
/* Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer */

#ifndef _BIGINIT_H
#define _BIGINIT_H

#include "bignum.h"
#include "bigflt.h"




/* values that bf_math can hold, */
/* 0 = bf_math is not being used */
/* 1 = bf_math is being used     */
/*
If bf_math is used, then bigflt_t numbers are always used to
keep track of corners and center-mag.  The actual calculations
are done with either bn_t or bf_t.
*/
#define BIGNUM 1  /* bf_math is being used with bn_t numbers */
#define BIGFLT 2  /* bf_math is being used with bf_t numbers */


/* functions defined in biginit.c */

void calc_lengths(void);
void free_bf_vars(void);
bn_t alloc_stack(size_t size);
int save_stack(void);
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi(void);
void show_var_bf(char *s, bf_t n);
void show_var_bf_hex(char *s, bf_t n);

#endif
