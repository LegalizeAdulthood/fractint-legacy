;	Generic assembler routines that have very little at all
;	to do with fractals.
;
;	(NOTE:  The video routines have been moved over to VIDEO.ASM)
;
; ---- Overall Support
;
;	initasmvars()
;
; ---- Quick-copy to/from Extraseg support
;
;	toextra()
;	fromextra()
;	cmpextra()
;
; ---- far memory allocation support
;
;	farmemalloc()
;	farmemfree()
;	erasesegment()
;
; ---- General Turbo-C (artificial) support
;
;	disable()
;	enable()
;
; ---- 32-bit Multiply/Divide Routines (includes 16-bit emulation)
;
;	multiply()
;	divide()
;
; ---- Keyboard, audio (and, hidden inside them, Mouse) support
;
;	keypressed()
;	getakey()
;	buzzer()
;	delay()
;	tone()
;
; ---- Expanded Memory Support
;
;	emmquery()
;	emmgetfree()
;	emmallocate()
;	emmdeallocate()
;	emmgetpage()
;	emmclearpage()
;
; ---- CPU, FPU Detectors
;
;	cputype()
;	fputype()



;			 required for compatibility if Turbo ASM
IFDEF ??version
	MASM51
	QUIRKS
ENDIF

	.MODEL  medium,c

	.8086

        ; these must NOT be in any segment!!
        ; this get's rid of TURBO-C fixup errors

        extrn   help:far		; help code (in help.c)

.DATA

; ************************ External variables *****************************

	extrn	sound:word		; if 0, supress sounds
	extrn	debugflag:word		; for debugging purposes only
	extrn	helpmode:word		; help mode (AUTHORS is special)


; ************************ Public variables *****************************

public		cpu			; used by 'calcmand'
public		fpu			; will be used by somebody someday
public		lookatmouse		; used by 'calcfrac'
public		_dataseg		; used by TARGA, other Turbo Code

public		extraseg		; extra 64K segment, if any

public		overflow		; Mul, Div overflow flag: 0 means none

;		arrays declared here, used elsewhere
;		arrays not used simultaneously are deliberately overlapped

public		lx0,ly0				; used by FRACTINT, CALCFRAC
public		prefix, suffix, dstack, decoderline	; Used by the Decoder
public		strlocn, teststring		; used by the Encoder
public		boxx, boxy, boxvalues		; zoom-box arrays
public		olddacbox			; temporary DAC saves
public		diskline			; Used by the Diskvid rtns
public		rlebuf				; Used ty the TARGA En/Decoder
public		paldata, stbuff			; 8514A arrays, (FR8514A.ASM)

; ************************* "Shared" array areas **************************

lx0		dd	0		; 8K X-pixel value array
prefix		dw	4096 dup(0)	; 8K Decoder array

ly0		dd	0		; 8K y-pixel value array
suffix		dw	2048 dup(0)	; 4K Decoder array
dstack		dw	2048 dup(0)	; 4K Decoder array

strlocn		dw	0		; 10K Encoder array
olddacbox	db	0		; (256*3) temporary dacbox values
boxx		dw	2048 dup(0)	; (previous) box data points - x axis
decoderline	db	0		; 2K Decoder array
boxy		dw	2048 dup(0)	; (previous) box data points - y axis
boxvalues	db	2048 dup(0)	; 2K of (previous) box color values

diskline	db	0		; 2K Diskvideo array
rlebuf		db	0		; 256 char TARGA encoder array
paldata		db	1024 dup (0)	; 8514A palette (used in FR8514A.ASM)
stbuff		db	415 dup (0)	; 8514A state   (used in FR8514A.ASM)
		db	609 dup(0)	; (fluff this area out to 2K)

teststring	db	100  dup(0)	; 100 byte Encoder array

; ************************ Internal variables *****************************

cpu		dw	0		; cpu type: 86, 186, 286, or 386
fpu		dw	0		; fpu type: 0, 87, 287, 387
_dataseg	dw	0		; our "near" data segment

overflow	dw	0		; overflow flag

kbd_type	db	0		; type of keyboard
keybuffer	dw	0		; real small keyboard buffer

delayloop	dw	32		; delay loop value
delaycount	dw	0		; number of delay "loops" per ms.

;	"buzzer()" codes:  strings of two-word pairs 
;		(frequency in cycles/sec, delay in milliseconds)
;		frequency == 0 means no sound
;		delay     == 0 means end-of-tune

buzzer0		dw	1047,100	; "normal" completion
		dw	1109,100
		dw	1175,100
		dw	0,0
buzzer1		dw	2093,100	; "interrupted" completion
		dw	1976,100
		dw	1857,100
		dw	0,0
buzzer2		dw	40,500		; "error" condition (razzberry)
		dw	0,0

extraseg	dw	0		; extra 64K segment (allocated by init)

; ********************** Mouse Support Variables **************************

mouse		db	0		; == -1 if/when a mouse is found.
mousekey	db	0		; status of mouse keys (up, down)
mousemickeys	dw	0		; running mickey counter
lookatmouse	dw	0		; if 0, ignore non-button mouse mvment


.CODE

; *************** Function toextra(tooffset,fromaddr, fromcount) *********

toextra	proc	uses es di si, tooffset:word, fromaddr:word, fromcount:word
	cmp	extraseg,0		; IS there extra memory?
	je	tobad			;  nope.  too bad.
	cld				; move forward
	mov	ax,extraseg		; load ES == extra segment
	mov	es,ax			;  ..
	mov	di,tooffset		; load to here
	mov	si,fromaddr		; load from here
	mov	cx,fromcount		; this many bytes
	rep	movsb			; do it.
tobad:
	ret				; we done.
toextra	endp


; *************** Function fromextra(fromoffset, toaddr, tocount) *********

fromextra proc	uses es di si, fromoffset:word, toaddr:word, tocount:word
	push	ds			; save DS for a tad
	pop	es			; restore it to ES
	cmp	extraseg,0		; IS there extra memory?
	je	frombad			;  nope.  too bad.
	cld				; move forward
	mov	si,fromoffset		; load from here
	mov	di,toaddr		; load to here
	mov	cx,tocount		; this many bytes
	mov	ax,extraseg		; load DS == extra segment
	mov	ds,ax			;  ..
	rep	movsb			; do it.
frombad:
	push	es			; save ES again.
	pop	ds			; restore DS 
	ret				; we done.
fromextra endp


; *************** Function cmpextra(cmpoffset,cmpaddr, cmpcount) *********

cmpextra proc	uses es di si, cmpoffset:word, cmpaddr:word, cmpcount:word
	cmp	extraseg,0		; IS there extra memory?
	je	cmpbad			;  nope.  too bad.
	cld				; move forward
	mov	ax,extraseg		; load ES == extra segment
	mov	es,ax			;  ..
	mov	di,cmpoffset		; load to here
	mov	si,cmpaddr		; load from here
	mov	cx,cmpcount		; this many bytes
	rep	cmpsb			; do it.
	jnz	cmpbad			; failed.
	mov	ax,0			; 0 == true
	jmp	cmpend
cmpbad:
	mov	ax,1			; 1 == false
cmpend:
	ret				; we done.
cmpextra	endp


; =======================================================
;
;	32-bit integer multiply routine with an 'n'-bit shift.
;	Overflow condition returns 0x7fffh with overflow = 1;
;
;	long x, y, z, multiply();
;	int n;
;
;	z = multiply(x,y,n)
;
;	requires the presence of an external variable, 'cpu'.
;		'cpu' == 386 if a 386 is present.

.MODEL	medium,c

.8086

.DATA

temp	dw	5 dup(0)		; temporary 64-bit result goes here
sign	db	0			; sign flag goes here

.CODE

multiply	proc	uses di si es, x:dword, y:dword, n:word

	cmp	cpu,386			; go-fast time?
	jne	slowmultiply		; no.  yawn...

.386					; 386-specific code starts here

	mov	eax,x			; load X into EAX
	imul	y			; do the multiply
	mov	cx,n			; set up the shift
	cmp	cx,32			; ugly klooge:  check for 32-bit shift
	jb	short fastm1		;  < 32 bits:  no problem
	mov	eax,edx			;  >= 32 bits:  manual shift
	mov	edx,0			;  ...
	sub	cx,32			;  ...
fastm1:	push	cx			; save to counter
	shrd	eax,edx,cl		; shift down 'n' bits
	pop	cx			; restore the counter
	sar	edx,cl			; shift down the top bits
	cmp	eax,0			; verify the resulting sign
	jge	fastm3			;  positive
	inc	edx			; negative - increment edx
fastm3:	cmp	edx,0			; if EDX isn't zero
	jne	overm1			;  we overflowed
	push	eax			; save the 64-bit result
	pop	ax			; low-order  16 bits
	pop	dx			; high-order 16 bits
	jmp	multiplyreturn		; back to common code

.8086					; 386-specific code ends here

slowmultiply:				; (sigh)  time to do it the hard way...

	mov	ax,0
	mov	temp+4,ax		; first, zero out the (temporary)
	mov	temp+6,ax		;  result
	mov	temp+8,ax

	les	bx,x			; move X to SI:BX
	mov	si,es			;  ...
	les	cx,y			; move Y to DI:CX
	mov	di,es			;  ...

	mov	sign,0			; clear out the sign flag
	cmp	si,0			; is X negative?
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
	mov	temp,ax			;  results in lowest 32 bits
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

	mov	cx,n			; set up for the shift loop
	cmp	cx,24			; shifting by three bytes or more?
	jl	multc1			;  nope.  check for something else
	sub	cx,24			; quick-shift 24 bits
	mov	ax,temp+3		; load up the registers
	mov	dx,temp+5		;  ...
	mov	si,temp+7		;  ...
	mov	bx,0			;  ...
	jmp	short multc4		; branch to common code
multc1:	cmp	cx,16			; shifting by two bytes or more?
	jl	multc2			;  nope.  check for something else
	sub	cx,16			; quick-shift 16 bits
	mov	ax,temp+2		; load up the registers
	mov	dx,temp+4		;  ...
	mov	si,temp+6		;  ...
	mov	bx,0			;  ...
	jmp	short multc4		; branch to common code
multc2:	cmp	cx,8			; shifting by one byte or more?
	jl	multc3			;  nope.  check for something else
	sub	cx,8			; quick-shift 8 bits
	mov	ax,temp+1		; load up the registers
	mov	dx,temp+3		;  ...
	mov	si,temp+5		;  ...
	mov	bx,temp+7		;  ...
	jmp	short multc4		; branch to common code
multc3:	mov	ax,temp			; load up the regs
	mov	dx,temp+2		;  ...
	mov	si,temp+4		;  ...
	mov	bx,temp+6		;  ...
multc4:	cmp	cx,0			; done shifting?
	je	multc5			;  yup.  bail out

multloop:
	shr	bx,1			; shift down 1 bit, cascading
	rcr	si,1			;  ...
	rcr	dx,1			;  ...
	rcr	ax,1			;  ...
	loop	multloop		; try the next bit, if any
multc5:
	cmp	si,0			; overflow time?
	jne	overm1			; yup.  Bail out.
	cmp	bx,0			; overflow time?
	jne	overm1			; yup.  Bail out.
	cmp	dx,0			; overflow time?
	jl	overm1			; yup.  Bail out.

	cmp	sign,0			; should we negate the result?
	je	mults5			;  nope.
	not	ax			;  yup.  flip signs.
	not	dx			;   ...
	mov	bx,0			;   ...
	stc				;   ...
	adc	ax,bx			;   ...
	adc	dx,bx			;   ...
mults5:
	jmp	multiplyreturn

overm1:
	mov	ax,0ffffh		; overflow value
	mov	dx,07fffh		; overflow value
	mov	overflow,1		; flag overflow

multiplyreturn:				; that's all, folks!
	ret
multiply	endp


; =======================================================
;
;	32-bit integer divide routine with an 'n'-bit shift.
;	Overflow condition returns 0x7fffh with overflow = 1;
;
;	long x, y, z, divide();
;	int n;
;
;	z = divide(x,y,n);	/* z = x / y; */
;
;	requires the presence of an external variable, 'cpu'.
;		'cpu' == 386 if a 386 is present.


.8086

divide		proc	uses di si es, x:dword, y:dword, n:word

	cmp	cpu,386			; go-fast time?
	jne	slowdivide		; no.  yawn...

.386					; 386-specific code starts here

	mov	edx,x			; load X into EDX (shifts to EDX:EAX)
	mov	ebx,y			; load Y into EBX

	mov	sign,0			; clear out the sign flag
	cmp	edx,0			; is X negative?
	jge	short divides1		;  nope
	not	sign			;  yup.  flip signs
	neg	edx			;   ...
divides1:
	cmp	ebx,0			; is Y negative?
	jge	short divides2		;  nope
	not	sign			;  yup.  flip signs
	neg	ebx			;   ...
divides2:

	mov	eax,0			; clear out the low-order bits
	mov	cx,32			; set up the shift
	sub	cx,n			; (for large shift counts - faster)
fastd1:	cmp	cx,0			; done shifting?
	je	fastd2			; yup.
	shr	edx,1			; shift one bit
	rcr	eax,1			;  ...
	loop	fastd1			; and try again
fastd2:
	cmp	edx,ebx			; umm, will the divide blow out?
	jae	overd1			;  yup.  better skip it.
	div	ebx			; do the divide
	cmp	eax,0			; did the sign flip?
	jl	overd1			;  then we overflowed
	cmp	sign,0			; is the sign reversed?
	je	short divides3		;  nope
	neg	eax			; flip the sign
divides3:
	push	eax			; save the 64-bit result
	pop	ax			; low-order  16 bits
	pop	dx			; high-order 16 bits
	jmp	dividereturn		; back to common code

.8086					; 386-specific code ends here

slowdivide:				; (sigh)  time to do it the hard way...

	les	ax,x			; move X to DX:AX
	mov	dx,es			;  ...

	mov	sign,0			; clear out the sign flag
	cmp	dx,0			; is X negative?
	jge	divides4		;  nope
	not	sign			;  yup.  flip signs
	not	ax			;   ...
	not	dx			;   ...
	stc				;   ...
	adc	ax,0			;   ...
	adc	dx,0			;   ...
divides4:

	mov	cx,32			; get ready to shift the bits
	sub	cx,n			; (shift down rather than up)
	mov	byte ptr temp+4,cl	;  ...

	mov	cx,0			;  clear out low bits of DX:AX:CX:BX
	mov	bx,0			;  ...

	cmp	byte ptr temp+4,16	; >= 16 bits to shift?
	jl	dividex0		;  nope
	mov	bx,cx			;  yup.  Take a short-cut
	mov	cx,ax			;   ...
	mov	ax,dx			;   ...
	mov	dx,0			;   ...
	sub	byte ptr temp+4,16	;   ...
dividex0:
	cmp	byte ptr temp+4,8	; >= 8 bits to shift?
	jl	dividex1		;  nope
	mov	bl,bh			;  yup.  Take a short-cut
	mov	bh,cl			;   ...
	mov	cl,ch			;   ...
	mov	ch,al			;   ...
	mov	al,ah			;   ...
	mov	ah,dl			;   ...
	mov	dl,dh			;   ...
	mov	dh,0			;   ...
	sub	byte ptr temp+4,8	;   ...
dividex1:
	cmp	byte ptr temp+4,0	; are we done yet?
	je	dividex2		;  yup
	shr	dx,1			; shift all 64 bits
	rcr	ax,1			;  ...
	rcr	cx,1			;  ...
	rcr	bx,1			;  ...
	dec	byte ptr temp+4		; decrement the shift counter
	jmp	short dividex1		;  and try again
dividex2:

	les	di,y			; move Y to SI:DI
	mov	si,es			;  ...

	cmp	si,0			; is Y negative?
	jge	divides5		;  nope
	not	sign			;  yup.  flip signs
	not	di			;   ...
	not	si			;   ...
	stc				;   ...
	adc	di,0			;   ...
	adc	si,0			;   ...
divides5:

	mov	byte ptr temp+4,33	; main loop counter 
	mov	temp,0			; results in temp
	mov	word ptr temp+2,0	;  ...

dividel1:
	shl	temp,1			; shift the result up 1
	rcl	word ptr temp+2,1	;  ...
	cmp	dx,si			; is DX:AX >= Y?
	jb	dividel3		;  nope
	ja	dividel2		;  yup
	cmp	ax,di			;  maybe
	jb	dividel3		;  nope
dividel2:
	cmp	byte ptr temp+4,32	; overflow city?
	jge	overd1			;  yup.
	sub	ax,di			; subtract Y
	sbb	dx,si			;  ...
	inc	temp			; add 1 to the result
	adc	word ptr temp+2,0	;  ...
dividel3:
	shl	bx,1			; shift all 64 bits
	rcl	cx,1			;  ...
	rcl	ax,1			;  ...
	rcl	dx,1			;  ...
	dec	byte ptr temp+4		; time to quit?
	jnz	dividel1		;  nope.  try again.

	mov	ax,temp			; copy the result to DX:AX
	mov	dx,word ptr temp+2	;  ...
	cmp	sign,0			; should we negate the result?
	je	divides6		;  nope.
	not	ax			;  yup.  flip signs.
	not	dx			;   ...
	mov	bx,0			;   ...
	stc				;   ...
	adc	ax,0			;   ...
	adc	dx,0			;   ...
divides6:
	jmp	short dividereturn

overd1:
	mov	ax,0ffffh		; overflow value
	mov	dx,07fffh		; overflow value
	mov	overflow,1		; flag overflow

dividereturn:				; that's all, folks!
	ret
divide		endp


; ****************** Function getakey() *****************************
; **************** Function keypressed() ****************************

;	'getakey()' gets a key from either a "normal" or an enhanced
;	keyboard.   Returns either the vanilla ASCII code for regular
;	keys, or 1000+(the scan code) for special keys (like F1, etc)
;	Use of this routine permits the Control-Up/Down arrow keys on
;	enhanced keyboards.
;
;	The concept for this routine was "borrowed" from the MSKermit
;	SCANCHEK utility
;
;	'keypressed()' returns a zero if no keypress is outstanding,
;	and the value that 'getakey()' will return if one is.  Note
;	that you must still call 'getakey()' to flush the character.
;	As a sidebar function, calls 'help()' if appropriate.
;	Think of 'keypressed()' as a super-'kbhit()'.

keypressed	proc
keypressed1:
	cmp	keybuffer,0			; is a keypress stacked up?
	jne	keypressed3			;  yup. use it.
	mov	ah,kbd_type			; get the keyboard type
	or	ah,1				; check if a key is ready
	int	16h				; has a key been hit?
 	jnz	keypressed2			; yes.  handle it
 	call	msemvd				; key pressed on the mouse?
 	jc	keypressed2			; yes.  handle it.
	cmp	lookatmouse,0			; look for mouse movement?
	je	keypressed5			; nope.  return: no action.
 	call	chkmse				; was the mouse moved
 	jnc	keypressed5			; nope.  return: no action.
keypressed2:
	call	far ptr getakey			; get the keypress code
	mov	keybuffer,ax			; and save the result.
keypressed3:
	mov	ax,keybuffer			; return the keypress code.
	cmp	helpmode,1			; is this HELPAUTHORS mode?
	je	keypressed5			;  yup.  forget help.
	cmp	ax,'h'				; help called?
	je	keypressed4			;  ...
	cmp	ax,'H'				; help called?
	je	keypressed4			;  ...
	cmp	ax,'?'				; help called?
	je	keypressed4			;  ...
	cmp	ax,'/'				; help called?
	je	keypressed4			;  ...
	jmp	keypressed5			; no help asked for.
keypressed4:
	mov	keybuffer,0			; say no key hit
	push	es				; save a few registers
	mov	ax,2				; ask for general help
	push	ax				;  ...
	call	far ptr help			; help!
	mov	keybuffer,ax			; save the result
	pop	ax				; returned value
	pop	es				; restore some registers
keypressed5:
	mov	ax,keybuffer			; return keypress, if any
	ret
keypressed	endp

getakey	proc
; TARGA 31 May 89 j mclain
; when using '.model size,c' with TASM,
;   TASM 'gifts' us with automatic insertions of:
;  proc xxxx
;   -> push bp
;   -> mov bp,sp
;    ...
;   ->pop bp
;  ret
;
; we corrected a situation here where we had a constant 'jz getakey'
; can we say 'stack overflow'
;
getakey0:
	mov	ax,keybuffer			; keypress may be here
	mov	keybuffer,0			; if it was, clear it
	cmp	ax,0				; is a keypress outstanding?
	jne	getakey4			;  if so, we're done!
 	call	msemvd				; key pressed on the mouse?
 	jc	getakey5			; yes.  handle it.
	cmp	lookatmouse,0			; look for mouse movement?
	je	getakey6			; nope.  return: no action.
getakey5:
 	call	chkmse				; see if the mouse was used
 	jc	short getakey4			; ax holds the phoney key
getakey6:
	mov	ah,kbd_type			; get the keyboard type
	or	ah,1				; check if a key is ready
	int	16h				; now check a key
	jz	getakey0			; so check the mouse again
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

; ****************** Function buzzer(int buzzertype) *******************
;
;	Sound a tone based on the value of the parameter
;
;	0 = normal completion of task
;	1 = interrupted task
;	2 = error contition

; ***********************************************************************

buzzer	proc	uses si, buzzertype:word
	cmp	sound,0			; is the sound supressed?
	je	buzzerreturn		;  yup.  bail out.
	mov	si, offset buzzer0	; normal completion frequency
	cmp	buzzertype,0		; normal completion?
	je	buzzerdoit		; do it
	mov	si,offset buzzer1	; interrupted task frequency
	cmp	buzzertype,1		; interrupted task?
	je	buzzerdoit		; do it
	mov	si,offset buzzer2	; error condition frequency
buzzerdoit:
	mov	ax,0[si]		; get the (next) frequency
	mov	bx,2[si]		; get the (next) delay
	add	si,4			; get ready for the next tone
	cmp	bx,0			; are we done?
	je	buzzerreturn		;  yup.
	push	bx			; put delay time on the stack
	push	ax			; put tone value on the stack
	call	far ptr tone		; do it
	pop	ax			; restore stack
	pop	bx			; restore stack
	jmp	short buzzerdoit	; get the next tone
buzzerreturn:
	ret				; we done
buzzer	endp

; ***************** Function delay(int delaytime) ************************
;
;	performs a delay loop for 'delaytime' milliseconds
;
; ************************************************************************

delayamillisecond	proc	near	; internal delay-a-millisecond code
	mov	bx,delaycount		; set up to burn another millisecond
delayamill1:
	mov	cx,delayloop		; start up the counter
delayamill2:				;
	loop	delayamill2		; burn up some time
	dec	bx			; have we burned up a millisecond?
	jnz	delayamill1		;  nope.  try again.
	ret				; we done
delayamillisecond	endp

delay	proc	uses es, delaytime:word	; delay loop (arg in milliseconds)
	mov	ax,delaytime		; get the number of milliseconds
	cmp	ax,0			; any delay time at all?
	je	delayreturn		;  nope.
delayloop1:
	call	delayamillisecond	; burn up a millisecond of time
	dec	ax			; have we burned up enough m-seconds?
	jnz	delayloop1		;  nope.  try again.
delayreturn:
	ret				; we done.
delay	endp

; ************** Function tone(int frequency,int delaytime) **************
;
;	buzzes the speaker with this frequency for this amount of time
;
; ************************************************************************

tone	proc	uses es, tonefrequency:word, tonedelay:word
	mov	al,0b6h			; latch to channel 2
	out	43h,al			;  ...
	cmp	tonefrequency,12h	; was there a frequency?
	jbe	tonebypass		;  nope.  delay only
	mov	bx,tonefrequency	; get the frequency value
	mov	ax,0			; ugly klooge: convert this to the
	mov	dx,12h			; divisor the 8253 wants to see
	div	bx			;  ...
	out	42h,al			; send the low value
	mov	al,ah			; then the high value
	out	42h,al			;  ...
	in	al,61h			; get the current 8255 bits
	or	al,3			; turn bits 0 and 1 on
	out	61h,al			;  ...
tonebypass:
	mov	ax,tonedelay		; get the delay value
	push	ax			; set the parameter
	call	far ptr delay		; and force a delay
	pop	ax			; restore the parameter

	in	al,61h			; get the current 8255 bits
	and	al,11111100b		; turn bits 0 and 1 off
	out	61h,al

	ret				; we done
tone	endp


; ********************* Mouse Support Code ******************************
;
; 		Contributed by Mike Kaufman
;	(and then hacked up beyond all recall by me (sorry) - Bert)
;
; ***********************************************************************

; ****************** Function initasmvars() *****************************

initasmvars	proc	uses es si di

	 cmp	cpu,0			; have we been called yet:
	 je	initasmvarsgo		;  nope.  proceed.
	 jmp	initreturn		;  yup.  no need to be here.

initasmvarsgo:
	mov	ax,ds			; save the data segment
	mov	_dataseg,ax		;  for the C code

	mov	overflow,0		; indicate no overflows so far

	mov	dx,1			; ask for 96K of far space
	mov	ax,8000h		;  ...
	push	dx			;  ...
	push	ax			;  ...
	call	far ptr farmemalloc	; use the assembler routine to do it
	pop	ax			; restore the stack
	pop	ax			;  ...
	mov	extraseg,dx		; save the results here.

	push	es			; save ES for a tad
	mov	ax,0			; reset ES to BIOS data area
	mov	es,ax			;  ...
	mov	dx,es:46ch		; obtain the current timer value
delaystartuploop:
	cmp	dx,es:46ch		; has the timer value changed?
	je	delaystartuploop	;  nope.  check again.
	mov	dx,es:46ch		; obtain the current timer value again
	mov	ax,0			; clear the delay counter
	mov	delaycount,55		; 55 millisecs = 1/18.2 secs
delaytestloop:
	call	delayamillisecond	; burn up a (fake) millisecond
	inc	ax			; indicate another loop has passed
	cmp	dx,es:46ch		; has the timer value changed?
	je	delaytestloop		; nope.  burn up some more time.
	mov	delaycount,ax		; save the results here
	pop	es			; restore ES again

				       ; first see if a mouse is installed 

	push	es			; (no, first check to ensure that
	mov	ax,0			; int 33h doesn't point to 0:0)
	mov	es,ax			; ...
	mov	ax,es:0cch		; ...
	pop	es			; ...
	cmp	ax,0			; does int 33h have a non-zero value?
	je	noint33			;  nope.  then there's no mouse.

         xor	ax,ax                  ; function for mouse check
         int	33h                    ; call mouse driver
noint33:
	 mov	mouse,al	       ; al holds info about mouse
	
	 			       ; now get the information about the kbd
	 push	es		       ; save ES for a tad
	 mov	ax,40h		       ; reload ES with BIOS data seg
	 mov	es,ax		       ;  ...
	 mov	ah,es:96h	       ; get the keyboard byte
	 pop	es		       ; restore ES
	 and	ah,10h		       ; isolate the Enhanced KBD bit
	 mov	kbd_type,ah	       ; and save it

	call	far ptr cputype		; what kind of CPU do we have here?
	cmp	ax,0			; protected mode of some sort?
	jge	positive		;  nope.  proceed.
	neg	ax			;  yup.  flip the sign.
positive:
	mov	cpu,ax			; save the cpu type.
itsa386:
	cmp	debugflag,8088		; say, should we pretend it's an 8088?
	jne	nodebug			;  nope.
	mov	cpu,86			; yup.  use 16-bit emulation.
nodebug:
	call far ptr fputype		; what kind of an FPU do we have?
	mov	fpu,ax			;  save the results

initreturn:
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
	cmp	mouse,-1	       ; is mouse installed
	jz	short mchkit	       ; yes, so do stuff
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

;===============================================================
;
; CPUTYPE.ASM : C-callable functions cputype() and ndptype() adapted
; by Lee Daniel Crocker from code appearing in the late PC Tech Journal,
; August 1987 and November 1987.  PC Tech Journal was a Ziff-Davis
; Publication.	Code herein is copyrighted and used with permission.
;
; The function cputype() returns an integer value based on what kind
; of CPU it found, as follows:
;
;	Value	CPU Type
;	=====	========
;	86	8086, 8088, V20, or V30
;	186	80186 or 80188
;	286	80286
;	386	80386 or 80386sx
;	-286	80286 in protected mode
;	-386	80386 or 80386sx in protected or 32-bit address mode
;
; The function ndptype() returns an integer based on the type of NDP
; it found, as follows:
;
;	Value	NDP Type
;	=====	========
;	0	No NDP found
;	87	8087
;	287	80287
;	387	80387
;
; No provisions are made for the 80486 CPU/FPU or Weitek FPA chips.
;
; Neither function takes any arguments or affects any external storage,
; so there should be no memory-model dependencies.

.model medium, c
.286P
.code

cputype proc
	push	bp

	push	sp			; 86/186 will push SP-2;
	pop	ax			; 286/386 will push SP.
	cmp	ax, sp
	jz	not86			; If equal, SP was pushed
	mov	ax, 186
	mov	cl, 32			;   186 uses count mod 32 = 0;
	shl	ax, cl			;   86 shifts 32 so ax = 0
	jnz	exit			; Non-zero: no shift, so 186
	mov	ax, 86			; Zero: shifted out all bits
	jmp	short exit
not86:
	pushf				; Test 16 or 32 operand size:
	mov	ax, sp			;   Pushed 2 or 4 bytes of flags?
	popf
	inc	ax
	inc	ax
	cmp	ax, sp			;   Did pushf change SP by 2?
	jnz	is32bit 		;   If not, then 4 bytes of flags
is16bit:
	sub	sp, 6			; Is it 286 or 386 in 16-bit mode?
	mov	bp, sp			; Allocate stack space for GDT pointer
	sgdt	fword ptr [bp]
	add	sp, 4			; Discard 2 words of GDT pointer
	pop	ax			; Get third word
	inc	ah			; 286 stores -1, 386 stores 0 or 1
	jnz	is386
is286:
	mov	ax, 286
	jmp	short testprot		; Check for protected mode
is32bit:
	db	66h			; 16-bit override in 32-bit mode
is386:
	mov	ax, 386
testprot:
	smsw	cx			; Protected?  Machine status -> CX
	ror	cx,1			; Protection bit -> carry flag
	jnc	exit			; Real mode if no carry
	neg	ax			; Protected:  return neg value
exit:
	pop	bp
	ret
cputype endp

.data

control dw	0			; Temp storage for 8087 control
					;   and status registers
.code

fputype proc
	push	bp

	fninit				; Defaults to 64-bit mantissa
	mov	byte ptr control+1, 0
	fnstcw	control 		; Store control word over 0
;	dw	3ed9h			; (klooge to avoid the MASM \e switch)
;	dw	offset control		; ((equates to the above 'fnstcw' cmd))
	mov	ah, byte ptr control+1	; Test contents of byte written
	cmp	ah, 03h 		; Test for 64-bit precision flags
	je	gotone			; Got one!  Now let's find which
	xor	ax, ax
	jmp	short fexit		; No NDP found
gotone:
	and	control, not 0080h	; IEM = 0 (interrupts on)
	fldcw	control
	fdisi				; Disable ints; 287/387 will ignore
	fstcw	control
	test	control, 0080h
	jz	not87			; Got 287/387; keep testing
	mov	ax, 87
	jmp	short fexit
not87:
	finit
	fld1
	fldz
	fdiv				; Divide 1/0 to create infinity
	fld	st
	fchs				; Push -infinity on stack
	fcompp				; Compare +-infinity
	fstsw	control
	mov	ax, control
	sahf
	jnz	got387			; 387 will compare correctly
	mov	ax, 287
	jmp	short fexit
got387: 				; Only one left (until 487/Weitek
	mov	ax, 387 		;   test is added)
fexit:
	pop	bp
	ret
fputype endp

; ************************* Far Segment RAM Support **************************
;
;
;	farptr = (char far *)farmemalloc(long bytestoalloc);
;	(void)farmemfree(farptr);
;
;	alternatives to Microsoft/TurboC routines
;
;
.8086

farmemalloc	proc	uses es, bytestoallocate:dword
	les	bx,bytestoallocate	; get the # of bytes into DX:BX
	mov	dx,es			;  ...
	add	bx,15			; round up to next paragraph boundary
	adc	dx,0			;  ...
	shr	dx,1			; convert to paragraphs
	rcr	bx,1			;  ...
	shr	dx,1			;  ...
	rcr	bx,1			;  ...
	shr	dx,1			;  ...
	rcr	bx,1			;  ...
	shr	dx,1			;  ...
	rcr	bx,1			;  ...
	cmp	dx,0			; ensure that we don't want > 1MB
	jne	farmemallocfailed	;  bail out if we do
	mov	ah,48h			; invoke DOS to allocate memory
	int	21h			;  ... 
	jc	farmemallocfailed	; bail out on failure
	mov	dx,ax			; set up DX:AX as far address
	mov	ax,0			;  ...
	jmp	short farmemallocreturn	; and return
farmemallocfailed:
	mov	ax,0			; (load up with a failed response)
	mov	dx,0			;  ...
farmemallocreturn:
	ret				; we done.
farmemalloc	endp

farmemfree	proc	uses es, farptr:dword
	les	ax,farptr		; get the segment into ES
	mov	ah,49h			; invoke DOS to free the segment
	int	21h			;  ...
	ret
farmemfree	endp

erasesegment	proc	uses es di si, segaddress:word, segvalue:word
	mov	ax,segaddress		; load up the segment address
	mov	es,ax			;  ...
	mov	di,0			; start at the beginning
	mov	ax,segvalue		; use this value
	mov	cx,8000h		; over the entire segment
	repnz	stosw			; do it
	ret				; we done
erasesegment	endp

disable	proc				; disable interrupts
	cli
	ret
disable	endp

enable	proc				; re-enable interrupts
	sti
	ret
enable	endp

; *************** Expanded Memory Manager Support Routines ******************
;		for use with LIM 3.2 or 4.0 Expanded Memory
;
;	farptr = emmquery()	; Query presence of EMM and initialize EMM code
;				; returns EMM FAR Address, or 0 if no EMM
;	freepages = emmgetfree(); Returns the number of pages (1 page = 16K)
;				; not already allocated for something else
;	handle = emmallocate(pages)	; allocate EMM pages (1 page = 16K)
;				; returns handle # if OK, or else 0
;	emmdeallocate(handle)	; return EMM pages to system - MUST BE CALLED
;				; or allocated EMM memory fills up
;	emmgetpage(page, handle); get an EMM page (actually, links the EMM
;				; page to the EMM Segment ADDR, saving any
;				; prior page in the process)
;	emmclearpage(page, handle) ; performs an 'emmgetpage()' and then clears
;				; it out (quickly) to zeroes with a 'REP STOSW'

.MODEL	medium,c

.8086

.DATA

emm_name	db	'EMMXXXX0',0	; device driver for EMM
emm_segment	dw	0		; EMM page frame segment
emm_zeroflag	db	0		; klooge flag for handle==0

.CODE

emmquery	proc
	mov	ah,3dh			; function 3dh = open file
	mov	al,0			;  read only
	mov	dx,offset emm_name	; DS:DX = address of name of EMM
	int	21h			; open it
	jc	emmqueryfailed		;  oops.  no EMM.

	mov	bx,ax			; BX = handle for EMM
	mov	ah,44h			; function 44h = IOCTL
	mov	al,7			; get outo. status
	mov	cx,0			; CX = # of bytes to read
	int	21h			; do it.
	push	ax			; save the IOCTL handle.

	mov	ah,3eh			; function 3H = close
	int	21h			; BX still cintains handle
	pop	ax			; restore AX for the status query
	jc	emmqueryfailed		; huh?  close FAILED?

	or	al,al			; was the status 0?
	jz	emmqueryfailed		; well then, it wasn't EMM!

	mov	ah,40h			; query EMM: hardware ok?
	int	67h			; EMM call
	cmp	ah,0			; is it ok?
	jne	emmqueryfailed		; if not, fail

	mov	ah,41h			; query EMM: Get Page Frame Segment
	int	67h			; EMM call
	cmp	ah,0			; is it ok?
	jne	emmqueryfailed		; if not, fail
	mov	emm_segment,bx		; save page frame segment
	mov	dx,bx			; return page frame address
	mov	ax,0			;  ...
	jmp	short	emmqueryreturn	; we done.

emmqueryfailed:
	mov	ax,0			; return 0 (no EMM found)
	mov	dx,0			;  ...
emmqueryreturn:
	ret				; we done.
emmquery	endp

emmgetfree	proc			; get # of free EMM pages
	mov	ah,42h			; EMM call: get total and free pages
	int	67h			; EMM call
	cmp	ah,0			; did we suceed?
	jne	emmgetfreefailed	;  nope.  return 0 free pages
	mov	ax,bx			; else return # of free pages
	jmp	emmgetfreereturn	; we done.
emmgetfreefailed:
	mov	ax,0			; failure mode
emmgetfreereturn:
	ret				; we done
emmgetfree	endp

emmallocate	proc	pages:word	; allocate EMM pages
	mov	bx,pages		; BX = # of 16K pages
	mov	ah,43h			; ask for the memory
	int	67h			; EMM call
	mov	emm_zeroflag,0		; clear the klooge flag
	cmp	ah,0			; did the call work?
	jne	emmallocatebad		;  nope.
	mov	ax,dx			; yup.  save the handle here
	cmp	ax,0			; was the handle a zero?
	jne	emmallocatereturn	;  yup.  no kloogy fixes
	mov	emm_zeroflag,1		; oops.  set an internal flag
	mov	ax,1234			; and make up a dummy handle.
	jmp	short	emmallocatereturn ; and return
emmallocatebad:
	mov	ax,0			; indicate no handle
emmallocatereturn:
	ret				; we done.
emmallocate	endp

emmdeallocate	proc	emm_handle:word	; De-allocate EMM memory
emmdeallocatestart:
	mov	dx,emm_handle		; get the EMM handle
	cmp	dx,1234			; was it our special klooge value?
	jne	emmdeallocatecontinue	;  nope.  proceed.
	cmp	emm_zeroflag,1		; was it really a zero handle?
	jne	emmdeallocatecontinue	;  nope.  proceed.
	mov	dx,0			; yup.  use zero instead.
emmdeallocatecontinue:
	mov	ah,45h			; EMM function: deallocate
	int	67h			; EMM call
	cmp	ah,0			; did it work?
	jne	emmdeallocatestart	; well then, try it again!
emmdeallocatereturn:
	ret				; we done
emmdeallocate	endp

emmgetpage	proc	pagenum:word, emm_handle:word	; get EMM page
	mov	bx,pagenum		; BX = page numper
	mov	dx,emm_handle		; DX = EMM handle
	cmp	dx,1234			; was it our special klooge value?
	jne	emmgetpagecontinue	;  nope.  proceed.
	cmp	emm_zeroflag,1		; was it really a zero handle?
	jne	emmgetpagecontinue	;  nope.  proceed.
	mov	dx,0			; yup.  use zero instead.
emmgetpagecontinue:
	mov	ah,44h			; EMM call: get page
	mov	al,0			; get it into page 0
	int	67h			; EMM call
	ret				; we done
emmgetpage	endp

emmclearpage	proc	pagenum:word, emm_handle:word	; clear EMM page
	mov	bx,pagenum		; BX = page numper
	mov	dx,emm_handle		; DX = EMM handle
	cmp	dx,1234			; was it our special klooge value?
	jne	emmclearpagecontinue	;  nope.  proceed.
	cmp	emm_zeroflag,1		; was it really a zero handle?
	jne	emmclearpagecontinue	;  nope.  proceed.
	mov	dx,0			; yup.  use zero instead.
emmclearpagecontinue:
	mov	ah,44h			; EMM call: get page
	mov	al,0			; get it into page 0
	int	67h			; EMM call
	mov	ax,emm_segment		; get EMM segment into ES
	push	es			;  ...
	mov	es,ax			;  ...
	mov	di,0			; start at offset 0
	mov	cx,8192			; for 16K (in words)
	mov	ax,0			; clear out EMM segment to zeroes
	rep	stosw			; clear the page
	pop	es			; restore ES
	ret				; we done
emmclearpage	endp

	end

