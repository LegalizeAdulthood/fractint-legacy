;	CALCMAND.ASM - Mandelbrot/Julia Set calculation Routines

;	The routines in this code perform Mandelbrot and Julia set
;	calculations using 32-bit integer math as opposed to the 
;	"traditional" floating-point approach.

;	This code relies on several tricks to run as quickly as it does.

;	One can fake floating point arithmetic by using integer
;	arithmetic and keeping track of the implied decimal point
;	if things are reasonable -- and in this case, they are.
;	I replaced code that looked like: z = x*y with code that
;	looks like:
;			ix = x * ifudge			(outside the loops)
;			iy = y * ifudge
;			....
;			iz = (ix * iy) / ifudge		(inside the loops)
;	(and keep remembering that all the integers are "ifudged" bigger)

;	The 386 has native 32-bit integer arithmetic, and (briefly) keeps
;	64-bit values around after 32-bit multiplies.   If the result is
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


;					Bert Tyler


;			 required for compatibility if Turbo ASM
IFDEF ??Version
MASM51
QUIRKS
ENDIF

.MODEL	medium,c

.8086

        ; these must NOT be in any segment!!
        ; this get's rid of TURBO-C fixup errors

	extrn	asmdotwrite:far		; this routine is in 'general.asm'
	extrn	asmvideocleanup:far	; this routine is in 'general.asm'
	extrn	keypressed:far		; this routine is in 'general.asm'
	extrn	getakey:far		; this routine is in 'general.asm'
	extrn	iplot_orbit:far		; this routine is in 'calcfrac.c'
	extrn	scrub_orbit:far		; this routine is in 'calcfrac.c'
	extrn	intpotential:far	; this routine is in 'calcfrac.c'

.DATA

FUDGEFACTOR	equ	29		; default (non-potential) fudgefactor
FUDGEFACTOR2	equ	24		; potential algoithm variant

; ************************ External variables *****************************

	extrn	fractype:word		; == 0 if Mandelbrot set, else Julia
	extrn	numpasses:word		; == 0 if single-pass, 1 if 2-pass
	extrn	inside:word		; "inside" color, normally 1 (blue)
	extrn	creal:dword, cimag:dword ; Julia Set Constant
	extrn	lx0:dword, ly0:dword	; arrays of (dword) increment values
	extrn	delx:dword, dely:dword	; actual increment values
	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	bitshift:word		; number of bits to shift
	extrn	lm:dword		; magnitude bailout limit
	extrn	potflag:word		; "continuous potential" flag

	extrn	ixstart:word, ixstop:word ; start, stop here
	extrn	iystart:word, iystop:word ; start, stop here
	extrn	guessing:word		  ; == 1 if solid-guessing

	extrn	cpu:word		; cpu type: 86, 186, 286, or 386

	extrn	andcolor:word		; "and" value used for color selection

	extrn	show_orbit:word		; "show-orbit" flag
	extrn	orbit_ptr:word		; "orbit pointer" flag

	extrn	symmetry:word		; "Symmetry" flag

	extrn	debugflag:word		; for debugging purposes only

; ************************ Internal variables *****************************

passnum		db	0		; pass number: 1 (blitz) or 0

ix		dw	0		; x-axis: 0 to (xdots-1)
iy		dw	0		; y-axis: 0 to (ydots-1)
x		dd	0		; temp value: x
y		dd	0		; temp value: y
a		dd	0		; temp value: a
b		dd	0		; temp value: b
xx		dd	0		; temp value: x-squared
yy		dd	0		; temp value: y-squared
xy		dd	0		; temp value: x-times-y
xxmyy		dd	0		; temp value: x-squared minus y-squared
sign		db	0		; sign calue: used by 16-bit multiplies
temp		dw	4 dup (0)	; temp value: used by 16-bit multiplies
k		dw	0		; local counter
kbdcount	dw	0		; keyboard counter
kbdflag		dw	0		; keyboard hit flag: 0 if no, 1 if yes
color		db	0		; the color to set a pixel
oldcolor	dw	0		; == "old" color
period		db	0		; periodicity, if in the lake
savedand	dw	0		; AND value for periodicity checks
savedincr	dw	0		; flag for incrementing AND value
savedmask	dd	0		; saved values mask
savedmax	dd	0		; saved max(delx, dely)
savedx		dd	0		; saved values of X and Y iterations
savedy		dd	0		;  (for periodicity checks)

.CODE

; ***************** Function calcmand() **********************************

calcmand proc	uses di si es

	push	es			; save the original ES value
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	ax,word ptr delx	; compute the periodicity check value
	mov	dx,word ptr delx+2	; first find largest of delx, dely
	or	ax,word ptr dely	; (only care about the first bit,
	or	dx,word ptr dely+2	;  so this klooge works)
	mov	word ptr savedmax,ax	; save this for later
	mov	word ptr savedmax+2,dx	;  ...
	mov	word ptr savedmask,0	; starting value for savedmask
	mov	word ptr savedmask+2,0c000h	;  ...
maskloop:
	sar	word ptr savedmask+2,1	; shift savedmask right one bit
	rcr	word ptr savedmask,1	;  ...
	shl	ax,1			; now shift delx.y left one bit
	rcl	dx,1			;  ...
	jnc	maskloop		; loop if no one bits hit yet

	mov	kbdcount,-1		; initialize keyboard counter (to -1)
	mov	kbdflag,0		; initialize keyboard int flag: nope

	mov	ax,numpasses		; detect 1 or 2-pass mode
	mov	passnum,al		; initialize pass counter
	cmp	guessing,2		; are we guessing and on pass 2?
	jne	passloop		; nope.  proceed.
	dec	passnum			;  else, assume pass #2.

passloop:
	mov	ax,iystart		; initialize outer loop
	mov	iy,ax			;  ...

yloop:					; for (y = iystart; y <= iystop; y++)
	mov	ax,ixstart		; initialize inner loop
	mov	ix,ax			;  ...
	mov	oldcolor,0		; set (dummy) flag: old color was high

xloop:					; for (x = ixstart; x <= ixstop; x++)
	mov	bx,ix			; pull lx0 value out of the array
	shl	bx,1			; convert to double-word pointer
	shl	bx,1			;  ...
	mov	ax,word ptr lx0[bx]	;  here it is!
	mov	dx,word ptr lx0+2[bx]	;  ...
	mov	word ptr a,ax		; save it for later
	mov	word ptr a+2,dx		;  ...

	mov	bx,iy			; pull ly0 value out of the array
	shl	bx,1			; convert to double-word pointer
	shl	bx,1			;  ...
	mov	ax,word ptr ly0[bx]	;  here it is!
	mov	dx,word ptr ly0+2[bx]	;  ...
	mov	word ptr b,ax		; save it for later
	mov	word ptr b+2,dx		;  ...

	mov	ax,word ptr creal	; initialize x == creal
	mov	dx,word ptr creal+2	;  ...
	mov	word ptr x,ax		;  ...
	mov	word ptr x+2,dx		;  ...

	mov	ax,word ptr cimag	; initialize y == cimag
	mov	dx,word ptr cimag+2	;  ...
	mov	word ptr y,ax		;  ...
	mov	word ptr y+2,dx		;  ...

	mov	ax,maxit		; setup k = maxit
	inc	ax			; (+ 1)
	mov	k,ax			;  (decrementing to 0 is faster)

	cmp	fractype,1		; julia or mandelbrot set?
	je	short dojulia		; julia set - go there

	cmp	word ptr x,0		; Mandelbrot shortcut:
	jne	short doeither		;  if creal = cimag = 0,
	cmp	word ptr x+2,0		; the first iteration can be emulated.
	jne	short doeither		;  ...
	cmp	word ptr y,0		;  ...
	jne	short doeither		;  ...
	cmp	word ptr y+2,0		;  ...
	jne	short doeither		;  ...
	dec	k			; we know the first iteration passed
	mov	dx,word ptr a+2		; copy x = a
	mov	ax,word ptr a		;  ...
	mov	word ptr x+2,dx		;  ...
	mov	word ptr x,ax		;  ...
	mov	dx,word ptr b+2		; copy y = b
	mov	ax,word ptr b		;  ...
	mov	word ptr y+2,dx		;  ...
	mov	word ptr y,ax		;  ...

dojulia:				; Julia Set initialization
					; "fudge" Mandelbrot start-up values
	mov	ax,word ptr x		; switch x with a
	mov	dx,word ptr x+2		;  ...
	mov	bx,word ptr a		;  ...
	mov	cx,word ptr a+2		;  ...
	mov	word ptr x,bx		;  ...
	mov	word ptr x+2,cx		;  ...
	mov	word ptr a,ax		;  ...
	mov	word ptr a+2,dx		;  ...

	mov	ax,word ptr y		; switch y with b
	mov	dx,word ptr y+2		;  ...
	mov	bx,word ptr b		;  ...
	mov	cx,word ptr b+2		;  ...
	mov	word ptr y,bx		;  ...
	mov	word ptr y+2,cx		;  ...
	mov	word ptr b,ax		;  ...
	mov	word ptr b+2,dx		;  ...

doeither:				; common Mandelbrot, Julia set code

	cmp	numpasses,0		; multiple-pass mode?
	jz	short singlepass	;  nope.  proceed.
	cmp	passnum,0		; second pass?
	jne	short singlepass	;  nope.  proceed
	test	iy,1			; odd dot?
	jnz	short singlepass	;  yup.  proceed.
	test	ix,1			; odd dot?
	jnz	short singlepass	;  yup.  proceed.
	jmp	loopchecks		;  nope. skip it.
singlepass:

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
	cmp	cpu,386			; ("appropriate" to the CPU)
	je	short quickkbd		;  ...
	cmp	word ptr savedmax+2,1	; is 16-bit math good enough?
	ja	quickkbd		;  yes. test less often
	mov	kbdcount,500		;  no.  test more often
quickkbd:
	call	far ptr keypressed	; has a key been pressed?
	cmp	ax,0			;  ...
 	je	nokey			; nope.  proceed
        cmp	ax,'o'			; orbit toggle hit?
	je	orbitkey		;  yup.  show orbits
	cmp	ax,'O'			; orbit toggle hit?
	jne	keyhit			;  nope.  normal key.
orbitkey:
	call	far ptr getakey		; read the key for real
	mov	ax,1			; reset orbittoggle = 1 - orbittoggle
	sub	ax,show_orbit		;  ...
	mov	show_orbit,ax		;  ...
	mov	kbdcount,10		; adjust the keyboard
	jmp	short nokey		; pretend no key was hit
keyhit:	mov	kbdflag,1		; yup.  reset kbd-hit flag: yes.
	jmp	wedone			; so, bail out!

nokey:
	cmp	show_orbit,0		; is orbiting on?
	jne	no16bitcode		;  yup.  slow down.
	cmp	word ptr savedmax+2,3	; is 16-bit math good enough?
	ja	yes16bitcode		;  YAY!!  16-bit speed!
	cmp	cpu,386			; are we on a 386?
	je	short code386bit	;  YAY!! 386-class speed!
	cmp	word ptr savedmax+2,1	; OK, we're desperate.  16 bits OK?
	ja	yes16bitcode		;  YAY!  16-bit speed!
no16bitcode:
	call	near ptr code32bit	; BOO!! nap time.  Full 32 bit math
	jmp	kloopend		;  bypass the 386-specific code.
yes16bitcode:
	call	near ptr code16bit	; invoke the 16-bit version
	jmp	kloopend		;  bypass the 386-specific code.

.386					; 386-specific code starts here

code386bit:
	mov	esi,x			; use ESI for X
	mov	edi,y			; use EDI for Y

;	This is the main processing loop.  Here, every T-state counts...

	cmp	bitshift,29		; only pathalogical concern for speed
	jne	kloop24			; causes us to use two algorithms here

kloop:					; for (k = 0; k <= maxit; k++)

	mov	eax,esi			; compute (x * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals)
	jne	short kloopend1		; bail out if too high
	mov	ebx,eax			; save this for below

	mov	eax,edi			; compute (y * y)
	imul	edi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals) 
	jne	short kloopend1		; bail out if too high

	mov	ecx,ebx			; compute (x*x - y*y) / fudge
	sub	ebx,eax			;  for the next iteration

	add	ecx,eax			; compute (x*x + y*y) / fudge
	jo	short kloopend1		; bail out if too high
	js	short kloopend1		;  ...
	cmp	ecx,lm			; while (lr < lm)
	jae	short kloopend1		;  ...

	mov	eax,edi			; compute (y * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR-1	;  ( * 2 / fudge)
	add	eax,b			;  (above) + b
	mov	edi,eax			;  save this as y

;	(from the earlier code)		; compute (x*x - y*y) / fudge
	add	ebx,a			;       + a
	mov	esi,ebx			; save this as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jb	short nonmax1		;  nope.  bypass periodicity check.
	mov	x,esi			; save x and y
	mov	y,edi			; for the periodicity check
	call	checkperiod		; check for periodicity
nonmax1:

	dec	k			; while (k < maxit) (dec to 0 is faster)
	jnz	short kloop		; while (k < maxit) ...
kloopend1:
	jmp	kloopend32		; we done.

kloop24:				; for (k = 0; k <= maxit; k++)

	mov	eax,esi			; compute (x * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR2	; ( / fudge)
	shr	edx,FUDGEFACTOR2-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals)
	jne	short kloopend32	; bail out if too high
	mov	ebx,eax			; save this for below

	mov	eax,edi			; compute (y * y)
	imul	edi			;  ...
	shrd	eax,edx,FUDGEFACTOR2	; ( / fudge)
	shr	edx,FUDGEFACTOR2-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals) 
	jne	short kloopend32	; bail out if too high

	mov	ecx,ebx			; compute (x*x - y*y) / fudge
	sub	ebx,eax			;  for the next iteration

	add	ecx,eax			; compute (x*x + y*y) / fudge
	mov	dword ptr temp+4,ecx	; save the last iteration value here
	jo	short kloopend32	; bail out if too high
	js	short kloopend32	;  ...
	cmp	ecx,lm			; while (lr < lm)
	jae	short kloopend32	;  ...

	mov	eax,edi			; compute (y * x)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR2-1	;  ( * 2 / fudge)
	add	eax,b			;  (above) + b
	mov	edi,eax			;  save this as y

;	(from the earlier code)		; compute (x*x - y*y) / fudge
	add	ebx,a			;       + a
	mov	esi,ebx			; save this as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jb	short nonmax241		;  nope.  bypass periodicity check.
	mov	x,esi			; save x and y
	mov	y,edi			; for the periodicity check
	call	checkperiod		; check for periodicity
nonmax241:

	dec	k			; while (k < maxit) (dec to 0 is faster)
	jnz	short kloop24		; while (k < maxit) ...
	jmp	kloopend32		; we done.


kloopend32:

.8086					; 386-specific code ends here

kloopend:
	cmp	orbit_ptr,0		; any orbits to clear?
	je	noorbit2		;  nope.
	push	es			; save es
	call	far ptr scrub_orbit	; clear out any old orbits
	pop	es			; restore es
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
	cmp	ax,maxit		; did we max out on iterations?
	jne	short coloradjust2	;  nope.
	mov	oldcolor,ax		; set "oldcolor" to maximum
	add	oldcolor,10		;  plus a few
	cmp	inside,0		; is "inside" >= 0?
	jl	coloradjust2		;  nope.  leave it at "maxit"
	mov	ax,inside		; reset max-out color to default
	cmp	debugflag,98		; show periodicity matches?
	jne	coloradjust2		;  nope.
	mov	al,period		;  reset color to periodicity flag
coloradjust2:				;  
	cmp	potflag,0		; continuous potential algorithm?
	je	coloradjust3		;  nope.  proceed.
	push	es			; prepare to call intpotential
	push	ax			; parameter #2 == the color
	mov	ax,word ptr temp+6	; parameter #1 == the overflow value
	push	ax			;  ...
	mov	ax,word ptr temp+4	;  ...
	push	ax			;  ...
	call	far ptr intpotential	; convert the color to potential
	pop	es			; restore the stack and registers
	pop	es			;  ...
	pop	es			;  ...
	pop	es			;  ...
coloradjust3:				;  
	and	ax,andcolor		; select the color
	mov	color,al		; color result

	mov	cx,ix			; set up the registers
	mov	dx,iy			; for the write routine
	mov	al,color		; color to use
	call	asmdotwrite		; invoke the appropriate write-a-dot

	cmp	symmetry,0		; does symmetry apply?
	jne	short passcheck		; nope.  proceed.

	mov	cx,ix			; set up the registers
	mov	dx,ydots		; for the write routine
	sub	dx,iy			;  (symmetrical version)
	dec	dx			;  ...
	cmp	fractype,0		; Mandelbrot set?
	je	sym01			;  yup.  Skip Julia symmetry
	neg	cx			; convert ix 
	add	cx,xdots		;  to (xdots-ix)
sym01:	mov	al,color		; color to use
	call	asmdotwrite		; invoke the appropriate write-a-dot

passcheck:
	cmp	passnum,0		; final pass?
	jne	passcheck1		;  nope. write more dots.
	jmp	loopchecks		; yup.  proceed.
passcheck1:
	mov	cx,ix			; set up the registers
	mov	dx,iy			; for the write routine
	inc	cx			; (but for the next x-dot)
	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again
	mov	cx,ix			; set up the registers
	mov	dx,iy			; for the write routine
	inc	dx			; (but for the next y-dot)
	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again
	mov	cx,ix			; set up the registers
	mov	dx,iy			; for the write routine
	inc	cx			; (but for the next x-dot)
	inc	dx			; (and the next y-dot)
	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again
	inc	ix			; note extra dots have been written

	cmp	symmetry,0		; does symmetry apply?
	jne	short loopchecks	; nope.  proceed.

	mov	cx,ix			; set up the registers
	mov	dx,ydots		; for the write routine
	sub	dx,iy			;  (symmetrical version)
	dec	dx			;  ...
	cmp	fractype,0		; Mandelbrot set?
	je	sym02			;  yup.  Skip Julia symmetry
	neg	cx			; convert ix 
	add	cx,xdots		;  to (xdots-ix)
sym02:	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again
	mov	cx,ix			; set up the registers
	mov	dx,ydots		; for the write routine
	sub	dx,iy			;  (symmetrical version)
	dec	dx			;  ...
	dec	cx			; (but for the previous x-dot)
	dec	dx			; (but for the previous y-dot)
	cmp	fractype,0		; Mandelbrot set?
	je	sym03			;  yup.  Skip Julia symmetry
	neg	cx			; convert ix 
	add	cx,xdots		;  to (xdots-ix)
sym03:	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again
	mov	cx,ix			; set up the registers
	mov	dx,ydots		; for the write routine
	sub	dx,iy			;  (symmetrical version)
	dec	dx			;  ...
	dec	dx			; (and the previous y-dot)
	cmp	fractype,0		; Mandelbrot set?
	je	sym04			;  yup.  Skip Julia symmetry
	neg	cx			; convert ix 
	add	cx,xdots		;  to (xdots-ix)
sym04:	mov	al,color		; color to use
	call	asmdotwrite		; write the dot again

loopchecks:
	inc	ix			; check for end of xloop
	mov	ax,ix			;  ...
	cmp	ax,ixstop		;  ...
	ja	loopcheck1		; we done.
	jmp	xloop			; more to go
loopcheck1:

	cmp	passnum,0		; last pass?
	je	short	lastpass	;  yup.  proceed.
	inc	iy			; adjust y-value
lastpass:

	inc	iy			; check for end of yloop
	mov	ax,iy			;  ...
	cmp	ax,iystop		;  ...
	ja	loopcheck2		; we done.
	cmp	symmetry,0		; does symmetry apply?
	je	symcheck2		;  yup.  go check.
	jmp	yloop			;  nope.  more to go

symcheck2:				; symmetry applies
	mov	ax,ydots		; gather the total dot count
	shr	ax,1			; are we done with the top half?
	cmp	iy,ax			;  ...
	jae	loopcheck2		; yup.  we done.
	jmp	yloop			; nope.  more to go

loopcheck2:

	dec	passnum			; decrement the pass counter
	js	loopcheck3		; we done.
	cmp	guessing,1		; are we guessing?
	je	loopcheck3		;  yes.  we're done.
	jmp	passloop		; more to go
loopcheck3:

wedone:					; restore everything and return.
	call	asmvideocleanup		; perform any video cleanup required
	pop	es			; restore the original ES value
	mov	ax,kbdflag		; return the keyboard-interrupt flag
	ret				; and return.

calcmand endp

; ******************** Function code16bit() *****************************
;
;	Performs "short-cut" 16-bit math where we can get away with it.
;

code16bit	proc	near

	mov	si,word ptr x+2		; use SI for X
	mov	di,word ptr y+2		; use DI for Y

start16bit:
	mov	ax,si			; compute (x * x)
	imul	si			;  ...
	cmp	dx,0			; say, did we overflow? <V20-compat>
	jl	end16bit		;  (oops.  We done.)
	mov	cx,32			; ( / fudge)
	sub	cx,bitshift		;  ...
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
	mov	cx,32			; ( / fudge)
	sub	cx,bitshift		;  ...
loop16bit2:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	jo	end16bit		;  (oops.  overflow)
	loop	loop16bit2		;  ...

	mov	cx,bx			; compute (x*x - y*y) / fudge
	sub	bx,dx			;  for the next iteration

	add	cx,dx			; compute (x*x + y*y) / fudge
	mov	word ptr temp+6,cx	; save last iteration value here
	jo	end16bit		; bail out if too high
	js	end16bit		;  ...

	cmp	cx,word ptr lm+2	; while (xx+yy < lm)
	jae	end16bit		;  ...	

	mov	ax,di			; compute (y * x)
	imul	si			;  ...
	mov	cx,33			; ( * 2 / fudge)
	sub	cx,bitshift		;  ...
loop16bit3:
	shl	ax,1			;  ...
	rcl	dx,1			;  ...
	loop	loop16bit3		;  ...
	add	dx,word ptr b+2		; (2*y*x) / fudge + b
	mov	di,dx			; save as y

	add	bx,word ptr a+2		; (from above) (x*x - y*y)/fudge + a
	mov	si,bx			; save as x

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jb	short nonmax3		;  nope.  bypass periodicity check.
	mov	word ptr x+2,si		; save x for periodicity check
	mov	word ptr y+2,di		; save y for periodicity check
	call	checkperiod		; check for periodicity
nonmax3:

	dec	k			; while (k < maxit)
	jz	end16bit		;  we done.

	jmp	start16bit		; try, try again.

end16bit:				; we done.
	ret
code16bit	endp

; ******************** Function code32bit() *****************************
;
;	Perform the 32-bit logic required using 16-bit logic

;	mult32bit performs 32bit x 32bit signed multiplies, with
;	(temporary) 64-bit results, and the appropriate right-shift
;	( >> FUDGEFACTOR) before returning.   That's why it is so long.
;		arguments in SI:BX and DI:CX, results in DX:AX

mult32bit	proc near

	mov	ax,0
	mov	temp+4,ax		; first, clear out the (temporary)
	mov	temp+6,ax		;  result

	mov	sign,0			; clear out the sign flag
	cmp	si,0			; is SI:BX negative?
	jge	mults1			;  nope
	not	sign			;  yup.  flip signs
	not	bx			;   ...
	not	si			;   ...
	stc				;   ...
	adc	bx,ax			;   ...
	adc	si,ax			;   ...
mults1:	cmp	di,0			; is DI:CX negative?
	jge	mults2			;  nope
	not	sign			;  yup.  flip signs
	not	cx			;   ...
	not	di			;   ...
	stc				;   ...
	adc	cx,ax			;   ...
	adc	di,ax			;   ...
mults2:

	mov	ax,bx			; perform BX x CX
	mul	cx			;  ...
;	mov	temp,ax			;  results in lowest 32 bits
	mov	temp+2,dx		;  ...

	mov	ax,bx			; perform BX x DI
	mul	di			;  ...
	add	temp+2,ax		;  results in middle 32 bits
	adc	temp+4,dx		;  ...
	jnc	mults3			;  carry bit set?
	inc	word ptr temp+6		;  yup.  overflow
mults3:

	mov	ax,si			; perform SI * CX
	mul	cx			;  ...
	add	temp+2,ax		;  results in middle 32 bits
	adc	temp+4,dx		;  ...
	jnc	mults4			;  carry bit set?
	inc	word ptr temp+6		;  yup.  overflow
mults4:

	mov	ax,si			; perform SI * DI
	mul	di			;  ...
	add	temp+4,ax		; results in highest 32 bits
	adc	temp+6,dx		;  ...

	mov	ax,temp+4		; set up to fake SHR FUDGEFACTOR
	mov	dx,temp+6		;  by SHL 3 and using high-order dword
	mov	si,temp+2		;  ...
	mov	cx,32			; ( / fudge)
	sub	cx,bitshift		;  ...
multl1:
	shl	si,1			; shift one bit
	rcl	ax,1			;  ...
	rcl	dx,1			;  ...
	jc	multovfl		;  (oops.  overflow)
	loop	multl1			; shift another bit, if need be

	cmp	dx,0			; overflow on "negative" result
	jl	multovfl		;  ...

	cmp	sign,0			; should we negate the result?
	je	mults5			;  nope.
	not	ax			;  yup.  flip signs.
	not	dx			;   ...
	mov	bx,0			;   ...
	stc				;   ...
	adc	ax,bx			;   ...
	adc	dx,bx			;   ...
mults5:

multnoovfl:				; normal exit
	clc				;  no carry
	ret				; we done.

multovfl:				; overflow exit
	stc				;  set carry
	ret				; we done.

mult32bit	endp

code32bit	proc near

start32bit:			
	mov	bx,word ptr x		; set up to multiply x * x
	mov	si,word ptr x+2		;  ...
	mov	cx,bx			;  ...
	mov	di,si			;  ...
	call	mult32bit		; do it.
	jnc	ok32bit1		; no overflow.  continue.
	jmp	end32bit		; overflow.  we done.
ok32bit1:
	mov	word ptr xx,ax		; save this for a tad.
	mov	word ptr xx+2,dx	;  ...

	mov	bx,word ptr y		; set up to multiply y * y
	mov	si,word ptr y+2		;  ...
	mov	cx,bx			;  ...
	mov	di,si			;  ...
	call	mult32bit		; do it.
	jnc	ok32bit2		; no overflow.  continue.
	jmp	end32bit		; overflow.  we done.
ok32bit2:
	mov	word ptr yy,ax		; save this for a tad.
	mov	word ptr yy+2,dx	;  ...

	mov	ax,word ptr xx		; calculate (x*x + y*y)
	mov	dx,word ptr xx+2	;  ...
	add	ax,word ptr yy		;  ...
	adc	dx,word ptr yy+2	;  ...
	jo	short end32bit		; bail out if too high
	js	short end32bit		;  ...

	cmp	dx,word ptr lm+2	; while (lr < lm)
	jae	end32bit		;  ...

	mov	ax,word ptr xx		; calculate (x*x - y*y)
	mov	dx,word ptr xx+2	;  ...
	sub	ax,word ptr yy		;  ...
	sbb	dx,word ptr yy+2	;  ...
	mov	word ptr xxmyy,ax	;  ...
	mov	word ptr xxmyy+2,dx	;  ...

	mov	bx,word ptr x		; set up to multiply x * y
	mov	si,word ptr x+2		;  ...
	mov	cx,word ptr y		;  ...
	mov	di,word ptr y+2		;  ...
	call	mult32bit		; do it.
	shl	ax,1			; multiply by 2
	rcl	dx,1			;  ...
	add	ax,word ptr b		; add b
	adc	dx,word ptr b+2		;  ...
	mov	word ptr y,ax		; save the result as y
	mov	word ptr y+2,dx		;  ...

	mov	ax,word ptr xxmyy	; remember (x*x - y*y)
	mov	dx,word ptr xxmyy+2	;  ...
	add	ax,word ptr a		; add a
	adc	dx,word ptr a+2		;  ..
	mov	word ptr x,ax		; save the result as x
	mov	word ptr x+2,dx		;  ...

	mov	ax,oldcolor		; recall the old color
	cmp	ax,k			; check it against this iter
	jb	short nonmax2		;  nope.  bypass periodicity check.
	call	checkperiod		; check for periodicity
nonmax2:
	call	checkorbit		; check for orbiting

	dec	k			; while (k < maxit)
	jz	end32bit		;  we done.

	jmp	start32bit		; try, try again.

end32bit:				; we done.
	mov	word ptr temp+6,dx	; save last iteration value here
	ret				;  return.
code32bit	endp

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
	mov	dx,word ptr x+2		; load up x 
	and	dx,word ptr savedmask+2	;  truncate to appropriate precision
	cmp	dx,word ptr savedx+2	; does X match?
	jne	checkdone		;  nope.  forget it.
	mov	ax,word ptr x		; load up x 
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	cmp	ax,word ptr savedx	; does X match?
	jne	checkdone		;  nope.  forget it.
	mov	dx,word ptr y+2		; now test y
	and	dx,word ptr savedmask+2	;  truncate to appropriate precision
	cmp	dx,word ptr savedy+2	; does Y match?
	jne	checkdone		;  nope.  forget it.
	mov	ax,word ptr y		; load up y 
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	cmp	ax,word ptr savedy	; does Y match?
	jne	checkdone		;  nope.  forget it.
	mov	period,1		; note that we have found periodicity
	mov	k,1			; pretend maxit reached
checksave:
	mov	dx,word ptr x+2		; load up x 
	and	dx,word ptr savedmask+2	;  truncate to appropriate precision
	mov	word ptr savedx+2,dx	;  and save it
	mov	ax,word ptr x		; load up x 
	and	ax,word ptr savedmask	;  truncate to appropriate precision
	mov	word ptr savedx,ax	;  and save it
	mov	dx,word ptr y+2		; load up y 
	and	dx,word ptr savedmask+2	;  truncate to appropriate precision
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

checkorbit	proc near
	cmp	show_orbit,0		; was orbiting on?
	je	nonorbit1		;  nope.
	push	es			; save ES
	mov	ax,word ptr y		; save y
	mov	dx,word ptr y+2		;  ...
	push	dx			;  ...
	push	ax			;  ...
	mov	ax,word ptr x		; save x
	mov	dx,word ptr x+2		;  ...
	push	dx			;  ...
	push	ax			;  ...
	call	far ptr iplot_orbit	; display the orbit
	pop	ax			; clear out the parameters
	pop	ax			;  ...
	pop	ax			; clear out the parameters
	pop	ax			;  ...
	pop	es			; restore ES
nonorbit1:
	ret
checkorbit	endp

           end

