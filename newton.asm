; NEWTON.ASM : Procedure Newton() from FRACTINT.
; Lee Daniel Crocker, 4/23/89.
;
; Tabs: 8
;
; Modifications:
;   BT = Bert Tyler
;   TW = Timothy Wegner
;   RD = Robert Day
;
; Assembled by Microsoft Macro Assembler 5.1, for use with Microsoft C 5.1.
;

;		 required for compatibility if Turbo ASM (BT 5/20/89)
IFDEF ??Version
MASM51
QUIRKS
ENDIF

.model	medium,c

	extrn	check_key:far		; (BT 5/20/89) moved out here for TASM

.data
	extrn	orbit_ptr:word, infinity:word, maxit:word, oldmax:word
	extrn	color:word, iterations:word, maxcolor:word, inside:word
	extrn	not_done:word, degree:word, oldcolor:word, colors:word
	extrn	init:qword, new:qword, d1overd:qword, roverd:qword
	extrn	threshold:qword, floatmin:qword, saved:qword
	extrn	roots:qword, old:qword, dx0:dword, dy0:dword
	extrn	passes:word, numpasses:word, guessing:word, row:word
	extrn	iystart:word, iystop:word, ixstart:word, ixstop:word
	extrn	fractype:word, plot:dword

	extrn	param:qword, invert:word		; LC 5/9/89

;state87 db	108 dup(?)		; BT 5/7/89 removed fsave/frstor
statw	dw	?
tempd	dq	?

.code
;
; Procedure newtonloop().  This calculates the iterations for one point.
;
newtonloop proc
	local	d1:word, c:word

	xor	ax, ax
	mov	c, ax			; c = 0
	mov	orbit_ptr, ax		; orbit_ptr = 0
	mov	iterations, ax		; iterations = 0
	inc	ax
	mov	not_done, ax		; not_done = 1
	neg	ax
	mov	color, ax		; color = -1
	mov	ax, degree
	dec	ax
	mov	d1, ax			; d1 = degree - 1

	fld	qword ptr init+8
	fld	qword ptr init		; old = init

	cmp	invert, 0		; LC 5/9/89: If second parameter was
	jz	nlloop			; given to newton, inversion is enabled
										; and performed here for each point.
	fld	st(1)	    ; init.y
	fld	qword ptr param+24
	fsub				; y -= ycenter
	fmul	st, st(2)
	fld	st(1)	    ; init.x
	fld	qword ptr param+16
	fsub				; x -= xcenter
	fmul	st, st(2)
	fadd
	fld	qword ptr param+8
	fdivr				; f = radius / (x*x + y*y)
	fmul	st(2), st		; y *= f
	fmul				; x *= f
	fld	qword ptr param+24
	faddp	st(2), st		; y += ycenter
	fld	qword ptr param+16
	fadd				; x += xcenter
nlloop:
	mov	ax, not_done		; while (not_done) {
	cmp	ax, 0
	jnz	nll2
	jmp	nlexit
nll2:
	inc	iterations		; ++iterations
	fld	st(1)
	fld	st(1)			; tmp = old
	mov	ax, d1
;
; Here begins a complex_power() routine which raises the complex number
; in ST, ST(1) to the power of AX.  It is expanded here inline to avoid
; the subroutine call overhead.
;
	shr	ax, 1
	jnc	load1			; if (d1 & 1)

	fld	st(1)
	fld	st(1)
	jmp	short looptop		; tmp = old
load1:
	fldz
	fld1				; else tmp = [1,0]
looptop:
	cmp	ax, 0
	je	loopexit		; while (d1)

;	fld	st(2)
;	fmul	st, st
;	fld	st(4)
;	fmul	st, st
;	fsub				; xt = xt * xt - yt * yt

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
	jnc	looptop 		; if (d1 & 1)

	fld	st(2)
	fmul	st, st(1)
	fld	st(4)
	fmul	st, st(3)
	fsub				; tmp.x = xt * tmp.x - yt * tmp.y
	fstp	tempd

	fld	st(2)
	fmul	st, st(2)
	fld	st(4)
	fmul	st, st(2)
	fadd				; tmp.y = xt * tmp.y + yt * tmp.x
	fstp	st(2)
	fld	tempd
	fstp	st(1)

	jmp	short looptop
loopexit:
	fstp	st(2)
	fstp	st(2)
;
; End of complex_power() routine.  Result is in ST, ST(1)
;
	fld	st(3)	    ; old.y
	fmul	st, st(1)   ; tmp.x
	fld	st(3)	    ; old.x
	fmul	st, st(3)   ; tmp.y
	fadd
	fld	st(3)	    ; old.x
	fmul	st, st(2)   ; tmp.x
	fld	st(5)	    ; old.y
	fmul	st, st(4)   ; tmp.y
	fsub				; new = tmp * old

	fld1
	fsubr	st, st(1)
	fmul	st, st
	fld	st(2)	    ; new.y
	fmul	st, st
	fadd
	fcomp	qword ptr threshold
	fstsw	statw
	mov	ax, statw
	sahf				; if dist(new, one) < threshold
	jc	exit1
	mov	ax, iterations
	cmp	ax, maxit		; or iterations > maxit
	jl	noexit2
exit1:
	xor	ax, ax
	mov	not_done, ax		; not_done = 0
noexit2:
	fld	qword ptr d1overd
	fmul	st(2), st		; new.y *= d1overd
	fmul
	fld	qword ptr roverd
	fadd				; new.x = d1overd * new.x + roverd

	fld	st(3)	    ; tmp.y
	fmul	st, st
	fld	st(3)	    ; tmp.x
	fmul	st, st
	fadd
	fcom	qword ptr floatmin
	fstsw	statw
	mov	ax, statw
	sahf
	jnc	cont			; if mod(tmp) < FLT_MIN
	xor	ax, ax
	mov	not_done, ax
	mov	ax, infinity
	fstp	st
	fstp	st
	fstp	st
	fstp	st
	fstp	st
;
; TW 5/6/89: Rearranged stack pops to get proper value into new
;
	fstp	st
	fstp	st
	jmp	short break
cont:
	fld1
	fdivr
	fst	st(6)	    ; old.y
	fstp	st(5)	    ; old.x
	fld	st(2)	    ; tmp.x
	fmul	st, st(1)   ; new.x
	fld	st(4)	    ; tmp.y
	fmul	st, st(3)   ; new.y
	fadd
	fmulp	st(5), st   ; old.x

	fld	st(2)	    ; tmp.x
	fmul	st, st(2)   ; new.y
	fld	st(4)	    ; tmp.y
	fmul	st, st(2)   ; new.x
	fsub
	fmulp	st(6), st   ; old.y	; old = new / tmp

	mov	ax, not_done
	cmp	ax, 0
	jz	nlexit

	fstp	st
	fstp	st
	fstp	st
	fstp	st
	jmp	nll2
nlexit:
;
; TW 5/6/89
;
	fstp	st
	fstp	st
	fstp	st
	fstp	st
	fstp	qword ptr new
	fstp	qword ptr new+8
	mov	ax, c
break:
;
; TW 5/6/89
;	fstp	st
;	fstp	st

	ret
newtonloop endp


Newton	proc

;	fsave	state87
	xor	ax, ax
	mov	passes, ax		; passes = 0
nloop1:
	mov	ax, passes
	cmp	ax, numpasses
	jle	ncont1
	jmp	nexit1			; while (passes <= numpasses) {
ncont1:
	mov	ax, guessing
	cmp	ax, 1
	jne	test2
	mov	ax, passes
	cmp	ax, 1
	jne	test2
	jmp	nexit1			; if (guesses==1 && passes==1) break;
test2:
	mov	ax, guessing
	cmp	ax, 2
	jne	test4
	mov	ax, passes
	cmp	ax, 0
	jne	test4
	jmp	ncont2			; if (guesses==2 && passes==0) continue;
test4:
	mov	ax, iystart
	mov	row, ax
nloop2:
	mov	ax, row
	cmp	ax, iystop		; while (row <= iystop
	jle	ncont12
	jmp	ncont2
ncont12:
; check key moved so checks evey pixel TW 5/10/89
;	call	check_key
;	cmp	ax, 0
;	je	ncont13
;	mov	ax, -1
;	jmp	nret			; if (check_key()) return -1
;ncont13:
	mov	ax, 1
	mov	oldcolor, ax		; oldcolor = 1
	mov	ax, maxit
	mov	oldmax, ax		; oldmax = maxit
	fldz
	fst	qword ptr saved
	fstp	qword ptr saved+8	; saved = [0,0]
	mov	bx, row
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1
	push	ds
	lds	si, dy0
	fld	qword ptr [si+bx]
	pop	ds
	fstp	qword ptr init+8	; init.y = dy0[row]

	mov	di, ixstart
nloop3:
	cmp	di, ixstop
	jle	ncont4
	jmp	ncont3			; while (col <= ixstop) {
ncont4:
	mov	ax, passes
	cmp	ax, 0
	je	ncont5
	mov	ax, row
	and	ax, 1
	jne	ncont5
	mov	ax, di
	and	ax, 1
	jne	ncont5
;
; TW 5/1/89: Following line changed to reflect changing "break"
; to "continue" in C code.
;
	jmp	ncont6			; if (passes && (row&1 == 0) && ...
ncont5:
	mov	bx, di
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1
	push	ds
	lds	si, dx0
	fld	qword ptr [si+bx]
	pop	ds
	fstp	qword ptr init		; init.x = dx0[col]
ncont7:
        ; check_key moved here for Bert to check every pixel
        ; TW 5/10/89
	call	check_key
	cmp	ax, 0
	je	ncont13
	mov	ax, -1
	jmp	nret			; if (check_key()) return -1
ncont13:
	call	newtonloop		; color = newtonloop()
	mov	si, ax

	mov	ax, fractype
	cmp	ax, 2
	je	ft2			; if (fractype == 2) {
	jmp	notft2
ft2:
	xor	si, si			; color = 0
	xor	cx, cx
nloop4:
	cmp	cx, degree
	jge	ncont9
	mov	bx, cx
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1			; complex is 16 bytes
	fld	qword ptr new
	add	bx, offset roots
	fld	qword ptr [bx]
	fsub
	fmul	st, st
	fcom	qword ptr threshold	; if (t2 = (new.x - roots[i].x)^2 ...
	fstsw	statw
	mov	ax, statw
	sahf
	jc	ncont11
	jz	ncont11
	fstp	st
	jmp	ncont10
ncont11:
	fld	qword ptr new+8
	fld	qword ptr [bx+8]
	fsub
	fmul	st, st
	fadd
	fcomp	qword ptr threshold	; if (t2 + (new.y - roots[i].y)^2 ...
	fstsw	statw
	mov	ax, statw
	sahf
	jnc	ncont10

	mov	ax, iterations
	and	ax, 1
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	push	cx    ; TW next statement trashes cx if degree large
	and	cx, 7
	add	ax, cx
	pop	cx    ; TW restore cx
	inc	ax
	mov	si, ax			; color=1 + (i&7) + ((iterations&1)<<3)
	jmp	ncont9
ncont10:
	inc	cx
	jmp	short nloop4
ncont9:
	cmp	si, 0			; if (!color) color = maxcolor
	jne	short ncont8
	mov	si, maxcolor
	jmp	short ncont8
notft2:
	mov	ax, iterations
	cmp	ax, maxit		; if (iterations < maxit)
	jl	notmax
	mov	si, inside		; color = inside
	jmp	short ncont8
notmax:
	mov	bx, colors
	dec	bx
	and	ax, bx
	mov	si, ax			; color = iterations & (colors-1)
ncont8:
	push	si
	mov	ax, row
	push	ax
	push	di
	call	dword ptr [plot]	; plot(col, row, color)
	add	sp, 6

	mov	ax, numpasses
	cmp	ax, 0
	je	ncont6			; if (numpasses && (passes == 0))

	mov	ax, passes
	cmp	ax, 0
	jne	ncont6

	push	si
	mov	ax, row
	inc	ax
	push	ax
	push	di
	call	dword ptr [plot]	; plot(col, row+1, color)
	add	sp, 6

	push	si
	mov	ax, row
	push	ax
	mov	ax, di
	inc	ax
	push	ax
	call	dword ptr [plot]	; plot(col+1, row, color)
	add	sp, 6

	push	si
	mov	ax, row
	inc	ax
	push	ax
	mov	ax, di
	inc	ax
	push	ax
	call	dword ptr [plot]	; plot(col+1, row+1, color)
	add	sp, 6
ncont6:
	inc	di
	add	di, numpasses
	sub	di, passes
	jmp	nloop3
ncont3:
	mov	ax, row
	inc	ax
	add	ax, numpasses
	sub	ax, passes
	mov	row, ax 		; row += 1 + numpasses - passes
	jmp	nloop2
ncont2:
	inc	passes
	jmp	nloop1			; ++passes
nexit1:
	mov	ax, 0			; return 0
nret:
;	frstor	state87
	ret
Newton	endp

	end

