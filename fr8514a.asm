

IFDEF ??Version
	MASM51
	QUIRKS
ENDIF

	.MODEL  medium,c

	.8086



HOPEN	equ	8
HINT	equ	16
HLDPAL	equ	19
HBBW	equ	21
HBBR	equ	23
HBBCHN	equ	24
HQMODE	equ	29
HCLOSE	equ	34
HINIT	equ	48
HSYNC	equ	49
HSPAL	equ	57
HRPAL	equ	58


.DATA

	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	dacbox:byte, daccount:word

afiptr		dd	0

xadj		dw	0
yadj		dw	0

extrn		paldata:byte		; 1024-byte array (in GENERAL.ASM)

extrn		stbuff:byte		; 415-byte array (in GENERAL.ASM)

linedata	db	0

hopendata	db	3, 0, 0, 0, 0
hclosedata	dw	2, 0
hinitdata	dw	2, 0
bbw		dw	10, 8, 0, 0, 0, 0
bbr		dw	12, 8, 0, 0, 0, 0, 0
chn		dw	6
		dd	linedata
		dw	1
pal		dw	10, 0, 0, 256
		dd	paldata
hidata		dw	4, 0, 8000h
amode		dw	18, 9 dup(?)

;svpaldata	dw	769
;		db	769 dup(?)

oops		db	13,10
		db	"Couldn't find the 8514/A interface"
		db	13,10
		db	"(Maybe you forgot to load HDIDLOAD)"
		db	13,10
		db	"$"

.CODE

	public	


callafi proc	near

	push	ds		; Pass the parameter pointer
	push	si

	shl	ax,1		; form offset from entry no. required
	shl	ax,1
	mov	si,ax

	les	bx, afiptr	; entry block address to es:bx
	call	dword ptr es:[bx][si]	 ; call entry point

	ret			; return to caller

callafi endp




getafi	proc	near


	mov	ax,357fh	; read interrupt vector 7f
	int	21h
	mov	ax,es
	or	ax,bx		; is 7f vector null
	stc
	jz	getafiret

	mov	ax,0105h	; get Interface address
	int	7fh		; by software interrupt 7f

	jc	getafiret		; Interface not OK if carry set

	mov	word ptr afiptr,dx	; save afi pointer offset
	mov	word ptr afiptr+2,cx	; save afi pointer segment

	clc			; clear carry flag

getafiret:
	ret			; return to caller

getafi endp


do85open proc	near

	push	ax
	mov	ax, HOPEN
	call	callafi

	mov	ax, offset stbuff	;get the state segment
	add	ax, 15
	mov	cl, 4
	shr	ax, cl

	mov	bx, ds
	add	ax, bx

	mov	si, offset hinitdata
	mov	[si] + 2, ax

	pop	ax
	call	callafi

	clc
	ret

do85open	endp


open8514	proc	far

	call	getafi		;get adapter interface
	jc	afinotfound

;	mov	si, offset svpaldata
;	mov	ax, HSPAL
;	call	callafi

	mov	bl, 0		;if > 640 x 480 then 1024 x 768

	mov	ax, xdots
	cmp	ax, 640
	ja	setupopen

	mov	ax, ydots
	cmp	ax, 480
	ja	setupopen

	inc	bl

setupopen:

	mov	si, offset hopendata	;open the adapter
	mov	byte ptr [si + 2], 40h		;zero the image but leave pallette
	mov	[si + 3], bl
	mov	ax, HINIT		;initialize state

	call	do85open
	jc	afinotfound

	mov	si, offset amode	;make sure on the size
	mov	ax, HQMODE		;get the adapter mode
	call	callafi

	mov	ax, amode + 10		;get the screen width
	cmp	ax, xdots
	jae	xdotsok		   	;check for fit
	mov	xdots, ax
xdotsok:
	sub	ax, xdots		;save centering factor
	shr	ax, 1
	mov	xadj, ax

	mov	ax, amode + 12		;get the screen height
	cmp	ax, ydots
	jae	ydotsok
	mov	ydots, ax
ydotsok:
	sub	ax, ydots
	shr	ax, 1
	mov	yadj, ax
	clc
	ret

afinotfound:				; No 8514/A interface found
	mov	ax,03h			; reset to text mode
	int	10h
	mov	dx,offset oops		; error out
	mov	ah,9			; sending the message
	int	21h
	mov	ax,4c00h		; end the program
	int	21h
	ret				; should never get here!

open8514	endp

reopen8514	proc	far

	mov	si, offset hopendata	;open the adapter
	mov	byte ptr [si + 2], 0C0h		;zero the image but leave pallette
	mov	ax, HSYNC		;initialize state
	call	do85open
	ret

reopen8514	endp


close8514	proc	far

	mov	si, offset hclosedata		;turn off 8514a
	mov	ax, HCLOSE
	call	callafi
	
;	mov	si, offset svpaldata		;restore the pallette
;	mov	ax, HRPAL
;	call	callafi

	ret

close8514	endp




fr85wdot	proc	far

	mov	linedata, al

	mov	bbw + 4, 1		;define the rectangle
	mov	bbw + 6, 1
	add	cx, xadj
	add	dx, yadj
	mov	bbw + 8, cx
	mov	bbw + 10, dx
	mov	si, offset bbw
	mov	ax, HBBW
	call	callafi

	mov	si, offset chn
	mov	word ptr [si + 2], offset linedata
	mov	word ptr [si + 6], 1	;send the data

	mov	ax, HBBCHN
	call	callafi

	ret

fr85wdot	endp


fr85wbox	proc	far

	add	ax, xadj
	add	bx, yadj
	mov	chn + 2, si		;point to data
	mov	bbw + 4, cx		;define the rectangle
	mov	bbw + 6, dx
	mov	bbw + 8, ax
	mov	bbw + 10, bx
	mov	ax, dx
	mul	cx
	mov	chn + 6, ax

	mov	si, offset bbw
	mov	ax, HBBW
	call	callafi

	mov	si, offset chn
	mov	ax, HBBCHN
	call	callafi

	ret

fr85wbox	endp


fr85rdot	proc	far


	mov	bbr + 4, 1		;define the rectangle
	mov	bbr + 6, 1
	add	cx, xadj
	add	dx, yadj
	mov	bbr + 10, cx
	mov	bbr + 12, dx
	mov	si, offset bbr
	mov	ax, HBBR
	call	callafi

	mov	chn, 1			;get the data
	mov	si, offset chn
	mov	ax, HBBCHN
	call	callafi


	mov	al, linedata


	ret

fr85rdot	endp

w8514pal	proc	far

	mov	si, offset dacbox

	mov	cx, daccount	;limit daccount to 128 to avoid fliker
	cmp	cx, 128
	jbe	countok

	mov	cx, 128
	mov	daccount, cx

countok:				;now build 8514 pallette
	mov	ax, 256			;from the data in dacbox
	mov	pal + 4, 0
	mov	di, offset paldata
	cld
cpallp:
	push	ax		     	;do daccount at a time
	mov	dx, di
	cmp	ax, cx
	jae	dopass
	mov	cx, ax
dopass:
	mov	pal + 6, cx		;entries this time
	push	cx
cpallp2:
	push	ds			;pallette format is r, b, g
	pop	es			;0 - 255 each

	lodsb				;red
	shl	al, 1
	shl	al, 1
	stosb
	lodsb				;green
	shl	al, 1
	shl	al, 1
	xchg	ah, al
	lodsb				;blue
	shl	al, 1
	shl	al, 1
	stosw
	mov	al, 0			;filler
	stosb
	loop	cpallp2

	push	si
	push	di
	push	dx

	mov	si, hidata		;wait for flyback
	mov	ax, HINT
	call	callafi

	pop	dx
	mov	pal + 8, dx
	
	mov	si, offset pal		;load this piece
	mov	ax, HLDPAL
	call	callafi

	pop	di
	pop	si
	pop	cx
	add	pal + 4, cx		;increment the pallette index
	pop	ax
	sub	ax, cx
	jnz	cpallp


	ret

w8514pal	endp


	end

