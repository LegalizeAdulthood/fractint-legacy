TITLE fpu087.asm (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
SUBTTL All rights reserved.
;
;  Code may be used in any program provided the author is credited
;    either during program execution or in the documentation.  Source
;    code may be distributed only in combination with public domain or
;    shareware source code.  Source code may be modified provided the
;    copyright notice and this message is left unchanged and all
;    modifications are clearly documented.
;
;    I would appreciate a copy of any work which incorporates this code.
;
;    Mark C. Peterson
;    253 West St., H
;    Plantsville, CT 06579
;    (203) 276-9474
;
;  References:
;     The VNR Concise Encyclopedia of Mathematics
;        by W. Gellert, H. Hustner, M. Hellwich, and H. Kastner
;        Published by Van Nostrand Reinhold Comp, 1975
;
;     80386/80286 Assembly Language Programming
;        by William H. Murray, III and Chris H. Pappas
;        Published by Osborne McGraw-Hill, 1986
;        
;
;

IFDEF ??version
MASM51
QUIRKS
EMUL
ENDIF

.model medium, c

extrn cos:far
extrn _Loaded387sincos:far
extrn compiled_by_turboc:word


.data

extrn cpu:WORD

PUBLIC TrigLimit, TrigOverflow

PiFg13         dw       6487h
InvPiFg33      dd       0a2f9836eh
InvPiFg16      dw       517ch
Ln2Fg16        dw       0b172h
TrigOverflow   dw       0
TrigLimit      dd       0
one            dw       ?
expSign        dw       ?
a              dw       ?
SinNeg         dw       ?
CosNeg	       dw	?
Ans	       dq	?
fake_es        dw       ?         ; <BDT> Windows can't use ES for storage

TaylorTerm  MACRO
LOCAL Ratio
   add   Factorial, one
   jnc   SHORT Ratio

   rcr   Factorial, 1
   shr   Num, 1
   shr   one, 1

Ratio:
   mul   Num
   div   Factorial
ENDM



_4_         dq    4.0
_2_         dq    2.0
_1_         dq    1.0
PointFive   dq    0.5
temp        dq     ?
Sign        dw     ?

extrn fpu:word

.code


FPUcplxmul     PROC     x:word, y:word, z:word
   mov   bx, x
   fld   QWORD PTR [bx]       ; x.x
   fld   QWORD PTR [bx+8]     ; x.y, x.x
   mov   bx, y
   fld   QWORD PTR [bx]       ; y.x, x.y, x.x
   fld   QWORD PTR [bx+8]     ; y.y, y.x, x.y, x.x
   mov   bx, z
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st(3)            ; y.y*x.y, y.y. y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*x.y, y.y, y.x, x.y, x.x
   fmul  st, st(5)            ; y.x*x.x, y.y*x.y, y.y, y.x, x.y, x.x
   fsubr                      ; y.x*x.x - y.y*x.y, y.y, y.x, x.y, x.x
   fstp  QWORD PTR [bx]       ; y.y, y.x, x.y, x.x
   fmulp st(3), st            ; y.x, x.y, x.x*y.y
   fmul                       ; y.x*x.y, x.x*y.y
   fadd                       ; y.x*x.y + x.x*y.y
   fstp  QWORD PTR [bx+8]
   ret
FPUcplxmul     ENDP





FPUcplxdiv     PROC     x:word, y:word, z:word
   mov   bx, x
   fld   QWORD PTR [bx]       ; x.x
   fld   QWORD PTR [bx+8]     ; x.y, x.x
   mov   bx, y
   fld   QWORD PTR [bx]       ; y.x, x.y, x.x
   fld   QWORD PTR [bx+8]     ; y.y, y.x, x.y, x.x
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st               ; y.y*y.y, y.y, y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*y.y, y.y, y.x, x.y, x.x
   fmul  st, st               ; y.x*y.x, y.y*y.y, y.y, y.x, x.y, x.x
   fadd                       ; mod, y.y, y.x, x.y, x.x
   fdiv  st(1), st            ; mod, y.y=y.y/mod, y.x, x.y, x.x
   fdivp st(2), st            ; y.y, y.x=y.x/mod, x.y, x.x
   mov   bx, z
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st(3)            ; y.y*x.y, y.y. y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*x.y, y.y, y.x, x.y, x.x
   fmul  st, st(5)            ; y.x*x.x, y.y*x.y, y.y, y.x, x.y, x.x
   fadd                       ; y.x*x.x - y.y*x.y, y.y, y.x, x.y, x.x
   fstp  QWORD PTR [bx]       ; y.y, y.x, x.y, x.x
   fmulp st(3), st            ; y.x, x.y, x.x*y.y
   fmul                       ; y.x*x.y, x.x*y.y
   fsubr                      ; y.x*x.y + x.x*y.y
   fstp  QWORD PTR [bx+8]
   ret
FPUcplxdiv     ENDP



FPUcplxlog     PROC     x:word, z:word
LOCAL Status:word
   mov   bx, x
   fld   QWORD PTR [bx+8]        ; x.y
   fld   QWORD PTR [bx]          ; x.x, x.y
   mov   bx, z
   fldln2                        ; ln2, x.x, x.y
   fdiv  _2_                     ; ln2/2, x.x, x.y
   fld   st(2)                   ; x.y, ln2/2, x.x, x.y
   fmul  st, st                  ; sqr(x.y), ln2/2, x.x, x.y
   fld   st(2)                   ; x.x, sqr(x.y), ln2/2, x.x, x.y
   fmul  st, st                  ; sqr(x.x), sqr(x.y), ln2/2, x.x, x.y
   fadd                          ; mod, ln2/2, x.x, x.y
   fyl2x                         ; z.x, x.x, x.y
   fstp  QWORD PTR [bx]          ; x.x, x.y
   cmp   fpu, 387
   jne   Restricted

   fpatan
   jmp   StoreZX

Restricted:
   mov   bx, x
   mov   dh, BYTE PTR [bx+7]
   or    dh, dh
   jns   ChkYSign

   fchs                          ; |x.x|, x.y

ChkYSign:
   mov   dl, BYTE PTR [bx+8+7]
   or    dl, dl
   jns   ChkMagnitudes

   fxch                          ; x.y, |x.x|
   fchs                          ; |x.y|, |x.x|
   fxch                          ; |x.x|, |x.y|

ChkMagnitudes:
   fcom  st(1)                   ; x.x, x.y
   fstsw Status                  ; x.x, x.y
   test  Status, 4500h
   jz    XisGTY

   test  Status, 4000h
   jz    XneY

   fstp  st                      ; x.y
   fstp  st                      ; <empty>
   fldpi                         ; Pi
   fdiv  _4_                     ; Pi/4
   jmp   ChkSignZ

XneY:
   fxch                          ; x.y, x.x
   fpatan                        ; Pi/2 - Angle
   fldpi                         ; Pi, Pi/2 - Angle
   fdiv  _2_                     ; Pi/2, Pi/2 - Angle
   fsubr                         ; Angle
   jmp   ChkSignZ

XisGTY:
   fpatan

ChkSignZ:
   or    dh, dh
   js    NegX

   or    dl, dl
   jns   StoreZX

   fchs
   jmp   StoreZX

NegX:
   or    dl, dl
   js    QuadIII

   fldpi
   fsubr
   jmp   StoreZX

QuadIII:
   fldpi
   fsubr
   fchs

StoreZX:
   mov   bx, z
   fstp  QWORD PTR [bx+8]        ; <empty>
   ret
FPUcplxlog     ENDP




FPUsinhcosh    PROC     x:word, sinh:word, cosh:word
LOCAL Control:word
   fstcw Control
   push  Control                       ; Save control word on the stack
   or    Control, 0000110000000000b 
   fldcw Control                       ; Set control to round towards zero

   mov   Sign, 0              ; Assume the sign is positive
   mov   bx, x

   fldln2                     ; ln(2)
   fdivr QWORD PTR [bx]       ; x/ln(2)

   cmp   BYTE PTR [bx+7], 0
   jns   DuplicateX

   fchs                       ; x = |x|

DuplicateX:   
   fld   st                   ; x/ln(2), x/ln(2)
   frndint                    ; int = integer(|x|/ln(2)), x/ln(2)
   fxch                       ; x/ln(2), int
   fsub  st, st(1)            ; rem < 1.0, int
   fdiv  _2_                  ; rem/2 < 0.5, int
   f2xm1                      ; (2**rem/2)-1, int
   fadd  _1_                  ; 2**rem/2, int
   fmul  st, st               ; 2**rem, int
   fscale                     ; e**|x|, int
   fstp  st(1)                ; e**|x|

   cmp   BYTE PTR [bx+7], 0
   jns   ExitFexp

   fdivr _1_                  ; e**x      

ExitFexp:
   fld   st                   ; e**x, e**x
   fdivr PointFive            ; e**-x/2, e**x
   fld   st                   ; e**-x/2, e**-x/2, e**x
   fxch  st(2)                ; e**x, e**-x/2, e**-x/2
   fdiv  _2_                  ; e**x/2,  e**-x/2, e**-x/2
   fadd  st(2), st            ; e**x/2,  e**-x/2, cosh(x)
   fsubr                      ; sinh(x), cosh(x)

   mov   bx, sinh             ; sinh, cosh
   fstp  QWORD PTR [bx]       ; cosh
   mov   bx, cosh
   fstp  QWORD PTR [bx]       ; <empty>

   pop   Control
   fldcw Control              ; Restore control word
   ret
FPUsinhcosh    ENDP


FPUsincos  PROC  x:word, sinx:word, cosx:word
LOCAL Status:word
   mov   bx, x
   fld   QWORD PTR [bx]       ; x

   cmp   fpu, 387
   jne   Use387FPUsincos

   call  _Loaded387sincos     ; cos(x), sin(x)
   mov   bx, cosx
   fstp  QWORD PTR [bx]       ; sin(x)
   mov   bx, sinx
   fstp  QWORD PTR [bx]       ; <empty>
   ret

Use387FPUsincos:

   sub   sp, 8                ; save 'x' on the CPU stack
   mov   bx, sp
   fstp  QWORD PTR [bx]       ; FPU stack:  <empty>

   call  cos

   add   sp, 8                ; take 'cos(x)' off the CPU stack
   mov   bx, ax
   cmp   compiled_by_turboc,0
   jne   turbo_c1

   fld   QWORD PTR [bx]       ; FPU stack:  cos(x)

turbo_c1:
   fld   st                   ; FPU stack:  cos(x), cos(x)
   fmul  st, st               ; cos(x)**2, cos(x)
   fsubr _1_                  ; sin(x)**2, cos(x)
   fsqrt                      ; +/-sin(x), cos(x)

   mov   bx, x
   fld   QWORD PTR [bx]       ; x, +/-sin(x), cos(x)
   fldpi                      ; Pi, x, +/-sin(x), cos(x)
   fadd  st, st               ; 2Pi, x, +/-sin(x), cos(x)
   fxch                       ; |x|, 2Pi, +/-sin(x), cos(x)
   fprem                      ; Angle, 2Pi, +/-sin(x), cos(x)
   fstp  st(1)                ; Angle, +/-sin(x), cos(x)
   fldpi                      ; Pi, Angle, +/-sin(x), cos(x)

   cmp   BYTE PTR [bx+7], 0
   jns   SignAlignedPi

   fchs                       ; -Pi, Angle, +/-sin(x), cos(x)

SignAlignedPi:
   fcompp                     ; +/-sin(x), cos(x)
   fstsw Status               ; +/-sin(x), cos(x)

   mov   ax, Status
   and   ah, 1
   jz    StoreSinCos          ; Angle <= Pi

   fchs                       ; sin(x), cos(x)

StoreSinCos:
   mov   bx, sinx
   fstp  QWORD PTR [bx]       ; cos(x)
   mov   bx, cosx
   fstp  QWORD PTR [bx]       ; <empty>
   ret
FPUsincos   ENDP


PUBLIC r16Mul
r16Mul     PROC    uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2

      xor   ax, ax
      shl   bx, 1
      jz    Exitr16Mult          ; Destination is zero

      rcl   ah, 1
      shl   cx, 1
      jnz   Chkr16Exp
      xor   bx, bx               ; Source is zero
      xor   si, si
      jmp   Exitr16Mult

   Chkr16Exp:
      rcl   al, 1
      xor   ah, al               ; Resulting sign in ah
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, 127              ; Determine resulting exponent
      add   bh, ch
      mov   al, bh
      mov   fake_es, ax          ; es has the resulting exponent and sign

      mov   ax, di
      mov   al, ah
      mov   ah, cl

      mov   dx, si
      mov   dl, dh
      mov   dh, bl

      mul   dx
      mov   cx, fake_es

      shl   ax, 1
      rcl   dx, 1
      jnc   Remr16MulOneBit      ; 'One' bit is the next bit over

      inc   cl                   ; 'One' bit removed with previous shift
      jmp   Afterr16MulNorm

   Remr16MulOneBit:
      shl   ax, 1
      rcl   dx, 1

   Afterr16MulNorm:
      mov   bl, dh               ; Perform remaining 8 bit shift
      mov   dh, dl
      mov   dl, ah
      mov   si, dx
      mov   bh, cl               ; Put in the exponent
      rcr   ch, 1                ; Get the sign
      rcr   bx, 1                ; Normalize the result
      rcr   si, 1
   Exitr16Mult:
      mov   ax, si
      mov   dx, bx
      ret
r16Mul      ENDP


PUBLIC RegFloat2Fg
RegFloat2Fg     PROC    x1:word, x2:word, Fudge:word
      mov   ax, WORD PTR x1
      mov   dx, WORD PTR x2
      mov   bx, ax
      or    bx, dx
      jz    ExitRegFloat2Fg

      xor   bx, bx
      mov   cx, bx

      shl   ax, 1
      rcl   dx, 1
      rcl   bx, 1                   ; bx contains the sign

      xchg  cl, dh                  ; cx contains the exponent

      stc                           ; Put in the One bit
      rcr   dl, 1
      rcr   ax, 1

      sub   cx, 127 + 23
      add   cx, Fudge
      jz    ChkFgSign
      jns   ShiftFgLeft

      neg   cx
   ShiftFgRight:
      shr   dx, 1
      rcr   ax, 1
      loop  ShiftFgRight
      jmp   ChkFgSign

   ShiftFgLeft:
      shl   ax, 1
      rcl   dx, 1
      loop  ShiftFgLeft

   ChkFgSign:
      or    bx, bx
      jz    ExitRegFloat2Fg

      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0

   ExitRegFloat2Fg:
      ret
RegFloat2Fg    ENDP



PUBLIC RegFg2Float
RegFg2Float     PROC   x1:word, x2:word, FudgeFact:byte
      mov   ax, x1
      mov   dx, x2

      mov   cx, ax
      or    cx, dx
      jz    ExitFudgedToRegFloat

      mov   ch, 127 + 32
      sub   ch, FudgeFact
      xor   cl, cl
      shl   ax, 1       ; Get the sign bit
      rcl   dx, 1
      jnc   FindOneBit

      inc   cl          ; Fudged < 0, convert to postive
      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0

   FindOneBit:
      shl   ax, 1
      rcl   dx, 1
      dec   ch
      jnc   FindOneBit
      dec   ch

      mov   al, ah
      mov   ah, dl
      mov   dl, dh
      mov   dh, ch

      shr   cl, 1       ; Put sign bit in
      rcr   dx, 1
      rcr   ax, 1

   ExitFudgedToRegFloat:
      ret
RegFg2Float      ENDP


PUBLIC ExpFudged
ExpFudged      PROC	uses si, x_low:word, x_high:word, Fudge:word
LOCAL exp:WORD
      xor   ax, ax
      mov   WORD PTR Ans, ax
      mov   WORD PTR Ans + 2, ax
      mov   ax, WORD PTR x_low
      mov   dx, WORD PTR x_high
      or    dx, dx
      js    NegativeExp

      div   Ln2Fg16
      mov   exp, ax
      or    dx, dx
      jz    Raiseexp

      mov   ax, dx
      mov   si, dx
      mov   bx, 1

   PosExpLoop:
      add   WORD PTR Ans, ax
      adc   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jnz   PosExpLoop

   Raiseexp:
      inc   WORD PTR Ans + 2
      mov   ax, WORD PTR Ans
      mov   dx, WORD PTR Ans + 2
      mov   cx, -16
      add   cx, Fudge
      add   cx, exp
      or    cx, cx
      jz    ExitExpFudged
      jns   LeftShift
      neg   cx

   RightShift:
      shr   dx, 1
      rcr   ax, 1
      loop  RightShift
      jmp   ExitExpFudged

   NegativeExp:
      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0
      div   Ln2Fg16
      neg   ax
      mov   exp, ax

      or    dx, dx
      jz    Raiseexp

      mov   ax, dx
      mov   si, dx
      mov   bx, 1

   NegExpLoop:
      sub   WORD PTR Ans, ax
      sbb   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jz    Raiseexp

      add   WORD PTR Ans, ax
      adc   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jnz   NegExpLoop
      jmp   Raiseexp

   LeftShift:
      shl   ax, 1
      rcl   dx, 1
      loop  LeftShift

   ExitExpFudged:
      ret
ExpFudged      ENDP



PUBLIC   LogFudged
LogFudged      PROC	uses si di, x_low:word, x_high:word, Fudge:word
LOCAL exp:WORD
      xor   bx, bx
      mov   cx, 16
      sub   cx, Fudge
      mov   ax, x_low
      mov   dx, x_high

      or    dx, dx
      jz    ChkLowWord

   Incexp:
      shr   dx, 1
      jz    DetermineOper
      rcr   ax, 1
      inc   cx
      jmp   Incexp

   ChkLowWord:
      or    ax, ax
      jnz   Decexp
      jmp   ExitLogFudged

   Decexp:
      dec   cx                      ; Determine power of two
      shl   ax, 1
      jnc   Decexp

   DetermineOper:
      mov   exp, cx
      mov   si, ax                  ; si =: x + 1
      shr   si, 1
      stc
      rcr   si, 1
      mov   dx, ax
      xor   ax, ax
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1                   ; dx:ax = x - 1
      div   si

      mov   bx, ax                  ; ax, Fudged 16, max of 0.3333333
      shl   ax, 1                   ; x = (x - 1) / (x + 1), Fudged 16
      mul   ax
      shl   ax, 1
      rcl   dx, 1
      mov   ax, dx                  ; dx:ax, Fudged 35, max = 0.1111111
      mov   si, ax                  ; si = (ax * ax), Fudged 19

      mov   ax, bx
   ; bx is the accumulator, First term is x
      mul   si                      ; dx:ax, Fudged 35, max of 0.037037
      mov   fake_es, dx             ; Save high word, Fudged (35 - 16) = 19
      mov   di, 0c000h              ; di, 3 Fudged 14
      div   di                      ; ax, Fudged (36 - 14) = 21
      or    ax, ax
      jz    Addexp

      mov   cl, 5
      shr   ax, cl
      add   bx, ax                  ; bx, max of 0.345679
   ; x = x + x**3/3

      mov   ax, fake_es             ; ax, Fudged 19
      mul   si                      ; dx:ax, Fudged 38, max of 0.004115
      mov   fake_es, dx             ; Save high word, Fudged (38 - 16) = 22
      mov   di, 0a000h              ; di, 5 Fudged 13
      div   di                      ; ax, Fudged (38 - 13) = 25
      or    ax, ax
      jz    Addexp

      mov   cl, 9
      shr   ax, cl
      add   bx, ax
   ; x = x + x**3/3 + x**5/5

      mov   ax, fake_es             ; ax, Fudged 22
      mul   si                      ; dx:ax, Fudged 41, max of 0.0004572
      mov   di, 0e000h              ; di, 7 Fudged 13
      div   di                      ; ax, Fudged (41 - 13) = 28
      mov   cl, 12
      shr   ax, cl
      add   bx, ax

   Addexp:
      shl   bx, 1                   ; bx *= 2, Fudged 16, max of 0.693147
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7)
      mov   cx, exp
      mov   ax, Ln2Fg16            ; Answer += exp * Ln2Fg16
      or    cx, cx
      js    SubFromAns

      mul   cx
      add   ax, bx
      adc   dx, 0
      jmp   ExitLogFudged

   SubFromAns:
      neg   cx
      mul   cx
      xor   cx, cx
      xchg  cx, dx
      xchg  bx, ax
      sub   ax, bx
      sbb   dx, cx

   ExitLogFudged:
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7) + (exp * Ln2Fg16)
      ret
LogFudged      ENDP




PUBLIC LogFloat14
LogFloat14     PROC     x1:word, x2:word
      mov   ax, WORD PTR x1
      mov   dx, WORD PTR x2
      shl   ax, 1
      rcl   dx, 1
      xor   cx, cx
      xchg  cl, dh

      stc
		rcr   dl, 1
		rcr   ax, 1

      sub   cx, 127 + 23
      neg   cx
      push  cx
      push  dx
      push  ax
      call  LogFudged
      add   sp, 6
      ret
LogFloat14     ENDP


PUBLIC RegSftFloat
RegSftFloat     PROC   x1:word, x2:word, Shift:byte
      mov   ax, x1
      mov   dx, x2

      shl   dx, 1
      rcl   cl, 1

      add   dh, Shift

      shr   cl, 1
      rcr   dx, 1

      ret
RegSftFloat      ENDP




PUBLIC RegDivFloat
RegDivFloat     PROC  uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2

      xor   ax, ax
      shl   bx, 1
      jnz   ChkOtherOp
      jmp   ExitRegDiv           ; Destination is zero

   ChkOtherOp:
      rcl   ah, 1
      shl   cx, 1
      jnz   ChkDivExp
      xor   bx, bx               ; Source is zero
      xor   si, si
      jmp   ExitRegDiv

   ChkDivExp:
      rcl   al, 1
      xor   ah, al               ; Resulting sign in ah
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, 127              ; Determine resulting exponent
      sub   bh, ch
      mov   al, bh
      mov   fake_es, ax          ; es has the resulting exponent and sign

      mov   ax, si               ; 8 bit shift, bx:si moved to dx:ax
      mov   dh, bl
      mov   dl, ah
      mov   ah, al
      xor   al, al

      mov   bh, cl               ; 8 bit shift, cx:di moved to bx:cx
      mov   cx, di
      mov   bl, ch
      mov   ch, cl
      xor   cl, cl

      shr   dx, 1
      rcr   ax, 1

      div   bx
      mov   si, dx               ; Save (and shift) remainder
      mov   dx, cx               ; Save the quess
      mov   cx, ax
      mul   dx                   ; Mult quess times low word
      xor   di, di
      sub   di, ax               ; Determine remainder
      sbb   si, dx
      mov   ax, di
      mov   dx, si
      jc    RemainderNeg

      xor   di, di
      jmp   GetNextDigit

   RemainderNeg:
      mov   di, 1                ; Flag digit as negative
      not   ax                   ; Convert remainder to positive
      not   dx
      add   ax, 1
      adc   dx, 0

   GetNextDigit:
      shr   dx, 1
      rcr   ax, 1
      div   bx
      xor   bx, bx
      shl   dx, 1
      rcl   ax, 1
      rcl   bl, 1                ; Save high bit

      mov   dx, cx               ; Retrieve first digit
      or    di, di
      jz    RemoveDivOneBit

      neg   ax                   ; Digit was negative
      neg   bx
      dec   dx

   RemoveDivOneBit:
      add   dx, bx
      mov   cx, fake_es
      shl   ax, 1
      rcl   dx, 1
      jc    AfterDivNorm

      dec   cl
      shl   ax, 1
      rcl   dx, 1

   AfterDivNorm:
      mov   bl, dh               ; Perform remaining 8 bit shift
      mov   dh, dl
      mov   dl, ah
      mov   si, dx
      mov   bh, cl               ; Put in the exponent
      shr   ch, 1                ; Get the sign
      rcr   bx, 1                ; Normalize the result
      rcr   si, 1

   ExitRegDiv:
      mov   ax, si
      mov   dx, bx
      ret
RegDivFloat      ENDP



Term        equ      <ax>
Num         equ      <bx>
Factorial   equ      <cx>
Sin         equ      <si>
Cos         equ      <di>
e           equ      <si>
Inve        equ      <di>
         
SinCos086   PROC     uses si di, LoNum:WORD, HiNum:WORD, SinAddr:WORD, \
                                CosAddr:WORD
   mov   ax, LoNum 
   mov   dx, HiNum
   
   xor   cx, cx
   mov   SinNeg, cx
   mov   CosNeg, cx
   mov   a, cx
   or    dx, dx
   jns   AnglePositive
   
   not   ax
   not   dx
   add   ax, 1
   adc   dx, cx
   mov   SinNeg, 1
      
AnglePositive:
   mov   si, ax
   mov   di, dx
   mul   WORD PTR InvPiFg33
   mov   bx, dx
   mov   ax, di
   mul   WORD PTR InvPiFg33
   add   bx, ax
   adc   cx, dx
   mov   ax, si
   mul   WORD PTR InvPiFg33 + 2
   add   bx, ax
   adc   cx, dx
   mov   ax, di
   mul   WORD PTR InvPiFg33 + 2
   add   ax, cx
   adc   dx, 0

   and   dx, 3
   mov   a, dx

   mov   Num, ax
   mov   Factorial, WORD PTR InvPiFg33 + 2
   mov   one, Factorial
   mov   Cos, Factorial          ; Cos = 1
   mov   Sin, Num                ; Sin = Num
      
LoopIntSinCos:
   TaylorTerm                    ; Term = Num * (x/2) * (x/3) * (x/4) * . . .
   sub   Cos, Term               ; Cos = 1 - Num*(x/2) + (x**4)/4! - . . .
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm
   sub   Sin, Term               ; Sin = Num - Num*(x/2)*(x/3) + (x**5)/5! - . . .
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos
      
   TaylorTerm
   add   Cos, Term
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos
      
   TaylorTerm                    ; Term = Num * (x/2) * (x/3) * . . .
   add   Sin, Term
   cmp   Term, WORD PTR TrigLimit
   jnbe  LoopIntSinCos
      
ExitIntSinCos:
   xor   ax, ax
   mov   cx, ax
   cmp   Cos, WORD PTR InvPiFg33 + 2
   jb    CosDivide               ; Cos < 1.0
      
   inc   cx                      ; Cos == 1.0
   jmp   StoreCos
      
CosDivide:
   mov   dx, Cos
   div   WORD PTR InvPiFg33 + 2
      
StoreCos:
   mov   Cos, ax                 ; cx:Cos

   xor   ax, ax
   mov   bx, ax
   cmp   Sin, WORD PTR InvPiFg33 + 2
   jb    SinDivide               ; Sin < 1.0
   
   inc   bx                      ; Sin == 1.0
   jmp   StoreSin
      
SinDivide:
   mov   dx, Sin
   div   WORD PTR InvPiFg33 + 2
      
StoreSin:
   mov   Sin, ax                 ; bx:Sin

   test  a, 1
   jz    ChkNegCos

   xchg  cx, bx
   xchg  Sin, Cos
   mov   ax, SinNeg
   xchg  ax, CosNeg
   mov   CosNeg, ax

ChkNegCos:
   mov   ax, a
   shr   al, 1
   rcl   ah, 1
   xor   ah, al
   jz    ChkNegSin

   xor   CosNeg, 1

ChkNegSin:
   test  a, 2
   jz    CorrectQuad

   xor   SinNeg, 1

CorrectQuad:

   cmp   CosNeg, 1
   jne   CosPolarized

   not   Cos
   not   cx 
   add   Cos, 1
   adc   cx, 0

CosPolarized:     
   mov   dx, bx
   mov   bx, CosAddr
   mov   WORD PTR [bx], Cos
   mov   WORD PTR [bx+2], cx

   cmp   SinNeg, 1
   jne   SinPolarized

   not   Sin
   not   dx
   add   Sin, 1
   adc   dx, 0

SinPolarized:
   mov   bx, SinAddr
   mov   WORD PTR [bx], Sin
   mov   WORD PTR [bx+2], dx
   ret
SinCos086      ENDP
      
      
      
_e2x   PROC
   mov   expSign, 0
   or    dx, dx
   jns   CalcExp
      
   mov   expSign, 1
   not   ax
   not   dx
   add   ax, 1
   adc   dx, 0
   
CalcExp:
   div   Ln2Fg16
   mov   a, ax
   mov   Num, dx                    ; 0 <= Num < Ln(2)
      
   xor   Factorial, Factorial
   stc
   rcr   Factorial, 1
   mov   one, Factorial
   mov   e, Num
   mov   Term, Num
   shr   Num, 1
      
Loop_e2x:
   TaylorTerm
   add   e, Term                 ; e = x + x*x/2 + (x**3)/3! + . . .
   cmp   Term, WORD PTR TrigLimit
   jnbe  SHORT Loop_e2x
      
ExitIntSinhCosh:
   stc
   rcr   e, 1                    ; e = e + 1, fg 15
   ret                           ; return e**x * (2**15), 1 < e**x < 2
_e2x   ENDP
      
      
      
Exp086    PROC     uses si di, LoNum:WORD, HiNum:WORD
   mov   ax, LoNum 
   mov   dx, HiNum
   
   call  _e2x
      
   cmp   a, 16
   jae   Overflow
      
   cmp   expSign, 0
   jnz   NegNumber
      
   mov   ax, e
   mov   dx, ax
   inc   a
   mov   cx, 16
   sub   cx, a
   shr   dx, cl
   mov   cx, a
   shl   ax, cl
   jmp   ExitExp086
      
Overflow:
   xor   ax, ax
   xor   dx, dx
   mov   TrigOverflow, 1
   jmp   ExitExp086
      
NegNumber:
   cmp   e, 8000h
   jne   DivideE
      
   mov   ax, e
   dec   a
   jmp   ShiftE
      
DivideE:
   xor   ax, ax
   mov   dx, ax
   stc
   rcr   dx, 1
   div   e
      
ShiftE:
   xor   dx, dx
   mov   cx, a
   shr   ax, cl
      
ExitExp086:
   ret
Exp086    ENDP



SinhCosh086    PROC     uses si di, LoNum:WORD, HiNum:WORD, SinhAddr:WORD, \
                                   CoshAddr:WORD
   mov   ax, LoNum
   mov   dx, HiNum

   call  _e2x

   cmp   e, 8000h
   jne   InvertE              ; e > 1

   mov   dx, 1
   xor   ax, ax
   cmp   a, 0
   jne   Shiftone

   mov   e, ax
   mov   cx, ax
   jmp   ChkSinhSign

Shiftone:
   mov   cx, a
   shl   dx, cl
   dec   cx
   shr   e, cl
   shr   dx, 1
   shr   e, 1
   mov   cx, dx
   sub   ax, e
   sbb   dx, 0
   xchg  ax, e
   xchg  dx, cx
   jmp   ChkSinhSign

InvertE:
   xor   ax, ax               ; calc 1/e
   mov   dx, 8000h
   div   e

   mov   Inve, ax

ShiftE:
   mov   cx, a
   shr   Inve, cl
   inc   cl
   mov   dx, e
   shl   e, cl
   neg   cl
   add   cl, 16
   shr   dx, cl
   mov   cx, dx               ; cx:e == e**Exp

   mov   ax, e                ; dx:e == e**Exp
   add   ax, Inve
   adc   dx, 0
   shr   dx, 1
   rcr   ax, 1                ; cosh(Num) = (e**Exp + 1/e**Exp) / 2

   sub   e, Inve
   sbb   cx, 0
   sar   cx, 1
   rcr   e, 1

ChkSinhSign:
   or    HiNum, 0
   jns   StoreHyperbolics

   not   e
   not   cx
   add   e, 1
   adc   cx, 0

StoreHyperbolics:
   mov   bx, CoshAddr
   mov   WORD PTR [bx], ax
   mov   WORD PTR [bx+2], dx

   mov   bx, SinhAddr
   mov   WORD PTR [bx], e
   mov   WORD PTR [bx+2], cx

   ret
SinhCosh086    ENDP



Numerator   equ      <bx>
Denominator equ      <di>
Counter     equ      <si>

Log086   PROC     uses si di, LoNum:WORD, HiNum:WORD, Fudge:WORD
LOCAL Exp:WORD, Accum:WORD, LoAns:WORD, HiAns:WORD
      xor   bx, bx
      mov   Accum, bx
      mov   LoAns, bx
      mov   HiAns, bx
      mov   cx, Fudge
      mov   ax, LoNum
      mov   dx, HiNum

      or    dx, dx
      js    Overflow
      jnz   ShiftUp

      or    ax, ax
      jnz   ShiftUp      

   Overflow:
      mov   TrigOverflow, 1
      jmp   ExitLog086

   ShiftUp:
      inc   cx                      ; cx = Exp
      shl   ax, 1
      rcl   dx, 1
      or    dx, dx
      jns   ShiftUp

      neg   cx
      add   cx, 31
      mov   Exp, cx
      mov   di, dx                  ; dx = x, 1 <= x < 2, fg 15
      add   di, 8000h
      rcr   di, 1                   ; di = x + 1, fg 14

      shl   ax, 1
      rcl   dx, 1
      push  ax
      push  dx

      mov   ax, dx
      mul   dx
      mov   Numerator, dx           ; Numerator = (x - 1)**2, fg 16
                                    ; 0 <= cx < 1

      mov   ax, di
      mul   di
      mov   si, dx                  ; Save (x + 1)**2, fg 12
      pop   dx
      pop   ax
      shr   dx, 1
      rcr   ax, 1
      div   di
      mov   LoAns, ax
      push  ax                      ; Rem = (x-1)/(x+1), fg 17
      mov   Denominator, si         ; Denominator = (x + 1)**2, fg 12
                                    ; 1 <= Denominator < 8
      mov   Counter, 3
      mov   cl, 3
      mov   ch, 13

   LogLoop:
      pop   ax                      ; ax = Rem, fg 13+cl
      mul   Numerator               ; dx:ax = Rem * (x-1)**2, fg 29+cl
      shr   dx, 1
      rcr   ax, 1
      div   Denominator
      push  ax                      ; Rem = [Rem*(x-1)**2]/(x+1)**2, fg 17+cl
      xor   dx, dx
      div   Counter                 ; ax = Rem / Counter, fg 17+cl
      mov   dx, ax
      shr   ax, cl                  ; ax = Rem / Counter, fg 17
      cmp   ax, WORD PTR TrigLimit
      jbe   SHORT MultExp

      xchg  cl, ch
      shl   dx, cl
      add   Accum, dx
      adc   LoAns, ax
      adc   HiAns, 0
      inc   Counter
      inc   Counter
      xchg  cl, ch
      add   cl, 3
      sub   ch, 3
      jmp   LogLoop

   MultExp:
      pop   cx                      ; Discard Rem
      mov   cx, Exp
      mov   ax, Ln2Fg16
      or    cx, cx
      js    SubFromAns

      mul   cx                      ; cx = Exp * Lg2Fg16, fg 16
      add   LoAns, ax
      adc   HiAns, dx
      jmp   ExitLog086

   SubFromAns:
      neg   cx
      mul   cx
      sub   LoAns, ax
      sbb   HiAns, dx

   ExitLog086:
      ; r = (x-1)/(x+1)
      ; Ans = 2 * (r + r**3/3 + r**5/5 + . . .) + (Exp * Ln2Fg16), fg 16
      mov   ax, LoAns
      mov   dx, HiAns
      ret
Log086   ENDP


END
