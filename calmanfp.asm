;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; calmanfp.asm - floating point version of the calcmand.asm file
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The following code was adapted from a little program called "Mandelbrot
; Sets by Wesley Loewer" which had a very limited distribution (my
; Algebra II class).  It didn't have any of the fancy integer math, but it
; did run floating point stuff pretty fast.
;
; The code was originally optimized for a 287 ('cuz that's what I've got)
; and for a large maxit (ie: use of generous overhead outside the loop to get
; slightly faster code inside the loop), which is generally the case when
; Fractint chooses to use floating point math.  This code also has the
; advantage that once the initial parameters are loaded into the fpu
; register, no transfers of fp values to/from memory are needed except to
; check periodicity and to show orbits and the like.  Thus, values keep all
; the significant digits of the full 10 byte real number format internal to
; the fpu.  Intermediate results are not rounded to the normal IEEE 8 byte
; format (double) at any time.
;
; The non fpu specific stuff, such as periodicity checking and orbits,
; was adapted from CALCFRAC.C and CALCMAND.ASM.
;
; This file must be assembled with floating point emulation turned on.  I
; suppose there could be some compiler differences in the emulation
; libraries, but this code has been successfully tested with the MSQC 2.51
; and MSC 5.1 emulation libraries.
;
;                                               Wes Loewer
;
; and now for some REAL fractal calculations...
; (get it, real, floating point..., never mind)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;                        required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.8086
.8087

.MODEL medium,c

; external functions
EXTRN   keypressed:FAR          ; this routine is in 'general.asm'
EXTRN   getakey:FAR             ; this routine is in 'general.asm'
EXTRN   plot_orbit:FAR          ; this routine is in 'fracsubr.c'
EXTRN   scrub_orbit:FAR         ; this routine is in 'fracsubr.c'

; external data
EXTRN init:WORD                 ; declared as type complex
EXTRN parm:WORD                 ; declared as type complex
EXTRN new:WORD                  ; declared as type complex
EXTRN maxit:WORD
EXTRN inside:WORD
EXTRN outside:WORD
EXTRN fpu:WORD                  ; fpu type: 87, 287, or 387
EXTRN rqlim:QWORD               ; bailout (I never did figure out
				;   what "rqlim" stands for. -Wes)
EXTRN color:WORD
EXTRN oldcolor:WORD
EXTRN realcolor:WORD
EXTRN periodicitycheck:WORD
EXTRN reset_periodicity:WORD
EXTRN closenuff:QWORD
EXTRN fractype:WORD             ; Mandelbrot or Julia
EXTRN kbdcount:WORD             ; keyboard counter
EXTRN dotmode:WORD
EXTRN show_orbit:WORD           ; "show-orbit" flag
EXTRN orbit_ptr:WORD            ; "orbit pointer" flag
EXTRN potflag:WORD              ; potential flag
EXTRN magnitude:QWORD           ; when using potential

JULIAFP  EQU 6                  ; from FRACTYPE.H
MANDELFP EQU 4
GREEN    EQU 2                  ; near y-axis
YELLOW   EQU 6                  ; near x-axis

initx    EQU <qword ptr init>   ; just to make life easier
inity    EQU <qword ptr init+8>
parmx    EQU <qword ptr parm>
parmy    EQU <qword ptr parm+8>
newx     EQU <qword ptr new>
newy     EQU <qword ptr new+8>

; Apparently, these might be needed for TC++ overlays. I don't know if
; these are really needed here since I am not familiar with TC++. -Wes
FRAME   MACRO regs
	push    bp
	mov     bp, sp
	IRP     reg, <regs>
	  push  reg
	  ENDM
	ENDM

UNFRAME MACRO regs
	IRP     reg, <regs>
	  pop reg
	  ENDM
	pop bp
	ENDM


.DATA
	align   2
savedx                  DQ  ?
savedy                  DQ  ?
orbit_real              DQ  ?
orbit_imag              DQ  ?
close                   DD  0.01
round_down_half         DD  0.5
tmp_word                DW  ?
inside_color            DW  ?
periodicity_color       DW  ?
;savedand               DW  ?
;savedincr              DW  ?
savedand                EQU     SI      ; this doesn't save much time or
savedincr               EQU     DI      ; space, but it doesn't hurt either

.CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This routine is called once per image.
; Put things here that won't change from one pixel to the next.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC calcmandfpasmstart
calcmandfpasmstart   PROC
					; not sure if needed here
	FRAME   <di,si>                 ; std frame, for TC++ overlays

	mov     ax,inside
	cmp     ax,0                    ; if (inside color == maxiter)
	jnl     non_neg_inside
	mov     ax,maxit                ;   use maxit as inside_color

non_neg_inside:                         ; else
	mov     inside_color,ax         ;   use inside as inside_color

	cmp     periodicitycheck,0      ; if periodicitycheck < 0
	jnl     non_neg_periodicitycheck
	mov     ax,7                    ;   use color 7 (default white)
non_neg_periodicitycheck:               ; else
	mov     periodicity_color,ax    ;   use inside_color still in ax
	mov     oldcolor,0              ; no periodicity checking on 1st pixel
	sub     ax,ax                   ; ax=0
	UNFRAME <si,di>                 ; pop stack frame
	ret
calcmandfpasmstart       ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; floating point version of calcmandasm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC calcmandfpasm
calcmandfpasm  PROC
	FRAME   <di,si>                 ; std frame, for TC++ overlays
; initialization stuff
	sub     ax,ax                   ; clear ax
	cmp     periodicitycheck,ax     ; periodicity checking?
	je      initoldcolor            ;  no, set oldcolor 0 to disable it
	cmp     inside,-59              ; zmag?
	je      initoldcolor            ;  set oldcolor to 0
	cmp     reset_periodicity,ax    ; periodicity reset?
	je      short initparms         ;  no, inherit oldcolor from prior invocation
	mov     ax,maxit                ; yup.  reset oldcolor to maxit-250
	sub     ax,250                  ; (avoids slowness at high maxits)
initoldcolor:
	mov     oldcolor,ax             ; reset oldcolor

initparms:
	sub     ax,ax                   ; clear ax
	mov     word ptr savedx,ax      ; savedx = 0.0
	mov     word ptr savedx+2,ax    ; needed since savedx is a QWORD
	mov     word ptr savedx+4,ax
	mov     word ptr savedx+6,ax
	mov     word ptr savedy,ax      ; savedy = 0.0
	mov     word ptr savedy+2,ax    ; needed since savedy is a QWORD
	mov     word ptr savedy+4,ax
	mov     word ptr savedy+6,ax
	inc     ax                      ; ax = 1
	mov     savedand,ax             ; savedand = 1
	mov     savedincr,ax            ; savedincr = 1
	mov     orbit_ptr,0             ; clear orbits
	dec     kbdcount                ; decrement the keyboard counter
	jns     short nokey             ;  skip keyboard test if still positive
	mov     kbdcount,10             ; stuff in a low kbd count
	cmp     show_orbit,0            ; are we showing orbits?
	jne     quickkbd                ;  yup.  leave it that way.
;this may need to be adjusted, I'm guessing at the "appropriate" values -Wes
	mov     kbdcount,5000           ; else, stuff an appropriate count val
	cmp     fpu,387                 ; ("appropriate" to the FPU)
	je      short kbddiskadj        ;     ...
	mov     kbdcount,3000           ;     ...
	cmp     fpu,287                 ;     ...
	je      short kbddiskadj        ;     ...
	mov     kbdcount,1000           ;     ...
	cmp     fpu,87                  ;     ...
	je      short kbddiskadj        ;     ...
	mov     kbdcount,500            ; emulation
kbddiskadj:
	cmp     dotmode,11              ; disk video?
	jne     quickkbd                ;  no, leave as is
	mov     cl,2                    ; yes, reduce count
	shr     kbdcount,cl             ;  ...

quickkbd:
	call    far ptr keypressed      ; has a key been pressed?
	cmp     ax,0                    ;  ...
	je      nokey                   ; nope.  proceed
	mov     kbdcount,0              ; make sure it goes negative again
	cmp     ax,'o'                  ; orbit toggle hit?
	je      orbitkey                ;  yup.  show orbits
	cmp     ax,'O'                  ; orbit toggle hit?
	jne     keyhit                  ;  nope.  normal key.
orbitkey:
	call    far ptr getakey         ; read the key for real
	mov     ax,1                    ; reset orbittoggle = 1 - orbittoggle
	sub     ax,show_orbit           ;  ...
	mov     show_orbit,ax           ;  ...
	jmp     short nokey             ; pretend no key was hit
keyhit:
	mov     ax,-1                   ; return with -1
	mov     color,ax                ; set color to -1
	UNFRAME <si,di>                 ; pop stack frame
	ret                             ; bail out!
nokey:

; OK, here's the heart of the floating point code.
; In my original program, the bailout value was loaded once per image and
; was left on the floating point stack after each pixel, and finally popped
; off the stack when the fractal was finished.  A lot of overhead for very
; little gain in my opinion, so I changed it so that it loads and unloads
; per pixel. -Wes

	fld     rqlim                   ; everything needs bailout first
	mov     cx,maxit                ; using cx as loop counter
	cmp     fpu,387                 ; jump to fpu specific code
	je      start_387               ; 387, slight efficiency tweeking
	cmp     fpu,287                 ;
	je      to_start_287            ; 287 (original version)
	jmp     start_87                ; else must be 87/emulation
to_start_287:
	jmp     start_287               ; needs a long jump here


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _387 code is just like _287 code except that it uses an FADD instead
; of an FSCALE per orbit and also saves an FLD1 per pixel.
;
; You could use .386/.387 here, but it is not necessary.  The _387 floating
; point routines in this file do not have any 387 specific op-codes,
; only 387 specific optimizations.  (And plus my MS QuickAssembler does not
; recognize the .386/.387 directives.) -Wes
;
.286
.287

start_387:
	cmp     fractype,JULIAFP        ; julia or mandelbrot set?
	je      short dojulia_387       ; julia set - go there

; Mandelbrot _387 initialization of stack
	dec     cx                      ; always requires at least 1 iteration

					; the fpu stack is shown below
					; st(0) ... st(7)
					; b (already on stack)
	fld     inity                   ; Cy b
	fld     initx                   ; Cx Cy b
	fld     st(1)                   ; Cy Cx Cy b
	fadd    parmy                   ; Py+Cy Cx Cy b
	fld     st                      ; Py+Cy Py+Cy Cx Cy b
	fmul    st,st                   ; (Py+Cy)^2 Py+Cy Cx Cy b
	fld     st(2)                   ; Cx (Py+Cy)^2 Py+Cy Cx Cy b
	fadd    parmx                   ; Px+Cx (Py+Cy)^2 Py+Cy Cx Cy b
	fmul    st(2),st                ; Px+Cx (Py+Cy)^2 (Py+Cy)(Px+Cx) Cx Cy b
	fmul    st,st                   ; (Px+Cx)^2 (Py+Cy)^2 (Py+Cy)(Px+Cx) Cx Cy b
	; which is the next               x^2 y^2 xy Cx Cy b
	jmp     short top_of_cx_loop_387 ; branch around the julia switch

dojulia_387:
					; Julia 387 initialization of stack
					; note that init and parm are "reversed"
					; b (already on stack)
	fld     parmy                   ; Cy b
	fld     parmx                   ; Cx Cy b
	fld     inity                   ; y Cx Cy b
	fld     st                      ; y y Cx Cy b
	fmul    st,st                   ; y^2 y Cx Cy b
	fld     initx                   ; x y^2 y Cx Cy b
	fmul    st(2),st                ; x y^2 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 xy Cx Cy b

top_of_cx_loop_387:                     ; x^2 y^2 xy Cx Cy b
	fsubr                           ; x^2-y^2 xy Cx Cy b
	fadd    st,st(2)                ; x^2-y^2+Cx xy Cx Cy b
	fxch                            ; xy x^2-y^2+Cx Cx Cy b
; FADD is faster than FSCALE for 387
	fadd    st,st                   ; 2xy x^2-y^2+Cx Cx Cy b
	fadd    st,st(3)                ; 2xy+Cy x^2-y^2+Cx Cx Cy b
					; now same as the new
					; y x Cx Cy b

	cmp     outside,-2              ; real, imag, mult, or sum ?
	jg      no_save_new_xy_387      ; if not, then skip this
	fld     st(1)                   ; x y x Cx Cy b
	fstp    newx                    ; y x Cx Cy b
	fst     newy                    ; y x Cx Cy b
no_save_new_xy_387:

	cmp     inside,-100                     ; epsilon cross ?
	jne     end_epsilon_cross_387
	call    near ptr epsilon_cross          ; y x Cx Cy b
	jcxz    pop_stack_387                   ; if cx=0, pop stack
end_epsilon_cross_387:

	cmp     cx,oldcolor                     ; if cx >= oldcolor
	jae     end_periodicity_check_387       ; don't check periodicity
	call    near ptr periodicity_check_287_387  ; y x Cx Cy b
	jcxz    pop_stack_387                   ; if cx=0, pop stack
end_periodicity_check_387:

	cmp     show_orbit,0            ; is show_orbit clear
	je      no_show_orbit_387       ; if so then skip
	call    near ptr show_orbit_xy  ; y x Cx Cy b
no_show_orbit_387:

					; y x Cx Cy b
	fld     st(1)                   ; x y x Cx Cy b
	fld     st(1)                   ; y x y x Cx Cy b
	fmul    st(3),st                ; y x y xy Cx Cy b
	fmulp   st(2),st                ; x y^2 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 xy Cx Cy b
	fld     st                      ; x^2 x^2 y^2 xy Cx Cy b
	fadd    st,st(2)                ; x^2+y^2 x^2 y^2 xy Cx Cy b

	cmp     potflag,0               ; check for potential
	je      no_potflag_387
	fst     magnitude               ; if so, save magnitude
no_potflag_387:

	fcomp   st(6)                   ; x^2 y^2 xy Cx Cy b
	fstsw   ax
	sahf
	ja      over_bailout_387

;less than or equal to bailout
	loop    top_of_cx_loop_387      ; x^2 y^2 xy Cx Cy b

; reached maxit, inside
	mov     oldcolor,0FFFFh         ; check periodicity immediately next time
	mov     ax,maxit
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     realcolor,ax            ; save unadjusted realcolor
	mov     ax,inside_color

	cmp     inside,-59              ; zmag ?
	jne     no_zmag_387
	fadd    st,st(1)                ; x^2+y^2 y^2 xy Cx Cy b
	fimul   maxit                   ; maxit*|z^2| x^2 y^2 xy Cx Cy b

; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  The following
; line cause the positive value to be truncated.
	fsub    round_down_half

	fist    tmp_word                ; tmp_word = |z^2|*maxit
	fwait
	mov     ax,tmp_word
	shr     ax,1                    ; |z^2|*maxit/2
	inc     ax                      ; |z^2|*maxit/2+1

no_zmag_387:

pop_stack_387:
	fninit

	mov     color,ax

	cmp     orbit_ptr,0             ; any orbits to clear?
	je      calcmandfpasm_ret_387   ; nope.
	call    far ptr scrub_orbit     ; clear out any old orbits
	mov     ax,color                ; restore color
					; speed not critical here in orbit land

calcmandfpasm_ret_387:
	UNFRAME <si,di>                 ; pop stack frame
	fwait				; just to make sure
	ret

	
over_bailout_387:                       ; x^2 y^2 xy Cx Cy b
; outside
	mov     ax,cx
	sub     ax,10                   ; 10 more next time before checking
	jns     no_fix_underflow_387
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
	sub     ax,ax                   ; don't check next time
no_fix_underflow_387:
	mov     oldcolor,ax             ; check when past this - 10 next time
	mov     ax,maxit
	sub     ax,cx                   ; leave 'times through loop' in ax

; zero color fix
	jnz     zero_color_fix_387
	inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_387:
	mov     realcolor,ax            ; save unadjusted realcolor
	sub     kbdcount,ax             ; adjust the keyboard count

	cmp     outside,-1              ; iter ? (most common case)
	je      pop_stack_387
	cmp     outside,-2		; outside <= -2 ?
	jle     special_outside_387     ; yes, go do special outside options
	mov     ax,outside              ; use outside color
	jmp     short pop_stack_387
special_outside_387:
	call    near ptr special_outside
	jmp     short pop_stack_387

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _287 version (closely resembles original code)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.286
.287
start_287:      ; 287
	cmp     fractype,JULIAFP        ; julia or mandelbrot set?
	je      short dojulia_287       ; julia set - go there

; Mandelbrot _287 initialization of stack
	dec     cx                      ; always requires at least 1 iteration

					; the fpu stack is shown below
					; st(0) ... st(7)
					; b (already on stack)
	fld     inity                   ; Cy b
	fld     initx                   ; Cx Cy b
	fld     st(1)                   ; Cy Cx Cy b
	fadd    parmy                   ; Py+Cy Cx Cy b
	fld1                            ; 1 Py+Cy Cx Cy b
	fld     st(1)                   ; Py+Cy 1 Py+Cy Cx Cy b
	fmul    st,st                   ; (Py+Cy)^2 1 Py+Cy Cx Cy b
	fld     st(3)                   ; Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
	fadd    parmx                   ; Px+Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
	fmul    st(3),st                ; Px+Cx (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
	fmul    st,st                   ; (Px+Cx)^2 (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
	; which is the next                x^2 y^2 1 xy Cx Cy b
	jmp     short top_of_cx_loop_287 ; branch around the julia switch

dojulia_287:
					; Julia 287 initialization of stack
					; note that init and parm are "reversed"
					; b (already on stack)
	fld     parmy                   ; Cy b
	fld     parmx                   ; Cx Cy b
	fld     inity                   ; y Cx Cy b
	fld1                            ; 1 y Cx Cy b
	fld     st(1)                   ; y 1 y Cx Cy b
	fmul    st,st                   ; y^2 1 y Cx Cy b
	fld     initx                   ; x y^2 1 y Cx Cy b
	fmul    st(3),st                ; x y^2 1 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b

top_of_cx_loop_287:                     ; x^2 y^2 1 xy Cx Cy b
	fsubr                           ; x^2-y^2 1 xy Cx Cy b
	fadd    st,st(3)                ; x^2-y^2+Cx 1 xy Cx Cy b
	fxch    st(2)                   ; xy 1 x^2-y^2+Cx Cx Cy b
; FSCALE is faster than FADD for 287
	fscale                          ; 2xy 1 x^2-y^2+Cx Cx Cy b
	fadd    st,st(4)                ; 2xy+Cy 1 x^2-y^2+Cx Cx Cy b
					; now same as the new
					; y 1 x Cx Cy b

	cmp     outside,-2              ; real, imag, mult, or sum ?
	jg      no_save_new_xy_287      ; if not, then skip this
	fld     st(2)                   ; x y 1 x Cx Cy b
	fstp    newx                    ; y 1 x Cx Cy b
	fst     newy                    ; y 1 x Cx Cy b
no_save_new_xy_287:

	cmp     inside,-100                     ; epsilon cross ?
	jne     end_epsilon_cross_287
	call    near ptr epsilon_cross          ; y 1 x Cx Cy b
	jcxz    pop_stack_287                   ; if cx=0, pop stack
end_epsilon_cross_287:

	cmp     cx,oldcolor                     ; if cx >= oldcolor
	jae     end_periodicity_check_287       ; don't check periodicity
	call    near ptr periodicity_check_287_387 ; y 1 x Cx Cy b
	jcxz    pop_stack_287                   ; if cx=0, pop stack
end_periodicity_check_287:

	cmp     show_orbit,0            ; is show_orbit clear
	je      no_show_orbit_287       ; if so then skip
	call    near ptr show_orbit_xy  ; y 1 x Cx Cy b
no_show_orbit_287:
					; y 1 x Cx Cy b
	fld     st(2)                   ; x y 1 x Cx Cy b
	fld     st(1)                   ; y x y 1 x Cx Cy b
	fmul    st(4),st                ; y x y 1 xy Cx Cy b
	fmulp   st(2),st                ; x y^2 1 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b
	fld     st                      ; x^2 x^2 y^2 1 xy Cx Cy b
	fadd    st,st(2)                ; x^2+y^2 x^2 y^2 1 xy Cx Cy b

	cmp     potflag,0		; check for potential
	je      no_potflag_287
	fst     magnitude		; if so, save magnitude
no_potflag_287:

	fcomp   st(7)                   ; x^2 y^2 1 xy Cx Cy b
	fstsw   ax
	sahf
	ja      over_bailout_287

;less than or equal to bailout
	loop    top_of_cx_loop_287      ; x^2 y^2 1 xy Cx Cy b

; reached maxit, inside
	mov     oldcolor,0FFFFh         ; check periodicity immediately next time
	mov     ax,maxit
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     realcolor,ax            ; save unadjusted realcolor
	mov     ax,inside_color

	cmp     inside,-59              ; zmag ?
	jne     no_zmag_287
	fadd    st,st(1)                ; x^2+y^2 y^2 1 xy Cx Cy b
	fimul   maxit                   ; maxit*|z^2| x^2 y^2 1 xy Cx Cy b

; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  The following
; line cause the positive value to be truncated.
	fsub    round_down_half

	fist    tmp_word                ; tmp_word = |z^2|*maxit
	fwait
	mov     ax,tmp_word
	shr     ax,1                    ; |z^2|*maxit/2
	inc     ax                      ; |z^2|*maxit/2+1

no_zmag_287:

pop_stack_287:
	fninit

	mov     color,ax

	cmp     orbit_ptr,0             ; any orbits to clear?
	je      calcmandfpasm_ret_287   ; nope.
	call    far ptr scrub_orbit     ; clear out any old orbits
	mov     ax,color                ; restore color
					; speed not critical here in orbit land

calcmandfpasm_ret_287:
	UNFRAME <si,di>                 ; pop stack frame
	fwait                           ; just to make sure
	ret

over_bailout_287:                       ; x^2 y^2 1 xy Cx Cy b
; outside
	mov     ax,cx
	sub     ax,10                   ; 10 more next time before checking
	jns     no_fix_underflow_287
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
	sub     ax,ax                   ; don't check next time
no_fix_underflow_287:
	mov     oldcolor,ax             ; check when past this - 10 next time
	mov     ax,maxit
	sub     ax,cx                   ; leave 'times through loop' in ax

; zero color fix
	jnz     zero_color_fix_287
	inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_287:
	mov     realcolor,ax            ; save unadjusted realcolor
	sub     kbdcount,ax             ; adjust the keyboard count

	cmp     outside,-1              ; iter ? (most common case)
	je      pop_stack_287
	cmp     outside,-2		; outside <= -2 ?
	jle     special_outside_287     ; yes, go do special outside options
	mov     ax,outside              ; use outside color
	jmp     short pop_stack_287
special_outside_287:
	call    near ptr special_outside
	jmp     short pop_stack_287


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _87 code is just like 287 code except that it must use
;       fstsw   tmp_word
;       fwait
;       mov     ax,tmp_word
; instead of
;       fstsw   ax
;
.8086
.8087
start_87:
; let emulation fall through to the 87 code here
; as it seems not emulate correctly on an 8088/86 otherwise
	cmp     fractype,JULIAFP        ; julia or mandelbrot set?
	je      short dojulia_87        ; julia set - go there

; Mandelbrot _87 initialization of stack
	dec     cx                      ; always requires at least 1 iteration

					; the fpu stack is shown below
					; st(0) ... st(7)
					; b (already on stack)
	fld     inity                   ; Cy b
	fld     initx                   ; Cx Cy b
	fld     st(1)                   ; Cy Cx Cy b
	fadd    parmy                   ; Py+Cy Cx Cy b
	fld1                            ; 1 Py+Cy Cx Cy b
	fld     st(1)                   ; Py+Cy 1 Py+Cy Cx Cy b
	fmul    st,st                   ; (Py+Cy)^2 1 Py+Cy Cx Cy b
	fld     st(3)                   ; Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
	fadd    parmx                   ; Px+Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
	fmul    st(3),st                ; Px+Cx (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
	fmul    st,st                   ; (Px+Cx)^2 (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
	; which is the next               x^2 y^2 1 xy Cx Cy b
	jmp     short top_of_cx_loop_87 ; branch around the julia switch

dojulia_87:
					; Julia 87 initialization of stack
					; note that init and parm are "reversed"
					; b (already on stack)
	fld     parmy                   ; Cy b
	fld     parmx                   ; Cx Cy b
	fld     inity                   ; y Cx Cy b
	fld1                            ; 1 y Cx Cy b
	fld     st(1)                   ; y 1 y Cx Cy b
	fmul    st,st                   ; y^2 1 y Cx Cy b
	fld     initx                   ; x y^2 1 y Cx Cy b
	fmul    st(3),st                ; x y^2 1 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b

top_of_cx_loop_87:                      ; x^2 y^2 1 xy Cx Cy b
	fsubr                           ; x^2-y^2 1 xy Cx Cy b
	fadd    st,st(3)                ; x^2-y^2+Cx 1 xy Cx Cy b
	fxch    st(2)                   ; xy 1 x^2-y^2+Cx Cx Cy b
; FSCALE is faster than FADD for 87
	fscale                          ; 2xy 1 x^2-y^2+Cx Cx Cy b
	fadd    st,st(4)                ; 2xy+Cy 1 x^2-y^2+Cx Cx Cy b
					; now same as the new
					; y 1 x Cx Cy b

	cmp     outside,-2              ; real, imag, mult, or sum ?
	jg      no_save_new_xy_87       ; if not, then skip this
	fld     st(2)                   ; x y 1 x Cx Cy b
	fstp    newx                    ; y 1 x Cx Cy b
	fst     newy                    ; y 1 x Cx Cy b
no_save_new_xy_87:

	cmp     inside,-100                     ; epsilon cross ?
	jne     end_epsilon_cross_87
	call    near ptr epsilon_cross          ; y 1 x Cx Cy b
	test    cx,cx                           ; if cx=0, pop stack
	jnz     end_epsilon_cross_87            ; replaces jcxz
	jmp     pop_stack_6_87                  ; with a long jump
end_epsilon_cross_87:

	cmp     cx,oldcolor                     ; if cx >= oldcolor
	jae     no_periodicity_check_87         ; don't check periodicity
	call    near ptr periodicity_check_87   ; y 1 x Cx Cy b
	jcxz    pop_stack_6_87                  ; if cx=0, pop stack
no_periodicity_check_87:

	cmp     show_orbit,0            ; is show_orbit clear
	je      no_show_orbit_87        ; if so then skip
	call    near ptr show_orbit_xy  ; y 1 x Cx Cy b
no_show_orbit_87:

					; y 1 x Cx Cy b
	fld     st(2)                   ; x y 1 x Cx Cy b
	fld     st(1)                   ; y x y 1 x Cx Cy b
	fmul    st(4),st                ; y x y 1 xy Cx Cy b
	fmulp   st(2),st                ; x y^2 1 xy Cx Cy b
	fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b
	fld     st                      ; x^2 x^2 y^2 1 xy Cx Cy b
	fadd    st,st(2)                ; x^2+y^2 x^2 y^2 1 xy Cx Cy b

	cmp     potflag,0               ; check for potential
	je      no_potflag_87
	fst     magnitude               ; if so, save magnitude
no_potflag_87:

	fcomp   st(7)                   ; x^2 y^2 1 xy Cx Cy b
	fstsw   tmp_word
	fwait
	mov     ax,tmp_word
	sahf
	ja      over_bailout_87

;less than or equal to bailout
	loop    top_of_cx_loop_87       ; x^2 y^2 1 xy Cx Cy b

; reached maxit
	mov     oldcolor,0FFFFh         ; check periodicity immediately next time
	mov     ax,maxit
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     realcolor,ax            ; save unadjusted realcolor
	mov     ax,inside_color

	cmp     inside,-59              ; zmag ?
	jne     no_zmag_87
	fadd    st,st(1)                ; x^2+y^2 y^2 1 xy Cx Cy b
	fimul   maxit                   ; maxit*|z^2| x^2 y^2 1 xy Cx Cy b

; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  The following
; line cause the positive value to be truncated.
	fsub    round_down_half

	fist    tmp_word                ; tmp_word = |z^2|*maxit
	fwait
	mov     ax,tmp_word
	shr     ax,1                    ; |z^2|*maxit/2
	inc     ax                      ; |z^2|*maxit/2+1

no_zmag_87:

pop_stack_7_87:
; The idea here is just to clear the floating point stack.  There was a
; problem using FNINIT with the emulation library.  It didn't seem to
; properly clear the emulated stack, resulting in "stack overflow"
; messages.  Therefore, if emulation is being used, then FSTP's are used
; instead.

	cmp     fpu,0                   ; are we using emulation?
	jne     no_emulation            ; if not, then jump
	fstp    st
; you could just jump over this next check, but its faster to just check again
pop_stack_6_87:
	cmp     fpu,0                   ; are we using emulation?
	jne     no_emulation            ; if not, then jump
	fstp    st
	fstp    st
	fstp    st
	fstp    st
	fstp    st
	fstp    st
	jmp     short end_pop_stack_87
no_emulation:                           ; if no emulation, then
	fninit                          ;   use the faster FNINIT
end_pop_stack_87:

	mov     color,ax

	cmp     orbit_ptr,0             ; any orbits to clear?
	je      calcmandfpasm_ret_87    ; nope.
	call    far ptr scrub_orbit     ; clear out any old orbits
	mov     ax,color                ; restore color
					; speed not critical here in orbit land

calcmandfpasm_ret_87:
	UNFRAME <si,di>                 ; pop stack frame
	fwait				; just to make sure
	ret

over_bailout_87:                        ; x^2 y^2 1 xy Cx Cy b
; outside
	mov     ax,cx
	sub     ax,10                   ; 10 more next time before checking
	jns     no_fix_underflow_87
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
	sub     ax,ax                   ; don't check next time
no_fix_underflow_87:
	mov     oldcolor,ax             ; check when past this - 10 next time
	mov     ax,maxit
	sub     ax,cx                   ; leave 'times through loop' in ax

; zero color fix
	jnz     zero_color_fix_87
	inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_87:
	mov     realcolor,ax            ; save unadjusted realcolor
	sub     kbdcount,ax             ; adjust the keyboard count

	cmp     outside,-1              ; iter ? (most common case)
	je      pop_stack_7_87
	cmp     outside,-2		; outside <= -2 ?
	jle     special_outside_87      ; yes, go do special outside options
	mov     ax,outside              ; use outside color
	jmp     short pop_stack_7_87
special_outside_87:
	call    near ptr special_outside
	jmp     short pop_stack_7_87

calcmandfpasm  ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Since periodicity checking is used most of the time, I decided to
; separate the periodicity_check routines into a _287_387 version
; and an _87 version to achieve a slight increase in speed.  The
; epsilon_cross, show_orbit_xy, and special_outside routines are less
; frequently used and therefore have been implemented as single routines
; usable by the 8087 and up. -Wes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.286
.287
periodicity_check_287_387   PROC    NEAR
; REMEMBER, the cx counter is counting BACKWARDS from maxit to 0
					; fpu stack is either
					; y x Cx Cy b (387)
					; y 1 x Cx Cy b (287/emul)
	cmp     fpu,387
	jb      pc_load_x
	fld     st(1)                   ; if 387
	jmp     short pc_end_load_x
pc_load_x:
	fld     st(2)                   ; if 287/emul
pc_end_load_x:
					; x y ...
	test    cx,savedand             ; save on 0, check on anything else
	jnz     do_check_287_387        ;  time to save a new "old" value

; save last value                       ; fpu stack is
	fstp    savedx                  ; x y ...
	fst     savedy                  ; y ...
	dec     savedincr               ; time to lengthen the periodicity?
	jnz     per_check_287_387_ret   ; if not 0, then skip
	shl     savedand,1              ; savedand = (savedand << 1) + 1
	inc     savedand                ; for longer periodicity
	mov     savedincr,4             ; and restart counter
	ret                             ; y ...

do_check_287_387:                       ; fpu stack is
					; x y ...
	fsub    savedx                  ; x-savedx y ...
	fabs                            ; |x-savedx| y ...
	fcomp   closenuff               ; y ...
	fstsw   ax
	sahf
	ja      per_check_287_387_ret
	fld     st                      ; y y ...
	fsub    savedy                  ; y-savedy y ...
	fabs                            ; |y-savedy| y ...
	fcomp   closenuff               ; y ...
	fstsw   ax
	sahf
	ja      per_check_287_387_ret
					; caught a cycle!!!
	mov     oldcolor,0FFFFh         ; check periodicity immediately next time

	mov     ax,maxit
	mov     realcolor,ax            ; save unadjusted realcolor as maxit
	sub     ax,cx                   ; subtract half c
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     ax,periodicity_color    ; set color
	sub     cx,cx                   ; flag to exit cx loop immediately

per_check_287_387_ret:
	ret
periodicity_check_287_387   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
periodicity_check_87    PROC    NEAR
; just like periodicity_check_287_387 except for the use of
;       fstsw tmp_word
; instead of
;       fstsw ax

; REMEMBER, the cx counter is counting BACKWARDS from maxit to 0
	fld     st(2)                   ; x y ...
	test    cx,savedand             ; save on 0, check on anything else
	jnz     do_check_87             ;  time to save a new "old" value

; save last value                       ; fpu stack is
					; x y ...
	fstp    savedx                  ; y ...
	fst     savedy                  ; y ...
	dec     savedincr               ; time to lengthen the periodicity?
	jnz     per_check_87_ret        ; if not 0, then skip
	shl     savedand,1              ; savedand = (savedand << 1) + 1
	inc     savedand                ; for longer periodicity
	mov     savedincr,4             ; and restart counter
	ret                             ; y ...

do_check_87:                            ; fpu stack is
					; x y ...
	fsub    savedx                  ; x-savedx y ...
	fabs                            ; |x-savedx| y ...
	fcomp   closenuff               ; y ...
	fstsw   tmp_word
	fwait
	mov     ax,tmp_word
	sahf
	ja      per_check_87_ret
	fld     st                      ; y y ...
	fsub    savedy                  ; y-savedy y ...
	fabs                            ; |y-savedy| y ...
	fcomp   closenuff               ; y ...
	fstsw   tmp_word
	fwait
	mov     ax,tmp_word
	sahf
	ja      per_check_87_ret
					; caught a cycle!!!
	mov     oldcolor,0FFFFh         ; check periodicity immediately next time

	mov     ax,maxit
	mov     realcolor,ax            ; save unadjusted realcolor as maxit
	sub     ax,cx
	sub     kbdcount,ax             ; adjust the keyboard count

	mov     ax,periodicity_color    ; set color
	sub     cx,cx                   ; flag to exit cx loop immediately

per_check_87_ret:
	ret
periodicity_check_87       ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
epsilon_cross   PROC    NEAR
					; fpu stack is either
					; y x Cx Cy b (387)
					; y 1 x Cx Cy b (287/87/emul)
	cmp     fpu,387
	jb      ec_load_x
	fld     st(1)                   ; if 387
	jmp     short ec_end_load_x
ec_load_x:
	fld     st(2)                   ; if 287/87/emul
ec_end_load_x:                          ; x y ...

	fabs                            ; |x| y 1 x Cx Cy b
	fcomp   close                   ; y 1 x Cx Cy b
	fstsw   tmp_word
	fwait
	mov     ax,tmp_word
	sahf
	jae     no_x_epsilon_cross
	mov     ax,maxit                ; x is close to y axis
	sub     ax,cx                   ; leave 'times through loop' in ax
; zero color fix
	jnz     zero_color_fix_1
	inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_1:
	mov     realcolor,ax            ; save unadjusted realcolor
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     ax,GREEN                ;
	sub     cx,cx                   ; flag to end loop
	mov     oldcolor,cx             ; don't check next time
	ret                             ; return
no_x_epsilon_cross:                     ; y 1 x Cx Cy b
	fld     st                      ; y y 1 x Cx Cy b
	fabs                            ; |y| y 1 x Cx Cy b
	fcomp   close                   ; y 1 x Cx Cy b
	fstsw   tmp_word
	fwait
	mov     ax,tmp_word
	sahf
	jae     no_y_epsilon_cross
	mov     ax,maxit                ; y is close to x axis
	sub     ax,cx                   ; leave 'times through loop' in ax
; zero color fix
	jnz     zero_color_fix_2
	inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_2:
	mov     realcolor,ax            ; save unadjusted realcolor
	sub     kbdcount,ax             ; adjust the keyboard count
	mov     ax,YELLOW
	sub     cx,cx                   ; flag to end loop
	mov     oldcolor,cx             ; don't check next time
	ret                             ; return
no_y_epsilon_cross:
	ret
epsilon_cross   ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
show_orbit_xy   PROC NEAR USES cx si di
	local	tmp_ten_byte_0:tbyte	; stupid klooge for MASM 5.1 LOCAL bug
	local	tmp_ten_byte_1:tbyte
	local	tmp_ten_byte_2:tbyte
	local	tmp_ten_byte_3:tbyte
	local	tmp_ten_byte_4:tbyte
	local	tmp_ten_byte_5:tbyte
	local	tmp_ten_byte_6:tbyte
; USES is needed because in all likelyhood, plot_orbit surely
; uses these registers.  It's ok to have to push/pop's here in the
; orbits as speed is not crucial when showing orbits.

					; fpu stack is either
					; y x Cx Cy b (387)
					; y 1 x Cx Cy b (287/87/emul)
	cmp     fpu,387
	jb      so_load_x
	fld     st(1)                   ; if 387
	jmp     short so_end_load_x
so_load_x:
	fld     st(2)                   ; if 287/87/emul
so_end_load_x:
					; x y ...
					; and needs to returned as
					; y ...

	fstp    orbit_real              ; y ...
	fst     orbit_imag              ; y ...
	mov     ax,-1                   ; color for plot orbit
	push    ax                      ;       ...
; since the number fpu registers that plot_orbit() preserves is compiler
; dependant, it's best to fstp the entire stack into 10 byte memories
; and fld them back after plot_orbit() returns.
	fstp    tmp_ten_byte_1          ; store the stack in 80 bit form
	fstp    tmp_ten_byte_2
	fstp    tmp_ten_byte_3
	fstp    tmp_ten_byte_4
	fstp    tmp_ten_byte_5
	cmp     fpu,287                 ; with 287/87/emul the stack is 6 high
	jg      no_store_6              ; with 387 it is only 5 high
	fstp    tmp_ten_byte_6
no_store_6:
	fwait                           ; just to be safe
	push    word ptr orbit_imag+6   ; co-ordinates for plot orbit
	push    word ptr orbit_imag+4   ;       ...
	push    word ptr orbit_imag+2   ;       ...
	push    word ptr orbit_imag     ;       ...
	push    word ptr orbit_real+6   ; co-ordinates for plot orbit
	push    word ptr orbit_real+4   ;       ...
	push    word ptr orbit_real+2   ;       ...
	push    word ptr orbit_real     ;       ...
	call    far ptr plot_orbit      ; display the orbit
	add     sp,9*2                  ; clear out the parameters

        cmp     fpu,287
        jg      no_load_6
        fld     tmp_ten_byte_6          ; load them back in reverse order
no_load_6:
	fld	tmp_ten_byte_5
	fld	tmp_ten_byte_4
	fld	tmp_ten_byte_3
	fld	tmp_ten_byte_2
	fld	tmp_ten_byte_1
	fwait                           ; just to be safe
	ret
show_orbit_xy   ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
special_outside PROC NEAR
; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  Using
; "FSUB round_down_half" causes the values to be rounded down.

	cmp     outside,-2
	jne     not_real
	fld     newx
	fsub    round_down_half
	fistp   tmp_word
	add     ax,7
	fwait
	add     ax,tmp_word
	jmp     short check_color
not_real:
	cmp     outside,-3
	jne     not_imag
	fld     newy
	fsub    round_down_half
	fistp   tmp_word
	add     ax,7
	fwait
	add     ax,tmp_word
	jmp     short check_color
not_imag:
	cmp     outside,-4
	jne     not_mult
	fld     newy
	ftst                    ; check to see if newy == 0
	fstsw   tmp_word
	push    ax              ; save current ax value
	fwait
	mov     ax,tmp_word
	sahf
	pop     ax              ; retrieve ax (does not affect flags)
	jne     non_zero_y
	ret                     ; if y==0, return with normal ax
non_zero_y:
	fdivr   newx            ; newx/newy
	mov     tmp_word,ax
	fimul   tmp_word        ; ax*newx/newy  (Use FIMUL instead of MUL
	fsub    round_down_half ; to make it match the C code.)
	fistp   tmp_word
	fwait
	mov     ax,tmp_word
	jmp     short check_color
not_mult:
	cmp     outside,-5      ; currently always equal, but put here
	jne     not_sum         ; for future outside types
	fld     newx
	fadd    newy            ; newx+newy
	fsub    round_down_half
	fistp   tmp_word
	fwait
	add     ax,tmp_word

not_sum:
check_color:
	cmp     ax,maxit                ; use UNSIGNED comparison
	jbe     special_outside_ret     ; color < 0 || color > maxit
	sub     ax,ax                   ; ax = 0
special_outside_ret:
	ret
special_outside ENDP

.8086   ;just to be sure
.8087
END
