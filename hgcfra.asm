	TITLE	Hercules Graphics Routines for FRACTINT

;			 required for compatibility if Turbo ASM
IFDEF ??version
	MASM51
	QUIRKS
ENDIF

	.MODEL  medium,c

	.8086
.data

HGCBase		equ	0B000h		;segment for HGC regen buffer page 0

herc_index	equ	03B4h
herc_cntrl	equ	03B8h
herc_status	equ	03BAh
herc_config	equ	03BFh


; Hercules control/configuration register settings

scrn_on	equ	08h
grph	equ	02h
text	equ	20h
enable  equ     03h

.code

;
;
; inithgc -  Initialize the HGC in graphics mode.
;           Code mostly from the Hercules Graphics Card Owner's Manual.
;
inithgc	PROC USES DI SI

	mov	al,enable		; enable mode changes
	mov	dx,herc_config		; same as HGC FULL command
	out	dx,al

	mov	al,grph			; set graphic mode
	lea	si,gtable		; address of graphic parameters
	mov	bx,0
	mov	cx,4000h
	call	setmd			; call set mode common processing

	ret

inithgc	ENDP

;
; termhgc -  Restore the Hercules Graphics Card to text mode.
;           Code mostly from the Hercules Graphics Card Owner's Manual.
;
termhgc	PROC USES DI SI

	mov	al,text			; set text mode
	lea	si,ttable		; get address of text parameters
	mov	bx,720h
	mov	cx,2000
	call	setmd

	ret

termhgc	ENDP

;
; setmd - sets mode to graphic or text depending on al
;         si = address of parameter table
;         cx = number of words to be cleared
;         bx = blank value
;
;         from Hercules Graphics Card Owner's Manual
;
setmd	PROC NEAR
;
	push	bp
	mov	bp,sp
	push	ax
	push	bx
	push	cx

;    change mode, but without screen on   
	mov	dx,herc_cntrl		; get address of control register
	out	dx,al                   ; al has the mode byte

;    initialize the 6845
	mov	ax,ds
	mov	es,ax			; also point es:si to parameter table

	mov	dx,herc_index           ; get index register address
	mov	cx,12                   ; 12 parameters to be output
	xor	ah,ah                   ; starting from register 0

parms:	mov	al,ah			; first output register number
	out	dx,al

	inc	dx			; get data register address
	lodsb                           ; get next byte from param. table
	out	dx,al                   ; output parameter data

	inc	ah                      ; increment register number
	dec	dx                      ; restore index register address
	loop	parms                   ; go do another one

;    now go clear the buffer
	pop	cx			; get number of words to clear
	mov	ax,HGCBase              ; get address off video buffer
	cld 				; set auto increment
	mov	es,ax                   ; set segment for string move
	xor	di,di                   ; start at offset 0
	pop	ax                      ; get blank value
	rep	stosw			; repeat store string

;   turn screen on with page 0 active
	mov	dx,herc_cntrl		; get control register address
	pop	ax			; get the mode byte
	add	al,scrn_on		; set the screen-on bit
	out	dx,al

	mov	sp,bp
	pop	bp
	ret

setmd	ENDP
;
; writehgc (x, y, c)  - write a dot at x, y in color c
;  x = x coordinate
;  y = y coordinate
;  c = color
;
writehgc	PROC USES DI SI, x, y, c

	cmp	y,348			; Clip for hardware boundaries
	jge	WtDot030
	cmp	x,720
	jge	WtDot030

	lea	bx,HGCRegen		;set up offset of regen scan line table
	mov	ax,HGCBase		;segment for regen buffer
	mov	es,ax

; calculate byte address of dot in regen buffer
	mov	si,y		;get y coordinate
	shl	si,1		;mult by 2 to get offset in Scan Line Table
	mov	si,[bx][si]	;get address of start of scan line from table
	mov	ax,x		;get x coordinate
	mov	cl,3
	shr	ax,cl		;divide by 8 to get byte offset
	add	si,ax		;es:si has address of byte with the bit
; build the bit mask for the specific dot in the byte
	mov	cx,x		;get x coordinate
	and	cx,0007h	;get bit number within the byte
	mov	al,80h		;prepare bit mask
	shr	al,cl		;al has bit mask
; either turn on the bit or turn it off, depending on the color
	cmp	word ptr c,0	;turn off bit?
	je	WtDot020	;yes -- branch
; turn on the bit
	or	byte ptr es:[si],al
	jmp	short WtDot030
WtDot020:
; turn off the bit
	xor	al,0FFh
	and	byte ptr es:[si],al
WtDot030:
	ret

writehgc	ENDP
;
; readhgc (x,y) - read a dot at x,y
;  x = x coordinate
;  y = y coordinate
;
;  dot value is to be returned in AX
;
readhgc	PROC USES DI SI, x,y

	lea	bx,HGCRegen		;set up offset of regen scan line table
	mov	ax,HGCBase		;segment for regen buffer
	mov	es,ax

; calculate byte address of dot in regen buffer
	mov	si,y		;get y coordinate
	shl	si,1		;mult by 2 to get offset in Scan Line Table
	mov	si,[bx][si]	;get address of start of scan line from table
	mov	ax,x		;get x coordinate
	mov	cl,3
	shr	ax,cl		;divide by 8 to get byte offset
	add	si,ax		;es:si has address of byte with the bit

; build the bit mask for the specific dot in the byte
	mov	cx,x		;get x coordinate
	and	cx,0007h	;get bit number within the byte
	mov	al,80h		;prepare bit mask
	shr	al,cl		;al has bit mask

; pick up the bit from the regen buffer
	test	byte ptr es:[si],al
	jz	readhgc020	;branch if bit is zero
	mov	ax,1		;else return foreground bit value
	jmp	short readhgc030
readhgc020:
	xor	ax,ax		;bit is zero
readhgc030:
	ret

readhgc	ENDP
;
;   linehgc (x1,y1, x2,y2, c)  - x1,y1 to x2,y2 in color c
;   x1 = x1 coordinate
;   y1 = y1 coordinate
;   x2 = x2 coordinate
;   y2 = y2 coordinate
;   c  = color
;

linehgc	PROC USES DI SI, x1,y1,x2,y2,c

	lea	bx,HGCRegen		;set up offset of regen scan line table
	mov	ax,HGCBase		;regen buffer segment
	mov	es,ax

; calculate scan line address in regen buffer for y1
	mov	ax,y1		;get y1 coordinate
	shl	ax,1		;mult by 2 to get offset of entry in table
	add	bx,ax		;add to table beginning offset
; set increment for x and y
	mov	ds:XStep[di],1	;default to forward
	mov	ds:YStep[di],1
	xor	dx,dx		;direction = 0
	mov	ax,x2		;get x2
	sub	ax,x1		;get delta x (x2 - x1)
	jns	DrLine020	;branch if result is positive
	neg	ax		;else take absolute value
	mov	ds:XStep[di],-1	;and remember to step backwards
	jmp	short DrLine030
DrLine020:
	jnz	DrLine030	;jump if delta x is not zero
	mov	dx,-1		;set direction = -1 if delta x = 0
DrLine030:
	mov	ds:DeltaX[di],ax
	mov	ax,y2		;get y2
	sub	ax,y1		;get delta y (y2 - y1)
	jns	DrLine040	;branch if result is positive
	neg	ax		;else take absolute value
	mov	ds:YStep[di],-1	;and remember to step backwards
DrLine040:
	mov	ds:DeltaY[di],ax
	mov	ax,x1		;get x1
	mov	cx,y1		;get y1
;;;
;;;  ax = x
;;;  bx = y (encoded as address in the Scan Line Table)
;;;  cx = y
;;;  dx = direction
;;;
	cmp	word ptr c,0 ;set to background color?
	je	DrLine080
DrLine050:
; loop drawing the points of the line
	cmp	ax,x2		;x = x2?
	jne	DrLine060	;no: continue with loop
	cmp	cx,y2		;y = y2?
	je	DrLineRet	;yes: finished!!!
DrLine060:
; draw a point at (x,y)
	push	cx		;save regs used
	push	ax
	mov	cl,3
	shr	ax,cl		;adjust ax for byte offset
	mov	si,ds:[bx]	;get scan line address
	add	si,ax		;get byte address in regen buffer
	pop	cx		;restore x value
	push	cx
	and	cx,0007h	;build bit mask
	mov	ah,80h
	shr	ah,cl
	xor	es:[si],ah	;turn on the bit
	pop	ax		;restore regs used
	pop	cx
; adjust for the next point
	cmp	dx,0		;direction less than zero?
	jge	DrLine070	;no: jump
	add	cx,ds:YStep[di]
	add	bx,ds:YStep[di]
	add	bx,ds:YStep[di]
	add	dx,ds:DeltaX[di]
	jmp	DrLine050
DrLine070:
	add	ax,ds:XStep[di]
	sub	dx,ds:DeltaY[di]
	jmp	DrLine050

DrLineRet:
	ret

DrLine080:
; loop setting points of the line to the background color
	cmp	ax,x2		;x = x2?
	jne	DrLine090	;no: continue with loop
	cmp	cx,y2		;y = y2?
	je	DrLineRet	;yes: finished!!!
DrLine090:
; draw a point at (x,y)
	push	cx		;save regs used
	push	ax
	mov	cl,3
	shr	ax,cl		;adjust ax for byte offset
	mov	si,ds:[bx]	;get scan line address
	add	si,ax		;get byte address in regen buffer
	pop	cx		;restore x value
	push	cx
	and	cx,0007h	;build bit mask
	mov	ah,80h
	shr	ah,cl
	xor	ah,0FFh
	and	es:[si],ah	;turn off the bit
	pop	ax		;restore regs used
	pop	cx
; adjust for the next point
	cmp	dx,0		;direction less than zero?
	jge	DrLine100	;no: jump
	add	cx,ds:YStep[di]
	add	bx,ds:YStep[di]
	add	bx,ds:YStep[di]
	add	dx,ds:DeltaX[di]
	jmp	DrLine080
DrLine100:
	add	ax,ds:XStep[di]
	sub	dx,ds:DeltaY[di]
	jmp	DrLine080

linehgc	ENDP
;
;   charhgc (x,y,c)  - draw char  c at x,y
;	x = x coordinate
;	y = y coordinate
;	c = character to draw
;
charhgc	PROC USES DI SI, x,y,c

	lea	bx,HGCRegen		;set up offset of regen scan line table
	mov	ax,HGCBase		;regen buffer segment
	mov	es,ax

; determine offset to character in character table
	mov	si,c		;get character
	and	si,007Fh	;strip invalid bits
	mov	cl,3		;mult by 8 to get offset into table
	shl	si,cl		;ds:si is addr of character in table
	lea	di,Crt_Char	;add in table base offset
	add	si,di

; adjust x coordinate for byte offset into regen buffer
	mov	dx,x		;get x coordinate
	add	dx,7		;adjust to next byte boundary if reqd
	mov	cl,3
	shr	dx,cl		;adjust for byte displacement into buffer
; get address in Regen Scan Line Table based on y coordinate
	mov	ax,y		;get y coordinate
	shl	ax,1		;mult by 2 for offset into scan line table
	add	bx,ax		;add to offset of table to get entry address
; loop 8 times to draw the character
	mov	cx,8
charhgc020:
	lodsb			;al = byte from DS:SI; incr SI
	mov	di,[bx]		;get offset into regen buffer
	add	di,dx		;adjust for x coordinate
	mov	es:[di],al	;store 8 pixels of char into regen buffer
	inc	bx		;incr for next scan line
	inc	bx
	loop	charhgc020
	ret

charhgc	ENDP

.data

gtable	db	35h,2dh,2eh,07h
	db	5bh,02h,57h,57h
	db	02h,03h,00h,00h

ttable	db	61h,50h,52h,0fh
	db	19h,06h,19h,19h
	db	02h,0dh,0bh,0ch

XStep	dw	0
YStep	dw	0
DeltaX	dw	0
DeltaY	dw	0

Crt_Char	db	000h,000h,000h,000h,000h,000h,000h,000h	; #0
		db	07Eh,081h,0A5h,081h,0BDh,099h,081h,07Eh	; #1
		db	07Eh,0FFh,0DBh,0FFh,0C3h,0E7h,0FFh,07Eh	; #2
		db	06Ch,0FEh,0FEh,0FEh,07Ch,038h,010h,000h	; #3
		db	010h,038h,07Ch,0FEh,07Ch,038h,010h,000h	; #4
		db	038h,07Ch,038h,0FEh,0FEh,07Ch,038h,07Ch	; #5
		db	010h,010h,038h,07Ch,0FEh,07Ch,038h,07Ch	; #6
		db	000h,000h,018h,03Ch,03Ch,018h,000h,000h	; #7
		db	0FFh,0FFh,0E7h,0C3h,0C3h,0E7h,0FFh,0FFh	; #8
		db	000h,03Ch,066h,042h,042h,066h,03Ch,000h	; #9
		db	0FFh,0C3h,099h,0BDh,0BDh,099h,0C3h,0FFh	; #10
		db	00Fh,007h,00Fh,07Dh,0CCh,0CCh,0CCh,078h	; #11
		db	03Ch,066h,066h,066h,03Ch,018h,07Eh,018h	; #12
		db	03Fh,033h,03Fh,030h,030h,070h,0F0h,0E0h	; #13
		db	07Fh,063h,07Fh,063h,063h,067h,0E6h,0C0h	; #14
		db	099h,05Ah,03Ch,0E7h,0E7h,03Ch,05Ah,099h	; #15
		db	080h,0E0h,0F8h,0FEh,0F8h,0E0h,080h,000h	; #16
		db	002h,00Eh,03Eh,0FEh,03Eh,00Eh,002h,000h	; #17
		db	018h,03Ch,07Eh,018h,018h,07Eh,03Ch,018h	; #18
		db	066h,066h,066h,066h,066h,000h,066h,000h	; #19
		db	07Fh,0dbh,0dbh,07Bh,01Bh,01Bh,01Bh,000h	; #20
		db	03Eh,063h,038h,06Ch,06Ch,038h,0CCh,078h	; #21
		db	000h,000h,000h,000h,07Eh,07Eh,07Eh,000h	; #22
		db	018h,03Ch,07Eh,018h,07Eh,03Ch,018h,0FFh	; #23
		db	018h,03Ch,07Eh,018h,018h,018h,018h,000h	; #24
		db	018h,018h,018h,018h,07Eh,03Ch,018h,000h	; #25
		db	000h,018h,00Ch,0FEh,00Ch,018h,000h,000h	; #26
		db	000h,030h,060h,0FEh,060h,030h,000h,000h	; #27
		db	000h,000h,0C0h,0C0h,0C0h,0FEh,000h,000h	; #28
		db	000h,024h,066h,0FFh,066h,024h,000h,000h	; #29
		db	000h,018h,03Ch,07Eh,0FFh,0FFh,000h,000h	; #30
		db	000h,0FFh,0FFh,07Eh,03Ch,018h,000h,000h	; #31
		db	000h,000h,000h,000h,000h,000h,000h,000h	; #32 ' '
		db	030h,078h,078h,030h,030h,000h,030h,000h	; #33 '!'
		db	06Ch,06Ch,06Ch,000h,000h,000h,000h,000h	; #34 '"'
		db	06Ch,06Ch,0FEh,06Ch,0FEh,06Ch,06Ch,000h	; #35 '#'
		db	030h,07Ch,0C0h,078h,00Ch,0F8h,030h,000h	; #36 '$'
		db	000h,0C6h,0CCh,018h,030h,066h,0C6h,000h	; #37 '%'
		db	038h,06Ch,038h,076h,0DCh,0CCh,076h,000h	; #38 '&'
		db	060h,060h,0C0h,000h,000h,000h,000h,000h	; #39 '''
		db	018h,030h,060h,060h,060h,030h,018h,000h	; #40 '('
		db	060h,030h,018h,018h,018h,030h,060h,000h	; #41 ')'
		db	000h,066h,03Ch,0FFh,03Ch,066h,000h,000h	; #42 '*'
		db	000h,030h,030h,0FCh,030h,030h,000h,000h	; #43 '+'
		db	000h,000h,000h,000h,000h,030h,030h,060h	; #44 ','
		db	000h,000h,000h,0FCh,000h,000h,000h,000h	; #45 '-'
		db	000h,000h,000h,000h,000h,030h,030h,000h	; #46 '.'
		db	006h,00Ch,018h,030h,060h,0C0h,080h,000h	; #47 '/'
		db	07Ch,0C6h,0CEh,0DEh,0F6h,0E6h,07Ch,000h	; #48 '0'
		db	030h,070h,030h,030h,030h,030h,0FCh,000h	; #49 '1'
		db	078h,0CCh,00Ch,038h,060h,0CCh,0FCh,000h	; #50 '2'
		db	078h,0CCh,00Ch,038h,00Ch,0CCh,078h,000h	; #51 '3'
		db	01Ch,03Ch,06Ch,0CCh,0FEh,00Ch,01Eh,000h	; #52 '4'
		db	0FCh,0C0h,0F8h,00Ch,00Ch,0CCh,078h,000h	; #53 '5'
		db	038h,060h,0C0h,0F8h,0CCh,0CCh,078h,000h	; #54 '6'
		db	0FCh,0CCh,00Ch,018h,030h,030h,030h,000h	; #55 '7'
		db	078h,0CCh,0CCh,078h,0CCh,0CCh,078h,000h	; #56 '8'
		db	078h,0CCh,0CCh,07Ch,00Ch,018h,070h,000h	; #57 '9'
		db	000h,030h,030h,000h,000h,030h,030h,000h	; #58 ':'
		db	000h,030h,030h,000h,000h,030h,030h,060h	; #59 ';'
		db	018h,030h,060h,0C0h,060h,030h,018h,000h	; #60 '<'
		db	000h,000h,0FCh,000h,000h,0FCh,000h,000h	; #61 '='
		db	060h,030h,018h,00Ch,018h,030h,060h,000h	; #62 '>'
		db	078h,0CCh,00Ch,018h,030h,000h,030h,000h	; #63 '?'
		db	07Ch,0C6h,0DEh,0DEh,0DEh,0C0h,078h,000h	; #64 '@'
		db	030h,078h,0CCh,0CCh,0FCh,0CCh,0CCh,000h	; #65 'A'
		db	0FCh,066h,066h,07Ch,066h,066h,0FCh,000h	; #66 'B'
		db	03Ch,066h,0C0h,0C0h,0C0h,066h,03Ch,000h	; #67 'C'
		db	0F8h,06Ch,066h,066h,066h,06Ch,0F8h,000h	; #68 'D'
		db	0FEh,062h,068h,078h,068h,062h,0FEh,000h	; #69 'E'
		db	0FEh,062h,068h,078h,068h,060h,0F0h,000h	; #70 'F'
		db	03Ch,066h,0C0h,0C0h,0CEh,066h,03Eh,000h	; #71 'G'
		db	0CCh,0CCh,0CCh,0FCh,0CCh,0CCh,0CCh,000h	; #72 'H'
		db	078h,030h,030h,030h,030h,030h,078h,000h	; #73 'I'
		db	01Eh,00Ch,00Ch,00Ch,0CCh,0CCh,078h,000h	; #74 'J'
		db	0E6h,066h,06Ch,078h,06Ch,066h,0E6h,000h	; #75 'K'
		db	0F0h,060h,060h,060h,062h,066h,0FEh,000h	; #76 'L'
		db	0C6h,0EEh,0FEh,0FEh,0D6h,0C6h,0C6h,000h	; #77 'M'
		db	0C6h,0E6h,0F6h,0DEh,0CEh,0C6h,0C6h,000h	; #78 'N'
		db	038h,06Ch,0C6h,0C6h,0C6h,06Ch,038h,000h	; #79 'O'
		db	0FCh,066h,066h,07Ch,060h,060h,0F0h,000h	; #80 'P'
		db	078h,0CCh,0CCh,0CCh,0DCh,078h,01Ch,000h	; #81 'Q'
		db	0FCh,066h,066h,07Ch,06Ch,066h,0E6h,000h	; #82 'R'
		db	078h,0CCh,0E0h,070h,01Ch,0CCh,078h,000h	; #83 'S'
		db	0FCh,0B4h,030h,030h,030h,030h,078h,000h	; #84 'T'
		db	0CCh,0CCh,0CCh,0CCh,0CCh,0CCh,0FCh,000h	; #85 'U'
		db	0CCh,0CCh,0CCh,0CCh,0CCh,078h,030h,000h	; #86 'V'
		db	0C6h,0C6h,0C6h,0D6h,0FEh,0EEh,0C6h,000h	; #87 'W'
		db	0C6h,0C6h,06Ch,038h,038h,06Ch,0C6h,000h	; #88 'X'
		db	0CCh,0CCh,0CCh,078h,030h,030h,078h,000h	; #89 'Y'
		db	0FEh,0C6h,08Ch,018h,032h,066h,0FEh,000h	; #90 'Z'
		db	078h,060h,060h,060h,060h,060h,078h,000h	; #91 '['
		db	0C0h,060h,030h,018h,00Ch,006h,002h,000h	; #92 '\'
		db	078h,018h,018h,018h,018h,018h,078h,000h	; #93 ']'
		db	010h,038h,06Ch,0C6h,000h,000h,000h,000h	; #94 '^'
		db	000h,000h,000h,000h,000h,000h,000h,0FFh	; #95 '_'
		db	030h,030h,018h,000h,000h,000h,000h,000h	; #96 '`'
		db	000h,000h,078h,00Ch,07Ch,0CCh,076h,000h	; #97 'a'
		db	0E0h,060h,060h,07Ch,066h,066h,0DCh,000h	; #98 'b'
		db	000h,000h,078h,0CCh,0C0h,0CCh,078h,000h	; #99 'c'
		db	01Ch,00Ch,00Ch,07Ch,0CCh,0CCh,076h,000h	; #100 'd'
		db	000h,000h,078h,0CCh,0FCh,0C0h,078h,000h	; #101 'e'
		db	038h,06Ch,060h,0F0h,060h,060h,0F0h,000h	; #102 'f'
		db	000h,000h,076h,0CCh,0CCh,07Ch,00Ch,0F8h	; #103 'g'
		db	0E0h,060h,06Ch,076h,066h,066h,0E6h,000h	; #104 'h'
		db	030h,000h,070h,030h,030h,030h,078h,000h	; #105 'i'
		db	00Ch,000h,00Ch,00Ch,00Ch,0CCh,0CCh,078h	; #106 'j'
		db	0E0h,060h,066h,06Ch,078h,06Ch,0E6h,000h	; #107 'k'
		db	070h,030h,030h,030h,030h,030h,078h,000h	; #108 'l'
		db	000h,000h,0CCh,0FEh,0FEh,0D6h,0C6h,000h	; #109 'm'
		db	000h,000h,0F8h,0CCh,0CCh,0CCh,0CCh,000h	; #110 'n'
		db	000h,000h,078h,0CCh,0CCh,0CCh,078h,000h	; #111 'o'
		db	000h,000h,0DCh,066h,066h,07Ch,060h,0F0h	; #112 'p'
		db	000h,000h,076h,0CCh,0CCh,07Ch,00Ch,01Eh	; #113 'q'
		db	000h,000h,0DCh,076h,066h,060h,0F0h,000h	; #114 'r'
		db	000h,000h,07Ch,0C0h,078h,00Ch,0F8h,000h	; #115 's'
		db	010h,030h,07Ch,030h,030h,034h,018h,000h	; #116 't'
		db	000h,000h,0CCh,0CCh,0CCh,0CCh,076h,000h	; #117 'u'
		db	000h,000h,0CCh,0CCh,0CCh,078h,030h,000h	; #118 'v'
		db	000h,000h,0C6h,0D6h,0FEh,0FEh,06Ch,000h	; #119 'w'
		db	000h,000h,0C6h,06Ch,038h,06Ch,0C6h,000h	; #120 'x'
		db	000h,000h,0CCh,0CCh,0CCh,07Ch,00Ch,0F8h	; #121 'y'
		db	000h,000h,0FCh,098h,030h,064h,0FCh,000h	; #122 'z'
		db	01Ch,030h,030h,0E0h,030h,030h,01Ch,000h	; #123 '{'
		db	018h,018h,018h,000h,018h,018h,018h,000h	; #124 '|'
		db	0E0h,030h,030h,01Ch,030h,030h,0E0h,000h	; #125 '}'
		db	076h,0DCh,000h,000h,000h,000h,000h,000h	; #126 '~'
		db	000h,010h,038h,06Ch,0C6h,0C6h,0FEh,000h	; #127

		;offsets into HGC regen buffer for each scan line
HGCRegen	dw	0,8192,16384,24576,90,8282,16474,24666
		dw	180,8372,16564,24756,270,8462,16654,24846
		dw	360,8552,16744,24936,450,8642,16834,25026
		dw	540,8732,16924,25116,630,8822,17014,25206
		dw	720,8912,17104,25296,810,9002,17194,25386
		dw	900,9092,17284,25476,990,9182,17374,25566
		dw	1080,9272,17464,25656,1170,9362,17554,25746
		dw	1260,9452,17644,25836,1350,9542,17734,25926
		dw	1440,9632,17824,26016,1530,9722,17914,26106
		dw	1620,9812,18004,26196,1710,9902,18094,26286
		dw	1800,9992,18184,26376,1890,10082,18274,26466
		dw	1980,10172,18364,26556,2070,10262,18454,26646
		dw	2160,10352,18544,26736,2250,10442,18634,26826
		dw	2340,10532,18724,26916,2430,10622,18814,27006
		dw	2520,10712,18904,27096,2610,10802,18994,27186
		dw	2700,10892,19084,27276,2790,10982,19174,27366
		dw	2880,11072,19264,27456,2970,11162,19354,27546
		dw	3060,11252,19444,27636,3150,11342,19534,27726
		dw	3240,11432,19624,27816,3330,11522,19714,27906
		dw	3420,11612,19804,27996,3510,11702,19894,28086
		dw	3600,11792,19984,28176,3690,11882,20074,28266
		dw	3780,11972,20164,28356,3870,12062,20254,28446
		dw	3960,12152,20344,28536,4050,12242,20434,28626
		dw	4140,12332,20524,28716,4230,12422,20614,28806
		dw	4320,12512,20704,28896,4410,12602,20794,28986
		dw	4500,12692,20884,29076,4590,12782,20974,29166
		dw	4680,12872,21064,29256,4770,12962,21154,29346
		dw	4860,13052,21244,29436,4950,13142,21334,29526
		dw	5040,13232,21424,29616,5130,13322,21514,29706
		dw	5220,13412,21604,29796,5310,13502,21694,29886
		dw	5400,13592,21784,29976,5490,13682,21874,30066
		dw	5580,13772,21964,30156,5670,13862,22054,30246
		dw	5760,13952,22144,30336,5850,14042,22234,30426
		dw	5940,14132,22324,30516,6030,14222,22414,30606
		dw	6120,14312,22504,30696,6210,14402,22594,30786
		dw	6300,14492,22684,30876,6390,14582,22774,30966
		dw	6480,14672,22864,31056,6570,14762,22954,31146
		dw	6660,14852,23044,31236,6750,14942,23134,31326
		dw	6840,15032,23224,31416,6930,15122,23314,31506
		dw	7020,15212,23404,31596,7110,15302,23494,31686
		dw	7200,15392,23584,31776,7290,15482,23674,31866
		dw	7380,15572,23764,31956,7470,15662,23854,32046	
		dw	7560,15752,23944,32136,7650,15842,24034,32226
		dw	7740,15932,24124,32316

	END
