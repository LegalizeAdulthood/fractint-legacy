
;	FRACT386 (assembler portion)	Version 4.0		 By Bert Tyler
;					      Mouse support by Michael Kaufman

;	NOTE: this routine REQUIRES a 386.  It does NOT require (or use)
;	a floating point co-processor.

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
;	((Examine FUDGEFACTOR to see the factor I'm currently using))

;	The 386 has native 32-bit integer arithmetic, and (briefly) keeps
;	64-bit values around after 32-bit multiplies.   If the result is
;	divided down right away, you've got 64-bit arithmetic.   You just
;	have to ensure that the result after the divide is <= 32 bits long.

;	Dividing is slow -- but the 386 can perform 32-bit wide shifting
;	-- and can even perform 64-bit shifts with the following logic:
;			shdr	eax,edx,cl
;			shr	edx,cl
;	so we make sure that our "fudge factor" is a power of 2 and shift
;	it down that way.


;					Bert Tyler (btyler on BIX)


.MODEL	medium,c

.386

.DATA

FUDGEFACTOR	equ	29

; ************************ External variables *****************************

	extrn	julia:word		; == 0 if Mandelbrot set, else Julia
	extrn	numpasses:word		; == 0 if single-pass, 1 if 2-pass
	extrn	creal:dword, cimag:dword ; Julia Set Constant
	extrn	lx0:dword, ly0:dword	; arrays of (dword) increment values
	extrn	dotmode: word		; video mode:   1 = use the BIOS (yuck)
;							2 = use EGA/VGA style
;							3 = use MCGA style
	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	ixmin:word, ixmax:word	; for zoom/pan: x and y limits
	extrn	iymin:word, iymax:word	;  for the local zoom-box

; ************************ Internal variables *****************************

passnum		db	0		; pass number: 1 (blitz) or 0
dotadjust	dd	0		; adjustment factor (xdots)

dotcount	dd	0		; dot-counter:  0 to a*b
dotwrite	dw	0		; write-a-dot routine:  mode-specific
dotread		dw	0		; read-a-dot routine:   mode-specific
x		dw	0		; x-axis: 0 to (xdots-1)
y		dw	0		; y-axis: 0 to (ydots-1)

;					; Zoom-Box values (2K x 2K screens max)
boxcount	dw	0		; (previous) box pt counter: 0 if none.
boxpoints	dd	1028 dup(0)	; (previous) box data points
boxvalues	db	1028 dup(0)	; (previous) box color values

dacinit		db	0		; flag: 0 = DAC not saved
		db	3   dup(0)	; temp space for DAC rotates
dacbox		db	773 dup(0)	; saved DAC goes here
kbd_type	db	0		; type of keyboard

; ********************** Mouse Support Variables **************************

mouse		db	0		; == -1 if/when a mouse is found.
mousekey	db	0		; status of mouse keys (up, down)
mousemickeys	dw	0		; running mickey counter

; ******************* "Tweaked" VGA mode variables ************************

						; 704 x 528 mode
x704y528	db	704/8			; number of screen columns
		db	528/16			; number of screen rows
		db	 68h, 57h, 58h, 8Bh	; CRTC Registers
		db	 59h, 86h, 3EH,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 3Dh
		db	 19h, 8Bh, 0Fh, 2Ch
		db	  0h, 18h, 38h,0E3h
		db	0FFh
						; 720 x 540 mode
x720y540	db	720/8			; number of screen columns
		db	540/16			; number of screen rows
		db	 6Ah, 59h, 5Ah, 8Dh	; CRTC Registers
		db	 5Eh, 8Bh, 4AH,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 49h
		db	 24h, 86h, 1Bh, 2Dh
		db	  0h, 24h, 44h,0E3h
		db	0FFh
						; 736 x 552 mode
x736y552	db	736/8			; number of screen columns
		db	552/16			; number of screen rows
		db	 6Ch, 5Bh, 5Ch, 8Fh	; CRTC Registers
		db	 5Fh, 8Ch, 56H,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 55h
		db	 2Bh, 8Dh, 27h, 2Eh
		db	  0h, 30h, 50h,0E3h
		db	0FFh
						; 752 x 564 mode
x752y564	db	752/8			; number of screen columns
		db	564/16			; number of screen rows
		db	 6Eh, 5Dh, 5Eh, 91h	; CRTC Registers
		db	 62h, 8Fh, 62H,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 61h
		db	 37h, 89h, 33h, 2Fh
		db	  0h, 3Ch, 5Ch,0E3h
		db	0FFh
						; 768 x 576 mode
x768y576	db	768/8			; number of screen columns
		db	576/16			; number of screen rows
		db	 70h, 5Fh, 60h, 93h	; CRTC Registers
		db	 66h, 93h, 6EH,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 6Dh
		db	 43h, 85h, 3Fh, 30h
		db	  0h, 48h, 68h,0E3h
		db	0FFh
						; 784 x 588 mode
x784y588	db	784/8			; number of screen columns
		db	588/16			; number of screen rows
		db	 72h, 61h, 62h, 95h	; CRTC Registers
		db	 69h, 96h, 7AH,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 79h
		db	 4Fh, 81h, 4Bh, 31h
		db	  0h, 54h, 74h,0E3h
		db	0FFh
						; 800 x 600 mode
x800y600	db	800/8			; number of screen columns
		db	600/16			; number of screen rows
		db	 74h, 63h, 64h, 97h	; CRTC Registers
		db	 68h, 95h, 86H,0F0h
		db	  0h, 60h,  0h,  0h
		db	  0h,  0h,  2h, 85h
		db	 5Bh, 8Dh, 57h, 32h
		db	  0h, 60h, 80h,0E3h
		db	0FFh

tweaks		dw	offset x704y528		; tweak table
		dw	offset x704y528
		dw	offset x720y540
		dw	offset x736y552
		dw	offset x752y564
		dw	offset x768y576
		dw	offset x784y588
		dw	offset x800y600

.CODE

; ***************** Function calcdots() **********************************

calcdots proc
	local	fluff1:dword		; stack fluff for safety's sake
	local	lm:dword		; bail-out value: 4 << fudgefactor
	local	lx:dword		; local value of lx0 (from above array)
	local	ly:dword		; local value of ly0 (from above array)
	local	fluff2:dword		; stack fluff for safety's sake
	local	k:word			; local counter: 1 to maxit
	local	kbdcount:word		; keyboard counter: nnnnn to 0
	local	kbdflag:word		; keyboard hit flag: 0 if no, 1 if yes
	local	fluff3:dword		; stack fluff for safety's sake

	push	es			; save the original ES value
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	kbdcount,ax		; initialize keyboard counter (to -1)
	mov	kbdflag,0		; initialize keyboard int flag: nope

					; prepare special video-mode speedups
	cmp	dotmode,3		; MCGA mode?
	je	short mcgamode		; yup.
	cmp	dotmode,2		; EGA/VGA mode?
	je	short vgamode		; yup.
dullnormalmode:
	mov	ax,offset normalwrite	; set up the BIOS write-a-dot routine
	mov	bx,offset normalread	; set up the BIOS read-a-dot  routine
	jmp	short videomode		; return to common code
mcgamode:
	mov	ax,offset mcgawrite	; set up MCGA write-a-dot routine
	mov	bx,offset mcgaread	; set up the BIOS read-a-dot  routine
	jmp	short videomode		; return to common code
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up the BIOS read-a-dot  routine
	jmp	short videomode		; return to common code
videomode:
	mov	dotwrite,ax		; save the results
	mov	dotread,bx		;  ...

	mov	eax,4			; initialize lm
	shl	eax,FUDGEFACTOR		;  ( == 4 << fudgefactor )
	mov	lm,eax			;  ...

	mov	ax,numpasses		; detect 1 or 2-pass mode
	mov	passnum,al		; initialize pass counter

passloop:
	mov	eax,0			; initialize dot counter
	dec	eax			;  (to -1: it gets incremented later)
	mov	dotcount,eax		;  ...
	mov	eax,0			; save dot adjustment factor
	mov	ax,xdots		;   == xdots
	mov	dotadjust,eax		;   ...
	mov	y,0			; initialize outer loop

yloop:					; for (y = 0; y < ydots; y++)
	mov	x,0			; initialize inner loop

xloop:					; for (x = 0; x < xdots; x++)
	mov	bx,y			; pull ly0 value out of the array
	shl	bx,2			; convert to double-word pointer
	mov	eax,ly0[bx]		;  here it is!
	mov	ly,eax			; save it for later

	mov	bx,x			; pull lx0 value out of the array
	shl	bx,2			; convert to double-word pointer
	mov	eax,lx0[bx]		;  here it is!
	mov	lx,eax			; save it for later

	mov	esi,0			; (esi == lx0) lx0 = 0
	mov	edi,esi			; (edi == ly0) ly0 = 0
	mov	ebx,esi			; (ebx == ...<calculated on the fly>)
					;      (lx0*lx0 - ly0*ly0) / fudge = 0

	cmp	julia,0			; julia or mandelbrot set?
	je	short doeither		; Mandelbrot set:  initialization done.

dojulia:				; Julia Set initialization
					; "fudge" Mandelbrot start-up values
	mov	ebx,lx			; get a running start on real part
	sub	ebx,creal		;  sub, later add Creal .. and get x0!

	mov	edi,ly			; get a running start on imag'ry part
	sub	edi,cimag		;  (subtract, later add Cimag)
	mov	esi,1			;  (multiply, later div by [2/fudge])
	shl	esi,FUDGEFACTOR-1	;   ... and we'll get y0!!!

	mov	edx,creal		; reset real, imaginary parts of const
	mov	lx,edx			;  Creal
	mov	edx,cimag		;  ...
	mov	ly,edx			;  Cimaginary

doeither:				; common Mandelbrot, Julia set code
	mov	ax,maxit			; setup k = maxit
	mov	k,ax			;  (decrementing to 0 is faster)

	inc	dotcount		; increment the dot-counter

	cmp	numpasses,0		; multiple-pass mode?
	jz	short singlepass	;  nope.  proceed.
	cmp	passnum,0		; second pass?
	jne	short singlepass	;  nope.  proceed
	test	y,1			; odd dot?
	jnz	short singlepass	;  yup.  proceed.
	test	x,1			; odd dot?
	jz	loopchecks		;  nope. skip it.
singlepass:

	dec	kbdcount		; decrement the keyboard counter
	jns	short kloop		;  skip keyboard test if still positive
	mov	kbdcount,5000		; else, stuff an appropriate count val
	mov	ah,1			; check the keyboard
	int	16h			; has it been hit?
 	jnz	short keyhit		; yes.  handle it
 	call	msemvd			; was the mouse moved
 	jnc	short kloop		; nope.  proceed
keyhit:	mov	kbdflag,1		; yup.  reset kbd-hit flag: yes.
	jmp	wedone			; so, bail out!

;	This is the main processing loop.  Here, every T-state counts...

kloop:					; for (k = 0; k <= maxit; k++)

	mov	eax,edi			; compute (ly * lx)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR-1	;  ( * 2 / fudge)
	add	eax,ly			;  (above) + ly0
	mov	edi,eax			;  save this as ly

;	(from the previous iteration)	; compute (lx*lx - ly*ly) / fudge
	add	ebx,lx			;       + lx0
	mov	esi,ebx			; save this as lx

	mov	eax,esi			; compute (lx * lx)
	imul	esi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals)
	jne	short kloopend		; bail out if too high
	mov	ecx,eax			; save this for below
	mov	eax,edi			; compute (ly * ly)
	imul	edi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals) 
	jne	short kloopend		; bail out if too high
	mov	ebx,ecx			; compute (lx*lx - ly*ly) / fudge
	sub	ebx,eax			;  for the next iteration
	add	eax,ecx			; compute (lx*lx + ly*ly) / fudge
	jo	short kloopend		; bail out if too high
	js	short kloopend		;  ...

	dec	k			; while (k < maxit) (dec to 0 is faster)
	jz	short kloopend		; while (k < maxit) ...

	cmp	eax,lm			; while ( lr <= lm)
	jbe	kloop			;  ...

kloopend:

	mov	ax,maxit		; compute color
	sub	ax,k			;  (first, re-compute "k")
	sub	kbdcount,ax		; adjust the keyboard count
	cmp	ax,0			; convert any "outlier" region 
	jne	short coloradjust1	;  (where abs(x) > 2 or abs(y) > 2)
	mov	ax,1			;   to look like we ran through
coloradjust1:				;    at least one loop.
	cmp	ax,maxit		; did we max out on iterations?
	jne	short coloradjust2	;  nope.
	mov	ax,1			; reset max-out color to border color
coloradjust2:				;  (it just looks better, somehow)
	mov	dx,0			;  convert to a 32-bit value
	div	colors			;  ...
	mov	al,dl			; result in al

	call	dotwrite		; invoke the appropriate write-a-dot

	cmp	passnum,0		; final pass?
	je	short loopchecks	; yup.  proceed.

	inc	dotcount		; bump up the dot counter
	call	dotwrite		; write the dot again
	mov	edx,dotadjust		; adjust the dot counter
	add	dotcount,edx		;  ...
	dec	dotcount		; bump down the dot counter
	call	dotwrite		; write the dot again
	inc	dotcount		; bump up the dot counter
	call	dotwrite		; write the dot again
	mov	edx,dotadjust		; adjust the dot counter
	sub	dotcount,edx		;  ...
	inc	x			; note extra dots have been written

loopchecks:
	inc	x			; check for end of xloop
	mov	ax,xdots		;  ...
	cmp	x,ax			;  ...
	jb	xloop			; more to go

	cmp	passnum,0		; last pass?
	je	short lastpass		;  yup.  proceed.
	inc	y			; adjust y-value
	mov	edx,dotadjust		; adjust the dot counter
	add	dotcount,edx		;  ...
lastpass:

	inc	y			; check for end of yloop
	mov	ax,ydots		;  ...
	cmp	y,ax			;  ...
	jb	yloop			; more to go

	dec	passnum			; decrement the pass counter
	jns	passloop		; more to go

wedone:					; restore everything and return.
	mov	ax,dotwrite		; check: were we in EGA/VGA mode?
	cmp	ax,offset vgawrite	;  ...
	jne	short wereallydone	; nope.  no adjustments
	mov	dx,03ceh		; graphics controller address
	mov	ax,0ff08h		; restore the default bit mask
	out	dx,ax			; ...
	mov	ax,0003h		; restore the function select
	out	dx,ax			;  ...
	mov	ax,0001h		; restore the enable set/reset
	out	dx,ax			;  ...
wereallydone:
	pop	es			; restore the original ES value
	mov	ax,kbdflag		; return the keyboard-interrupt flag
	mov	boxcount,0		; indicate no boxes drawn yet.
	ret				; and return.

calcdots endp

; **************** internal Read/Write-a-dot routines ***********************

normalwrite	proc	near		; generic write-a-dot routine
	push	ax			; save the AX register for a tad
	mov	eax,dotcount		; determine the row and cloumn
	mov	edx,dotcount		; need it in a DX:AX pair
	ror	edx,16			;  now we have it
	div	xdots			; perform the divide
	mov	cx,dx			; this is the x-coord
	mov	dx,ax			; this is the y-coord
	pop	ax			; now retrieve the AX register
	mov	ah,12			; write the dot (al == color)
	mov	bx,0			; this page
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
normalwrite	endp

normalread	proc	near		; generic read-a-dot routine
	mov	eax,dotcount		; determine the row and cloumn
	mov	edx,dotcount		; need it in a DX:AX pair
	ror	edx,16			;  now we have it
	div	xdots			; perform the divide
	mov	cx,dx			; this is the x-coord
	mov	dx,ax			; this is the y-coord
	mov	ah,13			; read the dot (al == color)
	mov	bx,0			; this page
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
normalread	endp

mcgawrite	proc	near		; MCGA 320*200, 246 colors
	mov	ebx,dotcount		; load up an offset register
	mov	es:[ebx],al		; write the dot
	ret				; we done.
mcgawrite	endp

mcgaread	proc	near		; MCGA 320*200, 246 colors
	mov	ebx,dotcount		; load up an offset register
	mov	al,es:[ebx]		; retrieve the previous value
	ret				; we done.
mcgaread	endp

vgawrite	proc	near		; EGA/VGA write mode 0
	push	ax			; save AX for a tad
	mov	bh,al			; save the color value for a bit
	mov	esi,dotcount		; compute the buffer offset
	mov	cx,si			; and bit mask
	shr	esi,3			; (buffer offset == dotcount / 8)
	and	cx,7			; bit-mask shift calculation
	xor	cl,7			;  ...
	mov	dx,03ceh		; graphics controller address
	mov	ax,0108h		; set up controller bit mask register
	shl	ah,cl			;  ...
	out	dx,ax			;  ...
	mov	ah,bh			; set set/reset registers
	mov	al,0			;  ...
	out	dx,ax			;  ...
	mov	ax,0f01h		; enable set/reset registers
	out	dx,ax			;  ...
	or	es:[esi],al		; update all bit planes
	pop	ax			; restore the original AX
	ret				; we done.
vgawrite	endp

vgaread	proc	near			; EGA/VGA read mode 0
	mov	esi,dotcount		; compute the buffer offset
	mov	cx,si			; and bit mask
	shr	esi,3			; (buffer offset == dotcount / 8)
	and	cx,7			; bit-mask shift calculation
	xor	cl,7			;  ...
	mov	ch,01h			; bit mask to shift
	shl	ch,cl			;  ...
	mov	bx,0			; initialize bits-read value (none)
	mov	dx,03ceh		; graphics controller address
	mov	ax,0304h		; set up controller address register
vgareadloop:
	out	dx,ax			; do it
	mov	bh,es:[esi]		; retrieve the old value
	and	bh,ch			; mask one bit
	neg	bh			; set bit 7 correctly
	rol	bx,1			; rotate the bit into bl
	dec	ah			; go for another bit?
	jge	vgareadloop		;  sure, why not.
	mov	al,bl			; returned pixel value
	ret				; we done.
vgaread	endp

; ******************** Function drawbox() *******************************

drawbox	proc
	local	fluff7:dword		; stack fluff for safety's sake
	local	xmax:dword		; double-word copies of x and y
	local	xmin:dword		;  zoom-box minimums and maximums
	local	ymax:dword		;  ...
	local	ymin:dword		;  ...
	local	dxdots:dword		; double-word versions of total
	local	dydots:dword		;  number of x, y dots on screen
	local	xstep:dword		; box drawing: x-step increments
	local	ystep:dword		; box drawing: y-step increments
	local	fluff8:dword		; stack fluff for safety's sake
	local	boxcolor:byte		; box drawing: box color
	local	fluff9:dword		; stack fluff for safety's sake

	mov	eax,0			; move word min, max values to dwords
	mov	ax,ixmin		;  ...
	mov	xmin,eax		;  ...
	mov	ax,ixmax		;  ...
	mov	xmax,eax		;  ...
	mov	ax,iymin		;  ...
	mov	ymin,eax		;  ...
	mov	ax,iymax		;  ...
	mov	ymax,eax		;  ...
	mov	ax,xdots		; move xdots, ydots values to dwords
	mov	dxdots,eax		;  ...
	mov	ax,ydots		;  ...
	mov	dydots,eax		;  ...

	mov	eax,1			; default x-step:  every pixel
	mov	xstep,eax		;  ...
	mov	eax,dxdots		; default y-step:  every row
	mov	ystep,eax		;  ...
	mov	eax,xmax		; just how big is this zoom-box?
	sub	eax,xmin		;  this many dots,...
	shl	eax,3			; an eighth of the screen or less?
	cmp	eax,dxdots		;  ...
	jb	short solidbox		;  yup.  keep the box solid.
	mov	xstep,2			; nope.  make the box every other pixel
	mov	edx,ystep		;  ...
	add	ystep,edx		;  ...
solidbox:
	shr	eax,1			; a quarter of the screen or less?
	cmp	eax,dxdots		;  ...
	jb	short solidbox2		;  yup.  keep the box (semi) solid.
	mov	xstep,4			; nope.  make the box every 4th pixel
	add	ystep,edx		;  ...
solidbox2:

	mov	ax,colors		; define the zoom-box color
	dec	al			;  ...
	cmp	al,15			; do we have 16 colors?
	jbe	short whitebox		;  nope.  use what we can get.
	mov	al,15			; force a white zoom box
whitebox:
	mov	boxcolor,al		; save the box color

	push	es			; save the original ES value
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	bx,boxcount		; load up a counter: # points to clear
	dec	bx			; switch to an offset value
	js	short calcnewbox	;  oops. no old box to clear.
eraseoldbox:
	shl	bx,2			; switch to double-word counter
	mov	edx,boxpoints[bx]	; get the (previous) point location
	mov	dotcount,edx		; save it for the subroutine call
	shr	bx,2			; switch back to character counter
	mov	al,boxvalues[bx]	; get the (previous) color
	push	bx			; save the counter
	call	dotwrite		; adjust the dot.
	pop	bx			; restore the counter
	dec	bx			; are we done yet?
	jns	eraseoldbox		;  nope.  try again.
calcnewbox:
	mov	eax,ymin		; calculate top-left point
	mul	dxdots			;  == yoffset * dots/xline
	add	eax,xmin		;     + xoffset
	mov	boxpoints,eax		; save it.
	sub	eax,xmin		; now calculate top-right point
	add	eax,xmax		;  ...
	mov	boxpoints+4,eax		; save it.
	mov	eax,ymax		; calculate bottom-left point
	mul	dxdots			;  == yoffset * dots/xline
	add	eax,xmin		;     + xoffset
	mov	boxpoints+8,eax		; save it.
	sub	eax,xmin		; now calculate bot-right point
	add	eax,xmax		;  ...
	mov	boxpoints+12,eax	; save it.
	mov	boxcount,4		; set flag: new box drawn.
	mov	bx,boxcount		; get set to draw lines
	shl	bx,2			; switch to double-word pointers
starttop:
	mov	eax,boxpoints		; now, draw the top line.
	mov	edx,xstep		; draw every 'step'th dot
topline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+4		; gone past the end-of-line?
	jae	short startbottom	;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	topline			; and try again.
startbottom:
	mov	eax,boxpoints+8		; now, draw the bottom line.
bottomline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+12	; gone past the end-of-line?
	jae	short startleft		;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	bottomline		; and try again.
startleft:
	mov	eax,boxpoints		; now, draw the left line.
	mov	edx,ystep		; draw every 'step'th dot
leftline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+8		; gone past the end-of-line?
	jae	short startright	;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	leftline		; and try again.
startright:
	mov	eax,boxpoints+4		; now, draw the right line.
rightline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+12	; gone past the end-of-line?
	jae	short endlines		;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	rightline		; and try again.
endlines:
	mov	bx,boxcount		; load up a counter: # points to draw
	dec	bx			; switch to an offset
readnewbox:
	shl	bx,2			; switch to double-word counter
	mov	edx,boxpoints[bx]	; get the (new) point location
	mov	dotcount,edx		; save it for the subroutine call
	shr	bx,2			; switch back to character counter
	push	bx			; save the counter
	call	dotread			; read the (previous) dot value
	pop	bx			; restore the counter
	mov	boxvalues[bx],al	; get the (previous) color
	dec	bx			; are we done yet?
	jns	readnewbox		;  nope.  try again.
	mov	bx,boxcount		; load up a counter: # points to draw
	dec	bx			; switch to an offset
drawnewbox:
	shl	bx,2			; switch to double-word counter
	mov	edx,boxpoints[bx]	; get the (new) point location
	mov	dotcount,edx		; save it for the subroutine call
	shr	bx,2			; switch back to character counter
	push	bx			; save the counter
	mov	al,boxcolor		; set the (new) box color
	call	dotwrite		; adjust the dot.
	pop	bx			; restore the counter
	dec	bx			; are we done yet?
	jns	drawnewbox		;  nope.  try again.

	mov	ax,dotwrite		; check: were we in EGA/VGA mode?
	cmp	ax,offset vgawrite	;  ...
	jne	short dotsdone		; nope.  no adjustments
	mov	dx,03ceh		; graphics controller address
	mov	ax,0ff08h		; restore the default bit mask
	out	dx,ax			; ...
	mov	ax,0003h		; restore the function select
	out	dx,ax			;  ...
	mov	ax,0001h		; restore the enable set/reset
	out	dx,ax			;  ...
dotsdone:
	pop	es			; restore ES register
	ret				; we done.

drawbox	endp

; **************** Function setvideomode(ax, bx, cx, dx) ****************

;	This function sets the (alphanumeric or graphic) video mode
;	of the monitor.   Called with the proper values of AX thru DX.
;	No returned values, as there is no particular standard to
;	adhere to in this case.

;	(SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a 
;	genuine VGA or register compatable adapter and program the registers
;	directly using the coded value in DX)

setvideomode	proc	argax:word, argbx:word, argcx:word, argdx:word
	push	ax			; save registers
	push	bx			;  ...
	push	cx			;  ...
	push	dx			;  ...
	push	bp			;  ...

	mov	ax,argax		; load up for the interrupt call
	mov	bx,argbx		;  ...
	mov	cx,argcx		;  ...
	mov	dx,argdx		;  ...

	cmp	ax,0			; TWEAK?:  look for AX==BX==CX==0
	jne	short setvideobios	;  ...
	cmp	bx,0			;  ...
	jne	short setvideobios	;  ...
	cmp	cx,0			;  ...
	je	short setvideoregs	;  ...

setvideobios:
	int	10h			; do it via the BIOS.
	jmp	short setvideoreturn	;  and return.

setvideoregs:				; assume genuine VGA and program regs

	mov	si,dx			; get the video table offset
	shl	si,1			;  ...
	mov	si,word ptr tweaks[si]	;  ...

	mov	ax,0012h		; invoke video mode 12h
	int	10h

	mov	ax,1124h		; load ROM 8*16 characters
	mov	bx,0
	mov	dh,0
	mov	dl,byte ptr [si+1]	; number of rows on the screen
	int	10h

	push	es			; save ES for a tad
	mov	ax,40h			; Video BIOS DATA area
	mov	es,ax			;  ...

	mov	ah,0
	mov	al,byte ptr [si]	; number of columns on the screen
	mov	word ptr es:[4ah],ax
	mul	byte ptr [si+1]		; number of characters on the screen
	shl	ax,1			; (attributes, also)
	mov	word ptr es:[4ch],ax

	mov	dx,word ptr es:[63h]	; say, where's the 6845?
	add	dx,6			; locate the status register
vrdly1:	in	al,dx			; loop until vertical retrace is off
	test	al,8			;   ...
	jnz	vrdly1			;   ...
vrdly2:	in	al,dx			; now loop until it's on!
	test	al,8			;   ...
	jz	vrdly2			;   ...

	mov	dx,03c4h		; Sequencer Synchronous reset
	mov	ax,0100h		; set sequencer reset
	out	dx,ax
	mov	dx,03c2h		; Update Misc Output Reg
	mov	al,0E7h
	out	dx,al
	mov	dx,03c4h		; Sequencer Synchronous reset
	mov	ax,0300h		; clear sequencer reset
	out	dx,ax

	mov	dx,word ptr es:[63h]	; say, where's the 6845?
	add	si,2			; point SI to the CRTC registers table
	mov	al,11h			; deprotect registers 0-7
	mov	ah,byte ptr [si+11h]
	and	ah,7fh
	out	dx,ax

	mov	cx,18h			; update this many registers
	mov	bx,00			; starting with this one.
crtcloop:
	mov	al,bl			; update this register
	mov	ah,byte ptr [bx+si]	; to this
	out	dx,ax
	inc	bx			; ready for the next register
	loop	crtcloop		; (if there is a next register)

	pop	es			; restore ES

setvideoreturn:
	pop	bp			; restore registers
	pop	dx			;  ...
	pop	cx			;  ...
	pop	bx			;  ...
	pop	ax			;  ...
	mov	dacinit,0		; indicate new DAC
	ret
setvideomode	endp

; ****************** Function palette(direction) ********************

;	(just for fun) Rotate the MCGA/VGA DAC in the (plus or minus)
;	"direction"

palette	proc	direction:word
	push	ax			; save a few registers
	push	bx			;  ...
	push	cx			;  ...
	push	dx			;  ...
	push	es			;  ...
	push	si			;  ...
	push	di			;  ...
	push	ds			;  ...
	pop	es			;  ...
	cld				; set the direction
	cmp	dacinit,0		; have we ever saved the DAC?
	jne	short gotDAC		;  yup.  proceed.
	mov	ax,1017h		; get the old DAC values
	mov	bx,0			;  ...
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	int	10h			; do it.
	mov	dacinit,1		; indicate it's done.
gotDAC:
	cmp	direction,1		; rotate upwards?
	jne	short downDAC		;  nope.  downwards
	mov	si,offset dacbox+0	; set up the rotate
	mov	di,offset dacbox+768	;  ...
	mov	cx,3			;  ...
	rep	movsb			; rotate it.
	mov	si,offset dacbox+3	; set up the rotate
	mov	di,offset dacbox+0	;  ...
	mov	cx,768			;  ...
	rep	movsb			; rotate it.
	jmp	short newDAC		; set the new DAC
downDAC:
	std				; set the direction
	mov	si,offset dacbox+767	; set up the rotate
	mov	di,offset dacbox-1	;  ...
	mov	cx,3			;  ...
	rep	movsb			; rotate it.
	mov	si,offset dacbox+764	; set up the rotate
	mov	di,offset dacbox+767	;  ...
	mov	cx,768			;  ...
	rep	movsb			; rotate it.
newDAC:
	cld				; set the direction
	mov	ax,1012h		; set the new DAC
	mov	bx,0			;  ...
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	int	10h			; do it.
	pop	di			; restore a few registers
	pop	si			;  ...
	pop	es			;  ...
	pop	dx			;  ...
	pop	cx			;  ...
	pop	bx			;  ...
	pop	ax			;  ...
	ret
palette	endp

; ****************** Function getakey() *****************************

;	This function gets a key from either a "normal" or an enhanced
;	keyboard.   Returns either the vanilla ASCII code for regular
;	keys, or 1000+(the scan code) for special keys (like F1, etc)
;	Use of this routine permits the Control-Up/Down arrow keys on
;	enhanced keyboards.
;
;	The concept for this routine was "borrowed" from the MSKermit
;	SCANCHEK utility

getakey	proc
 	call	chkmse				; see if the mouse was used
 	jc	short getakey4			; ax holds the phoney key
	mov	ah,kbd_type			; get the keyboard type
	or	ah,1				; check if a key is ready
	int	16h				; now check a key
	jz	getakey				; so check the mouse again
	mov	ah,kbd_type			; get the keyboard type
	int	16h				; now get a key
	cmp	al,0e0h				; check: Enhanced Keyboard key?
	jne	short getakey1			; nope.  proceed
	cmp	ah,0				; part 2 of Enhanced Key check
	je	short getakey1			; failed.  normal key.
	mov	al,0				; Turn enhanced key "normal"
	jmp	short getakey2			; jump to common code
getakey1:			
	cmp	ah,0e0h				; check again:  Enhanced Key?
	jne	short getakey2			;  nope.  proceed.
	mov	ah,al				; Turn Enhanced key "normal"
	mov	al,0				;  ...
getakey2:
	cmp	al,0				; Function Key?
	jne	short getakey3			;  nope.  proceed.
	mov	al,ah				; klooge into ASCII Key
	mov	ah,0				; clobber the scan code
	add	ax,1000				;  + 1000
	jmp	short getakey4			; go to common return
getakey3:
	mov	ah,0				; clobber the scan code
getakey4:
	ret
getakey	endp


; ********************* Mouse Support Code ******************************
;
; 		Contributed by Mike Kaufman
;	(and then hacked up beyond all recall by me (sorry) - Bert)
;
; ***********************************************************************

; ****************** Function initasmvars() *****************************
initasmvars proc
				       ; first see if a mouse is installed 
         xor	ax,ax                  ; function for mouse check
         int	33h                    ; call mouse driver
	 mov	mouse,al	       ; al holds info about mouse
	
	 			       ; now get the information about the kbd
	 push	es		       ; save ES for a tad
	 mov	ax,40h		       ; reload ES with BIOS data seg
	 mov	es,ax		       ;  ...
	 mov	ah,es:96h	       ; get the keyboard byte
	 pop	es		       ; restore ES
	 and	ah,10h		       ; isolate the Enhanced KBD bit
	 mov	kbd_type,ah	       ; and save it

	 ret                           ; return to caller
initasmvars endp


; This function checks if a mouse button has been pushed or if the mouse moved
chkmse	proc	near
	cmp	mouse,-1	       ; is mouse installed
	jz	short chkit	       ; yes, so do stuff
	clc			       ; clear the carry flag
	ret			       ; and return
	 
chkit:  push	bx                     ; save registers
	push	dx
	push	cx
	push	bp
mousecheck:
	mov	ax,3                    ; function for mouse status
	int	33h                     ; call mouse driver
	and	bx,7                    ; we only care about these bits
	mov	mousekey,bl		; save the results
	cmp	bx,0		        ; any buttons pressed?
	jz	short notpressed        ; no so check for mouse movement
	cmp	bx,1			; left-hand button (only) down?
	je	short mouseleft		;  yup.  deal with it.
	cmp	bx,2			; right-hand button (only) down?
	je	short mouseright	;  yup.  deal with it.

mousemiddle:				; multiple or middle button down
	mov	mousekey,4		; force it to be middle-button down
	mov	ax,13			; pretend the ENTER key was hit
	jmp	short pressed		; and return.
	
mouseleft:
	mov	ax,0bh		        ; distance moved function
	int	33h		        ; see how far the mouse has moved
	mov	ax, 1073		; indicate page up
	add	mousemickeys,dx		; compute a running movement sum
	cmp	mousemickeys,-10	; which way'd we go?
	jle	short pressed		;  Up.  Proceed.
	mov	ax, 1081		; indicate page down
	cmp	mousemickeys,10		; which way'd we go?
	jge	short pressed		;  Up.  Proceed.
	jmp	mousecheck		; else check again.
	
pressed:
	mov	mousemickeys,0		; clear out the mickey counter
	stc				; indicate something happened
	jmp	short exitpress 	; and exit
	 
; The purpose of this bit of code is to eliminate the effect of small mouse
; movements.  What I mean is that, the direction to move is the direction that
; the mouse has moved the most

mouseright:

notpressed:			       ; no button pressed, but maybe the 
				       ; mouse was moved so check that out
	mov	ax,0bh		       ; distance moved function
	int	33h		       ; see how far the mouse has moved
	mov	ax,dx		       ; now see who moved farther
	mov	bx,cx		       ; move to ax,bx so we can play
	cmp	ax,0		       ; find the abs(ax)
	jge	short chk_bx	       ; already postive
	not	ax		       ; ax is negative, so negate
chk_bx: cmp	bx,0		       ; find  the abs(bx)
	jge	short chk_grt	       ; already postive
	not	bx		       ; bx is negative, so negate
chk_grt:
	cmp	ax,bx		       ; see which one is greater
	jl	short nocol	       ; bx is greater so check the rows
	
	cmp	dx,0		       ; did the col change
	jz	short nthng	       ; no then nothing changed (ie cx=dx=0)
	jg	short ardown	       ; mouse moved down
	mov	ax,1072		       ; indicate an up arrow
	cmp	mousekey,0	       ; were any mouse keys hit?
	jne	pressed		       ;  yup.  proceed.
	mov	ax,1141		       ; indicate a control-arrow
	jmp	short pressed
ardown:
	mov	ax,1080		       ; indicate a down arrow
	cmp	mousekey,0	       ; were any mouse keys hit?
	jne	pressed		       ;  yup.  proceed.
	mov	ax,1145		       ; indicate a control-arrow
	jmp	short pressed
nocol:	cmp	cx,0		       ; did the row change up or down
	jg	short arright	       ; mouse moved to the right
	mov	ax,1075		       ; indicate a left arrow
	cmp	mousekey,0	       ; were any mouse keys hit?
	jne	pressed		       ;  yup.  proceed.
	mov	ax,1115		       ; indicate a control-arrow
	jmp	short pressed
arright:
	mov	ax,1077		       ;indeicate a right arrow
	cmp	mousekey,0	       ; were any mouse keys hit?
	jne	pressed		       ;  yup.  proceed.
	mov	ax,1116		       ; indicate a control-arrow
	jmp	short pressed
nthng:  clc                            ; indicate that nothing changed
exitpress:
	pop	bp		       ; restore registers
	pop	cx
	pop	dx
	pop	bx		       ; restore value 
	ret			       ; return to caller
chkmse	endp


; Check if a button was hit on the mouse
msemvd	proc  near
	cmp	mouse,0		       ; is mouse installed
	jnz	short mchkit	       ; yes, so do stuff
	clc			       ; clear the carry flag
	ret			       ; and return

mchkit:	push	bx                     ; save registers
	push	dx
	push	cx
	push	ax
mousecheck2:
	mov	ax,3                   ; function to check mouse status
	int	33h                    ; call mouse driver
	and	bx,7                   ; we only care about these bits
	cmp	mousekey,4	       ; ugly klooge for dual-key presses:
	jne	short mouseklooge       ; wait until they ALL clear out
	cmp	bx,0		       ; any keys still down?
	jne	mousecheck2	       ;  yup.  try again.
	mov	mousekey,0	       ; else clear out the klooge flag
mouseklooge:
	cmp	bx,0		       ; any buttons pressed?
	jnz	short mpress	       ; yes so exit with a yes
	clc			       ; indicate that nothing changed
	jmp	short mexit	       ; and exit
mpress:	stc			       ; indicate a change
mexit:	pop	ax
	pop	cx
	pop	dx
	pop	bx		       ; restore value 
	ret			       ; return to caller
msemvd	endp


; ****************** Function cputype() *****************************

; This program was downloaded from PC Tech Journal's Hotline service
; (it was originally in their November, 1987 issue), and is used here
; with their knowledge and permission.

; Function cputype(), for real OR protected mode.  Returns (in AX)
; the value 86, 186, 286 or 386; negative if protected mode.


           .286P                  ;enable protected-mode instr.

.code

cputype    proc
           push    bp
           push    sp             ;86/186 will push SP-2,
           pop     ax             ;286/386 will push SP
           cmp     ax,sp
           jz      not86          ;if equal, SP was pushed
           mov     ax,186         ;is it 86 or 186?
           mov     cl,32          ;  186 uses count mod 32 = 0;
           shl     ax,cl          ;  86 shifts 32 so ax = 0
           jnz     exit           ;non-zero: no shift, so 186
           mov     ax,86          ;zero: shifted out all bits
           jmp     short exit

not86:     pushf                  ;Test 16 or 32 operand size:
           mov     ax,sp          ;  pushed 2 or 4 bytes of flags?
           popf                   ;  restore SP
           inc     ax             ;  restore AX by 2 bytes
           inc     ax
           cmp     ax,sp          ;  did pushf change SP by 2?
           jnz     is32bit        ;  if not, then 4 bytes of flags

is16bit:   sub     sp,6           ;Is it 286 or 386 in 16-bit mode?
           mov     bp,sp          ;allocate stack space for GDT ptr
           sgdt    fword ptr[bp]  ;(use PWORD PTR for MASM5)
           add     sp,4           ;discard 2 words of GDT pointer
           pop     ax             ;get third word
           inc     ah             ;286 stores -1, 386 0 or 1
           jnz     is386
is286:     mov     ax,286         ;set return value
           jmp     short testprot

is32bit:   db      66H            ;16-bit override in 32-bit mode
is386:     mov     ax,386

testprot:  smsw    cx             ;Protected?  Machine status -> CX
           ror     cx,1           ;protection bit -> carry flag
           jnc     exit           ;real mode if no carry
           neg     ax             ;protected:  return neg value
exit:      pop     bp
           ret
cputype    endp

           end
