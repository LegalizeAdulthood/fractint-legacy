;	Generic assembler routines having to do with video adapter
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
;	putstr()
;	scroll()
;	storedac()		TARGA - NEW - 3 June 89 j mclain
;	loaddac()
;	spindac()
;	asmdotwrite		(called by CALCMAND.ASM using registers)
;	asmvideocleanup 		""
;	adapter_detect
;	setnullvideo
;
; ---- Help (Video) Support
;
;	setfortext()
;	setforgraphics()
;	setclear()
;	findfont()
;

;			 required for compatibility if Turbo ASM
IFDEF ??version
	MASM51
	QUIRKS
ENDIF

	.MODEL	medium,c

	.8086

	; these must NOT be in any segment!!
	; this get's rid of TURBO-C fixup errors

	extrn	startvideo:far		; start your-own-video routine
	extrn	readvideo:far		; read	your-own-video routine
	extrn	writevideo:far		; write your-own-video routine
	extrn	endvideo:far		; end	your-own-video routine
	extrn	readvideopalette:far	; read-your-own-palette routine
	extrn	writevideopalette:far	; write-your-own-palette routine

	extrn	startdisk:far		; start disk-video routine
	extrn	readdisk:far		; read	disk-video routine
	extrn	writedisk:far		; write disk-video routine
	extrn	enddisk:far		; end	disk-video routine

	extrn	buzzer:far		; nyaah, nyaah message

; TARGA 28 May 80 - j mclain
	extrn	StartTGA  :far		; start TARGA
	extrn	ReadTGA   :far		; read	TARGA
	extrn	WriteTGA  :far		; write TARGA
	extrn	LineTGA   :far		; line	TARGA
	extrn	EndTGA	  :far		; end	TARGA


; 8514/A routines
	extrn	open8514  :far	    ; start 8514a
	extrn	reopen8514:far	    ; restart 8514a
	extrn	close8514 :far	    ; stop 8514a
	extrn	fr85wdot  :far	    ; 8514a write dot
	extrn	fr85wbox  :far	    ; 8514a write box
	extrn	fr85rdot  :far	    ; 8514a read dot
	extrn	fr85rbox  :far	    ; 8514a read box
;	extrn	fr85zoom  :far	    ; 8514a zoom box
	extrn	w8514pal  :far	    ; 8514a pallete update

; Hercules Routines
	extrn	inithgc   :far	    ; Initialize Hercules card graphics mode
	extrn	termhgc   :far	    ; Terminate Hercules card graphics mode
	extrn	writehgc  :far	    ; Hercules write dot
	extrn	readhgc   :far	    ; Hercules read dot
	extrn	linehgc   :far	    ; Hercules line draw
	extrn	charhgc   :far	    ; Draw an ASCII char in Hercules graphics

.DATA

; ************************ External variables *****************************

	extrn	oktoprint: word 	; flag: == 1 if printf() will work
	extrn	videoentry:byte 	; video table entry flag
	extrn	dotmode: word		; video mode (see the comments
					; in front of the internal video
					; table for legal dot modes)

	extrn	xdots:word, ydots:word	; number of dots across and down
	extrn	maxit:word, colors:word ; maximum iterations, colors
	extrn	cyclelimit:word 	; limiting factor for DAC-cycler
	extrn	debugflag:word		; for debugging purposes only

	extrn	boxcount:word		; (previous) box pt counter: 0 if none.
	extrn	boxx:word, boxy:word	; zoom-box save-value locations
	extrn	boxvalues:byte		; zoom-box save-pixel locations

	extrn	xorTARGA:word		; TARGA 3 June 89 j mclain
					; flag says xor pixels for box

	extrn	cpu:word		; CPU type (86, 186, 286, or 386)
	extrn	extraseg:word		; location of the EXTRA segment

	extrn	suffix:word		; (safe place during video-mode switches)

; ************************ Public variables *****************************

public		andcolor		; used by 'calcmand'

public		loadPalette		; flag for loading VGA/TARGA palette from disk
public		dacbox			; GIF saves use this
public		daclearn, daccount	; Rotate may want to use this
public		rowcount		; row-counter for decoder and out_line
public		reallyega		; "really an EGA" (faking a VGA) flag
public		video_type		; video adapter type
public		boxcolor		; zoom box color

;		arrays declared here, used elsewhere
;		arrays not used simultaneously are deliberately overlapped

; ************************ Internal variables *****************************

goodmode	dw	0		; if non-zero, OK to read/write pixels
dotwrite	dw	0		; write-a-dot routine:	mode-specific
dotread 	dw	0		; read-a-dot routine:	mode-specific
linewrite	dw	0		; write-a-line routine: mode-specific
lineread	dw	0		; read-a-line routine: mode-specific
andcolor	dw	0		; "and" value used for color selection
color		db	0		; the color to set a pixel
videoflag	db	0		; special "your-own-video" flag
diskflag	db	0		; special "disk-video" flag
tgaflag 	db	0		; TARGA 28 May 89 - j mclain
loadPalette	db	0		; TARGA/VGA load palette from disk

f85flag 	db	0		;flag for 8514a

HGCflag 	db	0		;flag for Hercules Graphics Adapter

;					; Zoom-Box values (2K x 2K screens max)
boxcolor	dw	0		; Zoom-Box color
reallyega	dw	0		; 1 if its an EGA posing as a VGA
palettega	db	17 dup(0)	; EGA palette registers go here
daclearn	db	0		; 0 if "learning" DAC speed
dacnorm 	db	0		; 0 if "normal" DAC update
daccount	dw	0		; DAC registers to update in 1 pass
dacbox		db	773 dup(0)	; DAC goes here
saved_dacreg	dw	0ffffh,0,0,0	; saved DAC register goes here

rowcount	dw	0		; row-counter for decoder and out_line

orvideo 	db	0		; "or" value for setvideo
videomem	dw	0a000h		; VGA videomemory
videoax 	dw	0		; graphics mode values: ax
videobx 	dw	0		; graphics mode values: bx
videocx 	dw	0		; graphics mode values: cx
videodx 	dw	0		; graphics mode values: dx

video_type	dw	0		; video adapter type.  Legal values:
					;   0  = not yet determined
					;   1  = Hercules (not yet checked)
					;   2  = CGA
					;   3  = EGA
					;   4  = VGA
					;   5  = VESA (not yet checked)
					;  11  = 8514/A (not yet checked)
					;  12  = TIGA	(not yet checked)
					;  13  = TARGA	(not yet checked)
video_entries	dw	0		; offset into video_entries table
video_bankadr	dw	0		; offset  of  video_banking routine
video_bankseg	dw	0		; segment of  video_banking routine

badvideomsg	db	13,10
		db	"That video mode is not available with your adapter."
		db	13,10
		db	"Please select another video mode.$"

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


x320y400	db	320/8			; number of screen columns
		db	400/16			; number of screen rows
		db	 5fh, 4fh, 50h, 82h	; CRTC Registers
		db	 54h, 80h,0bfh, 1fh
		db	 00h, 40h, 00h, 00h
		db	 00h, 00h, 00h, 00h
		db	 9ch, 8eh, 8fh, 28h
		db	 00h, 96h,0b9h,0E3h
		db	0FFh

x640y400	db	640/8			; number of screen columns
		db	400/16			; number of screen rows
		db	 5eh, 4fh, 50h, 01h	; CRTC Registers
		db	 54h, 9fh,0c0h, 1fh
		db	 00h, 40h, 00h, 00h
		db	 00h, 00h, 00h, 00h
		db	 9ch,08eh, 8fh, 28h
		db	 00h, 95h,0bch,0c3h
		db	 0ffh
;for VGA
x400y600	db	400/8
		db	600/16
		db	74h,63h,64h,97h
		db	68h,95h,86h,0F0h
		db	00h,60h,00h,00h
		db	00h,00h,00h,31h
		db	5Bh,8Dh,57h,32h
		db	0h,60h,80h,0E3h
		db	0FFh
;for VGA
x376y564	db	376/8
		db	564/16
		db	6eh,5dh,5eh,91h
		db	62h,8fh,62h,0F0h
		db	00h,60h,00h,00h
		db	00h,00h,00h,31h
		db	37h,89h,33h,2fh
		db	0h,3ch,5ch,0E3h
		db	0FFh
;for VGA
x400y564	db	400/8
		db	564/16
		db	74h,63h,64h,97h
		db	68h,95h,62h,0F0h
		db	00h,60h,00h,00h
		db	00h,00h,00h,31h
		db	37h,89h,33h,32h
		db	0h,3ch,5ch,0E3h
		db	0FFh

testati 	db	832/8
		db	612/16
		db	7dh,65h,68h,9fh
		db	69h,92h,44h,1Fh
		db	00h,00h,00h,00h
		db	00h,00h,00h,00h
		db	34h,86h,37h,34h
		db	0fh,34h,40h,0E7h
		db	0FFh

tweaks		dw	offset x704y528 	; tweak table
		dw	offset x704y528
		dw	offset x720y540
		dw	offset x736y552
		dw	offset x752y564
		dw	offset x768y576
		dw	offset x784y588
		dw	offset x800y600
		dw	offset x360y480
		dw	offset x320y400
		dw	offset x640y400 	; Tseng Super VGA
		dw	offset x400y600 	; new tweak (VGA)
		dw	offset x376y564 	; new tweak (VGA)
		dw	offset x400y564 	; new tweak (VGA)
		dw	offset x720y540 	; ATI Tweak
		dw	offset x736y552 	; ATI Tweak
		dw	offset x752y564 	; ATI Tweak
		dw	offset testati		; ATI 832x816 (works!)

tweaktype	dw	0			; 8 or 9 (320x400 or 360x480)

.CODE

;			Video Table Entries
;
;	The Video Table has been moved to a FARDATA segment to relieve
;	some of the pressure on the poor little overloaded 64K DATA segment.

;	This code has three entry points:
;
;	sizeoftable = initvideotable(); 	returns the # of table entries
;
;	fromvideotable(i);			copies table entry #i into
;						the videoentry structure
;
;	tovideotable(i);			moves the videoentry structure
;						into the table at entry #i


.code

video_requirements	dw	0		; minimal video_type req'd
	dw	1, 3, 4, 4, 4, 4, 4, 4, 1, 1	; dotmodes  1 - 10
	dw	1, 4, 2, 1, 4, 4, 4, 4, 1, 4	; dotmodes 11 - 20
	dw	4, 4, 4, 4, 4, 4, 4, 4, 4, 4	; dotmodes 21 - 30

videotable	db	0	; video table actually starts on the NEXT byte

;	Feel free to add your favorite video adapter to the following table.
;	Just remember that only the first 100 or so entries get displayed and
;	assigned Function keys.

;	Currently available Video Modes are (use the BIOS as a last resort)

;		1) use the BIOS (INT 10H, AH=12/13, AL=color) ((SLOW))
;		2) pretend it's a (perhaps super-res) EGA/VGA
;		3) pretend it's an MCGA
;		4) SuperVGA 256-Color mode using the Tseng Labs Chipset
;		5) SuperVGA 256-Color mode using the Paradise Chipset
;		6) SuperVGA 256-Color mode using the Video-7 Chipset
;		7) Non-Standard IBM VGA 360 x 480 x 256-Color mode
;		8) SuperVGA 1024x768x16 mode for the Everex Chipset
;		9) TARGA video modes
;		10) HERCULES video mode
;		11) Non-Video [disk or RAM] "video"
;		12) 8514/A video modes
;		13) CGA 320x200x4-color and 640x200x2-color modes
;		14) Tandy 1000 640x200x16 mode
;		15) SuperVGA 256-Color mode using the Trident Chipset
;		16) SuperVGA 256-Color mode using the Chips & Tech Chipset
;		17) SuperVGA 256-Color mode using the ATI VGA Wonder Chipset
;		18) SuperVGA 256-Color mode using the Everex Chipset
;		19) Roll-Your-Own video, as defined in YOURVID.C
;		20) SuperVGA 1024x768x16 mode for the ATI VGA Wonder Chipset
;		21) SuperVGA 1024x768x16 mode for the Tseng Labs Chipset
;		22) SuperVGA 1024x768x16 mode for the Trident Chipset
;		23) SuperVGA 1024x768x16 mode for the Video 7 Chipset
;		24) SuperVGA 1024x768x16 mode for the Paradise Chipset
;		25) SuperVGA 1024x768x16 mode for the Chips & Tech Chipset
;		26) SuperVGA 1024x768x16 mode for the Everex Chipset
;		27) SuperVGA Auto-Detect mode
;		28) VESA modes

;	(Several entries have been commented out - they should/did work,
;	but are handled by alternative entries.  Where multiple SuperVGA
;	entries are covered by a single SuperVGA Autodetect mode, the
;	individual modes have been commented out.  Where a SuperVGA
;	Autodetect mode covers only one brand of adapter, the Autodetect
;	mode has been commented out to avoid confusion.)

;		|--Adapter/Mode-Name------|-------Comments-----------|

;		|------INT 10H------|Dot-|--Resolution---|
;		|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|

;	(low-rez EGA moved because F1 is now Help)
	db	"IBM 16-Color EGA          Standard EGA hi-res mode  "
	dw	  10h,	 0,   0,   0,	2, 640, 350,  16
	db	"IBM 256-Color MCGA        Quick and LOTS of colors  "
	dw	  13h,	 0,   0,   0,	3, 320, 200, 256
	db	"IBM 16-Color VGA          Nice high resolution      "
	dw	  12h,	 0,   0,   0,	2, 640, 480,  16
	db	"IBM 4-Color CGA           (Ugh - Yuck - Bleah)      "
	dw	   4h,	 0,   0,   0,  13, 320, 200,   4
	db	"IBM Hi-Rez B&W CGA        ('Hi-Rez' Ugh - Yuck)     "
	dw	   6h,	 0,   0,   0,  13, 640, 200,   2
	db	"IBM B&W EGA               (Monochrome EGA)          "
	dw	  0fh,	 0,   0,   0,	2, 640, 350,   2
	db	"IBM B&W VGA               (Monochrome VGA)          "
	dw	  11h,	 0,   0,   0,	2, 640, 480,   2
	db	"IBM Low-Rez EGA           Quick but chunky          "
	dw	  0dh,	 0,   0,   0,	2, 320, 200,  16
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,	 0,   0,   9,	7, 320, 400, 256
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,	 0,   0,   8,	7, 360, 480, 256
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27, 800, 600,  16
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27,1024, 768,  16
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27, 640, 400, 256
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27, 640, 480, 256
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27, 800, 600, 256
	db	"SuperVGA/VESA Autodetect  Works with most SuperVGA  "
	dw	    0,	 0,   0,   0,  27,1024, 768, 256
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,102h,   0,   0,  28, 800, 600,  16
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,104h,   0,   0,  28,1024, 768,  16
	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
	dw	4f02h,106h,   0,   0,  28,1280,1024,  16
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,100h,   0,   0,  28, 640, 400, 256
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,101h,   0,   0,  28, 640, 480, 256
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,103h,   0,   0,  28, 800, 600, 256
;	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
;	dw	4f02h,105h,   0,   0,  28,1024, 768, 256
	db	"VESA Standard interface   OK: Andy Fu - Chips&Tech  "
	dw	4f02h,107h,   0,   0,  28,1280,1024, 256
	db	"8514/A Low  Res           Requires IBM's HDILOAD    "
	dw	   3h,	 0,   0,   1,  12, 640, 480, 256
	db	"8514/A High Res           Requires IBM's HDILOAD    "
	dw	   3h,	 0,   0,   1,  12,1024, 768, 256
	db	"8514/A Low  W/Border      Requires IBM's HDILOAD    "
	dw	   3h,	 0,   0,   1,  12, 632, 474, 256
	db	"8514/A High W/Border      Requires IBM's HDILOAD    "
	dw	   3h,	 0,   0,   1,  12,1016, 762, 256
	db	"IBM Med-Rez EGA           (Silly but it's there!)   "
	dw	  0eh,	 0,   0,   0,	2, 640, 200,  16
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,	 0,   0,  12,	7, 376, 564, 256
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,	 0,   0,  13,	7, 400, 564, 256
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,	 0,   0,  11,	7, 400, 600, 256
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 60h,   0,   0,	2, 752, 410,  16
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 61h,   0,   0,	2, 720, 540,  16
;	db	"Video-7 Vram VGA          OK: Ira Emus              "
;	dw	6f05h, 62h,   0,   0,	2, 800, 600,  16
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 63h,   0,   0,	1,1024, 768,   2
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 64h,   0,   0,	1,1024, 768,   4
;	db	"Video-7 Vram VGA w/512K   OK: Sandy & Frank Lozier  "
;	dw	6f05h, 65h,   0,   0,  23,1024, 768,  16
;	db	"Video-7 Vram VGA          OK: Michael Kaufman       "
;	dw	6f05h, 66h,   0,   0,	6, 640, 400, 256
;	db	"Video-7  w/512K           OK: Greg Reznick          "
;	dw	6f05h, 67h,   0,   0,	6, 640, 480, 256
	db	"Video-7  w/512K           OK: Greg Reznick          "
	dw	6f05h, 68h,   0,   0,	6, 720, 540, 256
;	db	"Video-7  w/512K           OK: Greg Reznick          "
;	dw	6f05h, 69h,   0,   0,	6, 800, 600, 256
;	db	"Tseng SuperVGA tweaked    (adds missing Tseng mode) "
;	dw	   0h,	 0,   0,  10,	4, 640, 400, 256
;	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
;	dw	  2eh,	 0,   0,   0,	4, 640, 480, 256
;	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
;	dw	  29h,	 0,   0,   0,	2, 800, 600,  16
;	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
;	dw	  30h,	 0,   0,   0,	4, 800, 600, 256
;	db	"Orchid/STB/GENOA/SIGMA    OK: Timothy Wegner        "
;	dw	  37h,	 0,   0,   0,  21,1024, 768,  16
	db	"GENOA/STB                 OK: Timothy Wegner        "
	dw	  2dh,	 0,   0,   0,	4, 640, 350, 256
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  27h,	 0,   0,   0,	2, 720, 512,  16
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  2fh,	 0,   0,   0,	4, 720, 512, 256
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  7ch,	 0,   0,   0,	2, 512, 512,  16
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  7dh,	 0,   0,   0,	4, 512, 512, 256
	db	"STB                       UNTESTED: may not work    "
	dw	  36h,	 0,   0,   0,	1, 960, 720,  16
;	db	"Everex EVGA               OK: Travis Harrison       "
;	dw	  70h,	 0,   0,   0,	2, 640, 480,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h,	1h,   0,   0,	2, 752, 410,  16
;	db	"Everex EVGA               OK: Travis Harrison       "
;	dw	  70h,	2h,   0,   0,	2, 800, 600,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 11h,   0,   0,	1,1280, 350,   4
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 12h,   0,   0,	1,1280, 600,   4
	db	"Everex EVGA               UNTESTED: may not work    "
	dw	  70h, 13h,   0,   0,  18, 640, 350, 256
;	db	"Everex EVGA               UNTESTED: may not work    "
;	dw	  70h, 14h,   0,   0,  18, 640, 400, 256
	db	"Everex EVGA               UNTESTED: may not work    "
	dw	  70h, 15h,   0,   0,  18, 512, 480, 256
	db	"ATI EGA Wonder            OK: Garrett Wollman       "
	dw	  51h,	 0,   0,   0,	1, 640, 480,  16
	db	"ATI EGA Wonder            OK: Garrett Wollman       "
	dw	  52h,	 0,   0,   0,	1, 800, 560,  16
	db	"ATI VGA Wonder (512K)     Tweak - OK: M. Burkey     "
	dw	   0h,	 0,   0,  14,  17, 720, 540, 256
	db	"ATI VGA Wonder (512K)     Tweak - OK: M. Burkey     "
	dw	   0h,	 0,   0,  15,  17, 736, 552, 256
	db	"ATI VGA Wonder (512K)     Tweak - OK: M. Burkey     "
	dw	   0h,	 0,   0,  16,  17, 752, 564, 256
	db	"ATI VGA Wonder (512K)     Tweak - OK: M. Burkey     "
	dw	   0h,	 0,   0,  17,  17, 832, 612, 256
;	db	"ATI VGA Wonder            OK: Henry So              "
;	dw	  54h,	 0,   0,   0,	2, 800, 600,  16
;	db	"ATI VGA Wonder            OK: Mark Peterson         "
;	dw	  61h,	 0,   0,   0,  17, 640, 400, 256
;	db	"ATI VGA Wonder (512K)     OK: Mark Peterson         "
;	dw	  62h,	 0,   0,   0,  17, 640, 480, 256
;	db	"ATI VGA Wonder (512K)     OK: Mark Peterson         "
;	dw	  63h,	 0,   0,   0,  17, 800, 600, 256
;	db	"ATI VGA Wonder (512K)     OK: Mark Peterson         "
;	dw	  65h,	 0,   0,   0,  20,1024, 768,  16
	db	"Paradise EGA-480          UNTESTED: may not work    "
	dw	  50h,	 0,   0,   0,	1, 640, 480,  16
;	db	"Pdise/AST/COMPAQ VGA      OK: Tom Devlin            "
;	dw	  5eh,	 0,   0,   0,	5, 640, 400, 256
;	db	"Pdise/AST/COMPAQ VGA      OK: Phil Wilson           "
;	dw	  5fh,	 0,   0,   0,	5, 640, 480, 256
;	db	"Pdise/AST/COMPAQ VGA      OK: by Chris Green        "
;	dw	  58h,	 0,   0,   0,	2, 800, 600,  16
	db	"Pdise/AST/COMPAQ VGA      OK: Phil Wilson           "
	dw	  59h,	 0,   0,   0,	1, 800, 600,   2
;	db	"TRIDENT Chipset           OK: Warren Gold           "
;	dw	  5bh,	 0,   0,   0,	2, 800, 600,  16
;	db	"TRIDENT Chipset           OK: Warren Gold           "
;	dw	  5ch,	 0,   0,   0,  15, 640, 400, 256
;	db	"TRIDENT Chipset           OK: Warren Gold           "
;	dw	  5dh,	 0,   0,   0,  15, 640, 480, 256
;	db	"TRIDENT Chipset           OK: Warren Gold           "
;	dw	  5eh,	 0,   0,   0,  15, 800, 600, 256
;	db	"TRIDENT Chipset           OK: Larry Rosen           "
;	dw	  5fh,	 0,   0,   0,  22,1024, 768,  16
;	db	"Chips & Tech Chipset      OK: Andy Fu               "
;	dw	  78h,	 0,   0,   0,  16, 640, 400, 256
;	db	"Chips & Tech Chipset      OK: Andy Fu               "
;	dw	  79h,	 0,   0,   0,  16, 640, 480, 256
;	db	"Chips & Tech Chipset      OK: Andy Fu               "
;	dw	  7bh,	 0,   0,   0,  16, 800, 600, 256
;	db	"Chips & Tech Chipset      OK: Andy Fu               "
;	dw	  70h,	 0,   0,   0,	2, 800, 600,  16
;	db	"Chips & Tech Chipset      UNTESTED: May not work    "
;	dw	  72h,	 0,   0,   0,  25,1024, 768,  16
	db	"GENOA Super EGA           UNTESTED: May not work    "
	dw	  73h,	 0,   0,   0,	2, 640, 480,  16
	db	"GENOA Super EGA           UNTESTED: May not work    "
	dw	  79h,	 0,   0,   0,	2, 800, 600,  16
	db	"COMPAQ Portable 386       OK: Michael Kaufman       "
	dw	  40h,	 0,   0,   0,	1, 640, 400,   2
	db	"Tandy 1000 16 Clr LoRez   OK: Tom Price             "
	dw	   8h,	 0,   0,   0,	1, 160, 200,  16
	db	"Tandy 1000 16 Color CGA   OK: Tom Price             "
	dw	   9h,	 0,   0,   0,	1, 320, 200,  16
	db	"Tandy 1000 4 Color hi-rez OK: Tom Price             "
	dw	  0ah,	 0,   0,   0,	1, 640, 200,   4
	db	"Tandy 1000 16 Colr hi-rez UNTESTED: May not work    "
	dw	 0ffh,	 0,   0,   0,  14, 640, 200,  16
	db	"AT&T 6300                 UNTESTED: may not work    "
	dw	  41h,	 0,   0,   0,	1, 640, 200,  16
	db	"AT&T 6300                 OK: Michael Kaufman       "
	dw	  40h,	 0,   0,   0,	1, 640, 400,   2
	db	"AT&T 6300                 OK: Colby Norton          "
	dw	  42h,	 0,   0,   0,	1, 640, 400,  16
	db	"TARGA 256 Color video     OK: Bruce Goren           "
	dw	   0h,	 0,   0,   0,	9, 512, 482, 256
	db	"TARGA 256 Color 35mm      OK: Bruce Goren           "
	dw	   0h,	 0,   0,   0,	9, 512, 342, 256
	db	"TARGA 256 Color 4 x 5     OK: Bruce Goren           "
	dw	   0h,	 0,   0,   0,	9, 512, 384, 256
	db	"Hercules Graphics         OK: Timothy Wegner        "
	dw	   8h,	 0,   0,   0,  10, 720, 348,   2
	db	"Genoa Super EGA Hirez+    OK: John Jurewicz         "
	dw	  75h,	 0,   0,   0,	2, 640, 528,  16
	db	"Genoa Super EGA Hirez+    OK: John Jurewicz         "
	dw	  77h,	 0,   0,   0,	2, 752, 410,  16
	db	"Genoa Super EGA Hirez+    OK: John Jurewicz         "
	dw	  79h,	 0,   0,   0,	2, 800, 600,  16
	db	"Genoa Super EGA Hirez+    OK: John Jurewicz         "
	dw	  7bh,	 0,   0,   0,	2, 912, 480,  16
	db	"NEC GB-1                  UNTESTED: May not work    "
	dw	  26h,	 0,   0,   0,	2, 640, 480,  16
	db	"Disk/RAM 'Video'          Full-Page Epson @  60DPI  "
	dw	   3h,	 0,   0,   0,  11, 768, 480,   2
	db	"Disk/RAM 'Video'          Full-Page Epson @ 120DPI  "
	dw	   3h,	 0,   0,   0,  11, 768, 960,   2
	db	"Disk/RAM 'Video'          Full-Page Epson @ 240DPI  "
	dw	   3h,	 0,   0,   0,  11, 768,1920,   2
	db	"Disk/RAM 'Video'          Full-Page L-Jet @  75DPI  "
	dw	   3h,	 0,   0,   0,  11, 800, 600,   2
	db	"Disk/RAM 'Video'          Full-Page L-Jet @ 150DPI  "
	dw	   3h,	 0,   0,   0,  11,1600,1200,   2
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 320, 200, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 360, 480, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 640, 350, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 640, 400, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 640, 480, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11, 800, 600, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11,1024, 768, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,	 0,   0,   0,  11,2048,2048, 256
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   1,	2, 704, 528,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   2,	2, 720, 540,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   3,	2, 736, 552,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   4,	2, 752, 564,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   5,	2, 768, 576,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   6,	2, 784, 588,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,	 0,   0,   7,	2, 800, 600,  16
	db	"END                       Must be the END of list   "
	dw	   0h,	 0,   0,   0,	0,   0,   0,  0

	db	68*20 dup(0)		; room for 20 more video modes

.code

initvideotable proc uses ds es di
	mov	di, offset videotable+1 ; get the start of the video table
	mov	ax,0			; initially, no entries found
initvideotableloop:
	cmp	word ptr cs:66[di],0	; does the next entry have any colors?
	je	initvideotablereturn	;  nope.  we done.
	inc	ax			; indicate we found an entry
	add	di,68			; locate the next entry
	jmp	initvideotableloop	;  and try again.
initvideotablereturn:
	ret
initvideotable endp

fromvideotable proc uses es si di, tablenumber:word
	push	cs			; get the video table segment into ES
	pop	es			;  ...
	mov	cx,68			; retrieve the table entry length
	mov	ax,tablenumber		; compute the video table offset
	mul	cl			;  == that entry # x the entry length
	add	ax,offset videotable+1	;  + the start of the table
	mov	si,ax			; store it here
	mov	di,offset videoentry	; get the offset of the entry ptr
	push	ds			; swap DS and ES temporarily
	push	es			;  ...
	pop	ds			;  ...
	pop	es			;  ...
	rep	movsb			; move it
	push	es			; restore DS again
	pop	ds			;  ...
	mov	di,offset videoentry	; get the offset of the entry ptr
	mov	al,0			; stuff a few zeroes in where appr.
	mov	25[di],al		;  ...
	mov	51[di],al		;  ...
	ret
fromvideotable endp

tovideotable proc uses es si di, tablenumber:word
	push	cs			; get the video table segment into ES
	pop	es			;  ...
	mov	cx,68			; retrieve the table entry length
	mov	ax,tablenumber		; compute the video table offset
	mul	cl			;  == that entry # x the entry length
	add	ax,offset videotable+1	;  + the start of the table
	mov	di,ax			; store it here
	mov	si,offset videoentry	; get the offset of the entry ptr
	rep	movsb			; move it
	ret
tovideotable endp

;	Routines called by this code

; **************** Routines called by 'calcmand.asm' *********************
;
;	parameters passed directly in registers - just call near routine

asmdotwrite	proc	far		; called by 'calcmand.asm'
		call	dotwrite	; call the appropriate write-a-dot
		ret			; we done
asmdotwrite	endp

asmvideocleanup proc	far		; called by 'calcmand.asm'
		call	videocleanup	; call the local routine
		ret			; we done.
asmvideocleanup endp

; **************** internal Read/Write-a-dot routines ***********************
;
;	These Routines all assume the following register values:
;
;		AL = The Color (returned on reads, sent on writes)
;		CX = The X-Location of the Pixel
;		DX = The Y-Location of the Pixel

nullwrite	proc	near		; "do-nothing" write
	ret
nullwrite	endp

nullread	proc	near		; "do-nothing" read
	mov	ax,0			; "return" black pixels
	ret
nullread	endp

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

cgaread proc	near			; CGA 320x200 4-color, 640x200 2-color
	push	es			; save ES for a bit
	call	near ptr cgasegoff	; compute the segment and offset
	mov	dl,es:[bx]		; retrieve the byte
	not	al			; reset AL for the AND
	and	al,dl			; clear out all but the proper bits
	shr	al,cl			; apply the shift mask
	mov	ah,0			; clear out the high byte
	pop	es			; restore ES
	ret				; we done
cgaread endp

cgasegoff	proc	near		; common CGA routine
	mov	ax,0b800h		; buffer is really here
	shr	dx,1			; provide the interleaving logic
	jnc	cgasegeven		;  skip if odd
	add	ax,200h 		; use the other half of the buffer
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
	mov	al,0fch 		; set up the bit mask
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
	mov	al,0feh 		; set up the bit mask
	rol	al,cl			; AL now contains the bit mask
	pop	cx			; restore the shift count
	ret
cgasegoff	endp

mcgawrite	proc	near		; MCGA 320*200, 246 colors
	xchg	dh,dl			; bx := 256*y
	mov	bx,cx			; bx := x
	add	bx,dx			; bx := 256*y + x
	shr	dx,1
	shr	dx,1			; dx := 64*y
	add	bx,dx			; bx := 320*y + x
	mov	es:[bx],al		; write the dot
	ret				; we done.
mcgawrite	endp

mcgaread	proc	near		; MCGA 320*200, 246 colors
	xchg	dh,dl			; dx := 256*y
	mov	bx,cx			; bx := x
	add	bx,dx			; bx := 256*y + x
	shr	dx,1
	shr	dx,1			; dx := 64*y
	add	bx,dx			; bx := 320*y + x
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
	call	far ptr newbank 	; switch banks
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

vgaread proc	near		; bank-switched EGA/VGA read mode 0
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
	call	far ptr newbank 	; switch banks
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
vgaread endp

outax8bit	proc	near		; convert OUT DX,AX to
	push	ax			; several OUT DX,ALs
	out	dx,al			; (leaving registers intact)
	inc	dx
	mov	al,ah
	out	dx,al
	dec	dx
	pop	ax
	ret
outax8bit	endp

tandysetup	proc	near		; Tandy 640x200x16 mode setup
	mov	dx,03d4h		; write to this address
	mov	ax,07100h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,05001h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,05a02h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,00e03h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,0ff04h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,00605h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,0c806h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,0e207h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,00009h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,0000ch		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,01810h		; write to this register and value
	out	dx,ax			;  do it.
	mov	ax,04612h		; write to this register and value
	out	dx,ax			;  do it.
	mov	dx,03d8h		; new port
	mov	al,01bh 		;  and value
	out	dx,al			;  do it.
	mov	dx,03d9h		; new port
	mov	al,000h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03ddh		; new port
	mov	al,000h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dfh		; new port
	mov	al,024h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dah		; new port
	mov	al,001h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03deh		; new port
	mov	al,00fh 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dah		; new port
	mov	al,002h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03deh		; new port
	mov	al,000h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dah		; new port
	mov	al,003h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03deh		; new port
	mov	al,010h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dah		; new port
	mov	al,005h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03deh		; new port
	mov	al,001h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03dah		; new port
	mov	al,008h 		;  and value
	out	dx,al			;  do it.
	mov	dx,03deh		; new port
	mov	al,002h 		;  and value
	out	dx,al			;  do it.

	cmp	orvideo,0		; are we supposed to clear RAM?
	jne	tandysetup1		;  (nope)
	push	es			; save ES for a tad
	mov	ax,0a000h		; clear the memory
	mov	es,ax			;  ...
	mov	cx,07d00h		; this many words
	mov	di,0			; starting here
	mov	ax,0			; clear out to zero
	rep	stosw			; do it.
	pop	es			; restore ES

tandysetup1:
	mov	oktoprint,0		; no printing in this mode
	ret
tandysetup	endp

tandyread	proc	near		; Tandy 640x200x16 mode read
	push	es			; save ES for a tad
	mov	ax,0a000h		; and reload it with the base addr
	mov	es,ax			;  ...
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	adc	dx,0			; DX:AX now holds the pixel count
	mov	cx,ax			; save this for the bit mask
	shr	dx,1			; now compute the pixel address
	rcr	ax,1			;  (dividing by two)
	mov	si,ax			; save the address here
	mov	al,es:[si]		; get the appropriate byte
	test	cx,1			; is this an even or an odd address?
	jnz	tandyread1		; odd address
	shr	al,1			; even address - shift the high-order
	shr	al,1			;  four bits into the low-order
	shr	al,1			;  four bits
	shr	al,1			;  ...
tandyread1:
	and	ax,0fh			; use the low-order four bits only
	pop	es			; restore ES
	ret
tandyread	endp

tandywrite	proc	near		; Tandy 640x200x16 mode write
	mov	bl,al			; save the color value for a bit
	push	es			; save ES for a tad
	mov	ax,0a000h		; and reload it with the base addr
	mov	es,ax			;  ...
	mov	ax,xdots		; this many dots / line
	mul	dx			; times this many lines
	add	ax,cx			; plus this many x-dots
	adc	dx,0			; DX:AX now holds the pixel count
	mov	cx,ax			; save this for the bit mask
	shr	dx,1			; now compute the pixel address
	rcr	ax,1			;  (dividing by two)
	mov	si,ax			; save the address here
	mov	al,es:[si]		; get the appropriate byte
	mov	bh,0f0h 		; set up the bit mask
	and	bl,00fh 		; isolate the bits we want to change
	test	cx,1			; is this an even or an odd address?
	jnz	tandywrite1		; odd address
	shl	bl,1			; even address - shift the low-order
	shl	bl,1			;  four bits into the high-order
	shl	bl,1			;  four bits
	shl	bl,1			;  ...
	mov	bh,00fh 		; modify the bit mask
tandywrite1:
	and	al,bh			; mask out the bits to modify
	or	al,bl			; add in the new bits
	mov	es:[si],al		; save the appropriate byte
	pop	es			; restore ES
	ret
tandywrite	endp


;	The 360x480 mode draws heavily on Michael Abrash's article in
;	the January/February 1989 "Programmer's Journal" and files uploaded
;	to Compuserv's PICS forum by Dr. Lawrence Gozum - integrated here
;	by Timothy Wegner

; Michael Abrash equates. Not all used, but I'll leave for reference.

VGA_SEGMENT	  EQU	0A000h
SC_INDEX	  EQU	3C4h	 ;Sequence Controller Index register
GC_INDEX	  EQU	3CEh	 ;Graphics Controller Index register
CRTC_INDEX	  EQU	3D4h	 ;CRT Controller Index register
MAP_MASK	  EQU	2	 ;Map Mask register index in SC
MEMORY_MODE	  EQU	4	 ;Memory Mode register in SC
MAX_SCAN_LINE	  EQU	9	 ;Maximum Scan Line reg index in CRTC
				 ;Use 9 for 2 pages of 320x400
;MAX_SCAN_LINE	  EQU	1	 ;Use 1 for 4 pages of 320x200
START_ADD_HIGH	  EQU	0Ch	 ;Start Address High reg index in CRTC
UNDERLINE	  EQU	14h	 ;Underline Location reg index in CRTC
MODE_CONTROL	  EQU	17h	 ;Mode Control reg index in CRTC
READ_MAP	  EQU	4	 ;Read Mask register index in SC
GRAPHICS_MODE	  EQU	5	 ;Graphics Mode register index in SC
MISC		  EQU	6	 ;Miscellaneous register index in SC
WORD_OUTS_OK	  EQU	1	 ;set to 0 to assemble for computers
				 ;that can't handle word outs to indexed
				 ;VGA registers
;
;Macro to output a word value to a port
;
OUT_WORD MACRO
IF WORD_OUTS_OK
	 OUT	  DX,AX
ELSE
	 OUT	  DX,AL
	 INC	  DX
	 XCHG	  AH,AL
	 OUT	  DX,AL
	 DEC	  DX
	 XCHG	  AH,AL
ENDIF
	 ENDM

;Macro to ouput a constant value to an indexed VGA register
CONSTANT_TO_INDEXED_REGISTER	 MACRO	ADDRESS,INDEX,VALUE
	 MOV	  DX,ADDRESS
	 MOV	  AX,(VALUE SHL 8)+INDEX
	 OUT_WORD
	 ENDM

tweak256read	proc near uses si	; Tweaked-VGA ...x256 color mode

  mov	  ax,xdots
  shr	  ax,1
  shr	  ax,1			 ; now ax = xdots/4
  mul	  dx			 ;Point to start of desired row
  push	  cx			 ;Save X coordinate for later
  shr	  cx,1			 ;There are 4 pixels at each address
  shr	  cx,1			 ;so divide X by 4
  add	  ax,cx 		 ;Point to pixels address
  mov	  si,ax
  pop	  ax			 ;Retrieve X coordinate
  and	  al,3			 ;Get the plane number of the pixel
  mov	  ah,al
  mov	  al,READ_MAP
  mov	  dx,GC_INDEX
  OUT_WORD			 ;Set to write to the proper plane for the
				 ;pixel
  xor	  ax,ax
  lods	  byte ptr es:[si]	 ;Read the pixel
  ret

tweak256read	endp

tweak256write	proc near uses di	; Tweaked-VGA ...x256 color mode
  mov	  bl,al 		; color
  mov	  ax,xdots
  shr	  ax, 1
  shr	  ax, 1 		 ; now ax = xdots/4
  mul	  dx			;Point to start of desired row
  push	  cx			;Save X coordinate for later
  shr	  cx,1			;There are 4 pixels at each address
  shr	  cx,1			;so divide X by 4
  add	  ax,cx 		;Point to pixels address
  mov	  di,ax
  pop	  cx			;Retrieve X coordinate
  and	  cl,3			;Get the plane number of the pixel
  mov	  ah,1
  shl	  ah,cl 		;Set the bit corresponding to the plane
				;the pixel is in
  mov	  al,MAP_MASK
  mov	  dx,SC_INDEX
  OUT_WORD			 ;Set to write to the proper plane for the
				 ;pixel
  mov	  es:[di],bl		 ;Draw the pixel

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
	and	ax,0f0h 		; zero out the low-order bits
	mov	cl,4			; shift the results
	shr	al,cl			;  ...
	ret
ati1024read	endp

ati1024write	proc near		; ATI 1024x768x16 write
	call	ati1024addr		; calculate the address
	mov	dl,es:[bx]		; get the byte the pixel is in
	and	al,00fh 		; zero out the high-order color bits
	test	cl,1			; is X odd?
	jz	atiwritehigh		;  Nope.  Use the high bits
	and	dl,0f0h 		; zero out the low-order video bits
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
	adc	dx,0			; answer in dx:ax
	shr	dx,1			; shift the answer right one bit
	rcr	ax,1			;  .. in the 32-bit DX:AX combo
	mov	bx,ax			; save this in BX
	cmp	dx,curbk		; see if bank changed
	je	atisame_bank		; jump if old bank ok
	mov	ax,dx			; newbank expects bank in al
	call	far ptr newbank
atisame_bank:
	pop	ax			; restore AX
	ret
ati1024addr	endp

;
;	The following 'Super256' code is courtesy of Timothy Wegner.
;

super256write	proc near		; super-VGA ...x256 colors write-a-dot
	call	super256addr		; calculate address and switch banks
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
	adc	dx,0			; answer in dx:ax - dl=bank, ax=offset
	mov	bx,ax			; save this in BX
	cmp	dx,curbk		; see if bank changed
	je	same_bank		; jump if old bank ok
	mov	ax,dx			; newbank expects bank in al
	call	far ptr newbank
same_bank:
	pop	ax			; restore AX
	ret
super256addr	endp

;
;	BANKS.ASM was used verbatim except:
;	   1) removed ".model small"
;	   2) deleted "end"
;	Integrated by Tim Wegner 8/15/89
;	(switched to John's 9/7/89 version on 9/10/89 - Bert)
;	(switched to John's 1/5/90 version on 1/9/90  - Bert)
;	(switched to John's version 3 on 4/27/90  - Bert)
;	((added logic for various resolution on 9/10/90 - Bert)
;	[ and had to add '@codesize' equate for some reason (??) - Bert]
;

;	.MODEL medium,c
@codesize	equ	1	; ??? shouldn't have to DO this!!!

;
;	Copyright 1988,89,90 John Bridges
;	Free for use in commercial, shareware or freeware applications
;
;	SVGAMODE.ASM
;
.data

OSEG	equ	SS:			;segment override for variable access

bankadr dw	offset $nobank
if @codesize
bankseg dw	seg $nobank
endif

vesa_granularity	db	0	; BDT VESA Granularity value
vesa_bankswitch 	dd	0	; BDT VESA Bank-Switching Routine

	public	curbk

curbk	dw	0

	public	vga512

vga512	dw	0

	public	cirrus,video7,tseng,tseng4,paradise,chipstech,trident
	public	ativga,everex,aheada,aheadb,oaktech

	public	vesa		; Bert
vesa	dw	0		; Bert

cirrus	dw	0
video7	dw	0
tseng	dw	0
tseng4	dw	0
paradise dw	0
chipstech dw	0
trident dw	0
ativga	dw	0
everex	dw	0
aheada	dw	0
aheadb	dw	0
oaktech dw	0

first	dw	0		;flag so whichvga() is only called once
retval	dw	0		;first return value from whichvga()

		; this part is new - Bert
.code

vesa_entries	dw	0
	dw	 640, 400,256, 4f02h,100h
	dw	 640, 480,256, 4f02h,101h
	dw	 800, 600, 16, 4f02h,102h
	dw	 800, 600,256, 4f02h,103h
	dw	1024, 768, 16, 4f02h,104h
	dw	1024, 768,256, 4f02h,105h
	dw	1280,1024, 16, 4f02h,106h
	dw	1280,1024,256, 4f02h,107h
ahead_entries	dw	0
	dw	 800, 600, 16, 06ah,0
	dw	 800, 600, 16, 071h,0
	dw	1024, 768, 16, 074h,0
	dw	 640, 400,256, 060h,0
	dw	 640, 480,256, 061h,0
	dw	 800, 600,256, 062h,0
ati_entries	dw	0
	dw	 800, 600, 16, 054h,0
	dw	1024, 768, 16, 065h,0ffh	; (non-standard mode flag)
	dw	 640, 400,256, 061h,0
	dw	 640, 480,256, 062h,0
	dw	 800, 600,256, 063h,0
chips_entries	dw	0
	dw	 800, 600, 16, 070h,0
	dw	1024, 768, 16, 072h,0
	dw	 640, 400,256, 078h,0
	dw	 640, 480,256, 079h,0
	dw	 800, 600,256, 07bh,0
everex_entries	dw	0
	dw	 752, 410, 16, 070h,01h
	dw	 800, 600, 16, 070h,02h
	dw	1280, 350,  4, 070h,11h
	dw	1280, 600,  4, 070h,12h
	dw	 640, 350,256, 070h,13h
	dw	 640, 400,256, 070h,14h
	dw	 512, 480,256, 070h,15h
	dw	1024, 768, 16, 070h,20h
	dw	 640, 480,256, 070h,30h
	dw	 800, 600,256, 070h,31h
paradise_entries	dw	0
	dw	 800, 600,  2, 059h,0
	dw	 800, 600, 16, 058h,0
	dw	 640, 400,256, 05eh,0
	dw	 640, 480,256, 05fh,0
	dw	1024, 768, 16, 05dh,0
trident_entries dw	0
	dw	 800, 600, 16, 05bh,0
	dw	1024, 768, 16, 05fh,0
	dw	 640, 400,256, 05ch,0
	dw	 640, 480,256, 05dh,0
	dw	 800, 600,256, 05eh,0
tseng_entries	dw	0
	dw	 800, 600, 16, 029h,0
	dw	1024, 768, 16, 037h,0
	dw	 640, 350,256, 02dh,0
	dw	 640, 400,256, 0,0feh	; (non-standard mode flag)
	dw	 640, 480,256, 02eh,0
	dw	 720, 512,256, 02fh,0
	dw	 800, 600,256, 030h,0
video7_entries	dw	0
	dw	 752, 410, 16, 6f05h,60h
	dw	 720, 540, 16, 6f05h,61h
	dw	 800, 600, 16, 6f05h,62h
	dw	1024, 768,  2, 6f05h,63h
	dw	1024, 768,  4, 6f05h,64h
	dw	1024, 768, 16, 6f05h,65h
	dw	 640, 400,256, 6f05h,66h
	dw	 640, 480,256, 6f05h,67h
	dw	 720, 540,256, 6f05h,68h
	dw	 800, 600,256, 6f05h,69h
oaktech_entries dw	0
	dw	 800, 600, 16, 52h,0
	dw	 640, 480,256, 53h,0
	dw	 800, 600,256, 54h,0
	dw	1024, 768, 16, 56h,0
	dw	0
tseng4_entries	dw	0
	dw	 800, 600, 16, 29h,0
	dw	1024, 768, 16, 37h,0
	dw	 640, 350,256, 2dh,0
	dw	 640, 400,256, 2fh,0
	dw	 640, 480,256, 2eh,0
	dw	 800, 600,256, 30h,0
	dw	1024, 768,256, 38h,0
	dw	0
no_entries	dw	0
	dw	 320, 200,256, 13h,0
	dw	 640, 480, 16, 12h,0
	dw	0

.code

newbank proc			;bank number is in AX
	cli
	mov	OSEG[curbk],ax
if @codesize
	call	dword ptr OSEG[bankadr]
else
	call	word ptr OSEG[bankadr]
endif
	ret
newbank endp

$tseng	proc		;Tseng
	push	ax
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
$tseng	endp

$tseng4 proc		;Tseng 4000 series
	push	ax
	push	dx
	mov	ah,al
	mov	dx,3bfh 		;Enable access to extended registers
	mov	al,3
	out	dx,al
	mov	dl,0d8h
	mov	al,0a0h
	out	dx,al
	and	ah,15
	mov	al,ah
	shl	al,1
	shl	al,1
	shl	al,1
	shl	al,1
	or	al,ah
	mov	dl,0cdh
	out	dx,al
	sti
	pop	dx
	pop	ax
	ret
$tseng4 endp

$trident proc		;Trident
	push	ax
	push	dx
	mov	dx,3ceh 	;set page size to 64k
	mov	al,6
	out	dx,al
	inc	dl
	in	al,dx
	dec	dl
	or	al,4
	mov	ah,al
	mov	al,6
	out	dx,ax

	mov	dl,0c4h 	;switch to BPS mode
	mov	al,0bh
	out	dx,al
	inc	dl
	in	al,dx
	dec	dl

	mov	ah,byte ptr OSEG[curbk]
	xor	ah,2
	mov	dx,3c4h
	mov	al,0eh
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret
$trident endp

$video7 proc		;Video 7
	push	ax
	push	dx
	push	cx
; Video-7 1024x768x16 mode patch (thanks to Frank Lozier 11/8/89).
	cmp	colors,16
	jne	video7xx
	shl	ax,1
	shl	ax,1
video7xx:
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
$video7 endp

$paradise proc		;Paradise
	push	ax
	push	dx
	mov	dx,3ceh
	mov	ax,50fh 	;turn off write protect on VGA registers
	out	dx,ax
	mov	ah,byte ptr OSEG[curbk]
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
$paradise endp

$chipstech proc 	;Chips & Tech
	push	ax
	push	dx
	mov	dx,46e8h	;place chip in setup mode
	mov	ax,1eh
	out	dx,ax
	mov	dx,103h 	;enable extended registers
	mov	ax,0080h	; (patched per JB's msg - Bert)
	out	dx,ax
	mov	dx,46e8h	;bring chip out of setup mode
	mov	ax,0eh
	out	dx,ax
	mov	ah,byte ptr OSEG[curbk]
	shl	ah,1		;change 64k bank number into 16k bank number
	shl	ah,1
	mov	al,10h
	mov	dx,3d6h
	out	dx,ax
	sti
	pop	dx
	pop	ax
	ret
$chipstech endp

$ativga proc		;ATI VGA Wonder
	push	ax
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
$ativga endp

$everex proc		;Everex
	push	ax
	push	dx
	push	cx
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
	sti
	pop	cx
	pop	dx
	pop	ax
	ret
$everex endp

$aheada proc
	push	ax
	push	dx
	push	cx
	mov	ch,al
	mov	dx,3ceh 	;Enable extended registers
	mov	ax,200fh
	out	dx,ax
	mov	dl,0cch 	;bit 0
	in	al,dx
	mov	dl,0c2h
	and	al,11011111b
	shr	ch,1
	jnc	temp_1
	or	al,00100000b
temp_1: out	dx,al
	mov	dl,0cfh 	;bits 1,2,3
	mov	al,0
	out	dx,al
	inc	dx
	in	al,dx
	dec	dx
	and	al,11111000b
	or	al,ch
	mov	ah,al
	mov	al,0
	out	dx,ax
	sti
	pop	cx
	pop	dx
	pop	ax
	ret
$aheada endp

$aheadb proc
	push	ax
	push	dx
	push	cx
	mov	ch,al
	mov	dx,3ceh 	;Enable extended registers
	mov	ax,200fh
	out	dx,ax
	mov	ah,ch
	mov	cl,4
	shl	ah,cl
	or	ah,ch
	mov	al,0dh
	out	dx,ax
	sti
	pop	cx
	pop	dx
	pop	ax
	ret
$aheadb endp

$oaktech proc		;Oak Technology Inc OTI-067
	push	ax
	push	dx
	and	al,15
	mov	ah,al
	shl	al,1
	shl	al,1
	shl	al,1
	shl	al,1
	or	al,ah
	mov	dx,3dfh
	out	dx,al
	sti
	pop	dx
	pop	ax
	ret
$oaktech endp

$vesa	proc				; VESA bank switching
	push	ax
	push	bx
	push	cx
	push	dx
	mul	vesa_granularity	; Adjust for the granularity factor
	mov	dx,ax			; Select window position
	mov	ax,4f05h		; VESA window control command
	mov	bx,0			; select window (bank) sub-command
;	int	10h			; do it!
	call	dword ptr vesa_bankswitch  ; do it!
	sti
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
$vesa	endp

$nobank proc
	sti
	ret
$nobank endp

bkadr	macro	flag,func,entries		; Bert
	mov	video_entries, offset entries	; Bert
	mov	[flag],1
	mov	[bankadr],offset func
if @codesize
	mov	[bankseg],seg func
endif
	endm

nojmp	macro
	local	lbl
	jmp	lbl
lbl:
	endm

whichvga proc	uses si
	cmp	[first],0
	jz	gotest
	mov	ax,[retval]
	ret
gotest: mov	[first],1
	mov	ax,4f00h		; check for VESA adapter
	push	ds			; set ES == DS
	pop	es			;  ...
	mov	di, offset dacbox	; answer goes here (a safe place)
	int	10h			; do it.
	cmp	ax,004fh		; successful response?
	jne	notvesa 		; nope.  Not a VESA adapter

	cmp	byte ptr 0[di],'V'      ; string == 'VESA'?
	jne	notvesa 		; nope.  Not a VESA adapter
	cmp	byte ptr 1[di],'E'      ; string == 'VESA'?
	jne	notvesa 		; nope.  Not a VESA adapter
	cmp	byte ptr 2[di],'S'      ; string == 'VESA'?
	jne	notvesa 		; nope.  Not a VESA adapter
	cmp	byte ptr 3[di],'A'      ; string == 'VESA'?
	jne	notvesa 		; nope.  Not a VESA adapter
	bkadr	vesa,$vesa, vesa_entries
	jmp	fini
notvesa:
	mov	si,1
	mov	ax,0c000h
	mov	es,ax
	cmp	word ptr es:[40h],'13'
	jnz	noati
	bkadr	ativga,$ativga,ati_entries		; Bert
	cli
	mov	dx,1ceh
	mov	al,0bbh
	out	dx,al
	inc	dl
	in	al,dx
	sti
	and	al,20h
	jz	no512
	mov	[vga512],1
no512:	jmp	fini

noati:	mov	ax,7000h		;Test for Everex
	xor	bx,bx
	cld
	int	10h
	cmp	al,70h
	jnz	noev
	bkadr	everex,$everex, everex_entries		; Bert
	and	ch,11000000b
	jz	temp_2
	mov	[vga512],1
temp_2: and	dx,0fff0h
	cmp	dx,6780h
	jz	yeste
	cmp	dx,2360h
	jnz	note
yeste:	bkadr	trident,$trident, everex_entries	; Bert
	mov	everex,0
note:	jmp	fini

noev:	mov	dx,3c4h 		;Test for Trident
	mov	al,0bh
	out	dx,al
	inc	dl
	in	al,dx
	cmp	al,0fh
	ja	notri
	cmp	al,2
	jb	notri
	bkadr	trident,$trident, trident_entries	; Bert
	mov	[vga512],1
	jmp	fini

notri:	mov	ax,6f00h		;Test for Video 7
	xor	bx,bx
	cld
	int	10h
	cmp	bx,'V7'
	jnz	nov7
	bkadr	video7,$video7, video7_entries		; Bert
	mov	ax,6f07h
	cld
	int	10h
	and	ah,7fh
	cmp	ah,1
	jbe	temp_3
	mov	[vga512],1
temp_3: jmp	fini

nov7:	call	$cirrus 		;Test for Cirrus
	cmp	[cirrus],0
	je	noci
	jmp	fini

noci:	mov	dx,3ceh 		;Test for Paradise
	mov	al,9			;check Bank switch register
	out	dx,al
	inc	dx
	in	al,dx
	dec	dx
	or	al,al
	jnz	nopd

	mov	ax,50fh 		;turn off write protect on VGA registers
	out	dx,ax
	mov	dx,offset $pdrsub
	mov	cx,1
	call	$chkbk
	jc	nopd			;if bank 0 and 1 same not paradise
	bkadr	paradise,$paradise, paradise_entries	; Bert
;	mov	cx,64
;	call	$chkbk
;	jc	temp_4			;if bank 0 and 64 same only 256k
	mov	[vga512],1
temp_4: jmp	fini

nopd:	mov	ax,5f00h		;Test for Chips & Tech
	xor	bx,bx
	cld
	int	10h
	cmp	al,5fh
	jnz	noct
	bkadr	chipstech,$chipstech, chips_entries	; Bert
	cmp	bh,1
	jb	temp_5
	mov	[vga512],1
temp_5:
	jmp	fini

noct:	mov	ch,0
	mov	dx,3d4h 		;check for Tseng 4000 series
	mov	al,33h
	out	dx,al
	inc	dx
	in	al,dx
	dec	dx
	mov	cl,al
	mov	ax,00a33h
	out	dx,ax
	mov	al,33h
	out	dx,al
	inc	dx
	in	al,dx
	and	al,0fh
	dec	dx
	cmp	al,00ah
	jnz	noct2
	mov	ax,00533h
	out	dx,ax
	mov	al,33h
	out	dx,al
	inc	dx
	in	al,dx
	dec	dx
	and	al,0fh
	cmp	al,005h
	jnz	noct2
	mov	al,33h
	mov	ah,cl
	out	dx,ax
	mov	ch,1

	mov	dx,3bfh 		;Enable access to extended registers
	mov	al,3
	out	dx,al
	mov	dl,0d8h
	mov	al,0a0h
	out	dx,al

noct2:	mov	dx,3cdh 		;Test for Tseng
	in	al,dx			;save bank switch register
	mov	cl,al
	mov	al,0aah 		;test register with 0aah
	out	dx,al
	in	al,dx
	cmp	al,0aah
	jnz	nots
	mov	al,055h 		;test register with 055h
	out	dx,al
	in	al,dx
	cmp	al,055h
	jnz	nots
	mov	al,cl
	out	dx,al
	bkadr	tseng,$tseng, tseng_entries    ; Bert
	mov	[vga512],1

	cmp	ch,0
	jz	oldts
	mov	tseng,0
	bkadr	tseng4,$tseng4, tseng4_entries ; Bert
oldts:	jmp	fini

nots:
	mov	dx,3ceh 	;Test for Above A or B chipsets
	mov	ax,200fh
	out	dx,ax
	inc	dx
	nojmp
	in	al,dx
	cmp	al,21h
	jz	verb
	cmp	al,20h
	jnz	noab
	bkadr	aheada,$aheada, ahead_entries		; Bert
	mov	[vga512],1
	jmp	short fini

verb:	bkadr	aheadb,$aheadb, ahead_entries		; Bert
	mov	[vga512],1
	jmp	short fini

noab:	mov	dx,3deh
	in	al,dx
	and	al,11100000b
	cmp	al,01100000b
	jnz	nooak
	bkadr	oaktech,$oaktech, oaktech_entries	; Bert
	mov	al,0dh
	out	dx,al
	inc	dx
	nojmp
	in	al,dx
	test	al,80h
	jz	no4ram
	mov	[vga512],1
no4ram: jmp	short fini

nooak:	mov	si,0

fini:	mov	ax,si
	mov	[retval],ax
	ret
whichvga endp


$cirrus proc	near
	mov	dx,3d4h 	; assume 3dx addressing
	mov	al,0ch		; screen a start address hi
	out	dx,al		; select index
	inc	dx		; point to data
	mov	ah,al		; save index in ah
	in	al,dx		; get screen a start address hi
	xchg	ah,al		; swap index and data
	push	ax		; save old value
	push	dx		; save crtc address
	xor	al,al		; clear crc
	out	dx,al		; and out to the crtc

	mov	al,1fh		; Eagle ID register
	dec	dx		; back to index
	out	dx,al		; select index
	inc	dx		; point to data
	in	al,dx		; read the id register
	mov	bh,al		; and save it in bh

	mov	cl,4		; nibble swap rotate count
	mov	dx,3c4h 	; sequencer/extensions
	mov	bl,6		; extensions enable register

	ror	bh,cl		; compute extensions disable value
	mov	ax,bx		; extensions disable
	out	dx,ax		; disable extensions
	inc	dx		; point to data
	in	al,dx		; read enable flag
	or	al,al		; disabled ?
	jnz	exit		; nope, not an cirrus

	ror	bh,cl		; compute extensions enable value
	dec	dx		; point to index
	mov	ax,bx		; extensions enable
	out	dx,ax		; enable extensions
	inc	dx		; point to data
	in	al,dx		; read enable flag
	cmp	al,1		; enabled ?
	jne	exit		; nope, not an cirrus
	mov	[cirrus],1
	mov	video_entries, offset no_entries	; Bert
	mov	[bankadr],offset $nobank
if @codesize
	mov	[bankseg],seg $nobank
endif
exit:	pop	dx		; restore crtc address
	dec	dx		; point to index
	pop	ax		; recover crc index and data
	out	dx,ax		; restore crc value
	ret
$cirrus endp

$chkbk	proc	near		;paradise bank switch check
	mov	di,0b800h
	mov	es,di
	xor	di,di
	mov	bx,1234h
	call	$gochk
	jnz	nopd
	mov	bx,4321h
	call	$gochk
	jnz	nopd
	clc
	ret
nopd:	stc
	ret
$chkbk	endp

$gochk	proc	near
	push	si
	mov	si,bx

	mov	al,cl
	call	dx
	xchg	bl,es:[di]
	mov	al,ch
	call	dx
	xchg	bh,es:[di]

	xchg	si,bx

	mov	al,cl
	call	dx
	xor	bl,es:[di]
	mov	al,ch
	call	dx
	xor	bh,es:[di]

	xchg	si,bx

	mov	al,ch
	call	dx
	mov	es:[di],bh
	mov	al,cl
	call	dx
	mov	es:[di],bl

	mov	al,0
	call	dx
	or	si,si
	pop	si
	ret
$gochk	endp


$pdrsub proc	near		;Paradise
	push	dx
	mov	ah,al
	mov	dx,3ceh
	mov	al,9
	out	dx,ax
	pop	dx
	ret
$pdrsub endp

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
;	push	es			; save registers around the call
;	push	si			; save registers around the call
;	push	di			; save registers around the call
;	mov	ah,0			; clear the high-order color byte
	push	ax			; colors parameter
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr writedisk	; let the external routine do it
	add	sp,6			; pop the parameters
;	pop	cx			; restore registers
;	pop	dx			; restore registers
;	pop	ax			; restore registers
;	pop	di			; restore registers
;	pop	si			; restore registers
;	pop	es			; restore registers
	ret				; we done.
diskwrite	endp

diskread	proc	near		; disk-video read routine
;	push	es			; save registers around the call
;	push	si			; save registers around the call
;	push	di			; save registers around the call
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr readdisk	; let the external routine do it
	add	sp,4			; pop the parameters
;	pop	cx			; restore registers
;	pop	dx			; restore registers
;	pop	di			; restore registers
;	pop	si			; restore registers
;	pop	es			; restore registers
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

diskend 	proc	near		; disk-video end routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr enddisk 	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
diskend 	endp

; ***********************************************************************
;
; TARGA MODIFIED 1 JUNE 89 - j mclain
;
tgawrite	proc	near		;
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
	call	far ptr LineTGA 	; lineTGA( ldata, line, cnt )
	add	sp,8			; stack bias
	pop	di			;
	pop	si			;
	pop	es			;
	ret
tgaline endp

tgaread 	proc	near		;
	push	es			;
	push	si			;
	push	di			;
	push	dx			; 'y' parameter
	push	cx			; 'x' parameter
	call	far ptr ReadTGA 	; readTGA( x, y )
	pop	cx			;
	pop	dx			;
	pop	di			;
	pop	si			;
	pop	es			;
	ret				;
tgaread endp

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


f85end	proc	near

	call   far ptr close8514
	ret

f85end	endp


f85write    proc    near

	call   far ptr fr85wdot
	ret

f85write    endp

f85read proc	near

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
	call	far ptr readhgc 	; call the Hercules Read dot routine
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
	call	far ptr inithgc 	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
hgcstart	endp

hgcend		proc	near		; hercules end routine
	push	es			; save registers around the call
	push	si			; save registers around the call
	push	di			; save registers around the call
	call	far ptr termhgc 	; let the external routine do it
	pop	di			; restore registers
	pop	si			; restore registers
	pop	es			; restore registers
	ret				; we done.
hgcend		endp

; **************** video adapter detection routine ***********************
;
;	This routine performs a few quick checks on the type of
;	video adapter installed.  It leaves its results in the
;	encoded variable "video_type", and fills in a few bank-switching
;	routines along the way.
;


adapter_detect	proc	far		; video adapter detection routine
	mov	video_type,2		; set the video type: CGA
	mov	ax,[bankadr]		; Initialize the bank-switching
	mov	video_bankadr,ax	;  logic to the do-nothing routine
	mov	ax,[bankseg]		;  ...
	mov	video_bankseg,ax	;  ...
	mov	bx,0			; clear out all of the 256-mode flags
	mov	tseng,bx		;  ...
	mov	trident,bx		;  ...
	mov	video7,bx		;  ...
	mov	paradise,bx		;  ...
	mov	chipstech,bx		;  ...
	mov	ativga,bx		;  ...
	mov	everex,bx		;  ...
	mov	cirrus,bx		;  ...
	mov	aheada,bx		;  ...
	mov	aheadb,bx		;  ...
	mov	tseng4,bx		;  ...
	mov	oaktech,bx		;  ...
	mov	[bankadr],offset $nobank
	mov	[bankseg],seg $nobank
	mov	video_entries, offset no_entries	; ...
	mov	ax,1a00h		; look for a VGA/MCGA
	int	10h			;  by using a VGA-specific call
	cmp	al,1ah			; was AL modified?
	je	adapter_detect_4	;  Yup.  A VGA or an MCGA
	mov	ah,12h			; look for an EGA
	mov	bl,10h			;  by using an EGA-specific call
	int	10h			;  ...
	cmp	bl,10h			; was BL modified?
	je	adapter_detect_99	;  nope.  Not an EGA
	mov	video_type,3		; set the video type: EGA
	jmp	short adapter_detect_99 ;  We done.
adapter_detect_4:
	mov	video_type,4		; set the video type: VGA
	call	whichvga		; autodetect which VGA is there
	mov	ax,[bankadr]		; save the results
	mov	video_bankadr,ax	;  ...
	mov	ax,[bankseg]		;  ...
	mov	video_bankseg,ax	;  ...

adapter_detect_99:
	ret
adapter_detect	endp

; **************** internal Read/Write-a-line routines *********************
;
;	These routines are called by out_line(), put_line() and get_line().
;	They assume the following register values:
;
;		si = offset of array of colors for a row (write routines)
;		di = offset of array of colors for a row (read routines)
;
;		ax = stopping column
;		bx =
;		cx = starting column
;		dx = row
;
; Note: so far have converted only normaline, normalineread, mcgaline,
;	mcgareadline, super256line, super256readline -- Tim

normaline	proc	near		; Normal Line
normal_line1:
	push	ax			; save stop col
	mov	al,[si] 		; retrieve the color
	push	cx			; save the counter around the call
	push	dx			; save column around the call
	push	si			; save the pointer around the call also
	call	dotwrite		; write the dot via the approved method
	pop	si			; restore the pointer
	pop	dx			; restore the column
	pop	cx			; restore the counter
	inc	si			; bump it up
	inc	cx			; bump it up
	pop	ax			; retrieve number of dots
	cmp	cx,ax			; more to go?
	jle	normal_line1		; yup.	do it.
	ret
normaline	endp

normalineread	proc	near		; Normal Line
	mov	bx,videomem
	mov	es,bx
normal_lineread1:
	push	ax			; save stop col
	push	cx			; save the counter around the call
	push	dx			; save column around the call
	push	di			; save the pointer around the call also
	call	dotread 		; read the dot via the approved method
	pop	di			; restore the pointer
	pop	dx			; restore the column
	pop	cx			; restore the counter
	mov	bx,di			; locate the actual pixel color
	mov	[bx],al 		; retrieve the color
	inc	di			; bump it up
	inc	cx			; bump it up
	pop	ax			; retrieve number of dots
	cmp	cx,ax			; more to go?
	jle	normal_lineread1	; yup.	do it.
	ret
normalineread	endp

mcgaline	proc	near		; MCGA 320*200, 246 colors
	sub	ax,cx			; last col - first col
	inc	ax			;   + 1

	xchg	dh,dl			; bx := 256*y
	mov	bx,cx			; bx := x
	add	bx,dx			; bx := 256*y + x
	shr	dx,1
	shr	dx,1			; dx := 64*y
	add	bx,dx			; bx := 320*y + x
	mov	di,bx			; di = offset of row in video memory

	mov	cx,ax			; move this many bytes
	rep	movsb			; zap line into memory
	ret
mcgaline	endp

mcgareadline	proc	near		; MCGA 320*200, 246 colors

	sub	ax,cx			; last col - first col
	inc	ax			;   + 1

	xchg	dh,dl			; bx := 256*y
	mov	bx,cx			; bx := x
	add	bx,dx			; bx := 256*y + x
	shr	dx,1
	shr	dx,1			; dx := 64*y
	add	bx,dx			; bx := 320*y + x
	mov	si,bx			; di = offset of row in video memory

	mov	cx,ax			; move this many bytes
	mov	ax,ds			; copy data segment to ...
	mov	es,ax			;  ... es
	mov	ax,videomem		; copy video segment to ...
	mov	ds,ax			;  ... ds
	rep	movsb			; zap line into memory
	mov	ax,es
	mov	ds,ax			; restore data segement to ds
	ret
mcgareadline	endp

vgaline proc	near		; Bank Switch EGA/VGA line write
	push	cx			; save a few registers
	push	ax			;  ...
	push	dx			;  ...

	mov	bx,dx			; save the rowcount
	mov	ax,xdots		; compute # of dots / pass
	shr	ax,1			;  (given 8 passes)
	shr	ax,1			;  ...
	shr	ax,1			;  ...
	mul	bx			; now calc first video addr
	cmp	dx,curbk		; see if bank changed
	jne	bank_is_changing	; if bank change call normaline

	mov	di,cx			; compute the starting destination
	shr	di,1			; divide by 8
	shr	di,1			;  ...
	shr	di,1			;  ...
	add	di,ax			; add the first pixel offset

	mov	dx,03ceh		; set up graphics cntrlr addr
	mov	ax,8008h		; set up for the bit mask
	and	cx,7			; adjust for the first pixel offset
	ror	ah,cl			;  ...

	pop	bx			; flush old DX value
	pop	bx			; flush old AX value
	pop	cx			; flush old CX value
	sub	bx,cx			; convert to a length value
	add	bx,si			; locate the last source locn

	mov	cx,ax			; save the bit mask

vgaline1:
	out	dx,ax			; set the graphics bit mask
	push	ax			; save registers for a tad
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
	cmp	si,bx			; are we beyond the end?
	jbe	vgaline2		; loop if more dots this pass
	pop	di			; restore the saved registers
	pop	si			;  ...
	pop	ax			;  ...
	inc	si			; offset the source 1 byte
	cmp	si,bx			; are we beyond the end?
	ja	vgaline4		; stop if no more dots this pass
	ror	ah,1			; alter bit mask value
	cmp	ah,80h			; time to update DI:
	jne	vgaline3		;  nope
	inc	di			;  yup
vgaline3:
	cmp	ah,ch			; already done all 8 of them?
	jne	vgaline1		;  nope.  do another one.
vgaline4:
	call	videocleanup		; else cleanup time.
	ret				;  and we done.

bank_is_changing:
	pop	dx			; restore the registers
	pop	ax			;  ...
	pop	cx			;  ...
	call	normaline		; just calling newbank didn't quite
	ret				;  work. This depends on no bank
vgaline endp				;  change mid line (ok for 1024 wide)

vgareadline	proc	near		; Bank Switch EGA/VGA line read
	push	cx			; save a few registers
	push	ax			;  ...
	push	dx			;  ...

	mov	bx,dx			; save the rowcount
	mov	ax,xdots		; compute # of dots / pass
	shr	ax,1			;  (given 8 passes)
	shr	ax,1			;  ...
	shr	ax,1			;  ...
	mul	bx			; now calc first video addr
	cmp	dx,curbk		; see if bank changed
	jne	bank_is_changing	; if bank change call normaline

	mov	si,cx			; compute the starting destination
	shr	si,1			; divide by 8
	shr	si,1			;  ...
	shr	si,1			;  ...
	add	si,ax			; add the first pixel offset

	and	cx,7			; adjust for the first pixel offset
	mov	ch,cl			; save the original offset value

	pop	bx			; flush old DX value
	pop	bx			; flush old AX value
	pop	ax			; flush old CX value
	sub	bx,ax			; convert to a length value
	add	bx,di			; locate the last dest locn

	mov	ax,0a000h		; EGA/VGA screen starts here
	mov	es,ax			;  ...

	mov	dx,03ceh		; set up graphics cntrlr addr

vgaline1:
	push	bx			; save BX for a tad
	mov	ch,80h			; bit mask to shift
	shr	ch,cl			;  ...
	mov	bx,0			; initialize bits-read value (none)
	mov	ax,0304h		; set up controller address register
vgareadloop:
	out	dx,ax			; do it
	mov	bh,es:[si]		; retrieve the old value
	and	bh,ch			; mask one bit
	neg	bh			; set bit 7 correctly
	rol	bx,1			; rotate the bit into bl
	dec	ah			; go for another bit?
	jge	vgareadloop		;  sure, why not.
	mov	ds:[di],bl		; returned pixel value
	pop	bx			; restore BX
	inc	di			; set up the next dest addr
	cmp	di,bx			; are we beyond the end?
	ja	vgaline3		;  yup.  We done.
	inc	cl			; alter bit mask value
	cmp	cl,8			; time to update SI:
	jne	vgaline2		;  nope
	inc	si			;  yup
	mov	cl,0			;  ...
vgaline2:
	jmp	short vgaline1		;  do another one.

vgaline3:
	call	videocleanup		; else cleanup time.
	ret				;  and we done.

bank_is_changing:
	pop	dx			; restore the registers
	pop	ax			;  ...
	pop	cx			;  ...
	call	normalineread		; just calling newbank didn't quite
	ret				;  work. This depends on no bank
vgareadline	endp			;  change mid line (ok for 1024 wide)

super256lineaddr    proc    near		; super VGA 256 colors
	mov	ax,xdots	 ; this many dots / line
	mov	bx,dx		 ; rowcount
	mul	bx		 ; times this many lines
	push	ax		 ; save pixel address for later
	cmp	dx,curbk	 ; bank ok?
	push	dx		 ; save bank
	je	bank_is_ok	 ; jump if bank ok
	mov	al,dl		 ; newbank needs bank in al
	call	far ptr newbank
bank_is_ok:
	inc	bx		 ; next row
	mov	ax,xdots	 ; this many dots / line
	mul	bx		 ; times this many lines
	sub	ax,1		 ; back up some to the last pixel of the
	sbb	dx,0		 ; previous line
	pop	bx		 ; bank at start of row
	pop	ax		 ; ax = offset of row in video memory
	ret
super256lineaddr endp

super256line	proc	near		; super VGA 256 colors
	push	ax			; stop col
	push	dx			; row
	call super256lineaddr		; ax=video,dl=newbank,bl=oldbank
	mov	di,ax			; video offset
	cmp	dl,bl			; did bank change?
	pop	dx			; row
	pop	ax			; stop col
	jne	bank_did_chg
	add	di,cx			; add start col to video address
	sub	ax,cx			; ax = stop - start
	mov	cx,ax			;  + start column
	inc	cx			; number of bytes to move
	rep	movsb			; zap line into memory
	jmp	short linedone
bank_did_chg:
	call	normaline		; normaline can handle bank change
linedone:
	ret
super256line	endp

super256readline    proc    near     ; super VGA 256 colors
	push	ax			; stop col
	push	dx			; row
	call super256lineaddr		; ax=video,dl=newbank,bl=oldbank
	mov	si,ax			; video offset
	cmp	dl,bl			; did bank change?
	pop	dx			; row
	pop	ax			; stop col
	jne	bank_did_chg

	add	si,cx			; add start col to video address
	sub	ax,cx			; ax = stop - start
	mov	cx,ax			;  + start column
	inc	cx			; number of bytes to move
	mov	ax,ds			; save data segment to es
	mov	es,ax
	mov	ax,videomem		; video segment to es
	mov	ds,ax
	rep	movsb			; zap line into memory
	mov	ax,es			; restore data segment to ds
	mov	ds,ax
	jmp	short linedone
bank_did_chg:
	call	normalineread		; normaline can handle bank change
linedone:
	ret
super256readline    endp

tweak256line	proc	near		; Normal Line:	no assumptions
  local plane:byte
	mov	bx,ax			; bx = stop col
	sub	bx,cx			; bx = stop-start
	inc	bx			; bx = how many pixels to write
	cmp	bx,3			; less than four points?
	jg	nottoosmall		; algorithm won't work as written
	call	normaline		;  - give up and call normaline
	ret				; we done
nottoosmall:				; at least four points - go for it!
	push	bx			; save number of pixels
	and	bx,3			;  pixels modulo 4 = no of extra pts
	mov	ax,xdots		; width of video row
	shr	ax, 1
	shr	ax, 1			; now ax = xdots/4
	mul	dx			; ax points to start of desired row
	push	cx			; Save starting column for later
	shr	cx,1			; There are 4 pixels at each address
	shr	cx,1			;   so divide X by 4
	add	ax,cx			; Point to pixel's address
	mov	di,ax			; video offset of first point
	pop	cx			; Retrieve starting column
	and	cl,3			; Get the plane number of the pixel
	mov	ah,1
	shl	ah,cl			; Set the bit corresponding to the plane
					;  the pixel is in
	mov	plane,ah		; Save starting plane for ending test
	mov	al,MAP_MASK		;
	mov	dx,SC_INDEX
	pop	cx			; number of pixels to write
	shr	cx,1
	shr	cx,1			; cx = number of pixels/4
	cmp	bx,0			; extra pixels?
	je	tweak256line1		; nope - don't add one
	inc	cx			; yup - add one more pixel
tweak256line1:
	OUT	 DX,AX			; set up VGA registers for plane
	push	cx			; save registers changed by movsb
	push	si			;  ...
	push	di			;  ...
tweak256line2:
	movsb				; move the next pixel
	add	si,3			; adjust the source addr (+4, not +1)
	loop	tweak256line2		; loop if more dots this pass
	pop	di			; restore the saved registers
	pop	si			;  ...
	pop	cx			;  ...
	dec	bx			; one less extra pixel
	cmp	bx,0			; out of extra pixels?
	jne	noextra
	dec	cx			; yup - next time one fewer to write
noextra:
	inc	si			; offset the source 1 byte
	shl	ah,1			; set up for the next video plane
	cmp	ah,16			; at last plane?
	jne	notlastplane
	mov	ah,1			; start over with plane 0
	inc	di			; bump up video memory
notlastplane:
	cmp	ah,plane		; back to first plane?
	jne	tweak256line1		;  nope.  perform another loop.
	ret
tweak256line	endp

tweak256readline	proc	near		; Normal Line:	no assumptions
  local plane:byte
	mov	bx,ax			; bx = stop col
	sub	bx,cx			; bx = stop-start
	inc	bx			; bx = how many pixels to write
	cmp	bx,3			; less than four points?
	jg	nottoosmall		; algorithm won't work as written
	call	normalineread		;  - give up and call normalineread
	ret				; we done
nottoosmall:				; at least four points - go for it!
	push	bx			; save number of pixels
	and	bx,3			;  pixels modulo 4 = no of extra pts
	mov	ax,xdots		; width of video row
	shr	ax, 1
	shr	ax, 1			; now ax = xdots/4
	mul	dx			; ax points to start of desired row
	push	cx			; Save starting column for later
	shr	cx,1			; There are 4 pixels at each address
	shr	cx,1			;   so divide X by 4
	add	ax,cx			; Point to pixel's address
	mov	si,ax
	pop	cx			; Retrieve starting column
	and	cl,3			; Get the plane number of the pixel
	mov	ah,cl
	mov	plane,ah		; Save starting plane
	mov	al,READ_MAP
	mov	dx,GC_INDEX
	pop	cx			; number of pixels to write
	shr	cx,1
	shr	cx,1			; cx = number of pixels/4
	cmp	bx,0			; extra pixels?
	je	tweak256line1		; nope - don't add one
	inc	cx			; yup - add one more pixel
tweak256line1:
	out	dx,ax
	push	ax			; save registers
	push	cx			;  ...
	push	di			;  ...
	push	si			;  ...
	mov	ax,ds			; copy data segment to es
	mov	es,ax			;  ...
	mov	ax,videomem		; copy video segment to ds
	mov	ds,ax			;  ...
tweak256line2:
	movsb				; move the next pixel
	add	di,3			; adjust the source addr (+4, not +1)
	loop	tweak256line2		; loop if more dots this pass
	mov	ax,es
	mov	ds,ax			; restore data segement to ds
	pop	si			; restore the saved registers
	pop	di			;  ...
	pop	cx			;  ...
	pop	ax			;  ...
	dec	bx			; one less extra pixel
	cmp	bx,0			; out of extra pixels?
	jne	noextra
	dec	cx			; yup - next time one fewer to write
noextra:
	inc	di			; offset the source 1 byte
	inc	ah			; set up for the next video plane
	and	ah,3
	cmp	ah,0			; at last plane?
	jne	notlastplane
	inc	si			; bump up video memory
notlastplane:
	cmp	ah,plane		; back to first plane?
	jne	tweak256line1		;  nope.  perform another loop.
	ret
tweak256readline	endp


f85line proc	near

	call	fr85wbox	;put out the box
	ret

f85line endp

f85readline proc    near

	call	fr85rbox	;read the box
	ret

f85readline endp


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


; ******************** Zoombox functions **************************

; special code for 8514, commented out for now since it assumes rectangular box
;	push	es			; it's popped at the end of 'drawbox'
;	push	ds			; set ES == DS momentarily
;	pop	es			;  ...
;	mov	cx,512			; set up an array of 'FF's
;	mov	ax,0ffffh		;  (used for XOR operations in
;	mov	di,offset boxx+10	;  the 8514/A zoom-box routines)
;	rep	stosw			;  ...
;	cmp	boxcount,0		; is there an old box up?
;	je	draw8514		;  nope.  Skip this code.
;	mov	ax,boxx 		; restore bottom line
;	mov	cx,boxx+2		;  ...
;	mov	bx,boxy 		;  ...
;	mov	dx,boxy 		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,boxx 		; restore top line
;	mov	cx,boxx+2		;  ...
;	mov	bx,boxy+2		;  ...
;	mov	dx,boxy+2		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,boxx 		; restore left
;	mov	cx,boxx 		;  ...
;	mov	bx,boxy 		;  ...
;	mov	dx,boxy+2		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,boxx+2		; restore right
;	mov	cx,boxx+2		;  ...
;	mov	bx,boxy 		;  ...
;	mov	dx,boxy+2		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	boxcount,0		; flag zoom-box is down
;draw8514:
;	mov	ax,ixmax		; save coords for later
;	mov	cx,ixmin		;  ...
;	mov	bx,iymax		;  ...
;	mov	dx,iymin		;  ...
;	mov	boxx,ax 		; save coords for later
;	mov	boxx+2,cx		;  ...
;	mov	boxy,bx 		;  ...
;	mov	boxy+2,dx		;  ...
;	cmp	newbox,0		; time to draw a new box?
;	je	end8514 		;  nope
;	mov	ax,ixmax		; draw top line
;	mov	cx,ixmin		;  ...
;	mov	bx,iymin		;  ...
;	mov	dx,iymin		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,ixmax		; draw bottom line
;	mov	cx,ixmin		;  ...
;	mov	bx,iymax		;  ...
;	mov	dx,iymax		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,ixmin		; draw left line
;	mov	cx,ixmin		;  ...
;	mov	bx,iymax		;  ...
;	mov	dx,iymin		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	ax,ixmax		; draw right line
;	mov	cx,ixmax		;  ...
;	mov	bx,iymax		;  ...
;	mov	dx,iymin		;  ...
;	mov	si, offset boxx+10	; point to a line of 'ff's
;	call	fr85zoom		; do it
;	mov	boxcount,100		; flag zoom-box is up
;end8514:
;      jmp     endofdrawbox	       ;  we done

clearbox proc	 uses di si es
	mov	xorTARGA,1		; faster to flag xorTARGA rather
					; than check if TARGA is runnin

	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	bx,boxcount		; load up a counter: # points to clear
	dec	bx			; switch to an offset value
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
	mov	xorTARGA,0		; in case of TARGA, no more xor
	call	videocleanup		; perform any video cleanup required
	ret				; we done.
clearbox endp

dispbox proc	uses di si es
; TARGA 3 June 89 j mclain
	mov	xorTARGA,1		; faster to flag xorTARGA rather
					; than check if TARGA is runnin

	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine

	mov	bx,boxcount		; load up a counter: # points to draw
	dec	bx			; switch to an offset
readnewbox:
	shl	bx,1			; switch to word counter
	mov	cx,boxx[bx]		; get the (new) point location
	mov	dx,boxy[bx]		;  ...
	shr	bx,1			; switch back to character counter
	push	bx			; save the counter
	call	dotread 		; read the (previous) dot value
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
	mov	ax,colors		; set the (new) box color
	dec	ax
	and	ax,boxcolor
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
	mov	xorTARGA,0		; in case of TARGA, no more xor
					;
	call	videocleanup		; perform any video cleanup required
	ret				; we done.

dispbox endp


; **************** Function setvideomode(ax, bx, cx, dx) ****************

;	This function sets the (alphanumeric or graphic) video mode
;	of the monitor.   Called with the proper values of AX thru DX.
;	No returned values, as there is no particular standard to
;	adhere to in this case.

;	(SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;	genuine VGA or register compatable adapter and program the registers
;	directly using the coded value in DX)

setdacfortext	proc
	cmp	loadPalette,1		; palette override?
	jne	return			;  nope.
	cmp	video_type,4		; VGA or better?
	jb	return			;  nope
	mov	ax,1015h		; read a specific DAC register
	mov	bx,0			;  background color
	mov	cx,0ffffh		; stuff an invalid value into CX
	int	10h			;  do it
	mov	saved_dacreg,cx 	; save the DAC register
	mov	saved_dacreg+2,dx	;  ..
	mov	ax,1015h		; read a specific DAC register
	mov	bx,7			;  foreground color
	mov	cx,0ffffh		; stuff an invalid value into CX
	int	10h			;  do it
	mov	saved_dacreg+4,cx	; save the DAC register
	mov	saved_dacreg+6,dx	;  ..
	mov	ax,1010h		; write a specific DAC register
	mov	bx,0			;  background color
	mov	cx,0000h		; restore the DAC register
	mov	dx,0000h		;  ..
	int	10h			;  do it
	mov	ax,1010h		; write a specific DAC register
	mov	bx,7			; foreground color
	mov	cx,2a2ah		; restore the DAC register
	mov	dx,2a2ah		;  ..
	int	10h			;  do it
return:
	ret
setdacfortext	endp

setdacforgraphics	proc
	cmp	loadPalette,1		; palette override?
	jne	return			;  nope.
	cmp	video_type,4		; VGA or better?
	jb	return			;  nope
	cmp	saved_dacreg,0ffffh	; valid saved-DAC value?
	je	return			;  nope
	mov	ax,1010h		; write a specific DAC register
	mov	bx,0			;  background color
	mov	cx,saved_dacreg 	; restore the DAC register
	mov	dx,saved_dacreg+2	;  ..
	int	10h			;  do it
	mov	ax,1010h		; write a specific DAC register
	mov	bx,7			;  foreground color
	mov	cx,saved_dacreg+4	; restore the DAC register
	mov	dx,saved_dacreg+6	;  ..
	int	10h			;  do it
	mov	saved_dacreg,0ffffh	; invalidate the saved_dac
return:
	ret
setdacforgraphics	endp

setvideomode	proc	uses di si es,argax:word,argbx:word,argcx:word,argdx:word

	cmp	videoflag,1		; say, was the last video your-own?
	jne	novideovideo		;  nope.
	call	videoend		; yup.	end the your-own-video mode
	mov	videoflag,0		; set flag: no your-own-video
	jmp	short notarga
novideovideo:
	cmp	diskflag,1		; say, was the last video disk-video?
	jne	nodiskvideo		;  nope.
	call	diskend 		; yup.	end the disk-video mode
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
	call	f85end
	mov	f85flag, 0
no8514:
	cmp	HGCflag, 1		; was last video Hercules
	jne	noHGC			; nope
	call	hgcend
	mov	HGCflag, 0
	cmp	argax,3 		; klooge: Hercules => text?
	jne	noHGC
	mov	dotmode,0		; reset the DOTmode for BIOS
noHGC:
	mov	oktoprint,1		; say it's OK to use printf()
	mov	goodmode,1		; assume a good video mode
	mov	ax,video_bankadr	; restore the results of 'whichvga()'
	mov	[bankadr],ax		;  ...
	mov	ax,video_bankseg	;  ...
	mov	[bankseg],ax		;  ...

	mov	ax,argax		; load up for the interrupt call
	mov	bx,argbx		;  ...
	mov	cx,argcx		;  ...
	mov	dx,argdx		;  ...

	mov	videoax,ax		; save the values for future use
	mov	videobx,bx		;  ...
	mov	videocx,cx		;  ...
	mov	videodx,dx		;  ...

	call	setvideo		; call the internal routine first

	cmp	goodmode,0		; is it still a good video mode?
	jne	videomodeisgood 	; yup.
	mov	ax,2			; call the buzzer
	push	ax
	call	buzzer
	pop	ax
	mov	ah,9			; display an error message
	mov	dx, offset badvideomsg
	int	21h
	mov	ax,offset nullwrite	; set up null write-a-dot routine
	mov	bx,offset mcgaread	; set up null read-a-dot  routine
	mov	cx,offset normaline	; set up normal linewrite routine
	mov	dx,offset mcgareadline	; set up normal linewrite routine
	jmp	videomode		; return to common code

videomodeisgood:
	mov	bx,dotmode		; set up for a video table jump
	cmp	bx,30			; are we within the range of dotmodes?
	jbe	videomodesetup		; yup.	all is OK
	mov	bx,0			; nope.  use dullnormalmode
videomodesetup:
	shl	bx,1			; switch to a word offset
	mov	bx,cs:videomodetable[bx]	; get the next step
	jmp	bx			; and go there
videomodetable	dw	offset dullnormalmode	; mode 0
	dw	offset dullnormalmode	; mode 1
	dw	offset vgamode		; mode 2
	dw	offset mcgamode 	; mode 3
	dw	offset tseng256mode	; mode 4
	dw	offset paradise256mode	; mode 5
	dw	offset video7256mode	; mode 6
	dw	offset tweak256mode	; mode 7
	dw	offset everex16mode	; mode 8
	dw	offset targaMode	; mode 9
	dw	offset hgcmode		; mode 10
	dw	offset diskmode 	; mode 11
	dw	offset f8514mode	; mode 12
	dw	offset cgacolor 	; mode 13
	dw	offset tandy1000tlmode	; mode 14
	dw	offset trident256mode	; mode 15
	dw	offset chipstech256mode ; mode 16
	dw	offset ati256mode	; mode 17
	dw	offset everex256mode	; mode 18
	dw	offset yourownmode	; mode 19
	dw	offset ati1024mode	; mode 20
	dw	offset tseng16mode	; mode 21
	dw	offset trident16mode	; mode 22
	dw	offset video716mode	; mode 23
	dw	offset paradise16mode	; mode 24
	dw	offset chipstech16mode	; mode 25
	dw	offset everex16mode	; mode 26
	dw	offset VGAautomode	; mode 27
	dw	offset VESAmode 	; mode 28
	dw	offset dullnormalmode	; mode 29
	dw	offset dullnormalmode	; mode 30
	dw	offset dullnormalmode	; mode 31

tandy1000tlmode:
	mov	ax,offset tandywrite	; set up the Tandy write-a-dot routine
	mov	bx,offset tandyread	; set up the Tandy read-a-dot  routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	jmp	videomode		; return to common code
dullnormalmode:
	mov	ax,offset normalwrite	; set up the BIOS write-a-dot routine
	mov	bx,offset normalread	; set up the BIOS read-a-dot  routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	jmp	videomode		; return to common code
mcgamode:
	mov	ax,offset mcgawrite	; set up MCGA write-a-dot routine
	mov	bx,offset mcgaread	; set up MCGA read-a-dot  routine
	mov	cx,offset mcgaline	; set up the MCGA linewrite routine
;	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset mcgareadline	; set up the normal lineread  routine
	jmp	videomode		; return to common code
tseng16mode:
	mov	tseng,1 		; set chipset flag
	mov	[bankadr],offset $tseng
	mov	[bankseg],seg $tseng
	jmp	vgamode 	; set ega/vga functions
trident16mode:
	mov	trident,1		; set chipset flag
	mov	[bankadr],offset $trident
	mov	[bankseg],seg $trident
	jmp	vgamode
video716mode:
	mov	video7,1		; set chipset flag
	mov	[bankadr],offset $video7
	mov	[bankseg],seg $video7
	jmp	vgamode
paradise16mode:
	mov	paradise,1		; set chipset flag
	mov	[bankadr],offset $paradise
	mov	[bankseg],seg $paradise
	jmp	vgamode
chipstech16mode:
	mov	chipstech,1		; set chipset flag
	mov	[bankadr],offset $chipstech
	mov	[bankseg],seg $chipstech
	jmp	vgamode
everex16mode:
	mov	everex,1		; set chipset flag
	mov	[bankadr],offset $everex
	mov	[bankseg],seg $everex
	jmp	vgamode
VESAmode:				; set VESA 16-color mode
	mov	[bankadr],offset $vesa
	mov	[bankseg],seg $vesa
VGAautomode:				; set VGA auto-detect mode
	cmp	colors,256		; 256 colors?
	je	VGAauto256mode		; just like SuperVGA
	cmp	colors,16		; 16 colors?
	je	vgamode 		; just like a VGA
	jmp	dullnormalmode		; otherwise, use the BIOS
VGAauto256mode:
	jmp	super256mode		; just like a SuperVGA
egamode:
vgamode:
	mov	ax,offset vgawrite	; set up EGA/VGA write-a-dot routine.
	mov	bx,offset vgaread	; set up EGA/VGA read-a-dot  routine
	mov	cx,offset vgaline	; set up the EGA/VGA linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	dx,offset vgareadline	; set up the normal lineread  routine
	jmp	videomode		; return to common code
tseng256mode:
	mov	tseng,1 		; set chipset flag
	mov	[bankadr],offset $tseng
	mov	[bankseg],seg $tseng
	jmp	super256mode		; set super VGA linear memory functions
paradise256mode:
	mov	paradise,1		; set chipset flag
	mov	[bankadr],offset $paradise
	mov	[bankseg],seg $paradise
	jmp	super256mode		; set super VGA linear memory functions
video7256mode:
	mov	video7, 1		; set chipset flag
	mov	[bankadr],offset $video7
	mov	[bankseg],seg $video7
	jmp	super256mode		; set super VGA linear memory functions
trident256mode:
	mov	trident,1		; set chipset flag
	mov	[bankadr],offset $trident
	mov	[bankseg],seg $trident
	jmp	super256mode		; set super VGA linear memory functions
chipstech256mode:
	mov	chipstech,1		; set chipset flag
	mov	[bankadr],offset $chipstech
	mov	[bankseg],seg $chipstech
	jmp	super256mode		; set super VGA linear memory functions
ati256mode:
	mov	ativga,1		; set chipset flag
	mov	[bankadr],offset $ativga
	mov	[bankseg],seg $ativga
	jmp	super256mode		; set super VGA linear memory functions
everex256mode:
	mov	everex,1		; set chipset flag
	mov	[bankadr],offset $everex
	mov	[bankseg],seg $everex
	jmp	super256mode		; set super VGA linear memory functions
VGA256automode: 			; Auto-detect SuperVGA
	jmp	super256mode		; set super VGA linear memory functions
VESA256mode:				; set VESA 256-color mode
	mov	[bankadr],offset $vesa
	mov	[bankseg],seg $vesa
	jmp	super256mode		; set super VGA linear memory functions
super256mode:
	mov	ax,offset super256write ; set up superVGA write-a-dot routine
	mov	bx,offset super256read	; set up superVGA read-a-dot  routine
	mov	cx,offset super256line	; set up the  linewrite routine
	mov	dx,offset super256readline ; set up the normal lineread  routine
;	mov	dx,offset normalineread ; set up the normal lineread  routine
	jmp	videomode		; return to common code
tweak256mode:
	mov	oktoprint,0		; NOT OK to printf() in this mode
	mov	ax,offset tweak256write ; set up tweaked-256 write-a-dot
	mov	bx,offset tweak256read	; set up tweaked-256 read-a-dot
	mov	cx,offset tweak256line	; set up tweaked-256 read-a-line
;	mov	cx,offset normaline	; set up the normal linewrite routine
;	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	dx,offset tweak256readline ; set up the normal lineread  routine
	jmp	videomode		; return to common code
cgacolor:
	mov	ax,offset cgawrite	; set up CGA write-a-dot
	mov	bx,offset cgaread	; set up CGA read-a-dot
;	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	jmp	videomode		; return to common code
ati1024mode:
	mov	ativga,1		; set ATI flag.
	mov	ax,offset ati1024write	; set up ATI1024 write-a-dot
	mov	bx,offset ati1024read	; set up ATI1024 read-a-dot
;	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	jmp	videomode		; return to common code
diskmode:
	call	diskstart		; start up the disk routines
	mov	ax,offset diskwrite	; set up disk-vid write-a-dot routine
	mov	bx,offset diskread	; set up disk-vid read-a-dot routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	diskflag,1		; flag "disk-end" needed.
	jmp	videomode		; return to common code
yourownmode:
	call	videostart		; start up your-own-video routines
	mov	ax,offset videowrite	; set up ur-own-vid write-a-dot routine
	mov	bx,offset videoread	; set up ur-own-vid read-a-dot routine
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	videoflag,1		; flag "your-own-end" needed.
	jmp	videomode		; return to common code
targaMode:				; TARGA MODIFIED 2 June 89 - j mclain
	call	tgastart		;
	mov	ax,offset tgawrite	;
	mov	bx,offset tgaread	;
;	mov	cx,offset tgaline	;
	mov	cx,offset normaline	; set up the normal linewrite routine
	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	tgaflag,1		;
	jmp	videomode		; return to common code
f8514mode:		       ; 8514 modes
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
	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	HGCflag,1		; flag "HGC-end" needed.
	jmp	videomode		; return to common code
f85ok:
	mov	ax,offset f85write	;
	mov	bx,offset f85read	;
	mov	cx,offset f85line	;
	mov	dx,offset f85readline	;
;	mov	cx,offset normaline	; set up the normal linewrite routine
;	mov	dx,offset normalineread ; set up the normal lineread  routine
	mov	f85flag,1		;
	mov	oktoprint,0		; NOT OK to printf() in this mode
	jmp	videomode		; return to common code


videomode:
	mov	dotwrite,ax		; save the results
	mov	dotread,bx		;  ...
	mov	linewrite,cx		;  ...
	mov	lineread,dx		;  ...

	mov	ax,colors		; calculate the "and" value
	dec	ax			; to use for eventual color
	mov	andcolor,ax		; selection

	mov	boxcount,0		; clear the zoom-box counter

	mov	daclearn,0		; set the DAC rotates to learn mode
	mov	daccount,6		; initialize the DAC counter
	cmp	cpu,88			; say, are we on a 186/286/386?
	jbe	setvideoslow		;  boo!  hiss!
	mov	daclearn,1		; yup.	bypass learn mode
	mov	ax,cyclelimit		;  and go as fast as he wants
	mov	daccount,ax		;  ...
setvideoslow:

	call	far ptr loaddac 	; load the video dac, if we can

	cmp	loadPalette,1		; palette override?
	jne	textorgraphics		;  nope.
	cmp	videoax,3		; klooge:  text (3,0,0,0) mode?
	jne	nottextmode		;  nope.
	cmp	videodx,0		;  ...
	jne	nottextmode		;  nope.
	call	far ptr setdacfortext	; setup the DAC for text mode
	jmp	short textorgraphics	; jump to common code
nottextmode:
	call	far ptr setdacforgraphics ; setup the DAC for graphics mode
textorgraphics:

	ret
setvideomode	endp

setnullvideo proc
	mov	ax,offset nullwrite	; set up null write-a-dot routine
	mov	dotwrite,ax		;  ...
	mov	ax,offset nullread	; set up null read-a-dot routine
	mov	dotread,ax		;  ...
	ret
setnullvideo endp

setvideo	proc	near		; local set-video more

	cmp	ax,0			; TWEAK?:  look for AX==BX==CX==0

	jne	short setvideobios	;  ...
	cmp	bx,0			;  ...
	jne	short setvideobios	;  ...
	cmp	cx,0			;  ...
	jne	short setvideobios	;  ...

	cmp	dotmode, 27		; check for auto-detect modes
	je	setvideoauto1

	cmp	dotmode, 20		; check for auto-detect modes
	je	setvideoauto1

	cmp	dotmode, 4		; check for auto-detect modes
	je	setvideoauto1

	cmp	dotmode, 28		; check for auto-detect modes
	je	setvideoauto1

	jmp	setvideoregs		; anything else - assume register tweak

setvideoauto1:
	jmp	setvideoauto		; stupid short 'je' instruction!!

setvideobios:
	cmp	ax,3			; text mode?
	je	setvideobios_doit	;  yup.  Just do it.
	cmp	ax,6			; text mode?
	je	setvideobios_doit	;  yup.  Just do it.
	mov	si,dotmode		; compare the dotmode against
	mov	di,video_type		; the video type
	add	si,si			; (convert to a word pointer)
	cmp	cs:video_requirements[si],di
	ja	setvideoerror		;  yup.  Error.
setvideobios_doit:
	call	maybeor 		; maybe or AL or (for Video-7s) BL
	push	bp			; some BIOS's don't save this
	int	10h			; do it via the BIOS.
	pop	bp			; restore the saved register
	cmp	dotmode,14		; Tandy mode?
	jne	setvideobios_notandy	;  nope.
	cmp	ax,3			; trying to get into text mode?
	je	setvideobios_notandy	;  yup - avoid tandy-specific code.
	cmp	ax,6			; trying to get into text mode?
	je	setvideobios_notandy	;  yup - avoid tandy-specific code.
	cmp	ax,83h			; trying to get into text mode?
	je	setvideobios_notandy	;  yup - avoid tandy-specific code.
	cmp	ax,86h			; trying to get into text mode?
	je	setvideobios_notandy	;  yup - avoid tandy-specific code.
	call	tandysetup		; tandy-specific setup
setvideobios_notandy:
	cmp	dotmode,28		; VESA mode?
	jne	setvideobios_worked	;  Nope. Return.
	cmp	ah,0			; did it work?
	jne	setvideoerror		;  Nope. Failed.
	mov	vesa_granularity,1	; say use 64K granules
	push	es			; set ES == DS
	mov	ax,ds			;  ...
	mov	es,ax			;  ...
	mov	ax,4f01h		; ask about this video mode
	mov	cx,bx			;  this mode
	mov	di, offset suffix	; (a safe spot for 256 bytes)
	int	10h			; do it
;	mov	cx, word ptr suffix	; get the attributes
;	test	cx,1			; available video mode?
;	jz	nogoodvesamode		; nope.  skip some code
	mov	cx, word ptr suffix+12	; get the Bank-switching routine
	mov	word ptr vesa_bankswitch, cx  ;  ...
	mov	cx, word ptr suffix+14	;   ...
	mov	word ptr vesa_bankswitch+2, cx	;  ...
	mov	cx, word ptr suffix+4	; get the granularity
	cmp	cl,1			; ensure the divide won't blow out
	jb	nogoodvesamode		;  granularity == 0???
	mov	ax,64			;  ...
	div	cl			; divide 64K by granularity
	mov	vesa_granularity,al	; multiply the bank number by this
nogoodvesamode:
	pop	es			; restore ES
	mov	ax,4f02h		; restore the original call
setvideobios_worked:
	jmp	setvideoreturn		;  Return.

setvideoerror:				; oops.  No match found.
	mov	goodmode,0		; note that the video mode is bad
	mov	ax,3			; switch to text mode
	jmp	setvideobios_doit

setvideoauto:
	mov	si, video_entries	; look for the correct resolution
	sub	si,8			; get a running start
setvideoloop:
	add	si,10			; get next entry
	mov	ax,cs:0[si]		; check X-res
	cmp	ax,0			; anything there?
	je	setvideoerror		; nope.  No match
	cmp	ax,xdots
	jne	setvideoloop
	mov	ax,cs:2[si]		; check Y-res
	cmp	ax,ydots
	jne	setvideoloop
	mov	ax,cs:4[si]		; check Colors
	cmp	ax,colors
	jne	setvideoloop
	mov	ax,cs:6[si]		; got one!  Load AX
	mov	bx,cs:8[si]		;	    Load BX
	cmp	bx,0ffh 		; ATI 1024x768x16 special?
	jne	notatimode
	mov	dotmode,20		; Convert to ATI specs
	mov	al,65h
	mov	bx,0
	jmp	setvideobios
notatimode:
	cmp	bx,0feh 		; Tseng 640x400x256 special?
	jne	nottsengmode
	mov	ax,0			; convert to Tseng specs
	mov	bx,0
	mov	cx,0
	mov	dx,10
	mov	dotmode,4
	jmp	setvideoregs
nottsengmode:
	cmp	ax,4f02h		; VESA mode?
	jne	notvesamode
	mov	dotmode,28		; convert to VESA specs
	jmp	setvideobios
notvesamode:
	jmp	setvideobios

setvideoregs:				; assume genuine VGA and program regs
	mov	si, dotmode		; compare the dotmode against
	mov	di,video_type		; the video type
	add	si,si			; (convert to a word pointer)
	cmp	cs:video_requirements[si],di
	jbe	setvideoregs_doit	;  good value.	Do it.
	jmp	setvideoerror		;  bad value.  Error.
setvideoregs_doit:
	mov	si,dx			; get the video table offset
	shl	si,1			;  ...
	mov	si,word ptr tweaks[si]	;  ...

	mov	tweaktype, dx		; save tweaktype
	cmp	dx,8			; 360x480 tweak256mode?
	je	isatweaktype		; yup
	cmp	dx,9			; 320x400 tweak256mode?
	je	isatweaktype		; yup
	cmp	dx,10			; Tseng tweak?
	je	tsengtweak		; yup
;Patch - Michael D. Burkey (5/22/90)
	cmp	dx,14			; ATI Mode Support
	je	ATItweak
	cmp	dx,15
	je	ATItweak
	cmp	dx,16
	je	ATItweak
	cmp	dx,17
	je	ATItweak2		; ATI 832x616 mode
	cmp	dx,11			; tweak256mode? (11 & up)
	jae	isatweaktype		; yup
;End Patch
	jmp	not256			; nope none of the above
tsengtweak:
	mov	ax,46			; start with S-VGA mode 2eh
	call	maybeor 		; maybe don't clear the video memory
	int	10h			; let the bios clear the video memory
	mov	dx,3c2h 		; misc output
	mov	al,063h 		; dot clock
	out	dx,al			; select it
	mov	dx,3c4h 		; sequencer again
	mov	ax,0300h		; restart sequencer
	out	dx,ax			; running again
	jmp	is256;
ATItweak:
	mov	ax,62h
	int	10h
	mov	dx,3c2h
	mov	al,0e3h
	out	dx,al
	mov	dx,3c4h
	mov	ax,0300h
	out	dx,ax
	jmp	is256

ATItweak2:
	mov	ax,63h
	int	10h
	mov	dx,3c4h
	mov	ax,0300h
	out	dx,ax
	jmp	is256

isatweaktype:
	 mov	    ax,0013h		 ; invoke video mode 13h
	 call	    maybeor		 ; maybe or AL or (for Video-7s) BL
	 int	    10h 		 ; do it

	 mov	    dx,3c4h		 ; alter sequencer registers
	 mov	    ax,0604h		 ; disable chain 4
	 out	    dx,ax

	 cmp	   orvideo,0		 ; are we supposed to clear RAM?
	 jne	   noclear256		 ;  (nope)

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
	mov	dx,3c4h 		; alter sequencer registers
	mov	ax,0604h		; disable chain 4
	out	dx,ax

	jmp	short is256		; forget the ROM characters

not256:

	mov	ax,0012h		; invoke video mode 12h
	call	maybeor 		; maybe or AL or (for Video-7s) BL
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
	mul	byte ptr [si+1] 	; number of characters on the screen
	shl	ax,1			; (attributes, also)
	mov	word ptr es:[4ch],ax

	mov	dx,word ptr es:[63h]	; say, where's the 6845?
	add	dx,6			; locate the status register
vrdly1: in	al,dx			; loop until vertical retrace is off
	test	al,8			;   ...
	jnz	vrdly1			;   ...
vrdly2: in	al,dx			; now loop until it's on!
	test	al,8			;   ...
	jz	vrdly2			;   ...

	cli				; turn off all interrupts
	mov	dx,tweaktype
	cmp	dx,9			; 320x400 mode?
	je	not256mode		; yup - skip this stuff
	cmp	dx,10			; Tseng tweak mode?
	je	not256mode		; yup - skip this stuff
;patch #2 (M. Burkey 5/22/90)
	cmp	dx,17			; for 832x616 ATI Mode
	je	not256mode
;patch end
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

maybeor proc	near			; or AL or BL for mon-destr switch
	cmp	ah,4fh			; VESA special mode?
	je	maybeor2		;  yup.  Do this one different
	cmp	ah,6fh			; video-7 special mode?
	je	maybeor1		;  yup.  do this one different
	or	al,orvideo		; normal non-destructive switch
	jmp	short maybeor99 	; we done.
maybeor1:
	or	bl,orvideo		; video-7 switch
	jmp	short maybeor99
maybeor2:
	or	bh,orvideo		; VESA switch
maybeor99:
	ret				; we done.
maybeor endp


; ********* Functions setfortext() and setforgraphics() ************

;	setfortext() resets the video for text mode and saves graphics data
;	setforgraphics() restores the graphics mode and data
;	setclear() clears the screen after setfortext() [which may be wierd]

monocolors db  0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

setfortext	proc	uses es si di
	cmp	dotmode, 12		;check for 8514
	jne	tnot8514
	cmp	f85flag, 0		;check 8514 active flag
	je	dosettext
	call	close8514		;close adapter if not
	mov	f85flag, 0
	jmp	short dosettext
tnot8514:
	cmp	videoax,0		; check for CGA modes
	je	setfortextnocga 	;  not this one
	cmp	ydots,348		; (only Hercules modes have this res)
	je	setfortextcga		;  ...
	cmp	dotmode,14		; (klooge for Tandy 640x200x16)
	je	setfortextcga
	cmp	videoax,7		;  ...
	ja	setfortextnocga 	;  not this one
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
	cmp	dotmode,14		; (klooge for Tandy 640x200x16)
	jne	setfortextnotandy
	mov	di,0			; save the video data here
	mov	ax,0a000h		; video data starts here <XXX>
	mov	cx,4000h		; save this many words
setfortextnotandy:
	push	ds			; save DS for a tad
	mov	ds,ax			;  reset DS
	cld				; clear the direction flag
	rep	movsw			;  save them.
	pop	ds			; restore DS
	cmp	ydots,348		;check for Hercules-specific dotmode
	jne	tnotHGC
	cmp	HGCflag, 0		;check HGC active flag
	je	dosettext
	call	hgcend			;close adapter if not
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
	mov	bx,23			; set mode 6 fgrd to grey
	mov	cx,2a2ah		; register 23, rgb white
	mov	dh,2ah			;  ...
	mov	ax,1010h		; int 10 10-10 affects mcga,vga
	int	10h			;  ...
	mov	ax,cs			; do it again, another way
	mov	es,ax			;  ...
	mov	dx,offset monocolors	;  ...
	mov	ax,1002h		; int 10 10-02 handles pcjr,ega,vga
	int	10h			;  ...

setfortextreturn:
	call	far ptr home		; home the cursor
	ret
setfortext	endp

setforgraphics	proc	uses es si di
	cmp	 dotmode, 12		;check for 8514
	jne	gnot8514
	cmp	f85flag, 0
	jne	f85isgraphics
	call	reopen8514
	mov	f85flag, 1
f85isgraphics:
	jmp	setforgraphicsreturn
gnot8514:
	cmp	videoax,0		; check for CGA modes
	je	setforgraphicsnocga_x	;  not this one
	cmp	ydots,348		; (only Hercules modes have this res)
	je	setforgraphicscga	;  ...
	cmp	dotmode,14		; (klooge for Tandy 640x200x16)
	je	setforgraphicscga	;  ...
	cmp	videoax,7		;  ...
	jbe	setforgraphicscga	;  CGA mode
setforgraphicsnocga_x:
	jmp	setforgraphicsnocga
setforgraphicscga:
	cmp	ydots,348		;check for Hercules-specific dotmode
	jne	tnotHGC2		;  (nope.  dull-normal stuff)
	call	hgcstart		; Initialize the HGC card
	mov	HGCflag,1		; flag "HGC-end" needed.
	jmp	short twasHGC2		; bypass the normal setvideo call
tnotHGC2:
	cmp	dotmode,14		; tandy 640x200x16 mode?
	jne	tnottandy1		;  nope
	mov	orvideo,80h		; set the video to preserve memory
tnottandy1:
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
	jne	setforgraphicscganoherc ;  ...
	mov	si,0			; (restore 32K)
	mov	ax,0b000h		; (to here)
	mov	cx,4000h		; (restore this many words)
setforgraphicscganoherc:
	cmp	dotmode,14		; tandy 640x200x16 mode?
	jne	tnottandy2		;  nope
	mov	si,0			; (restore 32K)
	mov	ax,0a000h		; (to here)
	mov	cx,4000h		; (restore this many words)
tnottandy2:
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
	cmp	dotmode,11		; disk video mode?
	je	setforgraphicsend	;  yup, don't set real screen palette
	mov	ax,1			; set up call to spindac(0,1)
	push	ax			;  ...
	mov	ax,0			;  ...
	push	ax			;  ...
	call	far ptr spindac 	; do it.
	pop	ax			; restore the registers
	pop	ax			;  ...
setforgraphicsend:
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
	mov	cx,8192 		; this many blanks
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

; ************** Function findfont(n) ******************************

;	findfont(0) returns far pointer to 8x8 font table if it can
;		    find it, NULL otherwise;
;		    nonzero parameter reserved for future use

findfont	proc	uses es si di, fontparm:word
	mov	ax,01130h		; func 11, subfunc 30
	mov	bh,03h			; 8x8 font, bottom 128 chars
	sub	cx,cx			; so we can tell if anything happens
	int	10h			; ask bios
	sub	ax,ax			; default return, NULL
	sub	dx,dx			;  ...
	or	cx,cx			; did he set cx?
	jz	findfontret		; nope, return with NULL
	mov	dx,es			; yup, return far pointer
	mov	ax,bp			;  ...
findfontret:
	ret				; note that "uses" gets bp reset here
findfont	endp

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
	mov	cx,xdot 		; load up the registers
	mov	dx,ydot 		;  for the video routine
	call	dotread 		; read the dot via the approved method
	mov	ah,0			; clear the high-order bits
	ret				; we done.
getcolor	endp

; ************** Function putcolor(xdot, ydot, color) *******************

;	write the color on the screen at the (xdot,ydot) point

putcolor	proc	uses di si es, xdot:word, ydot:word, xcolor:word
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	cx,xdot 		; load up the registers
	mov	dx,ydot 		;  for the video routine
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
	mov	cx,xmin 		; x-loop
	dec	cx			; get a running start
putblx: inc	cx			; next x-dot
	mov	dx,ymin 		; y-loop
	dec	dx			; get a running start
putbly: inc	dx			; next y-dot
	push	cx			; save registers aroud the call
	push	dx			;  ...
	mov	ax,xcolor		;  ...
	call	dotwrite		; write the dot via the approved method
	pop	dx			; restore registers
	pop	cx			;  ...
	cmp	dx,ymax 		; done with y-loop?
	jne	putbly			; nope.
	cmp	cx,xmax 		; done with x-loop:
	jne	putblx			; nope.
	call	videocleanup		; perform any video cleanup required
	ret				; we done.
putblock	endp

; ***************Function out_line(pixels,linelen) *********************

;	This routine is a 'line' analog of 'putcolor()', and sends an
;	entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;	Called by the GIF decoder

out_line	proc	uses di si es, pixels:word, linelen:word
	xor	cx,cx			; start on left side
	mov	dx,rowcount		; sanity check: don't proceed
	cmp	dx,ydots		; beyond the end of the screen
	ja	out_lineret		;  ...
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	ax, linelen		; how many pixels to read
	dec	ax			; last pixel column
	mov	si,offset pixels	; get the color for dot 'x'
	call	linewrite		; mode-specific linewrite routine
	inc	rowcount		; next row
out_lineret:
	xor	ax,ax			; return 0
	ret

out_line	endp

; ***Function get_line(int row,int startcol,int stopcol, unsigned char *pixels) ***

;	This routine is a 'line' analog of 'getcolor()', and gets a segment
;	of a line from the screen and stores it in pixels[] at one byte per
;	pixel
;	Called by the GIF decoder

get_line	proc uses di si es, row:word, startcol:word, stopcol:word, pixels:word
	mov	cx,startcol		; sanity check: don't proceed
	cmp	cx,xdots		; beyond the right end of the screen
	ja	get_lineret		;  ...
	mov	dx,row			; sanity check: don't proceed
	cmp	dx,ydots		; beyond the bottom of the screen
	ja	get_lineret		;  ...
	mov	ax, stopcol		; how many pixels to read
	mov	di,offset pixels	; get the color for dot 'x'
	call	lineread		; mode-specific linewrite routine
get_lineret:
	xor	ax,ax			; return 0
	ret
get_line	endp

; ***Function put_line(int row,int startcol,int stopcol, unsigned char *pixels) ***

;	This routine is a 'line' analog of 'putcolor()', and puts a segment
;	of a line from the screen and stores it in pixels[] at one byte per
;	pixel
;	Called by the GIF decoder

put_line	proc uses di si es, row:word, startcol:word, stopcol:word, pixels:word
	mov	cx,startcol		; sanity check: don't proceed
	cmp	cx,xdots		; beyond the right end of the screen
	ja	put_lineret		;  ...
	mov	dx,row			; sanity check: don't proceed
	cmp	dx,ydots		; beyond the bottom of the screen
	ja	put_lineret		;  ...
	mov	ax,0a000h		; EGA, VGA, MCGA starts here
	mov	es,ax			; save it here during this routine
	mov	ax, stopcol		; how many pixels to read
	cmp	ax,xdots		; beyond the right end of the screen
	ja	put_lineret		;  ...
	mov	si,offset pixels	; put the color for dot 'x'
	call	linewrite		; mode-specific linewrite routine
put_lineret:
	xor	ax,ax			; return 0
	ret
put_line	endp

; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string, divider) where
;	  row, col = row and column to start printing.
;	  attr = color attribute.
;	  string = pointer to the null terminated string to print.
;	  divider = flag to add red column divider at end of string.
;    Written for the A86 assembler (which has much less 'red tape' than MASM)
;    by Bob Montgomery, Orlando, Fla.		  7-11-88
;    Adapted for MASM 5.1 by Tim Wegner 	 12-11-89
;    Furthur mucked up to handle graphics
;	video modes by Bert Tyler		  1-07-90

; .model medium,c

;-----------------------------------------------------------------
.data

extrn	  video_seg:word     ;Segment for video memory defined in C code.
extrn	  crtcols:word	     ;Columns/row
extrn	  crtrows:word
extrn	  isatextmode:word   ; 1 if text mode, 0 if graphice mode

;-----------------------------------------------------------------

.code

putstring	proc uses es di si, row:word, col:word, attr:word, string:word, divider:word

     cmp  isatextmode,1       ; are we in a text mode?
     je   put_text	      ; yup.  do it quickly.

put_graphics:
     push ds		      ; get address of text string
     pop  es		      ;  segment into ES
     mov  dx,col	      ; get location of string
     mov  cx,row
     mov  dh,cl
     mov  si,string	      ; find out how long the string is
     mov  cx,0
put_loop:
     lodsb		      ; are we at the zero yet?
     inc  cx
     cmp  al,0
     jne  put_loop	      ; nope.  Try again.
     dec  cx
     mov  bx,attr	      ; get the string attribute
     mov  ax,1300h	      ; set up the BIOS call
     mov  bh,0
     push bp		      ; ??? the BIOS call uses BP!!
     mov  bp,string	      ; put the string address into BP
     int  10h		      ; invoke the BIOS
     pop  bp		      ; restore BP
     jmp  B3		      ; we done.

put_text:
     mov  ax,row	      ;Get starting row
     mov  cx,crtcols	     ;Multiply by 2 x bytes/row (char & attr bytes)
     shl  cx,1
     imul cx
     add  ax,col	      ;Add starting column
     add  ax,col	      ;twice since 2 bytes/char
     mov  di,ax 	      ; di now points to start location in Video segment
     mov  ax,video_seg	     ;Get Video segment to es
     mov  es,ax
     mov  si,string	      ; ds:si points to string to print
     mov  ax,attr	      ;Get color attribute in ah
     xchg ah,al
; Now print the string
B1:  lodsb		      ;Get a char in al
     cmp  al,0		      ;End of string?
     je   B2		      ;Yes
     stosw		      ;No, store char & attribute
     jmp  B1		      ;Do next char

B2:  cmp  divider,0	      ;Divider requested?
     je   B3		      ;No
     mov  ax,04B3h	      ;Yes, red vertical line
	  stosw
B3:  ret
putstring endp


scroll	proc uses ds es di si, rows:word, startrow:word

; Get start offset and number of bytes to move.
     mov  ax,crtcols	     ;Get chars/row
     mov  bx,crtrows	     ;Get rows/screen
     sub  bx,rows	      ;Get rows not moved
     push bx		     ;Save rows/screen & cols/row
     push ax
     push ax
     mul  startrow	      ;Get chars to start
     shl  ax,1		      ;Since char & attr in each pos
     mov  si,ax 	      ;in si
     mov  di,0		      ;Move to start of screen
     pop  ax		      ;Get chars to move
     mul  rows
     mov  cx,ax 	      ;to cx
; Move the rows.
     mov  ax,video_seg	     ;Get Video segment to es
     mov  es,ax
     mov  ds,ax 	      ;and ds
     rep  movsw 	      ;Move the data (scroll screen)
; Clear the rest of the screen.
     pop  ax		      ;Get chars/row & rows to clear
     pop  bx
     mul  bx		      ;Get chars to do
     mov  cx,ax
     mov  ax,0620h	      ;Clear them
     rep  stosw
; Return to C.
     ret		      ;and return to C
scroll	   endp


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
	mov	dacbox+0[di],bh 	; save the red value
	mov	dacbox+1[di],bl 	;  and the green value
	mov	dacbox+2[di],ch 	;  and the blue value
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
	mov	bh,dacbox+0[di] 	; load up the VGA red value
	mov	bl,dacbox+1[di] 	;  and the green value
	mov	ch,dacbox+2[di] 	;  and the blue value
	call	dactopalette		; convert it to an EGA palette
	mov	palettega[si],cl	; save as a single palette register
	inc	si			; bump up the registers
	add	di,3			;  ...
	cmp	si,16			; more to go?
	jne	dactopalloop		;  yup.
	mov	cl,palettega		; copy palette 0
	mov	palettega+16,cl 	;  to the overscan register
	ret				;  we done.
dactopal	endp


; *********************** Function storedac() ****************************

;	Function to Store the dacbox[][] array into VGA,
;	called from loaddac() durring initialization.

storedac	proc
	cmp	dotmode,19		; roll-your-own vodeo mode?
	jne	storedac_notyourown
	pushf				; save a few registers
	push	si
	push	di
	call	far ptr writevideopalette
	pop	di			; restore a few registers
	pop	si
	popf
	cmp	ax,-1			; palette-write handled yet?
	jne	storedac_return		; yup.
	
storedac_notyourown:
	push	es			; need ES == DS temporarily
	push	ds			;  ...
	pop	es			;  ...
	mov	ax,1012h		; get the old DAC values
	mov	bx,0			;  (assuming, of course, they exist)
	mov	cx,256			;  ...
	mov	dx,offset dacbox	;  ...
	int	10h			; do it.
	pop	es			;  ...
storedac_return:
	ret
storedac endp

; *********************** Function loaddac() ****************************

;	Function to Load the dacbox[][] array, if it can
;	(sets dacbox[0][0] to an invalid '255' if it can't)

loaddac proc
	cmp	dotmode,19		; roll-your-own video mode?
	jne	loaddac_notyourown
	pushf				; save a few registers
	push	si
	push	di
	call	far ptr readvideopalette
	pop	di			; restore a few registers
	pop	si
	popf
	cmp	ax,-1			; palette-write handled yet?
	je	loaddac_notyourown	; nope.
	jmp	loaddacdone		; yup.
	
loaddac_notyourown:
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
	cmp	dacbox,255		; did it work?	do we have a VGA?
	jne	loaddacdone		;  yup.
	cmp	colors,16		; are we using 16 or more colors?
	jb	loaddacdone		;  nope.  forget it.
	cmp	ydots,350		; 640x350 range?
	jb	loaddacdone		;  nope.  forget it.
	mov	bx,offset palettega	; make up a dummy palette
	mov	cx,3800h		; start with color 0 == black
loaddacega1:				; and	     color 8 == low-white
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
loaddac endp

; *************** Function spindac(direction, rstep) ********************

;	Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;	in "rstep" increments - or, if "direction" is 0, just replace it.

spindac proc	uses di si es, direction:word, rstep:word
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
	loop	stepDAC 		; and loop until done.

	cmp	dotmode,19		; roll-your-own video?
	jne	spin_notyourown		; nope
	pushf				; save a few items
	push	si
	push	di
	call	far ptr writevideopalette
	pop	di			; restore a few items
	pop	si
	popf
	cmp	ax,-1			; negative result?
	je	spindoit		; yup.  handle it locally.
	jmp	spindacreturn		; else we done.
spin_notyourown:

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
	je	spinega 		;  yup.  spin it that way.
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
	jne	nolearn 		;  nope.
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
	call	w8514pal

spindacreturn:
	ret
spindac endp
	end

