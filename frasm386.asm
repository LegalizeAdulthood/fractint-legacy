
;	FRACT386 (assembler portion)	Version 2.1	By Bert Tyler

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

.CODE

FUDGEFACTOR	equ	29

; ************************ External variables *****************************

	extrn	lx0:dword, ly0:dword	; arrays of (dword) increment values
	extrn	dotmode: word		; video mode:   1 = use the BIOS (yuck)
;							2 = use EGA/VGA style
;							3 = use MCGA style
	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	ixmin:word, ixmax:word	; for zoom/pan: x and y limits
	extrn	iymin:word, iymax:word	;  for the local zoom-box

dotcount	dd	0		; dot-counter:  0 to a*b
dotwrite	dw	0		; write-a-dot routine:  mode-specific
dotread		dw	0		; read-a-dot routine:   mode-specific
x		dw	0		; x-axis: 0 to (xdots-1)
y		dw	0		; y-axis: 0 to (ydots-1)

;					; Zoom-Box values (2K x 2K screens max)
boxcount	dw	0		; (previous) box pt counter: 0 if none.
boxpoints	dd	1028 dup(0)	; (previous) box data points
boxvalues	db	1028 dup(0)	; (previous) box color values

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

	mov	eax,0			; initialize dot counter
	dec	eax			;  (to -1: it gets incremented later)
	mov	dotcount,eax		;  ...

	mov	kbdcount,ax		; initialize keyboard counter (to -1)
	mov	kbdflag,0		; initialize keyboard int flag: nope


					; prepare special video-mode speedups
	cmp	dotmode,3		; MCGA mode?
	je	mcgamode		; yup.
	cmp	dotmode,2		; EGA/VGA mode?
	je	vgamode			; yup.
dullnormalmode:
	mov	ax,offset normalwrite	; set up the BIOS write-a-dot routine
	mov	bx,offset normalread	; set up the BIOS read-a-dot  routine
	jmp	videomode		; return to common code
mcgamode:
	mov	ax,offset mcgawrite	; set up MCGA write-a-dot routine
	mov	bx,offset mcgaread	; set up the BIOS read-a-dot  routine
	jmp	videomode		; return to common code
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up the BIOS read-a-dot  routine
	jmp	videomode		; return to common code
videomode:
	mov	dotwrite,ax		; save the results
	mov	dotread,bx		;  ...

	mov	eax,4			; initialize lm
	shl	eax,FUDGEFACTOR		;  ( == 4 << fudgefactor )
	mov	lm,eax			;  ...

	mov	y,0			; initialize outer loop

yloop:					; for (y = 0; y < ydots; y++)
	mov	bx,y			; pull lq value out of the array
	shl	bx,2			; convert to double-word pointer
	mov	eax,ly0[bx]		;  here it is!
	mov	ly,eax			; save it for later

	mov	x,0			; initialize inner loop

xloop:					; for (x = 0; x < xdots; x++)
	mov	bx,x			; pull lp value out of the array
	shl	bx,2			; convert to double-word pointer
	mov	eax,lx0[bx]		;  here it is!
	mov	lx,eax			; save it for later

	mov	esi,0			; (esi == lx0) lx0 = 0
	mov	edi,esi			; (edi == ly0) ly0 = 0
	mov	ebx,esi			; (ebx == ...<calculated on the fly>)
					;      (lx0*lx0 - ly0*ly0) / fudge = 0
	mov	ax,maxit			; setup k = maxit
	mov	k,ax			;  (decrementing to 0 is faster)

	inc	dotcount		; increment the dot-counter

	dec	kbdcount		; decrement the keyboard counter
	jns	maxittest		;  skip keyboard test if still positive
	mov	kbdcount,5000		; else, stuff an appropriate count val
	mov	ah,1			; check the keyboard
	int	16h			; has it been hit?
	jz	maxittest		; nope.  proceed
	mov	kbdflag,1		; yup.  reset kbd-hit flag: yes.
	jmp	wedone			; so, bail out!

maxittest:				; timing check: avoid the main
	cmp	maxit,1			;  processing loop if maxit <= 1
	jg	kloop			;  ...
	mov	maxit,1			; avoid divides by zero
	mov	k,0			; pretend we have done 1 loop
	jmp	kloopend		; and bail out

;	This is the main processing loop.  Here, every T-state counts...

kloop:					; for (k = 0; k <= maxit; k++)

	mov	eax,esi			; compute (lx * ly)
	imul	edi			;  ...
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
	jne	kloopend		; bail out if too high
	mov	ecx,eax			; save this for below
	mov	eax,edi			; compute (ly * ly)
	imul	edi			;  ...
	shrd	eax,edx,FUDGEFACTOR	; ( / fudge)
	shr	edx,FUDGEFACTOR-1	; (complete 64-bit shift and check
	cmp	edx,0			;  for any overflow/sign reversals) 
	jne	kloopend		; bail out if too high
	mov	ebx,ecx			; compute (lx*lx - ly*ly) / fudge
	sub	ebx,eax			;  for the next iteration
	add	eax,ecx			; compute (lx*lx + ly*ly) / fudge
	jo	kloopend		; bail out if too high
	js	kloopend		;  ...

	dec	k			; while (k < maxit) (dec to 0 is faster)
	jz	kloopend		; while (k < maxit) ...

	cmp	eax,lm			; while ( lr <= lm)
	jbe	kloop			;  ...

kloopend:

	mov	ax,maxit		; compute color
	sub	ax,k			;  (first, re-compute "k")
	sub	kbdcount,ax		; adjust the keyboard count
	cmp	ax,0			; convert any "outlier" region 
	jne	coloradjust1		;  (where abs(x) > 2 or abs(y) > 2)
	mov	ax,1			;   to look like we ran through
coloradjust1:				;    at least one loop.
	cmp	ax,maxit		; did we max out on iterations?
	jne	coloradjust2		;  nope.
	mov	ax,1			; reset max-out color to border color
coloradjust2:				;  (it just looks better, somehow)
	mov	dx,0			;  convert to a 32-bit value
	div	colors			;  ...
	mov	al,dl			; result in al

	call	dotwrite		; invoke the appropriate write-a-dot

loopchecks:
	inc	x			; check for end of xloop
	mov	ax,xdots		;  ...
	cmp	x,ax			;  ...
	jb	xloop			; more to go

	inc	y			; check for end of yloop
	mov	ax,ydots		;  ...
	cmp	y,ax			;  ...
	jb	yloop			; more to go

wedone:					; restore everything and return.
	mov	ax,dotwrite		; check: were we in EGA/VGA mode?
	cmp	ax,offset vgawrite	;  ...
	jne	wereallydone		; nope.  no adjustments
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
	mov	es:[bx],al		; write the dot
	ret				; we done.
mcgawrite	endp

mcgaread	proc	near		; MCGA 320*200, 246 colors
	mov	ebx,dotcount		; load up an offset register
	mov	al,es:[bx]		; retrieve the previous value
	ret				; we done.
mcgaread	endp

vgawrite	proc	near		; EGA/VGA write mode 0
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
	or	es:[si],al		; update all bit planes
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
	mov	bh,es:[si]		; retrieve the old value
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
	jb	solidbox		;  yup.  keep the box solid.
	mov	xstep,2			; nope.  make the box every other pixel
	mov	edx,ystep		;  ...
	add	ystep,edx		;  ...
solidbox:
	shr	eax,1			; a quarter of the screen or less?
	cmp	eax,dxdots		;  ...
	jb	solidbox2		;  yup.  keep the box (semi) solid.
	mov	xstep,4			; nope.  make the box every 4th pixel
	add	ystep,edx		;  ...
solidbox2:

	mov	ax,colors		; define the zoom-box color
	dec	al			;  ...
	cmp	al,15			; do we have 16 colors?
	jbe	whitebox		;  nope.  use what we can get.
	mov	al,15			; force a white zoom box
whitebox:
	mov	boxcolor,al		; save the box color

	push	es			; save the original ES value
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	bx,boxcount		; load up a counter: # points to clear
	dec	bx			; switch to an offset value
	js	calcnewbox		;  oops. no old box to clear.
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
	jae	startbottom		;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	topline			; and try again.
startbottom:
	mov	eax,boxpoints+8		; now, draw the bottom line.
bottomline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+12	; gone past the end-of-line?
	jae	startleft		;  yup.  bail out.
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
	jae	startright		;  yup.  bail out.
	mov	boxpoints[bx],eax	; save this point.
	add	bx,4			; bump up the pointer offsets
	inc	boxcount		; and counters
	jmp	leftline		; and try again.
startright:
	mov	eax,boxpoints+4		; now, draw the right line.
rightline:
	add	eax,edx			; calculate the next dot address
	cmp	eax,boxpoints+12	; gone past the end-of-line?
	jae	endlines		;  yup.  bail out.
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
	jne	dotsdone		; nope.  no adjustments
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

setvideomode	proc	argax:word, argbx:word
	push	ax			; save registers
	push	bx			;  ...
	push	cx			;  ...
	push	dx			;  ...
	push	bp			;  ...
	mov	ax,argax		; load up for the interrupt call
	mov	bx,argbx		;  ...
	int	10h			; do it.
	pop	bp			; restore registers
	pop	dx			;  ...
	pop	cx			;  ...
	pop	bx			;  ...
	pop	ax			;  ...
	ret
setvideomode	endp


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
	push	es				; save ES for a tad
	mov	ax,40h				; reload ES with BIOS data seg
	mov	es,ax				;  ...
	mov	ah,es:96h			; get the keyboard byte
	pop	es				; restore ES
	and	ah,10h				; isolate the Enhanced KBD bit
	int	16h				; now get a key
	cmp	al,0e0h				; check: Enhanced Keyboard key?
	jne	getakey1			; nope.  proceed
	cmp	ah,0				; part 2 of Enhanced Key check
	je	getakey1			; failed.  normal key.
	mov	al,0				; Turn enhanced key "normal"
	jmp	getakey2			; jump to common code
getakey1:			
	cmp	ah,0e0h				; check again:  Enhanced Key?
	jne	getakey2			;  nope.  proceed.
	mov	ah,al				; Turn Enhanced key "normal"
	mov	al,0				;  ...
getakey2:
	cmp	al,0				; Function Key?
	jne	getakey3			;  nope.  proceed.
	mov	al,ah				; klooge into ASCII Key
	mov	ah,0				; clobber the scan code
	add	ax,1000				;  + 1000
	jmp	getakey4			; go to common return
getakey3:
	mov	ah,0				; clobber the scan code
getakey4:
	ret
getakey	endp


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
           jmp     exit

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
           jmp     testprot

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
