;
;	The Video Table has been moved to a FARDATA segment to relieve
;	some of the pressure on the poor little	overloaded 64K DATA segment.

;	This code has three entry points:
;
;	sizeoftable = initvideotable();		returns the # of table entries
;
;	fromvideotable(i);			copies table entry #i into
;						the videoentry structure
;
;	tovideotable(i);			moves the videoentry structure
;						into the table at entry #i


;			 required for compatibility if Turbo ASM
IFDEF ??Version
MASM51
QUIRKS
ENDIF


.model medium, c

extrn	videoentry:byte

.data

.code

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
;		8) Reserved for future use (Super-VGA)
;		9) TARGA video modes
;		10) Reserved for Future use (HERCULES)
;		11) Non-Video [disk or RAM] "video"
;		12) 8514/A video modes
;		13) CGA 320x200x4-color and 640x200x2-color modes
;		14) Reserved for Tandy 1000 video modes
;		15) SuperVGA 256-Color mode using the Trident Chipset
;		16) SuperVGA 256-Color mode using the Chips & Tech Chipset
;		17) SuperVGA 256-Color mode using the ATI VGA Wonder Chipset

;               |--Adapter/Mode-Name------|-------Comments-----------|

;		|------INT 10H------|Dot-|--Resolution---|
;		|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 
	db	"IBM Low-Rez EGA           Quick but chunky          "
	dw	  0dh,   0,   0,   0,   2, 320, 200,  16
	db	"IBM 16-Color EGA          Slower but lots nicer     "
	dw	  10h,   0,   0,   0,   2, 640, 350,  16
	db	"IBM 256-Color MCGA        Quick and LOTS of colors  "
	dw	  13h,   0,   0,   0,   3, 320, 200, 256
	db	"IBM 16-Color VGA          Nice high resolution      "
	dw	  12h,   0,   0,   0,   2, 640, 480,  16
	db	"IBM 4-Color CGA           (Ugh - Yuck - Bleah)      "
	dw	   4h,   0,   0,   0,  13, 320, 200,   4
	db	"IBM Hi-Rez B&W CGA        ('Hi-Rez' Ugh - Yuck)     "
	dw	   6h,   0,   0,   0,  13, 640, 200,   2
	db	"IBM B&W EGA               (Monochrome EGA)          "
	dw	  0fh,   0,   0,   0,   2, 640, 350,   2
	db	"IBM B&W VGA               (Monochrome VGA)          "
	dw	  11h,   0,   0,   0,   2, 640, 480,   2
	db	"IBM Med-Rez EGA           (Silly but it's there!)   "
	dw	  0eh,   0,   0,   0,   2, 640, 200,  16
	db	"IBM VGA (non-std/no text) Register Compatibles ONLY "
	dw	   0h,   0,   0,   8,   7, 360, 480, 256
	db	"8514/A Low  Res           Requires IBM's HDIDLOAD   "
	dw	   3h,   0,   0,   1,  12, 640, 480, 256
	db	"8514/A High Res           Requires IBM's HDIDLOAD   "
	dw	   3h,   0,   0,   1,  12,1024, 768, 256
	db	"8514/A Low  W/Border      Requires IBM's HDIDLOAD   "
	dw	   3h,   0,   0,   1,  12, 632, 474, 256
	db	"8514/A High W/Border      Requires IBM's HDIDLOAD   "
	dw	   3h,   0,   0,   1,  12,1016, 762, 256
	db	"VESA Standard interface   UNTESTED: may not work    "
	dw	  6ah,   0,   0,   0,   2, 800, 600,  16
	db	"COMPAQ Portable 386       OK: Michael Kaufman       "
	dw	  40h,   0,   0,   0,   1, 640, 400,   2
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 60h,   0,   0,   2, 752, 410,  16
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 61h,   0,   0,   2, 720, 540,  16
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 62h,   0,   0,   2, 800, 600,  16
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 63h,   0,   0,   1,1024, 768,   2
	db	"Video-7 Vram VGA          OK: Ira Emus              "
	dw	6f05h, 64h,   0,   0,   1,1024, 768,   4
	db	"Video-7 Vram VGA w/512K   OK: Ira Emus              "
	dw	6f05h, 65h,   0,   0,   1,1024, 768,  16
	db	"Video-7 Vram VGA          OK: Michael Kaufman       "
	dw	6f05h, 66h,   0,   0,   6, 640, 400, 256
	db	"Video-7  w/512K           OK: Greg Reznick          "
	dw	6f05h, 67h,   0,   0,   6, 640, 480, 256
	db	"Video-7  w/512K           OK: Greg Reznick          "
	dw	6f05h, 68h,   0,   0,   6, 720, 540, 256
	db	"Video-7  w/512K           OK: Greg Reznick          "
	dw	6f05h, 69h,   0,   0,   6, 800, 600, 256
	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
	dw	  2eh,   0,   0,   0,   4, 640, 480, 256
	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
	dw	  29h,   0,   0,   0,   2, 800, 600,  16
	db	"Orchid/STB/GENOA/SIGMA    OK: Monte Davis           "
	dw	  30h,   0,   0,   0,   4, 800, 600, 256
	db	"Orchid/STB/GENOA/SIGMA    OK: David Mills           "
	dw	  37h,   0,   0,   0,   1,1024, 768,  16
	db	"GENOA/STB                 OK: Timothy Wegner        "
	dw	  2dh,   0,   0,   0,   4, 640, 350, 256
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  27h,   0,   0,   0,   2, 720, 512,  16
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  2fh,   0,   0,   0,   4, 720, 512, 256
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  7ch,   0,   0,   0,   2, 512, 512,  16
	db	"GENOA                     OK: Timothy Wegner        "
	dw	  7dh,   0,   0,   0,   4, 512, 512, 256
	db	"STB                       UNTESTED: may not work    "
	dw	  36h,   0,   0,   0,   1, 960, 720,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h,   0,   0,   0,   2, 640, 480,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h,  1h,   0,   0,   2, 752, 410,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h,  2h,   0,   0,   2, 800, 600,  16
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 11h,   0,   0,   1,1280, 350,   4
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 12h,   0,   0,   1,1280, 600,   4
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 13h,   0,   0,   1, 640, 350, 256
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 14h,   0,   0,   1, 640, 400, 256
	db	"Everex EVGA               OK: Travis Harrison       "
	dw	  70h, 15h,   0,   0,   1, 512, 480, 256
	db	"ATI EGA Wonder            UNTESTED: may not work    "
	dw	  51h,   0,   0,   0,   1, 640, 480,  16
	db	"ATI EGA Wonder            UNTESTED: may not work    "
	dw	  52h,   0,   0,   0,   1, 800, 560,  16
	db	"ATI VGA Wonder            OK: Henry So              "
	dw	  54h,   0,   0,   0,   2, 800, 600,  16
	db	"ATI VGA Wonder            UNTESTED: may not work    "
	dw	  61h,   0,   0,   0,  17, 640, 400, 256
	db	"ATI VGA Wonder (512K)     UNTESTED: may not work    "
	dw	  62h,   0,   0,   0,  17, 640, 480, 256
	db	"ATI VGA Wonder (512K)     UNTESTED: may not work    "
	dw	  63h,   0,   0,   0,  17, 800, 600, 256
	db	"Paradise EGA-480          UNTESTED: may not work    "
	dw	  50h,   0,   0,   0,   1, 640, 480,  16
	db	"Pdise/AST/COMPAQ VGA      OK: Tom Devlin            "
	dw	  5eh,   0,   0,   0,   5, 640, 400, 256
	db	"Pdise/AST/COMPAQ VGA      OK: Phil Wilson           "
	dw	  5fh,   0,   0,   0,   5, 640, 480, 256
	db	"Pdise/AST/COMPAQ VGA      OK: by Chris Green        "
	dw	  58h,   0,   0,   0,   2, 800, 600,  16
	db	"Pdise/AST/COMPAQ VGA      OK: Phil Wilson           "
	dw	  59h,   0,   0,   0,   1, 800, 600,   2
	db	"Tandy 1000 16 Color CGA   OK: Tom Price             "
	dw	   9h,   0,   0,   0,   1, 320, 200,  16
	db	"Tandy 1000 4 Color hi-rez OK: Tom Price             "
	dw	  0ah,   0,   0,   0,   1, 640, 200,   4
	db	"AT&T 6300                 UNTESTED: may not work    "
	dw	  41h,   0,   0,   0,   1, 640, 200,  16
	db	"AT&T 6300                 OK: Michael Kaufman       "
	dw	  40h,   0,   0,   0,   1, 640, 400,   2
	db	"AT&T 6300                 OK: Colby Norton          "
	dw	  42h,   0,   0,   0,   1, 640, 400,  16
	db	"TARGA 256 Color video     OK: Bruce Goren           "
	dw	   0h,   0,   0,   0,   9, 512, 482, 256
	db	"TARGA 256 Color 35mm      OK: Bruce Goren           "
	dw	   0h,   0,   0,   0,   9, 512, 342, 256
	db	"TARGA 256 Color 4 x 5     OK: Bruce Goren           "
	dw	   0h,   0,   0,   0,   9, 512, 384, 256
	db	"TRIDENT Chipset           UNTESTED: may not work    "
	dw	  5bh,   0,   0,   0,   2, 800, 600,  16
	db	"TRIDENT Chipset           UNTESTED: may not work    "
	dw	  5ch,   0,   0,   0,  15, 640, 400, 256
	db	"TRIDENT Chipset           UNTESTED: may not work    "
	dw	  5dh,   0,   0,   0,  15, 640, 480, 256
	db	"TRIDENT Chipset           UNTESTED: may not work    "
	dw	  5eh,   0,   0,   0,  15, 800, 600, 256
	db	"TRIDENT Chipset           OK: Lew Ramsey Mode (0)   "
	dw	  5fh,   0,   0,   0,   1,1024, 768,  16
	db	"Chips & Tech Chipset      UNTESTED: may not work    "
	dw	  78h,   0,   0,   0,  16, 640, 400, 256
	db	"Chips & Tech Chipset      UNTESTED: may not work    "
	dw	  79h,   0,   0,   0,  16, 640, 480, 256
	db	"Chips & Tech Chipset      UNTESTED: may not work    "
	dw	  7bh,   0,   0,   0,  16, 800, 600, 256
	db	"Chips & Tech Chipset      UNTESTED: may not work    "
	dw	  72h,   0,   0,   0,   1,1024, 768,  16
	db	"Disk/RAM 'Video'          Full-Page Epson @  60DPI  "
	dw	   3h,   0,   0,   0,  11, 768, 480,   2
	db	"Disk/RAM 'Video'          Full-Page Epson @ 120DPI  "
	dw	   3h,   0,   0,   0,  11, 768, 960,   2
	db	"Disk/RAM 'Video'          Full-Page Epson @ 240DPI  "
	dw	   3h,   0,   0,   0,  11, 768,1920,   2
	db	"Disk/RAM 'Video'          Full-Page L-Jet @  75DPI  "
	dw	   3h,   0,   0,   0,  11, 800, 600,   2
	db	"Disk/RAM 'Video'          Full-Page L-Jet @ 150DPI  "
	dw	   3h,   0,   0,   0,  11,1600,1200,   2
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11, 320, 200, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11, 360, 480, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11, 640, 350, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11, 640, 480, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11, 800, 600, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11,1024, 768, 256
	db	"Disk/RAM 'Video'          For Background Fractals   "
	dw	   3h,   0,   0,   0,  11,2048,2048, 256
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   1,   2, 704, 528,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   2,   2, 720, 540,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   3,   2, 736, 552,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   4,   2, 752, 564,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   5,   2, 768, 576,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   6,   2, 784, 588,  16
	db	"IBM VGA (+tweaked+)       Register Compatibles ONLY "
	dw	   0h,   0,   0,   7,   2, 800, 600,  16
	db	"END                       Must be the END of list   "
	dw	   0h,   0,   0,   0,   0,   0,   0,  0

	db	680 dup(0)		; room for 10 more video modes

.code

initvideotable proc uses ds es di
	mov	di, offset videotable+1	; get the start of the video table
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

		end
