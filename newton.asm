; NEWTON.ASM : Procedure NewtonFractal() from FRACTINT.
; Lee Daniel Crocker, 4/23/89.
;
; Tabs: 8
;
; Modifications:
;   BT = Bert Tyler
;   TW = Timothy Wegner
;   RD = Robert Day
;   MP = Mark Peterson
;
; Note: newton.asm was totally rewritten by Lee Crocker for FRACTINT 10.0
;    for integration with the newly structured fractal engine in calcmand.c
;    and fractals.c. The current routine consists of the inner orbit
;    calculation, with the supporting code removed. Early versions of
;    newton.asm contained a complete newton fractal function.
;
; Assembled by Microsoft Macro Assembler 5.1, for use with Microsoft C 5.1.
;

; Required for compatibility if Turbo ASM (BT 5/20/89)

IFDEF ??version
MASM51
QUIRKS
ENDIF

.model	medium, c

public	NewtonFractal2
public	invertz2

.data
	extrn	color:word, maxcolor:word, degree:word, basin:word
	extrn	row:word, col:word

	extrn	dx0:dword, dy0:dword
	extrn	dx1:dword, dy1:dword

	extrn	old:qword, new:qword, d1overd:qword, roverd:qword
	extrn	threshold:qword, floatmin:qword, floatmax:qword
	extrn	f_radius:qword, f_xcenter:qword, f_ycenter:qword
	extrn	roots:word, tempsqrx:qword

statw	dw	?

.code

public	NewtonFractal2
NewtonFractal2 proc

;
; cpower(&old, degree-1, &tmp)
;
	mov	ax, degree
	dec	ax

	fld	old + 8
	fld	old
;
; cpower() is expanded inline here.
;
	shr	ax, 1
	jnc	load1			; if (exp & 1)

	fld	st(1)
	fld	st(1)
	jmp	short looptop		; tmp = old
load1:
	fldz
	fld1				; else tmp = [1,0]
looptop:
	cmp	ax, 0
	je	loopexit		; while (exp)

	fld	st(2)			; RD 5/7/89: Calculate xt^2 - yt^2
	fadd	st, st(4)		; by using (xt+yt)*(xt-yt), which
	fld	st(3)			; trades one multiplication for an
	fsub	st, st(5)		; addition.  This trick saves a
	fmul				; whopping 1.2% of time.

	fld	st(4)
	fmul	st, st(4)
	fadd	st, st			; yt = 2 * xt * yt

	fstp	st(5)			; tmp.y = yt
	fstp	st(3)			; tmp.x = xt

	shr	ax, 1
	jnc	looptop 		; if (exp & 1)

	fld	st(2)
	fmul	st, st(1)
	fld	st(4)
	fmul	st, st(3)
	fsub				; tmp.x = xt * tmp.x - yt * tmp.y

	fld	st(3)
	fmul	st, st(3)
	fld	st(5)
	fmul	st, st(3)
	fadd				; tmp.y = xt * tmp.y + yt * tmp.x
	fstp	st(3)
	fstp	st(1)

	jmp	short looptop
loopexit:
	fstp	st(2)
	fstp	st(2)
;
; End of complex_power() routine.  Result is in ST, ST(1)
;
;
; complex_mult(tmp, old, &new);
;
	fld	old + 8
	fld	old

	fld	st(3)	    ; tmp.y
	fmul	st, st(1)   ; old.x
	fld	st(3)	    ; tmp.x
	fmul	st, st(3)   ; old.y
	fadd
	fld	st(3)	    ; tmp.x
	fmul	st, st(2)   ; old.x
	fld	st(5)	    ; tmp.y
	fmul	st, st(4)   ; old.y
	fsub
;
; if (DIST1(new) < THRESHOLD) {
;
	fld1
	fsubr	st, st(1)
	fmul	st, st
	fld	st(2)	    ; new.y
	fmul	st, st
	fadd
	fcomp	threshold
	fstsw	statw
	mov	ax, statw
	sahf
	jnc	notless
;
; if (fractype == NEWTBASIN) {
;
	mov	ax, basin
	cmp	ax, 0
	je	notbasin

	mov	bx, roots
	mov	dx, -1			; tempcolor = -1
	sub	cx, cx
dloop:
	fld	qword ptr [bx]	; roots[i].x
	fsub	st, st(3)	; old.x
	fmul	st, st
	fld	qword ptr [bx+8]; roots[i].y
	fsub	st, st(5)	; old.y
	fmul	st, st
	fadd
	fcomp	threshold
	fstsw	statw
	mov	ax, statw
	sahf				; if (distance(roots[i],old) < threshold)...
	jnc	nl2

; TW commented out next few lines and add dx,ax to eliminate newtbasin
; color shades per Phil Wilson's request 12/03/89

; TW put it back in in response to another use as an option! 7/7/90
	mov	dx, cx
	cmp	basin,2			; basin==2 is flag for stripes
	jne	nostripes
	mov	ax, color
	and	ax, 1
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1

	and	dx, 7
	add	dx, ax
nostripes:
	inc	dx			; tempcolor = 1+(i&7)+((color&1)<<3)
	jmp	short nfb		; break
nl2:
	add	bx, 16
	inc	cx
	cmp	cx, degree
	jl	dloop
nfb:
	mov	ax, dx
	cmp	dx, -1
	jne	notm1
	mov	ax, maxcolor		; if (tmpcolor == -1)...
notm1:
	mov	color, ax
notbasin:
	mov	ax, 1
	jmp	nlexit
notless:
	fld	d1overd
	fmul	st(2), st		; new.y *= d1overd
	fmul
	fld	roverd
	fadd				; new.x = d1overd * new.x + roverd

	fld	st(5)	    ; tmp.y
	fmul	st, st
	fld	st(5)	    ; tmp.x
	fmul	st, st
	fadd
	fcom	floatmin
	fstsw	statw
	mov	ax, statw
	sahf				; if (mod(tmp) < FLT_MIN) {
	jnc	cont
	mov	ax, 1
	fstp	st
	jmp	nlexit
cont:
	fld1
	fdivr
	fst	st(4)	    ; old.y
	fstp	st(3)	    ; old.x

	fld	st(4)	    ; tmp.x
	fmul	st, st(1)   ; new.x
	fld	st(6)	    ; tmp.y
	fmul	st, st(3)   ; new.y
	fadd
	fmulp	st(3), st   ; old.x

	fld	st(4)	    ; tmp.x
	fmul	st, st(2)   ; new.y
	fld	st(6)	    ; tmp.y
	fmul	st, st(2)   ; new.x
	fsub
	fmulp	st(4), st   ; old.y	; old = new / tmp

; MP Orbit Bug Fix 11/1/90
;	fstp	new
;	fstp	new + 8
;	fstp	old
;	fstp	old + 8
	fstp	st
	fstp	st
	fst	new
	fstp	old
	fst	new + 8
	fstp	old + 8

	mov	ax, 0
	jmp	nlx2

nlexit:

;MP Orbit Bug Fix 11/1/90
;	fstp	new
;	fstp	new + 8
	fstp	st
	fstp	st
	fstp	new
	fstp	new + 8

nlx2:
	fstp	st
	fstp	st

	ret
NewtonFractal2 endp
;
;
;
public	invertz2
invertz2 proc	uses si, zval:word	; TW 11/03/89 changed zval to near

	fld	f_xcenter
	fld	f_ycenter

	mov	bx, col
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1

	mov	cx, row
	shl	cx, 1
	shl	cx, 1
	shl	cx, 1

	mov	ax, ds
	lds	si, dx0
	add	si, bx
	fld	qword ptr [si]
	mov	ds, ax
	lds	si, dx1
	add	si, cx
	fld	qword ptr [si]
	fadd
	fsub	st, st(2)

	mov	ds, ax
	lds	si, dy0
	add	si, cx
	fld	qword ptr [si]
	mov	ds, ax
	lds	si, dy1
	add	si, bx
	fld	qword ptr [si]
	fadd
	fsub	st, st(2)

	mov	ds, ax
	fld	st(1)
	fmul	st, st
	fld	st(1)
	fmul	st, st
	fadd

	fcom	floatmin
	fstsw	statw
	mov	ax, statw
	sahf
	jnc	inl1

	fstp	st
	fld	floatmax
	jmp	icom
inl1:
	fld	f_radius
	fdivr
icom:
	fst	tempsqrx

	fmul	st(2), st
	fmul
	faddp	st(2), st
	faddp	st(2), st

;	lds	si, zval	; TW 11/03/89 zval is now a near pointer
	mov	si, zval	; TW
	fstp	qword ptr [si+8]
	fstp	qword ptr [si]

	ret
invertz2 endp

END
