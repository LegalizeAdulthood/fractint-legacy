;
;	Generic assembler routines that have very little at all
;	to do with fractals.

.MODEL	medium,c

.8086

.DATA

; ************************ External variables *****************************

	extrn	lx0:dword, ly0:dword	; arrays of (dword) increment values
	extrn	oktoprint: word		; flag: == 1 if printf() will work
	extrn	dotmode: word		; video mode:   1 = use the BIOS (yuck)
;							2 = use EGA/VGA style
;							3 = use MCGA style
;							4 = use Tseng style
;							5 = use Paradise 
;							6 = use Video 7
;							7 = MVGA 256 color
	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	ixmin:word, ixmax:word	; for zoom/pan: x and y limits
	extrn	iymin:word, iymax:word	;  for the local zoom-box
	extrn	cyclelimit:word		; limiting factor for DAC-cycler
	extrn	debugflag:word		; for debugging purposes only

	extrn	boxcount:word		; (previous) box pt counter: 0 if none.

; ************************ Public variables *****************************

public		cpu			; used by 'calcmand'
public		andcolor		; used by 'calcmand'
public		lookatmouse		; used by 'calcfrac'

public		dacbox			; GIF saves use this
public		daclearn, daccount	; Rotate may want to use this
public		extraseg		; extra 64K segment, if any

;		arrays declared here, used elsewhere
;		arrays not used simultaneously are deliberately overlapped

public		prefix, suffix, stack		; Used by the Decoder
public		strlocn, entrynum, teststring	; used by the Encoder

; ************************ Internal variables *****************************

cpu		dw	0		; cpu type: 86, 186, 286, or 386

dotwrite	dw	0		; write-a-dot routine:  mode-specific
dotread		dw	0		; read-a-dot routine:   mode-specific
linewrite	dw	0		; write-a-line routine: mode-specific
andcolor	dw	0		; "and" value used for color selection
color		db	0		; the color to set a pixel

;					; Zoom-Box values (2K x 2K screens max)
step		dw	0		; Zoom-Box drawing step-size
boxcolor	db	0		; Zoom-Box color

strlocn		dw	0		; 8K Encoder array
prefix		dw	0		; 8K Decoder array
boxx		dw	1028 dup(0)	; (previous) box data points - x axis
boxy		dw	1028 dup(0)	; (previous) box data points - y axis
boxvalues	db	1028 dup(0)	; (previous) box color values
		db	3072 dup(0)	; pad up to 8K

entrynum	dw	0		; 8K Encoder array
suffix		dw	0		; 4K Decoder array
		dw	2048 dup(0)	; fluff up to 4K
stack		dw	0		; 4K Decoder array
		dw	2048 dup(0)	; fluff up to 8K total
teststring	dw	0		; 100 byte Encoder array
		dw	50 dup (0)	; fluff up to 100 bytes

daclearn	db	0		; 0 if "learning" DAC speed
dacnorm		db	0		; 0 if "normal" DAC update
daccount	dw	0		; DAC registers to update in 1 pass
dacbox		db	773 dup(0)	; DAC goes here

rowcount	dw	0		; row-counter for decoder and out_line

kbd_type	db	0		; type of keyboard
keybuffer	dw	0		; real small keyboard buffer

extraseg	dw	0		; extra 64K segment (allocated by init)

andvideo	db	0		; "and" value for setvideo
videoax		dw	0		; graphics mode values: ax
videobx		dw	0		; graphics mode values: bx
videocx		dw	0		; graphics mode values: cx
videodx		dw	0		; graphics mode values: dx

; ************************* Super-VGA Variables ***************************

current_bank	db	0ffh		; keeps track of current bank set...
					; anything that changes bank without
					; updating this might make a problem

; ********************** Mouse Support Variables **************************

mouse		db	0		; == -1 if/when a mouse is found.
mousekey	db	0		; status of mouse keys (up, down)
mousemickeys	dw	0		; running mickey counter
lookatmouse	dw	0		; if 0, ignore non-button mouse mvment

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

x360y480	db	360/8			; number of screen columns
		db	480/16			; number of screen rows
		db	 6bh, 59h, 5ah, 8eh	; CRTC Registers
		db	 5eh, 8ah, 0DH,03Eh
		db	  0h, 40h, 00h,  0h
		db	  0h,  0h,  0h, 31h
		db	0EAh, 0ACh, 0DFh, 2Dh
		db	  0h,0E7h, 06h,0E3h
		db	0FFh

tweaks		dw	offset x704y528		; tweak table
		dw	offset x704y528
		dw	offset x720y540
		dw	offset x736y552
		dw	offset x752y564
		dw	offset x768y576
		dw	offset x784y588
		dw	offset x800y600
		dw	offset x360y480

.CODE

;	Routines called by this code

extrn	help:far

; **************** Routines called by 'calcmand.asm' *********************
;
;	parameters passed directly in registers - just call near routine

asmdotwrite	proc	far		; called by 'calcmand.asm'
		call	dotwrite	; call the appropriate write-a-dot
		ret			; we done
asmdotwrite	endp

asmvideocleanup	proc	far		; called by 'calcmand.asm'
		call	videocleanup	; call the local routine
		ret			; we done.
asmvideocleanup	endp

; **************** internal Read/Write-a-dot routines ***********************
;
;	These Routines all assume the following register values:
;
;		AL = The Color (returned on reads, sent on writes)
;		CX = The X-Location of the Pixel
;		DX = The Y-Location of the Pixel

normalwrite	proc	near		; generic write-a-dot routine
	mov	ah,12			; write the dot (al == color)
	mov	bx,0			; this page
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
normalwrite	endp

normalread	proc	near		; generic read-a-dot routine
	mov	ah,13			; read the dot (al == color)
	mov	bx,0			; this page
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
normalread	endp

mcgawrite	proc	near		; MCGA 320*200, 246 colors
	push	ax			; save this for a tad
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	mov	bx,ax			; save this in BX
	pop	ax			; restore AX
	mov	es:[bx],al		; write the dot
	ret				; we done.
mcgawrite	endp

mcgaread	proc	near		; MCGA 320*200, 246 colors
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	mov	bx,ax			; save this in BX
	mov	al,es:[bx]		; retrieve the previous value
	ret				; we done.
mcgaread	endp

vgawrite	proc	near		; EGA/VGA write mode 0
	mov	bh,al			; save the color value for a bit
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	adc	dx,0			; DX:AX now holds the pixel count
	mov	cx,ax			; save this for the bit mask
	and	cx,7			; bit-mask shift calculation
	xor	cl,7			;  ...
	mov	si,ax			; set up for the address shift
	shr	dx,1			; (ugly) 32-bit shift-by-3 logic
	rcr	si,1			;  ((works on ANY 80x6 processor))
	shr	dx,1			;  ...
	rcr	si,1			;  ...
	shr	dx,1			;  ...
	rcr	si,1			;  ...
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
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	adc	dx,0			; DX:AX now holds the pixel count
	mov	cx,ax			; save this for the bit mask
	and	cx,7			; bit-mask shift calculation
	xor	cl,7			;  ...
	mov	si,ax			; set up for the address shift
	shr	dx,1			; (ugly) 32-bit shift-by-3 logic
	rcr	si,1			;  ((works on ANY 80x6 processor))
	shr	dx,1			;  ...
	rcr	si,1			;  ...
	shr	dx,1			;  ...
	rcr	si,1			;  ...
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


;	The 360x480 mode draws heavily on Michael Abrash's article in
;	the January/February 1989 "Programmer's Journal" and files uploaded
;	to Compuserv's PICS forum by Dr. Lawrence Gozum - integrated here
;	by Timothy Wegner

; Michael Abrash equates. Not all used, but I'll leave for reference.

VGA_SEGMENT       EQU   0A000h
SC_INDEX          EQU   3C4h     ;Sequence Controller Index register
GC_INDEX          EQU   3CEh     ;Graphics Controller Index register
CRTC_INDEX        EQU   3D4h     ;CRT Controller Index register
MAP_MASK          EQU   2        ;Map Mask register index in SC
MEMORY_MODE       EQU   4        ;Memory Mode register in SC
MAX_SCAN_LINE     EQU   9        ;Maximum Scan Line reg index in CRTC
                                 ;Use 9 for 2 pages of 320x400
;MAX_SCAN_LINE    EQU   1        ;Use 1 for 4 pages of 320x200
START_ADD_HIGH    EQU   0Ch      ;Start Address High reg index in CRTC
UNDERLINE         EQU   14h      ;Underline Location reg index in CRTC
MODE_CONTROL      EQU   17h      ;Mode Control reg index in CRTC
READ_MAP          EQU   4        ;Read Mask register index in SC
GRAPHICS_MODE     EQU   5        ;Graphics Mode register index in SC
MISC              EQU   6        ;Miscellaneous register index in SC
WORD_OUTS_OK      EQU   1        ;set to 0 to assemble for computers
                                 ;that can't handle word outs to indexed
                                 ;VGA registers
;
;Macro to output a word value to a port
;
OUT_WORD MACRO
IF WORD_OUTS_OK
         OUT      DX,AX
ELSE
         OUT      DX,AL
         INC      DX
         XCHG     AH,AL
         OUT      DX,AL
         DEC      DX
         XCHG     AH,AL
ENDIF
         ENDM

;Macro to ouput a constant value to an indexed VGA register
CONSTANT_TO_INDEXED_REGISTER     MACRO  ADDRESS,INDEX,VALUE
         MOV      DX,ADDRESS
         MOV      AX,(VALUE SHL 8)+INDEX
         OUT_WORD
         ENDM

tweak256read	proc near uses si 	; Tweaked-VGA ...x256 color mode

  mov     ax,90
  mul     dx                     ;Point to start of desired row
  push    cx                     ;Save X coordinate for later
  shr     cx,1                   ;There are 4 pixels at each address
  shr     cx,1                   ;so divide X by 4
  add     ax,cx                  ;Point to pixels address
  mov     si,ax
  pop     ax                     ;Retrieve X coordinate
  and     al,3                   ;Get the plane number of the pixel
  mov     ah,al
  mov     al,READ_MAP            
  mov     dx,GC_INDEX
  OUT_WORD                       ;Set to write to the proper plane for the
                                 ;pixel
  xor     ax,ax     
  lods	  byte ptr es:[si]       ;Read the pixel
  ret 

tweak256read	endp

tweak256write	proc near uses di	; Tweaked-VGA ...x256 color mode
  mov     bl,al			; color
  mov     ax,90      		;There are 4 pixels at each address so each
                                ;360-pixel row is 90 bytes wide in each plane
  mul     dx                  	;Point to start of desired row
  push    cx                    ;Save X coordinate for later
  shr     cx,1                  ;There are 4 pixels at each address
  shr     cx,1                  ;so divide X by 4
  add     ax,cx                 ;Point to pixels address
  mov     di,AX
  pop     cx                    ;Retrieve X coordinate
  and     cl,3                  ;Get the plane number of the pixel
  mov     ah,1
  shl     ah,CL                 ;Set the bit corresponding to the plane
                                ;the pixel is in
  mov     al,MAP_MASK            
  mov     dx,SC_INDEX
  OUT_WORD                       ;Set to write to the proper plane for the
                                 ;pixel
  mov     es:[di],bl             ;Draw the pixel

  ret 
tweak256write	endp

tweak400read	proc near		; Tweaked-VGA ...x400 color mode
;  MVGA "Meduim-resolution VGA" ?
        call	normalread
		ret
tweak400read	endp

tweak400write	proc near		; Tweaked-VGA ...x400 color mode
	call	normalwrite		; let the BIOS do it.
	ret
tweak400write	endp
;
;	The following 'Super256' code is courtesy of Timothy Wegner
;		and John Bridges (who wrote 'newbank')

;			Thanks, Guys!

; Super256read/write has been tested with Tseng (Genoa) at 640x480x256
; it should work with any 256 color mode on Tseng, Paradise,
; and video 7 

super256write	proc near		; super-VGA ...x256 colors write-a-dot
        call    super256addr		; calculate address and switch banks
	mov	es:[bx],al		; write the dot
	ret				; we done.
super256write	endp

super256read	proc near		; super-VGA ...x256 colors read-a-dot
        call	super256addr		; calculate address and switch banks
	mov	al,es:[bx]		; read the dot
	ret				; we done.
super256read	endp

super256addr	proc near		; can be put in-line but shared by
					; read and write routines
        clc				; clear carry flag
	push	ax			; save this for a tad
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines - ans in dx:ax
	add	ax,cx			; plus this many x-dots
        adc     dx,0			; answer in dx:ax - dl=bank, ax=offset
	mov	bx,ax			; save this in BX
        cmp	dl,current_bank		; see if bank changed
        je	same_bank		; jump if old bank ok
        mov	al,dl			; newbank expects bank in al
        call    newbank
same_bank:
	pop	ax			; restore AX
        ret
super256addr	endp

; modified newbank.asm from video adapter DL of PICS
; expects dotmode = 4,5,6 for Tseng, Paradise, Video 7
; Original author is John Bridges - integrated by Timothy Wegner

newbank	proc	near		;64k bank number is in AX
	push	ax
	push	dx
	cli
	mov	current_bank,al
	cmp	dotmode,4
	jne	nots
	and	al,7		;Tseng
	mov	ah,al
	shl	ah,1
	shl	ah,1
	shl	ah,1
	or	al,ah
	or	al,01000000b
	mov	dx,3cdh
	out	dx,al
	sti
	pop	dx
	pop	ax
	ret

nots:	cmp	dotmode,6
	jne	nov7
	and	ax,15		;Video 7
	push	cx
	mov	ch,al
	mov	dx,3c4h
	mov	ax,0ea06h
	out	dx,ax
	mov	ah,ch
	and	ah,1
	mov	al,0f9h
	out	dx,ax
	mov	al,ch
	and	al,1100b
	mov	ah,al
	shr	ah,1
	shr	ah,1
	or	ah,al
	mov	al,0f6h
	out	dx,al
	inc	dx
	in	al,dx
	dec	dx
	and	al,not 1111b
	or	ah,al
	mov	al,0f6h
	out	dx,ax
	mov	ah,ch
	mov	cl,4
	shl	ah,cl
	and	ah,100000b
	mov	dl,0cch
	in	al,dx
	mov	dl,0c2h
	and	al,not 100000b
	or	al,ah
	out	dx,al
	sti
	pop	cx
	pop	dx
	pop	ax
	ret
	
nov7:	cmp	dotmode,5
	jne	nopd
	mov	dx,3ceh		;Paradise
	mov	ax,50fh		;turn off write protect on VGA registers
	out	dx,ax
	mov	ah,current_bank
	shl	ah,1
	shl	ah,1
	shl	ah,1
	shl	ah,1
	mov	al,9
	out	dx,ax

nopd:	sti
	pop	dx
	pop	ax
	ret
newbank	endp

; **************** internal Write-a-line routines ***********************
;
;	These Routines all assume the following register values:
;
;		SI = Offset of array of colors for a row

normaline	proc	near		; Normal Line:  no assumptions
	xor	cx,cx			; zero the counter	
normal_line1:
	mov	dx,rowcount		; set the y-pixel
        mov     bx,si			; locate the actual pixel color
	add	bx,cx			;  ...
	mov	al,[bx]			; retrieve the color
	push	cx			; save the counter around the call
	push	si			; save the pointer around the call also
	call	dotwrite		; write the dot via the approved method
	pop	si     			; restore the pointer
	pop	cx     			; restore the counter
	inc	cx			; bump it up
	cmp	cx,xdots		; more to go?
	jl	normal_line1		; yup.  do it.
        ret
normaline	endp

mcgaline	proc	near		; MCGA 320*200, 246 colors
	mov	ax,xdots		; this many dots / line
	mul	rowcount		; times this many lines
        mov	di,ax			; di = offset of row in video memory
        mov	cx,xdots		; move this many bytes
	shr	cx,1			; convert bytes to words
	rep	movsw			; zap line into memory
        ret
mcgaline	endp

vgaline		proc	near		; EGA/VGA line write
	mov	ax,xdots		; compute # of dots / pass
	shr	ax,1			;  (given 8 passes)
	shr	ax,1			;  ...
	shr	ax,1			;  ...
	mov	cx,ax			; save the dots / pass here
	mul	rowcount		; now calc first video addr
	mov	di,ax			; save the starting addr here
	mov	dx,03ceh		; set up graphics cntrlr addr
	mov	ax,8008h		; set up for the bit mask
vgaline1:
	out	dx,ax			; set the graphics bit mask
	push	ax			; save registers for a tad
	push	cx			;  ...
	push	si			;  ...
	push	di			;  ...
vgaline2:
	mov	ah,ds:[si]		; get the color
	mov	al,0			; set set/reset registers
	out	dx,ax			;  do it.
	mov	ax,0f01h		; enable set/reset registers
	out	dx,ax			;  do it.
	or	es:[di],al		; update all bit planes
	inc	di			; set up the next video addr
	add	si,8			;  and the next source addr
	loop	vgaline2		; loop if more dots this pass
	pop	di			; restore the saved registers
	pop	si			;  ...
	pop	cx			;  ...
	pop	ax			;  ...
	inc	si			; offset the source 1 byte
	ror	ah,1			; alter bit mask value
	cmp	ah,80h			; already done all 8 of them?
	jne	vgaline1		;  nope.  do another one.
	call	videocleanup		; else cleanup time.
	ret				;  and we done.
vgaline		endp

super256line    proc    near     ; super VGA 256 colors
        mov     ax,xdots         ; this many dots / line
        mov     cx,rowcount      ; cx = row
        mul     cx               ; times this many lines
        push    ax               ; save pixel address for later
        cmp     dl,current_bank  ; bank ok?
        push    dx               ; save bank
        je      bank_is_ok       ; jump if bank ok
        mov     al,dl            ; newbank needs bank in al
        call    newbank
bank_is_ok:
        inc     cx               ; next row
        mov     ax,xdots         ; this many dots / line
        mul     cx               ; times this many lines
	sub	ax,1		 ; back up some to the last pixel of the 
	sbb	dx,0		 ; previous line
        pop     cx               ; bank at start of row
        cmp     dl,cl            ; did bank change?
        pop     di               ; di = offset of row in video memory
        jne     bank_did_chg
        mov     cx,xdots         ; move this many bytes
	shr	cx,1		; convert bytes to words
        rep     movsw            ; zap line into memory
        jmp     short linedone
bank_did_chg:
        call    normaline        ; normaline can handle bank change
linedone:
        ret
super256line    endp

tweak256line	proc	near		; Normal Line:  no assumptions
	mov	cx,90			; number of pixels / pass
	mov	ax,cx			; calculate the first video address
	mul	rowcount		;  (pixel 0 on this line)
	mov	di,ax			; save the first address here
        mov     al,MAP_MASK		; set up for the bit plane adjustment
	mov	ah,1			;  (first plane first)
        mov     dx,SC_INDEX		;  ...
tweak256line1:
	OUT_WORD			; set up the bit plane
	push	ax			; save a few registers around the loop
	push	cx			;  ...
	push	si			;  ...
	push	di			;  ...
tweak256line2:
	movsb				; move the next pixel
	add	si,3			; adjust the source addr (+4, not +1)
	loop	tweak256line2		; loop if more dots this pass
	pop	di			; restore the saved registers
	pop	si			;  ...
	pop	cx			;  ...
	pop	ax			;  ...
	inc	si			; offset the source 1 byte
	shl	ah,1			; set up for the next video plane
	cmp	ah,16			; there IS a next plane, isn't there?
	jne	tweak256line1		;  yup.  perform another loop.
        ret
tweak256line	endp

; ******************** Function videocleanup() **************************

;	Called at the end of any assembler video read/writes to make
;	the world safe for 'printf()'s.
;	Currently, only ega/vga needs cleanup work, but who knows?

videocleanup	proc	near
	mov	ax,dotwrite		; check: were we in EGA/VGA mode?
	cmp	ax,offset vgawrite	;  ...
	jne	short videocleanupdone	; nope.  no adjustments
	mov	dx,03ceh		; graphics controller address
	mov	ax,0ff08h		; restore the default bit mask
	out	dx,ax			; ...
	mov	ax,0003h		; restore the function select
	out	dx,ax			;  ...
	mov	ax,0001h		; restore the enable set/reset
	out	dx,ax			;  ...
videocleanupdone:
	ret
videocleanup	endp


; ******************** Function drawbox(newbox) **************************

;	if newbox == 0, just erase old box.  Else erase old and draw new.

drawbox	proc	uses di si es, newbox:word

	mov	step,1			; default step:  every pixel
	mov	ax,ixmax		; just how big is this zoom-box?
	sub	ax,ixmin		;  this many dots,...
	shl	ax,1			; an eighth of the screen or less?
	shl	ax,1			;  ...
	shl	ax,1			;  ...
	cmp	ax,xdots		;  ...
	jb	short solidbox		;  yup.  keep the box solid.
	mov	step,2			; nope.  make the box every other pixel
solidbox:
	shr	ax,1			; a quarter of the screen or less?
	cmp	ax,xdots		;  ...
	jb	short solidbox2		;  yup.  keep the box (semi) solid.
	mov	step,4			; nope.  make the box every 4th pixel
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
	shl	bx,1			; switch to a word pointer
	mov	cx,boxx[bx]		; get the (previous) point location
	mov	dx,boxy[bx]		;  ...
	shr	bx,1			; switch back to character pointer
	mov	al,boxvalues[bx]	; get the (previous) color
	push	bx			; save the counter
	call	dotwrite		; adjust the dot.
	pop	bx			; restore the counter
	dec	bx			; are we done yet?
	jns	eraseoldbox		;  nope.  try again.
calcnewbox:
	mov	boxcount,0		; set counter flag: no dots yet
	cmp	newbox,0		; should we draw a new box?
	jne	short calcnewbox2	; yup.
	jmp	endofdrawbox		; nope.  bail out now.
calcnewbox2:
	mov	bx,boxcount		; get set to draw lines
	shl	bx,1			; switch to word pointers
starttop:
	mov	cx,ixmin		; now, draw the top line.
	mov	dx,iymin		;  ...
topline:
	mov	boxx[bx],cx		; save this point.
	mov	boxy[bx],dx		;  ...
	add	bx,2			; bump up the pointer offsets
	inc	boxcount		; and counters
	add	cx,step			; calculate the next dot address
	cmp	cx,ixmax		; gone past the end-of-line?
	jbe	short topline		;  nope.  try again.
startbottom:
	mov	cx,ixmin		; now, draw the bottom line.
	mov	dx,iymax		; ...
bottomline:
	mov	boxx[bx],cx		; save this point.
	mov	boxy[bx],dx		;  ...
	add	bx,2			; bump up the pointer offsets
	inc	boxcount		; and counters
	add	cx,step			; calculate the next dot address
	cmp	cx,ixmax		; gone past the end-of-line?
	jbe	short bottomline	;  nope.  try again.
startleft:
	mov	cx,ixmin		; now, draw the left line.
	mov	dx,iymin		; ...
leftline:
	mov	boxx[bx],cx		; save this point.
	mov	boxy[bx],dx		;  ...
	add	bx,2			; bump up the pointer offsets
	inc	boxcount		; and counters
	add	dx,step			; calculate the next dot address
	cmp	dx,iymax		; gone past the end-of-line?
	jbe	short leftline		;  nope.  try again.
startright:
	mov	cx,ixmax		; now, draw the right line.
	mov	dx,iymin		;  ...
rightline:
	mov	boxx[bx],cx		; save this point.
	mov	boxy[bx],dx		;  ...
	add	bx,2			; bump up the pointer offsets
	inc	boxcount		; and counters
	add	dx,step			; calculate the next dot address
	cmp	dx,iymax		; gone past the end-of-line?
	jbe	short rightline		;  nope.  try again.
endlines:
	mov	bx,boxcount		; load up a counter: # points to draw
	dec	bx			; switch to an offset
readnewbox:
	shl	bx,1			; switch to word counter
	mov	cx,boxx[bx]		; get the (new) point location
	mov	dx,boxy[bx]		;  ...
	shr	bx,1			; switch back to character counter
	push	bx			; save the counter
	call	dotread			; read the (previous) dot value
	pop	bx			; restore the counter
	mov	boxvalues[bx],al	; get the (previous) color
	dec	bx			; are we done yet?
	jns	readnewbox		;  nope.  try again.
	mov	bx,boxcount		; load up a counter: # points to draw
	dec	bx			; switch to an offset
drawnewbox:
	shl	bx,1			; switch to word counter
	mov	cx,boxx[bx]		; get the (new) point location
	mov	dx,boxy[bx]		;  ...
	shr	bx,1			; switch back to character counter
	push	bx			; save the counter
	mov	al,boxcolor		; set the (new) box color
	call	dotwrite		; adjust the dot.
	pop	bx			; restore the counter
	dec	bx			; are we done yet?
	jns	drawnewbox		;  nope.  try again.

endofdrawbox:
	call	videocleanup		; perform any video cleanup required
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

setvideomode	proc	uses di si es,argax:word,argbx:word,argcx:word,argdx:word

	mov	ax,argax		; load up for the interrupt call
	mov	bx,argbx		;  ...
	mov	cx,argcx		;  ...
	mov	dx,argdx		;  ...

	mov	videoax,ax		; save the values for future use
	mov	videobx,bx		;  ...
	mov	videocx,cx		;  ...
	mov	videodx,dx		;  ...

	call	setvideo		; call the internal routine first
					; prepare special video-mode speedups
	mov	oktoprint,1		; say it's OK to use printf()
	cmp	dotmode,3		; MCGA mode?
	je	short mcgamode		; yup.
	cmp	dotmode,2		; EGA/VGA mode?
	je	short vgamode		; yup.
	cmp	dotmode,4		; Super-VGA ...x256 color mode?
	je	short super256mode	; yup.
	cmp	dotmode,5		; Super-VGA ...x256 color mode?
	je	short super256mode	; yup.
	cmp	dotmode,6		; Super-VGA ...x256 color mode?
	je	short super256mode	; yup.
	cmp	dotmode,7		; Tweaked-VGA ...x256 color mode?
	je	short tweak256mode	; yup
	cmp	dotmode,8		; Tweaked-Super-VGA...x256 color mode?
	je	short tweak400mode	; yup
dullnormalmode:
	mov	ax,offset normalwrite	; set up the BIOS write-a-dot routine
	mov	bx,offset normalread	; set up the BIOS read-a-dot  routine
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	short videomode		; return to common code
mcgamode:
	mov	ax,offset mcgawrite	; set up MCGA write-a-dot routine
	mov	bx,offset mcgaread	; set up MCGA read-a-dot  routine
	mov	cx,offset mcgaline 	; set up the MCGA linewrite routine
	jmp	short videomode		; return to common code
super256mode:
	mov	ax,offset super256write	; set up superVGA write-a-dot routine
	mov	bx,offset super256read	; set up superVGA read-a-dot  routine
	mov	cx,offset super256line 	; set up the " linewrite routine
	jmp	short videomode		; return to common code
tweak256mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak256write	; set up tweaked-256 write-a-dot
	mov	bx,offset tweak256read	; set up tweaked-256 read-a-dot
	mov	cx,offset tweak256line 	; set up the normal linewrite routine
	jmp	short videomode		; return to common code
tweak400mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak400write	; set up tweaked-400 write-a-dot
	mov	bx,offset tweak400read	; set up tweaked-400 read-a-dot
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	short videomode		; return to common code
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up EGA/VGA read-a-dot  routine
	mov	cx,offset vgaline 	; set up the EGA/VGA linewrite routine
	jmp	short videomode		; return to common code
videomode:
	mov	dotwrite,ax		; save the results
	mov	dotread,bx		;  ...
	mov	linewrite,cx		;  ...

	mov	ax,colors		; calculate the "and" value
	dec	ax			; to use for eventual color 
	mov	andcolor,ax		; selection

	mov	boxcount,0		; clear the zoom-box counter

	mov	daclearn,0		; set the DAC rotates to learn mode
	mov	daccount,6		; initialize the DAC counter
	cmp	cpu,88			; say, are we on a 186/286/386?
	jbe	setvideoslow		;  boo!  hiss!
	mov	daclearn,1		; yup.  bypass learn mode
	mov	ax,cyclelimit		;  and go as fast as he wants
	mov	daccount,ax		;  ...
setvideoslow:

	call	far ptr loaddac		; load the video dac, if we can

	call	initasmvars		; finally, check out the other hardware

	ret
setvideomode	endp

setvideo	proc	near		; local set-video more

	cmp	ax,0			; TWEAK?:  look for AX==BX==CX==0
	jne	short setvideobios	;  ...
	cmp	bx,0			;  ...
	jne	short setvideobios	;  ...
	cmp	cx,0			;  ...
	je	short setvideoregs	;  ...

setvideobios:
	or	al,andvideo		; this may preserve the video
	push	bp			; some BIOS's don't save this
	int	10h			; do it via the BIOS.
	pop	bp			; restore the saved register
	jmp	setvideoreturn		;  and return.

setvideoregs:				; assume genuine VGA and program regs

	mov	si,dx			; get the video table offset
	shl	si,1			;  ...
	mov	si,word ptr tweaks[si]	;  ...

	mov	ax,0012h		; invoke video mode 12h
	or	al,andvideo		; this may preserve the video
	int	10h			; do it.

        cmp	dx,8			; tweak256 mode?
        jne	not256			; if not, start tweak256-specific code

	mov	ax,0013h		; invoke video mode 13h
	or	al,andvideo		; this may preserve the video
	int	10h			; previous mode 12h cleared video
					;   memory - dirty but effective
	mov	dx,3c4h			; alter sequencer registers
	mov	ax,0604h		; disable chain 4
	out	dx,ax
        
not256:	mov	ax,1124h		; load ROM 8*16 characters
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

	mov	current_bank,0ffh	; stuff impossible value into cur-bank

	mov	rowcount,0		; clear row-counter for decoder

	mov	andvideo,0		; reset the video to clobber memory

	ret
setvideo	endp

; ********* Functions setfortext() and setforgraphics() ************

;	setfortext() resets the video for text mode and saves graphics data
;	setforgraphics() restores the graphics mode and data
;	setclear() clears the screen after setfortext() [which may be wierd]

setfortext	proc	uses es si di
	cmp	videoax,0		; check for CGA modes
	je	setfortextnocga		;  not this one
	cmp	videoax,7		;  ...
	ja	setfortextnocga		;  not this one
setfortextcga:
	mov	ax,extraseg		; set ES == Extra Segment
	mov	es,ax			;  ...
	mov	di,0c000h		; save the video data here
	mov	ax,0b800h		; video data starts here
	push	ds			; save DS for a tad
	cld				; clear the direction flag
	mov	ds,ax			;  reset DS
	mov	si,0			;  ...
	mov	cx,8192			; save this many words
	rep	movsw			;  save them.
	pop	ds			; restore DS
	mov	ax,3			; set up the text call
	mov	bx,0			;  ...
	mov	cx,0			;  ...
	mov	dx,0			;  ...
	call	setvideo		; set the video
	jmp	setfortextreturn
setfortextnocga:	
	mov	ax,0			; disable the video (I think)
	call	disablevideo		;  ...
	mov	andvideo,80h		; set the video to preserve memory
	mov	ax,6			; set up the text call
	mov	bx,0			;  ...
	mov	cx,0			;  ...
	mov	dx,0			;  ...
	call	setvideo		; set the video
	mov	ax,0			; disable the video (I think)
	call	disablevideo		;  ...
	cld				; clear the direction flag
	mov	ax,extraseg		; set ES == Extra Segment
	mov	es,ax			;  ...
	mov	di,0c000h		; save the video data here
	mov	ax,0b800h		; video data starts here
	push	ds			; save DS for a tad
	mov	ds,ax			;  reset DS
	mov	si,0			;  ...
	mov	cx,8192			; save this many words
	rep	movsw			;  save them.
	pop	ds			; restore DS
	mov	ax,0b800h		; clear the video buffer
	mov	es,ax			;  ...
	mov	di,0			;  ...
	mov	ax,0			; to blanks
	mov	cx,8192			; this many blanks
	rep	stosw			; do it.
	mov	ax,20h			; enable the video (I think)
	call	disablevideo		;  ...
setfortextreturn:
	call	far ptr home		; home the cursor
	ret
setfortext	endp

setforgraphics	proc	uses es si di
	cmp	videoax,0		; check for CGA modes
	je	setforgraphicsnocga	;  not this one
	cmp	videoax,7		;  ...
	ja	setforgraphicsnocga	;  not this one
setforgraphicscga:
	mov	ax,videoax		; set up the video call
	mov	bx,videobx		;  ...
	mov	cx,videocx		;  ...
	mov	dx,videodx		;  ...
	call	setvideo		; do it.
	cld				; clear the direction flag
	mov	ax,0b800h		; restore the video area
	mov	es,ax			; ES == video addr
	mov	di,0			;  ...
	push	ds			; reset DS to Extraseg temporarily
	mov	ax,extraseg		;  ...
	mov	ds,ax			;  ...
	mov	si,0c000h		;  ...
	mov	cx,8192			; restore this many words
	rep	movsw			; restore them.
	pop	ds			; restore DS
	jmp	setforgraphicsreturn
setforgraphicsnocga:
	mov	ax,0			; disable the video (I think)
	call	disablevideo		;  ...
	cld				; clear the direction flag
	mov	ax,0b800h		; restore the video area
	mov	es,ax			; ES == video addr
	mov	di,0			;  ...
	push	ds			; reset DS to Extraseg temporarily
	mov	ax,extraseg		;  ...
	mov	ds,ax			;  ...
	mov	si,0c000h		;  ...
	mov	cx,8192			; restore this many words
	rep	movsw			; restore them.
	pop	ds			; restore DS
	mov	andvideo,80h		; set the video to preserve memory
	mov	ax,videoax		; set up the video call
	mov	bx,videobx		;  ...
	mov	cx,videocx		;  ...
	mov	dx,videodx		;  ...
	call	setvideo		; do it.
	mov	ax,20h			; enable the video (I think)
	call	disablevideo		;  ...
setforgraphicsreturn:
	mov	ax,1			; set up call to spindac(0,1)
	push	ax			;  ...
	mov	ax,0			;  ...
	push	ax			;  ...
	call	far ptr spindac		; do it.
	pop	ax			; restore the registers
	pop	ax			;  ...
	ret
setforgraphics	endp

setclear	proc	uses es si di	; clear the screen after setfortext
	cmp	videoax,0		; check for CGA modes
	je	setclearnocga		;  not this one
	cmp	videoax,7		;  ...
	ja	setclearnocga		;  not this one
setclearcga:
	call	far ptr clscr		; cga mode; normal clear
	jmp	setclearreturn		; we done.
setclearnocga:
	mov	ax,0b800h		; clear the video buffer
	mov	es,ax			;  ...
	mov	di,0			;  ...
	mov	ax,0			; to blanks
	mov	cx,8192			; this many blanks
	rep	stosw			; do it.
setclearreturn:
	ret
setclear	endp

disablevideo	proc	near		; wierd video trick to disable/enable
	push	dx			; save some registers
	push	ax			;  ...
	mov	dx,03bah		; set attribute comtroller flip-flop
	in	al,dx			;  regardless of video mode
	mov	dx,03dah		;  ...
	in	al,dx			;  ...
	mov	dx,03c0h		; attribute controller address
	pop	ax			; 00h = disable, 20h = enable
	out	dx,al			;  trust me.
	pop	dx			; restore DX and we done.
	ret
disablevideo	endp

; **************** Function home()  ********************************

;	Home the cursor (called before printfs)

home	proc
	mov	ax,0200h		; force the cursor
	mov	bx,0			; in page 0
	mov	dx,0			; to the home position
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret
home	endp

; **************** Function clscr() ********************************

;	Clear the screen (in between text screens)

clscr	proc
	call	home			; home the cursor
	mov	ax,0600h		; clear the entire screen
	mov	bx,0700h		; to black
	mov	cx,0			; top left
	mov	dx,1979h		; bottom right
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
clscr	endp

; **************** Function getcolor(xdot, ydot) *******************

;	Return the color on the screen at the (xdot,ydot) point

getcolor	proc	uses di si es, xdot:word, ydot:word
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	cx,xdot			; load up the registers
	mov	dx,ydot			;  for the video routine
	call	dotread			; read the dot via the approved method
	mov	ah,0			; clear the high-order bits
	ret				; we done.
getcolor	endp

; ************** Function putcolor(xdot, ydot, color) *******************

;	write the color on the screen at the (xdot,ydot) point

putcolor	proc	uses di si es, xdot:word, ydot:word, xcolor:word
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	cx,xdot			; load up the registers
	mov	dx,ydot			;  for the video routine
	mov	ax,xcolor		;  ...
	call	dotwrite		; write the dot via the approved method
	call	videocleanup		; perform any video cleanup required
	ret				; we done.
putcolor	endp

; ************** Function putblock(xmin, ymin, xmax, ymax, color) *******************

;	write a block of color on the screen  
;	on the rectangle bounded by (xmin,ymin) and (xmax,ymax)

putblock	proc	uses di si es, xmin:word, ymin:word, xmax:word, ymax:word, xcolor:word
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	cx,xmin			; x-loop
	dec	cx			; get a running start
putblx:	inc	cx			; next x-dot
	mov	dx,ymin			; y-loop
	dec	dx			; get a running start
putbly:	inc	dx			; next y-dot
	push	cx			; save registers aroud the call
	push	dx			;  ...
	mov	ax,xcolor		;  ...
	call	dotwrite		; write the dot via the approved method
	pop	dx			; restore registers
	pop	cx			;  ...
	cmp	dx,ymax			; done with y-loop?
	jne	putbly			; nope.
	cmp	cx,xmax			; done with x-loop:
	jne	putblx			; nope.
	call	videocleanup		; perform any video cleanup required
	ret				; we done.
putblock	endp

; ***************Function out_line(color) *********************

;	This routine is a 'line' analog of 'putcolor()', and sends an
;	entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;	Called by the GIF decoder

out_line	proc	uses di si es, ycolor:word
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	si,offset ycolor	; get the color for dot 'x'
        call    linewrite		; mode-specific linewrite routine
        inc	rowcount		; next row
        xor	ax,ax			; return 0
	ret
out_line	endp

; *********************** Function loaddac() ****************************

;	Function to Load the dacbox[][] array, if it can
;	(sets dacbox[0][0] to an invalid '255' if it can't)

loaddac	proc
	push	es			; need ES == DS temporarily
	push	ds			;  ...
	pop	es			;  ...
	mov	dacbox,255		; a flag value to detect invalid DAC
	mov	ax,1017h		; get the old DAC values
	mov	bx,0			;  (assuming, of course, they exist)
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	int	10h			; do it.
	pop	es			;  ...
	ret
loaddac	endp

; *************** Function spindac(direction, rstep) ********************

;	Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;	in "rstep" increments - or, if "direction" is 0, just replace it.

spindac	proc	uses di si es, direction:word, rstep:word
	cmp	dacbox,255		; do we have DAC registers to spin?
	je	spinbailout		;  nope.  bail out.
	cmp	colors,16		; at least 16 colors?
	jge	spindacdoit		;  yup.  spin away.
spinbailout:
	jmp	spindacreturn		;  nope.  bail out.

spindacdoit:
	push	ds			; need ES == DS here
	pop	es			;  ...

	mov	cx, rstep		; loop through the rotate "rstep" times
stepDAC:
	push	cx			; save the loop counter for a tad

	cmp	direction,0		; just replace it?
	je	newDAC			;  yup.
	cmp	direction,1		; rotate upwards?
	jne	short downDAC		;  nope.  downwards
	cld				; set the direction
	mov	si,offset dacbox+3	; set up the rotate
	mov	di,offset dacbox+768	;  ...
	mov	cx,3			;  ...
	rep	movsb			; rotate it.
	mov	si,offset dacbox+6	; set up the rotate
	mov	di,offset dacbox+3	;  ...
	mov	cx,765			;  ...
	rep	movsb			; rotate it.
	jmp	short newDAC		; set the new DAC
downDAC:
	std				; set the direction
	mov	si,offset dacbox+767	; set up the rotate
	mov	di,offset dacbox+770	;  ...
	mov	cx,765			;  ...
	rep	movsb			; rotate it.
	mov	si,offset dacbox+770	; set up the rotate
	mov	di,offset dacbox+5	;  ...
	mov	cx,3			;  ...
	rep	movsb			; rotate it.
newDAC:
	cld				; set the direction
	pop	cx			; restore the loop counter
	loop	stepDAC			; and loop until done.

	mov	bx,0			;  set up to update the DAC
	mov	dacnorm,0		;  indicate no overflow
dacupdate:
	mov	cx,daccount		;  ...
	mov	ax,256			; calculate 256 - BX
	sub	ax,bx			;  ...
	cmp	ax,cx			; is that less than the update count?
	jge	retrace1		;  nope.  no adjustment
	mov	cx,ax			;  else adjust
	mov	dacnorm,1		; and indicate overflow
retrace1:
	mov	dx,03dah		; wait for no retrace
	in	al,dx			;  ...
	and	al,8			; this bit is high during a retrace
	jnz	retrace1		;  so loop until it goes low
retrace2:
	in	al,dx			; wait for no retrace
	and	al,8			; this bit is high during a retrace
	jz	retrace2		;  so loop until it goes low
	cmp	cpu,88			; are we on a (yuck, ugh) 8088/8086?
	jle	spinbios		;  yup. go through the BIOS
.186
	mov	dx,03c8h		; set up for a blitz-write
	mov	ax,bx			; from this register
	cli				; critical section:  no ints
	out	dx,al			; starting register
	inc	dx			; set up to update colors
	mov	si, offset dacbox	; get starting addr in SI
	add	si,bx			;  ...
	add	si,bx			;  ...
	add	si,bx			;  ...
	mov	ax,cx			; triple the value in CX
	add	cx,ax			;  ...
	add	cx,ax			;  ...
	rep	outsb			; whap!  Zango!  They're updated!
	sti				; end of critical section
	mov	cx,ax			; restore CX for code below
	jmp	spindone		; skip over the BIOS version.
.8086
spinbios:
	mov	dx,offset dacbox	; set up the DAC box offset
	add	dx,bx			;  ...
	add	dx,bx			;  ...
	add	dx,bx			;  ...
	mov	ax,1012h		; update the DAC
	int	10h			; do it.
spindone:
	cmp	daclearn,0		; are we still in learn mode?
	jne	nolearn			;  nope.
	mov	dx,03dah		; check for the retrace
	in	al,dx			;  ...
	and	al,1			; this bit is high if display disabled
	jz	donelearn		;  oops.  retrace finished first.
	cmp	dacnorm,0		; was this a "short" update?
	jne	short nolearn		;  then don't increment it
	inc	daccount		; increment the daccount value
	inc	daccount		; increment the daccount value
	inc	daccount		; increment the daccount value
	mov	ax,cyclelimit		; collect the cycle-limit value
	cmp	daccount,ax		; sanity check: don't update too far
	jle	short nolearn		;  proceed if reasonable.
donelearn:
	sub	daccount,6		; done learning: reduce the daccount
	mov	daclearn,1		; set flag: no more learning
nolearn:
	add	bx,cx			; set up for the next batch
	cmp	bx,256			; more to go?
	jge	spindacreturn		;  nope.  we done.
	jmp	dacupdate		;  yup.  do it.

spindacreturn:
	ret
spindac	endp

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
	mov	ax,extraseg		; load DS == extra segment
	mov	ds,ax			;  ..
	mov	si,fromoffset		; load from here
	mov	di,toaddr		; load to here
	mov	cx,tocount		; this many bytes
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
	mov	ax,keybuffer			; keypress may be here
	mov	keybuffer,0			; if it was, clear it
	cmp	ax,0				; is a keypress outstanding?
	jne	getakey4			;  if so, we're done!
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
initasmvars proc near

	 cmp	cpu,0			; have we been called yet:
	 jne	initreturn		;  yup.  no need to be here.

	mov	ah,48h			; grab 64K of memory, if we can
	mov	bx,4096			; (in paragraphs)
	int	21h			; do it.
	jc	nomemory		;  oops.  no can do.
	mov	extraseg,ax		; got it.  save here.
nomemory:
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

	call	cputype			; what kind of CPU do we have here?
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


; ****************** Function cputype() *****************************

; This program was downloaded from PC Tech Journal's Hotline service
; (it was originally in their November, 1987 issue), and is used here
; with their knowledge and permission.

; Function cputype(), for real OR protected mode.  Returns (in AX)
; the value 86, 186, 286 or 386; negative if protected mode.


           .286P                  ;enable protected-mode instr.

.code

cputype    proc    near
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

