; LSYSA.ASM: assembler support routines for optimized L-System code
; Nicholas Wilt, 11/13/91.
;

.MODEL	MEDIUM,C

EXTRN	reverse:BYTE,angle:BYTE,maxangle:BYTE,dmaxangle:BYTE
EXTRN	xpos:DWORD,ypos:DWORD,ssize:DWORD
EXTRN	realangle:DWORD
EXTRN	lsys_Xmin:DWORD,lsys_Xmax:DWORD,lsys_Ymin:DWORD,lsys_Ymax:DWORD
EXTRN	sins:DWORD
EXTRN	coss:DWORD
EXTRN	overflow:WORD

.CODE

decangle	PROC	NEAR
	mov	al,angle
	or	al,al
	jz	DecAngle0
	dec	angle
	ret
DecAngle0:
	mov	al,dmaxangle
	mov	angle,al
	ret
decangle	ENDP

incangle	PROC	NEAR
	mov	al,angle
	inc	al
	cmp	al,maxangle
	jnz	IncWriteAngle
	xor	ax,ax
IncWriteAngle:
	mov	angle,al
	ret
incangle	ENDP

	PUBLIC	lsys_doplus

lsys_doplus	PROC
	cmp	reverse,0
	jnz	PlusIncAngle
	call	decangle
	ret
PlusIncAngle:
	call	incangle
	ret
lsys_doplus	ENDP

	PUBLIC	lsys_dominus

lsys_dominus	PROC
	cmp	reverse,0
	jnz	MinusDecAngle
	call	incangle
	ret
MinusDecAngle:
	call	decangle
	ret
lsys_dominus	ENDP

	PUBLIC	lsys_doplus_pow2

lsys_doplus_pow2	PROC
	mov	al,angle
	cmp	reverse,0
	jnz	Plus2IncAngle
	dec	al
	and	al,dmaxangle
	mov	angle,al
	ret
Plus2IncAngle:
	inc	al
	and	al,dmaxangle
	mov	angle,al
	ret
lsys_doplus_pow2	ENDP

	PUBLIC	lsys_dominus_pow2

lsys_dominus_pow2	 PROC
	mov	al,angle
	cmp	reverse,0
	jz	Minus2IncAngle
	dec	al
	and	al,dmaxangle
	mov	angle,al
	ret
Minus2IncAngle:
	inc	al
	and	al,dmaxangle
	mov	angle,al
	ret
lsys_dominus_pow2	 ENDP

	PUBLIC	lsys_dopipe_pow2

lsys_dopipe_pow2	PROC
	xor	ax,ax
	mov	al,maxangle
	shr	ax,1
	xor	dx,dx
	mov	dl,angle
	add	ax,dx
	and	al,dmaxangle
	mov	angle,al
	ret
lsys_dopipe_pow2	ENDP

	PUBLIC	lsys_dobang

lsys_dobang	PROC
	mov	al,reverse	; reverse = ! reverse;
	dec	al		; -1 if was 0; 0 if was 1
	neg	al		; 1 if was 0; 0 if was 1
	mov	reverse,al	;
	ret
lsys_dobang	ENDP

; Some 386-specific leaf functions go here.

.386

	PUBLIC	lsys_doslash_386

lsys_doslash_386	PROC	N:DWORD
	mov	eax,N
	cmp	reverse,0
	jnz	DoSlashDec
	add	realangle,eax
	ret
DoSlashDec:
	sub	realangle,eax
	ret
lsys_doslash_386	ENDP

	PUBLIC	lsys_dobslash_386

lsys_dobslash_386	 PROC	 N:DWORD
	mov	eax,N
	cmp	reverse,0
	jz	DoBSlashDec
	add	realangle,eax
	ret
DoBSlashDec:
	sub	realangle,eax
	ret
lsys_dobslash_386	 ENDP

	PUBLIC	lsys_doat_386

lsys_doat_386	PROC	N:DWORD
	mov	eax,ssize	; Get size
	imul	N		; Mul by n
	shrd	eax,edx,19	; Shift right 19 bits
	mov	ssize,eax	; Save back
	ret
lsys_doat_386	ENDP

	PUBLIC	lsys_dosizegf_386

lsys_dosizegf_386	PROC
	mov	ecx,ssize	; Get size; we'll need it twice
	xor	bx,bx		; BX <- angle*sizeof(int)
	mov	bl,angle	;
	shl	bx,1		;
	shl	bx,1		;
	mov	eax,coss[bx]	; eax <- coss[angle]
	imul	ecx		; Mul by size
	shrd	eax,edx,29	; eax <- multiply(size, coss[angle], 29)
	add	eax,xpos	;
	jno	nooverfl	;   check for overflow
	mov	overflow,1	;    oops - flag the user later
nooverfl: 
	cmp	eax,lsys_Xmax	; If xpos <= lsys_Xmax,
	jle	GF1		;   jump
	mov	lsys_Xmax,eax	;
GF1:	cmp	eax,lsys_Xmin	; If xpos >= lsys_Xmin
	jge	GF2		;   jump
	mov	lsys_Xmin,eax	;
GF2:	mov	xpos,eax	; Save xpos
	mov	eax,sins[bx]	; eax <- sins[angle]
	imul	ecx		;
	shrd	eax,edx,29	;
	add	eax,ypos	;
	cmp	eax,lsys_Ymax	; If ypos <= lsys_Ymax,
	jle	GF3		;   jump
	mov	lsys_Ymax,eax	;
GF3:	cmp	eax,lsys_Ymin	; If ypos >= lsys_Ymin
	jge	GF4		;   jump
	mov	lsys_Ymin,eax	;
GF4:	mov	ypos,eax	;
	ret
lsys_dosizegf_386	ENDP

	PUBLIC	lsys_dodrawg_386

lsys_dodrawg_386	PROC
	mov	ecx,ssize	; Because we need it twice
	xor	bx,bx		; BX <- angle * sizeof(int)
	mov	bl,angle	;
	shl	bx,1		;
	shl	bx,1		;
	mov	eax,coss[bx]	; eax <- coss[angle]
	imul	ecx		;
	shrd	eax,edx,29	;
	add	xpos,eax	; xpos += size*coss[angle] >> 29
	mov	eax,sins[bx]	; eax <- sins[angle]
	imul	ecx		; ypos += size*sins[angle] >> 29
	shrd	eax,edx,29	;
	add	ypos,eax	;
	ret			;
lsys_dodrawg_386	ENDP

	END
