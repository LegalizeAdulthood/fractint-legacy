;
;	HELP (and other) strings that have been moved to a FARDATA
;	segment to relieve some of the pressure on the poor little
;	overloaded 64K DATA segment.


;			 required for compatibility if Turbo ASM
IFDEF ??Version
MASM51
QUIRKS
ENDIF


.model medium, c

; .fardata
.code

	public	helpmessagetitle
	public	helpmessageauthors
	public	helpmessagemenu
	public	helpmessagemain1, helpmessagemain2
	public	helpmessagecycling
	public	helpmessagemouse
	public	helpmessagecmdline1, helpmessagecmdline2
	public	helpmessagefractals1, helpmessagefractals2
	public	helpmessageformoreinfo
	public	helpmessageendtext
	public	helpmessagemoretext
	public	helpmessagevideo

	public	runningontarga

	public	helpmessage

helpmessagetitle	db	"FRACTINT    Version 9.3"
 db 13,10,0

helpmessageauthors	db	13,10
 db "------------------  Primary Authors (this changes over time)  -----------------",13,10
 db "Bert Tyler      - original author, coordinator, integer Mandelbrot/Julia",13,10
 db "                  routines, zoom and pan, video mode table, save-to-disk...",13,10
 db "                  Compuserve ID: [73477,433]   BIX ID: btyler",13,10
 db "Timothy Wegner  - superVGA support, restore-from-disk, color-cycling,",13,10
 db "                  new fractal types, configuration files, 3-D images ...",13,10
 db "                  Compuserve ID: [71320,675]",13,10
 db "------------------   A Partial List of Contributing Authors   -----------------",13,10
 db "Michael Abrash  - 360x480x256 mode",13,10
 db "Steve Bennett   - restore-from-disk logic",13,10
 db "John Bridges    - [73307,606] superVGA support, 360x480x256 mode",13,10
 db "Lee Crocker     - [73407,2030] Fast Newton algorithm, other features",13,10
 db "Mike Kaufman    - mouse support, other features",13,10
 db "Joe McLain      - [75066,1257] TARGA Support, color-map files",13,10
 db "Bret Mulvey     - plasma clouds",13,10
 db "Mark Peterson   - [70441,3353] periodicity logic, fast floating-point lib",13,10
 db "Kyle Powell     - [76704,12] 8514/A Support",13,10
 db "Matt Saucier    - [72371,3101] Printer Support",13,10
 db "Phil Wilson     - Distance Estimator fractal type",13,10
 db "Richard Wilton  - 'tweaked' VGA modes",13,10
 db 13,10
 db "Please press one of the Function Keys to select a video mode and begin an image",13,10
 db "(or press the 'h' key now or at any other time for help)"
 db 0

helpmessagemenu	db	13,10
 db 13,10,13,10
 db "The following help screens are available",13,10
 db 13,10
 db "1   - Commands available at the initial Credits Screen and main command level",13,10,13,10
 db "2   - Commands available at the Color-Cycling command level",13,10,13,10
 db "3   - Using FRACTINT with a mouse",13,10,13,10
 db "4   - The SSTOOLS.INI file and Command-Line arguments",13,10,13,10
 db "5   - Descriptions of currently available fractal types",13,10,13,10
 db "6   - List of Available Video Modes and the keys that select them",13,10
 db "      ( >>> and if you're at the initial Credits Screen, NO FRACTAL IMAGES",13,10
 db "      are going to get drawn UNTIL AFTER you select a video mode <<< )",13,10,13,10
 db "7   - Contacting the authors / obtaining the latest versions of FRACTINT",13,10,13,10
 db "Please press one of the above keys (or press ESCAPE to exit Help Mode)"
 db 0

helpmessagemain1	db	13,10
 db "The useful keys you can hit while this program is running (the commands marked",13,10
 db "                    with an '*' are also available at the credits screen) are:",13,10
 db 13,10
 db "* h or H or ?       HELP! (Enter help mode and display this screen)",13,10
 db "  PageUp, PageDown  Shrink or Expand the Zoom Box",13,10
 db "  Cursor Keys       Pan (Move) the Zoom Box across the screen",13,10
 db "  Ctrl-Cursor Keys  Fast-Pan the Zoom Box (may require an enhanced keyboard)",13,10
 db "  End or Enter      Redraw the Screen or area inside the Zoom Box",13,10
 db "* F1,F2,F3,F4...    Select a new Video Mode and THEN Redraw",13,10
 db "                    (see the Video-modes HELP screens for the full modes list)",13,10
 db "* 1 or 2 or g       Select Single-Pass, Dual-Pass, or Solid-Guessing mode",13,10
 db "  c or C or + or -  Enter Color-Cycling Mode (see Color-Cycling Help screen)",13,10
 db "  s or S            Save the current screen image to disk (restart with 'r')",13,10
 db "* r or R or 3 or o  Restart from a saved (or .GIF) file ('3' or 'o' for 3-D)",13,10
 db "* t or T            Select a new fractal type and parameters",13,10
 db 13,10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",13,10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagemain2	db	13,10
 db "The useful keys you can hit while this program is running (the commands marked",13,10
 db "                    with an '*' are also available at the credits screen) are:",13,10
 db 13,10
 db "  p or P            Print the screen (command-line options set printer type)",13,10
 db "  Spacebar          Mandelbrot/Julia Set toggle (read FRACTINT.DOC first)",13,10
 db "  < or >            Lower or Raise the Iteration Limit (display with Tab key)",13,10
 db "  b or B            Add the current fractal description to FRABATCH.BAT",13,10
 db "  o or O            toggles 'orbits' option on and off during image generation",13,10
 db "  Home              Redraw Previous screen (you can 'back out' recursively)",13,10
 db "  Tab               Display the current fractal image information",13,10
 db "  Control-Enter     'Zoom-out' - expands the image so that your current",13,10
 db "                    image is positioned inside the current zoom-box location.",13,10
 db "* Insert            Restart the program (at the credits screen)",13,10
 db "* d or D            Shell to DOS (type 'exit' at the DOS prompt to return)",13,10
 db "* Delete or Esc     Stop the program and return to MSDOS",13,10
 db 13,10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",13,10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagecycling	db	13,10
 db "Command Keys that are available in Color-Cycling mode (which is the mode you",13,10
 db "are in if you have hit the 'c' key and are now paused in Color-Cycling mode",13,10
 db "with a white overscan (border) area, or you hit the '+' or '-' keys and",13,10
 db "the the colors are now cycling on your screen).  Commands marked with an '*'",13,10
 db "are available only on VGA systems (the others also work on EGA systems).",13,10
 db 13,10
 db "  h or H or ?      HELP! (Enter help mode and display this screen)",13,10
 db "  + or -           (re)-set the direction of the color-cycling",13,10
 db "  Right/Left Arrow (re)-set the direction of the color-cycling (just like +/-)",13,10
 db "  Up/Down Arrow    SpeedUp/SlowDown the color cycling process",13,10
 db "  F1 thru F10      Select Short--Medium--Long (randomly-generated) color bands",13,10
 db "  1  thru 9        Cycle through 'nn' colors between screen updates (default=1)",13,10
 db "  Enter            Randomly (re)-select all new colors  [TRY THIS ONE!]",13,10
 db "  Spacebar         Pause until another key is hit (the overscan area is set",13,10
 db "                   to white as a visual indicator of a Color-Cycling pause)",13,10
 db "* SF1 thru AF10    Pause and re-set the Palette to one of 30 fixed sequences",13,10
 db "* r or g or b or   force a pause and Lower (lower case) or Raise (upper case)",13,10
 db "* R or G or B      the Red, Green, or Blue component of the fractal image",13,10
 db "  d or D or a or A pause and load the palette from DEFAULT.MAP or ALTERN.MAP",13,10
 db "  (any other key)  Exit Color-Cycling and return to main command level",13,10
 db 0

helpmessagemouse	db	13,10
 db "Using FRACTINT with a Mouse",13,10,13,10
 db "Left Button:   Brings up and sizes the Zoom Box.   While holding down the",13,10
 db "               left button, push the mouse forward to shrink the Zoom Box,",13,10
 db "               and pull it back to expand it.   Then let go of the button",13,10
 db "               and move the mouse around to 'pan' the Zoom Box (with no",13,10
 db "               buttons held down, you are in 'fast-pan' mode).",13,10
 db 13,10
 db "Right Button:  When the right button is held down, the 'panning' operation",13,10
 db "               switches from 'fast-pan' to 'slow-pan' mode, giving you",13,10
 db "               better control over the location of the Zoom Box.",13,10
 db 13,10
 db "Both Buttons:  (or the middle button, if you have three of them) Redraws",13,10
 db "               the area inside the Zoom Box over your full screen.",13,10
 db 13,10
 db "Zoom and Pan using the mouse typically consists of pushing in the left",13,10
 db "button, sizing the zoom box, letting go of the button, fast-panning to",13,10
 db "the general area, pushing in the right button and slow-panning to the",13,10
 db "exact area you want, and then (still holding down the right button) tapping",13,10
 db "the left button to perform the Zoom.",13,10
 db 0

helpmessagecmdline1	db	13,10
 db "If FRACTINT locates a file called 'SSTOOLS.INI' in the DOS PATH, it reads",13,10
 db "initialization variables from it.  These variables can also be initialized",13,10
 db "on the command-line as arguments, or from other files referenced on the",13,10
 db "command line using an '@' notation.  The variables currently available are:",13,10
 db 13,10
 db "@filename                  Read more cmd-line arguments from 'filename'",13,10
 db "                           (this option only available on the command-line)",13,10
 db "[filename=]filename        Start with this saved file (one saved by FRACTINT",13,10
 db "                           or a generic GIF file [treated as a plasma cloud])",13,10
 db "                           ('filename=' is mandatory inside '@' or .INI files)",13,10
 db "savename=filename          Save files using this name (instead of FRACT001)",13,10
 db "video=xxx                  Begin with this video mode (Example: Video=F2)",13,10
 db "                           See the video-modes HELP screen for a full list",13,10
 db "type=fractaltype           Perform this Fractal Type (Default = mandel)",13,10
 db "                           See the fractaltypes HELP screen for a full list",13,10
 db "params=xxx[/xxx[/xxx]]...  Begin with these extra Parameter values",13,10
 db "                           (Examples:  params=4   params=-0.480/0.626)",13,10
 db "corners=xmin/xmax/ymin/ymax  Begin with these X, Y Coordinates",13,10
 db "                           (Example: corners=-0.739/-0.736/0.288/0.291)",13,10
 db 0

helpmessagecmdline2	db	13,10
 db "The initialization variables available, and their formats are:",13,10
 db 13,10
 db "passes=x (x = 1, 2, or g)  Select Single-Pass, Dual-Pass, or Solid-Guessing",13,10
 db "potential=nn[/nn[/nn]]     Continuous Potential options (see FRACTINT.DOC)",13,10
 db "3d=[nn[/nn[/nn]]]...       Generate 'filename' (above) as a 3D image using",13,10
 db "                           'nn/nn...' as default answers to the 3D prompts",13,10
 db "maxiter=nnn                Maximum number of iterations (default = 150)",13,10
 db "iterincr=nnn               Iteration inc/decrement stepsize (default = 50)",13,10
 db "inside=nnn                 Mandelbrot Interior color (inside=0 for black)",13,10
 db "map=filename               (VGA or TARGA) get the color map from 'filename'",13,10
 db "warn=yes                   Tells FRACTINT to avoid over-writing existing files",13,10
 db "batch=yes                  Batch mode run (display image, save-to-disk, exit)",13,10
 db "batch=config               Batch mode run to generate a 'fractint.cfg' file",13,10
 db "cyclelimit=nnn             color-cycler speed-limit (1 to 256, default = 55)",13,10
 db "printer=type[/res[/lpt#]]  Set the printer type (HP-Laserjet, IBM, Epson),",13,10
 db "                           dots/inch, and port# (1-3 for LPTn, 11-14 for COMn)",13,10
 db "    ;                      indicates the rest of the line is a comment",13,10
 db "                           (IE, 'fractint type=plasma ; use plasma clouds')",13,10
 db "sound=off                  (nobody ever plays with fractals at work, do they?)",13,10
 db 0

helpmessagefractals1	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "mandel     = 'Classic' Mandelbrot fractals using 32-bit integer math for speed.",13,10
 db "             z(0) = 0; z(n+1) = z(n)**2 + C, where C = Xcoord + i * Ycoord.",13,10
 db "             Two optional params: real and imaginary parts of z(0) (if not 0).",13,10
 db "julia      = 'Classic' Julia set fractals using 32-bit integer math for speed.",13,10
 db "             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)**2 + C.",13,10
 db "             Two params required: real and imaginary parts of C.",13,10
 db "mandelfp,  = 'Classic' Mandelbrot and Julia set fractals using traditional",13,10
 db "juliafp      floating point math.  Params identical to above.  Included",13,10
 db "             mostly for historical purposes.",13,10
 db "newton,    = Newton Domains-of-attraction (only the coloring schemes are",13,10
 db "newtbasin    different).  First param:  the power (from 3 to 10) of the eqn.",13,10
 db "             If param=4, the eqn is z(n+1) = (4*z(n)**3+1)/(3*z(n)**4).",13,10
 db "             Other Parameters invoke an 'Inversion' option if selected:",13,10
 db "             the Radius and X/Y location of the Inversion point",13,10
 db "plasma     = plasma clouds - random, cloud-like formations.  Requires four or",13,10
 db "             more colors.  One param: 'graininess' (.5 to 50, default = 2)",13,10
 db 0

helpmessagefractals2	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "lambdasine = lambda-sine fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * sine(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "lambdacos  = lambda-cosine  fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * cosine(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "lambdaexp  = lambda-exponent fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * exp(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "test       = 'test' point letting us (and you!) easily add fractal types.",13,10
 db "             Currently, the 'Distance Estimator' M'brot/Julia Set algorithm.",13,10
 db "             two optional parameters - if none given, uses the M'brot Set",13,10
 db "             If given, they are the the same as the Julia Set parameters.",13,10
 db 13,10
 db " (Watch This Space for More Fractal Types)",13,10
 db 0

helpmessageformoreinfo	db	13,10
 db 13,10
 db "Virtually all of the FRACTINT authors can be found on the Compuserve",13,10
 db "network in the PICS S 16 forum (called the 'Artists Studio' section).",13,10
 db "Several of us can also be found on BIX in the GRAPHIC.DISP/FRACTALS area.",13,10
 db 13,10
 db "In addition, several of the authors have agreed to the listing of their",13,10
 db "home addresses at the end of the FRACTINT.DOC file.",13,10
 db 13,10
 db "New versions of FRACTINT are uploaded (as self-extracting archive files)",13,10
 db "to the Compuserve and BIX networks, and make their way to other systems",13,10
 db "from those points.  The latest version of the program can usually be found",13,10
 db "in the following locations:",13,10
 db 13,10
 db "FRAINT.EXE - (Executable/Docs)  Compuserve: PICS DL 16 and IBMNEW DL 5",13,10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",13,10
 db 13,10
 db "FRASRC.EXE - (Complete Source)  Compuserve: PICS DL 16 and IBMPRO DL 3",13,10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",13,10
 db 13,10
 db "(What's the latest version?  Well, THIS one was, way back when we uploaded it!)",13,10
 db 0

helpmessagemoretext	db	13,10
 db ">>MORE HELP AVAILABLE<< -- Press the ENTER key to see another screen"
 db 0;

helpmessageendtext	db	13,10
 db "Press ESCAPE to exit Help mode, or 'h' to see the help menu. ",13,10
 db "Pressing any other key passes that keypress back to your program. "
 db 0;

helpmessagevideo	db	" "
 db "   (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) ",13,10
 db "    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)",13,10
 db 13,10
 db ">>MORE HELP AVAILABLE<< -- Press the ENTER key to see another screen",13,10
 db "Press ESCAPE to exit Help mode, or 'h' to see the help menu.",13,10
 db "Pressing any other key passes that keypress back to your program. "
 db 0

runningontarga		db	13,10
 db "                FRACTINT Running On TrueVision TARGA Card"
 db 13,10,0

.code

helpmessage proc uses ds es di, message:far ptr byte
		les 	di, message
		sub 	ax, ax
		mov 	cx, 2050		; max chars
		cld
		repnz	scasb			; find 0 byte
		mov 	ax, 2049
		sub 	ax, cx

		mov 	cx, ax			; need cx=length for DOS
		mov 	ah, 40h 		; DOS write to file
		mov 	bx, 1			; stdout
		lds 	dx, message
		int 	21h

		xor 	ax, ax
		adc 	ax, 0			; return 1 if DOS failed
		ret
helpmessage endp

		end
