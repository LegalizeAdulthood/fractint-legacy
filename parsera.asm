   PAGE          ,132

;    Name: PARSERA.ASM
;  Author: Chuck Ebbert  CompuServe [76306,1226]
;                         internet: 76306.1226@compuserve.com
;    Date: 12 July 1993

; Fast floating-point routines for Fractint.

;   Copyright (C) 1992, 1993 Chuck Ebbert.  All rights reserved.

; This program is an assembler version of the C 'execution engine' part
;    of Mark Peterson's FRACTINT Formula Parser.  Many of the operator
;    functions were copied from Mark's code in the files FPU087.ASM
;    and FPU387.ASM.  The basic operator functions are assembler versions
;    of the code in PARSER.C.  Many 'combined' operators functions were
;    added to the program as well.

;    This code may be freely distributed and used in non-commercial
;    programs provided the author is credited either during program
;    execution or in the documentation, and this copyright notice
;    is left intact.  Sale of this code, or its use in any commercial
;    product requires permission from the author.  Nominal distribution
;    and handling fees may be charged by shareware and freeware
;    distributors.

;       Chuck Ebbert
;       1915 Blust Ln.
;       Enola, PA  17025

   .386                                ; this only works on a 386
   .387                                ;  with a 387

ifdef ??version
   masm51
   quirks
endif

ARGSZ              equ 16              ; size of complex arg
;;;ARGSZ              equ 32              ; size of hypercomplex arg
CPFX               equ 4               ; size of constarg prefix
CARG               equ CPFX+ARGSZ      ; size of constarg
LASTSQR            equ CARG*4+CPFX     ; offset of lastsqr from start of v

; ---------------------------------------------------------------------------
FRAME              MACRO regs          ; build a stack frame
      push         bp
      mov          bp, sp
   IRP             reg, <regs>
      push         reg
      ENDM
   ENDM

UNFRAME            MACRO regs          ; unframe before return
   IRP             reg, <regs>
      pop          reg
      ENDM
      pop          bp
   ENDM

; ---------------------------------------------------------------------------
; Pop a number of scalars from the FPU stack.
; Generate as many 'fcompp' instr.'s as possible.
; Then a 'fstp st(0)' if needed.
POP_STK            MACRO StkPop
   NumToPop        = StkPop SHR 1
   REPT            NumToPop
      fcompp
      ENDM
   NumToPop        = StkPop - ( NumToPop SHL 1 )
   REPT            NumToPop
      fstp         st(0)
      ENDM
   ENDM

; Uncomment the following line to enable compiler code generation.
;;;COMPILER           EQU 1
; ---------------------------------------------------------------------------
; Generate beginning code for operator fn.
BEGN_OPER          MACRO OperName
   ifdef           COMPILER
;; generate the fixups for compiler
;; size of fn.
      db           Size_&OperName
;; offset of x fixup
      db           XFixup_&OperName
;; offset of y fixup
      db           YFixup_&OperName
;; offset of included(called) fn
      db           Incl_&OperName
;; addr of fn to include
      dw           IAddr_&OperName
;; size of fn to include
      db           ILen_&OperName
   else

;; only align when no compiler
   align           4
   endif

;; always generate public and begin of proc
   public          _fStk&OperName
_fStk&OperName     proc near
   ENDM

; ---------------------------------------------------------------------------
; Generate end of operator fn. code.
;
END_OPER           MACRO OperName
   ifndef          COMPILER
;; gen a return instr.
      ret
   else

;; gen a jump label
End_&OperName:
;; generate zero for fixups not generated during fn.
   ifndef          Incl_&OperName
;; No included operator. Generate zero offset, address, and size.
Incl_&OperName     EQU 0
IAddr_&OperName    EQU 0
ILen_&OperName     EQU 0
   endif
   ifndef          XFixup_&OperName
;; No X fixup.
XFixup_&OperName   EQU 0
   endif
   ifndef          YFixup_&OperName
;; No Y fixup
YFixup_&OperName   EQU 0
   endif
   endif

;; Always gen size of fn
Size_&OperName     EQU $ - _fStk&OperName
;; and end of procedure.
_fStk&OperName     endp
   ENDM

; ---------------------------------------------------------------------------
; Generate beginning code for 'included' operator fn.
; No fixups here.
BEGN_INCL          MACRO OperName
   ifndef          COMPILER
;; No align for 'compiler' mode.
   align           4
   endif

;; Generate public (incl fns. can be called directly) and begin of proc.
   public          _fStk&OperName
_fStk&OperName     proc near
   ENDM

; ---------------------------------------------------------------------------
; Generate end of 'included' operator fn. code.
END_INCL           MACRO OperName
   ifndef          COMPILER
;; generate return
      ret
   else

;; generate label for jump to end of fn.
End_&OperName:
   endif

;; always generate actual size of fn.
   Size_&OperName  EQU $ - _fStk&OperName
;; always generate end-of-proc
_fStk&OperName     endp
   ENDM

; ---------------------------------------------------------------------------
; 'Include' a function inside another one
INCL_OPER          MACRO CallingOper,OperToIncl
   ifdef           COMPILER
;; Offset of include in outer fn.
Incl_&CallingOper  EQU $ - _fStk&CallingOper
;; Address of included fn.
IAddr_&CallingOper EQU _fStk&OperToIncl
;; Length of included fn.
ILen_&CallingOper  EQU Size_&OperToIncl
   else

;; Generate a call to the included fn.
      call         _fStk&OperToIncl
   endif
   ENDM

; ---------------------------------------------------------------------------
; Exit early from an operator function.
EXIT_OPER          MACRO FnToExit
   ifdef           COMPILER
;; jump to end of operator fn
      jmp          short End_&FnToExit
   else

;; return to caller
      ret
   endif
   ENDM

; ---------------------------------------------------------------------------
; Generate an FPU instruction and a fixup.
; AddrToFix is = X or Y
FIXUP              MACRO OperName, InstrToFix, Addr
   ifdef           COMPILER

;; Generate a fixup as an offset from start of fn.
;; This is why no includes allowed before a fixup.
;; Fixup is two bytes into the instruction, thus the '+ 2'.
;; This may not be true for all instructions.
   ifidni          <Addr>, <X>
XFixup_&OperName   EQU $ - _fStk&OperName + 2
   else
;; assume fixup is for y
YFixup_&OperName   EQU $ - _fStk&OperName + 2
   endif
;; Generate a load, store or whatever of any convenient value using DS.
      &InstrToFix  QWORD PTR ds:_fLastOp
   else

   ifidni          <Addr>, <X>
;; Gen load of X using SI.
      &InstrToFix  QWORD PTR [si]
   else
;; Assume fixup is for y, use SI+8.
      &InstrToFix  QWORD PTR [si+8]
   endif
   endif

   ENDM

; ---------------------------------------------------------------------------
; Align 4 if no compiler.
PARSALIGN          macro AlignFn
   ifndef          COMPILER
   align           4
   endif
   ENDM

; ---------------------------------------------------------------------------
; external functions
   extrn           _TranspPerPixel:far

; ---------------------------------------------------------------------------
_DATA              segment word public use16 'DATA'
   extrn           _maxit:WORD
   extrn           _inside:WORD
   extrn           _outside:WORD
   extrn           _color:WORD
   extrn           _realcolor:WORD
   extrn           _kbdcount:WORD      ; keyboard counter
   extrn           _dotmode:WORD
   extrn           __1_:QWORD, _PointFive:QWORD, __2_:QWORD, _infinity:QWORD
   extrn           _LastOp:WORD, _LastInitOp:WORD
   extrn           _InitOpPtr:WORD, _InitStoPtr:WORD, _InitLodPtr:WORD
   extrn           _s:WORD
   extrn           _OpPtr:WORD, _LodPtr:WORD, _StoPtr:WORD
   extrn           _Load:DWORD, _Store:DWORD
   extrn           _FormName:byte
   extrn           _dy1:DWORD, _dx1:DWORD, _dy0:DWORD, _dx0:DWORD
   extrn           _new:WORD, _old:WORD
   extrn           _overflow:WORD
   extrn           _col:WORD, _row:WORD
   extrn           _Transparent3D:WORD
   extrn           _Arg1:WORD, _Arg2:WORD
   extrn           _f:DWORD, _pfls:DWORD, _v:DWORD
_DATA               ends

_BSS               segment word public use16 'BSS'
_fLastOp           label DWORD         ; save seg, offset of lastop here
      dd           ?
_PtrToZ            label WORD
      dw           ?
_BSS               ends

DGROUP             group _DATA,_BSS

; ---------------------------------------------------------------------------
; Operator Functions follow.
; ---------------------------------------------------------------------------

; NOTE: None of these operator functions may change any registers but
;       ax and si.  The exceptions are those functions that update
;       the current values of the 'status' regs as needed.

;  On entry to these functions:
;   FPU stack is used as the evaluation stack.
;         The FPU stack can overflow into memory.  Accuracy is not lost but
;         calculations are slower.
;   es -> DGROUP
;   ds -> seg pfls, seg v
;   cx -> lastop
;   dx == orbit counter (if calcfrmfpasm is running)
;   di -> stack overflow area, used by push and pull functions
;   bx -> current operator, operand pair
;    [bx] = operator function address, i.e. addr. of current '_fStkXXX'
;    [bx+2] = operand pointer or zero if no operand
;   si = operand pointer (loaded from [bx+2] before call of operator fn.)

; New rules Feb 1993:
;  1. No EXIT_OPER before an INCL_OPER.
;  2. No jumps can be made past an included function.
;  2. No included fn may include another, or have any fixups.
;  3. Only one included fn. allowed per 'normal' fn.
;  4. Fixups must be before any included fn.

; --------------------------------------------------------------------------
   ;  Put this code in PARSERFP.C's code segment.
PARSERFP_TEXT     segment para public use16 'CODE'
   ;  Non-standard segment register setup.
   assume         es:DGROUP, ds:nothing, cs:PARSERFP_TEXT
; --------------------------------------------------------------------------
; Included functions must be first.
; --------------------------------------------------------------------------
   BEGN_INCL       Log                 ; Log
   ; From FPU387.ASM
   ; Log is called by Pwr and is also called directly.
      ftst
      fstsw        ax
      sahf
      jnz          short NotBothZero
      fxch                             ; y x
      ftst
      fstsw        ax
      sahf
      fxch                             ; x y
      jnz          short NotBothZero
      POP_STK      2                   ; clear two numbers
      fldz
      fldz
      EXIT_OPER    Log                 ; return (0,0)
   PARSALIGN
NotBothZero:
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      fpatan                           ; z.y x y
      fxch         st(2)               ; y x z.y
      fmul         st,st(0)            ; yy x z.y
      fxch                             ; x yy z.y
      fmul         st,st(0)            ; xx yy z.y
      fadd                             ; mod z.y
      fldln2                           ; ln2, mod, z.y
      fmul         _PointFive          ; ln2/2, mod, z.y
      fxch                             ; mod, ln2/2, z.y
      fyl2x                            ; z.x, z.y
   END_INCL        Log
; --------------------------------------------------------------------------
   BEGN_INCL       SinhCosh            ; Included fn, Sinh, Cosh of st
   ; From FPU087.ASM with mods to use less registers.
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      fldln2                           ; ln(2) x
      fdivp        st(1),st            ; x/ln(2), start the fdivr instr.
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      or           ax,0000110000000000b
      mov          _Arg2,ax
      fldcw        _Arg2               ; Set control to round towards zero
      ftst                             ; save the sign of x in ax
      fstsw        ax                  ; sahf instr. is below
      fabs                             ; |x|/ln2
      fld          st                  ; |x|/ln(2), |x|/ln(2)
      frndint                          ; int = integer(|x|/ln(2)), |x|/ln(2)
      fxch                             ; |x|/ln(2), int
      fsub         st,st(1)            ; rem < 1.0, int
      fmul         _PointFive          ; rem/2 < 0.5, int
      f2xm1                            ; (2**(rem/2))-1, int
      fadd         __1_                ; 2**(rem/2), int
      fmul         st,st               ; 2**rem, int
      sahf                             ; ah has result of ftst above
      fscale                           ; e**|x|, int
      fstp         st(1)               ; e**|x|
      jae          short ExitFexp      ; skip divide if x was >= 0

      fdivr        __1_                ; e**x
   PARSALIGN
ExitFexp:
      fld          st                  ; e**x, e**x
      fmul         _PointFive          ; e^x/2 e^x
      fstp         QWORD PTR es:[di]   ; e^x  use overflow stk for temp here
      fdivr        _PointFive          ; e**-x/2
      pop          ax                  ; restore old CW to Arg2 after fdivr
      mov          _Arg2,ax
      fld          st                  ; e**-x/2, e**-x/2
      fadd         QWORD PTR es:[di]   ; coshx, e**-x/2
      fxch                             ; e^-x/2, coshx
      fsubr        QWORD PTR es:[di]   ; sinhx, coshx
      fldcw        _Arg2               ; Restore control word
   END_INCL        SinhCosh
; --------------------------------------------------------------------------
   BEGN_OPER       Conj                ; Complex conjugate
      fxch                             ; y x ...
      fchs                             ; y=-y x ...
      fxch                             ; x y ...
   END_OPER        Conj
; --------------------------------------------------------------------------
   BEGN_OPER       Real                ; Real
      fstp         st(1)               ; x ...
      fldz                             ; 0 x ...
      fxch                             ; x 0 ...
   END_OPER        Real
; --------------------------------------------------------------------------
   BEGN_OPER       RealFlip            ; Real, flip combined.
      fstp         st(1)               ; y=x ...
      fldz                             ; x=0 y ...
   END_OPER        RealFlip
; --------------------------------------------------------------------------
   BEGN_OPER       Add                 ; Add
      faddp        st(2),st            ; Arg2->d.x += Arg1->d.x;
      faddp        st(2),st            ; Arg2->d.y += Arg1->d.y;
   END_OPER        Add
; --------------------------------------------------------------------------
   BEGN_OPER       Sub                 ; Subtract
      fsubp        st(2),st            ; Arg2->d.x -= Arg1->d.x;
      fsubp        st(2),st            ; Arg2->d.y -= Arg1->d.y;
   END_OPER        Sub
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealAdd          ; Load, Real, Add combined
      FIXUP        LodRealAdd, fadd, X ; Add x-value from memory
   END_OPER        LodRealAdd
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealSub          ; Load, Real, Subtract combined
      FIXUP        LodRealSub, fsub, X ; (fsub qword ptr X)
   END_OPER        LodRealSub
; --------------------------------------------------------------------------
   BEGN_OPER       Real2               ; Real value (fast version)
      fldz                             ; 0 x y ... (uses a reg)
      fstp         st(2)               ; x 0 ...
   END_OPER        Real2
; --------------------------------------------------------------------------
   BEGN_OPER       Lod                 ; Load
      FIXUP        Lod, fld, Y         ; y ...
      FIXUP        Lod, fld, X         ; x y ...
   END_OPER        Lod
; --------------------------------------------------------------------------
   BEGN_OPER       Clr1                ; Clear stack
      fninit
   END_OPER        Clr1
; --------------------------------------------------------------------------
   BEGN_OPER       Imag                ; Imaginary value
      POP_STK      1                   ; y
      fldz                             ; 0 y
      fxch                             ; x=y 0
   END_OPER        Imag
; --------------------------------------------------------------------------
   BEGN_OPER       ImagFlip            ; Imaginary value, flip combined
      POP_STK      1                   ; y ...
      fldz                             ; x=0 y ...
   END_OPER        ImagFlip
; --------------------------------------------------------------------------
   BEGN_OPER Abs                       ; Absolute value
      fxch
      fabs
      fxch
      fabs
   END_OPER Abs
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealMul          ; Load, Real, Multiply
      FIXUP        LodRealMul, fld, X  ; y.x x.x x.y
      fmul         st(2),st            ; y.x x.x z.y
      fmul                             ; z.x z.y
   END_OPER        LodRealMul
; --------------------------------------------------------------------------
   BEGN_OPER       Neg                 ; Negative
      fxch
      fchs                             ; Arg1->d.y = -Arg1->d.y;
      fxch
      fchs
   END_OPER        Neg
; --------------------------------------------------------------------------
   BEGN_OPER       EndInit             ; End of initialization expr.
      fninit
      mov          _LastInitOp,bx      ; LastInitOp=OpPtr
   END_OPER        EndInit
; --------------------------------------------------------------------------
   BEGN_OPER       StoClr1             ; Store, clear FPU
      FIXUP        StoClr1, fstp, X    ; y ...
      FIXUP        StoClr1, fst, Y     ; y ...
      finit                            ; use finit, not fninit
   END_OPER        StoClr1
; --------------------------------------------------------------------------
   BEGN_OPER       Sto                 ; Store, leave on ST
      fxch                             ; y x ...
      FIXUP        Sto, fst, Y
      fxch                             ; x y ...
      FIXUP        Sto, fst, X
      fwait                            ; to be safe
   END_OPER        Sto
; --------------------------------------------------------------------------
   BEGN_OPER       Sto2                ; Store, leave on ST (uses a reg)
      fld          st(1)               ; y x y
      FIXUP        Sto2, fstp, Y       ; x y
      FIXUP        Sto2, fst, X
   ; FWAIT should not be needed here since next operator is never Clr.
   END_OPER        Sto2
; --------------------------------------------------------------------------
   BEGN_OPER       LodReal             ; Load a real
      fldz                             ; 0 ...
      FIXUP        LodReal, fld, X     ; x 0 ...
   END_OPER        LodReal
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealC            ; Load real const
      fldz                             ; y=0 ...
      FIXUP        LodRealC, fld, X    ; x 0 ...
   END_OPER        LodRealC
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealFlip         ; Load real, flip
      FIXUP        LodRealFlip, fld, X ; y=x ...
      fldz                             ; x=0 y ...
   END_OPER        LodRealFlip
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealAbs          ; Load real, abs
      fldz                             ; 0 ...
      FIXUP        LodRealAbs, fld, X  ; x 0 ...
      fabs                             ; x=abs(x) 0 ...
   END_OPER        LodRealAbs
; --------------------------------------------------------------------------
   BEGN_OPER       Flip                ; Exchange real, imag
      fxch                             ; x=y y=x ...
   END_OPER        Flip
; --------------------------------------------------------------------------
   BEGN_OPER       LodImag             ; Load, imaginary
      fldz                             ; 0 ...
      FIXUP        LodImag, fld, Y     ; x=y 0
   END_OPER        LodImag
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagFlip         ; Load, imaginary, flip
      FIXUP        LodImagFlip, fld, Y ; y ...
      fldz                             ; 0 y ...
   END_OPER        LodImagFlip
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagAbs          ; Load, imaginary, absolute value
      fldz                             ; 0 ...
      FIXUP        LodImagAbs, fld, Y  ; x=y 0 ...
      fabs                             ; x=abs(y) 0 ...
   END_OPER        LodImagAbs
; --------------------------------------------------------------------------
   BEGN_OPER       LodConj             ; Load, conjugate
      FIXUP        LodConj, fld, Y     ; y ...
      fchs                             ; y=-y ...
      FIXUP        LodConj, fld, X     ; x y ...
   END_OPER        LodConj
; --------------------------------------------------------------------------
   BEGN_OPER       LodAdd              ; Load, Add (uses a reg)
      FIXUP        LodAdd, fadd, X
      FIXUP        LodAdd, fld, Y
      faddp        st(2),st
   END_OPER        LodAdd
; --------------------------------------------------------------------------
   BEGN_OPER       LodSub              ; Load, Subtract (uses a reg)
      FIXUP        LodSub, fsub, X
      FIXUP        LodSub, fld, Y
      fsubp        st(2),st
   END_OPER        LodSub
; --------------------------------------------------------------------------
   BEGN_OPER       StoDup              ; Store, duplicate top operand
      FIXUP        StoDup, fst, X      ; x y
      fld          st(1)               ; y x y
      FIXUP        StoDup, fst, Y      ; y x y
      fld          st(1)               ; x y x y
   END_OPER        StoDup
; --------------------------------------------------------------------------
   BEGN_OPER       StoDbl              ; Store, double (uses a reg)
      FIXUP        StoDbl, fst, X      ; x y (store x)
      fadd         st,st               ; 2x y
      fld          st(1)               ; y 2x y
      FIXUP        StoDbl, fst, Y      ; y 2x y (store y)
      faddp        st(2),st            ; 2x 2y
   END_OPER        StoDbl
; --------------------------------------------------------------------------
   BEGN_OPER       LodSubMod           ; Load, Subtract, Mod
      FIXUP        LodSubMod, fsub, X  ; x.x-y.x  x.y  ...
      fmul         st,st               ; sqr(x.x-y.x) x.y ...
      fldz                             ; 0 sqrx x.y ...
      fxch         st(2)               ; x.y sqrx 0 ...
      FIXUP        LodSubMod, fsub, Y  ; x.y-y.y sqrx 0 ...
      fmul         st,st               ; sqry sqrx 0 ...
      fadd                             ; mod 0
   END_OPER        LodSubMod
; --------------------------------------------------------------------------
   BEGN_OPER       Sqr                 ; Square, save magnitude in LastSqr
      fld          st(0)               ; x x y
      fmul         st(1),st            ; x x*x y
      fmul         st,st(2)            ; xy xx y
      mov          si, WORD PTR _v     ; si -> variables
      fadd         st,st(0)            ; 2xy xx y
      fxch         st(2)               ; y xx 2xy
      fmul         st,st(0)            ; yy xx 2xy
      fld          st(1)               ; xx yy xx 2xy
      fadd         st,st(1)            ; xx+yy yy xx 2xy
      fstp         QWORD PTR [si+LASTSQR] ; yy xx 2xy
      fsubp        st(1),st            ; xx-yy 2xy
   END_OPER        Sqr
; --------------------------------------------------------------------------
   BEGN_OPER       Sqr0                ; Square, don't save magnitude
      fld          st(0)               ; x x y
      fld          st(0)               ; x x x y
      fmul         st,st(3)            ; xy x x y
      fadd         st,st               ; 2xy x x y
      fxch         st(3)               ; y x x 2xy
      fadd         st(2),st            ; y x x+y 2xy
      fsubp        st(1),st            ; x-y x+y 2xy
      fmulp        st(1),st            ; xx-yy 2xy
   END_OPER        Sqr0
; --------------------------------------------------------------------------
   BEGN_OPER       Mul                 ; Multiply
   ; From FPU087.ASM
      fld          st(1)               ; y.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.y*x.y, y.x. y.y, x.x, x.y
      fld          st(1)               ; y.x, y.y*x.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.x*x.x,y.y*x.y,y.x y.y,x.x,x.y
      fsubr                            ; newx=y.x*x.x-y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x, y.x, y.y, newx, x.y
      fmulp        st(2),st            ; y.x, y.y*x.x, newx, x.y
      fmulp        st(3),st            ; y.y*x.x, newx, y.x*x.y
      faddp        st(2),st            ; newx newy = y.x*x.y + x.x*y.y
   END_OPER        Mul
; --------------------------------------------------------------------------
   BEGN_OPER       LodMul              ; Load, Multiply
   ; This is just load followed by multiply but it saves a fn. call.
      FIXUP        LodMul, fld, Y      ; y.y x.x x.y
      FIXUP        LodMul, fld, X      ; y.x y.y x.x x.y
      fld          st(1)               ; y.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.y*x.y, y.x. y.y, x.x, x.y
      fld          st(1)               ; y.x, y.y*x.y, y.x, y.y, x.x, x.y
      fmul         st, st(4)           ; y.x*x.x, y.y*x.y, y.x, y.y, x.x, x.y
      fsubr                            ; newx=y.x*x.x-y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x, y.x, y.y, newx, x.y
      fmulp        st(2), st           ; y.x, y.y*x.x, newx, x.y
      fmulp        st(3), st           ; y.y*x.x, newx, y.x*x.y
      faddp        st(2), st           ; newx newy = y.x*x.y + x.x*y.y
   END_OPER        LodMul
; --------------------------------------------------------------------------
   BEGN_OPER       Div                 ; Divide
   ; From FPU087.ASM with speedups
      fld          st(1)               ; y.y,y.x,y.y,x.x,x.y
      fmul         st,st               ; y.y*y.y,y.x,y.y,x.x,x.y
      fld          st(1)               ; y.x,y.y*y.y,y.x,y.y,x.x,x.y
      fmul         st,st               ; y.x*y.x,y.y*y.y,y.x,y.y,x.x,x.y
      fadd                             ; mod,y.x,y.y,x.x,x.y
      ftst
      fstsw        ax
      sahf
      jz           short DivNotOk
                                       ; can't do this divide until now
      fdiv         st(1),st            ; mod,y.x=y.x/mod,y.y,x.x,x.y
      fdivp        st(2),st            ; y.x,y.y=y.y/mod,x.x,x.y
      fld          st(1)               ; y.y,y.x,y.y,x.x,x.y
      fmul         st,st(4)            ; y.y*x.y,y.x,y.y,x.x,x.y
      fld          st(1)               ; y.x,y.y*x.y,y.x,y.y,x.x,x.y
      fmul         st,st(4)            ; y.x*x.x,y.y*x.y,y.x,y.y,x.x,x.y
      fadd                             ; y.x*x.x+y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x,y.x,y.y,newx,x.y
      fmulp        st(2),st            ; y.x,y.y*x.x,newx,x.y
      fmulp        st(3),st            ; x.x*y.y,newx,y.x*x.y
      fsubp        st(2),st            ; newx,newy
      EXIT_OPER    Div
DivNotOk:
      POP_STK      5                   ; clear 5 from stack (!)
      fld          _infinity           ; return a very large number
      fld          st(0)
   END_OPER        Div
; --------------------------------------------------------------------------
   BEGN_OPER       Recip               ; Reciprocal
   ; From FPU087.ASM
      fld          st(1)               ; y, x, y
      fmul         st,st               ; y*y, x, y
      fld          st(1)               ; x, y*y, x, y
      fmul         st,st               ; x*x, y*y, x, y
      fadd                             ; mod, x, y
      ftst
      fstsw        ax
      sahf
      jz           short RecipNotOk
      fdiv         st(1),st            ; mod, newx=x/mod, y
      fchs                             ; -mod newx y
      fdivp        st(2),st            ; newx, newy=y/-mod
      EXIT_OPER    Recip
RecipNotOk:
      POP_STK      3                   ; clear three from stack
      fld          _infinity           ; return a very large number
      fld          st(0)
   END_OPER        Recip
; --------------------------------------------------------------------------
   BEGN_OPER       StoSqr              ; Sto, Square, save magnitude
      fld          st(0)               ; x x y
      FIXUP        StoSqr, fst, X      ;   "   (store x)
      fmul         st(1),st            ; x x*x y
      fmul         st,st(2)            ; xy xx y
      fadd         st,st(0)            ; 2xy xx y
      fxch         st(2)               ; y xx 2xy
      FIXUP        StoSqr, fst, Y      ;    "     (store y)
      fmul         st,st(0)            ; yy xx 2xy
   ; It is now safe to overlay si here
      mov          si, WORD PTR _v     ; si -> variables
      fld          st(1)               ; xx yy xx 2xy
      fadd         st,st(1)            ; xx+yy yy xx 2xy
      fstp         QWORD PTR [si+LASTSQR] ; yy xx 2xy
      fsubp        st(1),st            ; xx-yy 2xy
   END_OPER        StoSqr
; --------------------------------------------------------------------------
   BEGN_OPER       StoSqr0             ; Sto, Square, don't save magnitude
      fld          st(0)               ; x x y
      FIXUP        StoSqr0, fst, X     ; store x
      fld          st(0)               ; x x x y
      fmul         st,st(3)            ; xy x x y
      fadd         st,st               ; 2xy x x y
      fxch         st(3)               ; y x x 2xy
      FIXUP        StoSqr0, fst, Y     ; store y
      fadd         st(2),st            ; y x x+y 2xy
      fsubp        st(1),st            ; x-y x+y 2xy
      fmulp        st(1),st            ; xx-yy 2xy
   END_OPER        StoSqr0
; --------------------------------------------------------------------------
   BEGN_OPER       Mod2                ; Modulus (uses a reg)
      fmul         st,st               ; xx y
      fldz                             ; 0 xx y
      fxch         st(2)               ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_OPER        Mod2
; --------------------------------------------------------------------------
   BEGN_OPER       LodMod2             ; Load, Modulus (uses a reg)
      fldz                             ; 0 ...
      FIXUP        LodMod2, fld, X     ; x 0 ...
      fmul         st,st               ; xx 0
      FIXUP        LodMod2, fld, Y     ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_OPER        LodMod2
; --------------------------------------------------------------------------
   BEGN_OPER       StoMod2             ; Store, Modulus (uses a reg)
      FIXUP        StoMod2, fst, X     ; x y
      fmul         st,st               ; xx y
      fldz                             ; 0 xx y
      fxch         st(2)               ; y xx 0
      FIXUP        StoMod2, fst, Y     ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_OPER        StoMod2
; --------------------------------------------------------------------------
   BEGN_OPER       Clr2                ; Test ST, clear FPU
      ftst
      fstsw        ax
      fninit
      and          ah,01000000b        ; return 1 if zf=1
      shr          ax,14               ; AX will be returned by fFormula()
   END_OPER        Clr2
; --------------------------------------------------------------------------
   BEGN_OPER       PLodAdd             ; Load, Add (uses no regs)
      fxch                             ; y x
      FIXUP        PLodAdd, fadd, Y    ; add y from memory
      fxch                             ; x y
      FIXUP        PLodAdd, fadd, X    ; add x, overlap execution
   END_OPER        PLodAdd
; --------------------------------------------------------------------------
   BEGN_OPER       PLodSub             ; Load, Subtract (uses no regs)
      fxch
      FIXUP        PLodSub, fsub, Y    ; sub y from memory
      fxch                             ; x y
      FIXUP        PLodSub, fsub, X    ; sub x, overlap execution
   END_OPER        PLodSub
; --------------------------------------------------------------------------
   BEGN_OPER       LodDup              ; Load, duplicate
      FIXUP        LodDup, fld, Y      ; y ...
      FIXUP        LodDup, fld, X      ; x y ...
      fld          st(1)               ; y x y ...
      fld          st(1)               ; x y x y ...
   END_OPER        LodDup
; --------------------------------------------------------------------------
   BEGN_OPER       LodSqr              ; Load, square (no save lastsqr)
      FIXUP        LodSqr, fld, Y      ; y ...
      fld          st(0)               ; y y ...
      fadd         st(1),st            ; y 2y ...
      fld          st(0)               ; y y 2y
      FIXUP        LodSqr, fld, X      ; x y y 2y ...
      fmul         st(3),st            ; x y y 2xy ...
      fadd         st(2),st				; x y X+y 2xy ...
      fsubrp       st(1),st            ; x-y x+y 2xy ...
      fmul                             ; xx-yy 2xy ...
   END_OPER        LodSqr
; --------------------------------------------------------------------------
   BEGN_OPER       LodSqr2             ; Load, square (save lastsqr)
      FIXUP        LodSqr2, fld, Y     ; y ...
      fld          st(0)               ; y y ...
      fadd         st(1),st            ; y 2y ...
      fmul         st,st(0)            ; yy 2y ...
      FIXUP        LodSqr2, fld, X     ; x yy 2y ...
      fmul         st(2),st            ; x yy 2xy ...
      mov          si,WORD PTR _v      ; put address of v in si
      fmul         st,st(0)            ; xx yy 2xy ...
      fld          st(0)               ; xx xx yy 2xy
      fadd         st,st(2)            ; mod xx yy 2xy
      fstp         QWORD PTR [si+LASTSQR] ; xx yy 2xy ... (save lastsqr)
      fsubrp       st(1),st            ; xx-yy 2xy ...
   END_OPER        LodSqr2
; --------------------------------------------------------------------------
   BEGN_OPER       LodDbl              ; Load, double
      FIXUP        LodDbl, fld, Y      ; load y
      fadd         st,st(0)            ; double it
      FIXUP        LodDbl, fld, X      ; same for x
      fadd         st,st(0)
   END_OPER        LodDbl
; --------------------------------------------------------------------------
   BEGN_OPER       Mod                 ; Modulus (uses no regs)
      fmul         st,st               ; x*x y
      fxch                             ; y x*x
      fmul         st,st               ; y*y x*x
      fadd                             ; mod
      fldz                             ; 0 mod
      fxch                             ; mod 0
   END_OPER        Mod
; --------------------------------------------------------------------------
; The following code was 'discovered' by experimentation.  The Intel manuals
;   really don't help much in writing this kind of code.
; --------------------------------------------------------------------------
   BEGN_OPER       Push2               ; Push stack down from 8 to 6
      fdecstp                          ; roll the stack
      fdecstp                          ; ...
      fstp         tbyte PTR es:[di]   ; store x on overflow stack
      fstp         tbyte PTR es:[di+10] ; and y (ten bytes each)
      add          di,20               ; adjust di
   END_OPER        Push2
; --------------------------------------------------------------------------
   BEGN_OPER       Pull2               ; Pull stack up from 2 to 4
      fld          tbyte PTR es:[di-10] ; oldy x y
      sub          di,20               ; adjust di
      fxch         st(2)               ; y x oldy
      fld          tbyte PTR es:[di]   ; oldx y x oldy
      fxch         st(2)               ; x y oldx oldy
   END_OPER        Pull2
; --------------------------------------------------------------------------
   BEGN_OPER       Push4               ; Push stack down from 8 to 4
      fdecstp                          ; roll the stack four times
      fdecstp
      fdecstp
      fdecstp
      fstp         tbyte PTR es:[di+20] ; save the bottom four numbers
      fstp         tbyte PTR es:[di+30] ; save full precision on overflow
      fstp         tbyte PTR es:[di]
      fstp         tbyte PTR es:[di+10]
      add          di,40                ; adjust di
   END_OPER        Push4
; --------------------------------------------------------------------------
   BEGN_OPER       Push2a              ; Push stack down from 6 to 4
      fdecstp                          ; roll the stack 4 times
      fdecstp
      fdecstp
      fdecstp
      fstp         tbyte PTR es:[di]   ; save only two numbers
      fstp         tbyte PTR es:[di+10]
      add          di, 20
      fincstp                          ; roll back 2 times
      fincstp
   END_OPER        Push2a
; --------------------------------------------------------------------------
; End of stack overflow/underflow code.
; --------------------------------------------------------------------------
   BEGN_OPER       Exp                 ; Exponent
   ; From FPU387.ASM with mods to use less registers.
      fstp         QWORD PTR es:[di]   ; y
      fsincos                          ; cosy, siny
      fld1                             ; 1, cos, sin
      fldln2                           ; ln2, 1, cos, sin
      fdivr        QWORD PTR es:[di]   ; x.x/ln2, 1, cos, sin
      fst          QWORD PTR es:[di]
      fprem                            ; prem, 1, cos, sin
      f2xm1                            ; e**prem-1, 1, cos, sin
      fadd                             ; e**prem, cos, sin
      fld          QWORD PTR es:[di]   ; x.x/ln2, e**prem, cos, sin
      fxch                             ; e**prem, x.x/ln2, cos, sin
      fscale                           ; e**x.x, x.x/ln2, cos, sin
      fstp         st(1)               ; e**x.x, cos, sin
      fmul         st(2),st            ; e**x.x, cos, z.y
      fmul                             ; z.x, z.y
   END_OPER        Exp
; --------------------------------------------------------------------------
   BEGN_OPER       Pwr                 ; Power
   ; First exchange the top two complex numbers.
      fxch         st(2)               ; x.x y.y y.x x.y
      fxch                             ; y.y x.x y.x x.y
      fxch         st(3)               ; x.y x.x y.x y.y
      fxch                             ; x.x x.y y.x y.y
   ; Now take the log of the # on st.
      INCL_OPER    Pwr, Log            ; l.x l.y y.x y.y
   ; Inline multiply function from FPU087.ASM instead of include.
      fld          st(1)               ; y.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.y*x.y, y.x. y.y, x.x, x.y
      fld          st(1)               ; y.x, y.y*x.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.x*x.x, y.y*x.y, y.x, y.y, x.x, x.y
      fsubr                            ; newx = y.x*x.x - y.y*x.y, y.x, y.y, x.x, x.y
      fxch         st(3)               ; x.x, y.x, y.y, newx, x.y
      fmulp        st(2),st            ; y.x, y.y*x.x, newx, x.y
      fmulp        st(3),st            ; y.y*x.x, newx, y.x*x.y
      faddp        st(2),st            ; newx newy = y.x*x.y + x.x*y.y
   ; Exp function from FPU387.ASM with mods to use less registers.
      fstp         QWORD PTR es:[di]   ; y
      fsincos                          ; cosy, siny
      fld1                             ; 1, cos, sin
      fldln2                           ; ln2, 1, cos, sin
      fdivr        QWORD PTR es:[di]   ; x.x/ln2, 1, cos, sin
      fst          QWORD PTR es:[di]
      fprem                            ; prem, 1, cos, sin
      f2xm1                            ; e**prem-1, 1, cos, sin
      fadd                             ; e**prem, cos, sin
      fld          QWORD PTR es:[di]   ; x.x/ln2, e**prem, cos, sin
      fxch                             ; e**prem, x.x/ln2, cos, sin
      fscale                           ; e**x.x, x.x/ln2, cos, sin
      fstp         st(1)               ; e**x.x, cos, sin
      fmul         st(2),st            ; e**x.x, cos, z.y
      fmul                             ; z.x, z.y
   END_OPER        Pwr
; --------------------------------------------------------------------------
   BEGN_OPER       Cosh                ; Cosh
      INCL_OPER    Cosh, SinhCosh      ; sinhx coshx y
      fxch         st(2)               ; y coshx sinhx
      fsincos                          ; cosy siny coshx sinhx
      fxch                             ; siny cosy coshx sinhx
      fmulp        st(3),st            ; cosy coshx y=sinhx*siny
      fmulp        st(1),st            ; x=cosy*coshx y
   END_OPER        Cosh
; --------------------------------------------------------------------------
   BEGN_OPER       Sinh                ; Sinh
      INCL_OPER    Sinh, SinhCosh      ; sinhx coshx y
      fxch         st(2)               ; y coshx sinhx
      fsincos                          ; cosy siny coshx sinhx
      fmulp        st(3),st            ; siny coshx x=sinhx*cosy
      fmulp        st(1),st            ; y=coshx*siny x
      fxch                             ; x y
   END_OPER        Sinh
; --------------------------------------------------------------------------
   BEGN_OPER       Sin                 ; Sin
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    Sin, SinhCosh       ; sinhy coshy sinx cosx
      fmulp        st(3),st            ; coshy sinx y=cosx*sinhy
      fmulp        st(1),st            ; x=sinx*coshy y
   END_OPER        Sin
; --------------------------------------------------------------------------
   BEGN_OPER       Cos                 ; Cos
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    Cos, SinhCosh       ; sinhy coshy sinx cosx
      fchs                             ; -sinhy coshy sinx cosx
      fmulp        st(2),st            ; coshy y=-sinhy*sinx cosx
      fmulp        st(2),st            ; y x=cosx*coshy
      fxch                             ; x y
   END_OPER        Cos
; --------------------------------------------------------------------------
   BEGN_OPER       CosXX               ; CosXX
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    CosXX, SinhCosh     ; sinhy coshy sinx cosx
      ; note missing fchs here
      fmulp        st(2),st            ; coshy y=sinhy*sinx cosx
      fmulp        st(2),st            ; y x=cosx*coshy
      fxch                             ; x y
   END_OPER        CosXX
; --------------------------------------------------------------------------
   BEGN_OPER       Tan                 ; Tan
      fadd         st,st               ; 2x y
      fsincos                          ; cos2x sin2x y
      fxch         st(2)               ; y sin2x cos2x
      fadd         st,st               ; 2y sin2x cos2x
      INCL_OPER    Tan, SinhCosh       ; sinh2y cosh2y sin2x cos2x
      fxch                             ; cosh2y sinh2y sin2x cos2x
      faddp        st(3),st            ; sinhy sinx denom=cos2x+cosh2y
      fld          st(2)               ; denom sinh2y sin2x denom
      fdivp        st(2),st            ; sinh2y x=sin2x/denom denom
      fdivrp       st(2),st            ; x y=sinh2y/denom
   END_OPER        Tan
; --------------------------------------------------------------------------
   BEGN_OPER       CoTan               ; CoTan
      fadd         st,st               ; 2x y
      fsincos                          ; cos2x sin2x y
      fxch         st(2)               ; y sin2x cos2x
      fadd         st,st               ; 2y sin2x cos2x
      INCL_OPER    CoTan, SinhCosh     ; sinh2y cosh2y sin2x cos2x
      fxch                             ; cosh2y sinh2y sin2x cos2x
      fsubrp       st(3),st            ; sinh2y sin2x denom=cosh2y-cos2x
      fld          st(2)               ; denom sinh2y sin2x denom
      fdivp        st(2),st            ; sinh2y x=sin2x/denom denom
      fchs                             ; -sinh2y x denom
      fdivrp       st(2),st            ; x y=-sinh2y/denom
   END_OPER        CoTan
; --------------------------------------------------------------------------
   BEGN_OPER       Tanh                ; Tanh
      fadd         st,st               ; 2x y
      INCL_OPER    Tanh, SinhCosh      ; sinh2x cosh2x y
      fxch         st(2)               ; y cosh2x sinh2x
      fadd         st,st               ; 2y cosh2x sinh2x
      fsincos                          ; cos2y sin2y cosh2x sinh2x
      faddp        st(2),st            ; sin2y denom=cos2y+cosh2x sinh2x
      fxch                             ; denom sin2y sinh2x
      fdiv         st(1),st            ; denom y=sin2y/denom sinh2x
      fdivp        st(2),st            ; y x=sinh2x/denom
      fxch                             ; x y
   END_OPER        Tanh
; --------------------------------------------------------------------------
   BEGN_OPER       CoTanh              ; CoTanh
      fadd         st,st               ; 2x y
      INCL_OPER    CoTanh, SinhCosh    ; sinh2x cosh2x y
      fxch         st(2)               ; y cosh2x sinh2x
      fadd         st,st               ; 2y cosh2x sinh2x
      fsincos                          ; cos2y sin2y cosh2x sinh2x
      fsubp        st(2),st            ; sin2y denom=cosh2x-cos2y sinh2x
      fchs                             ; -sin2y denom sinh2x
      fxch                             ; denom -sin2y sinh2x
      fdiv         st(1),st            ; denom y=-sin2y/denom sinh2x
      fdivp        st(2),st            ; y x=sinh2x/denom
      fxch                             ; x y
   END_OPER CoTanh
; --------------------------------------------------------------------------
   BEGN_OPER       LT                  ; <
   ; Arg2->d.x = (double)(Arg2->d.x < Arg1->d.x);
      fcomp        st(2)               ; y.y, x.x, x.y, comp arg1 to arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jbe          short LTfalse       ; jump if arg1 <= arg2
      fld1                             ; 1 0 (return arg2 < arg1)
      EXIT_OPER    LT
LTfalse:
      fldz                             ; 0 0
   END_OPER        LT
; --------------------------------------------------------------------------
   BEGN_OPER       LT2                 ; LT, set AX, clear FPU
   ; returns !(Arg2->d.x < Arg1->d.x) in ax
      fcom         st(2)               ; compare arg1, arg2
      fstsw        ax
      fninit
      sahf
      setbe        al                  ; return (Arg1 <= Arg2) in AX
      xor          ah,ah
   END_OPER        LT2
; --------------------------------------------------------------------------
   BEGN_OPER       LodLT               ; load, LT
   ; return (1,0) on stack if arg2 < arg1
      FIXUP        LodLT, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      jae          short LodLTfalse    ; jump when arg2 >= arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodLT
LodLTfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodLT
; --------------------------------------------------------------------------
   BEGN_OPER       LodLT2              ; Lod, LT, set AX, clear FPU
   ; returns !(Arg2->d.x < Arg1->d.x) in ax
      FIXUP        LodLT2, fcom, X     ; compare arg2, arg1
      fstsw        ax
      fninit                           ; clear fpu
      sahf
      setae        al                  ; set al when arg2 >= arg1
      xor          ah,ah               ; clear ah
   END_OPER        LodLT2              ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTMul            ; Lod, LT, Multiply (needs 4 on stack)
   ; for '<expr> * ( <expr> < <var> )'
   ; return number on stack if arg2 < arg1
      FIXUP        LodLTMul, fcomp, X  ; comp Arg2 to Arg1, pop st
      fstsw        ax                  ; save status
      POP_STK      1                   ; clear 1 from stack
      sahf
      jae          short LodLTMulfalse ; jump if arg2 >= arg1
      EXIT_OPER    LodLTMul            ; return value on st
   PARSALIGN
LodLTMulfalse:
      POP_STK      2                   ; return (0,0)
      fldz
      fldz
   END_OPER        LodLTMul
; --------------------------------------------------------------------------
   BEGN_OPER       GT                  ; >
   ; Arg2->d.x = (double)(Arg2->d.x > Arg1->d.x);
      fcomp        st(2)               ; compare arg1, arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jae          short GTfalse       ; jump if Arg1 >= Arg2
      fld1                             ; 1 0, return arg2 > arg1
      EXIT_OPER    GT
GTfalse:
      fldz                             ; 0 0
   END_OPER        GT
; --------------------------------------------------------------------------
   BEGN_OPER       GT2                 ; GT, set AX, clear FPU
   ; returns !(Arg2->d.x > Arg1->d.x) in ax
      fcom         st(2)               ; compare arg1, arg2
      fstsw        ax
      fninit
      sahf
      setae        al                  ; return (Arg1 >= Arg2) in AX
      xor          ah,ah
   END_OPER        GT2
; --------------------------------------------------------------------------
   BEGN_OPER       LodGT               ; load, GT
   ; return (1,0) on stack if arg2 > arg1
      FIXUP        LodGT, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      jbe          short LodGTfalse    ; jump when arg2 <= arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodGT
LodGTfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodGT
; --------------------------------------------------------------------------
   BEGN_OPER       LodGT2              ; Lod, GT, set AX, clear FPU
   ; returns !(Arg2->d.x > Arg1->d.x) in AX
      FIXUP        LodGT2, fcom, X     ; compare arg2, arg1
      fstsw        ax
      fninit                           ; clear fpu
      sahf
      setbe        al                  ; set al when arg2 <= arg1
      xor          ah,ah               ; clear ah
   END_OPER        LodGT2              ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_OPER       LTE                 ; <=
   ; Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
      fcomp        st(2)               ; y x y, comp Arg1 to Arg2
      fstsw        ax                  ; save status now
      POP_STK      3
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      sahf
      jb           short LTEfalse      ; jump if arg1 > arg2
      fld1                             ; 1 0, ret arg2 <= arg1
      EXIT_OPER    LTE
LTEfalse:
      fldz                             ; 0 0
   END_OPER        LTE
; --------------------------------------------------------------------------
   BEGN_OPER       LTE2                ; LTE, test ST, clear
   ; return !(Arg2->d.x <= Arg1->d.x) in AX
      fcom         st(2)               ; comp Arg1 to Arg2
      fstsw        ax
      fninit                           ; clear stack
      and          ah,1                ; mask cf
      shr          ax,8                ; ax=1 when arg1 < arg1
   END_OPER        LTE2                ; return (Arg1 < Arg2),
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTE              ; load, LTE
   ; return (1,0) on stack if arg2 <= arg1
      FIXUP        LodLTE, fcomp, X    ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      ja           short LodLTEfalse   ; jump when arg2 > arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodLTE
LodLTEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodLTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTE2             ; Load, LTE, test ST, clear
   ; return !(Arg2->d.x <= Arg1->d.x) in AX
      FIXUP        LodLTE2, fcom, X    ; comp Arg2 to Arg1
      fstsw        ax
      fninit
      sahf
      seta         al
      xor          ah,ah               ; ax=1 for expr. false
   END_OPER        LodLTE2             ; return (Arg2 > Arg1)
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTEMul           ; Lod, LTE, Multiply (needs 4 on stack)
   ; for '<expr> * ( <expr> <= <var> )'
   ; return number on stack if arg2 <= arg1
      FIXUP        LodLTEMul, fcomp, X ; comp Arg2 to Arg1, pop st
      fstsw        ax                  ; save status
      POP_STK      1                   ; clear 1 from stack
      sahf
      ja           short LodLTEMulfalse ; jump if arg2 > arg1
      EXIT_OPER    LodLTEMul           ; return value on st
   PARSALIGN
LodLTEMulfalse:
      POP_STK      2                   ; return (0,0)
      fldz
      fldz
   END_OPER        LodLTEMul
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTEAnd2          ; Load, LTE, AND, test ST, clear
   ; this is for 'expression && (expression <= value)'
   ; stack has {arg2.x arg2.y logical.x junk} on entry (arg1 in memory)
   ; Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
      FIXUP        LodLTEAnd2, fcom, X ; comp Arg2 to Arg1
      fstsw        ax
      sahf
      fxch         st(2)               ; logical.x arg2.y arg2.x junk ...
      ja           LTEA2RFalse         ; right side is false, Arg2 > Arg1
      ftst                             ; now see if left side of expr is true
      fstsw        ax
      sahf
      fninit                           ; clear fpu
      jz           LTEA2LFalse         ; jump if left side of && is false
      xor          ax,ax               ; return zero in ax for expr true
      EXIT_OPER    LodLTEAnd2
LTEA2RFalse:
      fninit
LTEA2LFalse:
      mov          ax,1                ; return ax=1 for condition false
   END_OPER        LodLTEAnd2
; --------------------------------------------------------------------------
   BEGN_OPER       GTE                 ; >=
   ; Arg2->d.x = (double)(Arg2->d.x >= Arg1->d.x);
      fcomp        st(2)               ; y x y (compare arg1,arg2)
      fstsw        ax
      POP_STK      3                   ; clear 3 from stk
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      ja           short GTEfalse      ; jmp if arg1 > arg2
      fld1                             ; 1 0 (return arg2 >= arg1 on stack)
      EXIT_OPER    GTE
GTEfalse:
      fldz                             ; 0 0
   END_OPER        GTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodGTE              ; load, GTE
   ; return (1,0) on stack if arg2 >= arg1
      FIXUP        LodGTE, fcomp, X    ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      jb           short LodGTEfalse   ; jump when arg2 < arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodGTE
LodGTEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodGTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodGTE2             ; Lod, GTE, set AX, clear FPU
   ; return !(Arg2->d.x >= Arg1->d.x) in AX
      FIXUP        LodGTE2, fcom, X    ; compare arg2, arg1
      fstsw        ax
      fninit                           ; clear fpu
      and          ah,1                ; mask cf
      shr          ax,8                ; shift it (AX = 1 when arg2 < arg1)
   END_OPER        LodGTE2             ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_OPER       EQ                  ; ==
   ; Arg2->d.x = (double)(Arg2->d.x == Arg1->d.x);
      fcomp        st(2)               ; compare arg1, arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jne          short EQfalse       ; jmp if arg1 != arg2
      fld1                             ; 1 0 (ret arg2 == arg1)
      EXIT_OPER    EQ
EQfalse:
      fldz
   END_OPER        EQ
; --------------------------------------------------------------------------
   BEGN_OPER       LodEQ               ; load, EQ
   ; return (1,0) on stack if arg2 == arg1
      FIXUP        LodEQ, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      jne          short LodEQfalse    ; jump when arg2 != arg1
      fld1                             ; 1 0 ... (return arg2 == arg1)
      EXIT_OPER    LodEQ
LodEQfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodEQ
; --------------------------------------------------------------------------
   BEGN_OPER       NE                  ; !=
   ; Arg2->d.x = (double)(Arg2->d.x != Arg1->d.x);
      fcomp        st(2)               ; compare arg1,arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz
      je           short NEfalse       ; jmp if arg1 == arg2
      fld1                             ; ret arg2 != arg1
      EXIT_OPER    NE
NEfalse:
      fldz
   END_OPER        NE
; --------------------------------------------------------------------------
   BEGN_OPER       LodNE               ; load, NE
   ; return (1,0) on stack if arg2 != arg1
      FIXUP        LodNE, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      je           short LodNEfalse    ; jump when arg2 == arg1
   ; CAE changed above 'jne' to 'je' 9 MAR 1993
      fld1                             ; 1 0 ...
      EXIT_OPER    LodNE
LodNEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodNE
; --------------------------------------------------------------------------
   BEGN_OPER       OR                  ; Or
   ; Arg2->d.x = (double)(Arg2->d.x || Arg1->d.x);
      ftst                             ; a1.x a1.y a2.x a2.y ...
      fstsw        ax
      sahf
      POP_STK      2                   ; a2.x a2.y ...
      jnz          short Arg1True
      ftst
      fstsw        ax
      sahf
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      jz           short NoneTrue
      fld1                             ; 1 0 ...
      EXIT_OPER    OR
   PARSALIGN
Arg1True:
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      fld1                             ; 1 0 ...
      EXIT_OPER    OR
NoneTrue:                              ; 0 ...
      fldz                             ; 0 0 ...
   END_OPER        OR
; --------------------------------------------------------------------------
   BEGN_OPER       AND                 ; And
   ; Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
      ftst                             ; a1.x a1.y a2.x a2.y ...
      fstsw        ax
      sahf
      POP_STK      2                   ; a2.x a2.y ...
      jz           short Arg1False
      ftst
      fstsw        ax
      sahf
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      jz           short Arg2False
      fld1                             ; 1 0 ...
      EXIT_OPER    AND
Arg1False:
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
Arg2False:
      fldz                             ; 0 0 ...
   END_OPER        AND
; --------------------------------------------------------------------------
   BEGN_OPER       ANDClr2           ; And, test ST, clear FPU
   ; for bailouts using <condition> && <condition>
   ;  Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
   ;  Returns !(Arg1 && Arg2) in ax
      ftst                             ; y.x y.y x.x x.y
      fstsw        ax
      sahf
      jz           short Arg1False2
      fxch         st(2)               ; x.x y.y y.x x.y
      ftst
      fstsw        ax
      sahf
      fninit
      jz           short Arg2False2
BothTrue2:
      xor          ax,ax
      EXIT_OPER    ANDClr2
Arg1False2:
      fninit
Arg2False2:
      mov           ax, 1
   END_OPER        ANDClr2

; --------------------------------------------------------------------------
   assume          ds:DGROUP, es:nothing
; called once per image
   public          _Img_Setup
   align           4
_Img_Setup         proc near
      les          si,_pfls            ; es:si = &pfls[0]
      mov          di,_LastOp          ; load index of lastop
      shl          di,2                ; convert to offset
      mov          bx,offset DGROUP:_fLastOp ; set bx for store
      add          di,si               ; di = offset lastop
      mov          WORD PTR [bx],di    ; save value of flastop
      mov          ax,es               ; es has segment value
      mov          WORD PTR [bx+2],ax  ; save seg for easy reload
      mov          ax,word ptr _v      ; build a ptr to Z
      add          ax,3*CARG+CPFX
      mov          _PtrToZ,ax          ; and save it
      ret
_Img_Setup         endp
; --------------------------------------------------------------------------
;	orbitcalc function follows
; --------------------------------------------------------------------------
   public          _fFormula
   align           16
_fFormula          proc far
      push         di                  ; don't build a frame here
      mov          di,offset DGROUP:_s ; reset this for stk overflow area
      mov          bx,_InitOpPtr       ; bx -> one before first token
      mov          ax,ds               ; save ds in ax
      lds          cx,_fLastOp         ; ds:cx -> one past last token
      mov          es,ax
   assume          es:DGROUP, ds:nothing ; swap es, ds before any fn. calls
      push         si
inner_loop:
      add          bx,4                ; point to next pointer pair
      cmp          bx,cx               ; time to quit yet?
      jae          short past_loop
      mov          si,WORD PTR [bx+2]  ; set si to operand pointer
      push         offset PARSERFP_TEXT:inner_loop
      jmp          WORD PTR [bx]       ; jmp to operator fn
past_loop:
   ; NOTE: AX is set by the last operator fn that was called.
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_new ; es:di -> new
      mov          cx,4
      rep          movsd               ; new = z
      mov          bx,es
      pop          si
      pop          di                  ; restore si, di
      mov          ds,bx               ; restore ds before return
   assume          ds:DGROUP, es:nothing
      ret                              ; return AX unmodified
_fFormula          endp
; --------------------------------------------------------------------------
   public          _fform_per_pixel    ; called once per pixel
   align           4
_fform_per_pixel   proc far
      FRAME        <si, di>
   ; if(!Transparent3D){
      cmp          _Transparent3D,0
      jne           abNormal_Pixel
   ;   /* v[5].a.d.x = */ (v[0].a.d.x = dx0[col]+dShiftx);
      mov          ax,_col
      shl          ax,3
      les          bx,_dx0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_row
      shl          ax,3
      les          bx,_dx1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX]
   ;;;;;;;;;fstp    QWORD PTR es:[bx+104]  kill this & prev=fstp
   ;   /* v[5].a.d.x = */ (v[0].a.d.y = dy0[row]+dShifty);
      mov          ax,_row
      shl          ax,3
      les          bx,_dy0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_col
      shl          ax,3
      les          bx,_dy1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX+8] ; make this an fstp
   ;;;;;;;;;fstp    QWORD PTR es:[bx+104]	; kill this
after_load:
      mov          di,offset DGROUP:_s ; di points to stack overflow area
      mov          ax,ds
      mov          bx,WORD PTR _pfls   ; bx -> pfls
      lds          cx,_fLastOp         ; cx = offset &f[LastOp],load ds
      mov          es,ax
   assume          es:DGROUP, ds:nothing
      cmp          _LastInitOp,0
      je           short skip_initloop ; no operators to do here
      mov          _LastInitOp,cx      ; lastinitop=lastop
   align           4
pixel_loop:
      mov          si,WORD PTR [bx+2]  ; get address of load or store
      call         WORD PTR [bx]       ; (*opptr)()
      add          bx,4                ; ++opptr
      cmp          bx,_LastInitOp
      jb           short pixel_loop
skip_initloop:
      mov          ax,es
      mov          ds,ax
   assume          ds:DGROUP, es:nothing ; for the rest of the program
      sub          bx,4                ; make initopptr point 1 token b4 1st
      mov          _InitOpPtr, bx      ; InitOptPtr = OpPtr;
      UNFRAME      <di, si>
      xor          ax,ax
      ret

abNormal_Pixel:
   ;   TranspPerPixel(MathType, &v[5].a, &v[6].a);
      mov          ax,WORD PTR _v
      add          ax,6*CARG+CPFX      ;v[6].a
      push         WORD PTR _v+2
      push         ax
      mov          ax,WORD PTR _v
      add          ax,5*CARG+CPFX      ;v[5].a
      push         WORD PTR _v+2
      push         ax
      push         1                   ;_MathType
      call         far PTR _TranspPerPixel
      add          sp,10
   ;   v[0].a = v[5].a;
      les          bx,_v
      fld          QWORD PTR es:[bx+5*CARG+CPFX]
      fstp         QWORD PTR es:[bx+CPFX]
      fld          QWORD PTR es:[bx+5*CARG+CPFX+8]
      fstp         QWORD PTR es:[bx+CPFX+8]
   ; }
      jmp          short after_load
_fform_per_pixel   endp
; --------------------------------------------------------------------------
PARSERFP_TEXT      ends
   end

