;	Generic assembler routines having to do with video adapters
;
; ---- Video Routines 
;
;	setvideomode()
;	getcolor()
;	putcolor()
;	out_line()
;	drawbox()
;	home()
;	movecursor()
;	clscr()
;	scrollup()
;	putblock()
;	storedac()		TARGA - NEW - 3 June 89 j mclain
;	loaddac()
;	spindac()
;	asmdotwrite		(called by CALCMAND.ASM using registers)
;	asmvideocleanup			""
;
; ---- Help (Video) Support
;
;	setfortext()
;	setforgraphics()
;	setclear()
;

;			 required for compatibility if Turbo ASM
IFDEF ??Version
	MASM51
	QUIRKS
ENDIF

	.MODEL  medium,c

	.8086

        ; these must NOT be in any segment!!
        ; this get's rid of TURBO-C fixup errors

        extrn   startvideo:far          ; start your-own-video routine
        extrn   readvideo:far           ; read  your-own-video routine
        extrn   writevideo:far          ; write your-own-video routine
        extrn   endvideo:far            ; end   your-own-video routine

        extrn   startdisk:far           ; start disk-video routine
        extrn   readdisk:far            ; read  disk-video routine
        extrn   writedisk:far           ; write disk-video routine
        extrn   enddisk:far             ; end   disk-video routine

; TARGA 28 May 80 - j mclain
        extrn   StartTGA  :far		; start	TARGA
        extrn   ReadTGA   :far		; read	TARGA
        extrn   WriteTGA  :far		; write	TARGA
	extrn	LineTGA   :far		; line	TARGA
        extrn   EndTGA    :far		; end	TARGA


; 8514/A routines
	extrn   open8514  :far      ; start 8514a
	extrn   reopen8514:far      ; restart 8514a
	extrn   close8514 :far      ; stop 8514a
	extrn   fr85wdot  :far      ; 8514a write dot
	extrn   fr85wbox  :far      ; 8514a write box
	extrn   fr85rdot  :far      ; 8514a read dot
	extrn   w8514pal  :far      ; 8514a pallete update

; Hercules Routines
	extrn	inithgc	  :far      ; Initialize Hercules card graphics mode
	extrn	termhgc   :far      ; Terminate Hercules card graphics mode
	extrn	writehgc  :far      ; Hercules write dot
	extrn   readhgc   :far      ; Hercules read dot
	extrn   linehgc   :far      ; Hercules line draw
	extrn   charhgc   :far      ; Draw an ASCII char in Hercules graphics 

.DATA

; ************************ External variables *****************************

	extrn	oktoprint: word		; flag: == 1 if printf() will work
	extrn	dotmode: word		; video mode:
;						1 = use the BIOS (yuck)
;						2 = use EGA/VGA style
;						3 = use MCGA style
;						4 = use Tseng 256 color
;						5 = use Paradise 256 color
;						6 = use Video 7 256 color
;						7 = MVGA 256 color
;						9 = use TARGA
;						10= Hercules
;						11= "disk" video
;						12= 8514/A 
;						13= CGA 4,2-color
;						14= Tandy-1000 (someday)
;						15= use Trident 256 color
;						16= use Chips & Tech 256 color
;						17= use ATI VGA Wonder 256 color
;						18= use Everex 256 color
;						19= use roll-your-own
;						    (see YOURVID.C)
;						20= use ATI 1024  16 color
;						21= use tseng     16 color
;						22= use trident   16 color
;						23= use video7    16 color
;						24= use paradise  16 color
;						25= use chipstech 16 color
;						26= use everex    16 color

	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	ixmin:word, ixmax:word	; for zoom/pan: x and y limits
	extrn	iymin:word, iymax:word	;  for the local zoom-box
	extrn	cyclelimit:word		; limiting factor for DAC-cycler
	extrn	debugflag:word		; for debugging purposes only

	extrn	boxcount:word		; (previous) box pt counter: 0 if none.
	extrn	boxx:word, boxy:word	; zoom-box save-value locations
	extrn	boxvalues:byte		; zoom-box save-pixel locations

	extrn	xorTARGA:word		; TARGA 3 June 89 j mclain
					; flag says xor pixels for box

	extrn	cpu:word		; CPU type (86, 186, 286, or 386)
	extrn	extraseg:word		; location of the EXTRA segment

; ************************ Public variables *****************************

public		andcolor		; used by 'calcmand'

public		loadPalette		; flag for loading VGA/TARGA palette from disk
public		dacbox			; GIF saves use this
public		daclearn, daccount	; Rotate may want to use this
public		rowcount		; row-counter for decoder and out_line
public		reallyega		; "really an EGA" (faking a VGA) flag

;		arrays declared here, used elsewhere
;		arrays not used simultaneously are deliberately overlapped

; ************************ Internal variables *****************************

dotwrite	dw	0		; write-a-dot routine:  mode-specific
dotread		dw	0		; read-a-dot routine:   mode-specific
linewrite	dw	0		; write-a-line routine: mode-specific
andcolor	dw	0		; "and" value used for color selection
color		db	0		; the color to set a pixel
videoflag	db	0		; special "your-own-video" flag
diskflag	db	0		; special "disk-video" flag
tgaflag		db	0		; TARGA 28 May 89 - j mclain
loadPalette	db	0		; TARGA/VGA load palette from disk

f85flag		db	0		;flag for 8514a

HGCflag		db	0		;flag for Hercules Graphics Adapter

;					; Zoom-Box values (2K x 2K screens max)
step		dw	0		; Zoom-Box drawing step-size
boxcolor	db	0		; Zoom-Box color
reallyega	dw	0		; 1 if its an EGA posing as a VGA
palettega	db	17 dup(0)	; EGA palette registers go here
daclearn	db	0		; 0 if "learning" DAC speed
dacnorm		db	0		; 0 if "normal" DAC update
daccount	dw	0		; DAC registers to update in 1 pass
dacbox		db	773 dup(0)	; DAC goes here

rowcount	dw	0		; row-counter for decoder and out_line

orvideo		db	0		; "or" value for setvideo
videoax		dw	0		; graphics mode values: ax
videobx		dw	0		; graphics mode values: bx
videocx		dw	0		; graphics mode values: cx
videodx		dw	0		; graphics mode values: dx

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


x320y400        db      320/8                   ; number of screen columns
                db      400/16                  ; number of screen rows
                db       5fh, 4fh, 50h, 82h     ; CRTC Registers
                db       54h, 80h,0bfh, 1fh
                db       00h, 40h, 00h, 00h
                db       00h, 00h, 00h, 00h
                db       9ch, 8eh, 8fh, 28h
                db       00h, 96h,0b9h,0E3h
                db      0FFh

x640y400        db      640/8			; number of screen columns
                db      400/16                  ; number of screen rows   
		db	 5eh, 4fh, 50h, 01h     ; CRTC Registers          
		db       54h, 9fh,0c0h, 1fh
		db       00h, 40h, 00h, 00h
		db       00h, 00h, 00h, 00h
		db       9ch,08eh, 8fh, 28h
		db       00h, 95h,0bch,0c3h
		db       0ffh               

tweaks		dw	offset x704y528		; tweak table
		dw	offset x704y528
		dw	offset x720y540
		dw	offset x736y552
		dw	offset x752y564
		dw	offset x768y576
		dw	offset x784y588
		dw	offset x800y600
		dw	offset x360y480
		dw	offset x320y400
		dw	offset x640y400		; Tseng Super VGA

tweaktype	dw	0			; 8 or 9 (320x400 or 360x480)

.CODE

;	Routines called by this code

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

cgawrite	proc	near		; CGA 320x200 4-color, 640x200 2-color
	mov	bx,colors		; restrict ourselves to the color cnt
	dec	bx			;  ...
	and	al,bl			;  ...
	push	es			; save ES for a bit
	push	ax			; save AX for a bit
	call	near ptr cgasegoff	; compute the segment and offset
	mov	dl,es:[bx]		; retrieve the byte
	and	dl,al			; clear out the proper bits
	pop	ax			; restore AX (the color)
	shl	al,cl			; apply the shift mask
	or	dl,al			; add in the color
	mov	es:[bx],dl		; and write the color back out
	pop	es			; restore ES
	ret				; we done
cgawrite	endp

cgaread	proc	near			; CGA 320x200 4-color, 640x200 2-color
	push	es			; save ES for a bit
	call	near ptr cgasegoff	; compute the segment and offset
	mov	dl,es:[bx]		; retrieve the byte
	not	al			; reset AL for the AND
	and	al,dl			; clear out all but the proper bits
	shr	al,cl			; apply the shift mask
	mov	ah,0			; clear out the high byte
	pop	es			; restore ES
	ret				; we done
cgaread	endp

cgasegoff	proc	near		; common CGA routine
	mov	ax,0b800h		; buffer is really here
	shr	dx,1			; provide the interleaving logic
	jnc	cgasegeven		;  skip if odd
	add	ax,200h			; use the other half of the buffer
cgasegeven:
	mov	es,ax			; set it up
	mov	bx,dx			; each row is 80 bytes - shift
	shl	bx,1			;  instead of multiply (8088s multiply
	shl	bx,1			;  REAL slowly)
	shl	bx,1			;  ...
	shl	bx,1			;  ...
	mov	ax,bx			;  x*80 = (x<<4) + ((x<<4)<<2)
	shl	bx,1			;  ...
	shl	bx,1			;  ...
	add	ax,bx			; (Finally!)
	cmp	colors,2		; 2-color mode?
	je	short cgaseg2color	;  yup.  branch off
	mov	bx,cx			; get the column offset
	shr	bx,1			; four columns per byte
	shr	bx,1			;  ...
	add	bx,ax			; BX now contains the offset
	and	cx,3			; calculate the bit mask
	xor	cx,3			;  ...
	shl	cx,1			; shift left this many bits
	push	cx			; save the bit shift
	mov	al,0fch			; set up the bit mask
	rol	al,cl			; AL now contains the bit mask
	pop	cx			; restore the shift count
	ret				; we done.
cgaseg2color:				; two-color option
	mov	bx,cx			; get the column offset
	shr	bx,1			; eight columns per byte
	shr	bx,1			;  ...
	shr	bx,1			;  ...
	add	bx,ax			; BX now contains the offset
	and	cx,7			; calculate the bit mask
	xor	cx,7			;  ...
	push	cx			; save the bit shift
	mov	al,0feh			; set up the bit mask
	rol	al,cl			; AL now contains the bit mask
	pop	cx			; restore the shift count
	ret
cgasegoff	endp

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

;	These routines are for bit-plane 16 color modes, including bank 
;	switched superVGA varieties such as the Tseng 1024x768x16 mode.
;		Tim Wegner
;
vgawrite	proc	near		; bank-switched EGA/VGA write mode 0
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

        cmp	dx,curbk		; see if bank changed
        je	vgasame_bank		; jump if old bank ok
        mov	ax,dx			; newbank expects bank in al
        call    far ptr newbank		; switch banks
vgasame_bank:

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

vgaread	proc	near		; bank-switched EGA/VGA read mode 0
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

        cmp	dx,curbk		; see if bank changed
        je	vgasame_bank		; jump if old bank ok
        mov	ax,dx			; newbank expects bank in al
        call    far ptr newbank		; switch banks
vgasame_bank:

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

  mov     ax,xdots
  shr     ax,1    
  shr     ax,1                   ; now ax = xdots/4     
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
  mov     ax,xdots
  shr     ax, 1    
  shr     ax, 1                  ; now ax = xdots/4     
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

;	The following ATI 1024x768x16 mode is courtesy of Mark Peterson

ati1024read	proc near		; ATI 1024x768x16 read
	call	ati1024addr		; calculate the address
	mov	al,es:[bx]		; get the byte the pixel is in
	test	cl,1			; is X odd?
	jz	atireadhigh		;  Nope.  Use the high bits
	and	ax,0fh			; zero out the high-order bits
	ret
atireadhigh:
	and	ax,0f0h			; zero out the low-order bits
	mov	cl,4			; shift the results
	shr	al,cl			;  ...
	ret
ati1024read	endp

ati1024write	proc near		; ATI 1024x768x16 write
	call	ati1024addr		; calculate the address
	mov	dl,es:[bx]		; get the byte the pixel is in
	and	al,00fh			; zero out the high-order color bits
	test	cl,1			; is X odd?
	jz	atiwritehigh		;  Nope.  Use the high bits
	and	dl,0f0h			; zero out the low-order video bits
	or	dl,al			; add the two together
	mov	es:[bx],dl		; and write the results
	ret
atiwritehigh:
	mov	cl,4			; shift the color bits
	shl	al,cl			;  ...
	and	dl,0fh			; zero out the high-order video bits
	or	dl,al			; add the two together
	mov	es:[bx],dl		; and write the results
	ret
ati1024write	endp

ati1024addr	proc	near		; modification of TIW's Super256addr
        clc				; clear carry flag
	push	ax			; save this for a tad
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines - ans in dx:ax
	add	ax,cx			; plus this many x-dots
        adc     dx,0			; answer in dx:ax
	shr	dx,1			; shift the answer right one bit
	rcr	ax,1			;  .. in the 32-bit DX:AX combo
	mov	bx,ax			; save this in BX
        cmp	dx,curbk		; see if bank changed
        je	atisame_bank		; jump if old bank ok
        mov	ax,dx			; newbank expects bank in al
        call    far ptr newbank
atisame_bank:
	pop	ax			; restore AX
	ret
ati1024addr	endp

;
;	The following 'Super256' code is courtesy of Timothy Wegner.
;

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
        cmp	dx,curbk		; see if bank changed
        je	same_bank		; jump if old bank ok
        mov	ax,dx			; newbank expects bank in al
        call    far ptr newbank
same_bank:
	pop	ax			; restore AX
        ret
super256addr	endp

;
;       BANKS.ASM was used verbatim except:
;          1) removed ".model small"
;          2) deleted "end"
;       Integrated by Tim Wegner 8/15/89 
;	(switched to John's 9/7/89 version on 9/10/89 - Bert)
;        

;
;	Copyright 1988,89 John Bridges
;	Free for use in commercial, shareware or freeware applications
;
;	BANKS.ASM
;
;
.data

	public	curbk

curbk	dw	0ffffh		;current bank number


	public	tseng,trident,video7,paradise,chipstech,ativga,everex

tseng	dw	0
trident	dw	0
video7	dw	0
paradise dw	0
chipstech dw	0
ativga	dw	0
everex	dw	0

.code

newbank	proc			;bank number is in AX
	cli
	mov	[curbk],ax
	cmp	[tseng],0
	jz	nots
	push	ax		;Tseng
	push	dx
	and	al,7
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

nots:	cmp	[trident],0
	jz	notri
	push	ax		;Trident
	push	dx
		
	mov	dx,3ceh		;set page size to 64k
	mov	al,6
	out	dx,al
	inc	dl
	in	al,dx
	dec	dl
	or	al,4
	mov	ah,al
	mov	al,6
	out	dx,ax
		
	mov	dl,0c4h		;switch to BPS mode
	mov	al,0bh
	out	dx,al
	inc	dl
	in	al,dx
	dec	dl

	mov	ah,byte ptr [curbk]
	xor	ah,2
	mov	dx,3c4h
	mov	al,0eh
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

notri:	cmp	[video7],0
	jz	nov7
	push	ax		;Video 7
	push	dx
	push	cx
	and	ax,15
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
	
nov7:	cmp	[paradise],0
	jz	nopd
	push	ax		;Paradise
	push	dx
	mov	dx,3ceh
	mov	ax,50fh		;turn off write protect on VGA registers
	out	dx,ax
	mov	ah,byte ptr [curbk]
	shl	ah,1		;change 64k bank number into 4k bank number
	shl	ah,1
	shl	ah,1
	shl	ah,1
	mov	al,9
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

nopd:	cmp	[chipstech],0
	jz	noct
	push	ax		;Chips & Tech
	push	dx

;	mov     dx,46e8h	;place chip in setup mode
;	mov     ax,1eh
;	out     dx,ax
;	mov     dx,103h		;enable extended registers
;	mov     ax,0080
;	out     dx,ax
;	mov     dx,46e8h	;bring chip out of setup mode
;	mov     ax,0eh
;	out     dx,ax

	mov	ah,byte ptr [curbk]
	shl	ah,1		;change 64k bank number into 16k bank number
	shl	ah,1
	mov	al,10h
	mov	dx,3d6h
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

noct:	cmp	[ativga],0
	jz	noati
	push	ax		;ATI VGA Wonder
	push	dx
	mov	ah,al
	mov	dx,1ceh
	mov	al,0b2h
	out	dx,al
	inc	dl
	in	al,dx
	shl	ah,1
	and	al,0e1h
	or	ah,al
	mov	al,0b2h
	dec	dl
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

noati:	cmp	[everex],0
	jz	noev
	push	ax		;Everex
	push	cx
	push	dx
	sti
	mov	cl,al
	mov	dx,3c4h
	mov	al,8
	out	dx,al
	inc	dl
	in	al,dx
	dec	dl
	shl	al,1
	shr	cl,1
	rcr	al,1
	mov	ah,al
	mov	al,8
	out	dx,ax
	mov	dl,0cch
	in	al,dx
	mov	dl,0c2h
	and	al,0dfh
	shr	cl,1
	jc	nob2
	or	al,20h
nob2:	out	dx,al
	pop	dx
	pop	cx
	pop	ax
	ret

noev:	sti
	ret
newbank	endp

videowrite	proc	near		; your-own-video write routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	mov	ah,0			; clear the high-order color byte
	push	ax			; colors parameter
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr writevideo	; let the external routine do it
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	ax			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
videowrite	endp

videoread	proc	near		; your-own-video read routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr readvideo	; let the external routine do it
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
videoread	endp

videostart	proc	near		; your-own-video start routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr startvideo	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
videostart	endp

videoend	proc	near		; your-own-video end routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr endvideo	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
videoend		endp

diskwrite	proc	near		; disk-video write routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	mov	ah,0			; clear the high-order color byte
	push	ax			; colors parameter
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr writedisk	; let the external routine do it
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	ax			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
diskwrite	endp

diskread	proc	near		; disk-video read routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr readdisk	; let the external routine do it
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
diskread	endp

diskstart	proc	near		; disk-video start routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr startdisk	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
diskstart	endp

diskend		proc	near		; disk-video end routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr enddisk		; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
diskend		endp

; ***********************************************************************
;
; TARGA MODIFIED 1 JUNE 89 - j mclain
;
tgawrite 	proc	near		;
        push	es			;
        push	si			;
        push	di			;
	push	ax			; colors parameter
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr WriteTGA	; writeTGA( x, y, color )
	pop	cx			;
	pop	dx			;
	pop	ax			;
        pop	di			;
	pop	si			;
	pop	es			;
	ret				;
tgawrite	endp

tgaline 	proc	near		; 
        push	es			;
  	push	si			;
	push	di			;
	mov	ax,xdots		; pixels on line
	push	ax			; 
	mov	ax,rowcount		; line to do it too
	push	ax			; 
	push	ds			; far ptr
	push	si			; line data
	call	far ptr LineTGA		; lineTGA( ldata, line, cnt )
	add	sp,8			; stack bias
	pop	di			;
	pop	si			;
	pop	es			;
	ret
tgaline	endp

tgaread		proc	near		;
	push	es			;
	push	si			;
	push	di			;
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr ReadTGA		; readTGA( x, y )
	pop	cx			;
	pop	dx			;
	pop	di			;
	pop	si			;
	pop	es			;
	ret				;
tgaread	endp

tgastart	proc	near		;
	push	es			;
	push	si			;
	push	di			;
	mov	ax,ydots
	push	ax
	mov	ax,xdots
	push	ax
	call	far ptr StartTGA	; startTGA( xdim, ydim )
	add	sp,4			; stack bias, pointer to dacbox
	pop	di			;
	pop	si			;
	pop	es			;
	ret				;
tgastart	endp

tgaend		proc	near		;
	push	es			;
	push	si			;
	push	di			;
	call	far ptr EndTGA		; endTGA( void )
	pop	di			;
	pop	si			;
	pop	es			;
	mov	tgaflag,0		; 
	ret				;
tgaend		endp

f85start    proc    near

	call   far ptr open8514
	ret

f85start    endp


f85end	proc    near

	call   far ptr close8514
	ret

f85end	endp


f85write    proc    near

	call   far ptr fr85wdot
	ret

f85write    endp

f85read proc    near

	call   far ptr fr85rdot
	ret

f85read endp

hgcwrite proc near
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	mov	ah,0			; clear the high-order color byte
	push	ax			; colors parameter
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr writehgc	; let the Herc. Write dot routine do it
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	ax			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret
hgcwrite endp

hgcread proc near
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr readhgc		; call the Hercules Read dot routine
	pop	cx			; restore registers
	pop	dx			; restore registers
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret
hgcread endp

hgcstart	proc	near		; hercules start routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr inithgc		; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
hgcstart	endp

hgcend		proc	near		; hercules end routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr termhgc		; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
hgcend		endp


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

vgaline	proc	near		; Bank Switch EGA/VGA line write
	mov	ax,xdots		; compute # of dots / pass
	shr	ax,1			;  (given 8 passes)
	shr	ax,1			;  ...
	shr	ax,1			;  ...
	mov	cx,ax			; save the dots / pass here
        xor     dx,dx
	mul	rowcount		; now calc first video addr

        cmp	dx,curbk		; see if bank changed
        jne	bank_is_changing	; if bank change call normaline
;same_bank:

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
bank_is_changing:
        call    normaline		; just calling newbank didn't quite
        ret				;  work. This depends on no bank
vgaline	endp				;  change mid line (ok for 1024 wide)

super256line    proc    near     ; super VGA 256 colors
        mov     ax,xdots         ; this many dots / line
        mov     cx,rowcount      ; cx = row
        mul     cx               ; times this many lines
        push    ax               ; save pixel address for later
        cmp     dx,curbk         ; bank ok?
        push    dx               ; save bank
        je      bank_is_ok       ; jump if bank ok
        mov     al,dl            ; newbank needs bank in al
        call    far ptr newbank
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
	shr	cx,1		 ; convert bytes to words
        rep     movsw            ; zap line into memory
        jmp     short linedone
bank_did_chg:
        call    normaline        ; normaline can handle bank change
linedone:
        ret
super256line    endp

tweak256line	proc	near		; Normal Line:  no assumptions
	mov     cx,xdots
	shr     cx, 1    
	shr     cx, 1                   ; now ax = xdots/4     
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

f85line proc    near

	mov ax, 0           ;a line is a box one line high
	mov bx, rowcount
	mov cx, xdots
	mov dx, 1
	
	call    fr85wbox        ;put out the box
	ret

f85line endp


; ******************** Function videocleanup() **************************

;	Called at the end of any assembler video read/writes to make
;	the world safe for 'printf()'s.
;	Currently, only ega/vga needs cleanup work, but who knows?
;

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
; TARGA 3 June 89 j mclain
					;
	mov	xorTARGA,1		; faster to flag xorTARGA rather
					; than check if TARGA is runnin
notTGAbox:
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
	mov	al,boxcolor		; set the (new) box color
	cmp	colors,2		; uhh, is this a B&W screen?
	jne	drawnewnotbw		;  nope.  proceed
	mov	al,1			; XOR the old color
	sub	al,boxvalues[bx]	;  for visibility
drawnewnotbw:
	push	bx			; save the counter
	call	dotwrite		; adjust the dot.
	pop	bx			; restore the counter
	dec	bx			; are we done yet?
	jns	drawnewbox		;  nope.  try again.

; TARGA 3 June 89 j mclain
					;
	mov	xorTARGA,0		; in case of TARGA, no more xor
					;			 
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

	cmp	videoflag,1		; say, was the last video your-own?
	jne	novideovideo		;  nope.
	call	videoend		; yup.  end the your-own-video mode
	mov	videoflag,0		; set flag: no your-own-video
	jmp	short notarga
novideovideo:
	cmp	diskflag,1		; say, was the last video disk-video?
	jne	nodiskvideo		;  nope.
	call	diskend			; yup.  end the disk-video mode
	mov	diskflag,0		; set flag: no disk-video
	jmp	short notarga
nodiskvideo:
	cmp	tgaflag,1		; TARGA MODIFIED 2 June 89 j mclain
	jne	notarga
	call	tgaend
	mov	tgaflag,0		; set flag: targa cleaned up
notarga:
	cmp	f85flag, 1		; was the last video 8514?
	jne	no8514			; nope.
	call    f85end
	mov	f85flag, 0
no8514:
	cmp	HGCflag, 1		; was last video Hercules
	jne	noHGC			; nope
	call	hgcend
	mov	HGCflag, 0
noHGC:
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
	mov	bx,0			; clear out all of the 256-mode flags
	mov	tseng,bx		;  ...
	mov	trident,bx		;  ...
	mov	video7,bx		;  ...
	mov	paradise,bx		;  ...
	mov	chipstech,bx		;  ...
	mov	ativga,bx		;  ...
	mov	everex,bx		;  ...
	mov	oktoprint,1		; say it's OK to use printf()
	mov	bx,dotmode		; set up for a video table jump
	cmp	bx,30			; are we within the range of dotmodes?
	jbe	videomodesetup		; yup.  all is OK
	mov	bx,0			; nope.  use dullnormalmode
videomodesetup:
	shl	bx,1			; switch to a word offset
	mov	bx,cs:videomodetable[bx]	; get the next step
	jmp	bx			; and go there
videomodetable 	dw	offset dullnormalmode	; mode 0
	dw	offset dullnormalmode	; mode 1
	dw	offset vgamode		; mode 2
	dw	offset mcgamode		; mode 3
	dw	offset tseng256mode	; mode 4
	dw	offset paradise256mode	; mode 5
	dw	offset video7256mode	; mode 6
	dw	offset tweak256mode	; mode 7
	dw	offset everex16mode	; mode 8
	dw	offset targaMode	; mode 9 
	dw	offset hgcmode		; mode 10
	dw	offset diskmode		; mode 11
	dw	offset f8514mode	; mode 12
	dw	offset cgacolor		; mode 13
	dw	offset dullnormalmode	; mode 14
	dw	offset trident256mode	; mode 15
	dw	offset chipstech256mode	; mode 16
	dw	offset ati256mode	; mode 17
	dw	offset everex256mode	; mode 18
	dw	offset yourownmode	; mode 19
	dw	offset ati1024mode	; mode 20
	dw	offset tseng16mode	; mode 21
	dw	offset trident16mode	; mode 22
	dw	offset video716mode	; mode 23
	dw	offset paradise16mode	; mode 24
	dw	offset chipstech16mode	; mode 25
	dw	offset dullnormalmode	; mode 26
	dw	offset dullnormalmode	; mode 27
	dw	offset dullnormalmode	; mode 28
	dw	offset dullnormalmode	; mode 29
	dw	offset dullnormalmode	; mode 30

dullnormalmode:
	mov	ax,offset normalwrite	; set up the BIOS write-a-dot routine
	mov	bx,offset normalread	; set up the BIOS read-a-dot  routine
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
mcgamode:
	mov	ax,offset mcgawrite	; set up MCGA write-a-dot routine
	mov	bx,offset mcgaread	; set up MCGA read-a-dot  routine
	mov	cx,offset mcgaline 	; set up the MCGA linewrite routine
	jmp	videomode		; return to common code
tseng16mode:
        mov     tseng,1			; set chipset flag
        jmp	vgamode		; set ega/vga functions
trident16mode:
        mov     trident,1		; set chipset flag
        jmp	vgamode
video716mode:
        mov     video7,1		; set chipset flag
        jmp	vgamode
paradise16mode:
        mov     paradise,1		; set chipset flag
        jmp	vgamode
chipstech16mode:
        mov     chipstech,1		; set chipset flag
        jmp	vgamode
everex16mode:
        mov     everex,1		; set chipset flag
        jmp	vgamode
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up EGA/VGA read-a-dot  routine
	mov	cx,offset vgaline 	; set up the EGA/VGA linewrite routine
	jmp	videomode		; return to common code
tseng256mode:
	mov	tseng,1			; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
paradise256mode:
	mov	paradise,1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
video7256mode:
	mov	video7,	1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
trident256mode:
	mov	trident,1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
chipstech256mode:
	mov	chipstech,1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
ati256mode:
	mov	ativga,1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
everex256mode:
	mov	everex,1		; set chipset flag
 	jmp	super256mode		; set super VGA linear memory functions 
super256mode:
	mov	ax,offset super256write	; set up superVGA write-a-dot routine
	mov	bx,offset super256read	; set up superVGA read-a-dot  routine
	mov	cx,offset super256line 	; set up the  linewrite routine
	jmp	videomode		; return to common code
tweak256mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak256write	; set up tweaked-256 write-a-dot
	mov	bx,offset tweak256read	; set up tweaked-256 read-a-dot
	mov	cx,offset tweak256line 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
cgacolor:
	mov	ax,offset cgawrite	; set up CGA write-a-dot
	mov	bx,offset cgaread	; set up CGA read-a-dot
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
ati1024mode:
	mov	ativga,1		; set ATI flag.
	mov	ax,offset ati1024write	; set up ATI1024 write-a-dot
	mov	bx,offset ati1024read	; set up ATI1024 read-a-dot
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
diskmode:
	call	diskstart		; start up the disk routines
	mov	ax,offset diskwrite	; set up disk-vid write-a-dot routine
	mov	bx,offset diskread	; set up disk-vid read-a-dot routine
	mov	cx,offset normaline 	; set up the normal linewrite routine
	mov	diskflag,1		; flag "disk-end" needed.
	jmp	videomode		; return to common code
yourownmode:
	call	videostart		; start up your-own-video routines
	mov	ax,offset videowrite	; set up ur-own-vid write-a-dot routine
	mov	bx,offset videoread	; set up ur-own-vid read-a-dot routine
	mov	cx,offset normaline 	; set up the normal linewrite routine
	mov	videoflag,1		; flag "your-own-end" needed.
	jmp	videomode		; return to common code
targaMode:				; TARGA MODIFIED 2 June 89 - j mclain
	call	tgastart		; 
	mov	ax,offset tgawrite	; 
	mov	bx,offset tgaread	; 
;	mov	cx,offset tgaline 	; 
	mov	cx,offset normaline 	; set up the normal linewrite routine
	mov	tgaflag,1		; 
	jmp	videomode		; return to common code
f8514Mode:                     ; 8514 modes
       cmp     videodx, 1      ; requiring dx=1 for turn on allows
       jne     not8514on       ; setvideomode(3,0,0,0) to display text
       call    open8514        ; start the 8514a
       jnc     f85ok
       mov     dotmode, 0      ; if problem starting use normal mode
not8514on:
       jmp     dullnormalmode
hgcmode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	call	hgcstart		; Initialize the HGC card
	mov	ax,offset hgcwrite	; set up HGC write-a-dot routine
	mov	bx,offset hgcread	; set up HGC read-a-dot  routine
	mov	cx,offset normaline	; set up normal linewrite routine
	mov	HGCflag,1		; flag "HGC-end" needed.
	jmp	videomode		; return to common code
f85ok:
	mov	ax,offset f85write  	; 
	mov	bx,offset f85read   	; 
	mov	cx,offset f85line   	; 
	mov	f85flag,1       	; 
	mov	oktoprint,0     	; NOT OK to printf() in this mode
	jmp	videomode       	; return to common code


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
	call	maybeor			; maybe or AL or (for Video-7s) BL
	push	bp			; some BIOS's don't save this
	int	10h			; do it via the BIOS.
	pop	bp			; restore the saved register
	jmp	setvideoreturn		;  and return.

setvideoregs:				; assume genuine VGA and program regs

	mov	si,dx			; get the video table offset
	shl	si,1			;  ...
	mov	si,word ptr tweaks[si]	;  ...

        mov     tweaktype, dx           ; save tweaktype 
	cmp	dx,8			; 360x480 tweak256mode?	
	je	isatweaktype		; yup	
	cmp	dx,9			; 320x400 tweak256mode?
	je	isatweaktype		; yup
        cmp	dx,10			; Tseng tweak?
	je	tsengtweak		; yup
        jmp	not256			; nope none of the above
tsengtweak:
	mov 	ax,46  			; start with S-VGA mode 2eh
	call	maybeor			; maybe don't clear the video memory
 	int 	10h  			; let the bios clear the video memory
 	mov 	dx,3c2h  		; misc output
 	mov 	al,063h  		; dot clock
 	out 	dx,al  			; select it
 	mov 	dx,3c4h  		; sequencer again
 	mov 	ax,0300h		; restart sequencer
 	out 	dx,ax 			; running again
        jmp 	is256;

isatweaktype:    
         mov        ax,0013h             ; invoke video mode 13h
         call       maybeor              ; maybe or AL or (for Video-7s) BL
         int        10h                  ; do it
   
         mov        dx,3c4h              ; alter sequencer registers
         mov        ax,0604h             ; disable chain 4
         out        dx,ax
   
         cmp       orvideo,0             ; are we supposed to clear RAM?
         jne       noclear256            ;  (nope)

	mov	dx,03c4h		; alter sequencer registers
	mov	ax,0f02h		; enable writes to all planes
	OUT_WORD

	push	es			; save ES for a tad
	mov	ax,VGA_SEGMENT		; clear out all 256K of 
	mov	es,ax			;  video memory
	sub	di,di			;  (64K at a time, but with
	mov	ax,di			;  all planes enabled)
	mov	cx,8000h		;# of words in 64K
	cld
	rep stosw			;clear all of display memory
	pop	es			; restore ES

noclear256:
	mov	dx,3c4h			; alter sequencer registers
	mov	ax,0604h		; disable chain 4
	out	dx,ax

	jmp	short is256		; forget the ROM characters

not256:

	mov	ax,0012h		; invoke video mode 12h
	call	maybeor			; maybe or AL or (for Video-7s) BL
	int	10h			; do it.

	mov	ax,1124h		; load ROM 8*16 characters
	mov	bx,0
	mov	dh,0
	mov	dl,byte ptr [si+1]	; number of rows on the screen
	int	10h

is256:	push	es			; save ES for a tad
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

	cli				; turn off all interrupts
        mov     dx,tweaktype
        cmp	dx,9			; 320x400 mode?
        je	not256mode		; yup - skip this stuff
        cmp	dx,10			; Tseng tweak mode?
        je	not256mode		; yup - skip this stuff
	mov	dx,03c4h		; Sequencer Synchronous reset
	mov	ax,0100h		; set sequencer reset
	out	dx,ax
	mov	dx,03c2h		; Update Misc Output Reg
	mov	al,0E7h
	out	dx,al
	mov	dx,03c4h		; Sequencer Synchronous reset
	mov	ax,0300h		; clear sequencer reset
	out	dx,ax
not256mode:
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
	sti				; restore interrupts

	pop	es			; restore ES

setvideoreturn:

	mov	curbk,0ffffh		; stuff impossible value into cur-bank

	mov	orvideo,0		; reset the video to clobber memory

	ret
setvideo	endp

maybeor	proc	near			; or AL or BL for mon-destr switch
	cmp	ah,6fh			; video-7 special mode?
	je	maybeor1		;  yup.  do this one different
	or	al,orvideo		; normal non-destructive switch
	jmp	short maybeor2		; we done.
maybeor1:
	or	bl,orvideo		; video-7 switch
maybeor2:
	ret				; we done.
maybeor	endp


; ********* Functions setfortext() and setforgraphics() ************

;	setfortext() resets the video for text mode and saves graphics data
;	setforgraphics() restores the graphics mode and data
;	setclear() clears the screen after setfortext() [which may be wierd]

setfortext	proc	uses es si di
	cmp	dotmode, 12		;check for 8514
	jne	tnot8514
	cmp	f85flag, 0	 	;check 8514 active flag
	je	dosettext
	call    close8514		;close adapter if not
	mov	f85flag, 0
	jmp	short dosettext
tnot8514:
	cmp	videoax,0		; check for CGA modes
	je	setfortextnocga		;  not this one
	cmp	ydots,348		; (only Hercules modes have this res)
	je	setfortextcga		;  ...
	cmp	videoax,7		;  ...
	ja	setfortextnocga		;  not this one
setfortextcga:
	mov	ax,extraseg		; set ES == Extra Segment
	add	ax,1000h		; (plus 64K)
	mov	es,ax			;  ...
	mov	di,4000h		; save the video data here
	mov	ax,0b800h		; video data starts here <XXX>
	mov	si,0			;  ...
	mov	cx,2000h		; save this many words
	cmp	ydots,348		; (only Hercules modes have this res)
	jne	setfortextcganoherc	;  ...
	mov	di,0			; (save 32K)
	mov	ax,0b000h		; (from here)
	mov	cx,4000h		; (save this many words)
setfortextcganoherc:
	push	ds			; save DS for a tad
	mov	ds,ax			;  reset DS
	cld				; clear the direction flag
	rep	movsw			;  save them.
	pop	ds			; restore DS
	cmp	dotmode, 10		;check for Hercules-specific dotmode
	jne	tnotHGC
	cmp	HGCflag, 0	 	;check HGC active flag
	je	dosettext
	call    hgcend			;close adapter if not
	mov	HGCflag, 0
	jmp	short dosettext
tnotHGC:
dosettext:
	mov	ax,3			; set up the text call
	mov	bx,0			;  ...
	mov	cx,0			;  ...
	mov	dx,0			;  ...
	call	setvideo		; set the video
	jmp	setfortextreturn
setfortextnocga:	
	mov	ax,0			; disable the video (I think)
	call	disablevideo		;  ...
	mov	orvideo,80h		; set the video to preserve memory
	mov	ax,6			; set up the text call
	mov	bx,0			;  ...
	mov	cx,0			;  ...
	mov	dx,0			;  ...
	call	setvideo		; set the video
	mov	ax,0			; disable the video (I think)
	call	disablevideo		;  ...
	cld				; clear the direction flag
	mov	ax,extraseg		; set ES == Extra Segment
	add	ax,1000h		; (plus 64K)
	mov	es,ax			;  ...
	mov	di,4000h		; save the video data here
	mov	ax,0b800h		; video data starts here
	push	ds			; save DS for a tad
	mov	ds,ax			;  reset DS
	mov	si,0			;  ...
	mov	cx,2000h		; save this many words
	rep	movsw			;  save them.
	pop	ds			; restore DS
	mov	ax,0b800h		; clear the video buffer
	mov	es,ax			;  ...
	mov	di,0			;  ...
	mov	ax,0			; to blanks
	mov	cx,2000h		; this many blanks
	rep	stosw			; do it.
	mov	ax,20h			; enable the video (I think)
	call	disablevideo		;  ...
setfortextreturn:
	call	far ptr home		; home the cursor
	ret
setfortext	endp

setforgraphics	proc	uses es si di
	cmp	 dotmode, 12		;check for 8514
	jne	gnot8514
	cmp	f85flag, 0
	jne	f85isgraphics
	call    reopen8514
	mov	f85flag, 1
f85isgraphics:
	jmp	setforgraphicsreturn
gnot8514:
	cmp	videoax,0		; check for CGA modes
	je	setforgraphicsnocga	;  not this one
	cmp	ydots,348		; (only Hercules modes have this res)
	je	setforgraphicscga	;  ...
	cmp	videoax,7		;  ...
	ja	setforgraphicsnocga	;  not this one
setforgraphicscga:
	cmp	dotmode, 10		;check for Hercules-specific dotmode
	jne	tnotHGC2		;  (nope.  dull-normal stuff)
	call	hgcstart		; Initialize the HGC card
	mov	HGCflag,1		; flag "HGC-end" needed.
	jmp	short twasHGC2		; bypass the normal setvideo call
tnotHGC2:
	mov	ax,videoax		; set up the video call
	mov	bx,videobx		;  ...
	mov	cx,videocx		;  ...
	mov	dx,videodx		;  ...
	call	setvideo		; do it.
twasHGC2:
	mov	bx,extraseg		; restore is from Extraseg
	add	bx,1000h		; (plus 64K)
	mov	si,4000h		; video data is saved here
	mov	ax,0b800h		; restore the video area
	mov	di,0			;  ...
	mov	cx,2000h		; restore this many words
	cmp	ydots,348		; (only Hercules modes have this res)
	jne	setforgraphicscganoherc	;  ...
	mov	si,0			; (restore 32K)
	mov	ax,0b000h		; (to here)
	mov	cx,4000h		; (restore this many words)
setforgraphicscganoherc:
	push	ds			; save DX for a tad
	mov	es,ax			; load the dest seg into ES
	mov	ds,bx			; restore it from the source seg
	cld				; clear the direction flag
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
	add	ax,1000h		; (plus 64K)
	mov	ds,ax			;  ...
	mov	si,4000h		; video data is saved here
	mov	cx,2000h		; restore this many words
	rep	movsw			; restore them.
	pop	ds			; restore DS
	mov	orvideo,80h		; set the video to preserve memory
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

; **************** Function movecursor(row, col)  **********************

;	Move the cursor (called before printfs)

movecursor	proc	cursorrow:word, cursorcol:word
	mov	ax,0200h		; force the cursor
	mov	bx,0			; in page 0
	mov	cx,cursorrow		; put this in a register temporarily
	mov	dx,cursorcol		; move to this column
	mov	dh,cl			; move to this row
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret
movecursor	endp

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

; ************* Function scrollup(toprow, botrow) ******************

;	Scroll the screen up (from toprow to botrow)

scrollup	proc	uses	es, toprow:word, botrow:word

	mov	ax,0601h		; scropp up one line
	mov	bx,0700h		; new line is black
	mov	cx,toprow		; this row,
	mov	ch,cl			;  ...
	mov	cl,0			;  first column
	mov	dx,botrow		; to this row,
	mov	dh,dl			;  ...
	mov	dl,79			;  last column
	push	bp			; some BIOS's don't save this
	int	10h			; do it.
	pop	bp			; restore the saved register
	ret				; we done.
scrollup	endp


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
	and	ax,andcolor		; (ensure that 'color' is in the range)
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
	mov	ax,rowcount		; sanity check: don't proceed
	cmp	ax,ydots		; beyond the end of the screen
	ja	out_lineret		;  ...
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	si,offset ycolor	; get the color for dot 'x'
        call    linewrite		; mode-specific linewrite routine
        inc	rowcount		; next row
out_lineret:
        xor	ax,ax			; return 0
	ret
out_line	endp


; ****************  EGA Palette <==> VGA DAC Conversion Routines **********

;	paltodac	converts a 16-palette EGA value to a 256-color VGA
;			value (duplicated 16 times)
;	dactopal	converts the first 16 VGA values to a 16-palette
;			EGA value

;	local routines called with register values
;		BH = VGA Red Color	xxRRRRRR
;		BL = VGA Green Color	xxGGGGGG
;		CH = VGA Blue Color	xxBBBBBB
;		CL = EGA Palette	xxrgbRGB
;
;	palettetodac	converts CL to BH/BL/CH
;	dactopalette	converte BH/BL/CH to CL

; *************************************************************************

palettetodac	proc	near
	mov	bx,0			; initialize RGB values to 0
	mov	ch,0			;  ...
	test	cl,20h			; low-red high?
	jz	palettetodac1		;  nope
	or	bh,10h			; set it
palettetodac1:
	test	cl,10h			; low-green high?
	jz	palettetodac2		;  nope
	or	bl,10h			; set it
palettetodac2:
	test	cl,08h			; low-blue high?
	jz	palettetodac3		;  nope
	or	ch,10h			; set it
palettetodac3:
	test	cl,04h			; high-red high?
	jz	palettetodac4		;  nope
	or	bh,20h			; set it
palettetodac4:
	test	cl,02h			; high-green high?
	jz	palettetodac5		;  nope
	or	bl,20h			; set it
palettetodac5:
	test	cl,01h			; high-blue high?
	jz	palettetodac6		;  nope
	or	ch,20h			; set it
palettetodac6:
	ret
palettetodac	endp

dactopalette	proc	near
	mov	cl,0			; initialize RGB values to 0
	test	bh,10h			; low-red high?
	jz	dactopalette1		;  nope
	or	cl,20h			; set it
dactopalette1:
	test	bl,10h			; low-green high?
	jz	dactopalette2		;  nope
	or	cl,10h			; set it
dactopalette2:
	test	ch,10h			; low-blue high?
	jz	dactopalette3		;  nope
	or	cl,08h			; set it
dactopalette3:
	test	bh,20h			; high-red high?
	jz	dactopalette4		;  nope
	or	cl,04h			; set it
dactopalette4:
	test	bl,20h			; high-green high?
	jz	dactopalette5		;  nope
	or	cl,02h			; set it
dactopalette5:
	test	ch,20h			; high-blue high?
	jz	dactopalette6		;  nope
	or	cl,01h			; set it
dactopalette6:
	ret
dactopalette	endp

paltodac	proc	uses es si di
	mov	si,0			; initialize the loop values
	mov	di,0
paltodacloop:
	mov	cl,palettega[si]	; load up a single palette register
	call	palettetodac		; convert it to VGA colors
	mov	dacbox+0[di],bh		; save the red value
	mov	dacbox+1[di],bl		;  and the green value
	mov	dacbox+2[di],ch		;  and the blue value
	inc	si			; bump up the registers
	add	di,3			;  ...
	cmp	si,16			; more to go?
	jne	paltodacloop		;  yup.
	push	ds			; set ES to DS temporarily
	pop	es			;  ...
	mov	ax,15			; do this 15 times to get to 256
	mov	di,offset dacbox+48	; set up the first destination
paltodacloop2:
	mov	cx,24			; copy another block of 16 registers
	mov	si,offset dacbox	; set up for the copy
	rep	movsw			;  do it
	dec	ax			; need to do another block?
	jnz	paltodacloop2		;  yup.  do it.
	ret				;  we done.
paltodac	endp

dactopal	proc	uses es si di
	mov	si,0			; initialize the loop values
	mov	di,0
dactopalloop:
	mov	bh,dacbox+0[di]		; load up the VGA red value
	mov	bl,dacbox+1[di]		;  and the green value
	mov	ch,dacbox+2[di]		;  and the blue value
	call	dactopalette		; convert it to an EGA palette
	mov	palettega[si],cl	; save as a single palette register
	inc	si			; bump up the registers
	add	di,3			;  ...
	cmp	si,16			; more to go?
	jne	dactopalloop		;  yup.
	mov	cl,palettega		; copy palette 0
	mov	palettega+16,cl		;  to the overscan register
	ret				;  we done.
dactopal	endp


; *********************** Function storedac() ****************************

;	Function to Store the dacbox[][] array into VGA,
;	called from loaddac() durring initialization.

storedac	proc
	push	es			; need ES == DS temporarily
	push	ds			;  ...
	pop	es			;  ...
	mov	ax,1012h		; get the old DAC values
	mov	bx,0			;  (assuming, of course, they exist)
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	int	10h			; do it.
	pop	es			;  ...
	ret
storedac endp

; *********************** Function loaddac() ****************************

;	Function to Load the dacbox[][] array, if it can
;	(sets dacbox[0][0] to an invalid '255' if it can't)

loaddac	proc
	mov	reallyega,0		; set flag: not an EGA posing as a VGA
	cmp	dotmode,9		; TARGA 3 June 89 j mclain
	je	loaddacdone
	cmp	f85flag, 0
	jne	loaddacdone
	cmp	loadPalette,1		; TARGA/VGA 3 June 89 j mclain
	jne	normalLoadDac
	call	storedac
	jmp	short loaddacdone
normalLoadDac:
	mov	dacbox,255		; a flag value to detect invalid DAC
	cmp	debugflag,16		; pretend we're not a VGA?
	je	loaddacdebug		;  yup.
	push	es			; need ES == DS temporarily
	push	ds			;  ...
	pop	es			;  ...
	mov	ax,1017h		; get the old DAC values
	mov	bx,0			;  (assuming, of course, they exist)
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	push	bp			; some older BIOSes don't save this
	int	10h			; do it.
	pop	bp			; restore registers
	pop	es			;  ...
loaddacdebug:
	cmp	dacbox,255		; did it work?  do we have a VGA?
	jne	loaddacdone		;  yup.
	cmp	colors,16		; are we using 16 or more colors?
	jb	loaddacdone		;  nope.  forget it.
	cmp	ydots,350		; 640x350 range?
	jb	loaddacdone		;  nope.  forget it.
	mov	bx,offset palettega	; make up a dummy palette
	mov	cx,3800h		; start with color 0 == black
loaddacega1:				; and        color 8 == low-white
	mov	0[bx],cl		; save one color
	mov	8[bx],ch		; and another color
	inc	bx			; bump up the DAC
	add	cx,0101h		; and the colors
	cmp	cl,8			; finished 8 colors?
	jne	loaddacega1		;  nope.  get more.
	mov	reallyega,1		; note that this is really an EGA
	call	far ptr paltodac	; "convert" it to a VGA DAC
	mov	daclearn,1		; bypass learn mode
	mov	ax,cyclelimit		;  and spin as fast as he wants
	mov	daccount,ax		;  ...
loaddacdone:
	ret
loaddac	endp

; *************** Function spindac(direction, rstep) ********************

;	Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;	in "rstep" increments - or, if "direction" is 0, just replace it.

spindac	proc	uses di si es, direction:word, rstep:word
	cmp	dotmode,9		; TARGA 3 June 89 j mclain
	je	spinbailout
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

	cmp	f85flag, 0		; if 8514a then update pallette
	je	spindoit
	jmp	spin8514

spindoit:
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
	jz	retrace2		;  so loop until it goes high
	cmp	reallyega,1		; is this really an EGA?
	je	spinega			;  yup.  spin it that way.
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
	jmp	spindone		; jump to common code
spinega:
	cmp	bx,0			; skip this if not the first time thru
	jne	spindone		;  ...
	push	bx			; save some registers
	push	cx			;  aroud the call
	call	far ptr dactopal	; convert the VGA DAC to an EGA palette
	pop	cx			; restore the registers
	pop	bx			;  from prior to the call
	mov	ax,1002h		; update the EGA palette
	mov	dx,offset palettega	;  ...
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

spin8514:
	call    w8514pal

spindacreturn:
	ret
spindac	endp
	end

