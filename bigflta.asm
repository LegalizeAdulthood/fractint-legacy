; bigflta.asm - asm routines for bigfloats
; Wesley Loewer's Big Numbers.

.MODEL medium, c

ifdef ??version
   ideal
TYPEDEF bf_t far ptr byte
TYPEDEF bn_t far ptr byte
   masm
   masm51
   quirks
extern equ extrn
real10 equ tbyte
REAL10 equ tbyte
sword  equ word
; this is a horrible kludge
; if more invokes are added to the code then this may fail
invoke macro a,b
   call      a, b
endm
endif

ifdef ??version
extrn clear_bf:far
extrn neg_a_bf:far
endif

ifndef ??version

bf_t TYPEDEF far ptr byte   ; far pointer to bigfloat array

; testing
bn_t TYPEDEF far ptr byte   ; far pointer to bigfloat array

endif ; ??version

.DATA

extern cpu:word, fpu:word
extern bflength:word

; testing
extern bnlength:word, intlength:word

.CODE

ifndef ??version
clear_bf PROTO, n:bf_t
neg_a_bf PROTO, n:bf_t
endif

.8086
.8087

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL extract_256(LDBL f, int *exp_ptr)
;
; extracts the mantissa and exponant of f
; finds m and n such that 1<=|m|<256 and f = m*256^n
; n is stored in *exp_ptr and m is returned, sort of like frexp()

extract_256   PROC f:real10, exp_ptr: ptr sword
local  expon:sword, exf:real10, tmp_word:word

        fld     f               ; f
        ftst                    ; test for zero
        fstsw   tmp_word
	fwait
	mov     ax,tmp_word
        sahf
        jnz     not_zero        ; proceed

        mov     bx, exp_ptr
        mov     word ptr [bx], 0    ; save = in *exp_ptr
        jmp     bottom          ; f, which is zero, is already on stack

not_zero:

; since a key fpu operation, fxtract, is not emulated by the MS floating
; point library, separate code is included under use_emul:
        cmp     fpu, 0
        je      use_emul

                                ; f is already on stack
        fxtract                 ; mant exp, where f=mant*2^exp
        fxch                    ; exp mant
        fistp   expon           ; mant
        fwait
        mov     ax, expon
        mov     dx, ax          ; make copy for later use

        cmp     ax, 0           ;
        jge     pos_exp         ; jump if exp >= 0

                                ; exp is neg, adjust exp
        add     ax, 8           ; exp+8

pos_exp:
; adjust mantissa
        and     ax, 7           ; ax mod 8
        jz      adjust_exponent ; don't bother with zero adjustments
        mov     expon, ax       ; use expon as a temp var
        fild    expon           ; exp mant

        fxch                    ; mant exp
        fscale                  ; mant*2^exp exp
        fstp    st(1)           ; mant*2^exp (store in 1 and pop)

adjust_exponent:
        mov     cl, 3
        sar     dx, cl          ; exp / 8
        mov     bx, exp_ptr
        mov     [bx], dx        ; save in *exp_ptr

        fwait
        jmp     bottom


use_emul:
; emulate above code by direct manipulation of 80 bit floating point format
                                    ; f is already on stack
        fstp    exf

        mov     ax, word ptr exf+8  ; get word with the exponent in it
        mov     dx, ax              ; make copy for later use

        and     dx, 8000h           ; keep just the sign bit
        or      dx, 3FFFh           ; 1<=f<2

        and     ax, 7FFFh           ; throw away the sign bit
        sub     ax, 3FFFh           ; unbiased -> biased
        mov     bx, ax
        cmp     bx, 0
        jge     pos_exp_emul
        add     bx, 8               ; adjust negative exponent
pos_exp_emul:
        and     bx, 7               ; bx mod 8
        add     dx, bx
        mov     word ptr exf+8, dx  ; put back word with the exponent in it

        mov     cl, 3
        sar     ax, cl              ; div by 8,  2^(8n) = 256^n
        mov     bx, exp_ptr
        mov     [bx], ax            ; save in *exp_ptr

        fld     exf                 ; for return value

bottom:
        ; unlike float and double, long double is returned on fpu stack
        ret
extract_256   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL scale_256( LDBL f, int n );
; calculates and returns the value of f*256^n
; sort of like ldexp()
;
; n must be in the range -2^12 <= n < 2^12 (2^12=4096),
; which should not be a problem

scale_256   PROC f:real10, n: sword

        cmp     n, 0
        jne     non_zero
        fld     f
        jmp     bottom          ; don't bother with scales of zero

non_zero:
        mov     cl, 3
        shl     n, cl           ; 8n
        fild    n               ; 8n
        fld     f               ; f 8n
; the fscale range limits for 8087/287 processors won't be a problem here
        fscale                  ; new_f=f*2^(8n)=f*256^n  8n
        fstp    st(1)           ; new_f

bottom:
        ; unlike float and double, long double is returned on fpu stack
        ret
scale_256   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL bftofloat(bf_t n);
; converts a bf number to a 10 byte real
;
bftofloat   PROC USES di si, n:bf_t
   LOCAL value[11]:BYTE   ; 11=10+1

      mov      cx, 9                   ; need up to 9 bytes
      cmp      bflength, 10            ; but no more than bflength-1
      jae      movebytes_set
      mov      cx, bflength            ; bflength is less than 10
      dec      cx                      ; cx=movebytes=bflength-1, 1 byte padding
movebytes_set:

      cmp     cpu, 386              ; check cpu
      je      use_32_bit            ; use faster 32 bit code if possible

; 16 bit code
      ; clear value
      mov      word ptr value[0], 0
      mov      word ptr value[2], 0
      mov      word ptr value[4], 0
      mov      word ptr value[6], 0
      mov      word ptr value[8], 0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      ax, ds               ; save ds
      mov      es, ax               ; also move to es
      mov      bx, bflength
      dec      bx
      sub      bx, cx               ; cx holds movebytes
      lds      si, n
      add      si, bx               ; n+bflength-1-movebytes
      rep movsb
      mov      bl, ds:[si]          ; save sign byte, si now points to it
      inc      si                   ; point to exponent
      mov      dx, ds:[si]          ; use dx as exponent
      mov      ds, ax               ; restore ds
      mov      cl, 3                ; put exponent (dx) in base 2
      shl      dx, cl               ; 256^n = 2^(8n)

      ; adjust for negative values
      and      bl, 10000000b           ; isolate sign bit
      jz       not_neg_16
      neg      word ptr value[0]       ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      word ptr value[2]
      adc      word ptr value[2], 0
      not      word ptr value[4]
      adc      word ptr value[4], 0
      not      word ptr value[6]
      adc      word ptr value[6], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
not_neg_16:

      cmp      byte ptr value[8], 0          ; test for 0
      jz       return_zero

      ; Shift until most signifcant bit is set.
top_shift_16:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom_shift_16
      dec      dx                      ; decrement exponent
      shl      word ptr value[0], 1    ; shift left the 9 byte number
      rcl      word ptr value[2], 1
      rcl      word ptr value[4], 1
      rcl      word ptr value[6], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_16
bottom_shift_16:

      ; round last byte
      cmp      byte ptr value[0], 80h  ;
      jb       bottom                  ; no rounding necessary
      add      word ptr value[1], 1
      adc      word ptr value[3], 0
      adc      word ptr value[5], 0
      adc      word ptr value[7], 0
      jnc      bottom

      ; to get to here, the pattern was rounded from +FFFF...
      ; to +10000... with the 1 getting moved to the carry bit
      jmp      rounded_past_end

return_zero:
      fldz
      jmp      return

use_32_bit:
.386
      ; clear value
      mov      dword ptr value[0], 0
      mov      dword ptr value[4], 0
      mov      word ptr value[8],  0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      ax, ds               ; save ds
      mov      es, ax               ; also move to es
      mov      bx, bflength
      dec      bx
      sub      bx, cx               ; cx holds movebytes
      lds      si, n
      add      si, bx               ; n+bflength-1-movebytes
      rep movsb
      mov      bl, ds:[si]          ; save sign byte, si now points to it
      inc      si                   ; point to exponent
      mov      dx, ds:[si]          ; use dx as exponent
      mov      ds, ax               ; restore ds
      shl      dx, 3                ; 256^n = 2^(8n)

      ; adjust for negative values
      and      bl, 10000000b           ; determine sign
      jz       not_neg_32
      neg      dword ptr value[0]      ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      dword ptr value[4]
      adc      dword ptr value[4], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
not_neg_32:

      cmp      byte ptr value[8], 0          ; test for 0
      jz       return_zero

      ; Shift until most signifcant bit is set.
top_shift_32:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom_shift_32
      dec      dx                      ; decrement exponent
      shl      dword ptr value[0], 1   ; shift left the 9 byte number
      rcl      dword ptr value[4], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_32
bottom_shift_32:

      ; round last byte
      cmp      byte ptr value[0], 80h  ;
      jb       bottom                  ; no rounding necessary
      add      dword ptr value[1], 1
      adc      dword ptr value[5], 0
      jnc      bottom

      ; to get to here, the pattern was rounded from +FFFF...
      ; to +10000... with the 1 getting moved to the carry bit
rounded_past_end:
.8086 ; used in 16 it code as well
      mov      byte ptr value[8], 10000000b
      inc      dx                      ; adjust the exponent

bottom:
.8086
      ; adjust exponent
      add      dx, 3FFFh+7             ; unbiased -> biased, + adjusted
      or       dh, bl                  ; set sign bit if set
      mov      word ptr value[9], dx

      ; unlike float and double, long double is returned on fpu stack
      fld      real10 ptr value[1]    ; load return value
return:
      ret

bftofloat   endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL floattobf(bf_t n, LDBL f);
; converts a 10 byte real to a bf number
;
floattobf   PROC USES di si, n:bf_t, f:REAL10
   LOCAL value[9]:BYTE   ; 9=8+1
; I figured out a way to do this with no local variables,
; but it's not worth it.

      invoke   clear_bf, n

      ; check to see if f is 0
      cmp      byte ptr f[7], 0        ; f[7] can only be 0 if f is 0
      jz       return                  ; if f is 0, bailout now

      mov      cx, 9                   ; need up to 9 bytes
      cmp      bflength, 10            ; but no more than bflength-1
      jae      movebytes_set
      mov      cx, bflength            ; bflength is less than 10
      dec      cx                      ; movebytes = bflength-1, 1 byte padding
movebytes_set:

      cmp     cpu, 386              ; check cpu
      je      use_32_bit            ; use faster 32 bit code if possible

; 16 bit code
      ; copy bytes from f's mantissa to value
      mov      byte ptr value[0], 0    ; clear least sig byte
      mov      ax, word ptr f[0]
      mov      word ptr value[1], ax
      mov      ax, word ptr f[2]
      mov      word ptr value[3], ax
      mov      ax, word ptr f[4]
      mov      word ptr value[5], ax
      mov      ax, word ptr f[6]
      mov      word ptr value[7], ax

      ; get exponent in dx
      mov      dx, word ptr f[8]       ; location of exponent
      and      dx, 7FFFh               ; remove sign bit
      sub      dx, 3FFFh+7             ; biased -> unbiased, + adjust

      ; Shift down until exponent is a mult of 8 (2^8n=256n)
top_shift_16:
      test     dx, 111b                ; expon mod 8
      jz       bottom
      inc      dx                      ; increment exponent
      shr      word ptr value[7], 1    ; shift right the 9 byte number
      rcr      word ptr value[5], 1
      rcr      word ptr value[3], 1
      rcr      word ptr value[1], 1
      rcr      byte ptr value[0], 1    ; notice this last one is byte ptr
      jmp      top_shift_16


use_32_bit:
.386
      ; copy bytes from f's mantissa to value
      mov      byte ptr value[0], 0    ; clear least sig byte
      mov      eax, dword ptr f[0]
      mov      dword ptr value[1], eax
      mov      eax, dword ptr f[4]
      mov      dword ptr value[5], eax

      ; get exponent in dx
      mov      dx, word ptr f[8]       ; location of exponent
      and      dx, 7FFFh               ; remove sign bit
      sub      dx, 3FFFh+7             ; biased -> unbiased, + adjust

      ; Shift down until exponent is a mult of 8 (2^8n=256n)
top_shift_32:
      test     dx, 111b                ; expon mod 8
      jz       bottom
      inc      dx                      ; increment exponent
      shr      dword ptr value[5], 1   ; shift right the 9 byte number
      rcr      dword ptr value[1], 1
      rcr      byte ptr value[0], 1    ; notice this last one is byte ptr
      jmp      top_shift_32

bottom:
.8086
      ; Don't bother rounding last byte as it would only make a difference
      ; when bflength < 9, and then only on the last bit.

      ; move data into place, from value to n
      lea      si, value+9
      sub      si, cx               ; cx holds movebytes
      les      di, n
      add      di, bflength
      dec      di
      sub      di, cx               ; cx holds movebytes
      rep movsb
      inc      di
      mov      cl, 3
      sar      dx, cl               ; divide expon by 8, 256^n=2^8n
      mov      word ptr es:[di], dx ; store exponent

      ; get sign
      test     byte ptr f[9], 10000000b           ; test sign bit
      jz       not_negative
      invoke   neg_a_bf, n
not_negative:
return:
      les      ax, n                   ; load es again in case f was 0
      mov      dx, es                  ; return r in dx:ax
      ret
floattobf   endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL bntofloat(bf_t n);
; converts a bn number to a 10 byte real
; (the most speed critical of these to/from float routines)
bntofloat   PROC USES di si, n:bn_t
   LOCAL value[11]:BYTE   ; 11=10+1

      ; determine the most significant byte, not 0 or FF
      les      si, n
      dec      si
      add      si, bnlength            ; n+bnlength-1
      mov      bl, es:[si]             ; top byte
      mov      cx, bnlength            ; initialize cx with full bnlength
      cmp      bl, 0                   ; test top byte against 0
      je       determine_sig_bytes
      cmp      bl, 0FFh                ; test top byte against -1
      jne      sig_bytes_determined

determine_sig_bytes:
      dec      cx                      ; now bnlength-1
top_sig_byte:
      dec      si                      ; previous byte
      cmp      es:[si], bl             ; does it have the right stuff?
      jne      sig_bytes_determined    ; (ie: does it match top byte?)
      loop     top_sig_byte            ; decrement cx and repeat

; At this point, it must be 0 with no sig figs at all
; or -1/(256^bnlength), one bit away from being zero.
      cmp      bl, 0                   ; was it zero?
      jz       return_zero             ; yes
                                       ; no, it was a very small negative
      mov      ax, intlength
      sub      ax, bnlength
      mov      cl, 3
      shl      ax, cl                  ; 256^n=2^8n, now more like movebits
      add      ax, 3FFFh+0             ; bias, no adjustment necessary
      or       ah, 10000000b           ; turn on sign flag
      mov      word ptr value[9], ax   ; store exponent
      mov      word ptr value[7], 8000h ; store mantissa of 1 in most sig bit
      ; clear rest of value that is actually used
      mov      word ptr value[1], 0
      mov      word ptr value[3], 0
      mov      word ptr value[5], 0

      fld      real10 ptr value[1]
      jmp      return

sig_bytes_determined:
      mov      dx, cx               ; save in dx for later
      cmp      cx, 9-1              ; no more than cx bytes
      jb       set_movebytes
      mov      cx, 9-1              ; up to 8 bytes
set_movebytes:                      ; cx now holds movebytes
                                    ; si still points to most non-0 sig byte
      sub      si, cx               ; si now points to first byte to be moved
      inc      cx                   ; can be up to 9

      cmp     cpu, 386              ; check cpu
      je      use_32_bit            ; use faster 32 bit code if possible

; 16 bit code
      ; clear value
      mov      word ptr value[0], 0
      mov      word ptr value[2], 0
      mov      word ptr value[4], 0
      mov      word ptr value[6], 0
      mov      word ptr value[8], 0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value  ; es:si still holds first move byte of n
      mov      di, es               ; swap es and ds
      mov      ax, ds
      mov      es, ax               ; ax is left with original ds
      mov      ds, di
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      rep movsb
      mov      ds, ax               ; restore ds

      ; adjust for negative values
      xor      ax, ax                  ; use ax as a flag
      ; get sign flag                  ; top byte is still in bl
      and      bl, 10000000b           ; isolate the sign bit
      jz       not_neg_16
      neg      word ptr value[0]       ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      word ptr value[2]
      adc      word ptr value[2], 0
      not      word ptr value[4]
      adc      word ptr value[4], 0
      not      word ptr value[6]
      adc      word ptr value[6], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
      jnc      not_neg_16              ; normal
      mov      byte ptr value[8], 10000000b    ;n was FFFF...0000...
      inc      ax                      ; set ax to 1 to flag this special case

not_neg_16:
      sub      dx, bnlength            ; adjust exponent
      add      dx, intlength           ; adjust exponent
      mov      cl, 3
      shl      dx, cl                  ; 256^n=2^8n
      add      dx, ax                  ; see special case above
      ; Shift until most signifcant bit is set.
top_shift_16:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom
      dec      dx                      ; decrement exponent
      shl      word ptr value[0], 1    ; shift left the 9 byte number
      rcl      word ptr value[2], 1
      rcl      word ptr value[4], 1
      rcl      word ptr value[6], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_16

; don't bother rounding, not really needed while speed is.

return_zero:
      fldz
      jmp      return

use_32_bit:
.386
      ; clear value
      mov      dword ptr value[0], 0
      mov      dword ptr value[4], 0
      mov      word ptr value[8],  0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value  ; es:si still holds first move byte of n
      mov      di, es               ; swap es and ds
      mov      ax, ds
      mov      es, ax               ; ax is left with original ds
      mov      ds, di
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      rep movsb
      mov      ds, ax               ; restore ds

      ; adjust for negative values
      xor      ax, ax                  ; use ax as a flag
      ; get sign flag                  ; top byte is still in bl
      and      bl, 10000000b           ; determine sign
      jz       not_neg_32
      neg      dword ptr value[0]      ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      dword ptr value[4]
      adc      dword ptr value[4], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
      jnc      not_neg_32              ; normal
      mov      byte ptr value[8], 10000000b    ;n was FFFF...0000...
      inc      ax                      ; set ax to 1 to flag this special case

not_neg_32:
      sub      dx, bnlength            ; adjust exponent
      add      dx, intlength           ; adjust exponent
      shl      dx, 3                   ; 256^n=2^8n
      add      dx, ax                  ; see special case above
      ; Shift until most signifcant bit is set.
top_shift_32:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom
      dec      dx                      ; decrement exponent
      shl      dword ptr value[0], 1   ; shift left the 9 byte number
      rcl      dword ptr value[4], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_32

; don't bother rounding, not really needed while speed is.

bottom:
.8086
      ; adjust exponent
      add      dx, 3FFFh+7-8           ; unbiased -> biased, + adjusted
      or       dh, bl                  ; set sign bit if set
      mov      word ptr value[9], dx

      ; unlike float and double, long double is returned on fpu stack
      fld      real10 ptr value[1]    ; load return value
return:
      ret

bntofloat   endp

;
; LDBL floattobn(bf_t n, LDBL f) is in BIGNUM.C
;

END
