;	FRACASM.ASM - Assembler subroutines for fractals.c

;			 required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.MODEL	medium,c

.8086

	; these must NOT be in any segment!!
	; this get's rid of TURBO-C fixup errors
	extrn	multiply:far		; this routine is in 'general.asm'

.data

	extrn	lold:qword, lnew:qword	; each defined as LCMPLX in fractals.c
	extrn	ltempsqrx:dword 	; for fractals.c
	extrn	ltempsqry:dword 	; for fractals.c
	extrn	lmagnitud:dword 	; for fractals.c
	extrn	llimit:dword		; from calcfrac.c
	extrn	llimit2:dword		; from calcfrac.c
	extrn	bitshift:word		; fudgefactor for integer math
	extrn	overflow:word		; error from integer math

.code


	public	longbailout

longbailout	proc
;
; equivalent to the C code:
;  ltempsqrx = lsqr(lnew.x); ltempsqry = lsqr(lnew.y);
;  lmagnitud = ltempsqrx + ltempsqry;
;  if (lmagnitud >= llimit || lmagnitud < 0 || labs(lnew.x) > llimit2
;	 || labs(lnew.y) > llimit2 || overflow)
;	       { overflow=0; return(1); }
;  lold = lnew;
;  return(0);
;
;  ltempsqrx = lsqr(lnew.x);
	push	bitshift
	push	WORD PTR lnew+2
	push	WORD PTR lnew
	push	WORD PTR lnew+2
	push	WORD PTR lnew
	call	FAR PTR multiply
	mov	WORD PTR ltempsqrx,ax
	mov	WORD PTR ltempsqrx+2,dx
;  ltempsqry = lsqr(lnew.y);
	push	bitshift
	push	WORD PTR lnew+6
	push	WORD PTR lnew+4
	push	WORD PTR lnew+6
	push	WORD PTR lnew+4
	call	FAR PTR multiply
	add	sp,20
	mov	WORD PTR ltempsqry,ax
	mov	WORD PTR ltempsqry+2,dx
;  lmagnitud = ltempsqrx + ltempsqry;
	add	ax,WORD PTR ltempsqrx
	adc	dx,WORD PTR ltempsqrx+2
	mov	WORD PTR lmagnitud,ax
	mov	WORD PTR lmagnitud+2,dx
;  if (lmagnitud >= llimit
	cmp	dx,WORD PTR llimit+2
	jl	chkvs0
	jg	bailout
	cmp	ax,WORD PTR llimit
	jae	bailout
;   || lmagnitud < 0
chkvs0: or	dx,dx
	js	bailout
;   || labs(lnew.x) > llimit2
	mov	ax,WORD PTR lnew
	mov	dx,WORD PTR lnew+2
	or	dx,dx
	jge	lnewx
	neg	ax
	adc	dx,0
	neg	dx
lnewx:	cmp	dx,WORD PTR llimit2+2
	jl	chklnewy
	jg	bailout
	cmp	ax,WORD PTR llimit2
	ja	bailout
;   || labs(lnew.y) > llimit2
chklnewy:
	mov	ax,WORD PTR lnew+4
	mov	dx,WORD PTR lnew+6
	or	dx,dx
	jge	lnewy
	neg	ax
	adc	dx,0
	neg	dx
lnewy:	cmp	dx,WORD PTR llimit2+2
	jl	chkoflow
	jg	bailout
	cmp	ax,WORD PTR llimit2
	ja	bailout
;   || overflow)
chkoflow:
	cmp	overflow,0
	jne	bailout
;  else {
;  lold = lnew;
	mov	ax,WORD PTR lnew
	mov	dx,WORD PTR lnew+2
	mov	WORD PTR lold,ax
	mov	WORD PTR lold+2,dx
	mov	ax,WORD PTR lnew+4
	mov	dx,WORD PTR lnew+6
	mov	WORD PTR lold+4,ax
	mov	WORD PTR lold+6,dx
;  return(0); }
	sub	ax,ax
	ret
bailout:
;  { overflow=0; return(1); }
	mov	overflow,0
	mov	ax,1
	ret
longbailout	endp

		end

