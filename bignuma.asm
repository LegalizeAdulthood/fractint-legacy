; bignuma.asm - asm routines for bignumbers
; far pointer version
; Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer

; IMPORTANT!!!  The following routines assume that all the bignumber arrays
; were allocated from the same far segment.  This allows the routines to
; switch the DS register to that segment and treat everything then as a
; near pointer.  If that array is not in this segment, then copy it into
; an array that is.

; See BIGNUMC.C for further documentation.

.MODEL medium, c

ifdef ??version ; for TASM
   ideal
TYPEDEF bn_t far ptr byte
   masm
   masm51
   quirks
; invoke w/ one arg
invoke macro a,b
   call a, b
endm
; invoke with 2 args
invoke2 macro a,b,c
   call a, b, c
endm
extern equ extrn

else

bn_t TYPEDEF far ptr byte   ; far pointer to bignumber array

endif

.DATA

extern cpu:word
extern bnlength:word, rlength:word;

.CODE
.8086

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = 0
clear_bn   PROC USES di, r:bn_t
        les     di, r                   ; load pointer in es:di for stos
        mov     cx, bnlength

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        sub     ax, ax                  ; clear ax
        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     stosw                   ; clear r, word at a time

        jmp     bottom

use_32_bit:
.386
        sub     eax, eax                ; clear eax
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     stosd                   ; clear r, dword at a time

bottom:
.8086
        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret

clear_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = max positive value
max_bn   PROC USES di, r:bn_t
        les     di, r                   ; load pointer in es:di for stos
        mov     cx, bnlength

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        mov     ax, 0FFFFh              ; set ax to max value
        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     stosw                   ; max out r, word at a time

        jmp     bottom

use_32_bit:
.386
        mov     eax, 0FFFFFFFFh         ; set eax to max value
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     stosd                   ; max out r, dword at a time

bottom:
.8086
        ; when the above stos is finished, di points to the byte past the end
        dec     di                          ; find sign bit in msb
        mov     byte ptr es:[di], 7Fh       ; turn off the sign bit

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret

max_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n
copy_bn   PROC USES di si, r:bn_t, n:bn_t
        les     di, r                   ; load pointer in es:di for movs
        mov     ax, ds                  ; save ds for later
        mov     cx, bnlength

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        lds     si, n                   ; load pointer in ds:si for movs
        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     movsw                   ; copy word at a time
        jmp     bottom

use_32_bit:
.386
        lds     si, n                   ; load pointer in ds:si for movs
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     movsd                   ; copy dword at a time

bottom:
.8086
        mov     ds, ax                  ; restore ds
        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret

copy_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; n1 != n2 ?
; RETURNS: if n1 == n2 returns 0
;          if n1 > n2 returns a positive (steps left to go when mismatch occured)
;          if n1 < n2 returns a negative (steps left to go when mismatch occured)
cmp_bn   PROC USES di, n1:bn_t, n2:bn_t

        mov     cx, bnlength

        push    ds                      ; save DS
        mov     di, word ptr n2
        lds     bx, n1                  ; load pointers
        add     bx, cx                  ; point to end of bignumbers
        add     di, cx                  ; where the msb is

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        mov     dx, cx                  ; keep a copy of cx
top_loop_16:
        sub     bx, 2                   ; decrement to previous word
        sub     di, 2
        mov     ax, [bx]                ; load n1
        cmp     ax, [di]                ; compare to n2
        jne     bottom                  ; don't match
        loop    top_loop_16
        jmp     match

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword
        mov     dx, cx                  ; keep a copy of cx
top_loop_32:
        sub     bx, 4                   ; decrement to previous dword
        sub     di, 4
        mov     eax, [bx]               ; load n1
        cmp     eax, [di]               ; compare to n2
        jne     bottom                  ; don't match
        loop    top_loop_32
        jmp     match

bottom:
.8086
; flags are still set from last cmp
; if cx == dx, then most significant part didn't match, use signed comparison
; else the decimals didn't match, use unsigned comparison
        lahf                            ; load results of last cmp
        cmp     cx, dx                  ; did they differ on very first cmp
        jne     not_first_step          ; no

        sahf                            ; yes
        jg      n1_bigger               ; signed comparison
        jmp     n2_bigger

not_first_step:
        sahf
        ja      n1_bigger               ; unsigned comparison

n2_bigger:
        neg     cx                      ; make it negative
n1_bigger:                              ; leave it positive
match:                                  ; leave it zero
        mov     ax, cx
        pop     ds                      ; restore DS
        ret

cmp_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r < 0 ?
; returns 1 if negative, 0 if positive or zero
is_bn_neg   PROC n:bn_t

        ; for a one-pass routine like this, don't bother with ds
        les     bx, n                       ; find sign bit
        add     bx, bnlength
        dec     bx                          ; here it is
        mov     al, es:[bx]                 ; got it
        and     al, 80h                     ; check the sign bit
        rol     al, 1                       ; rotate sign big to bit 0
        sub     ah, ah                      ; clear upper ax
        ret

is_bn_neg   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; n != 0 ?
; RETURNS: if n != 0 returns 1
;          else returns 0
is_bn_not_zero   PROC n:bn_t

        mov     cx, bnlength
        mov     ax, ds                  ; save DS
        lds     bx, n                   ; load pointers with DS

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
top_loop_16:
        cmp     word ptr [bx], 0        ; compare to n to 0
        jnz     bottom                  ; not zero
        add     bx, 2                   ; increment to next word
        loop    top_loop_16
        jmp     bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword
top_loop_32:
        cmp     dword ptr [bx], 0       ; compare to n to 0
        jnz     bottom                  ; not zero
        add     bx, 4                   ; increment to next dword
        loop    top_loop_32
        jmp     bottom

bottom:
.8086
        mov     ds, ax                  ; restore DS
        ; if cx is zero, then n was zero
        mov     ax, cx
        ret

is_bn_not_zero   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 + n2
add_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t

        mov     cx, bnlength
        push    ds                      ; save ds
        mov     di, word ptr n1         ; load pointers ds:di
        mov     si, word ptr n2         ;               ds:si
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, [di]                ; n1
        adc     ax, [si]                ; n1+n2
        mov     [bx], ax                ; r = n1+n2

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     si                      ; add  si, 2
        inc     si
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, [di]               ; n1
        adc     eax, [si]               ; n1+n2
        mov     [bx], eax               ; r = n1+n2

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     si, 4
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086
        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
add_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r += n
add_a_bn   PROC USES di, r:bn_t, n:bn_t

        mov     cx, bnlength
        push    ds
        mov     di, word ptr n          ; load pointers ds:di
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, [di]                ; n
        adc     [bx], ax                ; r += n

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add  di, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, [di]               ; n
        adc     [bx], eax               ; r += n

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086
        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
add_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 - n2
sub_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t

        mov     cx, bnlength
        push    ds
        mov     di, word ptr n1         ; load pointers ds:di
        mov     si, word ptr n2         ;               ds:si
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, [di]                ; n1
        sbb     ax, [si]                ; n1-n2
        mov     [bx], ax                ; r = n1-n2

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     si                      ; add  si, 2
        inc     si
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, [di]               ; n1
        sbb     eax, [si]               ; n1-n2
        mov     [bx], eax               ; r = n1-n2

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     si, 4
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
sub_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r -= n
sub_a_bn   PROC USES di, r:bn_t, n:bn_t

        mov     cx, bnlength
        push    ds
        mov     di, word ptr n          ; load pointers ds:di
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386                ; check cpu
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, [di]                ; n
        sbb     [bx], ax                ; r -= n

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add  di, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, [di]               ; n
        sbb     [bx], eax               ; r -= n

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
sub_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = -n
neg_bn   PROC USES di, r:bn_t, n:bn_t
        mov     cx, bnlength
        push    ds
        mov     di, word ptr n          ; load pointers ds:di
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word

top_loop_16:
        mov     ax, [di]
        neg     ax
        mov     [bx], ax
        jc      short no_more_carry_16  ; notice the "reverse" logic here

        add     di, 2                   ; increment by word size
        add     bx, 2

        loop    top_loop_16
        jmp     short bottom

no_more_carry_16:
        add     di, 2
        add     bx, 2
        loop    top_loop_no_more_carry_16   ; jump down
        jmp     short bottom

top_loop_no_more_carry_16:
        mov     ax, [di]
        not     ax
        mov     [bx], ax

        add     di, 2
        add     bx, 2

        loop    top_loop_no_more_carry_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword

top_loop_32:
        mov     eax, [di]
        neg     eax
        mov     [bx], eax
        jc     short no_more_carry_32   ; notice the "reverse" logic here

        add     di, 4                   ; increment by double word size
        add     bx, 4

        loop    top_loop_32
        jmp     short bottom

no_more_carry_32:
        add     di, 4                   ; increment by double word size
        add     bx, 4
        loop    top_loop_no_more_carry_32   ; jump down
        jmp     short bottom

top_loop_no_more_carry_32:
        mov     eax, [di]
        not     eax
        mov     [bx], eax

        add     di, 4                   ; increment by double word size
        add     bx, 4

        loop    top_loop_no_more_carry_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
neg_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= -1
neg_a_bn   PROC r:bn_t
        mov     cx, bnlength
        push    ds
        lds     bx, r                   ; ds:bx

        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word

top_loop_16:
        neg     word ptr [bx]
        jc      short no_more_carry_16  ; notice the "reverse" logic here

        add     bx, 2

        loop    top_loop_16
        jmp     short bottom

no_more_carry_16:
        add     bx, 2
        loop    top_loop_no_more_carry_16   ; jump down
        jmp     short bottom

top_loop_no_more_carry_16:
        not     word ptr [bx]

        add     bx, 2

        loop    top_loop_no_more_carry_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword

top_loop_32:
        neg     dword ptr [bx]
        jc     short no_more_carry_32   ; notice the "reverse" logic here

        add     bx, 4

        loop    top_loop_32
        jmp     short bottom

no_more_carry_32:
        add     bx, 4
        loop    top_loop_no_more_carry_32   ; jump down
        jmp     short bottom

top_loop_no_more_carry_32:
        not     dword ptr [bx]

        add     bx, 4

        loop    top_loop_no_more_carry_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
neg_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = 2*n
double_bn   PROC USES di, r:bn_t, n:bn_t
        mov     cx, bnlength
        push    ds
        mov     di, word ptr n          ; load pointers ds:di
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc

top_loop_16:
        mov     ax, [di]
        rcl     ax, 1                   ; rotate with carry left
        mov     [bx], ax

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add bx, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, [di]
        rcl     eax, 1                  ; rotate with carry left
        mov     [bx], eax

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
double_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= 2
double_a_bn   PROC r:bn_t
        mov     cx, bnlength
        push    ds
        lds     bx, r                   ;               ds:bx

        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word
        clc

top_loop_16:
        rcl     word ptr [bx], 1        ; rotate with carry left

                                        ; inc does not change carry flag
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword
        clc                             ; clear carry flag

top_loop_32:
        rcl     dword ptr [bx], 1       ; rotate with carry left

        lahf                            ; save carry flag
        add     bx, 4
	sahf                            ; restore carry flag

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
double_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n/2
half_bn   PROC USES di, r:bn_t, n:bn_t
        mov     cx, bnlength
        push    ds
        mov     di, word ptr n          ; load pointers ds:di
        lds     bx, r                   ;               ds:bx
        add     di, cx                  ; start with msb
        add     bx, cx

        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word

        ; handle the first step with sar, the rest with rcr
        sub     di, 2
        sub     bx, 2

        mov     ax, [di]
        sar     ax, 1                   ; shift arithmetic right
        mov     [bx], ax

        loop    top_loop_16
        jmp     short bottom


top_loop_16:
                                        ; inc does not change carry flag
        dec     di                      ; sub  di, 2
        dec     di
        dec     bx                      ; sub bx, 2
        dec     bx

        mov     ax, [di]
        rcr     ax, 1                   ; rotate with carry right
        mov     [bx], ax

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword

        sub     di, 4                   ; decrement by double word size
        sub     bx, 4

        mov     eax, [di]
        sar     eax, 1                  ; shift arithmetic right
        mov     [bx], eax

        loop    top_loop_32
        jmp     short bottom

top_loop_32:
        lahf                            ; save carry flag
        sub     di, 4                   ; decrement by double word size
        sub     bx, 4
	sahf                            ; restore carry flag

        mov     eax, [di]
        rcr     eax, 1                  ; rotate with carry right
        mov     [bx], eax

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
half_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r /= 2
half_a_bn   PROC r:bn_t
        mov     cx, bnlength
        push    ds
        lds     bx, r                   ;               ds:bx
        add     bx, cx


        cmp     cpu, 386
        je      short use_32_bit        ; use faster 32 bit code if possible

        shr     cx, 1                   ; byte = 1/2 word

        ; handle the first step with sar, the rest with rcr
        sub     bx, 2

        sar     word ptr [bx], 1        ; shift arithmetic right

        loop    top_loop_16
        jmp     short bottom


top_loop_16:
                                        ; inc does not change carry flag
        dec     bx                      ; sub bx, 2
        dec     bx

        rcr     word ptr [bx], 1        ; rotate with carry right

        loop    top_loop_16
        jmp     short bottom

use_32_bit:
.386
        shr     cx, 2                   ; byte = 1/4 dword

        sub     bx, 4                   ; decrement by double word size

        sar     dword ptr [bx], 1       ; shift arithmetic right

        loop    top_loop_32
        jmp     short bottom

top_loop_32:
        lahf                            ; save carry flag
        sub     di, 4                   ; decrement by double word size
        sub     bx, 4
	sahf                            ; restore carry flag

        rcr     dword ptr [bx], 1       ; rotate with carry right

        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        pop     ds                      ; restore ds
        mov     ax, word ptr r
        ret
half_a_bn   ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 * n2
; Note: r will be a double wide result, 2*bnlength
;       n1 and n2 can be the same pointer
; SIDE-EFFECTS: n1 and n2 are changed to their absolute values
;
unsafe_full_mult_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t
LOCAL sign1:byte, sign2:byte, samevar:byte, \
      i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      n1p: ptr byte, n2p: ptr byte

; This routine uses near variables and far pointers together in the
; inner-most loops.  Therefore, there is little to gain by setting DS
; to the far segment.  Instead, set ES to the far segment and use segment
; overrides where necessary.

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

        ; set the ES register
        les     bx, n1                      ; check for sign bits
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign1, al
        jz      already_pos1
        invoke  neg_a_bn, n1
already_pos1:

; Test to see if n1 and n2 are the same variable.  It would be better to
; use square_bn(), but it could happen.

        mov     samevar, 1                  ; assume they are the same
        mov     bx, word ptr n2
        cmp     bx, word ptr n1             ; are they the same?
        je      already_pos2                ; if so, it has already been negated
        mov     samevar, 0                  ; they weren't the same after all

        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign2, al
        jz      already_pos2
        invoke  neg_a_bn, n2
already_pos2:

; in the following loops, the following pointers are used
;   n1p, n2p = points to the part of n1, n2 being used
;   di = points to part of doublebignumber used in outer loop
;   si = points to part of doublebignumber used in inner loop
;   bx = points to part of doublebignumber for carry flag loop

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; byte = 1/2 word
        mov     steps, dx               ; save in steps
        mov     i, dx
        shl     dx, 1                   ; double steps

        sub     ax, ax                  ; clear ax
        mov     cx, dx                  ; size of doublebignumber (r) in words
        mov     di, word ptr r          ; es already set
        rep     stosw                   ; initialize r to 0

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here

top_outer_loop_16:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     bx, n1p
        mov     ax, es:[bx]
        mov     bx, n2p
        mul     word ptr es:[bx]

        mov     bx, si
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     n2p, 2                  ; increase by word size
        add     si, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     n1p, 2                  ; increase by word size
        add     di, 2
        mov     si, di                  ; start with si=di

        dec     doublesteps             ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax


        dec     i
        ja      top_outer_loop_16

        ; result is now r, a double wide bignumber
        jmp     bottom


use_32_bit:
.386
        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; byte = 1/4 dword
        mov     steps, dx               ; save in steps
        mov     i, dx
        shl     dx, 1                   ; double steps

        sub     eax, eax                ; clear eax
        mov     cx, dx                  ; size of doublebignumber in dwords
        mov     di, word ptr r          ; es already set
        rep     stosd                   ; initialize r to 0

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here

top_outer_loop_32:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     bx, n1p
        mov     eax, es:[bx]
        mov     bx, n2p
        mul     dword ptr es:[bx]

        mov     bx, si
        add     es:[bx], eax            ; add low dword
        lahf                            ; save carry flag
        add     bx, 4                   ; increase by size of dword
        sahf                            ; restor carry flag
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     n2p, 4                  ; increase by dword size
        add     si, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     n1p, 4                  ; increase by dword size
        add     di, 4
        mov     si, di                  ; start with si=di

        dec     doublesteps             ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax


        dec     i
        ja      top_outer_loop_32

        ; result is now r, a double wide bignumber

bottom:
.8086

        cmp     samevar, 1              ; were the variable the same ones?
        je      pos_answer              ; if yes, then jump

        mov     al, sign1               ; is result + or - ?
        cmp     al, sign2               ; sign(n1) == sign(n2) ?
        je      pos_answer              ; yes
        push    bnlength                ; save bnlength
        shl     bnlength, 1             ; temporarily double bnlength
                                        ; for double wide bignumber
        invoke  neg_a_bn, r             ; does not affect ES
        pop     bnlength                ; restore bnlength
pos_answer:

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret
unsafe_full_mult_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 * n2 calculating only the top rlength bytes
; Note: r will be of length rlength
;       2*bnlength <= rlength < bnlength
;       n1 and n2 can be the same pointer
; SIDE-EFFECTS: n1 and n2 are changed to their absolute values
;
unsafe_mult_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t
LOCAL sign1:byte, sign2:byte, samevar:byte, \
      i:word, j:word, steps:word, doublesteps:word, \
      carry_steps:word, skips:word, \
      n1p: ptr byte, n2p: ptr byte

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

        ; set the ES register here
        les     bx, n1                      ; check for sign bits
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign1, al
        jz      already_pos1
        invoke  neg_a_bn, n1
already_pos1:

; Test to see if n1 and n2 are the same variable.  It would be better to
; use square_bn(), but it could happen.

        mov     samevar, 1                  ; assume they are the same
        mov     bx, word ptr n2
        cmp     bx, word ptr n1             ; are they the same?
        je      already_pos2                ; if so, it has already been negated
        mov     samevar, 0                  ; they weren't the same after all

        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign2, al
        jz      already_pos2
        invoke  neg_a_bn, n2
already_pos2:

        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax

        mov     ax, bnlength
        shl     ax, 1                   ; 2*bnlength
        sub     ax, rlength             ; 2*bnlength-rlength
        add     word ptr n2, ax         ; n2 = n2+2*bnlength-rlength

; in the following loops, the following pointers are used
;   n1p, n2p = points to the part of n1, n2 being used
;   di = points to part of doublebignumber used in outer loop
;   si = points to part of doublebignumber used in inner loop
;   bx = points to part of doublebignumber for carry flag loop

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        sub     ax, ax                  ; clear ax
        mov     cx, rlength             ; size of r in bytes
        shr     cx, 1                   ; byte = 1/2 word
        mov     di, word ptr r          ; es already set
        rep     stosw                   ; initialize r to 0

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     steps, ax               ; save in steps

        mov     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     i, ax

        sub     ax, steps
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, rlength             ; set steps for first loop
        shr     ax, 1                   ; byte = 1/2 word
        sub     ax, 2                   ; only rlength/2-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     di, word ptr r          ; this is r
        mov     si, di                  ; both si and di are used here

top_outer_loop_16:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     bx, n1p
        mov     ax, es:[bx]
        mov     bx, n2p
        mul     word ptr es:[bx]

        mov     bx, si
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     n2p, 2                  ; increase by word size
        add     si, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     n1p, 2                  ; increase by word size

        cmp     skips, 0
        je      type2_shifts_16
        sub     word ptr n2, 2          ; shift n2 back a word
        inc     steps                   ; one more step this time
        ; leave di and doublesteps where they are
        dec     skips                   ; keep track of how many times we've done this
        jmp     shifts_bottom_16
type2_shifts_16:
        add     di, 2                   ; shift di forward a word
        dec     doublesteps             ; reduce the carry steps needed
shifts_bottom_16:
        mov     si, di                  ; start with si=di
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_16

        ; result is in r
        jmp     bottom


use_32_bit:
.386

        sub     eax, eax                ; clear eax
        mov     cx, rlength             ; size of r in bytes
        shr     cx, 2                   ; byte = 1/4 dword
        mov     di, word ptr r          ; es already set
        rep     stosd                   ; initialize r to 0

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     steps, ax               ; save in steps

        mov     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     i, ax

        sub     ax, steps
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, rlength             ; set steps for first loop
        shr     ax, 2                   ; byte = 1/4 dword
        sub     ax, 2                   ; only rlength/4-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     di, word ptr r          ; this is r
        mov     si, di                  ; both si and di are used here


top_outer_loop_32:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     bx, n1p
        mov     eax, es:[bx]
        mov     bx, n2p
        mul     dword ptr es:[bx]

        mov     bx, si
        add     es:[bx], eax            ; add low dword
        lahf                            ; save carry flag
        add     bx, 4                   ; increase by size of dword
        sahf                            ; restor carry flag
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of r
top_carry_loop_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     n2p, 4                  ; increase by dword size
        add     si, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     n1p, 4                  ; increase by dword size

        cmp     skips, 0
        je      type2_shifts_32
        sub     word ptr n2, 4          ; shift n2 back a dword
        inc     steps                   ; one more step this time
        ; leave di and doublesteps where they are
        dec     skips                   ; keep track of how many times we've done this
        jmp     shifts_bottom_32
type2_shifts_32:
        add     di, 4                   ; shift di forward a dword
        dec     doublesteps             ; reduce the carry steps needed
shifts_bottom_32:
        mov     si, di                  ; start with si=di
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_32

        ; result is in r

bottom:
.8086
        cmp     samevar, 1              ; were the variable the same ones?
        je      pos_answer              ; if yes, then jump

        mov     al, sign1               ; is result + or - ?
        cmp     al, sign2               ; sign(n1) == sign(n2) ?
        je      pos_answer              ; yes
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; set bnlength = rlength
        invoke  neg_a_bn, r             ; does not affect ES
        pop     bnlength                ; restore bnlength
pos_answer:

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret
unsafe_mult_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n^2
;   because of the symetry involved, n^2 is much faster than n*n
;   for a bignumber of length l
;      n*n takes l^2 multiplications
;      n^2 takes (l^2+l)/2 multiplications
;          which is about 1/2 n*n as l gets large
;  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
;
; Note: r will be a double wide result, 2*bnlength
; SIDE-EFFECTS: n is changed to its absolute value
;
unsafe_full_square_bn   PROC USES di si, r:bn_t, n:bn_t
LOCAL i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      rp1: ptr byte, rp2: ptr byte

; This routine uses near variables and far pointers together in the
; inner-most loops.  Therefore, there is little to gain by setting DS
; to the far segment.  Instead, set ES to the far segment and use segment
; overrides where necessary.

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

        ; load ES here
        les     bx, n                       ; check for sign bit
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:

; in the following loops, the following pointers are used
;   n1p(di), n2p(si) = points to the parts of n being used
;   rp1 = points to part of doublebignumber used in outer loop
;   rp2 = points to part of doublebignumber used in inner loop
;   bx = points to part of doublebignumber for carry flag loop

        mov     cx, bnlength            ; size of doublebignumber in words

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        sub     ax, ax                  ; clear ax
        ; 2{twice the size}*bnlength/2{bytes per word}
        mov     di, word ptr r          ; es already set
        rep     stosw                   ; initialize r to 0

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; byte = 1/2 word
        dec     dx                      ; don't need to do last one
        mov     i, dx                   ; loop counter
        mov     steps, dx               ; save in steps
        shl     dx, 1                   ; double steps
        sub     dx, 1                   ; only 2*s-1 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     ax, word ptr r
        add     ax, 2                   ; start with second word
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1
        mov     di, word ptr n          ; load n1p pointer

        cmp     i, 0                    ; if bignumberlength is 2
        je      skip_middle_terms_16

top_outer_loop_16:
        mov     si, di                  ; set n2p pointer
        add     si, 2                   ; to 1 word beyond n1p(di)
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     ax, es:[di]
        mul     word ptr es:[si]

        mov     bx, rp2
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     si, 2                   ; increase by word size
        add     rp2, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     di, 2                   ; increase by word size
        add     rp1, 4                  ; increase by 2*word size
        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     steps                   ; use one less step
        dec     i
        ja      top_outer_loop_16

        ; All the middle terms have been multiplied.  Now double it.
        push    bnlength
        shl     bnlength, 1             ; r is a double wide bignumber
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_16:
; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used
;   rp1(si) = points to part of doublebignumber used in outer loop
;   bx = points to part of doublebignumber for carry flag loop

        mov     di, word ptr n          ; load n1p pointer

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; 1 bytes = 1/2 word
        mov     i, dx                   ; loop counter
        shl     dx, 1                   ; double steps

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     si, word ptr r          ; set rp1

top_outer_loop_squares_16:

        mov     ax, es:[di]
        mul     ax                      ; square it

        mov     bx, si
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_squares_16 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_squares_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_squares_16

no_more_carry_squares_16:
        add     di, 2                   ; increase by word size
        add     si, 4                   ; increase by 2*word size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_16


        ; result is in r, a double wide bignumber
        jmp     bottom


use_32_bit:
.386

        sub     eax, eax                ; clear eax
        ; 2{twice the size}*bnlength/4{bytes per word}
        shr     cx, 1                   ; size of doublebignumber in dwords
        mov     di, word ptr r          ; es already set
        rep     stosd                   ; initialize r to 0

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; byte = 1/4 dword
        dec     dx                      ; don't need to do last one
        mov     i, dx                   ; loop counter
        mov     steps, dx               ; save in steps
        shl     dx, 1                   ; double steps

        sub     dx, 1                   ; only 2*s-1 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     ax, word ptr r
        add     ax, 4                   ; start with second dword
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1
        mov     di, word ptr n          ; load n1p pointer

        cmp     i, 0                    ; if bignumberlength is 4
        je      skip_middle_terms_32

top_outer_loop_32:
        mov     si, di                  ; set n2p pointer
        add     si, 4                   ; to 1 dword beyond n1p(di)
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     eax, es:[di]
        mul     dword ptr es:[si]

        mov     bx, rp2
        add     es:[bx], eax            ; add low dword
        lahf
        add     bx, 4                   ; increase by size of dword
        sahf
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     si, 4                   ; increase by dword size
        add     rp2, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     di, 4                   ; increase by dword size
        add     rp1, 8                  ; increase by 2*dword size
        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     steps                   ; use one less step
        dec     i
        ja      top_outer_loop_32

        ; All the middle terms have been multiplied.  Now double it.
        push    bnlength
        shl     bnlength, 1             ; r is a double wide bignumber
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_32:

; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used
;   rp1(si) = points to part of doublebignumber used in outer loop
;   bx = points to part of doublebignumber for carry flag loop

        mov     di, word ptr n          ; load n1p pointer

        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; 1 bytes = 1/4 dword
        mov     i, dx                   ; loop counter
        shl     dx, 1                   ; double steps

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     si, word ptr r          ; set rp1

top_outer_loop_squares_32:

        mov     eax, es:[di]
        mul     eax                     ; square it

        mov     bx, si
        add     es:[bx], eax            ; add low dword
        lahf
        add     bx, 4                   ; increase by size of dword
        sahf
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_squares_32 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_squares_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_squares_32

no_more_carry_squares_32:
        add     di, 4                   ; increase by dword size
        add     si, 8                   ; increase by 2*dword size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_32


        ; result is in r, a double wide bignumber

bottom:
.8086

; since it is a square, the result has to already be positive

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
        ret
unsafe_full_square_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n^2
;   because of the symetry involved, n^2 is much faster than n*n
;   for a bignumber of length l
;      n*n takes l^2 multiplications
;      n^2 takes (l^2+l)/2 multiplications
;          which is about 1/2 n*n as l gets large
;  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
;
; Note: r will be of length rlength
;       2*bnlength >= rlength > bnlength
; SIDE-EFFECTS: n is changed to its absolute value
;
unsafe_square_bn   PROC USES di si, r:bn_t, n:bn_t
LOCAL i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      skips:word, rodd:word, \
      n3p: ptr byte, \
      rp1: ptr byte, rp2: ptr byte

; This whole procedure would be a great deal simpler if we could assume that
; rlength < 2*bnlength (that is, not =).  Therefore, we will take the
; easy way out and call full_square_bn() if it is.
        mov     ax, rlength
        shr     ax, 1                   ; 1/2 * rlength
        cmp     ax, bnlength            ; 1/2 * rlength == bnlength?
        jne     not_full_square
ifndef ??version
        invoke  unsafe_full_square_bn, r, n
else
        invoke2 unsafe_full_square_bn, r, n
endif
        ; dx:ax is still loaded with return value
        jmp     quit_proc               ; we're outa here
not_full_square:

; This routine uses near variables and far pointers together in the
; inner-most loops.  Therefore, there is little to gain by setting DS
; to the far segment.  Instead, set ES to the far segment and use segment
; overrides where necessary.

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

        ; set ES here
        les     bx, n                       ; check for sign bit
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:

; in the following loops, the following pointers are used
;   n1p(di), n2p(si) = points to the parts of n being used
;   rp1 = points to part of doublebignumber used in outer loop
;   rp2 = points to part of doublebignumber used in inner loop
;   bx = points to part of doublebignumber for carry flag loop

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        sub     ax, ax                  ; clear ax
        mov     cx, rlength             ; size of rlength in bytes
        shr     cx, 1                   ; byte = 1/2 word
        mov     di, word ptr r          ; es already set
        rep     stosw                   ; initialize r to 0

        ; determine whether r is on an odd or even word in the number
        ; (even if rlength==2*bnlength, dec r alternates odd/even)
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        shr     ax, 1                   ; 1 byte = 1/2 word
        and     ax, 0001h               ; check the odd sign bit
        mov     rodd, ax

        mov     ax, bnlength            ; set outer loop counter
        shr     ax, 1                   ; byte = 1/2 word
        dec     ax                      ; don't need to do last one
        mov     i, ax                   ; loop counter

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     steps, ax               ; save in steps

        mov     dx, bnlength
        shr     dx, 1                   ; bnlength/2
        add     ax, dx                  ; steps+bnlength/2
        sub     ax, 2                   ; steps+bnlength/2-2
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ax, i
        sub     ax, steps
        shr     ax, 1                   ; for both words and dwords
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, word ptr r
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1
        mov     di, word ptr n          ; load n1p pointer
        mov     si, di
        mov     ax, bnlength
        shr     ax, 1                   ; 1 byte = 1/2 word
        sub     ax, steps
        shl     ax, 1                   ; 1 byte = 1/2 word
        add     si, ax                  ; n2p = n1p + 2*(bnlength/2 - steps)
        mov     n3p, si                 ; save for later use

        cmp     i, 0                    ; if bignumberlength is 2
        je      skip_middle_terms_16

top_outer_loop_16:
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     ax, es:[di]
        mul     word ptr es:[si]

        mov     bx, rp2
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     si, 2                   ; increase by word size
        add     rp2, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     di, 2                   ; increase by word size

        mov     ax, rodd                ; whether r is on an odd or even word

        cmp     skips, 0
        jle     type2_shifts_16
        sub     n3p, 2                  ; point to previous word
        mov     si, n3p
        inc     steps                   ; one more step this time
        ; leave rp1 and doublesteps where they are
        dec     skips
        jmp     shifts_bottom_16
type2_shifts_16:    ; only gets executed once
        jl      type3_shifts_16
        sub     steps, ax               ; steps -= (0 or 1)
        inc     ax                      ; ax = 1 or 2 now
        sub     doublesteps, ax         ; decrease double steps by 1 or 2
        shl     ax, 1                   ; 1 byte = 1/2 word
        add     rp1, ax                 ; add 1 or 2 words
        mov     si, di
        add     si, 2                   ; si = di + word
        dec     skips                   ; make skips negative
        jmp     shifts_bottom_16
type3_shifts_16:
        dec     steps
        sub     doublesteps, 2
        add     rp1, 4                  ; + two words
        mov     si, di
        add     si, 2                   ; si = di + word
shifts_bottom_16:

        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_16

        ; All the middle terms have been multiplied.  Now double it.
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; r is of length rlength
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_16:
; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used
;   rp1(si) = points to part of doublebignumber used in outer loop
;   bx = points to part of doublebignumber for carry flag loop

        ; be careful, the next dozen or so lines are confusing!

        ; determine whether r is on an odd or even word in the number
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        mov     dx, ax                  ; save this for a moment
        and     ax, 0002h               ; check the odd sign bit

        mov     si, word ptr r
        add     si, ax                  ; depending on odd or even byte

        shr     dx, 1                   ; assumes word size
        inc     dx
        and     dx, 0FFFEh              ; ~2+1, turn off last bit, mult of 2
        mov     di, word ptr n
        add     di, dx

        mov     ax, bnlength
        sub     ax, dx
        shr     ax, 1                   ; 1 byte = 1/2 word
        mov     i, ax

        shl     ax, 1                   ; double steps
        sub     ax, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

top_outer_loop_squares_16:

        mov     ax, es:[di]
        mul     ax                      ; square it

        mov     bx, si
        add     es:[bx], ax             ; add low word
        inc     bx                      ; increase by size of word
        inc     bx
        adc     es:[bx], dx             ; add high word
        jnc     no_more_carry_squares_16 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_16:
        add     word ptr es:[bx], 1     ; use add, not inc
        jnc     no_more_carry_squares_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_squares_16

no_more_carry_squares_16:
        add     di, 2                   ; increase by word size
        add     si, 4                   ; increase by 2*word size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_16


        ; result is in r
        jmp     bottom


use_32_bit:
.386
        sub     eax, eax                ; clear eax
        mov     cx, rlength             ; size of rlength in bytes
        shr     cx, 2                   ; byte = 1/4 dword
        mov     di, word ptr r          ; es already set
        rep     stosd                   ; initialize r to 0

        ; determine whether r is on an odd or even dword in the number
        ; (even if rlength==2*bnlength, dec r alternates odd/even)
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        shr     ax, 2                   ; 1 byte = 1/4 dword
        and     ax, 0001h               ; check the odd sign bit
        mov     rodd, ax

        mov     ax, bnlength            ; set outer loop counter
        shr     ax, 2                   ; byte = 1/4 dword
        dec     ax                      ; don't need to do last one
        mov     i, ax                   ; loop counter

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     steps, ax               ; save in steps

        mov     dx, bnlength
        shr     dx, 2                   ; bnlength/4
        add     ax, dx                  ; steps+bnlength/4
        sub     ax, 2                   ; steps+bnlength/4-2
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ax, i
        sub     ax, steps
        shr     ax, 1                   ; for both words and dwords
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, word ptr r
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1
        mov     di, word ptr n          ; load n1p pointer
        mov     si, di
        mov     ax, bnlength
        shr     ax, 2                   ; 1 byte = 1/4 dword
        sub     ax, steps
        shl     ax, 2                   ; 1 byte = 1/4 dword
        add     si, ax                  ; n2p = n1p + bnlength/4 - steps
        mov     n3p, si                 ; save for later use

        cmp     i, 0                    ; if bignumberlength is 2
        je      skip_middle_terms_32

top_outer_loop_32:
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     eax, es:[di]
        mul     dword ptr es:[si]

        mov     bx, rp2
        add     es:[bx], eax            ; add low dword
        lahf                            ; save carry flag
        add     bx, 4                   ; increase by size of dword
        sahf                            ; restore carry flag
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     si, 4                   ; increase by dword size
        add     rp2, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     di, 4                   ; increase by dword size

        mov     ax, rodd                ; whether r is on an odd or even dword

        cmp     skips, 0
        jle     type2_shifts_32
        sub     n3p, 4                  ; point to previous dword
        mov     si, n3p
        inc     steps                   ; one more step this time
        ; leave rp1 and doublesteps where they are
        dec     skips
        jmp     shifts_bottom_32
type2_shifts_32:    ; only gets executed once
        jl      type3_shifts_32
        sub     steps, ax               ; steps -= (0 or 1)
        inc     ax                      ; ax = 1 or 2 now
        sub     doublesteps, ax         ; decrease double steps by 1 or 2
        shl     ax, 2                   ; 1 byte = 1/4 dword
        add     rp1, ax                 ; add 1 or 2 dwords
        mov     si, di
        add     si, 4                   ; si = di + dword
        dec     skips                   ; make skips negative
        jmp     shifts_bottom_32
type3_shifts_32:
        dec     steps
        sub     doublesteps, 2
        add     rp1, 8                  ; + two dwords
        mov     si, di
        add     si, 4                   ; si = di + dword
shifts_bottom_32:

        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_32

        ; All the middle terms have been multiplied.  Now double it.
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; r is of length rlength
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_32:
; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used
;   rp1(si) = points to part of doublebignumber used in outer loop
;   bx = points to part of doublebignumber for carry flag loop

        ; be careful, the next dozen or so lines are confusing!

        ; determine whether r is on an odd or even word in the number
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        mov     dx, ax                  ; save this for a moment
        and     ax, 0004h               ; check the odd sign bit

        mov     si, word ptr r
        add     si, ax                  ; depending on odd or even byte

        shr     dx, 2                   ; assumes dword size
        inc     dx
        and     dx, 0FFFEh              ; ~2+1, turn off last bit, mult of 2
        shl     dx, 1
        mov     di, word ptr n
        add     di, dx

        mov     ax, bnlength
        sub     ax, dx
        shr     ax, 2                   ; 1 byte = 1/4 dword
        mov     i, ax

        shl     ax, 1                   ; double steps
        sub     ax, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

top_outer_loop_squares_32:

        mov     eax, es:[di]
        mul     eax                     ; square it

        mov     bx, si
        add     es:[bx], eax            ; add low dword
        lahf                            ; save carry flag
        add     bx, 4                   ; increase by size of dword
        sahf                            ; restore carry flag
        adc     es:[bx], edx            ; add high dword
        jnc     no_more_carry_squares_32 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_32:
        add     dword ptr es:[bx], 1    ; use add, not inc
        jnc     no_more_carry_squares_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_squares_32

no_more_carry_squares_32:
        add     di, 4                   ; increase by dword size
        add     si, 8                   ; increase by 2*dword size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_32


        ; result is in r

bottom:
.8086

; since it is a square, the result has to already be positive

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r
quit_proc:
        ret
unsafe_square_bn   ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n * u  where u is an unsigned integer
mult_bn_int   PROC USES di si, r:bn_t, n:bn_t, u:word
LOCAL   lu:dword  ; long unsigned integer in 32 bit math

        mov     cx, bnlength            ; set outer loop counter
        mov     ax, ds                  ; mov DS to ES for one segment override
        mov     es, ax
        mov     si, word ptr n          ; load pointers ds:si
        lds     di, r                   ;               ds:di

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        ; no need to clear r
        shr     cx, 1                   ; byte = 1/2 word
        sub     bx, bx                  ; use bx for temp holding carried word

top_loop_16:
        mov     ax, [si]                ; load next word from n
        mul     es:u                    ; n * u
        add     ax, bx                  ; add last carried upper word
        adc     dx, 0                   ; inc the carried word if carry flag set
        mov     bx, dx                  ; save high word in bx
        mov     [di], ax                ; save low word

        add     di, 2                   ; next word in r
        add     si, 2                   ; next word in n
        loop    top_loop_16
        jmp     bottom

use_32_bit:
.386
        ; no need to clear r

        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; use ebx for temp holding carried dword

        sub     eax, eax                ; clear upper eax
        mov     ax, es:u                ; convert u (unsigned int)
        mov     es:lu, eax              ;   to lu (long unsigned int)

top_loop_32:
        mov     eax, [si]               ; load next dword from n
        mul     es:lu                   ; n * lu
        add     eax, ebx                ; add last carried upper dword
        adc     edx, 0                  ; inc the carried dword if carry flag set
        mov     ebx, edx                ; save high dword in ebx
        mov     [di], eax               ; save low dword

        add     di, 4                   ; next dword in r
        add     si, 4                   ; next dword in n
        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        mov     ax, es                  ; restore ds
        mov     ds, ax
        mov     ax, word ptr r
        ret
mult_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= u  where u is an unsigned integer
mult_a_bn_int   PROC USES di si, r:bn_t, u:word

        mov     cx, bnlength            ; set outer loop counter
        mov     ax, ds                  ; save ds
        mov     es, ax
        mov     si, u                   ; save u in    ds:si
        lds     di, r                   ; load pointer ds:di

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        ; no need to clear r
        shr     cx, 1                   ; byte = 1/2 word
        sub     bx, bx                  ; use bx for temp holding carried word

top_loop_16:
        mov     ax, [di]                ; load next word from r
        mul     si                      ; r * u
        add     ax, bx                  ; add last carried upper word
        adc     dx, 0                   ; inc the carried word if carry flag set
        mov     bx, dx                  ; save high word in bx
        mov     [di], ax                ; save low word

        add     di, 2                   ; next word in r
        loop    top_loop_16
        jmp     bottom

use_32_bit:
.386
        ; no need to clear r
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; use ebx for temp holding carried dword
        sub     esi, esi                ; clear upper esi

top_loop_32:
        mov     eax, [di]               ; load next dword from r
        mul     esi                     ; r * u
        add     eax, ebx                ; add last carried upper dword
        adc     edx, 0                  ; inc the carried dword if carry flag set
        mov     ebx, edx                ; save high dword in ebx
        mov     [di], eax               ; save low dword

        add     di, 4                   ; next dword in r
        loop    top_loop_32

bottom:
.8086

        mov     dx, ds                  ; return r in dx:ax
        mov     ax, es                  ; restore ds
        mov     ds, ax
        mov     ax, word ptr r
        ret
mult_a_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n / u  where u is an unsigned integer
unsafe_div_bn_int   PROC USES di si, r:bn_t, n:bn_t, u:word
LOCAL  sign:byte

        les     bx, n                       ; check for sign bits
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign, al
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:
        mov     cx, bnlength            ; set outer loop counter
        mov     ax, ds                  ; use es for default ds
        mov     es, ax
        ; past most significant portion of the number
        mov     si, word ptr n              ; load pointers ds:si
        add     si, cx
        lds     di, r                       ; ds:di
        add     di, cx

        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 1                   ; byte = 1/2 word
        mov     bx, es:u

        ; need to start with most significant portion of the number
        sub     si, 2                   ; most sig word
        sub     di, 2                   ; most sig word

        sub     dx, dx                  ; clear dx register
                                        ; for first time through loop
top_loop_16:
        mov     ax, [si]                ; load next word from n
        div     bx
        mov     [di], ax                ; store low word
                                        ; leave remainder in dx

        sub     si, 2                   ; next word in n
        sub     di, 2                   ; next word in r
        loop    top_loop_16
        jmp     bottom

use_32_bit:
.386
        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; clear upper word or ebx
        mov     bx, es:u

        ; need to start with most significant portion of the number
        sub     si, 4                   ; most sig dword
        sub     di, 4                   ; most sig dword

        sub     edx, edx                ; clear edx register
                                        ; for first time through loop
top_loop_32:
        mov     eax, [si]               ; load next dword from n
        div     ebx
        mov     [di], eax               ; store low dword
                                        ; leave remainder in edx

        sub     si, 4                   ; next dword in n
        sub     di, 4                   ; next dword in r
        loop    top_loop_32

bottom:
.8086

        mov     ax, es                  ; swap es & ds
        mov     bx, ds
        mov     ds, ax
        mov     es, bx

        cmp     sign, 0                 ; is result + or - ?
        je      pos_answer              ; yes
        invoke  neg_a_bn, r             ; does not affect ES
pos_answer:

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r          ;
        ret
unsafe_div_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r /= u  where u is an unsigned integer
div_a_bn_int   PROC USES di, r:bn_t, u:word
LOCAL  sign:byte

        les     bx, r                       ; check for sign bits
        add     bx, bnlength
        dec     bx
        mov     al, es:[bx]
        and     al, 80h                     ; check the sign bit
        mov     sign, al
        jz      already_pos
        invoke  neg_a_bn, r
already_pos:

        mov     cx, bnlength            ; set outer loop counter
        mov     ax, ds                  ; save ds in es
        mov     es, ax
        lds     di, r                   ; load pointer r in ds:di
        add     di, cx


        cmp     cpu, 386                ; check cpu
        je      use_32_bit              ; use faster 32 bit code if possible

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 1                   ; byte = 1/2 word
        mov     bx, es:u

        ; need to start with most significant portion of the number
        sub     di, 2                   ; most sig word

        sub     dx, dx                  ; clear dx register
                                        ; for first time through loop
top_loop_16:
        mov     ax, [di]                ; load next word from r
        div     bx
        mov     [di], ax                ; store low word
                                        ; leave remainder in dx

        sub     di, 2                   ; next word in r
        loop    top_loop_16
        jmp     bottom

use_32_bit:
.386
        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; clear upper word or ebx
        mov     bx, es:u

        ; need to start with most significant portion of the number
        sub     di, 4                   ; most sig dword

        sub     edx, edx                ; clear edx register
                                        ; for first time through loop
top_loop_32:
        mov     eax, [di]               ; load next dword from r
        div     ebx
        mov     [di], eax               ; store low dword
                                        ; leave remainder in edx

        sub     di, 4                   ; next dword in r
        loop    top_loop_32

bottom:
.8086
        mov     ax, es                  ; swap es & ds
        mov     bx, ds
        mov     ds, ax
        mov     es, bx

        cmp     sign, 0                 ; is result + or - ?
        je      pos_answer              ; yes
        invoke  neg_a_bn, r             ; does not affect ES
pos_answer:

        mov     dx, es                  ; return r in dx:ax
        mov     ax, word ptr r          ;
        ret
div_a_bn_int   ENDP

END
