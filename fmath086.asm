
; FMath086.asm (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
;    All rights reserved.
;
;  Code may be used in any program provided the author is credited
;    either during program execution or in the documentation.  Source
;    code may be distributed only in combination with public domain or
;    shareware source code.  Source code may be modified provided the
;    copyright notice and this message is left unchanged and all
;    modifications are clearly documented.
;
;    I would appreciate a copy of any work which incorporates this code,
;    however this is optional.
;
;    Mark C. Peterson
;    128 Hamden Ave., F
;    Waterbury, CT 06704
;    (203) 754-1162

.model medium, c
.8086

.code

PUBLIC RegAddFloat
RegAddFloat     PROC    uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2
   
      xor   ax, ax
      shl   cx, 1
      jnz   SaveSignBit          ; Anything plus zero is zero
      jmp   ExitRegAdd

   SaveSignBit:
      rcr   ah, 1                ; ah has the cx:di sign bit
   
      shl   bx, 1
      jnz   ChkAddExp
      mov   si, y1               ; Destination is zero
      mov   bx, y2 
      jmp   ExitRegAdd

   ChkAddExp:
      rcr   al, 1                ; al has the bx:si sign bit
      stc                        ; Put in the 'one' bit
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, bh
      jz    BothSameMag
      jnc   SwapArguments
   
      neg   ch
      jmp   ChkMagDiff

   SwapArguments:
      add   bh, ch
      xchg  ah, al
      xchg  bl, cl   
      xchg  si, di
   
   ChkMagDiff:
      cmp   ch, 24
      jns   AfterAddNorm
   
   AdjustToSameMag:
      shr   cl, 1
      rcr   di, 1
      dec   ch
      jnz   AdjustToSameMag
   
   BothSameMag:
      xor   ah, al               ;    and both arguments already adjusted
      jnz   AddDifferentSigns
   
      add   si, di
      adc   bl, cl
      jnc   ExitAddSameMag       ; Magnitude did not change
      inc   bh                   ; Update binary exponent
      rcr   bl, 1                ; Divide by two
      rcr   si, 1
      jmp   ExitAddSameMag
   
   AddSignChanged:
      xor   al, 10000000b
      not   si                   ; Negate bl:si
      not   bl
      add   si, 1
      adc   bl, 0
      js    ExitAddSameMag
      jz    AddChkLowWord
      jmp   AddAdjustLoop
   
   AddDifferentSigns:
      sub   si, di
      sbb   bl, cl
      jc    AddSignChanged
      js    ExitAddSameMag
      jnz   AddAdjustLoop
   
   AddChkLowWord:                      ; We know bl is zero
      or    si, si
      jnz   AddAdjustLoop
   
      xor   bh, bh                     ; Result is zero
      jmp   ExitAddSameMag
   
   AddAdjustLoop:                      ; Adjust to 1 < mantisa < 2
      dec   bh
      shl   si, 1
      rcl   bl, 1
      jnc   AddAdjustLoop
      inc   bh
      jmp   PutInSign
   
   ExitAddSameMag:
   
   AfterAddNorm:
      shl   bl, 1                      ; Remove 'one' bit

   PutInSign:
      rcl   al, 1                      ; Put sign bit back into number
      rcr   bx, 1
   ExitRegAdd:
      mov   ax, si
      mov   dx, bx
      ret
RegAddFloat      ENDP




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
      mov   es, ax               ; es has the resulting exponent and sign

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
      mov   cx, es
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




PUBLIC RegMulFloat
RegMulFloat     PROC  uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2

      xor   ax, ax
      shl   bx, 1
      jz    ExitRegMult          ; Destination is zero

      rcl   ah, 1
      shl   cx, 1
      jnz   ChkMultExp
      xor   bx, bx               ; Source is zero
      xor   si, si
      jmp   ExitRegMult

   ChkMultExp:
      rcl   al, 1
      xor   ah, al               ; Resulting sign in ah
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, 127              ; Determine resulting exponent
      add   bh, ch
      mov   al, bh
      mov   es, ax               ; es has the resulting exponent and sign

      mov   ax, di
      mul   si                   ; (di) x (si)
      shl   ax, 1                ; round-off and drop LSW
      adc   dx, 0
      mov   ax, di
      mov   di, dx
      xor   bh, bh               ; Clear bh exponent
      mul   bx                   ; (di) x (bl)
      add   di, ax
      adc   dl, 0
      mov   al, bl
      mov   bl, dl
      mul   cl                   ; (cl) x (bl)
      add   bx, ax               ; bh already zeroed
      xor   ch, ch
      mov   ax, si
      mul   cx                   ; (cl) x (si)
      mov   cx, es
      add   ax, di
      adc   dx, bx
      shl   ax, 1
      rcl   dx, 1
      jnc   RemoveMulOneBit      ; 'One' bit is the next bit over

      inc   cl                   ; 'One' bit removed with previous shift
      jmp   AfterMultNorm

   RemoveMulOneBit:
      shl   ax, 1
      rcl   dx, 1

   AfterMultNorm:
      mov   bl, dh               ; Perform remaining 8 bit shift
      mov   dh, dl
      mov   dl, ah
      mov   si, dx
      mov   bh, cl               ; Put in the exponent
      rcr   ch, 1                ; Get the sign
      rcr   bx, 1                ; Normalize the result
      rcr   si, 1
   ExitRegMult:
      mov   ax, si
      mov   dx, bx
      ret
RegMulFloat      ENDP



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



PUBLIC RegSqrFloat
RegSqrFloat     PROC   uses si di, x1:word, x2:word
      mov   si, x1
      mov   bx, x2

      xor   ax, ax
      shl   bx, 1
      jnz   PutInOneBit          ; Destination is zero

      xor   dx, dx
      jmp   ExitRegSqr

   PutInOneBit:
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      mov   ch, bh
      sub   ch, 127              ; Determine resulting exponent
      add   ch, bh

      mov   ax, si
      mul   si                   ; (si) x (si)
      shl   ax, 1                ; round-off and drop LSW
      adc   dx, 0
      mov   ax, si
      mov   di, dx
      xor   bh, bh               ; Clear bh exponent
      mul   bx                   ; 2 x (si) x (bl)
      shl   ax, 1
      rcl   dx, 1
      add   di, ax
      adc   dl, bh               ; bh is zero
      mov   al, bl
      mul   bl                   ; (bl) x (bl)
      add   dx, ax
      mov   ax, di

      shl   ax, 1
      rcl   dx, 1
      jnc   RemoveSqrOneBit      ; 'One' bit is the next bit over

      inc   ch                   ; 'One' bit removed with previous shift
      jmp   AfterSqrNorm

   RemoveSqrOneBit:
      shl   ax, 1
      rcl   dx, 1

   AfterSqrNorm:
      mov   al, ah
      mov   ah, dl
      mov   dl, dh
      mov   dh, ch
      shr   dx, 1
      rcr   ax, 1

   ExitRegSqr:
      ret
RegSqrFloat      ENDP



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
      mov   es, ax               ; es has the resulting exponent and sign

      mov   ax, di
      mov   al, ah
      mov   ah, cl

      mov   dx, si
      mov   dl, dh
      mov   dh, bl

      mul   dx
      mov   cx, es

      shl   ax, 1
      rcl   dx, 1
      jnc   Remr16MulOneBit      ; 'One' bit is the next bit over

      inc   cl                   ; 'One' bit removed with previous shift
      jmp   Afterr16MulNorm

   Remr16MulOneBit:
      shl   ax, 1
      rcl   dx, 1

   Afterr16mulNorm:
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


PUBLIC r16Sqr
r16Sqr     PROC   uses si di, x1:word, x2:word
      mov   dx, x1
      mov   bx, x2

      xor   ax, ax
      shl   bx, 1
      jnz   PutInr16OneBit       ; Destination is zero

      xor   dx, dx
      jmp   Exitr16Sqr

   PutInr16OneBit:
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      mov   ch, bh
      sub   ch, 127              ; Determine resulting exponent
      add   ch, bh

      mov   dl, dh
      mov   dh, bl
      mov   ax, dx
      mul   dx

      shl   ax, 1
      rcl   dx, 1
      jnc   Remr16SqrOneBit      ; 'One' bit is the next bit over

      inc   ch                   ; 'One' bit removed with previous shift
      jmp   Afr16SqrNorm

   Remr16SqrOneBit:
      shl   ax, 1
      rcl   dx, 1

   Afr16SqrNorm:
      mov   al, ah
      mov   ah, dl
      mov   dl, dh
      mov   dh, ch
      shr   dx, 1
      rcr   ax, 1

   Exitr16Sqr:
      ret
r16Sqr      ENDP



; FastTrig by Mark C. Peterson, [70441,3353]         June 22, 1989

.model medium, c
.8086

.data

HighWord    dw    ?
Sign        db    ?

.code

PUBLIC   FastCosine
FastCosine     PROC     uses si di, x:word
      mov   di, 2000h               ; Seed accumulator with fudged '1'
      mov   bx, 2000h / 4
      mov   cx, 4000h / 4
      mov   si, x
      or    si, si
      jns   XisPositive
      neg   si

   XisPositive:
      mov   ax, si
      shl   ax, 1
      mul   ax
      mov   ax, dx

   CosineLoop:
      sub   di, ax                  ; di -= ax

      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx                  ;  ax *= (x / ++n)
      div   cx
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jz    ExitCosine

      add   di, ax                  ; di += ax
      
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jnz   CosineLoop

   ExitCosine:
      mov   ax, di
      ret
FastCosine  ENDP



PUBLIC FastHypCosine
FastHypCosine     PROC     uses si di, x:word
      xor   ax, ax
      mov   HighWord, ax
      mov   di, 2000h               ; Seed accumulator with fudged '1'
      mov   bx, 2000h / 4
      mov   cx, 4000h / 4
      mov   si, x
      or    si, si
      jns   HypXisPositive
      neg   si

   HypXisPositive:
      mov   ax, si
      shl   ax, 1
      mul   ax
      mov   ax, dx

   HypCosineLoop:
      add   di, ax                  ; di -= ax
      adc   HighWord, 0

      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx                  ;  ax *= (x / ++n)
      div   cx
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jz    ExitHypCosine

      add   di, ax                  ; di += ax
      adc   HighWord, 0
      
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jnz   HypCosineLoop

   ExitHypCosine:
      mov   ax, di
      mov   dx, HighWord
      ret
FastHypCosine     ENDP




PUBLIC FastSine
FastSine    PROC        uses si di, x:word
      mov   bx, 2000h / 4
      mov   cx, 4000h / 4
      mov   si, WORD PTR x
      mov   Sign, 0
      or    si, si
      jns   SineXisPositive
      neg   si
      inc   Sign

   SineXisPositive:
      mov   di, si                  ; Seed accumulator with 'x'
      mov   ax, si
      shl   ax, 1
      mul   ax                      ; ax = x*x/2
      mov   ax, dx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx                      ; di = x, ax = x**3/3!


   SineLoop:
      sub   di, ax

      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jz    ExitSine

      add   di, ax

      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jnz   SineLoop

   ExitSine:
      mov   ax, di

      cmp   Sign, 0
      jz    SineSignSet
      neg   ax

   SineSignSet:
      ret
FastSine    ENDP


PUBLIC FastHypSine
FastHypSine    PROC     uses si di, x:word
      xor   ax, ax
      mov   HighWord, ax
      mov   bx, 2000h / 4
      mov   cx, 4000h / 4
      mov   si, WORD PTR x
      mov   Sign, 0
      or    si, si
      jns   HypSineXisPositive
      neg   si
      inc   Sign

   HypSineXisPositive:
      mov   di, si                  ; Seed accumulator with 'x'
      mov   ax, si
      shl   ax, 1
      mul   ax                      ; ax = x*x/2
      mov   ax, dx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx                      ; di = x, ax = x**3/3!


   HypSineLoop:
      add   di, ax
      adc   HighWord, 0

      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jz    ExitHypSine

      add   di, ax
      adc   HighWord, 0

      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      mul   si
      sar   dx, 1
      rcr   ax, 1
      sar   dx, 1
      rcr   ax, 1
      add   cx, bx
      div   cx
      or    ax, ax
      jnz   HypSineLoop

   ExitHypSine:
      mov   ax, di
      mov   dx, HighWord

      cmp   Sign, 0
      jz    HypSineSignSet
      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0      

   HypSineSignSet:
      ret
FastHypSine    ENDP


; LogFudge by Mark C. Peterson, [70441,3353]         June 30, 1989

.model medium, c
.8086

.data

PwrTwo      dw    ?
Ans         dd    ?
LogTwo16    dw    45426       ; (int)(log(2.0) * (1 << 16)

.code

PUBLIC   LogFudged, LogTwo16
LogFudged      PROC     uses si di es, x_low:word, x_high:word, fudge:word
      xor   bx, bx
      mov   cx, 16
      sub   cx, Fudge
      mov   ax, x_low
      mov   dx, x_high

      or    dx, dx
      jz    ChkLowWord

   IncPwrTwo:
      shr   dx, 1
      jz    DetermineOper
      rcr   ax, 1
      inc   cx
      jmp   IncPwrTwo

   ChkLowWord:
      or    ax, ax
      jnz   DecPwrTwo
      jmp   ExitLogFudged

   DecPwrTwo:
      dec   cx                      ; Determine power of two
      shl   ax, 1
      jnc   DecPwrTwo

   DetermineOper:
      mov   PwrTwo, cx
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

      mov   bx, ax                  ; ax, fudged 16, max of 0.3333333
      shl   ax, 1                   ; x = (x - 1) / (x + 1), fudged 16
      mul   ax
      shl   ax, 1
      rcl   dx, 1
      mov   ax, dx                  ; dx:ax, fudged 35, max = 0.1111111
      mov   si, ax                  ; si = (ax * ax), fudged 19

      mov   ax, bx
   ; bx is the accumulator, First term is x
      mul   si                      ; dx:ax, fudged 35, max of 0.037037
      mov   es, dx                  ; Save high word, fudged (35 - 16) = 19
      mov   di, 0c000h              ; di, 3 fudged 14
      div   di                      ; ax, fudged (36 - 14) = 21
      or    ax, ax
      jz    AddPwrTwo

      mov   cl, 5
      shr   ax, cl
      add   bx, ax                  ; bx, max of 0.345679
   ; x = x + x**3/3

      mov   ax, es                  ; ax, fudged 19
      mul   si                      ; dx:ax, fudged 38, max of 0.004115
      mov   es, dx                  ; Save high word, fudged (38 - 16) = 22
      mov   di, 0a000h              ; di, 5 fudged 13
      div   di                      ; ax, fudged (38 - 13) = 25
      or    ax, ax
      jz    AddPwrTwo

      mov   cl, 9
      shr   ax, cl
      add   bx, ax
   ; x = x + x**3/3 + x**5/5

      mov   ax, es                  ; ax, fudged 22
      mul   si                      ; dx:ax, fudged 41, max of 0.0004572
      mov   di, 0e000h              ; di, 7 fudged 13
      div   di                      ; ax, fudged (41 - 13) = 28
      mov   cl, 12
      shr   ax, cl
      add   bx, ax

   AddPwrTwo:
      shl   bx, 1                   ; bx *= 2, fudged 16, max of 0.693147
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7)
      mov   cx, PwrTwo
      mov   ax, LogTwo16            ; Answer += PwrTwo * LogTwo16
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
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7) + (PwrTwo * LogTwo16)
      ret
LogFudged      ENDP




PUBLIC ExpFudged
ExpFudged      PROC     uses si, x_low:word, x_high:word, Fudge:word
      xor   ax, ax
      mov   WORD PTR Ans, ax
      mov   WORD PTR Ans + 2, ax
      mov   ax, WORD PTR x_low
      mov   dx, WORD PTR x_high
      or    dx, dx
      js    NegativeExp

      div   LogTwo16
      mov   PwrTwo, ax
      or    dx, dx
      jz    RaisePwrTwo

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

   RaisePwrTwo:
      inc   WORD PTR Ans + 2
      mov   ax, WORD PTR Ans
      mov   dx, WORD PTR Ans + 2
      mov   cx, -16
      add   cx, Fudge
      add   cx, PwrTwo
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
      div   LogTwo16
      neg   ax
      mov   PwrTwo, ax

      or    dx, dx
      jz    RaisePwrTwo

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
      jz    RaisePwrTwo

      add   WORD PTR Ans, ax
      adc   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jnz   NegExpLoop
      jmp   RaisePwrTwo

   LeftShift:
      shl   ax, 1
      rcl   dx, 1
      loop  LeftShift

   ExitExpFudged:
      ret
ExpFudged      ENDP



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


END

