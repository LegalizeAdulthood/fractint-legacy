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
	cmp	word ptr delmin+2,1	; is 16-bit math good enough?
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
	cmp	word ptr delmin+2,1	; OK, we're desperate.  16 bits OK?
	ja	yes16bitcode		;  YAY!  16-bit speed!
no16bitcode:
	call	near ptr code32bit	; BOO!! nap time.  Full 32 bit math
	jmp	kloopend		;  bypass the 386-specific code.
yes16bitcode:
	call	near ptr code16bit	; invoke the 16-bit version
	jmp	kloopend		;  bypass the 386-specific code.

.386					; 386-specific code starts here

code386bit:
	cmp	word ptr delmin+2,3	; is 16-bit math good enough?
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
	test	ebx,ebp 		; say, did we overflow? <V20-compat>
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
	mov	realcolor,ax		; result before adjustments
	cmp	ax,maxit		; did we max out on iterations?
	jne	short wedone		;  nope.
	mov	oldcolor,ax		; set "oldcolor" to maximum
	cmp	inside,0		; is "inside" >= 0?
	jl	wedone			;  nope.  leave it at "maxit"
	mov	ax,inside		; reset max-out color to default
	cmp	periodicitycheck,0	; show periodicity matches?
	jge	wedone			;  nope.
	mov	al,period		;  reset color to periodicity flag
wedone: 				;
	mov	color,ax		; save the color result
	UNFRAME <si,di> 		; pop stack frame
	ret				; and return with color

calcmandasm endp


; ******************** Function code16bit() *****************************
;
;	Performs "short-cut" 16-bit math where we can get away with it.
;

code16bit	proc	near

	mov	si,word ptr x+2 	; use SI for X
	mov	di,word ptr y+2 	; use DI for Y

start16bit:
	mov	ax,si			; compute (x * x)
	imul	si			;  ...
	cmp	dx,0			; say, did we overflow? <V20-compat>
	jl	end16bit		;  (oops.  We done.)
	mov	cx,32-FUDGEFACTOR	; ( / fudge)
loop16bit1:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	jo	end16bit		;  (oops.  overflow)
	loop	loop16bit1		;  ...
	mov	bx,dx			; save this for a tad

	mov	ax,di			; compute (y * y)
	imul	di			;  ...
	cmp	dx,0			; say, did we overflow? <V20-compat>
	jl	end16bit		;  (oops.  We done.)
	mov	cx,32-FUDGEFACTOR	; ( / fudge)
loop16bit2:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	jo	end16bit		;  (oops.  overflow)
	loop	loop16bit2		;  ...

	mov	cx,bx			; compute (x*x - y*y) / fudge
	sub	bx,dx			;  for the next iteration

	add	cx,dx			; compute (x*x + y*y) / fudge
	jo	end16bit		; bail out if too high
	js	end16bit		;  ...

	cmp	cx,word ptr lm+2	; while (xx+yy < lm)
	jae	end16bit		;  ...

	mov	ax,di			; compute (y * x)
	imul	si			;  ...
	mov	cx,33-FUDGEFACTOR	; ( * 2 / fudge)
loop16bit3:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	loop	loop16bit3		;  ...
	add	dx,word ptr linity+2	; (2*y*x) / fudge + linity
	mov	di,dx			; save as y

	add	bx,word ptr linitx+2	; (from above) (x*x - y*y)/fudge + linitx
	mov	si,bx			; save as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jl	short nonmax3		;  nope.  bypass periodicity check.
	mov	word ptr x+2,si 	; save x for periodicity check
	mov	word ptr y+2,di 	; save y for periodicity check
	call	checkperiod		; check for periodicity
nonmax3:

	dec	k			; while (k < maxit)
	jz	end16bit		;  we done.

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

square	macro	doneaddr
	local	donejp,skip1y,skip2y,squrey,allzero
	sub	bh,bh
	sub	cx,cx
	mov	bp,cx
	or	ax,ax			;GET LOW HWORD
	jz	skip1y			;LOW HWORD IS ZERO, ONLY DO HIGH HWORD
	mov	si,ax
	mul	ax			;SQUARE LOW HWORD - B*B
	mov	bh,dh
	or	di,di			;TEST HIGH HWORD
	jz	squrey			;HIGH HWORD ZERO, SKIP THE FOLLOWING
	mov	ax,di
	mul	si			;GET MIDDLE PART - A*B
	add	bh,ah			;add
	adc	cx,dx			; A*B
	adc	bp,0			;  twice
	add	bh,ah			;   -
	adc	cx,dx			;    -
	adc	bp,0			;     SI,CX,BH = 2(A*B)+(B*B)
	jmp	short skip2y
donejp: JMP	doneaddr		;M0=DONEJP
skip1y: or	di,di			;M1=SKIP1Y
	jz	allzero
skip2y: mov	ax,di			;M2=SKIP2Y
	mul	ax			;SQUARE HIGH HWORD - A*A
	add	cx,ax
	adc	bp,dx
squrey: shl	bh,1			;M3=SQUREY
	rcl	cx,1
	rcl	bp,1
;	jo	donejp			; squaring a +ve, top bit known off
	shl	bh,1
	rcl	cx,1
	rcl	bp,1
;	jo	donejp			; squaring a +ve, 2nd bit known off
	shl	bh,1
	rcl	cx,1
	rcl	bp,1
	jo	donejp
	add	bp,0
	js	donejp			; if went neg have overflow also
allzero:

	endm	;#EM


code32bit	proc near
;
; BL IS USED FOR THE FLAGS,TWO LOW ORDER BITS ARE "NEGSWT", THIRD BIT IS XSIGN
;   XSIGN IS ONE IF THE SIGN OF THE NEW "X" IS NEGATIVE
;   NEGSWT IS ONE IF EITHER "X" OR "Y" ARE NEGATIVE, BUT NOT BOTH NEGATIVE
;
xsign	equ	00000100b
negswt	equ	00000001b

	push	bp
	sub	si,si			; make a zero __________
	mov	bx,si			; BOTH XSIGN & NEGSWT START OFF ZERO
	mov	di,1			; MAKE A ONE
	mov	cx,word ptr y		; cx=y
	mov	bp,word ptr y+2 	; bp=y+2
	cmp	bp,si			; y>0?
	jns	qnotng			;  yup
	not	cx			; nope, negate it
	not	bp			; ...
	add	cx,di			; ...
	adc	bp,si			; ...
	inc	bl			; INCREMENT negswt
qnotng: mov	ax,word ptr x
	mov	dx,word ptr x+2
	cmp	dx,si			; x>0?
	jns	pnotng			;  yup
	not	ax			; nope, negate it
	not	dx			; ...
	add	ax,di			; ...
	adc	dx,si			; ...
	inc	bl			; INCREMENT negswt
pnotng: mov	word ptr absx,ax	; save unsigned x
	mov	word ptr absx+2,dx	; ...

;	iteration loop
nextit: push	bp			; save y on stack
	push	cx			; ...
	mov	ax,cx			; load y for squaring
	mov	di,bp			;  ...
	square	done2			; square di,ax & br to jmp_done2 if overflow
	push	cx			; return results in bp,cx
	push	bp			; save y*y on stack
	mov	ax,word ptr absx	; load x for squaring
	mov	di,word ptr absx+2	;  ...
	square	done			; square di,ax & br to done if overflow
	mov	di,bp			; return results in bp,cx
	mov	si,cx			; save x*x in di,si
	pop	dx			; unstack y*y into dx,ax
	pop	ax			;  ...
	ADD	cx,ax			; calc y*y + x*x
	ADC	bp,dx			;  ...
	jo	done2			; overflow?
	js	done2			; overflow?
	cmp	bp,word ptr lm+2	; while (lr < lm)
	jb	short nextxy		;  keep going
	jmp	short done2		; magnitude is past limit, bailout
done:	add	sp,4			; discard saved value of "y*y"
done2:	add	sp,4			; discard saved value of "y"
xydone: pop	bp			; restore saved bp
	ret

;---------------------------------------------------------------------------
;y=nz, x=nz, x+2=z
x2is0:	pop	ax			; get old y+2
	or	ax,ax			; test y+2
	jz	jgotxy
	mul	si			; do y+2(ax)*x(si)
	add	bh,ah
	adc	cx,dx
	adc	bp,0
jgotxy: jmp	gotxy

;x=z
xis0:	or	di,di			; test x+2
	jz	popy_2			; zero all done, around again please
	mul	di			; do y(ax)*x+2(di)
	add	bh,ah
	adc	cx,dx
	adc	bp,0
	pop	ax			; get old y+2
	or	ax,ax
	jz	gotxy
mulxyj: jmp	mulxy2

;x=z, x+2=z
popy_2: pop	ax			; discard old y+2
	sub	cx,cx			; x==0, so x*y is zero
	mov	bp,cx			; ...
	jmp	signok			; go add b and store new y

nextxy: sub	si,ax			; subtract y*y from x*x
	sbb	di,dx			;  ...
	add	si,word ptr linitx	; add "A"
	adc	di,word ptr linitx+2	;  ...
	mov	word ptr x,si		; store new x = x*x+y*y+a
	mov	word ptr x+2,di 	;  ...
	jns	swtxxx			; NO SIGN, NOT NEG
	or	bl,xsign		; REMEMBER THE NEW "X" IS NEGATIVE
	not	si			; make it positive in register
	not	di			;  ...
	add	si,1			;  ...
	adc	di,0			;  ...
swtxxx: xchg	di,word ptr absx+2	; save the new x, load the old
	xchg	si,word ptr absx	;  ...

	; now calculate x*y
	sub	bh,bh			; zero some registers
	sub	cx,cx			;  ...
	mov	bp,cx			;  ...
	pop	ax			; get old "y"
	or	ax,ax			; test "y"
	jz	yis0			; br if "y" is zero
	or	si,si			; test old "x"
	jz	xis0			; br if "x" is zero
	mov	bp,ax			; save old "y"
	mul	si			; do y(ax)*x(si)
	mov	ax,bp			; get old "y" again
	mov	bp,cx			; RE-ZERO BP
	mov	bh,dh			; save/set low hword(bx)
	or	di,di			; test old "x+2"
	jz	x2is0
	mul	di			; do y(ax)*x+2(di)
	add	bh,ah
	adc	cx,dx
	adc	bp,0

yis0:	pop	ax			; get old "y+2"
	or	ax,ax			; test y+2
	jz	gotxy			; br if y+2 is zero
	mov	dx,si			; dx="x"
	mov	si,ax			; si=save "y+2"
	mul	dx			; do y+2(ax)*x(dx)--(si)
	add	bh,ah
	adc	cx,dx
	adc	bp,0

	mov	ax,si			; get old "y+2"
mulxy2: mul	di			; do y+2(ax)*x+2(di)
	add	cx,ax
	adc	bp,dx

gotxy:	shl	bh,1
	rcl	cx,1			; bp,cx,bx
	rcl	bp,1
	jo	jmpxydone
	shl	bh,1
	rcl	cx,1
	rcl	bp,1
	jo	jmpxydone
	shl	bh,1
	rcl	cx,1
	rcl	bp,1
	jo	jmpxydone
	add	bp,0
	js	jmpxydone		; if went neg have overflow also

	test	bl,negswt		; ZERO IF NONE OR BOTH X , Y NEG
	jz	signok			; ONE IF ONLY ONE OF X OR Y IS NEG
	not	cx			; negate result
	not	bp			;  ...
	add	cx,1			;  ...
	adc	bp,0			;  ...
signok:
	shr	bl,1
	shr	bl,1			;MOVE "XSIGN"(X=NEG) DOWN TO "NEGSWT"
	add	cx,cx			;bp,CX = 2(X*Y)
	adc	bp,bp

	add	cx,word ptr linity
	adc	bp,word ptr linity+2	; BP,CX = 2(X*Y)+B
	mov	word ptr y,cx		; save the new value of y
	mov	word ptr y+2,bp 	;  ...
	jns	jmpnit			; NO SIGN, NOT NEG
	inc	bl			; INCREMENT NEGSWT
	not	cx			; negate
	not	bp			;  ...
	add	cx,1			;  ...
	adc	bp,0			;  ...

jmpnit: mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jl	short chkmaxit		;  nope.  bypass periodicity check.
	call	checkperiod		; check for periodicity

chkmaxit:
	dec	k			; while (k < maxit)
	jz	jmpxydone		;  we done.
	cmp	show_orbit,0		; orbiting on?
	jne	horbit			;  yep.
	jmp	nextit			;go around again

jmpxydone:
	jmp	xydone			; DOES [(X*X)-(Y*Y)+P] BEFORE THE DEC.

horbit: push	bx			; save my flags
	push	bp			; and registers
	push	cx			;  ...
	mov	ax,-1			; color for plot orbit
	push	ax			;  ...
	push	word ptr y+2		; co-ordinates for plot orbit
	push	word ptr y		;  ...
	push	word ptr x+2		;  ...
	push	word ptr x		;  ...
	call	far ptr iplot_orbit	; display the orbit
	add	sp,5*2			; clear out the parameters
	pop	cx			; restore registers
	pop	bp			;  ...
	pop	bx			; and flags
	jmp	nextit			; go around again

code32bit	endp


calcmand_TEXT	ends

	   end

