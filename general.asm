;	Generic assembler routines that have very little at all
;	to do with fractals.
;
; ---- Overall Support
;
;	initasmvars()
;
; ---- Video Routines 
;
;	setvideomode()
;	getcolor()
;	putcolor()
;	out_line()
;	drawbox()
;	home()
;	clscr()
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
; ---- Quick-copy to/from Extraseg support
;
;	toextra()
;	fromextra()
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
; ---- 32-bit Multiply (includes 16-bit emulation)
;
;	multiply()
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
IFDEF ??Version
	MASM51
	QUIRKS
ENDIF

	.MODEL  medium,c

	.8086

        ; these must NOT be in any segment!!
        ; this get's rid of TURBO-C fixup errors

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


	extrn   open8514  :far      ; start 8514a
	extrn   reopen8514:far      ; restart 8514a
	extrn   close8514 :far      ; stop 8514a
	extrn   fr85wdot  :far      ; 8514a write dot
	extrn   fr85wbox  :far      ; 8514a write box
	extrn   fr85rdot  :far      ; 8514a read dot
	extrn   w8514pal  :far      ; 8514a pallete update

        extrn   help:far		; help code (in help.c)

.DATA

; ************************ External variables *****************************

	extrn	oktoprint: word		; flag: == 1 if printf() will work
	extrn	dotmode: word		; video mode:   1 = use the BIOS (yuck)
;							2 = use EGA/VGA style
;							3 = use MCGA style
;							4 = use Tseng style
;							5 = use Paradise 
;							6 = use Video 7
;							7 = MVGA 256 color
;							9 = use TARGA
;							10= Hercules (someday)
;							11= "disk" video
;							12= 8514/A 
;							13= CGA 4,2-color

	extrn	sound:word		; if 0, supress sounds
	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word	; maximum iterations, colors
	extrn	ixmin:word, ixmax:word	; for zoom/pan: x and y limits
	extrn	iymin:word, iymax:word	;  for the local zoom-box
	extrn	cyclelimit:word		; limiting factor for DAC-cycler
	extrn	debugflag:word		; for debugging purposes only

	extrn	boxcount:word		; (previous) box pt counter: 0 if none.

	extrn	xorTARGA:word		; TARGA 3 June 89 j mclain
					; flag says xor pixels for box

; ************************ Public variables *****************************

public		cpu			; used by 'calcmand'
public		fpu			; will be used by somebody someday
public		andcolor		; used by 'calcmand'
public		lookatmouse		; used by 'calcfrac'
public		_dataseg		; used by TARGA, other Turbo Code

public		loadPalette		; flag for loading VGA/TARGA palette from disk
public		dacbox			; GIF saves use this
public		daclearn, daccount	; Rotate may want to use this
public		extraseg		; extra 64K segment, if any
public		rowcount		; row-counter for decoder and out_line

;		arrays declared here, used elsewhere
;		arrays not used simultaneously are deliberately overlapped

public		lx0,ly0				; used by FRACTINT, CALCFRAC
public		prefix, suffix, dstack, decoderline	; Used by the Decoder
public		strlocn, entrynum, teststring	; used by the Encoder
public		olddacbox			; temporary DAC saves
public		diskline			; Used by the Diskvid rtns
public		rlebuf				; Used ty the TARGA En/Decoder
public		reallyega			; flag: 1 if "VGA" is an EGA

; ************************* "Shared" array areas **************************

lx0		dd	0		; 8K X-pixel value array
prefix		dw	4096 dup(0)	; 8K Decoder array

ly0		dd	0		; 8K y-pixel value array
suffix		dw	2048 dup(0)	; 4K Decoder array
dstack		dw	2048 dup(0)	; 4K Decoder array

strlocn		dw	0		; 8K Encoder array
decoderline	db	0		; 2K Decoder array
olddacbox	db	0		; (256*3) temporary dacbox values
boxx		dw	2048 dup(0)	; (previous) box data points - x axis
boxy		dw	2048 dup(0)	; (previous) box data points - y axis

entrynum	dw	0		; 8K Encoder array
rlebuf		db	0		; TARGA encoder array
boxvalues	db	2048 dup(0)	; (previous) box color values
		db	6144 dup(0)	; fluff up to 8K

diskline	db	2049 dup(0)	; 2K Diskvideo array

teststring	db	100  dup(0)	; 100 byte Encoder array

; ************************ Internal variables *****************************

cpu		dw	0		; cpu type: 86, 186, 286, or 386
fpu		dw	0		; fpu type: 0, 87, 287, 387
_dataseg	dw	0		; our "near" data segment

dotwrite	dw	0		; write-a-dot routine:  mode-specific
dotread		dw	0		; read-a-dot routine:   mode-specific
linewrite	dw	0		; write-a-line routine: mode-specific
andcolor	dw	0		; "and" value used for color selection
color		db	0		; the color to set a pixel
diskflag	db	0		; special "disk-video" flag
tgaflag		db	0		; TARGA 28 May 89 - j mclain
loadPalette	db	0		; TARGA/VGA load palette from disk

f85flag		db	0		;flag for 8514a

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

orvideo		db	0		; "or" value for setvideo
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
;	and John Bridges ( mostly John Bridges, who wrote 'newbank')

;	The original "newbank" routine carries John's Copyright,
;	duplicated below:
;
;
;	Copyright 1988,89 John Bridges
;	Free for use in commercial, shareware or freeware applications
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
	sti
	pop	dx
	pop	ax
	ret

nopd:
	cmp	dotmode,15	;Trident
	jne	notri
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

	mov	ah,current_bank
	xor	ah,2
	mov	dx,3c4h
	mov	al,0eh
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

notri:
	cmp	dotmode,16	; Chips & Technologies
	jne	noct
	mov	ah,al
	mov	al,10h
	mov	dx,3d6h
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret

noct:
	cmp	dotmode,17	;ATI VGA Wonder
	jne	noati
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

noati:
	sti
	pop	dx
	pop	ax
	ret
newbank	endp

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
	mov	bx,dotmode		; set up for a video table jump
	cmp	bx,20			; are we within the range of dotmodes?
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
	dw	offset super256mode	; mode 4
	dw	offset super256mode	; mode 5
	dw	offset super256mode	; mode 6
	dw	offset tweak256mode	; mode 7
	dw	offset tweak400mode	; mode 8
	dw	offset targaMode	; mode 9 
	dw	offset dullnormalmode	; mode 10
	dw	offset diskmode		; mode 11
	dw	offset f8514mode	; mode 12
	dw	offset cgacolor		; mode 13
	dw	offset dullnormalmode	; mode 14
	dw	offset super256mode	; mode 15
	dw	offset super256mode	; mode 16
	dw	offset super256mode	; mode 17
	dw	offset dullnormalmode	; mode 18
	dw	offset dullnormalmode	; mode 19
	dw	offset dullnormalmode	; mode 20

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
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up EGA/VGA read-a-dot  routine
	mov	cx,offset vgaline 	; set up the EGA/VGA linewrite routine
	jmp	videomode		; return to common code
super256mode:
	mov	ax,offset super256write	; set up superVGA write-a-dot routine
	mov	bx,offset super256read	; set up superVGA read-a-dot  routine
	mov	cx,offset super256line 	; set up the " linewrite routine
	jmp	videomode		; return to common code
tweak256mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak256write	; set up tweaked-256 write-a-dot
	mov	bx,offset tweak256read	; set up tweaked-256 read-a-dot
	mov	cx,offset tweak256line 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
tweak400mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak400write	; set up tweaked-400 write-a-dot
	mov	bx,offset tweak400read	; set up tweaked-400 read-a-dot
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
cgacolor:
	mov	ax,offset cgawrite	; set up CGA write-a-dot
	mov	bx,offset cgaread	; set up CGA read-a-dot
	mov	cx,offset normaline 	; set up the normal linewrite routine
	jmp	videomode		; return to common code
diskmode:
	call	diskstart		; start up the disk routines
	mov	ax,offset diskwrite	; set up disk-vid write-a-dot routine
	mov	bx,offset diskread	; set up disk-vid read-a-dot routine
	mov	cx,offset normaline 	; set up the normal linewrite routine
	mov	diskflag,1		; flag "disk-end" needed.
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

	mov	ax,0012h		; invoke video mode 12h
	call	maybeor			; maybe or AL or (for Video-7s) BL
	int	10h			; do it.

        cmp	dx,8			; tweak256 mode?
        jne	not256			; if not, start tweak256-specific code

	mov	ax,0013h		; invoke video mode 13h
	call	maybeor			; maybe or AL or (for Video-7s) BL
	int	10h			; previous mode 12h cleared video
					;   memory - dirty but effective
	mov	dx,3c4h			; alter sequencer registers
	mov	ax,0604h		; disable chain 4
	out	dx,ax

	jmp	short is256		; forget the ROM characters

not256:	mov	ax,1124h		; load ROM 8*16 characters
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
	sti				; restore interrupts

	pop	es			; restore ES

setvideoreturn:

	mov	current_bank,0ffh	; stuff impossible value into cur-bank

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

; =======================================================
;
;	quick-and-dirty (no error/overflow checks performed)
;	32-bit integer multiply routine with an 'n'-bit shift.
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

temp	dw	4 dup(0)		; temporary 64-bit result goes here
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
fastm1:	shrd	eax,edx,cl		; shift down 'n' bits
	push	eax			; save the 64-bit result
	pop	ax			; low-order  16 bits
	pop	dx			; high-order 16 bits
	jmp	multiplyreturn		; back to common code

.8086					; 386-specific code ends here

slowmultiply:				; (sigh)  time to do it the hard way...

	mov	ax,0
	mov	temp+4,ax		; first, zero out the (temporary)
	mov	temp+6,ax		;  result

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
	mov	bx,temp+7		;  ...
	jmp	short multc4		; branch to common code
multc1:	cmp	cx,16			; shifting by two bytes or more?
	jl	multc2			;  nope.  check for something else
	sub	cx,16			; quick-shift 16 bits
	mov	ax,temp+2		; load up the registers
	mov	dx,temp+4		;  ...
	mov	bx,temp+6		;  ...
	jmp	short multc4		; branch to common code
multc2:	cmp	cx,8			; shifting by one byte or more?
	jl	multc3			;  nope.  check for something else
	sub	cx,8			; quick-shift 8 bits
	mov	ax,temp+1		; load up the registers
	mov	dx,temp+3		;  ...
	mov	bx,temp+5		;  ...
	jmp	short multc4		; branch to common code
multc3:	mov	ax,temp			; load up the regs
	mov	dx,temp+2		;  ...
	mov	bx,temp+4		;  ...
multc4:	cmp	cx,0			; done shifting?
	je	multc5			;  yup.  bail out

multloop:
	shr	bx,1			; shift down 1 bit, cascading
	rcr	dx,1			;  ...
	rcr	ax,1			;  ...
	loop	multloop		; try the next bit, if any
multc5:

	cmp	sign,0			; should we negate the result?
	je	mults5			;  nope.
	not	ax			;  yup.  flip signs.
	not	dx			;   ...
	mov	bx,0			;   ...
	stc				;   ...
	adc	ax,bx			;   ...
	adc	dx,bx			;   ...
mults5:

multiplyreturn:				; that's all, folks!
	ret
multiply	endp


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
 	call	chkmse				; see if the mouse was used
 	jc	short getakey4			; ax holds the phoney key
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

	mov	dx,1			; ask for 64K of far space
	mov	ax,0			;  ...
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

