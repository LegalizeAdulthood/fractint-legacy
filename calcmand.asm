;	CALCMAND.ASM - Mandelbrot/Julia Set calculation Routines

;	This module runs as part of an overlay with calcfrac.c.
;	It must not be called from anywhere other than calcfrac.

;	The routines in this code perform Mandelbrot and Julia set
;	calculations using 32-bit integer math as opposed to the
;	"traditional" floating-point approach.

;	This code relies on several tricks to run as quickly as it does.

;	One can fake floating point arithmetic by using integer
;	arithmetic and keeping track of the implied decimal point
;	if things are reasonable -- and in this case, they are.
;	I replaced code that looked like: z = x*y with code that
;	looks like:
;			ix = x * ifudge 		(outside the loops)
;			iy = y * ifudge
;			....
;			iz = (ix * iy) / ifudge 	(inside the loops)
;	(and keep remembering that all the integers are "ifudged" bigger)

;	The 386 has native 32-bit integer arithmetic, and (briefly) keeps
;	64-bit values around after 32-bit multiplies.	If the result is
;	divided down right away, you've got 64-bit arithmetic.   You just
;	have to ensure that the result after the divide is <= 32 bits long.
;	CPUs predating the 386 have to emulate 32-bit arithmetic using
;	16-bit arithmetic, which is significantly slower.

;	Dividing is slow -- but shifting is fast, and we can select our
;	"fudge factor" to be a power of two, permitting us to use that
;	method instead.   In addition, the 386 can perform 32-bit wide
;	shifting -- and even 64-bit shifts with the following logic:
;			shdr	eax,edx,cl
;			shr	edx,cl
;	so we make sure that our "fudge factor" is a power of 2 and shift
;	it down that way.
;	Calcmand is hardcoded for a fudge factor of 2**29.


;					Bert Tyler
; History since Fractint 16.0
;  (See comments with CJLT in them)
;  CJLT=Chris Lusby Taylor who has...
;
;   1. Speeded up 16 bit on 16 bit CPU
;	Minor changes, notably prescaling to fg14 before multiplying
;	instead of scaling the answer.
;	Also, I added overflow detection after adding linit, since it
;	seems this could overflow.  
;	Overall effect is about 10% faster on 386 with debugflag=8088
;   2. Speeded up 32 bit on 16 bit CPU
;	The macro `square' is totally rewritten, as is the logic for 2xy,
;	by prescaling x and y to fg31, not fg29. This allows us to do a
;	32 bit multiply in 3, not 4, 16 bit chunks while retaining full
;	fg29 accuracy.
;       Also, I removed lots of well-meaning but ineffective code handling
;	special cases of zeros and tidied up the handling of negative numbers,
;	so the routine is quite a bit shorter now and overall throughput of
;	Mandel is now over 40% faster on a 386 with debugflag=8088.
;       By the way, I was tempted to go the whole hog and replace x*x-y*y
;	by (x+y)*(x-y) to reduce 4 16-bit multiplys to 3, but it makes
;	escape detection a bit trickier. Another time, maybe.
;

;			 required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.MODEL	medium,c
DGROUP	      group   _DATA,_DATA2

.8086

	; these must NOT be in any segment!!
	; this get's rid of TURBO-C fixup errors

	extrn	keypressed:far		; this routine is in 'general.asm'
	extrn	getakey:far		; this routine is in 'general.asm'
	extrn	iplot_orbit:far 	; this routine is in 'calcfrac.c'
	extrn	scrub_orbit:far 	; this routine is in 'calcfrac.c'

_DATA2		segment DWORD PUBLIC 'DATA'

FUDGEFACTOR	equ	29		; default (non-potential) fudgefactor

; ************************ External variables *****************************

	extrn	fractype:word		; == 0 if Mandelbrot set, else Julia
	extrn	inside:word		; "inside" color, normally 1 (blue)
	extrn   outside:word            ; "outside" color, normally -1 (iter)
	extrn	creal:dword, cimag:dword ; Julia Set Constant
	extrn	delmin:dword		; min increment - precision required
	extrn	maxit:word		; maximum iterations
	extrn	lm:dword		; magnitude bailout limit

	extrn	row:word, col:word	; current pixel to calc
	extrn	color:word		; color calculated for the pixel
	extrn	realcolor:word		; color before inside,etc adjustments

	extrn	reset_periodicity:word	; nonzero if to be reset
	extrn	kbdcount:word		; keyboard counter

	extrn	cpu:word		; cpu type: 86, 186, 286, or 386
	extrn	dotmode:word

	extrn	show_orbit:word 	; "show-orbit" flag
	extrn	orbit_ptr:word		; "orbit pointer" flag
	extrn	periodicitycheck:word	; no periodicity if zero

	public	linitx,linity		; caller sets these
	public	savedmask		; caller sets this

; ************************ Internal variables *****************************

		align	4
x		dd	0		; temp value: x
y		dd	0		; temp value: y
absx		dd	0		; temp value: abs(x)
linitx		dd	0		; initial value, set by calcfrac
linity		dd	0		; initial value, set by calcfrac
savedmask	dd	0		; saved values mask
savedx		dd	0		; saved values of X and Y iterations
savedy		dd	0		;  (for periodicity checks)
k		dw	0		; iteration countdown counter
oldcolor	dw	0		; prior pixel's escape time k value
savedand	dw	0		; AND value for periodicity checks
savedincr	dw	0		; flag for incrementing AND value
period		db	0		; periodicity, if in the lake

_DATA2		ends

.CODE

; ***************** Function calcmandasm() **********************************

	public	calcmandasm

FRAME	MACRO regs
	push	bp
	mov	bp, sp
	IRP	reg, <regs>
	  push	reg
	  ENDM
	ENDM

UNFRAME MACRO regs
	IRP	reg, <regs>
	  pop reg
	  ENDM
	pop bp
	ENDM

calcmandasm	proc
	FRAME	<di,si> 		; std frame, for TC++ overlays
	sub	ax,ax			; clear ax
	cmp	periodicitycheck,ax	; periodicity checking disabled?
	je	initoldcolor		;  yup, set oldcolor 0 to disable it
	cmp	reset_periodicity,ax	; periodicity reset?
	je	short initparms 	; inherit oldcolor from prior invocation
	mov	ax,maxit		; yup.	reset oldcolor to maxit-250
	sub	ax,250			; (avoids slowness at high maxits)
initoldcolor:
	mov	oldcolor,ax		; reset oldcolor

initparms:
	mov	ax,word ptr creal	; initialize x == creal
	mov	dx,word ptr creal+2	;  ...
	mov	word ptr x,ax		;  ...
	mov	word ptr x+2,dx 	;  ...

	mov	ax,word ptr cimag	; initialize y == cimag
	mov	dx,word ptr cimag+2	;  ...
	mov	word ptr y,ax		;  ...
	mov	word ptr y+2,dx 	;  ...

	mov	ax,maxit		; setup k = maxit
	inc	ax			; (+ 1)
	mov	k,ax			;  (decrementing to 0 is faster)

	cmp	fractype,1		; julia or mandelbrot set?
	je	short dojulia		; julia set - go there

;	(Tim wants this code changed so that, for the Mandelbrot,
;	Z(1) = (x + iy) + (a + ib).  Affects only "fudged" Mandelbrots.
;	(for the "normal" case, a = b = 0, and this works, too)
;	cmp	word ptr x,0		; Mandelbrot shortcut:
;	jne	short doeither		;  if creal = cimag = 0,
;	cmp	word ptr x+2,0		; the first iteration can be emulated.
;	jne	short doeither		;  ...
;	cmp	word ptr y,0		;  ...
;	jne	short doeither		;  ...
;	cmp	word ptr y+2,0		;  ...
;	jne	short doeither		;  ...
;	dec	k			; we know the first iteration passed
;	mov	dx,word ptr linitx+2	; copy x = linitx
;	mov	ax,word ptr linitx	;  ...
;	mov	word ptr x+2,dx 	;  ...
;	mov	word ptr x,ax		;  ...
;	mov	dx,word ptr linity+2	; copy y = linity
;	mov	ax,word ptr linity	;  ...
;	mov	word ptr y+2,dx 	;  ...
;	mov	word ptr y,ax		;  ...

	dec	k			; we know the first iteration passed
	mov	dx,word ptr linitx+2	; add x += linitx
	mov	ax,word ptr linitx	;  ...
	add	word ptr x,ax		;  ...
	adc	word ptr x+2,dx 	;  ...
	mov	dx,word ptr linity+2	; add y += linity
	mov	ax,word ptr linity	;  ...
	add	word ptr y,ax		;  ...
	adc	word ptr y+2,dx 	;  ...
	jmp	short doeither		; branch around the julia switch

dojulia:				; Julia Set initialization
					; "fudge" Mandelbrot start-up values
	mov	ax,word ptr x		; switch x with linitx
	mov	dx,word ptr x+2 	;  ...
	mov	bx,word ptr linitx	;  ...
	mov	cx,word ptr linitx+2	;  ...
	mov	word ptr x,bx		;  ...
	mov	word ptr x+2,cx 	;  ...
	mov	word ptr linitx,ax	;  ...
	mov	word ptr linitx+2,dx	;  ...

	mov	ax,word ptr y		; switch y with linity
	mov	dx,word ptr y+2 	;  ...
	mov	bx,word ptr linity	;  ...
	mov	cx,word ptr linity+2	;  ...
	mov	word ptr y,bx		;  ...
	mov	word ptr y+2,cx 	;  ...
	mov	word ptr linity,ax	;  ...
	mov	word ptr linity+2,dx	;  ...

doeither:				; common Mandelbrot, Julia set code
	mov	period,0		; claim periodicity of 1
	mov	savedand,1		; initial periodicity check
	mov	savedincr,1		;  flag for incrementing periodicity
	mov	word ptr savedx+2,0ffffh; impossible value of "old" x
	mov	word ptr savedy+2,0ffffh; impossible value of "old" y
	mov	orbit_ptr,0		; clear orbits

	dec	kbdcount		; decrement the keyboard counter
	jns	short nokey		;  skip keyboard test if still positive
	mov	kbdcount,10		; stuff in a low kbd count
	cmp	show_orbit,0		; are we showing orbits?
	jne	quickkbd		;  yup.  leave it that way.
	mov	kbdcount,5000		; else, stuff an appropriate count val
	cmp	cpu,386 		; ("appropriate" to the CPU)
	je	short kbddiskadj	;  ...
;;	cmp	word ptr delmin+2,1	; is 16-bit math good enough?
	cmp	word ptr delmin+2,8	; is 16-bit math good enough?
	ja	kbddiskadj		;  yes. test less often
	mov	kbdcount,500		;  no.	test more often
kbddiskadj:
	cmp	dotmode,11		; disk video?
	jne	quickkbd		;  no, leave as is
	shr	kbdcount,1		; yes, reduce count
	shr	kbdcount,1		;  ...
quickkbd:
	call	far ptr keypressed	; has a key been pressed?
	cmp	ax,0			;  ...
	je	nokey			; nope.  proceed
	mov	kbdcount,0		; make sure it goes negative again
	cmp	ax,'o'                  ; orbit toggle hit?
	je	orbitkey		;  yup.  show orbits
	cmp	ax,'O'                  ; orbit toggle hit?
	jne	keyhit			;  nope.  normal key.
orbitkey:
	call	far ptr getakey 	; read the key for real
	mov	ax,1			; reset orbittoggle = 1 - orbittoggle
	sub	ax,show_orbit		;  ...
	mov	show_orbit,ax		;  ...
	jmp	short nokey		; pretend no key was hit
keyhit: mov	ax,-1			; return with -1
	mov	color,ax		; set color to -1
	UNFRAME <si,di> 		; pop stack frame
	ret				; bail out!

nokey:
	cmp	show_orbit,0		; is orbiting on?
	jne	no16bitcode		;  yup.  slow down.
	cmp	cpu,386 		; are we on a 386?
	je	short code386bit	;  YAY!! 386-class speed!
;;	cmp	word ptr delmin+2,1	; OK, we're desperate.  16 bits OK?
	cmp	word ptr delmin+2,8	; OK, we're desperate.  16 bits OK?
	ja	yes16bitcode		;  YAY!  16-bit speed!
no16bitcode:
	call	near ptr code32bit	; BOO!! nap time.  Full 32 bit math
	jmp	kloopend		;  bypass the 386-specific code.
yes16bitcode:
	call	near ptr code16bit	; invoke the 16-bit version
	jmp	kloopend		;  bypass the 386-specific code.

.386					; 386-specific code starts here

code386bit:
;;	cmp	word ptr delmin+2,3	; is 16-bit math good enough?
	cmp	word ptr delmin+2,8	; is 16-bit math good enough?
	jbe	code386_32		; nope, go do 32 bit stuff
IFDEF ??version
	jmp	code386_32		; TASM screws up IMUL EBX,EBX!!
ENDIF

	; 16 bit on 386, now we are really gonna move
	movsx	esi,word ptr x+2	; use SI for X
	movsx	edi,word ptr y+2	; use DI for Y
	push	ebp
	mov	ebp,-1
	shl	ebp,FUDGEFACTOR-1
	mov	cx,FUDGEFACTOR-16

kloop386_16:   ; cx=bitshift-16, ebp=overflow.mask

	mov	ebx,esi 		; compute (x * x)
	imul	ebx,ebx 		;  ...
	test	ebx,ebp 		;
	jnz	short end386_16 	;  (oops.  We done.)
	shr	ebx,cl			; get result down to 16 bits

	mov	edx,edi 		; compute (y * y)
	imul	edx,edx 		;  ...
	test	edx,ebp 		; say, did we overflow? <V20-compat>
	jnz	short end386_16 	;  (oops.  We done.)
	shr	edx,cl			; get result down to 16 bits

	mov	ax,bx			; compute (x*x - y*y) / fudge
	sub	bx,dx			;  for the next iteration

	add	ax,dx			; compute (x*x + y*y) / fudge

	cmp	ax,word ptr lm+2	; while (xx+yy < lm)
	jae	short end386_16 	;  ...

	imul	edi,esi 		; compute (y * x)
	shl	edi,1			; ( * 2 / fudge)
	sar	edi,cl
	add	di,word ptr linity+2	; (2*y*x) / fudge + linity
	movsx	edi,di			; save as y

	add	bx,word ptr linitx+2	; (from above) (x*x - y*y)/fudge + linitx
	movsx	esi,bx			; save as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jge	short chkpd386_16	;  yup.  do periodicity check.
nonmax386_16:
	dec	k			; while (k < maxit)
	jnz	short kloop386_16	; try, try again
end386_16:
	pop	ebp
	jmp	kloopend		; we done

chkpd386_16:
	mov	ax,k			; set up to test for save-time
	test	ax,savedand		; save on 0, check on anything else
	jz	short chksv386_16	;  time to save a new "old" value
	mov	ax,si			; load up x
	xor	ax,word ptr savedx+2	; does X match?
	test	ax,word ptr savedmask+2 ;  truncate to appropriate precision
	jne	short nonmax386_16	;  nope.  forget it.
	mov	ax,di			; now test y
	xor	ax,word ptr savedy+2	; does Y match?
	test	ax,word ptr savedmask+2 ;  truncate to appropriate precision
	jne	short nonmax386_16	;  nope.  forget it.
	mov	period,1		; note that we have found periodicity
	mov	k,0			; pretend maxit reached
	jmp	short end386_16
chksv386_16:
	mov	word ptr savedx+2,si	; save x
	mov	word ptr savedy+2,di	; save y
	dec	savedincr		; time to change the periodicity?
	jnz	short nonmax386_16	;  nope.
	shl	savedand,1		; well then, let's try this one!
	inc	savedand		;  (2**n -1)
	mov	savedincr,4		; and reset the increment flag
	jmp	short nonmax386_16

	; 32bit on 386:
code386_32:
	mov	esi,x			; use ESI for X
	mov	edi,y			; use EDI for Y

;	This is the main processing loop.  Here, every T-state counts...

kloop:					; for (k = 0; k <= maxit; k++)

	mov	eax,esi 		; compute (x * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	jne	short kloopend1 	; bail out if too high
	mov	ebx,eax 		; save this for below

	mov	eax,edi 		; compute (y * y)
	imul	edi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	jne	short kloopend1 	; bail out if too high

	mov	ecx,ebx 		; compute (x*x - y*y) / fudge
	sub	ebx,eax 		;  for the next iteration

	add	ecx,eax 		; compute (x*x + y*y) / fudge
	cmp	ecx,lm			; while (lr < lm)
	jae	short kloopend1 	;  ...

	mov	eax,edi 		; compute (y * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR-1	;  ( * 2 / fudge)
	add	eax,linity		;  (above) + linity
	mov	edi,eax 		;  save this as y

;	(from the earlier code) 	; compute (x*x - y*y) / fudge
	add	ebx,linitx		;	+ linitx
	mov	esi,ebx 		; save this as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jge	short chkperiod1
nonmax1:
	dec	k			; while (k < maxit) (dec to 0 is faster)
	jnz	short kloop		; while (k < maxit) ...
kloopend1:
	jmp	short kloopend32	; we done.

chkperiod1:
	mov	eax,esi
	xor	eax,savedx
	test	eax,savedmask
	jnz	short chksave1
	mov	eax,edi
	xor	eax,savedy
	test	eax,savedmask
	jnz	short chksave1
	mov	period,1		; note that we have found periodicity
	mov	k,0			; pretend maxit reached
	jmp	short kloopend1
chksave1:
	mov	ax,k
	test	ax,savedand
	jne	short nonmax1
	mov	savedx,esi
	mov	savedy,edi
	dec	savedincr		; time to change the periodicity?
	jnz	short nonmax1		;  nope.
	shl	savedand,1		; well then, let's try this one!
	inc	savedand		;  (2**n -1)
	mov	savedincr,4		; and reset the increment flag
	jmp	short nonmax1

kloopend32:

.8086					; 386-specific code ends here

kloopend:
	cmp	orbit_ptr,0		; any orbits to clear?
	je	noorbit2		;  nope.
	call	far ptr scrub_orbit	; clear out any old orbits
noorbit2:

	mov	ax,k			; set old color
	sub	ax,10			; minus 10, for safety
	mov	oldcolor,ax		; and save it as the "old" color
	mov	ax,maxit		; compute color
	sub	ax,k			;  (first, re-compute "k")
	sub	kbdcount,ax		; adjust the keyboard count
	cmp	ax,1			; convert any "outlier" region
	jge	short coloradjust1	;  (where abs(x) > 2 or abs(y) > 2)
	mov	ax,1			;   to look like we ran through
coloradjust1:				;    at least one loop.
	mov     realcolor,ax            ; result before adjustments
	cmp     ax,maxit                ; did we max out on iterations?
	jne     short notmax            ;  nope.
	mov     oldcolor,ax             ; set "oldcolor" to maximum
	cmp     inside,0                ; is "inside" >= 0?
	jl      wedone                  ;  nope.  leave it at "maxit"
	mov     ax,inside               ; reset max-out color to default
	cmp     periodicitycheck,0      ; show periodicity matches?
	jge     wedone                  ;  nope.
	mov     al,period               ;  reset color to periodicity flag
	jmp     short wedone

notmax:
	cmp     outside,0               ; is "outside" >= 0?
	jl      wedone                  ;   nope. leave as realcolor
	mov     ax, outside             ; reset to "outside" color

wedone:                                 ;
	mov     color,ax                ; save the color result
	UNFRAME <si,di>                 ; pop stack frame
	ret                             ; and return with color

calcmandasm endp


; ******************** Function code16bit() *****************************
;
;	Performs "short-cut" 16-bit math where we can get away with it.
; CJLT has modified it, mostly by preshifting x and y to fg30 from fg29
; or, since we ignore the lower 16 bits, fg14 from fg13.
; If this shift overflows we are outside x*x+y*y=2, so have escaped.
; Also, he commented out several conditional jumps which he calculated could
; never be taken (e.g. mov ax,si / imul si ;cannot overflow).

code16bit	proc	near

	mov	si,word ptr x+2 	; use SI for X fg13
	mov	di,word ptr y+2 	; use DI for Y fg13

start16bit:
	add	si,si			;CJLT-Convert to fg14
	jo	end16bit		;overflows if <-2 or >2
	mov	ax,si			; compute (x * x)
	imul	si			; Answer is fg14+14-16=fg12
;	cmp	dx,0			;CJLT commented out-
;	jl	end16bit		;CJLT-  imul CANNOT overflow
;	mov	cx,32-FUDGEFACTOR	;CJLT. FUDGEFACTOR=29 is hard coded
loop16bit1:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	jo	end16bit		;  (oops.  overflow)
;	loop	loop16bit1		;CJLT...do it once only. dx now fg13.
	mov	bx,dx			; save this for a tad

;ditto for y*y...

	add	di,di			;CJLT-Convert to fg14
	jo	end16bit		;overflows if <-2 or >2
	mov	ax,di			; compute (y * y)
	imul	di			;  ...
;	cmp	dx,0			; say, did we overflow? <V20-compat>
;	jl	end16bit		;  (oops.  We done.)
;	mov	cx,32-FUDGEFACTOR	; ( / fudge)
;loop16bit2:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	jo	end16bit		;  (oops.  overflow)
;	loop	loop16bit2		;  ...

	mov	cx,bx			; compute (x*x - y*y) / fudge
	sub	bx,dx			;  for the next iteration

	add	cx,dx			; compute (x*x + y*y) / fudge
	jo	end16bit		; bail out if too high
;	js	end16bit		;  ...

	cmp	cx,word ptr lm+2	; while (xx+yy < lm)
	jae	end16bit		;  ...
	dec	k			; while (k < maxit)
	jz	end16bit		;  we done.

	mov	ax,di			; compute (y * x) fg14+14=fg28
	imul	si			;  ...
;	mov	cx,33-FUDGEFACTOR-2	; ( * 2 / fudge)
;loop16bit3:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	shl	ax,1			;  shift two bits
	rcl	dx,1			;  cannot overflow as |x|<=2, |y|<=2
;	loop	loop16bit3		;  ...
	add	dx,word ptr linity+2	; (2*y*x) / fudge + linity
	jo	end16bit		; bail out if too high
	mov	di,dx			; save as y

	add	bx,word ptr linitx+2	; (from above) (x*x - y*y)/fudge + linitx
	jo	end16bit		; bail out if too high
	mov	si,bx			; save as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jle	short nonmax3		;  nope.  bypass periodicity check.
	mov	word ptr x+2,si 	; save x for periodicity check
	mov	word ptr y+2,di 	; save y for periodicity check
	call	checkperiod		; check for periodicity
nonmax3:
	jmp	start16bit		; try, try again.

end16bit:				; we done.
	ret
code16bit	endp


;	The following routine checks for periodic loops (a known
;	method of decay inside "Mandelbrot Lake", and an easy way to
;	bail out of "lake" points quickly).  For speed, only the
;	high-order sixteen bits of X and Y are checked for periodicity.
;	For accuracy, this routine is only fired up if the previous pixel
;	was in the lake (which means that the FIRST "wet" pixel was
;	detected by the dull-normal maximum iteration process).

checkperiod	proc near		; periodicity check
	mov	ax,k			; set up to test for save-time
	test	ax,savedand		; save on 0, check on anything else
	jz	checksave		;  time to save a new "old" value
	mov	dx,word ptr x+2 	; load up x
	and	dx,word ptr savedmask+2 ;  truncate to appropriate precision
	cmp	dx,word ptr savedx+2	; does X match?
	jne	checkdone		;  nope.  forget it.
	mov	ax,word ptr x		; load up x
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	cmp	ax,word ptr savedx	; does X match?
	jne	checkdone		;  nope.  forget it.
	mov	dx,word ptr y+2 	; now test y
	and	dx,word ptr savedmask+2 ;  truncate to appropriate precision
	cmp	dx,word ptr savedy+2	; does Y match?
	jne	checkdone		;  nope.  forget it.
	mov	ax,word ptr y		; load up y
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	cmp	ax,word ptr savedy	; does Y match?
	jne	checkdone		;  nope.  forget it.
	mov	period,1		; note that we have found periodicity
	mov	k,1			; pretend maxit reached
checksave:
	mov	dx,word ptr x+2 	; load up x
	and	dx,word ptr savedmask+2 ;  truncate to appropriate precision
	mov	word ptr savedx+2,dx	;  and save it
	mov	ax,word ptr x		; load up x
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	mov	word ptr savedx,ax	;  and save it
	mov	dx,word ptr y+2 	; load up y
	and	dx,word ptr savedmask+2 ;  truncate to appropriate precision
	mov	word ptr savedy+2,dx	;  and save it
	mov	ax,word ptr y		; load up y
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	mov	word ptr savedy,ax	;  and save it
	dec	savedincr		; time to change the periodicity?
	jnz	checkdone		;  nope.
	shl	savedand,1		; well then, let's try this one!
	inc	savedand		;  (2**n -1)
	mov	savedincr,4		; and reset the increment flag
checkdone:
	ret				; we done.
checkperiod	endp


; ******************** Function code32bit() *****************************
;
;	Perform the 32-bit logic required using 16-bit logic
;
;	New twice as fast logic,
;	   Courtesy of Bill Townsend and Mike Gelvin (CIS:73337,520)
;	Even newer, faster still by Chris Lusby Taylor
;	 who noted that we needn't square the low word if we first multiply
;	 by 4, since we only need 29 places after the point and this will
;	 give 30. (We divide answer by two to give 29 bit shift in answer)
;	Also, he removed all testing for special cases where a word of operand
;	happens to be 0, since testing 65536 times costs more than the saving
;	1 time out of 65536! (I benchmarked it. Just removing the tests speeds
;	us up by 3%.)
;
;Note that square returns DI,AX squared in DX,AX now.
; DI,AX is first converted to unsigned fg31 form.
; (For its square to be representable in fg29 (range -4..+3.999)
; DI:AX must be in the range 0..+1.999 which fits neatly into unsigned fg31.)
; This allows us to ignore the part of the answer corresponding to AX*AX as it
; is less than half a least significant bit of the final answer.
; I thought you'd like that.
;
; As we prescaled DI:AX, we need to shift the answer 1 bit to the right to
; end up in fg29 form since 29=(29+2)+(29+2)-32-1
; However, the mid term AX*DI is needed twice, so the shifts cancel.
;
; Since abs(x) and abs(y) in fg31 form will be needed in calculating 2*X*Y
; we push them onto the stack for later use.

; Note that square does nor affect bl,si,bp
; and leaves highword of argument in di
; but destroys bh,cx
square	MACRO	donepops
	LOCAL	notneg
	shl	ax,1		;Multiply by 2 to convert to fg30
	rcl	di,1		;If this overflows DI:AX was negative
	jnc	notneg
	not	ax			; so negate it
	not	di			; ...
	add	ax,1			; ...
	adc	di,0			; ...
	not	bl			; change negswt
notneg:	shl	ax,1		;Multiply by 2 again to give fg31
	rcl	di,1		;If this gives a carry then DI:AX was >=2.0
				;If its high bit is set then DI:AX was >=1.0
				;This is OK, but note that this means that
				;DI:AX must now be treated as unsigned.
	jc	donepops
	push	di		; save y or x (in fg31 form) on stack
	push	ax		; ...
	mul	di		;GET MIDDLE PART - 2*A*B
	mov	bh,ah		;Miraculously, it needs no shifting!
	mov	cx,dx
	mov	ax,di
	mul	ax		;SQUARE HIGH HWORD - A*A
	shl	bh,1		;See if we round up
	adc	ax,1		;Anyway, add 1 to round up/down accurately
	adc	dx,0
	shr	dx,1		;This needs shifting one bit
	rcr	ax,1
	add	ax,cx		;Add in the 2*A*B term
	adc	dx,0
	ENDM	;#EM


code32bit	proc near
;
; BL IS USED FOR THE "NEGSWT" FLAG
;   NEGSWT IS ONE IF EITHER "X" OR "Y" ARE NEGATIVE, BUT NOT BOTH NEGATIVE
;

	push	bp
	xor	bl,bl			; NEGSWT STARTS OFF ZERO

;	iteration loop

nextit:	mov	ax,word ptr y		; ax=low(y)
	mov	di,word ptr y+2 	; di=high(y)
	square done0	;square y and quit via done0 if it overflows
	mov	si,ax		; square returns results in dx,ax
	mov	bp,dx		; save y*y in bp,si
	mov	ax,word ptr x
	mov	di,word ptr x+2
	square	done2		; square x and quit via done2 if it overflows
	mov	cx,ax		; Save low answer in cx.
	ADD	ax,si		; calc y*y + x*x
	mov	ax,bp
	ADC	ax,dx		;  ...
	jno	nextxy		; overflow?
				;NOTE: The original code tests against lm
				;here, but as lm=4<<29 this is the same
				;as testing for signed overflow
done4:	add	sp,4			; discard saved value of |x| fg 31
done2:	add	sp,4			; discard saved value of |y| fg 31
done0: pop	bp			; restore saved bp
	ret

;---------------------------------------------------------------------------

nextxy:	dec	k		; while (k < maxit)
	jz	done4		;  we done.
	sub	cx,si			; subtract y*y from x*x
	sbb	dx,bp			;  ...
	add	cx,word ptr linitx	; add "A"
	adc	dx,word ptr linitx+2	;  ...
	jo	done4			;CJLT-Must detect overflow here
					; but increment loop count first
	mov	word ptr x,cx		; store new x = x*x-y*y+a
	mov	word ptr x+2,dx 	;  ...

; now calculate x*y
;
;More CJLT tricks here. We use the pushed abs(x) and abs(y) in fg31 form
;which, when multiplied, give x*y in fg30, which, luckily, is the same as...
;2*x*y fg29.
;As with squaring, we can ignore the product of the low order words, and still
;be more accurate than the original algorithm.
;
	pop	bp		;Xlow
	pop	di		;Xhigh (already there, actually)
	pop	ax		;Ylow
	mul	di		;Xhigh * Ylow
	mov	bh,ah		;Discard lowest 8 bits of answer
	mov	cx,dx
	pop	ax		;Yhigh
	mov	si,ax		; copy it
	mul	bp		;Xlow * Yhigh
	xor	bp,bp		;Clear answer
	add	bh,ah
	adc	cx,dx
	adc	bp,0
	mov	ax,si		;Yhigh
	mul	di		;Xhigh * Yhigh
	shl	bh,1		;round up/down
	adc	ax,cx		;Answer-low
	adc	dx,bp		;Answer-high
				;NOTE: The answer is 0..3.9999 in fg29
	js	done0		;Overflow if high bit set
	or	bl,bl		; ZERO IF NONE OR BOTH X , Y NEG
	jz	signok		; ONE IF ONLY ONE OF X OR Y IS NEG
	not	ax		; negate result
	not	dx		;  ...
	add	ax,1		;  ...
	adc	dx,0		;  ...
	xor	bl,bl		;Clear negswt
signok:
	add	ax,word ptr linity
	adc	dx,word ptr linity+2	; dx,ax = 2(X*Y)+B
	jo	done0
	mov	word ptr y,ax		; save the new value of y
	mov	word ptr y+2,dx 	;  ...
	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jle	short chkmaxit		;  nope.  bypass periodicity check.
	call	checkperiod		; check for periodicity

chkmaxit:
	cmp	show_orbit,0		; orbiting on?
	jne	horbit			;  yep.
	jmp	nextit			;go around again

jmpdone0:
	jmp	done0			; DOES [(X*X)-(Y*Y)+P] BEFORE THE DEC.

horbit: push	bx			; save my flags
	mov	ax,-1			; color for plot orbit
	push	ax			;  ...
	push	word ptr y+2		; co-ordinates for plot orbit
	push	word ptr y		;  ...
	push	word ptr x+2		;  ...
	push	word ptr x		;  ...
	call	far ptr iplot_orbit	; display the orbit
	add	sp,5*2			; clear out the parameters
	pop	bx			; restore flags
	jmp	nextit			; go around again

code32bit	endp

	   end
